# COMMAND_LOG

2026-03-25T09:18:00-07:00 | insulin standalone cleanup | removed Smart Pong app folder from insulin repo and transplanted required HAL files into insulin app platform/
2026-03-25T09:21:00-07:00 | runtime switch cleanup | updated proj_cm55 Makefile/main.c to insulin-only boot path (APP_INSULIN_PUMP_MODE=1)
2026-03-25T09:24:00-07:00 | docs refresh | rewrote insulin repo README/start docs for self-contained pull-build-flash workflow
2026-03-25T09:28:00-07:00 | build verify | make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
2026-03-25T09:29:00-07:00 | flash verify | make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
