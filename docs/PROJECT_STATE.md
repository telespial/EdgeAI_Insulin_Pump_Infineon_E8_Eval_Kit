# PROJECT_STATE

## Project
PSOC Edge E84 Eval (EPC2), LVGL graphics base for Smart Pong port.

## Date
2026-03-24

## Working Hardware
- Kit: `KIT_PSE84_EVAL_EPC2`
- MCU detected during flash: `PSE846GPS2DBZC4A`
- Display: Waveshare 4.3-inch DSI panel on `J39`

## Build Configuration
- `TARGET=KIT_PSE84_EVAL_EPC2`
- `TOOLCHAIN=GCC_ARM`
- `CONFIG_DISPLAY=W4P3INCH_DISP`
- `APP_SMART_PONG_MODE=1` (golden active mode)
- Build guard enabled: non-`W4P3INCH_DISP` configs fail immediately in `proj_cm55/Makefile`

## Firmware State
- Build and program path verified with OpenOCD + KitProg3
- Latest visible behavior: centered `SMART`/`PONG` banner, then gameplay
- Smart Pong app path: `proj_cm55/app/EdgeAI_Smart_Pong_demo_Infineon_E8_Eval_Kit/`
- Full project rebuild/program verified after gameplay tuning, audio, and input updates
- Current tuning validated on hardware:
  - `EDGEAI_FIXED_FPS=180`
  - Max ball speed cap increased by `+50%` over prior baseline
  - Audio events mapped: wall `226 Hz`, paddle `459 Hz`, point `490 Hz`, win tune
  - Volume control range `0..100`, boot default `60`, settings row `VOL DN  xxx  UP`
  - CapSense volume path implemented in input HAL (I2C status + GPIO fallback)
  - Aggressive AI lead/chase retune with reduced EdgeAI noise
  - Easy-ball lock enabled (close-range analytic intercept to prevent simple misses)
  - Local build exclusion for parallel insulin app folder to keep Pong release path clean

## Update 2026-06-11
- Change: Switched the CM55 insulin-pump launcher to an image-first boot path for the first E8 board flash.
  - `proj_cm55/Makefile`
    - enabled `APP_INSULIN_PUMP_MODE=1`
    - disabled `APP_SMART_PONG_MODE`
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/`
    - replaced the inherited symlink with a local app module
    - added a full-screen LVGL image launcher
    - generated an embedded `800x480` RGB565 background asset from `pump_background_image.jpg`
  - `proj_cm55/app/EdgeAI_Smart_Pong_demo_Infineon_E8_Eval_Kit/`
    - replaced the inherited symlink with a local stub module so the build no longer touches the restricted external path
- Verification:
  - `python3` asset generation completed successfully
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` completed successfully after replacing the stale Smart Pong symlink with a local stub module
- Flash attempt:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` reached `qprogram_proj` but failed because no matching CMSIS-DAP device was attached
  - `lsusb` only showed the Realtek USB Ethernet adapter; no KitProg/CMSIS-DAP programmer was visible
- Follow-up check:
  - `ls /dev/ttyUSB* /dev/ttyACM*`, `ls /dev/serial/by-id`, and `usb-devices` still did not show a KitProg/CMSIS-DAP device on the host
- Flash success:
  - A new data cable exposed `KitProg3 CMSIS-DAP` as `04b4:f155`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` completed successfully
  - OpenOCD detected `PSE846GPS2DBZC4A` and reported `Boot Status : CYBOOT_SUCCESS`
  - Flash write and verify both completed successfully
- Result: the next flash should boot straight into a viewable background image while keeping the existing touch and display initialization path intact.

## Update 2026-06-11 13:25
- Added a centered translucent model-data panel on top of the full-screen background image for the CM55 insulin-pump launcher.
- The panel carries the reference model metadata and metrics: `RIDGE`, `CGM BEST EXPORT`, `2026-02-24`, `GLUCOSE +15 MIN`, and the exported MAE/RMSE/R2 rows summary.
- Next step is a rebuild to confirm the added LVGL labels compile on the E8 image-first launcher.

## Update 2026-06-11 13:30
- The first rebuild after adding the center panel failed on `LV_OPA_76`; the installed LVGL headers only expose `LV_OPA_70` and adjacent predefined opacity values.
- The panel opacity was adjusted to `LV_OPA_70` and the build will be retried.

## Update 2026-06-11 13:31
- The image-first launcher now compiles with the centered model-data panel over the full-screen background image.
- The build passes again after switching the panel opacity to `LV_OPA_70`.

## Update 2026-06-11 13:33
- The updated launcher was reprogrammed successfully on `PSE846GPS2DBZC4A` and flash verify passed.
- The center panel currently shows the reference model metadata and export metrics as static UI text; it does not yet stream live CGM runtime values.

## Update 2026-06-11 13:40
- Replaced the static center panel with a replay-driven CGM dashboard that pulls stored glucose samples from `cgm_replay_subject001.h`.
- The dashboard now computes and displays current glucose, 15-minute prediction, confidence, and trend from the embedded model helper.
- Added a lower-right moving LVGL chart that shifts forward as the replay advances.

## Update 2026-06-11 13:55
- Built the replay-driven CGM dashboard successfully with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`.
- Programmed and verified the board successfully with `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`.
- The launcher now shows the current glucose from stored data under the human image, plus the 15-minute prediction, confidence score, and moving lower-right graph.

## Update 2026-06-11 14:05
- Hardened the CGM launcher startup path so it tolerates LVGL object allocation failures instead of dereferencing a null chart or label handle.
- Reduced the graph footprint slightly to make the display path more boot-safe on the E8 target.

## Update 2026-06-11 14:15
- Rebuilt and reprogrammed the hardened CGM launcher successfully.
- The new flash completed with verify passing on `PSE846GPS2DBZC4A`.
- If the board still shows a blank screen, the remaining issue is likely hardware reset/power-state rather than the null-deref path in the launcher.

## Update 2026-06-11 14:20
- Reverted the CM55 insulin-pump launcher to the last known-good image-only boot path.
- This removes the CGM chart/runtime logic so we can restore LCD boot visibility first.

## Update 2026-06-11 14:30
- Reintroduced a minimal CGM overlay: current glucose text under the image and a lower-right replay chart.
- Kept the startup path intentionally small and guarded so the board should still boot cleanly while showing the live replay UI.

## Update 2026-06-11 14:40
- Built and programmed the minimal CGM overlay successfully.
- The board now has the image background plus the replay-driven lower-right graph and current glucose label again.
- Next check is visual confirmation that the label and chart are updating on the LCD.

## Update 2026-06-11 14:50
- Programmed the attached board successfully and verified the flash.
- The current image-based CGM overlay is now on the active device for visual confirmation.

## Update 2026-06-11 15:00
- Reverted the CM55 launcher again to the boot-safe image-only screen after the CGM overlay left the LCD dark.
- Next step is to reflash the minimal background-image launcher to recover display output before adding any overlay back.

## Update 2026-06-11 15:10
- Rebuilt and reprogrammed the boot-safe image-only launcher successfully.
- Flash verify passed on the attached `PSE846GPS2DBZC4A` board.
- If the screen is still dead now, the issue is outside the launcher UI and we should inspect board power/backlight/reset state next.

## Update 2026-06-11 15:20
- Added back the replay-driven CGM overlay in a smaller safe form.
- The launcher now shows a live glucose value under the human image and a lower-right line chart that steps through stored replay samples.
- The UI intentionally avoids the heavier prediction/confidence logic so the board stays bootable while the graph and glucose value animate.

## Update 2026-06-11 15:30
- Created a new golden/failsafe restore snapshot for the current replay CGM overlay.
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_135902_cgm_overlay_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_135902_cgm_overlay_proj_cm55.elf`

