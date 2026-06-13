
## Follow-on Experiment: Move Accuracy Label Out of Chart
- Running branch: `move-accuracy-label-out-of-chart`
- Change under test: create `gDashboard.prediction_accuracy_label` as a child of `screen` instead of `gDashboard.chart` while keeping the existing `push_sample()` mutation path intact.
- Purpose: isolate whether chart-subtree invalidation plus `lv_chart_refresh()` is the real LCD kill path.
- Result: pending physical verification.

## Confirmed Result
- Moving `gDashboard.prediction_accuracy_label` from `chart` to `screen` passed on hardware.
- The existing `push_sample()` `lv_label_set_text(gDashboard.prediction_accuracy_label, header_buffer)` call stayed unchanged.
- This strongly points to chart-child label invalidation plus chart refresh/full-render/VG-Lite as the fragile path, not LVGL text mutation in general.
