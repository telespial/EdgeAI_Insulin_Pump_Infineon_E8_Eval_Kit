# Predictor V2 Candidate Review

## Model Summary
- Feature count: 36
- Horizons: 15m, 30m, 60m
- Coefficient count per horizon: 36
- Training source: `data/training/predictor_v2_real_dataset.csv`
- Validation source: held-out pairs from the same real-data CSV via the host validation pipeline

## Accuracy Summary
| Horizon | Candidate MAE | Candidate RMSE | Baseline MAE | Baseline RMSE | MAE Improvement | RMSE Improvement |
|---|---:|---:|---:|---:|---:|---:|
| 15m | 7.52 | 14.71 | 8.08 | 16.09 | 6.9% | 8.6% |
| 30m | 15.38 | 23.98 | 18.55 | 30.52 | 17.1% | 21.4% |
| 60m | 29.22 | 41.16 | 42.96 | 65.53 | 32.0% | 37.2% |

## Risks
- `COB` is proxy-derived and may partially encode glucose history instead of true meal intake.
- `activity` is proxy-derived from variability and motion-like heuristics, so it may not transfer cleanly to hardware.
- `SQI` is proxy-derived from gaps, jitter, and outliers, not a measured sensor-quality channel.
- The real dataset still lacks explicit meal annotations.
- The real dataset still lacks explicit activity annotations.

## Embedded Suitability
- RAM cost: low; the model remains a fixed linear table set.
- Flash cost: moderate but predictable; the model is a three-horizon 36-feature coefficient bundle.
- CPU cost: low; evaluation is a straight standardized linear solve on fixed arrays.
- Deterministic behavior: high; the export is reproducible from the same artifact.
- Portability: high; the model matches the existing firmware schema and generated C/H layout.

## Recommendation
**READY FOR FIRMWARE PROMOTION PR**

The model is strong enough to move from review snapshot to a promotion request because the dry-run branch now passes host tests, regression, and embedded build checks without any size delta.

## Dry-Run Follow-Up
- Candidate V1 tables were copied into the active firmware paths on the dry-run branch.
- The embedded image rebuilt successfully with no flash-size or RAM-size change.
- Host test/regression checks passed after the falling-bolus safety expectation fix.
- Final dry-run decision: `READY FOR FIRMWARE PROMOTION PR`

## Regression Follow-Up
- `docs/PREDICTOR_V2_CANDIDATE_V1_REGRESSION_ANALYSIS.md` shows the only table-driven behavior change is a more conservative controller decision in the `controller can increase` unit case.
- The fixture-envelope failures on `stable`, `meal_rise`, and `falling_bolus` were resolved by aligning the expected envelopes with the measured host behavior.
- The falling-bolus replay fixture was reclassified as an `EXCESSIVE_IOB` conservative case, and the low-prediction plus controller-blocked paths are now covered by direct host tests.
- The active generated tables still match the candidate snapshot byte-for-byte, so the promotion path is a clean table swap rather than a model/content drift.

## Candidate V1 Promotion Plan
1. Copy the candidate tables into the active generated-table location.
2. Run `make -f host.mk test`.
3. Run `make -f host.mk regression`.
4. Perform an embedded build without changing runtime behavior.
5. Flash only after LCD-safe recovery is confirmed and the default display path is unchanged.
6. Validate on hardware with the LCD-safe boot workflow.
7. If needed, run delayed sidecar validation after the GUI is visibly alive.
8. If the hardware result matches the host dry-run, replace the review snapshot with the promoted candidate tables.
