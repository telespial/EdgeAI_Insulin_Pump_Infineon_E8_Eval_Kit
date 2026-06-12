#!/usr/bin/env python3
"""Shared host-side Predictor V2 training/export helpers."""

from __future__ import annotations

import csv
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


FEATURE_NAMES: List[str] = [
    "current_glucose",
    "lag_1",
    "lag_2",
    "lag_3",
    "lag_6",
    "lag_9",
    "lag_12",
    "delta_1",
    "delta_3",
    "delta_6",
    "slope_15m",
    "slope_30m",
    "slope_60m",
    "rolling_mean_3",
    "rolling_mean_6",
    "rolling_mean_12",
    "rolling_variance_6",
    "rolling_stddev_6",
    "volatility_score",
    "iob",
    "cob",
    "basal_rate",
    "insulin_30m",
    "insulin_120m",
    "carbs_30m",
    "carbs_120m",
    "sqi",
    "cgm_age_s",
    "tod_sin",
    "tod_cos",
    "activity_state",
    "activity_confidence",
    "motion_rms_5m",
    "motion_rms_15m",
    "active_minutes",
    "post_exercise_minutes",
]

FEATURE_COUNT = len(FEATURE_NAMES)
HORIZON_SECONDS = {"15m": 15 * 60, "30m": 30 * 60, "60m": 60 * 60}
DEFAULT_SCALE = 1.0
SMOOTHING_RIDGE = 1e-3
MAX_VALID_GLUCOSE = 400.0
MIN_VALID_GLUCOSE = 40.0

OPTIONAL_NUMERIC_FIELDS = {
    "cgm_age_s": 0.0,
    "iob_u": 0.0,
    "cob_g": 0.0,
    "basal_u_per_hr": 0.0,
    "insulin_30m_u": 0.0,
    "insulin_120m_u": 0.0,
    "carbs_30m_g": 0.0,
    "carbs_120m_g": 0.0,
    "activity_confidence_pct": 0.0,
    "motion_rms_5m": 0.0,
    "motion_rms_15m": 0.0,
    "active_minutes": 0.0,
    "post_exercise_minutes": 0.0,
}

REQUIRED_FIELDS = ["timestamp", "glucose_mgdl", "sqi_pct"]


def _parse_float(value: object, default: float = 0.0) -> float:
    if value is None:
        return default
    if isinstance(value, (int, float)):
        if math.isnan(float(value)) or math.isinf(float(value)):
            return default
        return float(value)
    text = str(value).strip()
    if text == "":
        return default
    try:
        parsed = float(text)
    except ValueError:
        return default
    if math.isnan(parsed) or math.isinf(parsed):
        return default
    return parsed


def _parse_int(value: object, default: int = 0) -> int:
    return int(round(_parse_float(value, float(default))))


def _parse_boolish(value: object, default: bool = False) -> bool:
    if value is None:
        return default
    if isinstance(value, bool):
        return value
    text = str(value).strip().lower()
    if text in {"1", "true", "yes", "y", "on"}:
        return True
    if text in {"0", "false", "no", "n", "off"}:
        return False
    return default


def _parse_activity_state(value: object) -> int:
    if value is None:
        return 0
    if isinstance(value, (int, float)) and not math.isnan(float(value)):
        return max(0, min(5, int(round(float(value)))))
    text = str(value).strip().lower()
    if text == "":
        return 0
    mapping = {
        "unknown": 0,
        "sedentary": 1,
        "light": 2,
        "moderate": 3,
        "exercise": 4,
        "sleep": 5,
    }
    if text in mapping:
        return mapping[text]
    try:
        return max(0, min(5, int(round(float(text)))))
    except ValueError:
        return 0


@dataclass(frozen=True)
class Sample:
    series_id: str
    timestamp: int
    source_epoch_s: int
    glucose_mgdl: float
    sqi_pct: float
    cgm_age_s: float = 0.0
    iob_u: float = 0.0
    cob_g: float = 0.0
    basal_u_per_hr: float = 0.0
    insulin_30m_u: float = 0.0
    insulin_120m_u: float = 0.0
    carbs_30m_g: float = 0.0
    carbs_120m_g: float = 0.0
    activity_state: int = 0
    activity_confidence_pct: float = 0.0
    motion_rms_5m: float = 0.0
    motion_rms_15m: float = 0.0
    active_minutes: float = 0.0
    post_exercise_minutes: float = 0.0
    physiology_present: bool = False


