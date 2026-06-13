
## Experiment: Move Accuracy Label Out of Chart Subtree

| Test | Change | Build | Program | OpenOCD | Physical LCD | Result | Notes |
|---|---|---|---|---|---|---|---|
| Move Accuracy Label Out of Chart | Existing `gDashboard.prediction_accuracy_label` parent changed from `chart` to `screen`; same update path retained | passed | passed | healthy pre/post reset-run (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`) | pending | pending | No new objects beyond relocating the existing label; `lv_label_set_text()` path unchanged |

## Confirmed Boundary
- `prediction_accuracy_label` as a chart child: LCD can fail during text mutation / chart refresh interaction.
- `prediction_accuracy_label` as a screen child: passed with the same text mutation path.
