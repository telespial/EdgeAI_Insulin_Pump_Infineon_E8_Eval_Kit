# APS Mini Terminal V1

## Branch
- `aps-mini-terminal-v1`

## Intent
- Add a small lower-left APS status panel without replacing the existing GUI.
- Keep the work behind `APP_APS_MINI_TERMINAL=1`.

## Implemented Path
- File changed:
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`
- Added:
  - a lower-left LVGL panel
  - a 5-second cooperative timer path
  - real Predictor V2 / controller / safety evaluation using the embedded Candidate V1 tables

## Commands Run
- Host validation:
  - `make -f host.mk test`
  - `make -f host.mk regression`
- Embedded build:
  - `make clean`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_MINI_TERMINAL=1 -j8`
- Embedded flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_MINI_TERMINAL=1`
- OpenOCD reset-run used before build/flash flow and again after programming.

## Results
- Host validation passed.
- Embedded build passed.
- Embedded programming passed.
- OpenOCD remained healthy and reported:
  - `PSE846GPS2DBZC4A`
  - `CYBOOT_SUCCESS`
- UART capture after flash was garbled and inconclusive.
- Physical hardware result failed:
  - LCD blank / screen dead.

## Status
- This branch is preserved as a review snapshot for engineering analysis.
- It is not a known-good LCD-safe branch.
