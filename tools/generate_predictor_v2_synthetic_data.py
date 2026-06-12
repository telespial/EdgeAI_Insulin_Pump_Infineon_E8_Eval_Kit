#!/usr/bin/env python3
"""Generate larger deterministic replay-style CSV data for Predictor V2 training."""

from __future__ import annotations

import argparse
import csv
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Dict, Iterable, List


STEP_SECONDS = 300
DEFAULT_ROWS_PER_SCENARIO = 36

FIELDNAMES = [
    "scenario",
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
]


@dataclass(frozen=True)
class ScenarioSpec:
    name: str
    glucose_start: float
    glucose_step: float
    glucose_curve: Callable[[int, int], float]
    sqi_curve: Callable[[int, int], float]
    cgm_age_curve: Callable[[int, int], float]
    iob_curve: Callable[[int, int], float]
    cob_curve: Callable[[int, int], float]
    activity_curve: Callable[[int, int], str]
    activity_confidence_curve: Callable[[int, int], float]
    motion_5_curve: Callable[[int, int], float]
    motion_15_curve: Callable[[int, int], float]
    active_minutes_curve: Callable[[int, int], float]
    post_exercise_curve: Callable[[int, int], float]
    basal_curve: Callable[[int, int], float]
    insulin_30_curve: Callable[[int, int], float]
    insulin_120_curve: Callable[[int, int], float]
    carbs_30_curve: Callable[[int, int], float]
    carbs_120_curve: Callable[[int, int], float]
    physiology_present: bool = True


def _clamp(value: float, minimum: float, maximum: float) -> float:
    return max(minimum, min(maximum, value))


def _stable_overnight_glucose(step: int, count: int) -> float:
    return 109.5 + 0.9 * math.sin(step / 5.0) + 0.4 * math.cos(step / 9.0)


def _meal_rise_glucose(step: int, count: int) -> float:
    ramp = 118.0 + 2.8 * step + 8.0 * math.tanh((step - count / 3.0) / 4.0)
    return ramp


def _falling_bolus_glucose(step: int, count: int) -> float:
    return 220.0 - 3.4 * step - 5.5 * math.tanh((step - count / 3.0) / 3.0)


def _exercise_glucose(step: int, count: int) -> float:
    return 165.0 - 1.9 * step + 2.5 * math.sin(step / 6.0)


def _post_exercise_glucose(step: int, count: int) -> float:
    return 95.0 + 1.4 * step - 2.0 * math.exp(-step / 8.0)


def _bad_sqi_glucose(step: int, count: int) -> float:
    return 176.0 + 0.35 * math.sin(step / 3.0)


def _stale_cgm_glucose(step: int, count: int) -> float:
    return 136.0 + 0.15 * math.cos(step / 4.0)


def _stable_sqi(step: int, count: int) -> float:
    return 99.0


def _meal_sqi(step: int, count: int) -> float:
    return 97.0


def _falling_sqi(step: int, count: int) -> float:
    return 98.0


def _exercise_sqi(step: int, count: int) -> float:
    return 96.0


def _post_exercise_sqi(step: int, count: int) -> float:
    return 98.0


def _bad_sqi(step: int, count: int) -> float:
    return 35.0


def _stale_cgm_sqi(step: int, count: int) -> float:
    return 97.0


def _zero(step: int, count: int) -> float:
    return 0.0


def _stable_iob(step: int, count: int) -> float:
    return 0.25 + 0.03 * math.sin(step / 7.0)


def _meal_iob(step: int, count: int) -> float:
    return 0.35 + 0.05 * step / max(count - 1, 1)


def _falling_iob(step: int, count: int) -> float:
    midpoint = count / 2.0
    return 2.2 * math.exp(-((step - midpoint) ** 2) / max((count / 4.0) ** 2, 1.0)) + 0.4


def _exercise_iob(step: int, count: int) -> float:
    return 1.4 + 0.6 * math.exp(-step / 18.0)


def _post_exercise_iob(step: int, count: int) -> float:
    return 0.9 * math.exp(-step / 20.0) + 0.15


def _bad_sqi_iob(step: int, count: int) -> float:
    return 0.2


def _stale_cgm_iob(step: int, count: int) -> float:
    return 0.15


def _stable_cob(step: int, count: int) -> float:
    return 0.0


def _meal_cob(step: int, count: int) -> float:
    return _clamp(60.0 - 1.7 * step, 0.0, 200.0)


