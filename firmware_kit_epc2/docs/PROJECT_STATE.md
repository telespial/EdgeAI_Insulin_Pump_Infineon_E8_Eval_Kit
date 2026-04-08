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

## Verification Attempt
- Date: `2026-04-08`
- Requested operation: build + flash on unlocked repo path.
- Access status: repo is now readable/writable (`drwxrwxrwx`).
- Build result: failed before compile due missing ModusToolbox tools package path.
  - Error: `Unable to find any of the available CY_TOOLS_PATHS -- .../ModusToolbox/tools_3.7`
- Flash result: not run (build/tooling prerequisite not satisfied).
- Hardware probe status: USB `KitProg3 CMSIS-DAP` is detected by host.
- Note: a temporary local `.local_modus_tools` shim was tested and then removed; full Modus `tools_3.7` runtime is still required for normal make flow.

## Flash Verification
- Date: `2026-04-08`
- Method: direct OpenOCD programming with installed programmer package.
- Tool path:
  - `/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd`
- Image:
  - `build/app_combined.hex`
- Result:
  - Program + verify succeeded on `PSE846GPS2DBZC4A` (Rev `B0`).

## UI Fix Attempt
- Date: `2026-04-08`
- Issue reported: screen frozen and data windows blank/misaligned.
- Code fix applied:
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/medical_display_stub.c`
  - `MedicalHal_DrawRect()` now routes through `MedicalHal_FillRect()` so all rectangle edges use the same 480x320 -> 800x480 mapping path as other primitives.
- Build attempt:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - Failed before compile with: `Unable to find any of the available CY_TOOLS_PATHS -- .`
- Flash result:
  - Not run for this fix revision (new image was not produced in this environment).

## Tool Path Hardening
- Date: `2026-04-08`
- Problem addressed: repeated confusion between full Modus build tools (`tools_X.Y`) and programmer-only install (`ModusToolboxProgtools-1.7`).
- Updates made:
  - `common_app.mk` now searches additional Linux paths:
    - `/opt/Tools/ModusToolbox/tools_*`
    - `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_*`
  - `common_app.mk` error message now explicitly states that programmer-only installs can flash but cannot build.
  - `docs/OPS_RUNBOOK.md` now uses auto-detection for `CY_TOOLS_PATHS` and keeps direct OpenOCD flash fallback documented with the verified `/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd` path.

## Flash Execution
- Date: `2026-04-08`
- Requested operation: flash now.
- Build status before flash:
  - Rebuild retried with auto-detected `CY_TOOLS_PATHS`; still empty in this environment (full Modus `tools_X.Y` not present).
- Flashed image:
  - `build/app_combined.hex` (existing artifact currently in workspace).
- Flash method:
  - `/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd`
  - QSPI loader: `bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource/PSE84_SMIF.FLM`
- Result:
  - Program + verify succeeded on `PSE846GPS2DBZC4A` (Rev `B0`).

## Build + Flash Recovery (Fresh Image)
- Date: `2026-04-08`
- Objective: stop stale flashes; enforce build-then-flash from current code.
- Resolved tooling links:
  - Full Modus tools extracted and linked to:
    - `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
  - Edge Protect Security Suite extracted and linked to:
    - `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1`
  - `modus-shell/bin/{make,git,bash}` wrapper links added under `tools_3.7` for `mtbquery`.
- Build env used:
  - `CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
  - `CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc`
  - `CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools`
- Build result:
  - `make clean TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed.
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed including combine-sign.
  - New image timestamp: `build/app_combined.hex` mtime `2026-04-08 06:29:23 -0700`.
- Flash result:
  - Flashed that newly built `build/app_combined.hex` using Progtools OpenOCD + `PSE84_SMIF.FLM`.
  - Program + verify passed on `PSE846GPS2DBZC4A` (Rev `B0`).

