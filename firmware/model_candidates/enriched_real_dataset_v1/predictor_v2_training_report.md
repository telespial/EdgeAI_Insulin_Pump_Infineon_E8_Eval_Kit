# Predictor V2 Training Report

- Feature count: 36
- Feature schema: current_glucose, lag_1, lag_2, lag_3, lag_6, lag_9, lag_12, delta_1, delta_3, delta_6, slope_15m, slope_30m, slope_60m, rolling_mean_3, rolling_mean_6, rolling_mean_12, rolling_variance_6, rolling_stddev_6, volatility_score, iob, cob, basal_rate, insulin_30m, insulin_120m, carbs_30m, carbs_120m, sqi, cgm_age_s, tod_sin, tod_cos, activity_state, activity_confidence, motion_rms_5m, motion_rms_15m, active_minutes, post_exercise_minutes
- Source files: 1
- Total rows: 8221
- Holdout ratio: 0.2
- Ridge: 0.001

## Holdout Metrics
- 15m: MAE 8.96, RMSE 22.78
- 30m: MAE 16.31, RMSE 28.74
- 60m: MAE 27.89, RMSE 40.07

## Horizon Summary
- 15m: train MAE 7.16, train RMSE 11.87, holdout MAE 8.96, holdout RMSE 22.78
- 30m: train MAE 15.15, train RMSE 22.63, holdout MAE 16.31, holdout RMSE 28.74
- 60m: train MAE 29.55, train RMSE 41.43, holdout MAE 27.89, holdout RMSE 40.07

## Notes
- Export is deterministic for a fixed training artifact.
- Generated sources mirror the firmware table layout but live in `generated/`.
- The model remains a linear regressor; no neural network is introduced here.
