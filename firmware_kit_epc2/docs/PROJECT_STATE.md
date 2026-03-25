# PROJECT_STATE

## Date
2026-03-25

## Hardware
- Kit: `KIT_PSE84_EVAL_EPC2`
- Display: Waveshare 4.3-inch DSI panel

## Build config
- `TOOLCHAIN=GCC_ARM`
- `CONFIG_DISPLAY=W4P3INCH_DISP`
- `APP_INSULIN_PUMP_MODE=1`

## Current state
- Insulin demo is self-contained in this repo.
- Smart Pong app folder was removed from insulin repo.
- Local insulin platform HAL now contains `display_hal`, `time_hal`, and `touch_hal`.
