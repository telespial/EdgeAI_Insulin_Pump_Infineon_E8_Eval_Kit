#!/usr/bin/env python3
"""Summarize Predictor V2 generated export artifacts."""

from __future__ import annotations

import argparse
import json
from datetime import datetime
from pathlib import Path
from typing import Dict, List


def _build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Summarize Predictor V2 generated export artifacts.")
    parser.add_argument(
        "--generated-dir",
        type=Path,
        default=Path("generated"),
        help="Directory containing generated Predictor V2 export files.",
    )
    parser.add_argument(
        "--out",
        type=Path,
        default=Path("generated/predictor_v2_export_manifest.md"),
        help="Output markdown manifest path.",
    )
    return parser


def _load_json(path: Path) -> Dict[str, object]:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def _count_defaults(values: List[float], default_value: float = 0.0) -> int:
    return sum(1 for value in values if abs(float(value) - default_value) <= 1e-12)


def main() -> int:
    parser = _build_argument_parser()
    args = parser.parse_args()

    generated_dir = args.generated_dir
    model_path = generated_dir / "predictor_v2_model.json"
    header_path = generated_dir / "predictor_v2_generated.h"
    source_path = generated_dir / "predictor_v2_generated.c"
    training_report_path = generated_dir / "predictor_v2_training_report.md"
    validation_report_path = generated_dir / "predictor_v2_validation_report.md"
    training_artifact_path = generated_dir / "predictor_v2_training.json"

    model_artifact = _load_json(model_path)
    training_artifact = _load_json(training_artifact_path) if training_artifact_path.exists() else model_artifact

    models = model_artifact.get("models", {})
    lines = [
        "# Predictor V2 Export Manifest",
        "",
        f"- Generated directory: `{generated_dir}`",
        f"- Training artifact: `{training_artifact_path}`" if training_artifact_path.exists() else "- Training artifact: not available",
        f"- Model artifact: `{model_path}`",
        f"- Header: `{header_path}`",
        f"- Source: `{source_path}`",
        f"- Training report: `{training_report_path}`",
        f"- Validation report: `{validation_report_path}`",
        f"- Export timestamp: {datetime.fromtimestamp(model_path.stat().st_mtime).isoformat(timespec='seconds') if model_path.exists() else 'unknown'}",
        "",
        "## Schema",
        f"- Feature count: {int(model_artifact.get('feature_count', 0))}",
        f"- Horizons present: {', '.join(sorted(models.keys())) if isinstance(models, dict) else 'unknown'}",
    ]

    for horizon in ("15m", "30m", "60m"):
        model = models.get(horizon, {})
        means = list(model.get("feature_mean", [])) if isinstance(model, dict) else []
        scales = list(model.get("feature_scale", [])) if isinstance(model, dict) else []
        medians = list(model.get("feature_median", [])) if isinstance(model, dict) else []
        coefficients = list(model.get("coefficients", [])) if isinstance(model, dict) else []
        lines.extend(
            [
                "",
                f"## {horizon}",
                f"- Intercept present: {'yes' if 'intercept' in model else 'no'}",
                f"- Coefficient count: {len(coefficients)}",
                f"- Mean count: {len(means)}",
                f"- Scale count: {len(scales)}",
                f"- Median/default count: {len(medians)}",
                f"- Zero/default means: {_count_defaults(means)}",
                f"- Zero/default scales: {_count_defaults(scales, 1.0)}",
                f"- Zero/default medians: {_count_defaults(medians)}",
                f"- C/H compatibility: {'yes' if header_path.exists() and source_path.exists() else 'no'}",
            ]
        )

    source_files = training_artifact.get("source_files", [])
    lines.extend(
        [
            "",
            "## Training Source",
            f"- Source files: {', '.join(source_files) if source_files else 'unknown'}",
            f"- Total rows: {training_artifact.get('total_rows', 'unknown')}",
            f"- Holdout ratio: {training_artifact.get('holdout_ratio', 'unknown')}",
            f"- Ridge: {training_artifact.get('ridge', 'unknown')}",
        ]
    )

    if isinstance(training_artifact.get("summary"), str):
        lines.extend(["", "## Summary", str(training_artifact["summary"])])

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(f"generated manifest: {args.out}")
    print(f"feature_count={int(model_artifact.get('feature_count', 0))}")
    print(f"horizons={', '.join(sorted(models.keys())) if isinstance(models, dict) else 'unknown'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
