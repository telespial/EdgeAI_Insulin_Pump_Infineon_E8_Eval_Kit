#!/usr/bin/env python3
"""Validate exported Predictor V2 models on held-out CSV data."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List, Sequence

from predictor_v2_pipeline import (
    HORIZON_SECONDS,
    baseline_linear_trend_delta,
    build_feature_vector,
    build_training_pairs,
    evaluate_model,
    group_samples,
    load_json,
    regression_metrics,
    save_json,
    split_pairs,
    summarize_metrics,
)


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Validate Predictor V2 exports on held-out replay data.")
    parser.add_argument(
        "--model",
        type=Path,
        default=Path("generated/predictor_v2_model.json"),
        help="Exported model JSON from export_predictor_v2.py.",
    )
    parser.add_argument(
        "--input",
        nargs="+",
        type=Path,
        required=True,
        help="Held-out CSV file(s) used for evaluation.",
    )
    parser.add_argument(
        "--holdout-ratio",
        type=float,
        default=0.0,
        help="Optional pair-level holdout split when the evaluation data is not pre-split.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("generated/predictor_v2_validation_report.md"),
        help="Validation summary report path.",
    )
    return parser


def _evaluate_horizon(
    horizon: str,
    model: Dict[str, object],
    samples: Sequence,
    holdout_ratio: float,
) -> Dict[str, float]:
    pairs = build_training_pairs(samples, HORIZON_SECONDS[horizon])
    if holdout_ratio > 0.0:
        _, pairs = split_pairs(pairs, holdout_ratio)
    if not pairs:
        return {"mae": 0.0, "rmse": 0.0, "baseline_mae": 0.0, "baseline_rmse": 0.0, "rows": 0}

    predictor_predictions: List[float] = []
    baseline_predictions: List[float] = []
    targets: List[float] = []
    horizon_minutes = HORIZON_SECONDS[horizon] / 60.0
    for feature_row, target in pairs:
        predictor_delta = evaluate_model(model, feature_row)
        current_glucose = feature_row[0]
        predictor_predictions.append(current_glucose + predictor_delta)
        baseline_predictions.append(current_glucose + feature_row[10] * horizon_minutes)
        targets.append(current_glucose + target)

    predictor_metrics = regression_metrics(predictor_predictions, targets)
    baseline_metrics = regression_metrics(baseline_predictions, targets)
    return {
        "mae": predictor_metrics["mae"],
        "rmse": predictor_metrics["rmse"],
        "baseline_mae": baseline_metrics["mae"],
        "baseline_rmse": baseline_metrics["rmse"],
        "rows": len(pairs),
    }


def main() -> int:
    parser = _build_argument_parser()
    args = parser.parse_args()
    artifact = load_json(args.model)
    models = artifact["models"]
    samples, _, _ = group_samples(args.input, holdout_ratio=0.0)
    if len(samples) < 2:
        raise SystemExit("not enough rows for validation")

    results = {}
    for horizon in ("15m", "30m", "60m"):
        results[horizon] = _evaluate_horizon(horizon, models[horizon], samples, args.holdout_ratio)

    report_lines = [
        "# Predictor V2 Validation Report",
        "",
        f"- Model: {args.model}",
        f"- Input files: {', '.join(str(path) for path in args.input)}",
        f"- Total rows: {len(samples)}",
        "",
        "## Metrics",
    ]
    for horizon in ("15m", "30m", "60m"):
        result = results[horizon]
        report_lines.append(
            f"- {horizon}: predictor MAE {result['mae']:.2f}, predictor RMSE {result['rmse']:.2f}, "
            f"baseline MAE {result['baseline_mae']:.2f}, baseline RMSE {result['baseline_rmse']:.2f}, "
            f"rows {result['rows']}"
        )
    report_lines.extend(
        [
            "",
            "## Summary",
            summarize_metrics(len(samples), {h: {"mae": results[h]["mae"], "rmse": results[h]["rmse"]} for h in results}),
        ]
    )
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

    print(f"validation report: {args.output}")
    for horizon in ("15m", "30m", "60m"):
        result = results[horizon]
        print(
            f"{horizon}: predictor MAE={result['mae']:.2f} RMSE={result['rmse']:.2f} "
            f"baseline MAE={result['baseline_mae']:.2f} RMSE={result['baseline_rmse']:.2f} rows={result['rows']}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