## Flash Verification Refresh (2026-04-08)
- Attempted direct OpenOCD program/verify path without QSPI loader; verify failed as expected for SMIF sections.
- Re-ran using documented fallback command in OPS_RUNBOOK with `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
- Result: write + verify succeeded for `build/app_combined.hex` on device `PSE846GPS2DBZC4A` (Rev B0).
- Note: use only the fallback command (or `make program` when full tools are available) to avoid false-negative verifies.

## Background-Only Bringup (2026-04-08)
- Implemented request to remove all screen text/windows/buttons and leave only the background image.
- `ported_nxp/medical_lcd_layout.c` now draws only `pump_bg` in `MedicalLcd_DrawStaticLayout()`; `MedicalLcd_DrawDynamic()` is a no-op.
- `ported_nxp/edgeai_insulin_pump_port.c` runtime loop no longer performs sensor/UI update flow.
- Gyro path in this active port loop is disabled; accelerometer/barometer runtime handling is also disabled in this port path.
- Fresh build produced `build/app_combined.hex` (mtime 2026-04-08 06:42 local).
- Board flashed immediately after build using OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`; write + verify passed on `PSE846GPS2DBZC4A` rev B0.

## Flicker Fix (2026-04-08)
- Symptom after background-only mode: display blinking roughly 1 Hz.
- Root cause: frame present was still called in the runtime loop without redraw, causing buffer swap flicker.
- Fix: in ported_nxp/edgeai_insulin_pump_port.c, removed per-loop MedicalLcd_DrawDynamic/present path and kept only initial background present.
- Build: make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP succeeded.
- Flash: immediate reflash of fresh build/app_combined.hex via OpenOCD + QSPI loader succeeded with verify pass.

## EdgeAI Toggle Button (2026-04-08)
- Added a top-center touch button in insulin pump project only, visually aligned to the Drone Hunter HUD glass button style.
- File changed: proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/edgeai_insulin_pump_port.c.
- Button behavior:
  - Label toggles between EDGEAI ON and EDGEAI OFF.
  - Touch area is centered at top of screen (x:182..297, y:8..41).
  - Screen redraw/present happens only on toggle events to avoid blink.
- Drone Hunter project files were not modified.
- Built and flashed immediately after change; OpenOCD write+verify passed for build/app_combined.hex.

## EdgeAI Button Alignment and Touch Fix (2026-04-08)
- Enabled touch path in insulin platform by setting INSULIN_PLATFORM_ENABLE_TOUCH to 1.
- Adjusted top-center EdgeAI glass button text size/placement so label remains inside the button bounds.
- Kept button in mapped 480x320 virtual space so draw and touch coordinates align consistently on 800x480 panel.
- Rebuilt and reflashed immediately; write and verify passed.

## EdgeAI Button Text Position Fix (2026-04-08)
- Corrected text placement issue by removing double coordinate mapping in MedicalHal_DrawText.
- Text renderer now receives virtual coordinates directly (single map path via fill-rect pipeline).
- Rebuilt and reflashed immediately; verify passed.

## EdgeAI Glass Style Match Pass (2026-04-08)
- User reported button was still opaque and not matching Drone Hunter liquid-glass style.
- Updated `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/edgeai_insulin_pump_port.c` to render the button with per-pixel blending over the background instead of opaque fill rectangles.
- Style values now mirror Drone Hunter HUD button constants:
  - Fill color `0x8FD3FF` with opacities `26` (ON) / `36` (OFF)
  - Border color `0xE6F4FF` with opacities `68` (ON) / `88` (OFF)
  - Glow/shadow color `0x6BBEFF` with opacities `34` (ON) / `42` (OFF)
  - Rounded corners radius `14`
  - Text color `0xCFF2FF` (ON) and `0xFCA5A5` (OFF)
