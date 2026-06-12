# Predictor V2 Feature Schema

Predictor V2 uses a fixed 36-feature vector. The order below is stable and should be treated as part of the export contract.

| Index | Feature | Units | Source | Expected Range | Fallback / Default |
| --- | --- | --- | --- | --- | --- |
| 0 | current_glucose | mg/dL | Current CGM sample | 40–400 | Reject invalid input |
| 1 | lag_1 | mg/dL | Previous CGM sample | 40–400 | Current glucose |
| 2 | lag_2 | mg/dL | Two samples back | 40–400 | Current glucose |
| 3 | lag_3 | mg/dL | Three samples back | 40–400 | Current glucose |
| 4 | lag_6 | mg/dL | Six samples back | 40–400 | Current glucose |
| 5 | lag_9 | mg/dL | Nine samples back | 40–400 | Current glucose |
| 6 | lag_12 | mg/dL | Twelve samples back | 40–400 | Current glucose |
| 7 | delta_1 | mg/dL | Current minus lag 1 | -300 to 300 | 0 |
| 8 | delta_3 | mg/dL | Current minus lag 3 | -300 to 300 | 0 |
| 9 | delta_6 | mg/dL | Current minus lag 6 | -300 to 300 | 0 |
| 10 | slope_15m | mg/dL/min | Short-term slope from 3-sample lag | -10 to 10 | Trend input |
| 11 | slope_30m | mg/dL/min | Medium-term slope from 6-sample lag | -10 to 10 | Trend input |
| 12 | slope_60m | mg/dL/min | Long-term slope from 12-sample lag | -10 to 10 | Trend input |
| 13 | rolling_mean_3 | mg/dL | Mean of current + two prior valid samples | 40–400 | Current glucose |
| 14 | rolling_mean_6 | mg/dL | Mean of current + five prior valid samples | 40–400 | Current glucose |
| 15 | rolling_mean_12 | mg/dL | Mean of current + eleven prior valid samples | 40–400 | Current glucose |
| 16 | rolling_variance_6 | (mg/dL)^2 | Variance over six-sample window | 0–2000 | 0 |
| 17 | rolling_stddev_6 | mg/dL | Standard deviation over six-sample window | 0–50 | 0 |
| 18 | volatility_score | unitless | Composite of stddev and recent deltas | 0–100 | 0 |
| 19 | iob | U | Physiology estimate | 0–20 | 0 |
| 20 | cob | g | Physiology estimate | 0–200 | 0 |
| 21 | basal_rate | U/hr | Current basal estimate | 0–5 | 0 |
| 22 | insulin_30m | U | Physiology accumulation | 0–20 | 0 |
| 23 | insulin_120m | U | Physiology accumulation | 0–40 | 0 |
| 24 | carbs_30m | g | Physiology accumulation | 0–200 | 0 |
| 25 | carbs_120m | g | Physiology accumulation | 0–300 | 0 |
| 26 | sqi | % | Current sensor quality | 0–100 | Current SQI |
| 27 | cgm_age_s | s | Replay helper / capture age | 0–3600 | 0 |
| 28 | tod_sin | unitless | Time-of-day encoding | -1 to 1 | 0 |
| 29 | tod_cos | unitless | Time-of-day encoding | -1 to 1 | 1 |
| 30 | activity_state | enum | Physiology estimate | 0–5 | Unknown / median fallback |
| 31 | activity_confidence | % | Physiology estimate | 0–100 | 0 |
| 32 | motion_rms_5m | mg | Physiology estimate | 0–200 | 0 |
| 33 | motion_rms_15m | mg | Physiology estimate | 0–200 | 0 |
| 34 | active_minutes | min | Physiology estimate | 0–1440 | 0 |
| 35 | post_exercise_minutes | min | Physiology estimate | 0–1440 | 0 |

## Horizon Tables

Each horizon table stores:

- `feature_mean[36]`
- `feature_scale[36]`
- `feature_median[36]`
- `coefficients[36]`
- `intercept`
- `valid`

The generated model tables live in `firmware/include/predictor_v2_generated.h` and `firmware/src/predictor_v2_generated.c`.

## Fallback Rules

- Missing history and missing physiology features use the median/default values during evaluation.
- Poor SQI, stale CGM, or invalid model tables force a linear-trend fallback.
- Outputs are clamped to a safe glucose range before they reach the controller and safety supervisor.

## Training and Export Notes

- Host-side training scripts live in `tools/train_predictor_v2.py`, `tools/export_predictor_v2.py`, and `tools/validate_predictor_export.py`.
- The training pipeline consumes replay-style CSV rows and derives the 36-feature history vector from the sample sequence.
- Supported smoke columns include:
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
- The exported host artifacts are written under `generated/` and mirror the firmware table shape without changing embedded runtime behavior.
