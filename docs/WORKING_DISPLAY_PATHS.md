# Working Display Paths

## Goal
Capture the exact LCD/UI paths that are currently known-good versus known-failing so future APS demos only use proven-safe display behavior.

## Dashboard Object Table

| Display Element | Parent | Created Where | Updated Where | Update Type | Known LCD Result | Notes |
|---|---|---|---|---|---|---|
| Background image | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_image_create(screen)` | static only | image source set once | passed | Core baseline visual; safest path |
| Glucose panel | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_obj_create(screen)` | static only | style/layout only | passed | Existing panel container is stable |
| Glucose row | `panel` child | `edgeai_insulin_pump_app_start()` via `lv_obj_create(panel)` | static only | flex layout only | passed | Stable row container |
| `glucose_label` | `row` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(row)` | `update_glucose_label()` | dynamic numeric text | passed | Frequently updated and currently safe; good candidate for numeric-path reuse only if existing semantics remain intact |
| `glucose_shadow_label` | not created in current baseline | set to `NULL` in `edgeai_insulin_pump_app_start()` | `update_glucose_label()` checks for it | none in current baseline | not used | No active shadow label object in current source |
| `glucose_unit_label` | `row` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(row)` | static only | fixed text `MG/DL` | passed | Static label; no failure observed |
| `glucose_title_label` | `panel` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(panel)` | static only | fixed text `GLUCOSE LEVEL` | passed | Static label; no failure observed |
| Status bars | `screen` children | `edgeai_insulin_pump_app_start()` via `lv_bar_create(screen)` | `update_status_bars()` | dynamic bar value + color | passed | Existing dynamic bar path appears safe so far; strong APS reuse candidate |
| `CONF` label | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(screen)` | static only | fixed text | passed | Static label; moved near left of bar earlier and remained safe |
| `ACC` label | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(screen)` | static only | fixed text | passed | Static label; safe as static text |
| Chart | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_chart_create(screen)` | `push_sample()` | dynamic chart redraw | passed in baseline | Core baseline chart path works as long as fragile label mutations are avoided |
| Glucose chart series | chart-owned series | `lv_chart_add_series(chart, ...)` | `push_sample()` | `lv_chart_set_next_value()` | passed in baseline | Existing chart data feed is safe in baseline |
| Prediction chart series | chart-owned series | `lv_chart_add_series(chart, ...)` | `push_sample()` | `lv_chart_set_next_value()` | passed in baseline | Existing prediction line path is safe in baseline |
| `prediction_label` | `chart` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(chart)` | static in baseline | fixed text `EdgeAI Prediction` | baseline passed; text mutation failed | Chart-subtree text path is fragile; not a safe APS reuse candidate right now |
| `prediction_accuracy_label` baseline design | originally `chart` child in earlier baseline experiments | `edgeai_insulin_pump_app_start()` | `push_sample()` | dynamic percent text via `lv_label_set_text()` | failed when mutated in chart subtree | Chart-child invalidation path is unsafe |
| `prediction_accuracy_label` current good design | `screen` child | `edgeai_insulin_pump_app_start()` via `lv_label_create(screen)` | `push_sample()` | dynamic percent text via `lv_label_set_text()` | passed with original percent behavior | This is the only proven-safe mutable label path, but only with original `%d%%` behavior intact |

## Parent/Update Notes

- `glucose_label`, `glucose_unit_label`, and `glucose_title_label` live outside the chart subtree.
- `prediction_label` still lives inside the chart subtree.
- `prediction_accuracy_label` was successfully moved out of the chart subtree to a `screen` child.
- The known-good baseline keeps `prediction_accuracy_label` using the original percent-shaped update path in `push_sample()`.

## Known Failures

| Experiment | Result | Interpretation |
|---|---|---|
| New `1x1` `lv_obj_create(screen)` object | failed | Adding even one extra object can trip the current render stack |
| Chart-child `prediction_label` text change | failed | Chart-subtree text mutation is unsafe |
| Chart-child `prediction_accuracy_label` text change | failed | Chart-subtree invalidation + refresh path is unsafe |
| Screen-level `prediction_accuracy_label` with original percent behavior | passed | Structural reparenting fixed one failure path |
| Screen-level `prediction_accuracy_label` changed to `APS 110` | failed | Arbitrary text mutation is still unsafe even on the screen-level path |
| Comment-only no-op rebuild | passed | Rebuild alone does not kill LCD |
| Compiled no-op change | passed | Small code/data layout changes alone do not kill LCD |

## Recommendation

### Best Next Display Experiment
Reuse an original numeric/percent-shaped display path only.

### Safest Candidates
- `prediction_accuracy_label`, but keep output numeric/percent-like first
- existing status bars, by changing only numeric inputs or bar values

### Example Next Experiment
Change only the accuracy value source while preserving the existing `"%d%%"` format.

### Avoid For Now
- arbitrary text such as `APS 110`
- new LVGL objects
- chart-child labels
- any new LVGL text path for APS overlays