def load_series_from_csv(path: Path) -> List[Sample]:
    rows: List[Sample] = []
    with path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        missing = [field for field in REQUIRED_FIELDS if field not in (reader.fieldnames or [])]
        if missing:
            raise ValueError(f"{path}: missing required columns: {', '.join(missing)}")
        for raw in reader:
            series_id = str(raw.get("series_id") or path.stem).strip() or path.stem
            timestamp = _parse_int(raw.get("timestamp"))
            source_epoch_s = _parse_int(raw.get("source_epoch_s"), timestamp)
            glucose_mgdl = _parse_float(raw.get("glucose_mgdl"), float("nan"))
            sqi_pct = _parse_float(raw.get("sqi_pct"), 0.0)
            if math.isnan(glucose_mgdl):
                raise ValueError(f"{path}: invalid glucose_mgdl value on timestamp {timestamp}")
            physiology_present = _parse_boolish(raw.get("physiology_present"), False)
            optional_values = {
                "cgm_age_s": _parse_float(raw.get("cgm_age_s"), OPTIONAL_NUMERIC_FIELDS["cgm_age_s"]),
                "iob_u": _parse_float(raw.get("iob_u"), OPTIONAL_NUMERIC_FIELDS["iob_u"]),
                "cob_g": _parse_float(raw.get("cob_g"), OPTIONAL_NUMERIC_FIELDS["cob_g"]),
                "basal_u_per_hr": _parse_float(raw.get("basal_u_per_hr"), OPTIONAL_NUMERIC_FIELDS["basal_u_per_hr"]),
                "insulin_30m_u": _parse_float(raw.get("insulin_30m_u"), OPTIONAL_NUMERIC_FIELDS["insulin_30m_u"]),
                "insulin_120m_u": _parse_float(raw.get("insulin_120m_u"), OPTIONAL_NUMERIC_FIELDS["insulin_120m_u"]),
                "carbs_30m_g": _parse_float(raw.get("carbs_30m_g"), OPTIONAL_NUMERIC_FIELDS["carbs_30m_g"]),
                "carbs_120m_g": _parse_float(raw.get("carbs_120m_g"), OPTIONAL_NUMERIC_FIELDS["carbs_120m_g"]),
                "activity_confidence_pct": _parse_float(raw.get("activity_confidence_pct"), OPTIONAL_NUMERIC_FIELDS["activity_confidence_pct"]),
                "motion_rms_5m": _parse_float(raw.get("motion_rms_5m"), OPTIONAL_NUMERIC_FIELDS["motion_rms_5m"]),
                "motion_rms_15m": _parse_float(raw.get("motion_rms_15m"), OPTIONAL_NUMERIC_FIELDS["motion_rms_15m"]),
                "active_minutes": _parse_float(raw.get("active_minutes"), OPTIONAL_NUMERIC_FIELDS["active_minutes"]),
                "post_exercise_minutes": _parse_float(raw.get("post_exercise_minutes"), OPTIONAL_NUMERIC_FIELDS["post_exercise_minutes"]),
            }
            if not physiology_present:
                physiology_present = any(
                    raw.get(field) not in (None, "")
                    for field in (
                        "iob_u",
                        "cob_g",
                        "basal_u_per_hr",
                        "insulin_30m_u",
                        "insulin_120m_u",
                        "carbs_30m_g",
                        "carbs_120m_g",
                        "activity_state",
                        "activity_confidence_pct",
                        "motion_rms_5m",
                        "motion_rms_15m",
                        "active_minutes",
                        "post_exercise_minutes",
                    )
                )
            rows.append(
                Sample(
                    series_id=series_id,
                    timestamp=timestamp,
                    source_epoch_s=source_epoch_s,
                    glucose_mgdl=glucose_mgdl,
                    sqi_pct=sqi_pct,
                    cgm_age_s=optional_values["cgm_age_s"],
                    iob_u=optional_values["iob_u"],
                    cob_g=optional_values["cob_g"],
                    basal_u_per_hr=optional_values["basal_u_per_hr"],
                    insulin_30m_u=optional_values["insulin_30m_u"],
                    insulin_120m_u=optional_values["insulin_120m_u"],
                    carbs_30m_g=optional_values["carbs_30m_g"],
                    carbs_120m_g=optional_values["carbs_120m_g"],
                    activity_state=_parse_activity_state(raw.get("activity_state")),
                    activity_confidence_pct=optional_values["activity_confidence_pct"],
                    motion_rms_5m=optional_values["motion_rms_5m"],
                    motion_rms_15m=optional_values["motion_rms_15m"],
                    active_minutes=optional_values["active_minutes"],
                    post_exercise_minutes=optional_values["post_exercise_minutes"],
                    physiology_present=physiology_present,
                )
            )
    rows.sort(key=lambda sample: (sample.series_id, sample.timestamp))
    return rows


