# Dataset Feature Audit

This audit covers the real insulin-pump source files used for the Predictor V2 real-data training pass:

- `/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump/d1namo_train_excl001.csv`
- `/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump/d1namo_test_subject001.csv`

Both CSVs share the same core schema, so the column-level audit is the same for each dataset.

## Column Audit

| Column | Type | Value Range | Missing % | Current Usage | Possible Predictor Value |
|---|---|---|---:|---|---|
| `subject` | categorical/string | `001`–`009` | 0% | Provenance only; used to form per-series IDs | Helps keep subjects separated during training |
| `ts_iso` | timestamp string | 2014-09-30 → 2014-10-06 | 0% | Timestamp provenance | Useful for ordering and audit trails |
| `epoch_s` | integer timestamp | 1412067600–1412614442 | 0% | Ordering / sample timing | Enables series spacing, gap detection, and horizon alignment |
| `glucose_mgdl` | numeric | 40–400 | 0% | Core glucose signal and training target history | Primary Predictor V2 signal |
| `glucose_mmol_l` | numeric | 2.2–22.2 | 0% | Redundant unit conversion | Useful for cross-checking data integrity |
| `glucose_type` | categorical | `cgm`, `manual` | 0% | Proxy for sensor quality / freshness | Helps derive SQI and CGM-age proxies |
| `insulin_fast_u` | numeric | 0–12 | 0% | Current IOB proxy input | Useful for improved decay-based IOB estimation |
| `insulin_slow_u` | numeric | 0–31 | 0% | Basal proxy input | Useful for basal-rate context |
| `insulin_total_u` | numeric | 0–35 | 0% | Longer-window insulin proxy input | Useful for smoothing total insulin context |

## Not Present In The Raw Datasets

The source files do not contain explicit columns for:

- `trend`
- `bolus`
- `carbs`
- `activity`
- `exercise`
- direct `sensor quality`

Those features are therefore derived as host-side proxies in the training importer.

## Data Quality Summary

- Missing values: none observed in the two CSV files
- Duplicate timestamps: 44 in `d1namo_train_excl001.csv`, none in `d1namo_test_subject001.csv`
- Gaps / jitter: mostly 300-second spacing with some irregular offsets
- Units: glucose is available in both `mg/dL` and `mmol/L`; insulin values are unit-based and internally consistent
- Corrupted rows: none observed during the audit

## Predictor V2 Interpretation

- `glucose_mgdl` remains the dominant signal and the training target history.
- `insulin_fast_u` and `insulin_slow_u` are converted into decay-based physiology proxies instead of being used as raw snapshots only.
- `glucose_type`, timestamp jitter, and long gaps are folded into SQI and CGM-age proxies.
- The dataset is still incomplete for full physiology fidelity, but it is now materially richer than the first importer pass.

