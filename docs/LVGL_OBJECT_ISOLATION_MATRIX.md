
## Experiment: Move Accuracy Label Out of Chart Subtree

| Test | Change | Build | Program | OpenOCD | Physical LCD | Result | Notes |
|---|---|---|---|---|---|---|---|
| Move Accuracy Label Out of Chart | Existing `gDashboard.prediction_accuracy_label` parent changed from `chart` to `screen`; same update path retained | passed | passed | healthy pre/post reset-run (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`) | LCD live / GUI visible and accuracy label appears | passed | No new objects beyond relocating the existing label; `lv_label_set_text()` path unchanged |

## Confirmed Boundary
- `prediction_accuracy_label` as a chart child: LCD can fail during text mutation / chart refresh interaction.
- `prediction_accuracy_label` as a screen child: passed with the same text mutation path.

## Experiment: APS Screen-Level Accuracy Label Placeholder

| Test | Change | Build | Program | OpenOCD | Physical LCD | Result | Notes |
|---|---|---|---|---|---|---|---|
| APS Screen Label Readout V1 | Reused existing screen-level `gDashboard.prediction_accuracy_label`; compile-gated `header_buffer` forced to `APS 110` | passed | passed | healthy pre/post reset-run (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`) | LCD blank / dead / frozen | failed | Existing `lv_label_set_text()` call unchanged; no new objects, timers, or APS math |

## Current Boundary
- Structural reparenting of the frequently updated accuracy label from chart child to screen child is safe.
- Arbitrary runtime text mutation on that same label path is still unsafe under the current render stack.

## Working-Path Guidance
- Reuse only proven numeric/percent-shaped update paths for the next display experiment.
- Prefer existing status bars or the original percent label path before any new APS wording.
