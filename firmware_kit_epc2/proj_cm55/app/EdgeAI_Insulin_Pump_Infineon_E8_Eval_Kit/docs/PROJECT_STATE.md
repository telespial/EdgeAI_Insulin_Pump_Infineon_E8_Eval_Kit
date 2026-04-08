# PROJECT_STATE

- Project: EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit
- Last Updated: 2026-04-08T06:29:57-07:00
- Repo Root: EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit
- Build Command: `cd firmware_kit_epc2 && make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash Command: `openocd -s /opt/Tools/ModusToolboxProgtools-1.7/openocd/scripts -s bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource -c "set QSPI_FLASHLOADER PSE84_SMIF.FLM" -c "source [find interface/kitprog3.cfg]; transport select swd; source [find target/infineon/pse84xgxs2.cfg]; init; reset init; adapter speed 12000; flash write_image erase build/app_combined.hex; verify_image build/app_combined.hex; reset run; shutdown;"`
- Binary: `firmware_kit_epc2/build/app_combined.hex`
- Status File: docs/STATUS.md

## Current Status
- Repository organized under PSOC_EDGE_E8_EVAL/projects/.
- Project docs scaffolded.
- UI rendering fix applied in `ported_nxp/medical_display_stub.c`:
  - `MedicalHal_DrawRect()` now uses mapped coordinate path (`MedicalHal_FillRect`) to keep panel/window borders aligned with scaled content.
- Build is currently blocked in this environment due missing Modus `tools_3.x` path (`CY_TOOLS_PATHS` unresolved), so this fix is code-complete but not locally reflashed.
- Immediate flash request (`2026-04-08`) completed using existing artifact:
  - Flashed `firmware_kit_epc2/build/app_combined.hex` through Progtools OpenOCD + `PSE84_SMIF.FLM`.
  - Program + verify succeeded on `PSE846GPS2DBZC4A`.
  - Note: this flash did not include a newly rebuilt image for the latest source edit because full Modus `tools_X.Y` build tools are still unavailable here.

## Latest Verification
- Full build tooling links are now fixed and build is operational in this workspace.
- `medical_display_stub.c` fix is included in a fresh successful build.
- Newly built `firmware_kit_epc2/build/app_combined.hex` was flashed and verified on hardware.

## Next Actions
- Resolve `CY_TOOLS_PATHS` to a valid Modus `tools_3.x` install, then rebuild.
- Flash rebuilt `build/app_combined.hex` and verify window placement/data updates on hardware.

## Latest Update (2026-04-08T08:03:40-07:00)
- Runtime status for insulin app on E8 board:
  - EdgeAI liquid-glass toggle button active at top center.
  - Right-side `CGM STREAM` chart and `AI TERM` terminal active.
  - Terminal now includes board temperature in degrees C.
  - Center glucose readout (`mg/dL`) is rendered below the figure with threshold coloring:
    - Green `80..180`
    - Orange `70..79` and `181..249`
    - Red `<=69` or `>=250`
- Stream continuity:
  - Replay source: `upstream_nxp/src/cgm_replay_subject001.h`
  - Replay rows: `1438`
  - Replay advance tuned to avoid visual freeze (`DATA_STREAM_ADVANCE_DS=600`) and loop continuously.
- Layout refinement:
  - Stream and terminal panels shifted left by 10 px from prior build.
- Build/flash:
  - Fresh image: `firmware_kit_epc2/build/app_combined.hex` mtime `2026-04-08 08:02`.
  - Flashed immediately with OpenOCD + QSPI loader; verify passed on `PSE846GPS2DBZC4A` (rev `B0`).
- Restore points:
  - `failsafe/restore_points/golden_20260408_080340_app_combined.hex`
  - `failsafe/restore_points/failsafe_20260408_080340_app_combined.hex`
  - `failsafe/restore_points/20260408_080340_sha256.txt`
