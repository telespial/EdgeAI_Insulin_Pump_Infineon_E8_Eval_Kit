#!/usr/bin/env python3
"""Convert the real insulin-pump training source into Predictor V2 CSV format."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from collections import defaultdict
from pathlib import Path
from typing import DefaultDict, Dict, Iterable, List, Sequence, Tuple


DEFAULT_SOURCE_DIR = Path(
    "/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump"
)
DEFAULT_OUTPUT = Path("data/training/predictor_v2_real_dataset.csv")
SAMPLE_INTERVAL_S = 300
IOB_DECAY_S = 3 * 60 * 60
COB_DECAY_S = 2 * 60 * 60
ACTIVITY_DECAY_S = 45 * 60
EXERCISE_DECAY_S = 90 * 60


@dataclass(frozen=True)
class SourceRow:
    source_file: str
    source_row: int
    series_id: str
    subject: str
    ts_iso: str
    source_epoch_s: int
    glucose_mgdl: float
    glucose_type: str
    insulin_fast_u: float
    insulin_slow_u: float
    insulin_total_u: float


def _clamp(value: float, lower: float, upper: float) -> float:
    return max(lower, min(upper, value))


def _mean(values: Sequence[float]) -> float:
    if not values:
        return 0.0
    return sum(values) / float(len(values))


def _variance(values: Sequence[float]) -> float:
    if len(values) <= 1:
        return 0.0
    average = _mean(values)
    return sum((value - average) ** 2 for value in values) / float(len(values))


def _slope_from_history(rows: Sequence[SourceRow], index: int, lag: int) -> float:
    if index - lag < 0:
        lag_row = rows[0]
    else:
        lag_row = rows[index - lag]
    current_row = rows[index]
    delta_minutes = max((current_row.source_epoch_s - lag_row.source_epoch_s) / 60.0, 1e-6)
    return (current_row.glucose_mgdl - lag_row.glucose_mgdl) / delta_minutes


def _parse_float(value: str, default: float = 0.0) -> float:
    text = (value or "").strip()
    if text == "":
        return default
    try:
        return float(text)
    except ValueError:
        return default


def _parse_int(value: str, default: int = 0) -> int:
    text = (value or "").strip()
    if text == "":
        return default
    try:
        return int(float(text))
    except ValueError:
        return default


def _load_rows(source_dir: Path) -> Tuple[List[SourceRow], List[str]]:
    rows: List[SourceRow] = []
    skipped: List[str] = []
    for csv_path in sorted(source_dir.glob("*.csv")):
        with csv_path.open(newline="", encoding="utf-8") as handle:
            reader = csv.DictReader(handle)
            required = {"subject", "ts_iso", "epoch_s", "glucose_mgdl", "glucose_type", "insulin_fast_u", "insulin_slow_u", "insulin_total_u"}
            missing = required.difference(reader.fieldnames or [])
            if missing:
                skipped.append(csv_path.name)
                continue
            for index, raw in enumerate(reader):
                subject = str(raw.get("subject", "")).strip()
                series_id = f"{csv_path.stem}:{subject or 'unknown'}"
                rows.append(
                    SourceRow(
                        source_file=csv_path.name,
                        source_row=index + 1,
                        series_id=series_id,
                        subject=subject,
                        ts_iso=str(raw.get("ts_iso", "")).strip(),
                        source_epoch_s=_parse_int(raw.get("epoch_s", "")),
                        glucose_mgdl=_parse_float(raw.get("glucose_mgdl", "")),
                        glucose_type=str(raw.get("glucose_type", "")).strip(),
                        insulin_fast_u=_parse_float(raw.get("insulin_fast_u", "")),
                        insulin_slow_u=_parse_float(raw.get("insulin_slow_u", "")),
                        insulin_total_u=_parse_float(raw.get("insulin_total_u", "")),
                    )
                )
    rows.sort(key=lambda row: (row.series_id, row.source_epoch_s, row.source_row))
    return rows, skipped


def _normalize_rows(rows: Sequence[SourceRow]) -> List[Dict[str, object]]:
    normalized: List[Dict[str, object]] = []
    if not rows:
        return normalized
    grouped: DefaultDict[str, List[SourceRow]] = defaultdict(list)
    for row in rows:
        grouped[row.series_id].append(row)

    series_base = 0
    for series_id in sorted(grouped):
        series_rows = grouped[series_id]
        series_rows.sort(key=lambda row: (row.source_epoch_s, row.source_row))
        base_timestamp = series_base
        previous_epoch = None
        previous_glucose = None
        previous_slope_15m = 0.0
        iob_u = 0.0
        cob_g = 0.0
        active_minutes = 0.0
        post_exercise_minutes = 0.0
        last_cgm_epoch = None
        for index, row in enumerate(series_rows):
            if previous_epoch is None:
                dt_s = SAMPLE_INTERVAL_S
            else:
                dt_s = max(SAMPLE_INTERVAL_S, row.source_epoch_s - previous_epoch)
            timestamp = base_timestamp + index * SAMPLE_INTERVAL_S

            history_3 = series_rows[max(0, index - 2) : index + 1]
            history_6 = series_rows[max(0, index - 5) : index + 1]
            history_12 = series_rows[max(0, index - 11) : index + 1]
            history_24 = series_rows[max(0, index - 23) : index + 1]

            glucose_values_3 = [sample.glucose_mgdl for sample in history_3]
            glucose_values_6 = [sample.glucose_mgdl for sample in history_6]
            glucose_values_12 = [sample.glucose_mgdl for sample in history_12]
            glucose_values_24 = [sample.glucose_mgdl for sample in history_24]
            mean_3 = _mean(glucose_values_3)
            mean_6 = _mean(glucose_values_6)
            mean_12 = _mean(glucose_values_12)
            var_6 = _variance(glucose_values_6)
            std_6 = math.sqrt(var_6)
            slope_15m = _slope_from_history(series_rows, index, 3)
            slope_30m = _slope_from_history(series_rows, index, 6)
            slope_60m = _slope_from_history(series_rows, index, 12)
            accel = slope_15m - previous_slope_15m
            excursion = max(glucose_values_6) - min(glucose_values_6) if glucose_values_6 else 0.0
            above_range = sum(1 for value in glucose_values_12 if value >= 180.0) / float(len(glucose_values_12) or 1)
            below_range = sum(1 for value in glucose_values_12 if value <= 70.0) / float(len(glucose_values_12) or 1)
            volatility = std_6 + abs(slope_15m) * 1.2 + abs(accel) * 2.0
            physiology_present = 1 if any(value > 0.0 for value in (row.insulin_fast_u, row.insulin_slow_u, row.insulin_total_u)) else 0

            iob_u = _clamp(iob_u * math.exp(-dt_s / IOB_DECAY_S) + max(0.0, row.insulin_fast_u), 0.0, 25.0)
            meal_drive = max(0.0, slope_30m * 5.0) + max(0.0, mean_3 - mean_6) * 0.18 + max(0.0, excursion - 18.0) * 0.04
            if row.insulin_fast_u > 0.0 and slope_15m < 0.0:
                meal_drive *= 0.7
            cob_g = _clamp(cob_g * math.exp(-dt_s / COB_DECAY_S) + meal_drive, 0.0, 140.0)

            if row.glucose_type.lower() == "cgm":
                last_cgm_epoch = row.source_epoch_s
                cgm_age_s = 0.0
            elif last_cgm_epoch is None:
                cgm_age_s = float(dt_s)
            else:
                cgm_age_s = float(row.source_epoch_s - last_cgm_epoch)
            cgm_age_s = _clamp(cgm_age_s, 0.0, 3600.0)

            quality_penalty = 0.0
            if row.glucose_type.lower() != "cgm":
                quality_penalty += 18.0
            jitter = abs(dt_s - SAMPLE_INTERVAL_S)
            quality_penalty += min(jitter / 60.0 * 2.0, 16.0)
            if row.glucose_mgdl <= 40.0 or row.glucose_mgdl >= 400.0:
                quality_penalty += 10.0
            if previous_glucose is not None and abs(row.glucose_mgdl - previous_glucose) > 60.0:
                quality_penalty += 8.0
            if cgm_age_s > 900.0:
                quality_penalty += 4.0
            sqi_pct = _clamp(100.0 - quality_penalty, 35.0, 100.0)

            activity_drive = max(0.0, abs(slope_15m) * 0.6 + volatility * 0.03 + above_range * 0.8 - below_range * 0.2 - iob_u * 0.04)
            if row.glucose_type.lower() != "cgm":
                activity_drive *= 0.85
            if activity_drive < 0.55:
                activity_state = "unknown" if index < 3 else "sedentary"
            elif activity_drive < 1.15:
                activity_state = "sedentary"
            elif activity_drive < 2.0:
                activity_state = "light"
            elif activity_drive < 3.0:
                activity_state = "moderate"
            else:
                activity_state = "exercise"
            activity_confidence_pct = _clamp(activity_drive * 28.0, 0.0, 100.0)

            if activity_state == "exercise":
                active_minutes = min(240.0, active_minutes * math.exp(-dt_s / ACTIVITY_DECAY_S) + dt_s / 60.0)
                post_exercise_minutes = min(480.0, post_exercise_minutes * math.exp(-dt_s / EXERCISE_DECAY_S))
            elif activity_state in {"moderate", "light"}:
                active_minutes = min(240.0, active_minutes * math.exp(-dt_s / ACTIVITY_DECAY_S) + dt_s / 180.0)
                post_exercise_minutes = min(480.0, post_exercise_minutes * math.exp(-dt_s / EXERCISE_DECAY_S))
            else:
                active_minutes = max(0.0, active_minutes * math.exp(-dt_s / ACTIVITY_DECAY_S))
                post_exercise_minutes = min(480.0, post_exercise_minutes * math.exp(-dt_s / EXERCISE_DECAY_S) + (2.0 if slope_15m < -1.0 else 0.0))

            motion_rms_5m = _clamp((std_6 * 0.08) + (abs(slope_15m) * 0.12) + (excursion * 0.02), 0.0, 20.0)
            motion_rms_15m = _clamp((std_6 * 0.12) + (abs(slope_30m) * 0.08) + (excursion * 0.03), 0.0, 30.0)
            carbs_30m_g = _clamp(meal_drive * 0.55, 0.0, 80.0)
            carbs_120m_g = _clamp(cob_g, 0.0, 140.0)

            normalized.append(
                {
                    "series_id": series_id,
                    "series_row": index + 1,
                    "source_file": row.source_file,
                    "source_row": row.source_row,
                    "subject": row.subject,
                    "source_epoch_s": row.source_epoch_s,
                    "source_ts_iso": row.ts_iso,
                    "timestamp": timestamp,
                    "glucose_mgdl": round(row.glucose_mgdl, 3),
                    "sqi_pct": round(sqi_pct, 3),
                    "cgm_age_s": round(cgm_age_s, 3),
                    "iob_u": round(iob_u, 3),
                    "cob_g": round(cob_g, 3),
                    "activity_state": activity_state,
                    "activity_confidence_pct": round(activity_confidence_pct, 3),
                    "motion_rms_5m": round(motion_rms_5m, 3),
                    "motion_rms_15m": round(motion_rms_15m, 3),
                    "active_minutes": round(active_minutes, 3),
                    "post_exercise_minutes": round(post_exercise_minutes, 3),
                    "basal_u_per_hr": round(row.insulin_slow_u, 3),
                    "insulin_30m_u": round(row.insulin_fast_u, 3),
                    "insulin_120m_u": round(row.insulin_total_u, 3),
                    "carbs_30m_g": round(carbs_30m_g, 3),
                    "carbs_120m_g": round(carbs_120m_g, 3),
                    "physiology_present": physiology_present,
                    "trend_mgdl_min": round(slope_15m, 3),
                    "glucose_type": row.glucose_type,
                    "rolling_volatility_6": round(std_6, 3),
                    "glucose_accel_15m": round(accel, 3),
                    "time_above_range": round(above_range, 3),
                    "time_below_range": round(below_range, 3),
                    "recent_excursion_mgdl": round(excursion, 3),
                }
            )
            previous_epoch = row.source_epoch_s
            previous_glucose = row.glucose_mgdl
            previous_slope_15m = slope_15m
        series_base += len(series_rows) * SAMPLE_INTERVAL_S + SAMPLE_INTERVAL_S
    return normalized


def _fieldnames() -> List[str]:
    return [
        "series_id",
        "series_row",
        "source_file",
        "source_row",
        "subject",
        "source_epoch_s",
        "source_ts_iso",
        "timestamp",
        "glucose_mgdl",
        "sqi_pct",
        "cgm_age_s",
        "iob_u",
        "cob_g",
        "activity_state",
        "activity_confidence_pct",
        "motion_rms_5m",
        "motion_rms_15m",
        "active_minutes",
        "post_exercise_minutes",
        "basal_u_per_hr",
        "insulin_30m_u",
        "insulin_120m_u",
        "carbs_30m_g",
        "carbs_120m_g",
        "physiology_present",
        "trend_mgdl_min",
        "glucose_type",
        "rolling_volatility_6",
        "glucose_accel_15m",
        "time_above_range",
        "time_below_range",
        "recent_excursion_mgdl",
    ]


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Import real insulin-pump training data into Predictor V2 CSV format.")
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=DEFAULT_SOURCE_DIR,
        help="Directory containing the real insulin pump CSV source files.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Output CSV path.",
    )
    return parser


def main() -> int:
    parser = _build_parser()
    args = parser.parse_args()
    rows, skipped = _load_rows(args.source_dir)
    normalized = _normalize_rows(rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=_fieldnames())
        writer.writeheader()
        writer.writerows(normalized)
    print(f"imported {len(normalized)} rows from {args.source_dir}")
    if skipped:
        print(f"skipped non-training files: {', '.join(skipped)}")
    print(f"wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
