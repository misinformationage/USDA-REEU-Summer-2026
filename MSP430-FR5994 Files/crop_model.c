#include "crop_model.h"

#include <stddef.h>

/*
 * Decision Tree representation exported from scikit-learn.
 *
 * Internal nodes:
 *   tree_feature[node] >= 0
 *
 * Leaf nodes:
 *   tree_feature[node] == -2
 */

static const int8_t tree_feature[
    CROP_MODEL_NUM_NODES
] =
{
    5, 7, -2, 9, -2, 7, 3, 9, 7, -2, 9, 4,
    -2, 7, -2, -2, 4, -2, 8, -2, -2, 4, -2, -2,
    9, 10, -2, -2, -2, 4, 5, -2, 9, -2, 9, -2,
    -2, 7, 9, -2, 4, 3, 9, 5, -2, 8, -2, -2,
    -2, 5, -2, -2, -2, -2, 5, -2, 7, -2, -2
};

static const float tree_threshold[
    CROP_MODEL_NUM_NODES
] =
{
    65.0f, 27.685081481933594f, -2.0f, 30.393478393554688f,
    -2.0f, 70.81499481201172f, 59.5f, 82.10353088378906f,
    60.01599884033203f, -2.0f, 57.67864227294922f, 52.0f,
    -2.0f, 60.2315788269043f, -2.0f, -2.0f,
    53.5f, -2.0f, 7.754196643829346f, -2.0f,
    -2.0f, 47.5f, -2.0f, -2.0f,
    112.45397186279297f, 10.691497802734375f, -2.0f, -2.0f,
    -2.0f, 32.5f, 20.0f, -2.0f,
    81.1391372680664f, -2.0f, 121.78255462646484f, -2.0f,
    -2.0f, 90.01739501953125f, 60.26306915283203f, -2.0f,
    65.0f, 99.5f, 199.9622802734375f, 29.5f,
    -2.0f, 6.001246929168701f, -2.0f, -2.0f,
    -2.0f, 30.5f, -2.0f, -2.0f,
    -2.0f, -2.0f, 140.0f, -2.0f,
    87.004638671875f, -2.0f, -2.0f
};

static const int16_t tree_children_left[
    CROP_MODEL_NUM_NODES
] =
{
    1, 2, -1, 4, -1, 6, 7, 8, 9, -1,
    11, 12, -1, 14, -1, -1, 17, -1, 19, -1,
    -1, 22, -1, -1, 25, 26, -1, -1, -1, 30,
    31, -1, 33, -1, 35, -1, -1, 38, 39, -1,
    41, 42, 43, 44, -1, 46, -1, -1, -1, 50,
    -1, -1, -1, -1, 55, -1, 57, -1, -1
};

static const int16_t tree_children_right[
    CROP_MODEL_NUM_NODES
] =
{
    54, 3, -1, 5, -1, 29, 24, 21, 10, -1,
    16, 13, -1, 15, -1, -1, 18, -1, 20, -1,
    -1, 23, -1, -1, 28, 27, -1, -1, -1, 37,
    32, -1, 34, -1, 36, -1, -1, 53, 40, -1,
    52, 49, 48, 45, -1, 47, -1, -1, -1, 51,
    -1, -1, -1, -1, 56, -1, 58, -1, -1
};

static const uint8_t tree_class[
    CROP_MODEL_NUM_NODES
] =
{
    0u, 1u, 9u, 1u, 15u, 1u, 2u, 10u, 10u, 13u, 10u, 10u,
    13u, 10u, 10u, 10u, 2u, 13u, 2u, 2u, 2u, 12u, 12u, 18u,
    5u, 11u, 11u, 11u, 5u, 1u, 4u, 16u, 4u, 21u, 4u, 19u,
    4u, 1u, 1u, 14u, 1u, 6u, 20u, 8u, 11u, 8u, 20u, 8u,
    20u, 6u, 6u, 8u, 1u, 17u, 0u, 3u, 0u, 7u, 0u
};

/*
 * Reproduce the exact preprocessing used by the
 * scikit-learn ColumnTransformer.
 *
 * No 13-element temporary feature array is created.
 * This saves RAM on the MSP430.
 */
static float crop_model_get_feature(
    uint8_t feature_index,
    const crop_model_input_t *input
)
{
    switch (feature_index)
    {

        case 0u:
            return (
                input->soil_type
                == CROP_SOIL_CLAY
            ) ? 1.0f : 0.0f;

        case 1u:
            return (
                input->soil_type
                == CROP_SOIL_LOAMY
            ) ? 1.0f : 0.0f;

        case 2u:
            return (
                input->soil_type
                == CROP_SOIL_SANDY
            ) ? 1.0f : 0.0f;

        case 3u:
            return input->nitrogen;

        case 4u:
            return input->P;

        case 5u:
            return input->K;

        case 6u:
            return input->temperature;

        case 7u:
            return input->humidity;

        case 8u:
            return input->ph;

        case 9u:
            return input->rainfall;

        case 10u:
            return input->soil_moisture;

        case 11u:
            return input->sunlight_exposure;

        case 12u:
            return input->wind_speed;

        default:
            return 0.0f;
    }
}

uint8_t crop_model_predict(
    const crop_model_input_t *input
)
{
    int16_t node = 0;
    uint8_t steps = 0u;

    if (input == NULL)
    {
        return CROP_MODEL_INVALID_CLASS;
    }

    if (
        input->soil_type < CROP_SOIL_SANDY
        || input->soil_type > CROP_SOIL_CLAY
    )
    {
        return CROP_MODEL_INVALID_CLASS;
    }

    while (
        node >= 0
        && node < (int16_t)CROP_MODEL_NUM_NODES
        && steps <= CROP_MODEL_MAX_DEPTH
    )
    {
        int8_t feature_index =
            tree_feature[node];

        /*
         * scikit-learn marks leaves with feature = -2.
         */
        if (feature_index < 0)
        {
            return tree_class[node];
        }

        if (
            crop_model_get_feature(
                (uint8_t)feature_index,
                input
            )
            <= tree_threshold[node]
        )
        {
            node = tree_children_left[node];
        }
        else
        {
            node = tree_children_right[node];
        }

        steps++;
    }

    return CROP_MODEL_INVALID_CLASS;
}

const char *crop_model_class_name(
    uint8_t class_id
)
{
    switch (class_id)
    {
        case 0u: return "apple";
        case 1u: return "banana";
        case 2u: return "blackgram";
        case 3u: return "chickpea";
        case 4u: return "coconut";
        case 5u: return "coffee";
        case 6u: return "cotton";
        case 7u: return "grapes";
        case 8u: return "jute";
        case 9u: return "kidneybeans";
        case 10u: return "lentil";
        case 11u: return "maize";
        case 12u: return "mango";
        case 13u: return "mothbeans";
        case 14u: return "mungbean";
        case 15u: return "muskmelon";
        case 16u: return "orange";
        case 17u: return "papaya";
        case 18u: return "pigeonpeas";
        case 19u: return "pomegranate";
        case 20u: return "rice";
        case 21u: return "watermelon";

        default:
            return "invalid";
    }
}
