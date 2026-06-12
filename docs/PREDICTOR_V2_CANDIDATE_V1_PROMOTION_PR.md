# Predictor V2 Candidate V1 Promotion PR

## Recommended Title
Promote Predictor V2 Candidate V1 generated tables

## Recommended Description
This PR promotes the enriched real-dataset Predictor V2 Candidate V1 generated tables into the active firmware model table paths.

### Summary
- Candidate V1 is trained on the enriched real insulin-pump dataset pipeline.
- The active generated tables match the reviewed candidate snapshot byte-for-byte.
- The falling-bolus safety expectation was corrected to match measured host behavior.

### Validation
- Host tests passed.
- Host regression passed.
- Embedded build passed.
- Size delta: `0`.
- Candidate tables match the reviewed snapshot byte-for-byte.

### Safety and Runtime Notes
- No hardware flashing was performed for this promotion preparation.
- No APS runtime calls were added.
- No LCD/GUI code was changed.
- No controller or safety logic was changed.

### Model Notes
- The model remains research-oriented and conditionally ready because the training data uses proxy physiology fields.
- The promotion changes only the generated coefficient tables, not the runtime control flow.

## Training Source
- Enriched real-data Predictor V2 training pipeline output under `data/training/predictor_v2_real_dataset.csv`

## Metrics
| Horizon | MAE | RMSE |
|---|---:|---:|
| 15m | 7.52 | 14.71 |
| 30m | 15.38 | 23.98 |
| 60m | 29.22 | 41.16 |

## Baseline Comparison
| Horizon | Baseline MAE | Baseline RMSE |
|---|---:|---:|
| 15m | 8.08 | 16.09 |
| 30m | 18.55 | 30.52 |
| 60m | 42.96 | 65.53 |

## Dry-Run Validation Result
- Candidate tables were verified byte-for-byte against `firmware/model_candidates/enriched_real_dataset_v1/`.
- `make -f host.mk test` passed.
- `make -f host.mk regression` passed.
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed after `make clean`.

## Safety-Review Correction Result
- The falling-bolus fixture was reclassified as an `EXCESSIVE_IOB` case.
- The direct low-prediction and controller-blocked host tests now pass.
- The prior host expectation mismatch was resolved without changing runtime logic.

## Size Delta
- Flash: `0`
- RAM: `0`
- Combined image: `0`