def _falling_cob(step: int, count: int) -> float:
    return 0.0


def _exercise_cob(step: int, count: int) -> float:
    return 0.0


def _post_exercise_cob(step: int, count: int) -> float:
    return 0.0


def _bad_sqi_cob(step: int, count: int) -> float:
    return 0.0


def _stale_cgm_cob(step: int, count: int) -> float:
    return 0.0


def _stable_activity(step: int, count: int) -> str:
    return "sedentary"


def _meal_activity(step: int, count: int) -> str:
    return "light"


def _falling_activity(step: int, count: int) -> str:
    return "sedentary"


def _exercise_activity(step: int, count: int) -> str:
    return "exercise"


def _post_exercise_activity(step: int, count: int) -> str:
    return "light"


def _bad_sqi_activity(step: int, count: int) -> str:
    return "sedentary"


def _stale_cgm_activity(step: int, count: int) -> str:
    return "unknown"


def _stable_confidence(step: int, count: int) -> float:
    return 96.0


def _meal_confidence(step: int, count: int) -> float:
    return 94.0


def _falling_confidence(step: int, count: int) -> float:
    return 92.0


def _exercise_confidence(step: int, count: int) -> float:
    return 90.0


def _post_exercise_confidence(step: int, count: int) -> float:
    return 88.0


def _bad_sqi_confidence(step: int, count: int) -> float:
    return 40.0


def _stale_cgm_confidence(step: int, count: int) -> float:
    return 93.0


def _stable_motion_5(step: int, count: int) -> float:
    return 3.0 + 0.2 * math.sin(step / 4.0)


def _meal_motion_5(step: int, count: int) -> float:
    return 4.0 + 0.15 * math.sin(step / 3.0)


def _falling_motion_5(step: int, count: int) -> float:
    return 3.2 + 0.1 * math.cos(step / 3.5)


def _exercise_motion_5(step: int, count: int) -> float:
    return 8.0 + 0.8 * math.sin(step / 2.5)


def _post_exercise_motion_5(step: int, count: int) -> float:
    return 5.0 + 0.4 * math.sin(step / 4.5)


def _bad_sqi_motion_5(step: int, count: int) -> float:
    return 2.0


def _stale_cgm_motion_5(step: int, count: int) -> float:
    return 1.5


def _stable_motion_15(step: int, count: int) -> float:
    return 2.5 + 0.1 * math.cos(step / 5.0)


def _meal_motion_15(step: int, count: int) -> float:
    return 3.4 + 0.1 * math.cos(step / 4.0)


def _falling_motion_15(step: int, count: int) -> float:
    return 2.6 + 0.1 * math.sin(step / 6.0)


def _exercise_motion_15(step: int, count: int) -> float:
    return 7.0 + 0.7 * math.cos(step / 3.0)


def _post_exercise_motion_15(step: int, count: int) -> float:
    return 4.5 + 0.3 * math.cos(step / 5.0)


def _bad_sqi_motion_15(step: int, count: int) -> float:
    return 1.5


def _stale_cgm_motion_15(step: int, count: int) -> float:
    return 1.0


def _stable_active(step: int, count: int) -> float:
    return 10.0 + step


def _meal_active(step: int, count: int) -> float:
    return 20.0 + step


def _falling_active(step: int, count: int) -> float:
    return 15.0 + step / 2.0


def _exercise_active(step: int, count: int) -> float:
    return 40.0 + step


def _post_exercise_active(step: int, count: int) -> float:
    return 30.0 + step / 2.0


def _bad_sqi_active(step: int, count: int) -> float:
    return 5.0


def _stale_cgm_active(step: int, count: int) -> float:
    return 0.0


def _stable_post(step: int, count: int) -> float:
    return 0.0


def _meal_post(step: int, count: int) -> float:
    return 0.0


def _falling_post(step: int, count: int) -> float:
    return 0.0


def _exercise_post(step: int, count: int) -> float:
    return 0.0


def _post_exercise_post(step: int, count: int) -> float:
    return max(0.0, 30.0 - step)


def _bad_sqi_post(step: int, count: int) -> float:
    return 0.0


def _stale_cgm_post(step: int, count: int) -> float:
    return 0.0


def _stable_basal(step: int, count: int) -> float:
    return 0.8


def _meal_basal(step: int, count: int) -> float:
    return 0.9


def _falling_basal(step: int, count: int) -> float:
    return 0.7


def _exercise_basal(step: int, count: int) -> float:
    return 0.75


