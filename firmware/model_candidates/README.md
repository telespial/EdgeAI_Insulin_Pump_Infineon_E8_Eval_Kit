# Model Candidates

This directory holds review-only Predictor V2 export snapshots.

Use it to stage generated coefficient sets, reports, and manifests for comparison before replacing the active embedded defaults.

Rules:

- Do not wire candidate files into the firmware build automatically.
- Do not replace the active `firmware/include/predictor_v2_generated.h` or `firmware/src/predictor_v2_generated.c` tables here.
- Keep candidate snapshots deterministic and clearly labeled by dataset and version.

Suggested snapshot layout:

- `firmware/model_candidates/<name>/predictor_v2_generated.h`
- `firmware/model_candidates/<name>/predictor_v2_generated.c`
- `firmware/model_candidates/<name>/predictor_v2_training_report.md`
- `firmware/model_candidates/<name>/predictor_v2_validation_report.md`
- `firmware/model_candidates/<name>/predictor_v2_export_manifest.md`

