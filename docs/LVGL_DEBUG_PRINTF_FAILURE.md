# LVGL Debug Printf Failure

## 2026-06-14 LVGL hot-path printf failure

### What changed
Temporary UART `printf` checkpoints were added in the active LVGL / dashboard runtime path, including:
- the CM55 loop around `lv_timer_handler()`
- `dashboard_timer_cb()`
- `push_sample()` / render-adjacent code paths

Temporary isolation compile gates were also introduced during the same investigation:
- `APP_LVGL_DISABLE_CENTER_GLUCOSE`
- `APP_LVGL_DISABLE_CHART_REFRESH`

### Observed hardware result
- Build: passed
- Program: passed
- OpenOCD reset-run before/after: healthy
- Physical LCD result with the hot-path debug image: LCD did not reliably light / later isolation images blanked the LCD

### Recovery result
After reverting the uncommitted debug/isolation diff from:
- `proj_cm55/main.c`
- `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`

and rebuilding / reprogramming the normal image:
- LCD returned live / GUI visible
- later user observation showed the display still froze after around seven steps

### Conclusion
Hot-path UART debug is unsafe on this display stack because it perturbs LVGL timing and first-frame / redraw behavior.

However, the later seven-step visible freeze still occurs on the clean image, so the debug `printf` code was not the root cause of the long-running freeze. It was a separate LCD-bring-up hazard that masked the deeper issue.

### Debug policy going forward
- Never `printf` inside the tight `lv_timer_handler()` loop
- Never `printf` every LVGL tick
- Never `printf` inside high-rate redraw / render / chart update paths
- Prefer low-rate breadcrumbs outside critical display timing
- Prefer one-shot boot markers or deferred ring-buffer capture over synchronous UART spam