def _post_exercise_basal(step: int, count: int) -> float:
    return 0.8


def _bad_sqi_basal(step: int, count: int) -> float:
    return 0.8


def _stale_cgm_basal(step: int, count: int) -> float:
    return 0.8


def _meal_insulin_30(step: int, count: int) -> float:
    return 0.5 + 0.03 * step


def _meal_insulin_120(step: int, count: int) -> float:
    return 1.0 + 0.04 * step


def _falling_insulin_30(step: int, count: int) -> float:
    return 1.8 + 0.06 * step


def _falling_insulin_120(step: int, count: int) -> float:
    return 3.0 + 0.09 * step


def _exercise_insulin_30(step: int, count: int) -> float:
    return 1.2 + 0.02 * step


def _exercise_insulin_120(step: int, count: int) -> float:
    return 2.4 + 0.03 * step


def _post_exercise_insulin_30(step: int, count: int) -> float:
    return 0.8 + 0.01 * step


def _post_exercise_insulin_120(step: int, count: int) -> float:
    return 1.4 + 0.015 * step


def _stable_insulin_30(step: int, count: int) -> float:
    return 0.3


def _stable_insulin_120(step: int, count: int) -> float:
    return 0.6


def _bad_sqi_insulin_30(step: int, count: int) -> float:
    return 0.2


def _bad_sqi_insulin_120(step: int, count: int) -> float:
    return 0.5


def _stale_cgm_insulin_30(step: int, count: int) -> float:
    return 0.1


def _stale_cgm_insulin_120(step: int, count: int) -> float:
    return 0.3


def _meal_carbs_30(step: int, count: int) -> float:
    return _clamp(40.0 - 0.8 * step, 0.0, 200.0)


def _meal_carbs_120(step: int, count: int) -> float:
    return _clamp(80.0 - 1.1 * step, 0.0, 300.0)


def _stable_carbs_30(step: int, count: int) -> float:
    return 0.0


def _stable_carbs_120(step: int, count: int) -> float:
    return 0.0


def _falling_carbs_30(step: int, count: int) -> float:
    return 0.0


def _falling_carbs_120(step: int, count: int) -> float:
    return 0.0


def _exercise_carbs_30(step: int, count: int) -> float:
    return 0.0


def _exercise_carbs_120(step: int, count: int) -> float:
    return 0.0


def _post_exercise_carbs_30(step: int, count: int) -> float:
    return 5.0


def _post_exercise_carbs_120(step: int, count: int) -> float:
    return 12.0


def _bad_sqi_carbs_30(step: int, count: int) -> float:
    return 0.0


def _bad_sqi_carbs_120(step: int, count: int) -> float:
    return 0.0


def _stale_cgm_carbs_30(step: int, count: int) -> float:
    return 0.0


def _stale_cgm_carbs_120(step: int, count: int) -> float:
    return 0.0


