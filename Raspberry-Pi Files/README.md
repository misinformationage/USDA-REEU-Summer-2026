# Raspberry Pi 4B — Smart Farming AI

This folder contains the Raspberry Pi side of the USDA REEU Summer 2026 Smart Farming project.

The Raspberry Pi receives sensor packets from the MSP430 through the CC1101 radio, converts the transmitted sensor values, sends them to the trained HAT stress model, logs predictions, and displays the latest results in a Flask dashboard.

## 1. Files already included here

Core runtime files recovered from the project:

- `RX_Demo_Sensors.cpp` — receives and decodes the CC1101 sensor packet.
- `start_radio_model.sh` — compiles the receiver and pipes converted sensor JSON to the HAT inference program.
- `smart_farming_system.sh` — starts/stops the complete receiver + model + dashboard system.
- `templates/index.html` — dashboard webpage.
- `tests/` — example Healthy, Moderate Stress, and High Stress sensor inputs.
- `training/Train_Two_HAT_Models_Raspberry_Pi.ipynb` — notebook used for HAT model training.
- `README_CONVERSIONS.md` — notes about raw-to-model sensor conversions.
- `README_SYSTEM_LAUNCHER.md` — notes about the complete system launcher.

Older bridge ZIPs and project reference PDFs are kept in `archive_versions/` and `docs/`.

## 2. Files you still need from the original Raspberry Pi

Copy these five files from the Raspberry Pi into this same folder:

```text
cc1100_raspi.cpp
cc1100_raspi.h
predict_and_log.py
nutrient_stress_hat.pkl
dashboard_app.py
```

Also create/copy `requirements.txt` from the original Raspberry Pi Python environment if possible:

```bash
source .venv/bin/activate
pip freeze > requirements.txt
```

Do not upload the `.venv` folder itself to GitHub.

## 3. Expected runtime layout

After adding the missing files, the important part of the folder should look like this:

```text
Raspberry-Pi Files/
├── RX_Demo_Sensors.cpp
├── cc1100_raspi.cpp
├── cc1100_raspi.h
├── start_radio_model.sh
├── smart_farming_system.sh
├── predict_and_log.py
├── nutrient_stress_hat.pkl
├── dashboard_app.py
├── requirements.txt
├── templates/
│   └── index.html
└── tests/
    ├── test_healthy.json
    ├── test_moderate.json
    └── test_high.json
```

## 4. Raspberry Pi setup

The project expects Linux on the Raspberry Pi, `g++`, the CC1101/WiringPi dependencies used by the receiver, and Python 3.

Create a virtual environment in the project folder:

```bash
cd "Raspberry-Pi Files"
python3 -m venv .venv
source .venv/bin/activate
```

If you copied the original `requirements.txt`:

```bash
pip install -r requirements.txt
```

Make the launch scripts executable:

```bash
chmod +x start_radio_model.sh
chmod +x smart_farming_system.sh
```

## 5. Run the complete system

With the MSP430 transmitting and the CC1101 connected to the Raspberry Pi:

```bash
./smart_farming_system.sh
```

This starts:

1. CC1101 receiver
2. sensor-value conversion bridge
3. HAT stress inference
4. prediction logging
5. Flask dashboard
6. Chromium automatically when a desktop session is available

Useful commands:

```bash
./smart_farming_system.sh status
./smart_farming_system.sh logs
./smart_farming_system.sh restart
./smart_farming_system.sh stop
```

To run without automatically opening a browser:

```bash
OPEN_BROWSER=0 ./smart_farming_system.sh start
```

## 6. Run only the radio/model pipeline

For testing without the dashboard:

```bash
./start_radio_model.sh
```

The script automatically compiles `RX_Demo_Sensors.cpp` together with `cc1100_raspi.cpp` when needed.

The final radio settings in the recovered script are:

```text
Raspberry Pi receiver address: 3
Expected MSP430 sender:        1
CC1101 channel:                10
Frequency:                     434 MHz
Modulation setting:            1
```

These can also be overridden with environment variables if required.

## 7. Sensor conversions

Before inference, the receiver converts the transmitted integer values as follows:

- ambient temperature: raw / 10
- soil temperature: raw / 10
- humidity: raw / 10
- soil pH: raw / 100
- soil moisture: direct
- light intensity: direct
- nitrogen: direct
- phosphorus: direct
- potassium: direct

Example:

```text
Temperature 235 -> 23.5
Humidity    552 -> 55.2
pH          658 -> 6.58
```

## 8. Generated files

While running, the system creates runtime data such as:

```text
prediction_logs/hat_predictions.csv
prediction_logs/raw_and_converted_cc1101_packets.jsonl
prediction_logs/converted_model_inputs.jsonl
system_logs/
.runtime/
radio_receiver.log
```

These are generated files and normally should not be committed to GitHub.

## 9. Suggested `.gitignore`

```gitignore
.venv/
__pycache__/
*.pyc

.runtime/
system_logs/
prediction_logs/
radio_receiver.log

RX_Demo_Sensors
```

## 10. Important note about the HAT model

The recovered final launcher uses `nutrient_stress_hat.pkl` in inference-only mode. The runtime does not call `learn_one()` automatically. Model updates should only be performed when a trustworthy ground-truth plant-health label is available.
