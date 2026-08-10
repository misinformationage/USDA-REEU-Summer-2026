# Smart Farming Complete System Launcher

Copy `smart_farming_system.sh` into the same Raspberry Pi project folder that
contains:

- `.venv/`
- `nutrient_stress_hat.pkl`
- `predict_and_log.py`
- `dashboard_app.py`
- `start_radio_model.sh`
- `RX_Demo_Sensors.cpp`
- `cc1100_raspi.cpp`
- `cc1100_raspi.h`

Make it executable:

```bash
chmod +x smart_farming_system.sh
```

Start everything:

```bash
./smart_farming_system.sh
```

The script starts:

1. CC1101 receiver
2. Incoming-value conversion bridge
3. Nutrient HAT inference
4. Prediction logging
5. Flask dashboard
6. Chromium browser when a desktop session is available

Other commands:

```bash
./smart_farming_system.sh status
./smart_farming_system.sh logs
./smart_farming_system.sh restart
./smart_farming_system.sh stop
```

Start without opening the browser:

```bash
OPEN_BROWSER=0 ./smart_farming_system.sh start
```

The processes continue running after the start command returns. Use the `stop`
command to shut down the dashboard and radio/model pipeline.