SCENARIOS: List[ScenarioSpec] = [
    ScenarioSpec(
        name="stable_overnight",
        glucose_start=110.0,
        glucose_step=0.0,
        glucose_curve=_stable_overnight_glucose,
        sqi_curve=_stable_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_stable_iob,
        cob_curve=_stable_cob,
        activity_curve=_stable_activity,
        activity_confidence_curve=_stable_confidence,
        motion_5_curve=_stable_motion_5,
        motion_15_curve=_stable_motion_15,
        active_minutes_curve=_stable_active,
        post_exercise_curve=_stable_post,
        basal_curve=_stable_basal,
        insulin_30_curve=_stable_insulin_30,
        insulin_120_curve=_stable_insulin_120,
        carbs_30_curve=_stable_carbs_30,
        carbs_120_curve=_stable_carbs_120,
    ),
    ScenarioSpec(
        name="meal_rise",
        glucose_start=118.0,
        glucose_step=2.8,
        glucose_curve=_meal_rise_glucose,
        sqi_curve=_meal_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_meal_iob,
        cob_curve=_meal_cob,
        activity_curve=_meal_activity,
        activity_confidence_curve=_meal_confidence,
        motion_5_curve=_meal_motion_5,
        motion_15_curve=_meal_motion_15,
        active_minutes_curve=_meal_active,
        post_exercise_curve=_stable_post,
        basal_curve=_meal_basal,
        insulin_30_curve=_meal_insulin_30,
        insulin_120_curve=_meal_insulin_120,
        carbs_30_curve=_meal_carbs_30,
        carbs_120_curve=_meal_carbs_120,
    ),
    ScenarioSpec(
        name="falling_after_bolus",
        glucose_start=220.0,
        glucose_step=-3.4,
        glucose_curve=_falling_bolus_glucose,
        sqi_curve=_falling_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_falling_iob,
        cob_curve=_falling_cob,
        activity_curve=_falling_activity,
        activity_confidence_curve=_falling_confidence,
        motion_5_curve=_falling_motion_5,
        motion_15_curve=_falling_motion_15,
        active_minutes_curve=_falling_active,
        post_exercise_curve=_falling_post,
        basal_curve=_falling_basal,
        insulin_30_curve=_falling_insulin_30,
        insulin_120_curve=_falling_insulin_120,
        carbs_30_curve=_falling_carbs_30,
        carbs_120_curve=_falling_carbs_120,
    ),
    ScenarioSpec(
        name="exercise_with_iob",
        glucose_start=165.0,
        glucose_step=-1.9,
        glucose_curve=_exercise_glucose,
        sqi_curve=_exercise_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_exercise_iob,
        cob_curve=_exercise_cob,
        activity_curve=_exercise_activity,
        activity_confidence_curve=_exercise_confidence,
        motion_5_curve=_exercise_motion_5,
        motion_15_curve=_exercise_motion_15,
        active_minutes_curve=_exercise_active,
        post_exercise_curve=_exercise_post,
        basal_curve=_exercise_basal,
        insulin_30_curve=_exercise_insulin_30,
        insulin_120_curve=_exercise_insulin_120,
        carbs_30_curve=_exercise_carbs_30,
        carbs_120_curve=_exercise_carbs_120,
    ),
    ScenarioSpec(
        name="post_exercise_recovery",
        glucose_start=95.0,
        glucose_step=1.4,
        glucose_curve=_post_exercise_glucose,
        sqi_curve=_post_exercise_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_post_exercise_iob,
        cob_curve=_post_exercise_cob,
        activity_curve=_post_exercise_activity,
        activity_confidence_curve=_post_exercise_confidence,
        motion_5_curve=_post_exercise_motion_5,
        motion_15_curve=_post_exercise_motion_15,
        active_minutes_curve=_post_exercise_active,
        post_exercise_curve=_post_exercise_post,
        basal_curve=_post_exercise_basal,
        insulin_30_curve=_post_exercise_insulin_30,
        insulin_120_curve=_post_exercise_insulin_120,
        carbs_30_curve=_post_exercise_carbs_30,
        carbs_120_curve=_post_exercise_carbs_120,
    ),
    ScenarioSpec(
        name="bad_sqi",
        glucose_start=176.0,
        glucose_step=0.0,
        glucose_curve=_bad_sqi_glucose,
        sqi_curve=_bad_sqi,
        cgm_age_curve=lambda step, count: 0.0,
        iob_curve=_bad_sqi_iob,
        cob_curve=_bad_sqi_cob,
        activity_curve=_bad_sqi_activity,
        activity_confidence_curve=_bad_sqi_confidence,
        motion_5_curve=_bad_sqi_motion_5,
        motion_15_curve=_bad_sqi_motion_15,
        active_minutes_curve=_bad_sqi_active,
        post_exercise_curve=_bad_sqi_post,
        basal_curve=_bad_sqi_basal,
        insulin_30_curve=_bad_sqi_insulin_30,
        insulin_120_curve=_bad_sqi_insulin_120,
        carbs_30_curve=_bad_sqi_carbs_30,
        carbs_120_curve=_bad_sqi_carbs_120,
    ),
    ScenarioSpec(
        name="stale_cgm",
        glucose_start=136.0,
        glucose_step=0.0,
        glucose_curve=_stale_cgm_glucose,
        sqi_curve=_stale_cgm_sqi,
        cgm_age_curve=lambda step, count: 1200.0,
        iob_curve=_stale_cgm_iob,
        cob_curve=_stale_cgm_cob,
        activity_curve=_stale_cgm_activity,
        activity_confidence_curve=_stale_cgm_confidence,
        motion_5_curve=_stale_cgm_motion_5,
        motion_15_curve=_stale_cgm_motion_15,
        active_minutes_curve=_stale_cgm_active,
        post_exercise_curve=_stale_cgm_post,
        basal_curve=_stale_cgm_basal,
        insulin_30_curve=_stale_cgm_insulin_30,
        insulin_120_curve=_stale_cgm_insulin_120,
        carbs_30_curve=_stale_cgm_carbs_30,
        carbs_120_curve=_stale_cgm_carbs_120,
    ),
]


