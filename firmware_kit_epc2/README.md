# PSOC Edge E8 LVGL Firmware (Insulin Demo)

Firmware workspace for the insulin pump demo on EPC2.

## Active app
- `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/`

## Runtime selection
- `proj_cm55/Makefile` sets:
  - `DEFINES+=APP_INSULIN_PUMP_MODE=1`

## Build
```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
```

## Flash
```bash
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```