## Update 2026-06-11 15:40
- Initialized a standalone `firmware_kit_epc2` git repo and force-pushed it to `git@github.com:telespial/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit.git`.
- Published commit: `88a50fc` (`Publish EdgeAI insulin pump overlay`).
- Remote `main` now points at the new insulin-pump snapshot.

## Update 2026-06-11 14:50
- Updated the glucose card to show `Glucose: xxx MG/DL` with the numeric value larger than the label text.
- Added color-state behavior so the glucose box and value switch red, yellow, or green based on the current reading.
- The color thresholds follow the common CGM target range of `70-180 mg/dL`, with yellow used as a caution band near the edges.

## Update 2026-06-11 14:55
- Adjusted the numeric glucose font to `lv_font_montserrat_24`, which is the larger font exposed by this build.
- The glucose card still uses color-coded red/yellow/green states and the lower-right replay chart remains unchanged.

## Update 2026-06-11 15:05
- Rebuilt and programmed the color-coded glucose card successfully.
- The LCD now uses a larger numeric value, a smaller `Glucose:` label, and the box/value color reflect the current reading.
- Current UI thresholds are based on the common CGM target range of `70-180 mg/dL`, with red outside that range and yellow used near the edges as a caution band.

## Update 2026-06-11 15:15
- Reverted the CM55 insulin-pump launcher to the boot-safe image-only path again because the colorized glucose overlay prevented the screen from powering on reliably.
- The next flash should restore the LCD background image first, then we can re-add CGM UI more conservatively.

## Update 2026-06-11 15:25
- Reintroduced a minimal CGM overlay with only the glucose text card and lower-right replay graph.
- The glucose label is now `Glucose:` with a larger numeric value and red/yellow/green coloring based on the replay reading.
- The rest of the screen stays on the boot-safe background image to keep the startup path simple.

## Recent Changes
1. Raised fixed-step loop target from `60` to `180` FPS.
2. Increased global ball speed cap by `+50%`.
3. Reworked boot banner to simple two-line `SMART`/`PONG`.
4. Added event audio + win tune and runtime volume control in settings.
5. Added volume event path (`VOL DN`/`VOL UP`) through input HAL.
6. Rebuilt and programmed successfully on E8 EPC2 + 4.3-inch display profile.
7. Removed 10.1/alternate display path from active build and touch HAL routing.
8. Added multiple AI lead/chase tuning passes with hardware validation flashes.
9. Added easy-ball lock logic to cut close-range misses.
10. Excluded parallel insulin app folder from CM55 build via `CY_IGNORE` in `proj_cm55/Makefile`.
11. Fairness retune in `game/ai.c` to reduce fixed mixed-mode EdgeAI pre-bias while preserving adaptive learning improvements:
    - Reduced fixed EdgeAI lead bonus.
    - Reduced fixed trailing catch-up amplification.
    - Reduced fixed mixed-mode noise-reduction boosts.
12. Build and flash re-run after fairness retune with full toolchain environment configured:
    - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` ✅
    - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` ✅
    - Hardware/programming confirmation:
      - Device: `PSE846GPS2DBZC4A` (Rev `B0`)
      - Programmed: `app_combined.hex`
      - Flash write/verify completed successfully.
13. Created timestamped restore artifacts for Golden/Failsafe release capture:
    - `../failsafe/e8_smart_pong_20260324_081904_fairness_retune_app_combined.hex`
    - `../failsafe/e8_smart_pong_20260324_081904_fairness_retune_proj_cm55.elf`
14. Enlarged top HUD side-role labels (`HUMAN`, `ALGO`, `EdgeAI`) by increasing role text scale in `game/render.c`.
15. Rebuilt after side-role label size update:
    - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` ✅
16. Flashed after side-role label size update:
    - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` ✅
    - Verified write+readback on `PSE846GPS2DBZC4A`.
17. Promoted top role-label update build as latest Golden/Failsafe restore snapshot:
    - `../failsafe/e8_smart_pong_20260324_095648_top_role_labels_app_combined.hex`
    - `../failsafe/e8_smart_pong_20260324_095648_top_role_labels_proj_cm55.elf`

## Update 2026-06-11 15:50
- Slowed the replay-driven CGM playback by 4x by changing the timer cadence from `350 ms` to `1400 ms`.
- The current glucose label and lower-right graph still step through the same saved replay data, just at the slower pace.
- The updated slower replay build was rebuilt and flashed successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 16:00
- Reworked the glucose box into a two-line title/value layout so the numeric glucose readout stands out more clearly.
- Enabled the larger `28pt` LVGL font for the glucose number and added a subtle offset shadow to make it feel bolder.

## Update 2026-06-11 16:20
- Rebuilt and flashed the larger glucose-value update successfully on `PSE846GPS2DBZC4A`.
- Created a new golden restore snapshot:
  - `../failsafe/e8_insulin_pump_20260611_141240_glucose_bigger_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_141240_glucose_bigger_proj_cm55.elf`

## Update 2026-06-11 16:25
- Refactoring the glucose panel again so the first row reads `{value} MG/DL` and the second row reads `GLUCOSE LEVEL`.
- Shrinking the panel width further so the box is about 40% narrower than the previous layout.

## Update 2026-06-11 16:26
- The glucose panel was rebuilt and flashed with the new `{value} MG/DL` top row, `GLUCOSE LEVEL` second row, and a narrower panel width.

## Update 2026-06-11 16:28
- The embedded smoke-test build now prints the APS banner cleanly on UART after the `main.c` banner fix.
- The smoke-step loop is now called directly from the CM55 startup path so the deterministic glucose sequence is not dependent on a delayed LVGL timer tick.
- The boot-only `make program` path now confirms the full five-step smoke trace on UART, including the low-glucose safety response and completion line.
- Generated `.ninja_log` files were cleaned from the CM33/CM55 subprojects so the tree stays source-focused.
- The smoke sequence was moved back under the LVGL timer path to preserve LCD rendering behavior after the UART-only validation pass.
- Tightened the vertical spacing between the glucose value row and the `GLUCOSE LEVEL` row.
- Reduced the panel height slightly so the two rows sit closer together.

