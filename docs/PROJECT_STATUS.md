# Project Status Snapshot

## Board
- Target: `KIT_PSE84_EVAL_EPC2`
- Display profile: `W4P3INCH_DISP` (Waveshare 4.3-inch)

## Runtime Mode
- `APP_INSULIN_PUMP_MODE=1`
- Smart Pong runtime path removed.

## Verification (2026-03-25)
- Build: `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` ✅
- Flash: `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` ✅
- Detected MCU: `PSE846GPS2DBZC4A` (Rev `B0`)
- Programmed artifact: `firmware_kit_epc2/build/app_combined.hex`

## Restore tags
- Golden: `golden-e8-insulin-pump`, `golden-e8-insulin-pump-20260325`
- Failsafe: `failsafe-e8-insulin-pump`, `failsafe-e8-insulin-pump-20260325`
