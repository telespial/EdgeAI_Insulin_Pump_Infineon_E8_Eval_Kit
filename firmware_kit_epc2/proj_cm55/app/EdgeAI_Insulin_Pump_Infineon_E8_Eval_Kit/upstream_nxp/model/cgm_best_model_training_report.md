# Generic Training Report

- created_at: 2026-02-24T04:54:46.067922+00:00
- input: data/preloaded/insulin_pump/d1namo_train_excl001.csv
- target: glucose_mgdl_future_15m
- task: regression
- model: ridge
- rows_total: 6759
- rows_train: 5407
- rows_test: 1352

## Metrics
```json
{
  "mae": 8.315212474468533,
  "rmse": 14.830753724674057,
  "r2": 0.9582884300534537
}
```

## Holdout Metrics
- input: data/preloaded/insulin_pump/d1namo_test_subject001.csv
- rows: 1435
```json
{
  "mae": 7.421604172662143,
  "rmse": 10.4367135941972,
  "r2": 0.9858453047058702
}
```

## Feature Columns
- numeric: epoch_s, glucose_mgdl, insulin_fast_u, insulin_slow_u, insulin_total_u, glucose_mgdl_lag1, glucose_mgdl_lag2, glucose_mgdl_lag3, insulin_total_u_lag1, insulin_total_u_lag2, insulin_total_u_lag3, insulin_fast_u_lag1, insulin_fast_u_lag2, insulin_fast_u_lag3, insulin_slow_u_lag1, insulin_slow_u_lag2, insulin_slow_u_lag3, glucose_delta1, glucose_delta2, glucose_accel, glucose_roll3, glucose_roll6
- categorical: (none)