- Touch hit area remains unchanged and functional at `(x:182..297, y:8..41)`.
- Fresh rebuild completed successfully and produced `build/app_combined.hex` (mtime `2026-04-08 07:06` local).
- Flashed that exact fresh image with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`; write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## EdgeAI Button Shape/Text Pass (2026-04-08)
- User feedback requested a less slot-like button, larger label text, and explicit state colors.
- Updated `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/edgeai_insulin_pump_port.c`:
  - Button corner radius reduced from `14` to `5` to make the touch button a small-corner rounded rectangle.
  - Label changed to `AI ON` / `AI OFF`.
  - Label scale increased to `2` and centered programmatically in the button.
  - Label colors changed to green for ON and red for OFF.
- Rebuilt and produced fresh `build/app_combined.hex` (mtime `2026-04-08 07:11` local).
- Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`; write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## EdgeAI Glass Opacity + Radius Tuning (2026-04-08)
- User requested lower transparency and half corner radius.
- Updated `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/edgeai_insulin_pump_port.c`:
  - Corner radius reduced from `5` to `2` (half).
  - Glass fill opacity increased from `26/36` to `46/56` (ON/OFF) to make the box less transparent.
- Rebuilt and produced fresh `build/app_combined.hex` (mtime `2026-04-08 07:13` local).
- Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`; write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## CGM Stream Chart + Terminal Overlay (2026-04-08)
- Added a right-side live telemetry experience in insulin firmware only:
  - Upper-right chart panel renders a scrolling glucose trend graph.
  - Bottom-right liquid-glass terminal panel renders live text telemetry.
- Data stream source and prediction path:
  - Stream source is the recorded replay dataset `kCgmReplaySubject001Mgdl` from project snapshot header `upstream_nxp/src/cgm_replay_subject001.h` (read-only reference file).
  - Prediction uses `CgmModel_Predict(...)` from `ported_nxp/cgm_preprocess.c` with `cgm_model_features_t` derived per replay step.
  - AI toggle button now controls model enable state via `CgmModel_SetEnabled(...)`.
- Files changed:
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/ported_nxp/edgeai_insulin_pump_port.c`
- Build + flash:
  - Fresh image produced: `build/app_combined.hex` (mtime `2026-04-08 07:24` local).
  - Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Stream Continuity + Layout Alignment Pass (2026-04-08)
- User reported stream appeared to freeze after a few seconds and requested geometry changes.
- Stream timing/data fix in `ported_nxp/edgeai_insulin_pump_port.c`:
  - Replaced per-tick hard sample stepping with timeline interpolation over `kCgmReplaySubject001Mgdl`.
  - Added `DATA_STREAM_ADVANCE_DS=50` and retained 250 ms render cadence so replay now advances smoothly and spans hours instead of consuming points too quickly.
  - Trend computation now scales by decisecond advance (`trend_x100 = delta * 60000 / DATA_STREAM_ADVANCE_DS`).
- Geometry updates:
  - AI ON/OFF button width reduced ~20% (from 116 px to 93 px virtual width), keeping top placement.
  - CGM STREAM panel moved up so its top edge aligns with AI button top edge.
  - AI TERM panel moved up and made taller.
- Build + flash:
  - Fresh image produced: `build/app_combined.hex` (mtime `2026-04-08 07:29` local).
  - Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Stream Freeze Follow-up + Right Shift (2026-04-08)
- User requested moving stream/terminal right by 20 px and reported stream freeze persisted.
- Updated `ported_nxp/edgeai_insulin_pump_port.c`:
  - Shifted both right-side panels by +20 px (`CHART_X0/X1`, `TERM_X0/X1`) while keeping rendering clipped within 480x320 virtual bounds.
  - Replaced stream update trigger from microsecond delta checks to deterministic frame-divided cadence (`STREAM_STEP_FRAME_DIV`) to avoid platform timer jitter/rollover freeze effects.
  - Increased timeline continuity by using finer replay timeline advancement (`DATA_STREAM_ADVANCE_DS=10`).
- Build + flash:
  - Fresh image produced: `build/app_combined.hex` (mtime `2026-04-08 07:32` local).
  - Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Continuous Replay + Temperature + Clinical Readout Pass (2026-04-08)