def split_series(samples: Sequence[Sample], holdout_ratio: float) -> Tuple[List[Sample], List[Sample]]:
    if not samples:
        return [], []
    holdout_size = max(1, int(round(len(samples) * holdout_ratio)))
    if holdout_size >= len(samples):
        holdout_size = max(1, len(samples) // 4)
    split_index = len(samples) - holdout_size
    if split_index <= 0:
        split_index = len(samples) - 1
    return list(samples[:split_index]), list(samples[split_index:])


def group_samples(paths: Sequence[Path], holdout_ratio: float) -> Tuple[List[Sample], List[Sample], List[str]]:
    training_rows: List[Sample] = []
    holdout_rows: List[Sample] = []
    source_names: List[str] = []
    for path in paths:
        series = load_series_from_csv(path)
        if holdout_ratio <= 0.0:
            training_rows.extend(series)
        else:
            train_part, holdout_part = split_series(series, holdout_ratio)
            training_rows.extend(train_part)
            holdout_rows.extend(holdout_part)
        source_names.append(str(path))
    training_rows.sort(key=lambda sample: (sample.series_id, sample.timestamp))
    holdout_rows.sort(key=lambda sample: (sample.series_id, sample.timestamp))
    return training_rows, holdout_rows, source_names


def _window(samples: Sequence[Sample], index: int, width: int) -> List[Sample]:
    current_series = samples[index].series_id
    window: List[Sample] = []
    cursor = index
    while cursor >= 0 and len(window) < width:
        sample = samples[cursor]
        if sample.series_id != current_series:
            break
        window.append(sample)
        cursor -= 1
    window.reverse()
    return window


def _mean(values: Sequence[float]) -> float:
    if not values:
        return 0.0
    return sum(values) / float(len(values))


def _variance(values: Sequence[float]) -> float:
    if len(values) <= 1:
        return 0.0
    avg = _mean(values)
    return sum((value - avg) ** 2 for value in values) / float(len(values))


def _stddev(values: Sequence[float]) -> float:
    return math.sqrt(max(0.0, _variance(values)))


def _time_of_day_components(timestamp_s: int) -> Tuple[float, float]:
    seconds_in_day = timestamp_s % 86400
    angle = (seconds_in_day / 86400.0) * 2.0 * math.pi
    return math.sin(angle), math.cos(angle)


def _history_sample(samples: Sequence[Sample], index: int, lag: int) -> Sample:
    current_series = samples[index].series_id
    seen = 0
    cursor = index - 1
    while cursor >= 0:
        sample = samples[cursor]
        if sample.series_id != current_series:
            break
        seen += 1
        if seen == lag:
            return sample
        cursor -= 1
    return samples[index]


def build_feature_vector(samples: Sequence[Sample], index: int) -> List[float]:
    current = samples[index]
    current_glucose = current.glucose_mgdl

    lag_1 = _history_sample(samples, index, 1)
    lag_2 = _history_sample(samples, index, 2)
    lag_3 = _history_sample(samples, index, 3)
    lag_6 = _history_sample(samples, index, 6)
    lag_9 = _history_sample(samples, index, 9)
    lag_12 = _history_sample(samples, index, 12)

    delta_1 = current_glucose - lag_1.glucose_mgdl
    delta_3 = current_glucose - lag_3.glucose_mgdl
    delta_6 = current_glucose - lag_6.glucose_mgdl

    slope_15m = _slope(current, lag_3)
    slope_30m = _slope(current, lag_6)
    slope_60m = _slope(current, lag_12)

    roll_3 = _window(samples, index, 3)
    roll_6 = _window(samples, index, 6)
    roll_12 = _window(samples, index, 12)

    mean_3 = _mean([sample.glucose_mgdl for sample in roll_3])
    mean_6 = _mean([sample.glucose_mgdl for sample in roll_6])
    mean_12 = _mean([sample.glucose_mgdl for sample in roll_12])
    var_6 = _variance([sample.glucose_mgdl for sample in roll_6])
    stddev_6 = math.sqrt(var_6)
    volatility = stddev_6 + (abs(delta_3) * 0.25) + (abs(delta_6) * 0.125)
    tod_sin, tod_cos = _time_of_day_components(current.source_epoch_s)

    values = [
        current_glucose,
        lag_1.glucose_mgdl,
        lag_2.glucose_mgdl,
        lag_3.glucose_mgdl,
        lag_6.glucose_mgdl,
        lag_9.glucose_mgdl,
        lag_12.glucose_mgdl,
        delta_1,
        delta_3,
        delta_6,
        slope_15m,
        slope_30m,
        slope_60m,
        mean_3,
        mean_6,
        mean_12,
        var_6,
        stddev_6,
        volatility,
        current.iob_u,
        current.cob_g,
        current.basal_u_per_hr,
        current.insulin_30m_u,
        current.insulin_120m_u,
        current.carbs_30m_g,
        current.carbs_120m_g,
        current.sqi_pct,
        current.cgm_age_s,
        tod_sin,
        tod_cos,
        float(current.activity_state),
        current.activity_confidence_pct,
        current.motion_rms_5m,
        current.motion_rms_15m,
        current.active_minutes,
        current.post_exercise_minutes,
    ]
    return values


def _slope(current: Sample, past: Sample) -> float:
    delta_minutes = max((current.timestamp - past.timestamp) / 60.0, 1e-6)
    return (current.glucose_mgdl - past.glucose_mgdl) / delta_minutes


def feature_matrix(samples: Sequence[Sample]) -> List[List[float]]:
    return [build_feature_vector(samples, index) for index in range(len(samples))]


def statistics_from_rows(rows: Sequence[Sequence[float]]) -> Tuple[List[float], List[float], List[float]]:
    means: List[float] = []
    scales: List[float] = []
    medians: List[float] = []
    columns = list(zip(*rows)) if rows else [[] for _ in range(FEATURE_COUNT)]
    for column in columns:
        numeric = [value for value in column if math.isfinite(value)]
        if not numeric:
            means.append(0.0)
            scales.append(DEFAULT_SCALE)
            medians.append(0.0)
            continue
        means.append(sum(numeric) / float(len(numeric)))
        variance = sum((value - means[-1]) ** 2 for value in numeric) / float(len(numeric))
        scales.append(math.sqrt(variance) if variance > 1e-12 else DEFAULT_SCALE)
        sorted_values = sorted(numeric)
        midpoint = len(sorted_values) // 2
        if len(sorted_values) % 2:
            medians.append(sorted_values[midpoint])
        else:
            medians.append((sorted_values[midpoint - 1] + sorted_values[midpoint]) / 2.0)
    return means, scales, medians


def _standardize_row(row: Sequence[float], means: Sequence[float], scales: Sequence[float]) -> List[float]:
    return [
        (value - means[index]) / (scales[index] if abs(scales[index]) > 1e-12 else DEFAULT_SCALE)
        for index, value in enumerate(row)
    ]


def build_training_pairs(
    samples: Sequence[Sample],
    horizon_seconds: int,
) -> List[Tuple[List[float], float]]:
    if not samples:
        return []
    lookup = {(sample.series_id, sample.timestamp): index for index, sample in enumerate(samples)}
    pairs: List[Tuple[List[float], float]] = []
    for index, sample in enumerate(samples):
        target_index = lookup.get((sample.series_id, sample.timestamp + horizon_seconds))
        if target_index is None:
            continue
        target = samples[target_index]
        feature_row = build_feature_vector(samples, index)
        delta_target = target.glucose_mgdl - sample.glucose_mgdl
        if not math.isfinite(delta_target):
            continue
        pairs.append((feature_row, delta_target))
    return pairs


def split_pairs(
    pairs: Sequence[Tuple[List[float], float]],
    holdout_ratio: float,
) -> Tuple[List[Tuple[List[float], float]], List[Tuple[List[float], float]]]:
    if not pairs:
        return [], []
    holdout_size = max(1, int(round(len(pairs) * holdout_ratio)))
    if holdout_size >= len(pairs):
        holdout_size = max(1, len(pairs) // 4)
    split_index = len(pairs) - holdout_size
    if split_index <= 0:
        split_index = len(pairs) - 1
    return list(pairs[:split_index]), list(pairs[split_index:])


def fit_linear_model(
    feature_rows: Sequence[Sequence[float]],
    targets: Sequence[float],
    means: Optional[Sequence[float]] = None,
    scales: Optional[Sequence[float]] = None,
    ridge: float = SMOOTHING_RIDGE,
) -> Tuple[float, List[float]]:
    if not feature_rows:
        raise ValueError("no training rows available for regression")
    if means is None or scales is None:
        means, scales, _ = statistics_from_rows(feature_rows)
    standardized_rows = [_standardize_row(row, means, scales) for row in feature_rows]
    design = [[1.0] + row for row in standardized_rows]
    solution = _solve_ridge_normal_equation(design, targets, ridge)
    return solution[0], solution[1:]


def _solve_ridge_normal_equation(
    design: Sequence[Sequence[float]],
    targets: Sequence[float],
    ridge: float,
) -> List[float]:
    rows = len(design)
    cols = len(design[0])
    xtx = [[0.0 for _ in range(cols)] for _ in range(cols)]
    xty = [0.0 for _ in range(cols)]

    for row_index in range(rows):
        row = design[row_index]
        target = targets[row_index]
        for i in range(cols):
            xty[i] += row[i] * target
            for j in range(i, cols):
                xtx[i][j] += row[i] * row[j]
    for i in range(cols):
        for j in range(i):
            xtx[i][j] = xtx[j][i]
        if i > 0:
            xtx[i][i] += ridge
    return _gaussian_elimination(xtx, xty)


def _gaussian_elimination(matrix: List[List[float]], vector: List[float]) -> List[float]:
    size = len(vector)
    augmented = [list(row) + [vector[index]] for index, row in enumerate(matrix)]
    for pivot in range(size):
        best_row = max(range(pivot, size), key=lambda row: abs(augmented[row][pivot]))
        if abs(augmented[best_row][pivot]) < 1e-12:
            raise ValueError("singular training system")
        if best_row != pivot:
            augmented[pivot], augmented[best_row] = augmented[best_row], augmented[pivot]
        pivot_value = augmented[pivot][pivot]
        for column in range(pivot, size + 1):
            augmented[pivot][column] /= pivot_value
        for row in range(size):
            if row == pivot:
                continue
            factor = augmented[row][pivot]
            if abs(factor) < 1e-18:
                continue
            for column in range(pivot, size + 1):
                augmented[row][column] -= factor * augmented[pivot][column]
    return [augmented[row][size] for row in range(size)]


def evaluate_model(model: Dict[str, object], feature_row: Sequence[float]) -> float:
    means = model["feature_mean"]
    scales = model["feature_scale"]
    coefficients = model["coefficients"]
    intercept = float(model["intercept"])
    standardized = _standardize_row(feature_row, means, scales)
    return intercept + sum(coefficients[index] * standardized[index] for index in range(FEATURE_COUNT))


def regression_metrics(predictions: Sequence[float], targets: Sequence[float]) -> Dict[str, float]:
    if not predictions:
        return {"mae": 0.0, "rmse": 0.0}
    errors = [prediction - target for prediction, target in zip(predictions, targets)]
    mae = sum(abs(error) for error in errors) / float(len(errors))
    rmse = math.sqrt(sum(error * error for error in errors) / float(len(errors)))
    return {"mae": mae, "rmse": rmse}


def baseline_linear_trend_delta(current: Sample, horizon_seconds: int, past: Optional[Sample] = None) -> float:
    history_sample = past if past is not None else current
    delta_minutes = max((current.timestamp - history_sample.timestamp) / 60.0, 1e-6)
    slope = (current.glucose_mgdl - history_sample.glucose_mgdl) / delta_minutes
    horizon_minutes = horizon_seconds / 60.0
    return slope * horizon_minutes


def model_to_dict(
    horizon: str,
    intercept: float,
    coefficients: Sequence[float],
    means: Sequence[float],
    scales: Sequence[float],
    medians: Sequence[float],
    metrics: Dict[str, float],
    train_rows: int,
    holdout_rows: int,
    ridge: float,
) -> Dict[str, object]:
    return {
        "horizon": horizon,
        "valid": True,
        "ridge": ridge,
        "feature_names": FEATURE_NAMES,
        "feature_mean": [float(value) for value in means],
        "feature_scale": [float(value if abs(value) > 1e-12 else DEFAULT_SCALE) for value in scales],
        "feature_median": [float(value) for value in medians],
        "coefficients": [float(value) for value in coefficients],
        "intercept": float(intercept),
        "metrics": metrics,
        "train_rows": train_rows,
        "holdout_rows": holdout_rows,
    }


def save_json(path: Path, payload: Dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as handle:
        json.dump(payload, handle, indent=2, sort_keys=True)
        handle.write("\n")


def load_json(path: Path) -> Dict[str, object]:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def format_float(value: float, precision: int = 6) -> str:
    text = f"{float(value):.{precision}f}"
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    if text in {"-0", "-0.0"}:
        return "0"
    if "." not in text:
        text += ".0"
    return text + "f"


def emit_array(values: Sequence[float], indent: int = 8) -> str:
    pieces: List[str] = []
    current_line: List[str] = []
    for index, value in enumerate(values):
        current_line.append(format_float(value))
        if len(current_line) == 10 or index == len(values) - 1:
            pieces.append(" " * indent + ", ".join(current_line) + ",")
            current_line = []
    return "\n".join(pieces)


def write_generated_header(path: Path) -> None:
    content = """#ifndef PREDICTOR_V2_GENERATED_H
#define PREDICTOR_V2_GENERATED_H

#include "aps_types.h"

typedef enum
{
    PREDICTOR_V2_FEATURE_CURRENT_GLUCOSE = 0u,
    PREDICTOR_V2_FEATURE_LAG_1 = 1u,
    PREDICTOR_V2_FEATURE_LAG_2 = 2u,
    PREDICTOR_V2_FEATURE_LAG_3 = 3u,
    PREDICTOR_V2_FEATURE_LAG_6 = 4u,
    PREDICTOR_V2_FEATURE_LAG_9 = 5u,
    PREDICTOR_V2_FEATURE_LAG_12 = 6u,
    PREDICTOR_V2_FEATURE_DELTA_1 = 7u,
    PREDICTOR_V2_FEATURE_DELTA_3 = 8u,
    PREDICTOR_V2_FEATURE_DELTA_6 = 9u,
    PREDICTOR_V2_FEATURE_SLOPE_15M = 10u,
    PREDICTOR_V2_FEATURE_SLOPE_30M = 11u,
    PREDICTOR_V2_FEATURE_SLOPE_60M = 12u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_3 = 13u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_6 = 14u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_12 = 15u,
    PREDICTOR_V2_FEATURE_ROLL_VAR_6 = 16u,
    PREDICTOR_V2_FEATURE_ROLL_STDDEV_6 = 17u,
    PREDICTOR_V2_FEATURE_VOLATILITY_SCORE = 18u,
    PREDICTOR_V2_FEATURE_IOB = 19u,
    PREDICTOR_V2_FEATURE_COB = 20u,
    PREDICTOR_V2_FEATURE_BASAL_RATE = 21u,
    PREDICTOR_V2_FEATURE_INSULIN_30M = 22u,
    PREDICTOR_V2_FEATURE_INSULIN_120M = 23u,
    PREDICTOR_V2_FEATURE_CARBS_30M = 24u,
    PREDICTOR_V2_FEATURE_CARBS_120M = 25u,
    PREDICTOR_V2_FEATURE_SQI = 26u,
    PREDICTOR_V2_FEATURE_CGM_AGE_S = 27u,
    PREDICTOR_V2_FEATURE_TOD_SIN = 28u,
    PREDICTOR_V2_FEATURE_TOD_COS = 29u,
    PREDICTOR_V2_FEATURE_ACTIVITY_STATE = 30u,
    PREDICTOR_V2_FEATURE_ACTIVITY_CONFIDENCE = 31u,
    PREDICTOR_V2_FEATURE_MOTION_RMS_5M = 32u,
    PREDICTOR_V2_FEATURE_MOTION_RMS_15M = 33u,
    PREDICTOR_V2_FEATURE_ACTIVE_MINUTES = 34u,
    PREDICTOR_V2_FEATURE_POST_EXERCISE_MINUTES = 35u,
    PREDICTOR_V2_FEATURE_COUNT = 36u
} predictor_v2_feature_index_t;

typedef struct
{
    float values[PREDICTOR_V2_FEATURE_COUNT];
    uint64_t valid_mask;
} predictor_v2_feature_vector_t;

typedef enum
{
    PREDICTOR_V2_HORIZON_15M = 0u,
    PREDICTOR_V2_HORIZON_30M = 1u,
    PREDICTOR_V2_HORIZON_60M = 2u
} predictor_v2_horizon_t;

typedef struct
{
    float feature_mean[PREDICTOR_V2_FEATURE_COUNT];
    float feature_scale[PREDICTOR_V2_FEATURE_COUNT];
    float feature_median[PREDICTOR_V2_FEATURE_COUNT];
    float coefficients[PREDICTOR_V2_FEATURE_COUNT];
    float intercept;
    bool valid;
} predictor_v2_generated_model_t;

typedef struct
{
    const predictor_v2_generated_model_t *horizon_15m;
    const predictor_v2_generated_model_t *horizon_30m;
    const predictor_v2_generated_model_t *horizon_60m;
} predictor_v2_generated_bundle_t;

typedef struct
{
    uint16_t prediction_mgdl;
    int16_t delta_mgdl;
    uint8_t confidence_pct;
    uint16_t risk_flags;
    uint16_t status_flags;
    bool used_fallback;
} predictor_v2_horizon_eval_t;

#define PREDICTOR_V2_STATUS_VALID_MODEL (1u << 0)
#define PREDICTOR_V2_STATUS_INVALID_MODEL (1u << 1)
#define PREDICTOR_V2_STATUS_INVALID_FEATURES (1u << 2)
#define PREDICTOR_V2_STATUS_FALLBACK_LINEAR (1u << 3)
#define PREDICTOR_V2_STATUS_LOW_CONFIDENCE (1u << 4)
#define PREDICTOR_V2_STATUS_BAD_SQI (1u << 5)
#define PREDICTOR_V2_STATUS_STALE_CGM (1u << 6)
#define PREDICTOR_V2_STATUS_MISSING_PHYSIOLOGY (1u << 7)

const predictor_v2_generated_bundle_t *PredictorV2Generated_DefaultBundle(void);

#endif
"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_generated_source(path: Path, models: Dict[str, Dict[str, object]]) -> None:
    content_lines = [
        '#include "predictor_v2_generated.h"',
        "",
    ]
    for horizon in ("15m", "30m", "60m"):
        model = models[horizon]
        content_lines.append(f"static const predictor_v2_generated_model_t k_model_{horizon} =")
        content_lines.append("{")
        content_lines.append("    .feature_mean = {")
        content_lines.append(emit_array(model["feature_mean"], indent=8))
        content_lines.append("    },")
        content_lines.append("    .feature_scale = {")
        content_lines.append(emit_array(model["feature_scale"], indent=8))
        content_lines.append("    },")
        content_lines.append("    .feature_median = {")
        content_lines.append(emit_array(model["feature_median"], indent=8))
        content_lines.append("    },")
        content_lines.append("    .coefficients = {")
        content_lines.append(emit_array(model["coefficients"], indent=8))
        content_lines.append("    },")
        content_lines.append(f"    .intercept = {format_float(model['intercept'])},")
        content_lines.append("    .valid = true,")
        content_lines.append("};")
        content_lines.append("")
    content_lines.extend(
        [
            "static const predictor_v2_generated_bundle_t k_default_bundle =",
            "{",
            "    .horizon_15m = &k_model_15m,",
            "    .horizon_30m = &k_model_30m,",
            "    .horizon_60m = &k_model_60m,",
            "};",
            "",
            "const predictor_v2_generated_bundle_t *PredictorV2Generated_DefaultBundle(void)",
            "{",
            "    return &k_default_bundle;",
            "}",
            "",
        ]
    )
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(content_lines), encoding="utf-8")


def summarize_metrics(rows: int, metrics: Dict[str, Dict[str, float]]) -> str:
    parts = [f"rows={rows}"]
    for horizon in ("15m", "30m", "60m"):
        if horizon in metrics:
            parts.append(
                f"{horizon}:mae={metrics[horizon]['mae']:.2f},rmse={metrics[horizon]['rmse']:.2f}"
            )
    return "; ".join(parts)
