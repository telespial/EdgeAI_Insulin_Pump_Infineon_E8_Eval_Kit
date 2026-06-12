#!/usr/bin/env python3
"""Convert the real insulin-pump training source into Predictor V2 CSV format."""

from __future__ import annotations

import argparse
import csv
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


DEFAULT_SOURCE_DIR = Path(
    "/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump"
)
DEFAULT_OUTPUT = Path("data/training/predictor_v2_real_dataset.csv")
SAMPLE_INTERVAL_S = 300


@dataclass(frozen=True)
class SourceRow:
    source_file: str
    source_row: int
    subject: str
    ts_iso: str
    source_epoch_s: int
    glucose_mgdl: float
    glucose_type: str
    insulin_fast_u: float
    insulin_slow_u: float
    insulin_total_u: float


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
                rows.append(
                    SourceRow(
                        source_file=csv_path.name,
                        source_row=index + 1,
                        subject=str(raw.get("subject", "")).strip(),
                        ts_iso=str(raw.get("ts_iso", "")).strip(),
                        source_epoch_s=_parse_int(raw.get("epoch_s", "")),
                        glucose_mgdl=_parse_float(raw.get("glucose_mgdl", "")),
                        glucose_type=str(raw.get("glucose_type", "")).strip(),
                        insulin_fast_u=_parse_float(raw.get("insulin_fast_u", "")),
                        insulin_slow_u=_parse_float(raw.get("insulin_slow_u", "")),
                        insulin_total_u=_parse_float(raw.get("insulin_total_u", "")),
                    )
                )
    rows.sort(key=lambda row: (row.source_epoch_s, row.source_file, row.source_row))
    return rows, skipped


def _normalize_rows(rows: Sequence[SourceRow]) -> List[Dict[str, object]]:
    normalized: List[Dict[str, object]] = []
    if not rows:
        return normalized
    base_timestamp = rows[0].source_epoch_s - (rows[0].source_epoch_s % SAMPLE_INTERVAL_S)
    previous_glucose = None
    for index, row in enumerate(rows):
        timestamp = base_timestamp + index * SAMPLE_INTERVAL_S
        trend = 0.0
        if previous_glucose is not None:
            trend = (row.glucose_mgdl - previous_glucose) / (SAMPLE_INTERVAL_S / 60.0)
        previous_glucose = row.glucose_mgdl
        physiology_present = 1 if any(value > 0.0 for value in (row.insulin_fast_u, row.insulin_slow_u, row.insulin_total_u)) else 0
        normalized.append(
            {
                "source_file": row.source_file,
                "source_row": row.source_row,
                "subject": row.subject,
                "source_epoch_s": row.source_epoch_s,
                "source_ts_iso": row.ts_iso,
                "timestamp": timestamp,
                "glucose_mgdl": round(row.glucose_mgdl, 3),
                "sqi_pct": 95.0,
                "cgm_age_s": 0.0,
                "iob_u": round(row.insulin_fast_u, 3),
                "cob_g": 0.0,
                "activity_state": "unknown",
                "activity_confidence_pct": 0.0,
                "motion_rms_5m": 0.0,
                "motion_rms_15m": 0.0,
                "active_minutes": 0.0,
                "post_exercise_minutes": 0.0,
                "basal_u_per_hr": round(row.insulin_slow_u, 3),
                "insulin_30m_u": round(row.insulin_fast_u, 3),
                "insulin_120m_u": round(row.insulin_total_u, 3),
                "carbs_30m_g": 0.0,
                "carbs_120m_g": 0.0,
                "physiology_present": physiology_present,
                "trend_mgdl_min": round(trend, 3),
                "glucose_type": row.glucose_type,
            }
        )
    return normalized


def _fieldnames() -> List[str]:
    return [
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
