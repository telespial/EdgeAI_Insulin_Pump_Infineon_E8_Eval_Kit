#!/usr/bin/env python3
"""Train the host-side Predictor V2 linear models."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict, List

from predictor_v2_pipeline import (
    FEATURE_COUNT,
    FEATURE_NAMES,
    HORIZON_SECONDS,
    build_training_pairs,
    feature_matrix,
    fit_linear_model,
    group_samples,
    model_to_dict,
    regression_metrics,
    save_json,
    split_pairs,
    statistics_from_rows,
    summarize_metrics,
)


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Train Predictor V2 from replay CSV files.")
    parser.add_argument(
        "--input",
        nargs="+",
        type=Path,
        required=True,
        help="One or more training CSV files with replay/feature columns.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=Path("generated/predictor_v2_training.json"),
        help="Training artifact output path.",
    )
    parser.add_argument(
        "--holdout-ratio",
        type=float,
        default=0.2,
        help="Fraction of eligible horizon pairs to reserve for holdout metrics.",
    )
    parser.add_argument(
        "--ridge",
        type=float,
        default=1e-3,
        help="Ridge regularization strength used for the closed-form fit.",
    )
    return parser


def _train_horizon(
    horizon: str,
    samples: List,
    means: List[float],
    scales: List[float],
    medians: List[float],
    holdout_ratio: float,
    ridge: float,
) -> Dict[str, object]:
    pairs = build_training_pairs(samples, HORIZON_SECONDS[horizon])
    if len(pairs) < 8:
        raise ValueError(f"{horizon}: not enough training pairs ({len(pairs)})")
    train_pairs, holdout_pairs = split_pairs(pairs, holdout_ratio)
    train_features = [row for row, _ in train_pairs]
    train_targets = [target for _, target in train_pairs]
    intercept, coefficients = fit_linear_model(
        train_features,
        train_targets,
        means=means,
        scales=scales,
        ridge=ridge,
    )

    train_predictions = [intercept + sum(
        coefficients[index] * ((row[index] - means[index]) / (scales[index] if abs(scales[index]) > 1e-12 else 1.0))
        for index in range(FEATURE_COUNT)
    ) for row in train_features]
    train_metrics = regression_metrics(train_predictions, train_targets)

    holdout_predictions = [
        intercept
        + sum(
            coefficients[index]
            * ((row[index] - means[index]) / (scales[index] if abs(scales[index]) > 1e-12 else 1.0))
            for index in range(FEATURE_COUNT)
        )
        for row, _ in holdout_pairs
    ]
    holdout_targets = [target for _, target in holdout_pairs]
    holdout_metrics = regression_metrics(holdout_predictions, holdout_targets)

    return {
        "model": model_to_dict(
            horizon=horizon,
            intercept=intercept,
            coefficients=coefficients,
            means=means,
            scales=scales,
            medians=medians,
            metrics={
                "train": train_metrics,
                "holdout": holdout_metrics,
            },
            train_rows=len(train_pairs),
            holdout_rows=len(holdout_pairs),
            ridge=ridge,
        ),
        "pair_count": len(pairs),
    }


def main() -> int:
    parser = _build_argument_parser()
    args = parser.parse_args()
    if not args.input:
        parser.error("at least one input CSV is required")

    training_rows, _, source_names = group_samples(args.input, holdout_ratio=0.0)
    if len(training_rows) < 2:
        raise SystemExit("not enough rows to build a feature matrix")

    feature_rows = feature_matrix(training_rows)
    means, scales, medians = statistics_from_rows(feature_rows)

    horizon_models: Dict[str, object] = {}
    horizon_summary: Dict[str, Dict[str, float]] = {}
    pair_counts: Dict[str, int] = {}
    for horizon in ("15m", "30m", "60m"):
        horizon_result = _train_horizon(
            horizon=horizon,
            samples=training_rows,
            means=means,
            scales=scales,
            medians=medians,
            holdout_ratio=args.holdout_ratio,
            ridge=args.ridge,
        )
        horizon_models[horizon] = horizon_result["model"]
        pair_counts[horizon] = horizon_result["pair_count"]
        horizon_summary[horizon] = horizon_result["model"]["metrics"]["holdout"]

    artifact = {
        "schema_version": 1,
        "feature_count": FEATURE_COUNT,
        "feature_names": FEATURE_NAMES,
        "source_files": source_names,
        "total_rows": len(training_rows),
        "feature_means": means,
        "feature_scales": scales,
        "feature_medians": medians,
        "pair_counts": pair_counts,
        "holdout_ratio": args.holdout_ratio,
        "ridge": args.ridge,
        "models": horizon_models,
        "holdout_summary": horizon_summary,
        "summary": summarize_metrics(len(training_rows), horizon_summary),
    }
    save_json(args.output, artifact)
    print(f"trained Predictor V2 on {len(training_rows)} rows from {len(source_names)} file(s)")
    for horizon in ("15m", "30m", "60m"):
        metrics = horizon_summary[horizon]
        print(
            f"{horizon}: holdout MAE={metrics['mae']:.2f} RMSE={metrics['rmse']:.2f} pairs={pair_counts[horizon]}"
        )
    print(f"artifact: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
