# Predictor V2 Export Manifest

- Generated directory: `generated`
- Training artifact: `generated/predictor_v2_training.json`
- Model artifact: `generated/predictor_v2_model.json`
- Header: `generated/predictor_v2_generated.h`
- Source: `generated/predictor_v2_generated.c`
- Training report: `generated/predictor_v2_training_report.md`
- Validation report: `generated/predictor_v2_validation_report.md`
- Export timestamp: 2026-06-12T10:56:55

## Schema
- Feature count: 36
- Horizons present: 15m, 30m, 60m

## 15m
- Intercept present: yes
- Coefficient count: 36
- Mean count: 36
- Scale count: 36
- Median/default count: 36
- Zero/default means: 0
- Zero/default scales: 0
- Zero/default medians: 6
- C/H compatibility: yes

## 30m
- Intercept present: yes
- Coefficient count: 36
- Mean count: 36
- Scale count: 36
- Median/default count: 36
- Zero/default means: 0
- Zero/default scales: 0
- Zero/default medians: 6
- C/H compatibility: yes

## 60m
- Intercept present: yes
- Coefficient count: 36
- Mean count: 36
- Scale count: 36
- Median/default count: 36
- Zero/default means: 0
- Zero/default scales: 0
- Zero/default medians: 6
- C/H compatibility: yes

## Training Source
- Source files: data/training/predictor_v2_real_dataset.csv
- Total rows: 8221
- Holdout ratio: 0.2
- Ridge: 0.001

## Summary
rows=8221; 15m:mae=8.96,rmse=22.78; 30m:mae=16.31,rmse=28.74; 60m:mae=27.89,rmse=40.07
