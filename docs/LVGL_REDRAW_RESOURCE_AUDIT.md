
## Confirmed Hardware Evidence
- Reparenting `gDashboard.prediction_accuracy_label` from the chart subtree to the screen fixed the LCD failure without changing the existing label update call.
- Changing that same screen-level label text content to `APS 110` still killed the LCD even though no new LVGL objects, timers, or APS math were introduced.
- Strongest current hypothesis: the render stack is fragile both for chart-child invalidation and for arbitrary text mutation on the current frequently updated label path; the chart subtree is one confirmed failure amplifier, but it is not the only one.

## Updated Closeout
- Recovery image flashed from the known-good baseline with no APS text flags restored the LCD to `live / GUI visible`.
- The unsafe behavior is now specifically tied to LVGL text mutation for non-baseline strings on the dashboard label path.
- Recommended near-term strategy: avoid arbitrary LVGL text mutation for APS overlays until the render stack is fixed; prefer pre-rendered background art or existing numeric/bar paths only.

## Working Display Path Summary
- Safe baseline paths today are the background image, glucose panel/row, glucose numeric label path, screen-level status bars, and the chart/series update path when label behavior stays at the known-good baseline.
- The current `prediction_accuracy_label` path is only conditionally safe: it passes as a screen child when the original percent-shaped update behavior is preserved.
- Known unsafe paths include extra LVGL object creation, chart-child label mutation, and arbitrary string mutation on the screen-level `prediction_accuracy_label`.