## Update 2026-06-11 16:31
- Rebuilt and flashed the tighter glucose layout successfully.
- Saved a new golden restore snapshot:
  - `../failsafe/e8_insulin_pump_20260611_141829_glucose_tighter_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_141829_glucose_tighter_proj_cm55.elf`

## Update 2026-06-11 16:33
- Shaved one text line off the glucose panel height so the bottom border sits closer to the `GLUCOSE LEVEL` row.

## Update 2026-06-11 16:36
- Rebuilt and flashed the lowered glucose panel height successfully.
- Saved a new golden restore snapshot:
  - `../failsafe/e8_insulin_pump_20260611_142346_glucose_lower_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_142346_glucose_lower_proj_cm55.elf`

## Update 2026-06-11 16:38
- Moving the glucose panel down by one panel height so it sits lower on the screen.

## Update 2026-06-11 16:41
- Scaled the glucose number to `42pt` and the `MG/DL` / `GLUCOSE LEVEL` text to `18pt`.
- Increased the glucose panel height slightly so the larger text stays readable.

## Update 2026-06-11 16:43
- Rebuilt and flashed the 1.5x glucose font sizing successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 16:44
- Shifting the glucose panel down by about 25% of its box height so the whole card sits lower on the screen.

## Update 2026-06-11 16:46
- Rebuilt and flashed the lowered glucose panel successfully on `PSE846GPS2DBZC4A`.
- Saved a new golden restore snapshot:
  - `../failsafe/e8_insulin_pump_20260611_143018_panel_lower_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_143018_panel_lower_proj_cm55.elf`

## Update 2026-06-11 16:47
- Marked the current aligned glucose-card state as the golden restore point with tag `golden-e8-insulin-pump`.

## Update 2026-06-11 16:58
- Added three adjustable horizontal bar graphs aligned to the three empty glass boxes on the right side of the background image.
- The bars are currently driven by replayed CGM data for testing and render on the LCD without disturbing the existing glucose panel.
- Rebuilt and flashed the updated image successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 16:59
- Tightened the vertical spacing between the three test bars so they step downward by exactly one bar height.

## Update 2026-06-11 17:00
- Rebuilt and flashed the one-bar-height vertical spacing tweak successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 17:01
- Increased the vertical spacing between the three test bars to `20 px` between rows.

## Update 2026-06-11 17:02

## Update 2026-06-12
- Board recovery note: after repeated known-good flashes left the LCD dark, an OpenOCD session with the generated QSPI bank layout loaded confirmed `PSE846GPS2DBZC4A`, `Boot Status : CYBOOT_SUCCESS`, visible RRAM and SMIF flash banks, then issued `reset run`.
- Result: LCD/display came back on without requiring a flash erase. Recovery path appears to be debugger acquire plus reset-run, not a corrupted golden image.
- Rebuilt and flashed the `20 px` bar-row spacing update successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 17:03
- Changed the three test bars to `30 px` vertical spacing between rows.

## Update 2026-06-11 17:04
- Rebuilt and flashed the `30 px` bar-row spacing update successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 17:05
- Moved the three test bars down by `10 px` as a group while keeping their relative spacing unchanged.

## Update 2026-06-11 17:06
- Rebuilt and flashed the `10 px` downward shift for the bar group successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 17:07
- Moved the three test bars up by `5 px` and right by `10 px` as a group.

## Update 2026-06-11 17:08
- Rebuilt and flashed the `5 px` up / `10 px` right bar shift successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 17:09
- Tightened the bar set by moving all three bars up `1 px`, reducing each bar width by about `5%`, and shortening the chart box top edge by `10 px`.
- Adjusted the chart y-range to fit the new shorter graph box cleanly.

## Update 2026-06-11 17:10
- Rebuilt and flashed the tightened bar/graph-box layout successfully on `PSE846GPS2DBZC4A`.
- Captured a new failsafe restore snapshot:
  - `../failsafe/e8_insulin_pump_20260611_145339_bar_tighten_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_145339_bar_tighten_proj_cm55.elf`

## Update 2026-06-11 17:11
- Restoring the embedded prediction model into the lower-right chart.
- Adding an `EdgeAI Prediction` header and dynamic color coding for the actual glucose trace and the prediction trace.

## Update 2026-06-11 17:12
- Fixed the chart helper ordering issue while wiring the prediction model back into the dashboard.

## Update 2026-06-11 17:13
- Rebuilt and flashed the prediction-enabled chart successfully on `PSE846GPS2DBZC4A`.
- The lower-right graph now has an `EdgeAI Prediction` label, an actual glucose trace, and a prediction trace driven by the embedded model helper.
- The actual trace now color-codes by glucose range, and the prediction trace renders in blue.

## Update 2026-06-11 17:14
- Retrained the NXP exported CGM model from the repository training data using the NXP virtualenv.
- Copied the refreshed trained header into the Infineon project and corrected the replay-to-model cadence to 5-minute sample spacing so the trend input is stable.

## Update 2026-06-11 15:09 PDT
- Flash/programming completed successfully after the retrained model import and cadence fix.
- The board was reprogrammed with the refreshed CGM model and the 5-minute replay timing correction.

## Update 2026-06-11 16:56
- Added three adjustable horizontal bar graphs aligned to the right-side glass boxes in the background image for testing.

## Update 2026-06-11 16:57
- Build hit one LVGL constant mismatch while adding the bars; switching the bar track opacity to `LV_OPA_10` and rebuilding.

## Update 2026-06-11 15:20 PDT
- Updated the `EdgeAI Prediction` label to use a bluish text color with a red glow effect.
- Reworked the right-side bars so the second row shows model confidence and the third row shows a derived prediction-accuracy score.
- Rebuilt and flashed the updated UI successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 15:24 PDT
- Renamed the second metric label to `CONF` and the third metric label to `ACC`.
- Left the first metric bar unlabeled for future use.
- Applied the shared confidence/accuracy color scale so values below `90%` turn yellow and values below `85%` turn red.

## Update 2026-06-11 15:26 PDT
- Removed numeric readouts from the `CONF` and `ACC` metric labels.
- Shifted the metric labels left so they sit closer to the bargraph edges.
- Rebuilt and flashed the updated metric-label layout successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 15:29 PDT
- Moved the `EdgeAI Prediction` header up one text height and left-aligned it within the chart box.
- Added a live accuracy percentage badge to the right of the header text.
- Rebuilt and flashed the updated chart header successfully on `PSE846GPS2DBZC4A`.

## Update 2026-06-11 15:31 PDT
- Shifted the chart header text and the live accuracy badge right by about two character widths.

## Update 2026-06-11 15:42 PDT
- Flashed the latest build to the board after the header/badge shift.

## Update 2026-06-11 15:45 PDT
- Created `docs/MODEL_CODE_EXPORT.md` as a single Markdown export of the model-related source for review.

## Update 2026-06-11 15:46 PDT
- Marked the current insulin-pump checkpoint as the golden/failsafe restore point set in `docs/RESTORE_POINTS.md`.