- Updated `ported_nxp/edgeai_insulin_pump_port.c` to complete runtime wiring and stabilize long-run demo behavior:
  - Integrated board temperature source (`power_data_source`) into AI terminal output as degrees C.
  - Added center glucose readout (`mg/dL`) below the figure area with threshold colors:
    - Green: `80..180`
    - Orange: `70..79` and `181..249`
    - Red: `<=69` or `>=250`
  - Moved Stream and Terminal panels left by 10 px from the previous position:
    - Chart: `CHART_X0..X1 = 296..469`
    - Terminal: `TERM_X0..X1 = 296..469`
  - Addressed apparent stream freeze by increasing replay timeline advance to `DATA_STREAM_ADVANCE_DS=600` so values visibly progress continuously while replay still loops indefinitely over full recorded data.
- Data source confirmation:
  - Replay file remains `upstream_nxp/src/cgm_replay_subject001.h`.
  - Dataset length: `CGM_REPLAY_SUBJECT001_LEN = 1438`.
- Build + flash:
  - Fresh image produced: `build/app_combined.hex` (mtime `2026-04-08 08:02` local).
  - Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.
- Restore points:
  - Created `failsafe/restore_points/golden_20260408_080340_app_combined.hex`
  - Created `failsafe/restore_points/failsafe_20260408_080340_app_combined.hex`
  - Checksums saved in `failsafe/restore_points/20260408_080340_sha256.txt`

## Dummy Hour Stream + Board Temp Sensor Route (2026-04-08)
- User-requested UI/data adjustments completed in insulin project:
  - Center `mg/dL` readout moved up by one rendered text height (scale-2 glyph height).
  - Glucose stream source changed from long replay file to 1-hour dummy series at 5-minute cadence (12 points) with interpolation between points.
- Temperature routing update:
  - Added new platform module:
    - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/platform/board_temp_sensor.c`
    - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/platform/board_temp_sensor.h`
  - Module uses E8 BSP I3C controller (`CYBSP_I3C_CONTROLLER_*`) and probes legacy temp addresses `0x48..0x4B`, register `0x00`, then decodes 12-bit temperature to tenth-C.
  - Source references reviewed:
    - BSP generated config/pins (`cycfg_peripherals.h`, `cycfg_pins.h`) show I3C controller and P3.0/P3.1 pin route.
    - Upstream sample logic confirms static temp target addressing and decode model (`BOARD_TEMP_REG=0x00`, static `0x48` range, P3T1755-style decode).
  - Runtime behavior:
    - Terminal temp now prefers hardware board sensor reading each stream step.
    - Automatic fallback remains to existing simulated `PowerData` temp if hardware read is unavailable.
- Build + flash:
  - Fresh image produced: `build/app_combined.hex` (mtime `2026-04-08 08:21` local).
  - Flashed immediately with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Rebuild/Reflash Verification Pass (2026-04-08)
- Re-verified that requested turn changes are active in source:
  - `mg/dL` center text Y shift by one text height.
  - 1-hour dummy glucose source (12 points at 5-minute cadence).
  - Board temperature routed through E8 I3C temp path with fallback.
