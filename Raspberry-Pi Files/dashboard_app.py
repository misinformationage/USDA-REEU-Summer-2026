#!/usr/bin/env python3
"""
Local conference dashboard for saved HAT predictions.

The dashboard only reads prediction_logs/hat_predictions.csv.
It does not load, train, or modify nutrient_stress_hat.pkl.
"""

from __future__ import annotations

import csv
from pathlib import Path
from typing import Any

from flask import Flask, jsonify, render_template

BASE_DIR = Path(__file__).resolve().parent
LOG_FILE = BASE_DIR / "prediction_logs" / "hat_predictions.csv"

FEATURES = [
    ("Soil_Moisture", "Soil Moisture"),
    ("Ambient_Temperature", "Ambient Temperature"),
    ("Soil_Temperature", "Soil Temperature"),
    ("Humidity", "Humidity"),
    ("Light_Intensity", "Light Intensity"),
    ("Soil_pH", "Soil pH"),
    ("Nitrogen_Level", "Nitrogen"),
    ("Phosphorus_Level", "Phosphorus"),
    ("Potassium_Level", "Potassium"),
]

PROBABILITY_FIELDS = {
    "Healthy": "probability_Healthy",
    "Moderate Stress": "probability_Moderate_Stress",
    "High Stress": "probability_High_Stress",
}

app = Flask(__name__)


def _as_float(value: Any, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def read_prediction_log() -> dict[str, Any]:
    """Read the latest prediction and recent history from the CSV log."""
    if not LOG_FILE.exists():
        return {
            "available": False,
            "message": f"No prediction log found at {LOG_FILE}",
            "latest": None,
            "recent": [],
            "count": 0,
        }

    with LOG_FILE.open("r", newline="", encoding="utf-8") as file:
        rows = list(csv.DictReader(file))

    if not rows:
        return {
            "available": False,
            "message": "The prediction log exists but is empty.",
            "latest": None,
            "recent": [],
            "count": 0,
        }

    latest_raw = rows[-1]
    probabilities = {
        label: _as_float(latest_raw.get(field))
        for label, field in PROBABILITY_FIELDS.items()
    }
    sensors = [
        {"key": key, "label": label, "value": latest_raw.get(key, "")}
        for key, label in FEATURES
    ]
    latest = {
        "record_id": latest_raw.get("record_id", ""),
        "timestamp": latest_raw.get("timestamp_local", ""),
        "prediction": latest_raw.get("prediction", "Unknown"),
        "verification_status": latest_raw.get("verification_status", "UNVERIFIED"),
        "probabilities": probabilities,
        "sensors": sensors,
    }

    recent = []
    for row in reversed(rows[-10:]):
        recent.append(
            {
                "timestamp": row.get("timestamp_local", ""),
                "prediction": row.get("prediction", "Unknown"),
                "healthy": _as_float(row.get("probability_Healthy")),
                "moderate": _as_float(row.get("probability_Moderate_Stress")),
                "high": _as_float(row.get("probability_High_Stress")),
                "status": row.get("verification_status", "UNVERIFIED"),
            }
        )

    return {
        "available": True,
        "message": "",
        "latest": latest,
        "recent": recent,
        "count": len(rows),
    }


@app.route("/")
def index():
    return render_template("index.html")


@app.route("/api/data")
def api_data():
    return jsonify(read_prediction_log())


if __name__ == "__main__":
    print(f"Reading predictions from: {LOG_FILE}")
    print("Dashboard: http://127.0.0.1:5000")
    app.run(host="0.0.0.0", port=5000, debug=False)
