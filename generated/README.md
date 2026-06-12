# Generated Predictor V2 Artifacts

This directory is intentionally reserved for reproducible host-side outputs from the Predictor V2 training/export pipeline.

## Policy

- Generated files are not committed by default.
- Regenerate them with:
  - `python3 tools/train_predictor_v2.py`
  - `python3 tools/export_predictor_v2.py`
  - `python3 tools/validate_predictor_export.py`
- Keep the source scripts, docs, and smoke dataset under version control; treat the contents of this directory as disposable build output unless a release note explicitly says otherwise.

