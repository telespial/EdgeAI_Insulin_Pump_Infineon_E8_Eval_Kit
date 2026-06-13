
## Confirmed Hardware Evidence
- Reparenting `gDashboard.prediction_accuracy_label` from the chart subtree to the screen fixed the LCD failure without changing the existing label update call.
- Strongest current hypothesis: chart-child label invalidation combined with `lv_chart_set_next_value()` + `lv_chart_refresh()` under the full-render VG-Lite path is the actual failure mode.
