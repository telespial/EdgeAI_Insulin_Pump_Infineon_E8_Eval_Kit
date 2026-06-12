#!/usr/bin/env python3
"""Export Predictor V2 training artifacts to generated C/H files."""

from __future__ import annotations

import argparse
from pathlib import Path
from typing import Dict

from predictor_v2_pipeline import (
    FEATURE_COUNT,
    FEATURE_NAMES,
    load_json,
    save_json,
    summarize_metrics,
    write_generated_header,
    write_generated_source,
)


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Export Predictor V2 models to generated sources.")
    parser.add_argument(
        "--artifact",
        type=Path,
        default=Path("generated/predictor_v2_training.json"),
        help="Training artifact produced by train_predictor_v2.py.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("generated"),
        help="Directory for the generated export files.",
    )
    return parser


def _validate_artifact(artifact: Dict[str, object]) -> None:
    if int(artifact.get("feature_count", 0)) != FEATURE_COUNT:
        raise ValueError("feature count mismatch")
    if artifact.get("feature_names") != FEATURE_NAMES:
        raise ValueError("feature schema mismatch")
    models = artifact.get("models")
    if not isinstance(models, dict):
        raise ValueError("missing models section")
    for horizon in ("15m", "30m", "60m"):
        model = models.get(horizon)
        if not isinstance(model, dict):
            raise ValueError(f"missing {horizon} model")
        if not model.get("valid", False):
            raise ValueError(f"{horizon} model is not valid")


def main() -> int:
    parser = _build_argument_parser()
    args = parser.parse_args()
    artifact = load_json(args.artifact)
    _validate_artifact(artifact)

    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)
    header_path = output_dir / "predictor_v2_generated.h"
    source_path = output_dir / "predictor_v2_generated.c"
    model_path = output_dir / "predictor_v2_model.json"
    report_path = output_dir / "predictor_v2_training_report.md"

    write_generated_header(header_path)
    write_generated_source(source_path, artifact["models"])
    save_json(model_path, artifact)

    holdout = artifact.get("holdout_summary", {})
    report_lines = [
        "# Predictor V2 Training Report",
        "",
        f"- Feature count: {artifact['feature_count']}",
        f"- Feature schema: {', '.join(FEATURE_NAMES)}",
        f"- Source files: {len(artifact.get('source_files', []))}",
        f"- Total rows: {artifact.get('total_rows', 0)}",
        f"- Holdout ratio: {artifact.get('holdout_ratio', 0.0)}",
        f"- Ridge: {artifact.get('ridge', 0.0)}",
        "",
        "## Holdout Metrics",
        f"- 15m: MAE {holdout.get('15m', {}).get('mae', 0.0):.2f}, RMSE {holdout.get('15m', {}).get('rmse', 0.0):.2f}",
        f"- 30m: MAE {holdout.get('30m', {}).get('mae', 0.0):.2f}, RMSE {holdout.get('30m', {}).get('rmse', 0.0):.2f}",
        f"- 60m: MAE {holdout.get('60m', {}).get('mae', 0.0):.2f}, RMSE {holdout.get('60m', {}).get('rmse', 0.0):.2f}",
        "",
        "## Horizon Summary",
    ]
    for horizon in ("15m", "30m", "60m"):
        model = artifact["models"][horizon]
        metrics = model["metrics"]
        report_lines.append(
            f"- {horizon}: train MAE {metrics['train']['mae']:.2f}, train RMSE {metrics['train']['rmse']:.2f}, "
            f"holdout MAE {metrics['holdout']['mae']:.2f}, holdout RMSE {metrics['holdout']['rmse']:.2f}"
        )
    report_lines.extend(
        [
            "",
            "## Notes",
            "- Export is deterministic for a fixed training artifact.",
            "- Generated sources mirror the firmware table layout but live in `generated/`.",
            "- The model remains a linear regressor; no neural network is introduced here.",
        ]
    )
    report_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8")

    print(f"generated: {header_path}")
    print(f"generated: {source_path}")
    print(f"generated: {model_path}")
    print(f"generated: {report_path}")
    print(f"summary: {summarize_metrics(int(artifact.get('total_rows', 0)), holdout)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