def _trend_mgdl_min(values: List[float], index: int) -> float:
    if index == 0:
        return 0.0
    delta = values[index] - values[index - 1]
    return delta / (STEP_SECONDS / 60.0)


def _write_rows(handle, rows: Iterable[Dict[str, object]]) -> None:
    writer = csv.DictWriter(handle, fieldnames=FIELDNAMES)
    writer.writeheader()
    for row in rows:
        writer.writerow(row)


def generate_rows(rows_per_scenario: int) -> List[Dict[str, object]]:
    rows: List[Dict[str, object]] = []
    timestamp = 0
    for scenario in SCENARIOS:
        glucose_track: List[float] = []
        scenario_rows: List[Dict[str, object]] = []
        for step in range(rows_per_scenario):
            glucose = float(scenario.glucose_curve(step, rows_per_scenario))
            glucose_track.append(glucose)
            scenario_rows.append(
                {
                    "scenario": scenario.name,
                    "timestamp": timestamp,
                    "glucose_mgdl": round(_clamp(glucose, 40.0, 400.0), 2),
                    "sqi_pct": round(_clamp(float(scenario.sqi_curve(step, rows_per_scenario)), 0.0, 100.0), 2),
                    "cgm_age_s": round(max(0.0, float(scenario.cgm_age_curve(step, rows_per_scenario))), 2),
                    "iob_u": round(_clamp(float(scenario.iob_curve(step, rows_per_scenario)), 0.0, 20.0), 3),
                    "cob_g": round(_clamp(float(scenario.cob_curve(step, rows_per_scenario)), 0.0, 200.0), 3),
                    "activity_state": scenario.activity_curve(step, rows_per_scenario),
                    "activity_confidence_pct": round(_clamp(float(scenario.activity_confidence_curve(step, rows_per_scenario)), 0.0, 100.0), 2),
                    "motion_rms_5m": round(_clamp(float(scenario.motion_5_curve(step, rows_per_scenario)), 0.0, 200.0), 3),
                    "motion_rms_15m": round(_clamp(float(scenario.motion_15_curve(step, rows_per_scenario)), 0.0, 200.0), 3),
                    "active_minutes": round(_clamp(float(scenario.active_minutes_curve(step, rows_per_scenario)), 0.0, 1440.0), 2),
                    "post_exercise_minutes": round(_clamp(float(scenario.post_exercise_curve(step, rows_per_scenario)), 0.0, 1440.0), 2),
                    "basal_u_per_hr": round(_clamp(float(scenario.basal_curve(step, rows_per_scenario)), 0.0, 5.0), 3),
                    "insulin_30m_u": round(_clamp(float(scenario.insulin_30_curve(step, rows_per_scenario)), 0.0, 20.0), 3),
                    "insulin_120m_u": round(_clamp(float(scenario.insulin_120_curve(step, rows_per_scenario)), 0.0, 40.0), 3),
                    "carbs_30m_g": round(_clamp(float(scenario.carbs_30_curve(step, rows_per_scenario)), 0.0, 200.0), 3),
                    "carbs_120m_g": round(_clamp(float(scenario.carbs_120_curve(step, rows_per_scenario)), 0.0, 300.0), 3),
                    "physiology_present": "1" if scenario.physiology_present else "0",
                    "trend_mgdl_min": round(_trend_mgdl_min(glucose_track, step), 3),
                }
            )
            timestamp += STEP_SECONDS
        rows.extend(scenario_rows)
    return rows


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Generate deterministic synthetic replay data for Predictor V2.")
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("data/training/synthetic_predictor_v2.csv"),
        help="Output CSV path.",
    )
    parser.add_argument(
        "--rows-per-scenario",
        type=int,
        default=DEFAULT_ROWS_PER_SCENARIO,
        help="Number of rows to generate for each scenario block.",
    )
    return parser


def main() -> int:
    parser = _build_parser()
    args = parser.parse_args()
    if args.rows_per_scenario < 12:
        raise SystemExit("rows-per-scenario must be at least 12")

    rows = generate_rows(args.rows_per_scenario)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        _write_rows(handle, rows)
    print(f"wrote {len(rows)} rows to {args.output}")
    for scenario in SCENARIOS:
        print(f"scenario: {scenario.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
