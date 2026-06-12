# Predictor V2 Training

This repo now has a host-only training and export pipeline for Predictor V2.

## Purpose

The goal is to train and export the linear Predictor V2 tables from replay-style CSV data without changing embedded runtime behavior.

## Scripts

- `tools/train_predictor_v2.py`
- `tools/export_predictor_v2.py`
- `tools/validate_predictor_export.py`
- `tools/generate_predictor_v2_synthetic_data.py`

## Supported Inputs

The training CSV accepts the core replay columns plus optional physiology fields:

- `timestamp`
- `glucose_mgdl`
- `sqi_pct`
- `cgm_age_s`
- `iob_u`
- `cob_g`
- `activity_state`
- `activity_confidence_pct`
- `motion_rms_5m`
- `motion_rms_15m`
- `active_minutes`
- `post_exercise_minutes`

The trainer derives the 36-feature Predictor V2 vector from sequence history. It also supports the remaining physiology fields already used by the firmware schema when they are available.

## Horizons

Separate linear models are trained for:

- `15m`
- `30m`
- `60m`

The pipeline does not derive the longer horizons from the shorter one.

## Outputs

The exported host artifacts are written to `generated/`:

- `generated/predictor_v2_generated.h`
- `generated/predictor_v2_generated.c`
- `generated/predictor_v2_model.json`
- `generated/predictor_v2_training_report.md`
- `generated/predictor_v2_validation_report.md`

The `generated/` directory is treated as reproducible build output and is ignored by default except for its own `README.md` and `.gitignore`.

## Smoke Data

The committed `data/training/smoke_predictor_v2.csv` file is only a pipeline smoke test.
It is not a real training corpus and should not be used for medical claims.

## Synthetic Dataset

The committed `data/training/synthetic_predictor_v2.csv` file is generated deterministically and currently contains `252` rows.
It includes the following scenario blocks:

- stable overnight
- meal rise
- falling after bolus
- exercise with IOB
- post-exercise recovery
- bad SQI
- stale CGM

Generate it again with:

```bash
python3 tools/generate_predictor_v2_synthetic_data.py --output data/training/synthetic_predictor_v2.csv
```

The synthetic dataset is committed because it is small enough to review and reproduce locally, but it remains host-only evidence rather than embedded runtime data.

## Real Training Source

The larger real training data is located at:

`/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump/`

The real-data importer is series-aware and now derives proxy physiology values for:

- `iob_u`
- `cob_g`
- `activity_state`
- `activity_confidence_pct`
- `motion_rms_5m`
- `motion_rms_15m`
- `active_minutes`
- `post_exercise_minutes`
- `sqi_pct`
- `cgm_age_s`

It also adds glucose-dynamics helpers such as rolling volatility, acceleration, range exposure, and recent excursion magnitude before training the existing 36-feature Predictor V2 model.
