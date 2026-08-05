#ifndef CROP_MODEL_H
#define CROP_MODEL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CROP_MODEL_NUM_NODES       59u
#define CROP_MODEL_NUM_CLASSES     22u
#define CROP_MODEL_NUM_FEATURES    13u
#define CROP_MODEL_MAX_DEPTH       12u
#define CROP_MODEL_INVALID_CLASS   255u

/*
 * soil_type values expected by crop_model_predict().
 *
 * These values match the original dataset coding:
 * 1 = sandy
 * 2 = loamy
 * 3 = clay
 */
typedef enum
{
    CROP_SOIL_SANDY = 1u,
    CROP_SOIL_LOAMY = 2u,
    CROP_SOIL_CLAY  = 3u
} crop_soil_type_t;

/*
 * Output class numbers.
 *
 * crop_model_predict() returns one of these values.
 */
typedef enum
{
    CROP_CLASS_APPLE = 0u,
    CROP_CLASS_BANANA = 1u,
    CROP_CLASS_BLACKGRAM = 2u,
    CROP_CLASS_CHICKPEA = 3u,
    CROP_CLASS_COCONUT = 4u,
    CROP_CLASS_COFFEE = 5u,
    CROP_CLASS_COTTON = 6u,
    CROP_CLASS_GRAPES = 7u,
    CROP_CLASS_JUTE = 8u,
    CROP_CLASS_KIDNEYBEANS = 9u,
    CROP_CLASS_LENTIL = 10u,
    CROP_CLASS_MAIZE = 11u,
    CROP_CLASS_MANGO = 12u,
    CROP_CLASS_MOTHBEANS = 13u,
    CROP_CLASS_MUNGBEAN = 14u,
    CROP_CLASS_MUSKMELON = 15u,
    CROP_CLASS_ORANGE = 16u,
    CROP_CLASS_PAPAYA = 17u,
    CROP_CLASS_PIGEONPEAS = 18u,
    CROP_CLASS_POMEGRANATE = 19u,
    CROP_CLASS_RICE = 20u,
    CROP_CLASS_WATERMELON = 21u
} crop_class_t;

/*
 * Original model inputs.
 *
 * soil_type is categorical.
 * All other fields are numeric sensor values.
 */
typedef struct
{
    float nitrogen;
    float P;
    float K;
    float temperature;
    float humidity;
    float ph;
    float rainfall;
    float soil_moisture;
    float sunlight_exposure;
    float wind_speed;
    uint8_t soil_type;
} crop_model_input_t;

/*
 * Run one crop recommendation.
 *
 * Returns:
 *   0 to 21 = predicted crop class
 *   255 = invalid input or tree traversal error
 */
uint8_t crop_model_predict(
    const crop_model_input_t *input
);

/*
 * Convert a crop class number into a readable name.
 *
 * This function is useful for testing and debugging.
 * The MSP430 may transmit only the numeric class ID
 * if the strings are not required on the device.
 */
const char *crop_model_class_name(
    uint8_t class_id
);

#ifdef __cplusplus
}
#endif

#endif
