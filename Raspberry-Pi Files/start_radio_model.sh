#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RX_SOURCE="${RX_SOURCE:-RX_Demo_Sensors.cpp}"
RX_BINARY="${RX_BINARY:-RX_Demo_Sensors}"

RX_ADDRESS="${RX_ADDRESS:-3}"
EXPECTED_SENDER="${EXPECTED_SENDER:-1}"
CHANNEL="${CHANNEL:-10}"
FREQUENCY="${FREQUENCY:-434}"
MODULATION="${MODULATION:-1}"

PYTHON="$SCRIPT_DIR/.venv/bin/python"
PREDICTOR="$SCRIPT_DIR/predict_and_log.py"

LOG_DIR="$SCRIPT_DIR/prediction_logs"
RAW_LOG="$LOG_DIR/raw_and_converted_cc1101_packets.jsonl"
MODEL_INPUT_LOG="$LOG_DIR/converted_model_inputs.jsonl"
RADIO_LOG="$SCRIPT_DIR/radio_receiver.log"

mkdir -p "$LOG_DIR"

if [[ ! -x "$PYTHON" ]]; then
    echo "ERROR: .venv Python not found." >&2
    exit 1
fi

if [[ ! -f "$PREDICTOR" ]]; then
    echo "ERROR: predict_and_log.py not found." >&2
    exit 1
fi

if [[ ! -f "$SCRIPT_DIR/nutrient_stress_hat.pkl" ]]; then
    echo "ERROR: nutrient_stress_hat.pkl not found." >&2
    exit 1
fi

if [[ ! -f "$SCRIPT_DIR/cc1100_raspi.cpp" ]] ||
   [[ ! -f "$SCRIPT_DIR/cc1100_raspi.h" ]]; then
    echo "ERROR: cc1100_raspi.cpp/.h not found." >&2
    exit 1
fi

if [[ ! -x "$SCRIPT_DIR/$RX_BINARY" ]] ||
   [[ "$SCRIPT_DIR/$RX_SOURCE" -nt "$SCRIPT_DIR/$RX_BINARY" ]]; then
    echo "Compiling converted-value receiver..." >&2

    g++ \
        -std=c++17 \
        -O2 \
        -Wall \
        -Wextra \
        "$SCRIPT_DIR/$RX_SOURCE" \
        "$SCRIPT_DIR/cc1100_raspi.cpp" \
        -o "$SCRIPT_DIR/$RX_BINARY" \
        -lwiringPi
fi

echo "====================================================" >&2
echo " CC1101 → Nutrient HAT converted-value bridge" >&2
echo "====================================================" >&2
echo "Temperature: raw / 10    (235 -> 23.5)" >&2
echo "Humidity:    raw / 10    (552 -> 55.2)" >&2
echo "Soil pH:     raw / 100   (658 -> 6.58)" >&2
echo "Moisture:    direct" >&2
echo "Light:       direct raw full-spectrum value" >&2
echo "N/P/K:       direct" >&2
echo "Raw log:     $RAW_LOG" >&2
echo "Model inputs:$MODEL_INPUT_LOG" >&2
echo "HAT update:  DISABLED" >&2
echo "Press Ctrl+C to stop." >&2

sudo stdbuf -oL -eL "$SCRIPT_DIR/$RX_BINARY" \
    -a "$RX_ADDRESS" \
    -s "$EXPECTED_SENDER" \
    -c "$CHANNEL" \
    -f "$FREQUENCY" \
    -m "$MODULATION" \
    2> >(tee -a "$RADIO_LOG" >&2) \
| stdbuf -oL awk \
    -v raw_log="$RAW_LOG" \
    -v model_log="$MODEL_INPUT_LOG" '
        /^RAW_PACKET_JSON:/ {
            line = substr($0, length("RAW_PACKET_JSON:") + 1)
            print line >> raw_log
            fflush(raw_log)
            next
        }

        /^SENSOR_JSON:/ {
            line = substr($0, length("SENSOR_JSON:") + 1)
            print line >> model_log
            fflush(model_log)

            print line
            fflush()
            next
        }

        {
            print "[RX OUTPUT] " $0 > "/dev/stderr"
            fflush("/dev/stderr")
        }
    ' \
| "$PYTHON" "$PREDICTOR" --stdin