- Rebuilt with explicit toolchain paths:
  - `CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
  - `CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc`
  - `CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools`
- Reflashed `build/app_combined.hex` with OpenOCD + QSPI loader and verify pass on `PSE846GPS2DBZC4A` rev `B0`.

## Dummy Data Expansion to 800+ Rows (2026-04-08)
- Expanded dummy glucose source from 12 rows to 864 rows (5-minute interval, ~72 hours equivalent).
- Replaced static short array with deterministic generator function (`demo_glucose_row`) while keeping existing interpolation pipeline unchanged.
- Build + flash:
  - Fresh image: `build/app_combined.hex` (mtime `2026-04-08 08:30` local).
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Stream Freeze Mitigation Pass (2026-04-08)
- Implemented fail-safe stream runtime update to prevent UI stalls:
  - decoupled board temp sensor reads from high-rate render updates,
  - added cached board-temp handling with low-frequency refresh (`TEMP_HW_REFRESH_STEPS=120`) and automatic fallback to `PowerData` temp when hardware refresh fails.
- Goal: keep CGM/chart/terminal streaming continuous even if I3C temp transactions intermittently fail.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Stream Cadence Visibility Update (2026-04-08)
- Adjusted stream cadence to avoid apparent value stall around ~100 mg/dL:
  - `DATA_STREAM_ADVANCE_DS` changed to `3000` (advance one 5-minute row per update),
  - `STREAM_STEP_FRAME_DIV` changed to `1` (update each frame tick).
- This keeps replay index and glucose display moving visibly every update while still reading from the 864-row source.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Full Stream/Display Runtime Rewrite (2026-04-08)
- Reworked the insulin runtime data/display loop for deterministic non-stalling behavior:
  - replaced frame-div stepping with monotonic scheduled cadences (`STREAM_STEP_US`, `RENDER_STEP_US`, `TOUCH_POLL_US`) and catch-up guard,
  - changed glucose stepping to strict row-by-row traversal across the 864-row source (no interpolation hold behavior),
  - rewired temperature update to fallback-first non-blocking path with sparse opportunistic board-temp refresh.
- Objective: prevent freeze after a few seconds and keep glucose/chart/terminal moving continuously.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Hard Freeze Isolation Pass (2026-04-08)
- User reported hard freeze with touchscreen stopping simultaneously.
- Applied isolation change to remove runtime hardware board-temp reads from the live loop (I3C path), leaving non-blocking fallback temperature source from `PowerData` only.
- Purpose: eliminate I3C blocking as a freeze source while keeping stream/render scheduler rewrite active.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## No-Touch Diagnostic Build (2026-04-08)
- Because hard freeze persisted (including touch lock-up), created an isolation firmware with runtime touch polling disabled:
  - `ENABLE_TOUCH_INPUT=0` in `ported_nxp/edgeai_insulin_pump_port.c`.
- Purpose: determine if the freeze source is touch-bus path versus non-touch runtime.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Deep Isolation Runtime Build (2026-04-08)
- Additional freeze isolation changes applied in live runtime:
  - touch polling disabled (`ENABLE_TOUCH_INPUT=0`),
  - model inference bypassed in loop (`ENABLE_MODEL_INFERENCE=0`),
  - render cadence reduced to stream-driven updates only (avoid high-frequency display present pressure),
  - board-temp hardware reads already removed from runtime path.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## Minimal mg/dL-Only UI Build (2026-04-08)
- Per request, disconnected graph and terminal from runtime render path.
- Active render now shows only center `mg/dL` text line over static layout background.
- Build + flash:
  - Rebuilt fresh `build/app_combined.hex`.
  - Flashed with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - Write + verify passed on `PSE846GPS2DBZC4A` rev `B0`.

## 2026-04-08T09:00:36-07:00
- Request handled: print demo data row number under center `mg/dL` line.
- Code status: row label already present in `draw_center_glucose_readout()` and rendered as `row: <current>/<total>` under the glucose line.
- Build: `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` (no compile changes; up-to-date build verified).
- Flash: succeeded using OpenOCD with `-s bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource` and `set QSPI_FLASHLOADER PSE84_SMIF.FLM`.
- Note: generic OpenOCD flash command without GeneratedSource/SMIF mapping fails for this board image.

## 2026-04-08T09:08:00-07:00
- Freeze root cause identified: runtime scheduler clock used absolute conversion of 32-bit DWT cycle counter, which wraps at CM55 frequency in ~10-11 seconds; this matched freezing around row 22.
- Fix applied in `platform/insulin_platform.c`:
  - `insulin_platform_now_us()` changed to wrap-safe delta accumulation (`now_cycles - prev_cycles`) instead of absolute `cycles_to_us(cycles)`.
  - Added initialization of timing state in `insulin_platform_init()`.
- Expected effect: stream/render scheduling remains monotonic across DWT wraps; row counter should continue past 22/864.
- Build + flash:
  - Built with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`.
  - Flashed via OpenOCD with `-s bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource` and `set QSPI_FLASHLOADER PSE84_SMIF.FLM`.
  - Program + verify succeeded on PSE846GPS2DBZC4A (B0).

