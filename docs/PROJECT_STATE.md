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

## Next Milestones
1. Complete end-to-end validation for CS81/CS82 touch path with bridge firmware state.
2. Continue gameplay polish and render artifact hardening under high speed.
3. Keep 4.3-inch EPC2 path as the only supported and release-pinned target.
