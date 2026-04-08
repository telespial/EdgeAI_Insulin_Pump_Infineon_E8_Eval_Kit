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

## Verification Attempt (2026-04-08)
- Repo access: unlocked and accessible.
- Build command executed from `firmware_kit_epc2`: failed due missing configured tools path.
  - Error: `Unable to find any of the available CY_TOOLS_PATHS -- /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
- Flash command: not executed because build/tooling prerequisites are unresolved.
- Host probe detection: `Cypress KitProg3 CMSIS-DAP` detected over USB.

## Verification (2026-04-08, direct programmer path)
- Build artifact used:
  - `firmware_kit_epc2/build/app_combined.hex`
- Flash tool path used:
  - `/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd`
- Flash result: ✅ success
- Verify result: ✅ success
- Detected MCU during flash:
  - `PSE846GPS2DBZC4A` (Rev `B0`)

## Restore tags
- Golden: `golden-e8-insulin-pump`, `golden-e8-insulin-pump-20260325`
- Failsafe: `failsafe-e8-insulin-pump`, `failsafe-e8-insulin-pump-20260325`