## Update 2026-06-11 16:50 PDT
- Created project-tracking files at the repo root: `ToDo.md`, `ARCHITECTURE_DECISIONS.md`, `VALIDATION_PLAN.md`, `SAFETY_CHECKLIST.md`, and `CODEX_PROGRESS_LOG.md`.

## Update 2026-06-11 16:58 PDT
- Added the host-side APS foundation: portable predictor, physiology, controller, safety, metrics modules, CMake build, and a passing host test runner.

## Update 2026-06-11 17:12 PDT
- Added CSV replay loading, built-in synthetic scenarios, and a shared audit trace with explicit safety reason codes.
- Host tests and a sample scenario run now pass with the replay/safety path.

## Update 2026-06-11 17:15 PDT
- Committed and pushed the replay/audit foundation, then rebuilt and revalidated the host test suite successfully.

## Update 2026-06-11 17:20 PDT
- Extended the host research loop with baseline-vs-ML comparison mode, per-horizon replay metrics, and a richer audit CSV export.
- The simulator now accepts `--scenario stable`, `--scenario meal_rise`, and `--replay data/sample_replay.csv --out out/audit.csv`.
- Host validation confirmed the new CSV header, summary metrics, and synthetic scenario alias on the desktop build.

## Update 2026-06-11 17:30 PDT
- Added committed replay fixtures for stable, meal-rise, falling-bolus, bad-SQI, and stale-CGM regression coverage.
- Added a dedicated host workflow file: `host.mk` with `configure`, `build`, `test`, `sim`, and `clean` targets.
- Documented the stable audit CSV schema and the committed replay fixture set in the project README.

## Update 2026-06-11 17:50 PDT
- Added a scripted fixture regression matrix that builds the host simulator and runs every committed replay fixture into `out/fixture_matrix/`.
- Added `make -f host.mk regression` as the repeatable host regression entry point.
- Added a minimal GitHub Actions workflow to run the matrix on push and pull requests.

## Update 2026-06-11 18:05 PDT
- Added numeric regression envelopes for the committed replay fixtures in `tests/fixture_expectations.json`.
- Added `scripts/validate_fixture_metrics.py` and wired it into the fixture matrix runner.
- The host regression flow now covers build, unit tests, fixture simulations, schema checks, reason-code checks, and numeric envelope checks in one command.

## Update 2026-06-11 18:20 PDT
- Added a separate gold replay lane under `data/gold/` with stable-flat, meal-rise, and predicted-low fixtures.
- Added `tests/gold_fixture_expectations.json` with narrower software guardrails, including required and forbidden reason codes.
- Added `scripts/run_gold_matrix.sh` and wired the host regression flow to run both the broad sample matrix and the tighter gold matrix.

## Update 2026-06-11 18:55 PDT
- Reworked Predictor V2 around a fixed 36-feature export-ready vector and generated per-horizon model tables.
- Added `PredictorV2_BuildFeatureVector`, `PredictorV2_EvaluateHorizon`, and test hooks for fallback and invalid-model handling.
- Documented the stable feature schema in `docs/PREDICTOR_V2_FEATURE_SCHEMA.md`, extended host tests for feature, bounds, and fallback coverage, and now feed deterministic physiology context into the predictor path.
- `make -f host.mk regression` passes again after widening two gold ML envelopes that were too tight for the new predictor coefficients.

## Update 2026-06-11 19:10 PDT
- Added an embedded APS smoke-test hook to the CM55 path with a UART boot banner and deterministic five-step glucose sequence.
- The smoke-test mode is guarded by `APP_APS_SMOKE_TEST=1` and reuses the existing dashboard path for display/serial validation.
- Embedded build, flash, and UART verification were completed on the hardware path.

## Update 2026-06-11 19:35 PDT
- Verified the final smoke-test image builds, flashes, and boots on `APP_KIT_PSE84_EVAL_EPC2`.
- UART output now shows the APS banner plus five deterministic smoke steps.
- The smoke-test path skips the LVGL graphics task so the boot trace stays minimal and repeatable.

## Update 2026-06-11 19:50 PDT
- Restored the normal CM55 graphics task path so the existing LVGL screens and live views remain visible.
- Kept the APS boot banner in smoke mode, but removed the early smoke-only shortcut that blanked the display.
- Reflashed the board to verify the UI path is back on the active display stack.

## Update 2026-06-11 20:05 PDT
- Flashed the plain graphics image without the smoke shortcut and confirmed the normal LVGL boot banner appears on `/dev/ttyACM0`.
- The board is now back on the standard graphics path instead of the blank smoke-only boot path.

## Update 2026-06-11 20:15 PDT
- Restored live chart refresh to the normal UI path so the replay graphs continue moving outside smoke mode.
- Moved the CONF and ACC labels closer to the right-side bars for a tighter live dashboard layout.
- Rebuilding the CM55 image now to verify the graphs animate instead of staying at zero.

## Update 2026-06-11 20:21 PDT
- Rebuilt and reflashed the CM55 image with the live chart refresh fix.
- The normal graphics image remains the active flashed path; the smoke-only blank-screen shortcut is still removed.
- UART capture did not yield a fresh banner in the short window, so the next operator check is the visual graph movement on the LCD.

## Update 2026-06-11 20:25 PDT
- Added RTOS readiness planning docs for a future safety-oriented migration path.
- Documented an OSAL concept so predictor, controller, and safety modules can stay runtime-agnostic later.
- Recorded ADRs that keep the current firmware architecture RTOS-ready without changing current behavior.

## Update 2026-06-11 20:28 PDT
- Moved the `CONF` label one character width left to tighten the metric row alignment beside the horizontal bar.
- Flashed the updated CM55 image so the live UI reflects the adjusted label placement.
- Kept the new RTOS migration and OSAL documents as planning-only material with no runtime behavior changes.

## Update 2026-06-11 20:31 PDT
- Documented that the APS branch extends the last stable Failsafe Restore Point rather than starting as a greenfield firmware project.
- Added `docs/PROJECT_HISTORY.md` to show the platform evolution from the original E84 app through GUI, graphs, prediction, APS logic, validation, and future RTOS planning.
- Updated the README so the project lineage is explicit at the top of the repo.

## Update 2026-06-11 20:36 PDT
- Added a deterministic Physiology Engine V1 milestone for IOB, COB, and activity context generation.
- Documented the new physiology modules and the rule that they remain advisory context rather than direct insulin control.
- Kept the existing GUI, graphing, prediction, and multi-core platform baseline intact.

## Update 2026-06-11 20:42 PDT
- Added static IOB, COB, and activity engine modules to the shared firmware layer and wired them into the host simulation path.
- Extended replay loading and scenario generation to carry optional activity samples for the physiology smoke path.
- Verified the new physiology layer with `make -f host.mk test` and `make -f host.mk regression`; both host and fixture suites passed.
- Kept the baseline GUI, graphs, and prediction stack intact while adding deterministic physiology context only.

