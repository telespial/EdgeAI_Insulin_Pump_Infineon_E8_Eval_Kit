# Build Report

## Branch
- `embedded-bringup-smoke-test`

## Commit
- `working tree`

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
- Observed on UART after the cooperative-sidecar fix: steps 0 through 3 printed cleanly without LCD loss during the watch window

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
- `dashboard_timer_cb` is now restored in smoke mode so the GUI/timers remain active while APS prints as a sidecar

## Root Cause Hypothesis
- The earlier active smoke loop likely stalled the panel because APS work was driven from the GUI/timer path with repeated `push_sample()` updates and UART prints, effectively turning smoke mode into a competing UI workload.
- The LCD-safe fix restores the baseline GUI timer path and keeps APS output in a lightweight UART-only sidecar so the panel never depends on APS work to refresh.
- A second likely contributor was the CM55 loop sleeping too long when `lv_timer_handler()` had no near-term work scheduled; capping the loop delay to a small value keeps the GUI and touch service alive even when APS mode is quiet.
- The sidecar also needed a pacing fallback because the timer-driven GUI loop alone was not guaranteeing repeated APS service on this port.

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
- The GUI timer and APS sidecar now share the display safely; the next validation is to confirm the final smoke step/completion banner on a longer UART watch.
- The boot banner still shows the older build timestamp string in the UART watch window; that stamp should be refreshed or explained in a follow-up.
- No host regression was needed because this pass only changed embedded firmware smoke wiring.
