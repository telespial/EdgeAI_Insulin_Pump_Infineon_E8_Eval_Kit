# Build Report — APS Smoke-Test Flash

Date: 2026-06-11

## Scope
- Verified the embedded CM33 secure, CM33 non-secure, and CM55 path on `APP_KIT_PSE84_EVAL_EPC2`.
- Built and flashed the APS smoke-test image with `APP_APS_SMOKE_TEST=1`.
- Captured UART output at `115200 8N1` on `/dev/ttyACM0`.

## Commands Used
```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Toolchain
- ModusToolbox 3.7
- GNU Arm Embedded Compiler 14.2.1
- Edge Protect Security Suite 1.6.1

## Artifacts
- `proj_cm33_s/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_s.elf`
- `proj_cm33_s/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_s.hex`
- `proj_cm33_ns/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_ns.elf`
- `proj_cm33_ns/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_ns.hex`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.elf`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.hex`
- `build/app_combined.hex`

## Memory / Size

`arm-none-eabi-size -B` output:

| Image | text | data | bss | dec |
| --- | ---: | ---: | ---: | ---: |
| CM33 secure | 28872 | 1400 | 133121 | 163393 |
| CM33 non-secure | 10504 | 104 | 258897 | 269505 |
| CM55 | 209312 | 1376852 | 3084640 | 4670804 |

## Flash Result
- Programming succeeded.
- Verification succeeded.
- The board rebooted cleanly after flash.

## UART Output
```text
APS Research Platform
Build: Jun 11 2026 19:02:30
Core: CM55
Predictor: present
Controller: present
Safety: present
Mode: smoke test only
APS smoke step 1 glucose=100 predicted_15=103 predicted_30=106 predicted_60=112 controller=NO_CHANGE safety=NO_CHANGE reasons=NONE loop_time=timing not available
APS smoke step 2 glucose=120 predicted_15=138 predicted_30=156 predicted_60=192 controller=NO_CHANGE safety=NO_CHANGE reasons=NONE loop_time=timing not available
APS smoke step 3 glucose=150 predicted_15=174 predicted_30=198 predicted_60=246 controller=NO_CHANGE safety=NO_CHANGE reasons=NONE loop_time=timing not available
APS smoke step 4 glucose=80 predicted_15=57 predicted_30=40 predicted_60=40 controller=SUSPEND_BASAL safety=SUSPEND_BASAL reasons=PREDICTED_LOW_15M|PREDICTED_LOW_30M loop_time=timing not available
APS smoke step 5 glucose=70 predicted_15=55 predicted_30=40 predicted_60=40 controller=SUSPEND_BASAL safety=SUSPEND_BASAL reasons=PREDICTED_LOW_15M|PREDICTED_LOW_30M loop_time=timing not available
APS smoke test complete
```

## Warnings / Notes
- `timing not available` is currently a placeholder because no lightweight timer hook was added in this pass.
- The smoke path intentionally bypasses the LVGL graphics task so the boot trace remains deterministic and easy to verify.