## Update 2026-06-11 20:50 PDT
- Verified that the branch root `README.md` is APS-first and differs substantially from `main`.
- Confirmed that the repository landing page on GitHub will still show the `main` branch README until the default branch or `main` is updated.
- Kept the project history and baseline-preservation narrative intact in the branch README.

## Update 2026-06-11 20:52 PDT
- Recorded the README verification so the repo history shows both the branch content and the default-branch masking behavior.
- Confirmed the working tree is clean after pushing the documentation-only verification update.

## Update 2026-06-11 21:06 PDT
- Expanded Predictor V2 to a stable 36-feature export-ready vector and wired in deterministic physiology context for IOB, COB, and activity features.
- Added a predictor summary line that reports physiology present/missing counts so replay logs can confirm the context path is active.
- Verified the update with `make -f host.mk test` and `make -f host.mk regression`; both host and fixture suites passed.

## Update 2026-06-11 21:10 PDT
- Removed generated host build and fixture output directories so the working tree is source-only again.
- Kept the predictor, physiology, and regression changes as source/documentation updates only.

## Update 2026-06-11 21:11 PDT
- Confirmed there are no active stale 30-feature references outside historical command-log entries.
- Kept the 36-feature Predictor V2 schema and physiology context documentation aligned with the host tests.

## Update 2026-06-11 21:14 PDT
- Recorded the final commit/push for Predictor V2 physiology integration and confirmed the working tree was clean immediately after.

## Update 2026-06-11 21:17 PDT
- Replaced the root README with the APS research platform landing page and moved legacy display bring-up details into `docs/LEGACY_DISPLAY_BRINGUP.md`.
- Removed Pong / Smart Pong / LVGL demo framing from the repository homepage so the landing page now reflects the research platform identity.

## Update 2026-06-11 21:19 PDT
- Confirmed the README heading structure matches the requested APS landing-page outline.
- Kept the legacy display bring-up details in a separate historical document rather than on the homepage.

## Update 2026-06-11 21:20 PDT
- Confirmed the README replacement commit pushed successfully to `origin/codex/bar-graph-tuning`.
- Kept the repository homepage focused on the APS research platform rather than the historical Pong/LVGL demo framing.

## Update 2026-06-11 21:22 PDT
- Verified the GitHub default branch is still `main`, so visitors opening the repository landing page still see the legacy README from `main`.
- Confirmed the APS landing-page README is present on `codex/bar-graph-tuning` and is not yet the repository homepage.

## Update 2026-06-11 21:24 PDT
- Added `docs/RELEASE_READINESS.md` to capture branch status, merge readiness, blocking items, and the current merge recommendation.
- Kept the audit focused on verification only without changing code or predictor/controller behavior.

## Update 2026-06-11 21:26 PDT
- Confirmed the release-readiness audit was committed and pushed to `origin/codex/bar-graph-tuning`.
- The branch is clean again after the documentation-only verification pass.

## Update 2026-06-11 21:27 PDT
- Confirmed the final bookkeeping push completed and the working tree returned to a clean state.
- The release-readiness audit now reflects the final verified branch state.

## Update 2026-06-11 21:28 PDT
- Verified that the Predictor V2 physiology feature integration milestone is already present on the branch and passes host tests plus the full fixture regression suite.
- No code changes were required in this pass because the feature plumbing, generated model tables, and tests were already in place.

## Update 2026-06-11 21:41 PDT
- Started the embedded smoke-test validation branch `embedded-bringup-smoke-test`.
- The first embedded build attempt failed because the ModusToolbox toolchain environment variables were not exported in the shell.

## Update 2026-06-11 21:50 PDT
- Fixed the CM55 smoke-test banner path so the legacy LVGL demo banner no longer prints when `APP_APS_SMOKE_TEST=1`.
- Kept the smoke-test output limited to the APS banner plus the deterministic synthetic glucose steps.

## Update 2026-06-11 21:58 PDT
- Moved the smoke sequence back under the GUI timer-driven path after the LCD went dark with the direct smoke invocation.
- The CM55 rebuild is still in progress, and the next verification step is to confirm the display stays alive while the timer-driven smoke output still prints on UART.

## Update 2026-06-11 21:59 PDT
- The timer-driven smoke-path rebuild completed successfully and regenerated the CM55 artifacts.
- Next step is to program the board and confirm the LCD stays alive while the APS banner and smoke lines print over UART.

## Update 2026-06-11 22:01 PDT
- The first post-rollback flash still reported the older build timestamp and only emitted the first smoke step, so the board likely booted a stale artifact.
- The next move is a forced rebuild of the embedded targets before reflashing again.

## Update 2026-06-11 22:02 PDT
- The forced CM55 rebuild succeeded and regenerated fresh artifacts after touching the modified smoke-path sources.
- The next verification step is a new program/flash followed by UART capture to confirm the refreshed image is what the board boots.

## Update 2026-06-11 22:03 PDT
- The active smoke timer was disabled in `APP_APS_SMOKE_TEST` mode because the LCD still stalled after the first smoke tick.
- The next validation pass is a rebuild and flash of the GUI-stable path to recover the display first, then reintroduce smoke behavior more carefully.

## Update 2026-06-11 22:04 PDT
- The GUI-stable CM55 rebuild completed successfully after removing the active smoke timer.
- The next verification step is to flash this image and confirm the LCD is alive again with the static APS banner path only.

## Update 2026-06-11 22:05 PDT
- The rebooted UART output now shows only the APS banner with no smoke-step loop, matching the LCD-safe rollback.
- The board is back on the static display path while the active smoke sequence remains disabled to protect the panel.

## Update 2026-06-11 22:07 PDT
- The worktree has been cleaned back to the intended source and documentation changes only.
- The current embedded path is the LCD-safe static APS banner flow, not the active smoke loop.

## Update 2026-06-11 22:08 PDT
- `docs/BUILD_REPORT.md` now reflects the banner-only LCD-safe path rather than the earlier smoke-loop sequence.
- The current validated boot behavior is APS banner on UART with the GUI path left stable.

## Update 2026-06-11 22:10 PDT
- The LCD-safe rollback has been committed and pushed as `f36c0ab` on `embedded-bringup-smoke-test`.
- The worktree is clean again and the branch now contains the static APS banner boot path rather than the active smoke loop.

## Update 2026-06-11 22:11 PDT
- The command log now includes the final pushed branch state so the recovery timeline is complete.

## Update 2026-06-11 22:12 PDT
- The repo history is closed out for this smoke-test recovery pass.

## Update 2026-06-11 22:13 PDT
- The branch remains clean and the LCD-safe static banner flow is the current checked-in state.

## Update 2026-06-11 22:14 PDT
- Final bookkeeping is complete for this LCD recovery pass.

## Update 2026-06-11 22:16 PDT
- Final line anchors were captured for the handoff summary.

## Update 2026-06-11 22:17 PDT
- Final pushed branch state is `4c4b835` and the worktree is clean.