## 2026-04-08T09:12:49-07:00
- Restored UI panels in insulin demo runtime:
  - `render_screen()` now draws EdgeAI button, right-side CGM stream chart, and AI terminal, in addition to center glucose/readout.
- Wired temperature path into runtime stream state:
  - added board sensor integration via `board_temp_sensor_read_c10()`.
  - stream step now uses board temperature when available; falls back to `PowerData_Get()->temp_c` if sensor read fails.
  - `PowerData_Tick()` is called on each stream update to keep fallback sample fresh.
- Freeze fix from previous step retained (monotonic wrap-safe time accumulation).
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - OpenOCD flash+verify with GeneratedSource path + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`
  - Program + verify passed on PSE846GPS2DBZC4A (B0).
- Note: touch handling remains compile-disabled (`ENABLE_TOUCH_INPUT=0`), so the EdgeAI button is visual-only in this build.

## 2026-04-08T09:20:00-07:00
- Freeze-at-32 isolation update after restoring chart/terminal + live temp:
  - retained chart and terminal rendering.
  - changed temperature path to avoid repeated board I3C reads in the hot stream loop.
  - board sensor remains wired-in via startup snapshot (`board_temp_sensor_read_c10()` in init).
  - per-step temperature now comes from `PowerData_Tick()/PowerData_Get()` fallback path; if board snapshot was valid at startup, that value is retained.
- Rationale: deterministic freeze at `32/864` strongly indicated repeated live board-temp I3C polling instability.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - OpenOCD flash+verify with GeneratedSource path + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`
  - Program + verify passed on PSE846GPS2DBZC4A (B0).

## 2026-04-08T09:22:11-07:00
- Per request, removed only the graph panel from active rendering.
- Current render composition: EdgeAI button + AI terminal + center glucose/readout (graph hidden).
- Code change: removed `draw_chart_panel(stream)` call from `render_screen()` in `ported_nxp/edgeai_insulin_pump_port.c`.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - OpenOCD flash+verify with GeneratedSource path + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`
  - Program + verify passed on PSE846GPS2DBZC4A (B0).

## 2026-04-08T09:30:00-07:00
- Temperature path updated to real live on-board measurement in runtime.
- `cgm_stream_step()` now attempts `board_temp_sensor_read_c10()` on every stream update.
- If a read fails, it falls back to last known good board temp; if none exists yet, it falls back to `PowerData_Get()->temp_c`.
- Graph remains disabled per latest request; terminal and center glucose readout remain active.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - OpenOCD flash+verify with GeneratedSource path + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`
  - Program + verify passed on PSE846GPS2DBZC4A (B0).

## 2026-04-08T09:36:00-07:00
- Terminal label cleanup:
  - trend line prefix changed from `T` to `ROC`.
  - temperature line prefix changed from `T` to `TMP`.
- Board temperature reliability hardening:
  - retained live runtime reads via `board_temp_sensor_read_c10()` each stream step.
  - added one-shot re-probe/rebind recovery in `board_temp_sensor_read_c10()` when raw read or decode fails.
  - probe scans legacy I2C addresses `0x48..0x4B` and reattaches devices to recover from bus/address drift.
