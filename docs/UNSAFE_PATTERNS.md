## LVGL hot-path printf instrumentation
- Do not place `printf` inside the CM55 loop around `lv_timer_handler()`.
- Do not print on every dashboard timer tick.
- Do not print inside chart-refresh / redraw / frequent label-update paths.
- These patterns can perturb LVGL timing enough to blank the LCD or hide the real bug.