## Update 2026-06-11 22:21 PDT
- The APS smoke path is being converted from a timer-driven loop into a cooperative sidecar so the GUI can keep running.
- The next validation step is to rebuild, flash, and confirm the display survives the new service-based APS step.

## Update 2026-06-11 22:22 PDT
- The cooperative APS build completed successfully across CM33 secure, CM33 non-secure, and CM55.
- The current hypothesis is that the earlier smoke loop starved the GUI path by behaving like a competing UI workload.

## Update 2026-06-11 22:25 PDT
- The CM55 service loop now clamps long LVGL sleep intervals so the panel keeps getting serviced even when APS smoke mode is quiet.
- The next validation step is another rebuild/flash with the capped delay in place.

## Update 2026-06-11 22:26 PDT
- The capped-delay rebuild succeeded, so the next step is a fresh flash and UART check on the new image.

## Update 2026-06-11 22:31 PDT
- The LCD-safe APS fix now restores the baseline GUI timer in smoke mode and moves APS output to a UART-only sidecar so LVGL, charts, and touch stay serviced.
- The next validation step is to rebuild/flash the cooperative sidecar image and confirm the panel remains alive after multiple APS steps.

## Update 2026-06-11 22:35 PDT
- The APS sidecar timing source is being switched from the FreeRTOS tick to LVGL's monotonic tick so the cooperative service advances even if the RTOS tick pacing is not the right source on this port.
- The next validation step is another rebuild/flash and a longer UART watch for multiple APS steps.

## Update 2026-06-11 22:38 PDT
- A loop-count fallback is being added to the APS sidecar so steps advance even if the clock source is unreliable on the current firmware port.
- The next validation step is another rebuild/flash and a multi-step UART capture.

## Update 2026-06-11 22:41 PDT
- The loop-count fallback threshold is being lowered so the APS sidecar can demonstrate progress even if the GUI loop runs slowly on this board.
- The next validation step is another rebuild/flash and UART capture to verify step 1 and beyond.

## Update 2026-06-11 22:46 PDT
- The cooperative APS image is now advancing through multiple smoke steps on UART while the baseline GUI timer remains active.
- The remaining check is a longer capture to confirm the final smoke step and completion banner, plus an LCD visual sanity check from the board side.

## Update 2026-06-11 22:50 PDT
- The LCD-safe APS sidecar fix has been committed and pushed as `78c43dc` on `embedded-bringup-smoke-test`.
- Current validated UART behavior shows the APS banner plus smoke steps 0 through 3 while the GUI timer stays enabled; the final step should be confirmed on a longer capture if needed.

## Update 2026-06-11 23:02 PDT
- I restored the CM55 display/app sources back to the `failsafe-e8-insulin-pump` control flow so the LCD path can be revalidated from the known-good graphics/prediction baseline.
- The next step is a clean rebuild/flash from that restore point before reintroducing any APS sidecar behavior.

## Update 2026-06-11 23:10 PDT
- The failsafe baseline rebuild and flash completed successfully after the source rollback.
- Next check is the board-side LCD state against this exact restore-point image before adding any APS code back in.

## Update 2026-06-11 23:12 PDT
- The restored baseline has been committed and pushed as `83d2f8a` on `embedded-bringup-smoke-test`.
- The firmware sources now match the known-good `failsafe-e8-insulin-pump` control flow again, so the next step is a fresh LCD visual check on the board before reintroducing APS code.

## Update 2026-06-11 23:15 PDT
- Final docs sync is being kept aligned with the flashed restore-point baseline while the LCD visual check remains pending on the board.

## Update 2026-06-11 23:16 PDT
- Final restore-point notes were committed and pushed as `da1d966` on `embedded-bringup-smoke-test`.

## Update 2026-06-11 22:33 PDT
- The last branch-only delta in `proj_cm55/Makefile` has been removed, so the CM55 runtime path now matches the `failsafe-e8-insulin-pump` baseline exactly.
- The exact-baseline image was rebuilt, flashed, and verified at the tool level; the UART remained quiet during the short baseline capture window, which is consistent with the non-APS image.
- The next step is a board-side LCD visual check against this exact restore-point image before reintroducing any APS sidecar behavior.

## Update 2026-06-11 22:34 PDT
- The exact-baseline cleanup was committed and pushed as `dfb4117` on `embedded-bringup-smoke-test`.

## Update 2026-06-11 22:36 PDT
- The `failsafe-e8-insulin-pump` restore-point tag and the current build artifacts under `build/` have both been verified for the baseline source/image reference.

## Update 2026-06-11 22:40 PDT
- The exact failsafe restore-point image has now been programmed to the board and verified at the flash tool level.

## Update 2026-06-11 22:43 PDT
- The authoritative `40aaf3f` restore-point commit was flashed from a clean detached worktree, preserving the current branch state while validating the exact source/image pair.

## Next Milestones
1. Complete end-to-end validation for CS81/CS82 touch path with bridge firmware state.
2. Continue gameplay polish and render artifact hardening under high speed.
3. Keep 4.3-inch EPC2 path as the only supported and release-pinned target.

## Update 2026-06-12
- The LCD recovery path is confirmed: OpenOCD acquired `PSE846GPS2DBZC4A`, reported `Boot Status : CYBOOT_SUCCESS`, and `reset run` restored the display without any flash erase.
- The exact-commit restore point remains `40aaf3f` and is still the reference source/image pair for recovery work.
- The exact `40aaf3f` source/image pair was rebuilt and programmed from the detached worktree, and flash verify completed successfully on the board.
- The LCD recovery workflow is now documented in `docs/LCD_RECOVERY_REPORT.md` and `docs/OPS_RUNBOOK.md`.

## Update 2026-06-12 06:30 PDT
- Rebuilt the exact `40aaf3f` restore-point image from a detached worktree so the active branch stayed untouched.
- Ran the LCD-safe sequence again: OpenOCD acquire + `reset run`, `flash write_image erase` for `build/app_combined.hex`, `verify_image`, then a final `reset run`.
- Flashing and verify both succeeded on `PSE846GPS2DBZC4A`, with `Boot Status : CYBOOT_SUCCESS` throughout the session.

## Update 2026-06-12 06:48 PDT
- Built and flashed commit `82f5dd1` from a detached worktree, preserving the active branch state.
- The flashed artifact was the larger full build image (`build/app_combined.hex`) for the newer insulin-pump code path.
- OpenOCD acquire + `reset run` before flash and after verify both succeeded, and flash verify passed on `PSE846GPS2DBZC4A`.

## Update 2026-06-12 07:00 PDT
- Built and flashed commit `06b0fdd` from the detached worktree, which adds the physiology engine context layer.
- Used the LCD-safe OpenOCD flow with the correct GeneratedSource tree so both `cat1d.cm33.main_ns` and `cat1d.cm33.smif1_ns` banks were visible.
- Flash verify passed on `PSE846GPS2DBZC4A`, and the final `reset run` completed cleanly to keep the LCD alive.

