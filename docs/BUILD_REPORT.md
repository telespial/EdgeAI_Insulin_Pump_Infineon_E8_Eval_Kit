# Build Report

## Branch
- `embedded-bringup-smoke-test`

## Commit
- `df41472`

## Target
- `APP_KIT_PSE84_EVAL_EPC2`

## Toolchain
- `GCC_ARM`
- ModusToolbox tools path: `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
- GCC ARM path: `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc`

## Build Command
```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 \
  DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Flash Command
```bash
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 \
  DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Result
- Embedded build: success
- Flash/program: success
- UART smoke banner: success
- UART smoke sequence: success

## Artifact Paths
- `build/app_combined.hex`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.elf`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.hex`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.bin`
- `proj_cm33_s/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_s.elf`
- `proj_cm33_ns/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_ns.elf`

## Size Report
- CM55 DTCM_INTERNAL: `207,316 / 262,144`
- CM55 ITCM_INTERNAL: `225,880 / 262,144`
- RRAM: `159,744 / 524,288`
- SOCMEM_RAM: `4,446,208 / 5,242,880`
- SRAM: `399,882 / 1,048,576`
- m55 NVM region: `246,416 / 3,932,160`

## Warnings
- `ClampF32` unused warning in `cgm_model_runtime.c`
- `touchpad_read` unused variable warnings in `lv_port_indev.c`
- RWX load-segment linker warning for `proj_cm33_s.elf`
- `dashboard_timer_cb` unused warning after disabling the active smoke loop to preserve the LCD path

## UART Output
```text
APS Research Platform
Mode: Embedded smoke test
Predictor: present
Controller: present
Safety: present
Build: Jun 11 2026 21:43:59
```

## Known Gaps
- Timing instrumentation still reports `timing not available`.
- The active smoke loop is currently disabled to keep the LCD alive; only the APS banner prints on boot.
- No host regression was needed because this pass only changed embedded firmware smoke wiring.
