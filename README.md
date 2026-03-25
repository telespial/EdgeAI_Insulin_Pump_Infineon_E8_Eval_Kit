# EdgeAI Insulin Pump Demo - Infineon PSOC Edge E8 Evaluation Kit

Standalone insulin pump EdgeAI demo for `KIT_PSE84_EVAL_EPC2` with Waveshare 4.3-inch display (`CONFIG_DISPLAY=W4P3INCH_DISP`).

## What this repo contains
- Self-contained insulin pump app in:
  - `firmware_kit_epc2/proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/`
- CM55 runtime boots insulin demo directly (`APP_INSULIN_PUMP_MODE=1`).
- No Smart Pong source dependency.

## Build and flash
```bash
export CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7
export CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc
export CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools

cd firmware_kit_epc2
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```

## Start here
1. `start_here.md`
2. `docs/PROJECT_STATUS.md`
3. `firmware_kit_epc2/docs/START_HERE.md`
