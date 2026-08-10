# CC1101 → HAT Bridge with Value Conversion

This revision converts scaled integers into the floating-point values expected
by the nutrient HAT while preserving all raw values.

## Conversions

| Payload field | Raw example | HAT value |
|---|---:|---:|
| Ambient temperature | 235 | 23.5 |
| Soil temperature | 235 | 23.5 |
| Humidity | 552 | 55.2 |
| Soil pH | 658 | 6.58 |
| Soil moisture | 27 | 27.0 |
| Light intensity | 556 | 556.0 |
| Nitrogen | 10 | 10.0 |
| Phosphorus | 45 | 45.0 |
| Potassium | 39 | 39.0 |

Temperature and humidity are divided by 10 because the MSP430/DHT22 values
are stored in tenths. pH is divided by 100 because the transmitter sends the
raw Modbus pH value in hundredths.

The two transmitted temperature fields are both preserved and converted
separately.

## Replace files

Replace the older files in the Raspberry Pi project folder:

- `RX_Demo_Sensors.cpp`
- `start_radio_model.sh`

Then run:

```bash
chmod +x start_radio_model.sh
./start_radio_model.sh
```

## Logs

Raw and converted values:

```text
prediction_logs/raw_and_converted_cc1101_packets.jsonl
```

Exact converted model inputs:

```text
prediction_logs/converted_model_inputs.jsonl
```

HAT results:

```text
prediction_logs/hat_predictions.csv
```

The HAT remains inference-only. No `learn_one()` call is made.
