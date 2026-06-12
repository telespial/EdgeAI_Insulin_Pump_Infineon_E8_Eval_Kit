# Training Data Notes

This folder is for host-side Predictor V2 training and export smoke data.

## Current Smoke Dataset

- `smoke_predictor_v2.csv` is a tiny synthetic replay sequence used to prove the pipeline runs end-to-end.
- It is not medically useful and should not be treated as a real CGM training set.
- It exists only to exercise parsing, feature extraction, export, and validation.

## Real Training Requirements

Real Predictor V2 training needs a much larger dataset with:

- continuous CGM samples
- aligned IOB and COB estimates
- activity labels and confidence
- replay timestamps that preserve sequence order
- separate held-out data for validation

Use the smoke dataset only as a build and CLI sanity check.
