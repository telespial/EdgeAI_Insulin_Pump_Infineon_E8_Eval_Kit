# Predictor V2 Export Readiness

This review checks whether the enriched real-data Predictor V2 model is a credible candidate firmware coefficient set.

## Current Metrics

| Horizon | Predictor MAE | Predictor RMSE | Baseline MAE | Baseline RMSE |
|---|---:|---:|---:|---:|
| 15m | 7.52 | 14.71 | 8.08 | 16.09 |
| 30m | 15.38 | 23.98 | 18.55 | 30.52 |
| 60m | 29.22 | 41.16 | 42.96 | 65.53 |

## Relative Improvement

| Horizon | MAE Improvement | RMSE Improvement |
|---|---:|---:|
| 15m | 6.9% | 8.6% |
| 30m | 17.1% | 21.4% |
| 60m | 32.0% | 37.2% |

## Proxy Features Used

The enriched importer derives proxy physiology from the real insulin-pump data:

- decay-based `iob_u`
- proxy `cob_g`
- proxy `activity_state`
- proxy `activity_confidence_pct`
- proxy `motion_rms_5m`
- proxy `motion_rms_15m`
- proxy `active_minutes`
- proxy `post_exercise_minutes`
- proxy `sqi_pct`
- proxy `cgm_age_s`
- rolling volatility and excursion helpers

## Dataset Limitations

- The source data does not contain direct carbohydrate logs.
- The source data does not contain a true activity sensor stream.
- SQI and CGM age are proxies, not measured fields.
- The training data is still dominated by glucose history and insulin history.
- The dataset is real, but it is not yet a labeled physiology corpus.

## Overfitting Risks

- The proxy features are heuristics derived from the same glucose sequence used for training.
- The dataset is relatively small and narrow compared with production use.
- The strongest gains may partly reflect fitted signal shape rather than durable physiology knowledge.
- Extra proxy engineering could overfit faster than it improves hardware behavior.

## Recommendation

**Conditionally ready.**

The enriched real-data model is strong enough to become a review candidate for firmware coefficient replacement, but it should remain a candidate snapshot until the embedded build path is rechecked and the hardware baseline is confirmed against the same export.

Recommended next step:

- compare the candidate snapshot against the current embedded default tables and confirm the firmware still builds cleanly before any runtime wiring changes.

See also:

- `docs/PREDICTOR_V2_CANDIDATE_REVIEW.md`
- `docs/PREDICTOR_V2_FEATURE_IMPORTANCE.md`
- `generated/predictor_v2_table_comparison.md`
