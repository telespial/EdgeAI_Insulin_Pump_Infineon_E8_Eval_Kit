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

## Next Milestones
1. Complete end-to-end validation for CS81/CS82 touch path with bridge firmware state.
2. Continue gameplay polish and render artifact hardening under high speed.
3. Keep 4.3-inch EPC2 path as the only supported and release-pinned target.
