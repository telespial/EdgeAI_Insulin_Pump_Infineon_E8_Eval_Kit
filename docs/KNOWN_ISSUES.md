## 2026-06-14 LVGL hot-path UART debug hazard
- Synchronous UART `printf` inside the LVGL hot path can keep the LCD from lighting or can destabilize display bring-up.
- Affected paths include the main loop around `lv_timer_handler()`, dashboard timer callbacks, and frequent render/update helpers.
- Use low-rate or deferred debug methods instead.