- Graph remains disabled (per user request); terminal and center glucose/readout remain active.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - OpenOCD flash+verify with GeneratedSource path + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`
  - Program + verify passed on PSE846GPS2DBZC4A (B0).

## Board Temp Sensor Dynamic-I3C Recovery (2026-04-08)
- Addressed non-working terminal temperature by updating board temp probe flow in:
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/platform/board_temp_sensor.c`
- Changes:
  - Added I3C dynamic-address probe path (`RSTDAA` + `SETDASA`) targeting static temp address `0x48`, assigning dynamic `0x08`.
  - Init and recovery now try dynamic-I3C first, then legacy I2C probe fallback (`0x48..0x4B`).
  - Kept existing decode guardrails and runtime re-probe behavior.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` succeeded.
  - Flashed fresh `build/app_combined.hex` via OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - OpenOCD verify passed (`verified 1639348 bytes`).

## TMP N/A Resolution Attempt: ADC Thermistor Fallback (2026-04-08)
- User report: terminal still showed `TMP N/A` after I3C dynamic-address probe update.
- Hardware doc finding (E84 kit user guide):
  - onboard thermistor `TH1` exists,
  - MCU thermistor path is not connected by default and is enabled via board rework,
  - board also exposes `CYBSP_ADC_6_POT` on `P15[1]` via SAR channel in generated BSP config.
- Code update in `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/platform/board_temp_sensor.c`:
  - retained I3C temp probe (`RSTDAA` + `SETDASA`) and legacy I2C probe attempts,
  - added ADC fallback path using `mtb_hal_adc` with `CYBSP_SAR_ADC_hal_config`,
  - ADC channel value now converts to estimated temperature via a bounded NTC-like lookup/interpolation table.
- Expected runtime behavior:
  - if I3C temp target is absent/unreachable, terminal now shows measured ADC-based temperature instead of `N/A`.
- Build + flash:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` succeeded.
  - Flashed fresh `build/app_combined.hex` via OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - OpenOCD verify passed (`verified 1640544 bytes`).
- Follow-up robustness patch:
  - if I3C init fails, ADC thermistor fallback is now attempted immediately in `board_temp_sensor_init()`.
- Rebuild + reflash:
  - write + verify passed (`verified 1640552 bytes`).

## Blank-Screen Recovery Flash (2026-04-08)
- User report: display became blank after the ADC thermistor fallback iteration.
- Recovery action:
  - rolled temperature implementation back to I3C dynamic probe + legacy I2C fallback only in:
    - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/platform/board_temp_sensor.c`
  - removed ADC fallback path from active build.
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - fresh artifact: `build/app_combined.hex` (mtime `2026-04-08 09:48:33 -0700`).
- Flash:
  - flashed `build/app_combined.hex` with OpenOCD + `QSPI_FLASHLOADER=PSE84_SMIF.FLM`.
  - OpenOCD reported successful write and verify (`verified 1639348 bytes`).
  - Note: tool exit code returned non-zero despite successful write/verify text; this behavior was also observed via `make program` wrapper because it uses the same OpenOCD runner.

## Golden + Failsafe Restore Point Refresh (2026-04-08)
- Created fresh restore artifacts from the latest verified build image:
  - `failsafe/restore_points/golden_20260408_095131_app_combined.hex`
  - `failsafe/restore_points/failsafe_20260408_095131_app_combined.hex`
  - `failsafe/restore_points/20260408_095131_sha256.txt`
- Updated docs for this baseline:
  - `docs/RESTORE_POINTS.md`
  - `docs/START_HERE.md`
  - `docs/OPS_RUNBOOK.md`
- Baseline for restore points:
  - Build image: `build/app_combined.hex` (mtime `2026-04-08 09:48:33 -0700`)
  - Flash verification: OpenOCD write+verify completed on `PSE846GPS2DBZC4A` rev `B0`.

## Git Publish + Restore Tags (2026-04-08)
- Pushed branch `main` to `git@github.com:telespial/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit.git`.
- Published dated restore tags:
  - `golden-e8-insulin-pump-20260408`
  - `failsafe-e8-insulin-pump-20260408`
- Updated rolling restore tags to current baseline:
  - `golden-e8-insulin-pump`
  - `failsafe-e8-insulin-pump`
- Current pushed baseline commit: `e3b3845`.
