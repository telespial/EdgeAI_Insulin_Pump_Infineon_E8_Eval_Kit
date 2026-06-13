# APS Screen Label Readout V1

## Goal
Validate the proven screen-level `gDashboard.prediction_accuracy_label` path by showing a compact APS-style placeholder without adding any new LVGL objects, timers, or APS logic.

## Branch / Baseline
- Branch: `aps-screen-label-readout-v1`
- Baseline commit: `6f8ac6c`

## Exact Code Change
In `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c` inside `push_sample()`:
- Kept the existing screen-level `gDashboard.prediction_accuracy_label`
- Kept the existing `lv_label_set_text(gDashboard.prediction_accuracy_label, header_buffer);` call unchanged
- Added compile-gated formatting only:

```c
#if defined(APP_APS_SCREEN_LABEL_READOUT) && (APP_APS_SCREEN_LABEL_READOUT == 1)
    snprintf(header_buffer, sizeof(header_buffer), "APS 110");
#else
    snprintf(header_buffer, sizeof(header_buffer), "%d%%", (int)accuracy_pct);
#endif
```

## Constraints Preserved
- No new LVGL objects
- No new labels
- No new timers
- No APS math
- No Predictor / Controller / Safety calls
- No chart logic changes
- No chart refresh changes

## Build / Program Evidence
- Build command: `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_SCREEN_LABEL_READOUT=1 -j8`
- Program command: `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_SCREEN_LABEL_READOUT=1`
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)

## Physical LCD Result
- Physical result: `LCD blank / dead / frozen`
- Recovery image: restored known-good source/build with no APS text flags, then reflashed successfully
- Recovery physical result: `LCD live / GUI visible`

## Conclusion
- The prior structural fix remains valid: moving `gDashboard.prediction_accuracy_label` out of the chart subtree is LCD-safe when the original percent-text behavior stays intact.
- Changing that same screen-level label text content to `APS 110` still kills the LCD.
- No APS math was involved.
- No new LVGL objects were created.
- Strongest conclusion: the current LVGL text-mutation path is unsafe for arbitrary APS display strings under this render stack, even on the screen-level label path.

## Next Safe Strategy
- Do not continue LVGL text experiments for APS display.
- Prefer pre-rendered APS background art or reuse of existing numeric/bar paths only.
