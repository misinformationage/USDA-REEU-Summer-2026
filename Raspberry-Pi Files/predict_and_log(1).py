#!/usr/bin/env python3
"""
Inference-only runner for the nutrient-inclusive HAT model.

This program:
1. Loads nutrient_stress_hat.pkl
2. Accepts one sensor reading
3. Predicts Healthy / Moderate Stress / High Stress
4. Appends the reading, prediction, and probabilities to a CSV log

IMPORTANT:
- This program never calls learn_one().
- This program never modifies or overwrites the model file.
- Predictions are marked as unverified.
"""

from __future__ import annotations

import argparse
import csv
import json
import pickle
import sys
import uuid
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable

FEATURES = [
    "Soil_Moisture",
    "Ambient_Temperature",
    "Soil_Temperature",
    "Humidity",
    "Light_Intensity",
    "Soil_pH",
    "Nitrogen_Level",
    "Phosphorus_Level",
    "Potassium_Level",
]

CLASS_LABELS = [
    "Healthy",
    "Moderate Stress",
    "High Stress",
]

DEFAULT_MODEL = Path(__file__).resolve().parent / "nutrient_stress_hat.pkl"
DEFAULT_LOG = Path(__file__).resolve().parent / "prediction_logs" / "hat_predictions.csv"


def load_model_read_only(model_path: Path):
    """Load a trusted pickle file for prediction only."""
    if not model_path.is_file():
        raise FileNotFoundError(
            f"Model file not found: {model_path}\n"
            "Copy nutrient_stress_hat.pkl into this folder or pass --model PATH."
        )

    # Opened read-only. This script never writes to the model file.
    with model_path.open("rb") as model_file:
        return pickle.load(model_file)


def prepare_reading(raw: dict[str, Any]) -> dict[str, float]:
    """Validate feature names and convert all values to floats."""
    missing = [feature for feature in FEATURES if feature not in raw]
    if missing:
        raise ValueError(f"Missing required features: {missing}")

    reading: dict[str, float] = {}

    for feature in FEATURES:
        value = float(raw[feature])

        if value != value or value in (float("inf"), float("-inf")):
            raise ValueError(f"{feature} must be a finite number.")

        reading[feature] = value

    return reading


def predict_only(model, reading: dict[str, float]) -> tuple[str, dict[str, float]]:
    """
    Make a prediction without training or updating the model.
    There is intentionally no model.learn_one() call in this program.
    """
    prediction = model.predict_one(reading)
    probabilities_raw = model.predict_proba_one(reading)

    if prediction is None:
        raise RuntimeError("The loaded model returned no prediction.")

    probabilities = {
        label: float(probabilities_raw.get(label, 0.0))
        for label in CLASS_LABELS
    }

    return str(prediction), probabilities


def append_prediction_log(
    log_path: Path,
    reading: dict[str, float],
    prediction: str,
    probabilities: dict[str, float],
) -> str:
    """Append one unverified prediction to a CSV file."""
    log_path.parent.mkdir(parents=True, exist_ok=True)

    record_id = str(uuid.uuid4())
    timestamp = datetime.now().astimezone().isoformat(timespec="seconds")

    fieldnames = [
        "record_id",
        "timestamp_local",
        *FEATURES,
        "prediction",
        "probability_Healthy",
        "probability_Moderate_Stress",
        "probability_High_Stress",
        "verification_status",
        "verified_label",
    ]

    row = {
        "record_id": record_id,
        "timestamp_local": timestamp,
        **reading,
        "prediction": prediction,
        "probability_Healthy": probabilities["Healthy"],
        "probability_Moderate_Stress": probabilities["Moderate Stress"],
        "probability_High_Stress": probabilities["High Stress"],
        "verification_status": "UNVERIFIED",
        "verified_label": "",
    }

    write_header = not log_path.exists() or log_path.stat().st_size == 0

    with log_path.open("a", newline="", encoding="utf-8") as csv_file:
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        if write_header:
            writer.writeheader()
        writer.writerow(row)

    return record_id


def process_one(model, raw: dict[str, Any], log_path: Path) -> None:
    reading = prepare_reading(raw)
    prediction, probabilities = predict_only(model, reading)
    record_id = append_prediction_log(
        log_path,
        reading,
        prediction,
        probabilities,
    )

    print("\nPrediction saved")
    print("----------------")
    print(f"Record ID: {record_id}")
    print(f"Prediction: {prediction}")
    print("Probabilities:")
    for label in CLASS_LABELS:
        print(f"  {label}: {probabilities[label]:.4f}")
    print("Verification status: UNVERIFIED")
    print(f"CSV log: {log_path}")


def read_json_file(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise FileNotFoundError(f"JSON input file not found: {path}")

    with path.open("r", encoding="utf-8") as json_file:
        raw = json.load(json_file)

    if not isinstance(raw, dict):
        raise ValueError("The JSON file must contain one JSON object.")

    return raw


def iter_json_lines() -> Iterable[dict[str, Any]]:
    """
    Read one JSON object per line from standard input.
    Useful later when another program sends sensor records.
    """
    for line_number, line in enumerate(sys.stdin, start=1):
        stripped = line.strip()
        if not stripped:
            continue

        try:
            raw = json.loads(stripped)
        except json.JSONDecodeError as exc:
            print(
                f"Skipping invalid JSON on line {line_number}: {exc}",
                file=sys.stderr,
            )
            continue

        if not isinstance(raw, dict):
            print(
                f"Skipping line {line_number}: expected a JSON object.",
                file=sys.stderr,
            )
            continue

        yield raw


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Make and save HAT predictions without updating the model."
    )

    parser.add_argument(
        "--model",
        type=Path,
        default=DEFAULT_MODEL,
        help=f"Path to the trained pickle model. Default: {DEFAULT_MODEL}",
    )

    parser.add_argument(
        "--log",
        type=Path,
        default=DEFAULT_LOG,
        help=f"CSV prediction log. Default: {DEFAULT_LOG}",
    )

    input_group = parser.add_mutually_exclusive_group(required=True)
    input_group.add_argument(
        "--json",
        type=Path,
        help="Read one sensor observation from a JSON file.",
    )
    input_group.add_argument(
        "--stdin",
        action="store_true",
        help="Read newline-delimited JSON observations from standard input.",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        model = load_model_read_only(args.model)

        if args.json is not None:
            process_one(model, read_json_file(args.json), args.log)
        else:
            for raw in iter_json_lines():
                try:
                    process_one(model, raw, args.log)
                except Exception as exc:
                    print(f"Observation failed: {exc}", file=sys.stderr)

    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
