# Training Data Sources

This page documents the real training data available for Predictor V2 work and how ready it is for host-side training.

## Dataset Inventory

| File | Rows | Size | Columns | Date Range | Sample Interval |
|---|---:|---:|---|---|---|
| `d1namo_train_excl001.csv` | 6,783 | 469,489 bytes | `subject`, `ts_iso`, `epoch_s`, `glucose_mgdl`, `glucose_mmol_l`, `glucose_type`, `insulin_fast_u`, `insulin_slow_u`, `insulin_total_u` | `2014-10-01T12:30:00Z` → `2014-10-05T07:00:00Z` | Mostly `300s`, with minor jitter and 44 duplicate timestamps |
| `d1namo_test_subject001.csv` | 1,438 | 99,812 bytes | `subject`, `ts_iso`, `epoch_s`, `glucose_mgdl`, `glucose_mmol_l`, `glucose_type`, `insulin_fast_u`, `insulin_slow_u`, `insulin_total_u` | `2014-10-01T19:14:00Z` → `2014-10-06T16:54:02Z` | Mostly `300s`, with minor jitter and no duplicate timestamps |
| `label_first_labels.json` | N/A | 434 bytes | `training_mode`, `labels`, `notes` | N/A | N/A |
| `label_first_template.csv` | 0 | 191 bytes | `label_key`, `motor_rpm`, `motor_current_ma`, `motor_step_error`, `motor_load_proxy`, `temp_c`, `ambient_c`, `temp_slope_c_per_min`, `accel_peak_g`, `gyro_peak_dps`, `baro_delta_hpa`, `inverted_flag`, `drop_event`, `activity_state` | N/A | Template only |

## Predictor-Relevant Fields

### Present

- `timestamp` equivalents: `epoch_s`, `ts_iso`
- `glucose`: `glucose_mgdl`, `glucose_mmol_l`
- `insulin`: `insulin_fast_u`, `insulin_slow_u`, `insulin_total_u`
- `sensor quality`: not present directly; only `glucose_type` is available

### Missing

- `trend` as an explicit field
- `basal` as an explicit per-hour rate
- `bolus` as an explicit event log
- `carbs`
- `activity`
- `exercise`
- `sensor quality`

## Data Quality

Observed from the two CSV datasets:

- Missing values: none in the CSV files inspected
- Duplicate timestamps: 44 in `d1namo_train_excl001.csv`, none in `d1namo_test_subject001.csv`
- Gaps / jitter: mostly 300-second spacing with small irregular offsets such as 1, 4, 58, 240, 242, 299, and 301 seconds
- Inconsistent units: none obvious in the CSV headers; glucose is provided in both `mg/dL` and `mmol/L`
- Corrupted rows: none observed during the audit

## Training Readiness

Classification: `PARTIALLY READY`

Reasoning:

- The datasets contain useful glucose and insulin history for sequence modeling.
- They do not contain direct carbohydrate, activity, exercise, or sensor-quality fields required for the full Predictor V2 feature set.
- A host-side importer can still derive a usable training CSV by applying documented defaults and proxies, but the result is not yet a full-fidelity real-world physiology dataset.
- The current importer is series-aware and uses proxy physiology fields derived from raw glucose/insulin dynamics, timestamp gaps, and glucose type.

## Notes

- The preferred real training source path is:
  - `/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump/`
- Synthetic and smoke datasets remain pipeline-validation only.