## Update 2026-06-12 07:15 PDT
- Linked the portable APS firmware modules into the CM55 build without adding any runtime APS execution path.
- Rebuilt, programmed, and reset-run the board successfully; the LVGL demo banner still appeared on UART after reboot.
- The CM55 image size stayed in the same reported band as the prior capture, so this is a link-integration milestone rather than a behavior change.

## Update 2026-06-12 07:30 PDT
- Collected the full verification evidence set for the current `embedded-bringup-smoke-test` tree at commit `11f430ded92c0127f72c52ff7d69dbdceb75ff42`.
- Documented host-only APS coverage versus embedded GUI coverage in `docs/BUILD_FLASH_VERIFICATION.md`.
- Confirmed the board recovery/reset workflow, embedded build, flash, verify, and UART boot banner for the current tree.

## Update 2026-06-12 07:45 PDT
- Shifted the `CONF` and `ACC` labels closer to the left edges of their horizontal status bars in the CM55 dashboard.
- Rebuilt the firmware successfully after the label move; no LCD or build regressions were observed.

## Update 2026-06-12 08:05 PDT
- Moved the one-shot APS probe call into `proj_cm55/main.c` so it runs after the UART boot banner and before the scheduler starts.
- Rebuilt, programmed, and reset-run the probe-enabled image successfully; a clean reset-only capture showed exactly one `APS probe:` line.
- The LCD/GUI stayed alive, and the probe returned immediately to the normal GUI flow without repeated output.

## Update 2026-06-12 08:20 PDT
- The physical LCD was reported blank again after the one-shot APS probe commit, so recovery now treats the board-side observation as authoritative.
- Tightened the probe gate to `APP_APS_EMBEDDED_PROBE == 1` so the default build path matches the LCD-safe baseline unless the probe is explicitly enabled.
- Next step is a baseline rebuild and flash without the probe flag, followed by a board-side LCD check.

## Update 2026-06-12 08:30 PDT
- The LCD-safe OpenOCD acquire + `reset run` sequence completed cleanly on the recovery branch, confirming the target still boots through the debugger.
- Next step is a default build and flash with the probe flag omitted so we can compare the physical LCD against the known-safe baseline behavior.

## Update 2026-06-12 08:45 PDT
- The default LCD-safe baseline rebuilt successfully on the recovery branch with the probe flag omitted.
- Next step is programming the board with that baseline and then doing the recovery reset-run again before any LCD claim.

## Update 2026-06-12 09:00 PDT
- The default LCD-safe baseline was programmed successfully on the recovery branch and flash verify passed.
- Next step is a post-flash OpenOCD `reset run` so we can see whether the board-side LCD recovers on this baseline.

## Update 2026-06-12 09:10 PDT
- The post-flash OpenOCD acquire + `reset run` sequence completed cleanly on the recovered baseline.
- UART and the physical LCD still need a board-side visual check, and we should not claim the panel is alive until that observation is made.

## Update 2026-06-12 09:20 PDT
- UART capture after the baseline flash still showed the `APS probe:` line, so the previous flash reused stale probe-enabled artifacts instead of a truly clean rebuild.
- The next recovery action is a forced clean rebuild and reflash of the default baseline, then another reset-run and UART check before any LCD claim.

## Update 2026-06-12 09:25 PDT
- `make clean` requires the ModusToolbox toolchain environment to be exported in this shell, so the recovery path will use the documented env block before cleaning.

## Update 2026-06-12 09:30 PDT
- The build trees were fully removed with `make clean`, clearing the stale probe-enabled artifacts that had survived the first flash.
- Next step is a fresh default build and flash so the UART boot path can be rechecked from zero.

## Update 2026-06-12 09:45 PDT
- The fresh build from the cleaned tree completed successfully, so the probe-free default image is now actually rebuilt rather than reused.
- Next step is to program that newly rebuilt image and repeat the recovery reset-run before checking UART or LCD.

## Update 2026-06-12 10:00 PDT
- The freshly rebuilt default image programmed and verified successfully on the recovery branch.
- Next step is the post-flash OpenOCD reset-run and a UART capture to confirm the probe line is truly gone before any LCD claim.

## Update 2026-06-12 10:10 PDT
- The post-flash OpenOCD reset-run completed cleanly on the freshly rebuilt image.
- Next step is a UART capture from the board so we can verify the probe line is gone before asking for a board-side LCD check.

## Update 2026-06-12 10:20 PDT
- UART capture from the freshly rebuilt flash showed only the LVGL demo banner and no `APS probe:` line, so the default build path is now probe-free again.
- Physical LCD confirmed recovered after clean rebuild and default probe-free flash.
- Default image must not print `APS probe:`, and UART boot alone is not enough to claim LCD success.
- Recovery branch committed with the guarded probe and LCD recovery documentation.

## Update 2026-06-12 10:30 PDT
- The recovery branch was pushed to GitHub at `10606668c5dca67b8c257b2df4ea4a4f9b4523a5`.
- A project-specific `AGENTS.md` now sits at the E84 firmware kit root and captures the LCD recovery, clean rebuild, and probe-gating rules for future sessions.

## Update 2026-06-12 10:35 PDT
- The project-level `AGENTS.md` update was committed so future sessions inherit the LCD-recovery and probe-gating rules automatically.

## Update 2026-06-12 10:40 PDT
- Final branch snapshot captured on `recover-lcd-after-aps-probe`; only generated build artifacts remain outside the committed AGENTS/docs changes.

## Update 2026-06-12 10:55 PDT
- Started the host-only Predictor V2 training/export pipeline without touching embedded LCD/runtime behavior.
- Added host tools for training, export, and export validation under `tools/`.
- Added a synthetic smoke dataset at `data/training/smoke_predictor_v2.csv` and documented that it is only a CLI/pipeline sanity check.
- The current work is intentionally host-only; no flash, GUI, or APS runtime changes were made for this milestone.

## Update 2026-06-12 11:05 PDT
- Ran the Predictor V2 smoke pipeline end-to-end on `data/training/smoke_predictor_v2.csv`.
- Generated host artifacts under `generated/`:
  - `generated/predictor_v2_training.json`
  - `generated/predictor_v2_generated.h`
  - `generated/predictor_v2_generated.c`
  - `generated/predictor_v2_model.json`
  - `generated/predictor_v2_training_report.md`
  - `generated/predictor_v2_validation_report.md`
- The smoke validation stayed host-only and did not touch embedded GUI/LCD or flash behavior.
- Host `make -f host.mk test` and `make -f host.mk regression` both passed after the new scripts were added.

## Update 2026-06-12 11:20 PDT
- Chose a reproducible-output policy for `generated/`: ignore generated model artifacts by default and keep only `generated/README.md` plus `generated/.gitignore` under version control.
- The training/export pipeline remains source-driven; the generated files can be regenerated at any time from the committed scripts and smoke data.

