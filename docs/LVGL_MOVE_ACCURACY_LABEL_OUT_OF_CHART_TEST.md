# LVGL Move Accuracy Label Out of Chart Test

## Goal
Test whether the LCD failure is caused by `gDashboard.prediction_accuracy_label` living inside the chart subtree while `push_sample()` mutates its text and refreshes the chart.

## Baseline
- Source baseline: `087abe1`
- Branch: `move-accuracy-label-out-of-chart`
- Known-good restore point: `087abe1`

## Exact Code Change
In `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`:
- Changed `gDashboard.prediction_accuracy_label` parent from `gDashboard.chart` to `screen`
- Kept the label itself, the `"0%"` initial text, font, text color, and shadow styling
- Kept the existing `push_sample()` update path unchanged: `lv_label_set_text(gDashboard.prediction_accuracy_label, header_buffer);`
- Replaced the old chart-relative alignment with a fixed screen position near the original visual location: `lv_obj_set_pos(label, 500, 338);`

## Build / Program Evidence
- Clean build: passed
- Program: passed
- OpenOCD pre-reset: healthy; detected `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`, flash banks visible
- OpenOCD post-reset: healthy; detected `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`, flash banks visible

## Physical LCD Result
- Pending physical verification

## Stop Condition
Do not commit this experiment as success until the physical LCD result is observed on hardware.

## Confirmed Physical Result
- Physical LCD result: `LCD live / GUI visible and accuracy label appears`
- Conclusion: moving `gDashboard.prediction_accuracy_label` out of the chart subtree preserves the LCD while keeping the existing `lv_label_set_text()` update path unchanged.