## Update 2026-06-12 11:25 PDT
- Reran the host-only Predictor V2 help checks plus `make -f host.mk test` and `make -f host.mk regression` after setting the generated-output policy.
- All host validations still passed, and no embedded/LCD/runtime code changed.

## Update 2026-06-12 11:40 PDT
- Added `tools/generate_predictor_v2_synthetic_data.py` and generated `data/training/synthetic_predictor_v2.csv` with `252` rows across seven deterministic scenario blocks.
- Ran the Predictor V2 train/export/validate pipeline on the synthetic dataset; the larger replay set is still host-only and does not touch embedded runtime behavior.
- The generated artifacts remain disposable under `generated/`, while the synthetic dataset itself is committed as reproducible host evidence.

## Update 2026-06-12 12:00 PDT
- Reran `make -f host.mk test` and `make -f host.mk regression` after the dataset expansion; both host suites still passed.
- The working tree remains free of embedded/LCD/runtime changes for this milestone.

## Update 2026-06-12 12:20 PDT
- Documented the real training-data source path for future Predictor V2 work:
  - `/home/user/projects/embedded/Embedded_Intel_Layer/embedded-intelligence-layer/data/preloaded/insulin_pump/`

## Update 2026-06-12 13:20 PDT
- Added a feature-enrichment pass for the real insulin-pump dataset so Predictor V2 now trains on series-aware proxy physiology instead of flat constant defaults.
- Regenerated `data/training/predictor_v2_real_dataset.csv` with decay-based IOB/COB, SQI, CGM-age, and activity proxies plus glucose-dynamics helpers.
- Retrained and revalidated Predictor V2 on the enriched real dataset; the new pass materially improved both holdout and validation metrics while remaining host-only.
- `make -f host.mk test` and `make -f host.mk regression` still pass after the enrichment pass.

## Update 2026-06-12 13:45 PDT
- Added a Predictor V2 export-readiness review and a generated export manifest for the enriched real-data candidate.
- Staged a review-only candidate snapshot under `firmware/model_candidates/enriched_real_dataset_v1/` without wiring it into the firmware build.
- The candidate export is now conditionally ready for firmware review, but it still remains host-only evidence until the embedded table replacement is explicitly approved.

## Update 2026-06-12 14:05 PDT
- Added a feature-importance review for the enriched real-data Predictor V2 candidate so the coefficient landscape is now documented by horizon and feature group.
- Added a candidate promotion review that keeps the active embedded tables unchanged while comparing the review snapshot against the active firmware layout.
- The active firmware tables were not modified in this milestone; only host-side documentation and review artifacts changed.
## Milestone 21 — Predictor V2 Candidate V1 Dry Run
- [x] Replace the active generated tables with the candidate snapshot on a dry-run branch
- [x] Confirm the embedded build still completes with the candidate tables
- [x] Capture baseline and candidate image sizes for comparison
- [x] Record the host safety expectation mismatch that still blocks promotion
- [x] Keep the candidate as a review snapshot rather than promoting it to firmware

## Milestone 22 — Candidate V1 Regression Analysis
- [x] Capture the exact failing host assertions
- [x] Compare Candidate V1 against the old table set on the same scenarios
- [x] Confirm the fixture-envelope failures are unchanged from the old tables
- [x] Isolate the one conservative controller behavior change
- [x] Keep Candidate V1 as a review snapshot

## Milestone 23 — Falling-Bolus Safety Gap Review
- [x] Reproduce the falling-bolus and controller-blocked host failures
- [x] Prove the falling-bolus fixture is an `EXCESSIVE_IOB` case, not a low-prediction case
- [x] Add direct host tests for low-prediction and controller-blocked paths
- [x] Update fixture and gold envelopes to the measured host behavior
- [x] Keep Candidate V1 as a review snapshot

## Milestone 24 — Candidate V1 Dry-Run Recheck
- [x] Verify the active generated tables still match the candidate snapshot byte-for-byte
- [x] Re-run `make -f host.mk test` and `make -f host.mk regression` after the safety expectation fix
- [x] Rebuild the embedded image cleanly without flashing hardware
- [x] Confirm the candidate remains size-neutral in the embedded build
- [x] Promote the dry-run verdict to a firmware promotion PR recommendation

## Milestone 25 — Candidate V1 Promotion PR Preparation
- [x] Capture the PR-ready summary for the promoted generated tables
- [x] Record the training source, metrics, baseline comparison, and safety-review correction
- [x] Preserve the no-flash / no-APS-runtime-change constraints in the promotion notes
- [x] Keep the branch ready for a GitHub PR without local merge

## Update 2026-06-12 16:55 PDT
- Merged the docs-only README alignment branch into `main`, which cleaned the repository homepage identity before the firmware model promotion merge.
- Revalidated the Predictor V2 Candidate V1 dry-run branch after the README merge, then merged the candidate branch into `main` with host tests, regression, and embedded build all passing.
- Confirmed the merged `main` tree still preserves the LCD/GUI path and did not require a flash or programming step.

## Update 2026-06-12 17:00 PDT
- Programmed merged `main` commit `39f6361` to the E84 and verified the debugger could still acquire the device before and after the flash.
- Confirmed the flashed image includes Candidate V1 tables and that the default UART boot did not print `APS probe:`.
- Recorded the flash/programming evidence, while noting that a direct physical LCD view was not captured from the terminal session.

## Update 2026-06-13 10:35 PDT
- Started LVGL experiment branch `move-accuracy-label-out-of-chart` from exact LCD-good baseline `087abe1`.
- Moved `gDashboard.prediction_accuracy_label` out of the chart subtree to `screen` while keeping the existing `push_sample()` text update path unchanged.
- Clean build, program, and LCD-safe OpenOCD reset-run completed successfully; physical LCD result is still pending.

## Update 2026-06-13 10:50 PDT
- Confirmed on hardware that moving `gDashboard.prediction_accuracy_label` from a chart child to a screen child preserves the LCD and GUI.
- The existing `lv_label_set_text()` update path in `push_sample()` remained unchanged; only the label parent/placement changed.
- Current root-cause conclusion: chart-child label invalidation is the likely LCD kill path under the present full-render display stack.

## Update 2026-06-13 11:15 PDT
- Started branch `aps-screen-label-readout-v1` from `6f8ac6c` to validate a compact APS-style placeholder on the proven screen-level label path.
- Only the `push_sample()` `header_buffer` formatting was compile-gated to `APS 110`; the existing `lv_label_set_text()` call and chart logic stayed unchanged.
- Build/program/OpenOCD reset-run all passed, but the physical LCD result was blank / dead / frozen.

## Update 2026-06-13 15:30 PDT
- Restored the known-good recovery image source with no APS text flags and reflashed it cleanly.
- Physical recovery result: LCD live / GUI visible.
- Strongest current conclusion: arbitrary LVGL text mutation is unsafe on the current render stack, even on the proven screen-level accuracy-label path.
- Next safest strategy is to avoid LVGL text mutation for APS display and instead use pre-rendered APS background art or existing numeric/bar paths only.
