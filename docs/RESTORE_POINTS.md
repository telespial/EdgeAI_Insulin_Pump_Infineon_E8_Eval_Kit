# RESTORE_POINTS

## Purpose
Known-good checkpoints for fast recovery when development changes break boot, display, or flashing.

## Tagged Restore Points (Repo-Level)
- `failsafe-e8-smart-pong`
  - Meaning: Last known-stable Smart Pong restore point for rapid recovery.
- `golden-e8-smart-pong`
  - Meaning: Current development base for Smart Pong feature work.
- `failsafe-e8-insulin-pump`
  - Meaning: Last known-stable insulin-pump checkpoint for rapid recovery.
- `golden-e8-insulin-pump`
  - Meaning: Current development base for the insulin-pump CGM dashboard and model export work.

## Expected Behavior At Current Baseline
- Boot shows centered `SMART` / `PONG` banner then enters gameplay.
- Build/flash target: `KIT_PSE84_EVAL_EPC2` with `CONFIG_DISPLAY=W4P3INCH_DISP`.
- Display support is pinned to 4.3-inch only (non-4.3 configs are intentionally blocked).
- Runtime tuning:
  - `EDGEAI_FIXED_FPS=180`
  - max ball speed cap: `+50%` over prior baseline.
  - audio event tones and win tune enabled.
  - fairness retune active: reduced fixed EdgeAI mixed-mode pre-bias, adaptive learning retained.
  - easy-ball lock active for close-range analytic intercept.
  - volume UI row enabled (`VOL DN  xxx  UP`), default `VOL=60`.

## Latest Verified Restore Artifact (2026-03-24)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_smart_pong_20260324_095648_top_role_labels_app_combined.hex`
  - `../failsafe/e8_smart_pong_20260324_095648_top_role_labels_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-11)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_135902_cgm_overlay_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_135902_cgm_overlay_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-11, glucose text refresh)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_141240_glucose_bigger_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_141240_glucose_bigger_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-11, tighter glucose card spacing)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_141829_glucose_tighter_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_141829_glucose_tighter_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-11, lower glucose box border)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_142346_glucose_lower_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_142346_glucose_lower_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-11, panel lowered)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_143018_panel_lower_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_143018_panel_lower_proj_cm55.elf`

## Golden Restore Tag (2026-06-11)
- Tag: `golden-e8-insulin-pump`
- Meaning: Current aligned glucose-card state with tightened bar graphs and the shortened lower-right chart box, ready for fast restore.

## Latest Verified Restore Artifact (2026-06-11, tightened bar graphs and chart box)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260611_145339_bar_tighten_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260611_145339_bar_tighten_proj_cm55.elf`

## Latest Verified Restore Artifact (2026-06-14, prediction title/score placement)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Physical result:
  - LCD live
  - GUI visible
  - `EdgeAI` / `Prediction` title split into two left-justified lines
  - large prediction score repositioned left for the current approved layout
- Build:
  - `make clean TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260614_080746_prediction_layout_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260614_080746_prediction_layout_proj_cm55.elf`

## Golden Restore Tag (2026-06-14)
- Tag: `golden-e8-insulin-pump`
- Meaning: Current approved insulin-pump LCD layout with the two-line `EdgeAI Prediction` title and the repositioned large prediction score.

## Failsafe Restore Tag (2026-06-14)
- Tag: `failsafe-e8-insulin-pump`
- Meaning: Emergency recovery point for the same physically verified LCD-safe insulin-pump layout.

## Restore Procedure
1. Fetch tags:
   ```bash
   git fetch --tags
   ```
2. Checkout restore point:
   ```bash
   git checkout <tag>
   ```
3. Build/program using runbook environment:
   ```bash
    make clean TOOLCHAIN=GCC_ARM
    make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
   ```

## Notes
- Use `failsafe-e8-smart-pong` when recovering from major regressions.
- Use `golden-e8-smart-pong` as the default base for new feature work.

## Update 2026-06-14 Final Artificial Pancreas Restore Point
- Status: active required restore truth for the left-shifted bright-orange-on-black `Artificial Pancreas` layout.
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260614_081754_artificial_pancreas_truth_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260614_081754_artificial_pancreas_truth_proj_cm55.elf`
- Intent: this supersedes older June 14 prediction-layout restore artifacts and is now the required golden/failsafe point-of-truth.

## Update 2026-06-14 APS Placeholder Display Restore Point
- Status: active required restore truth for the physically verified `Artificial Pancreas` placeholder header plus green readout block.
- Commit: `c77941f`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260614_085312_aps_placeholder_display_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260614_085312_aps_placeholder_display_proj_cm55.elf`
- Physical result:
  - LCD live
  - GUI visible
  - white `Artificial Pancreas` header visible
  - green placeholder block visible at the approved final position
- Intent: this supersedes the earlier June 14 artificial-pancreas title-only restore point and is now the required golden/failsafe insulin-pump source of truth.

## Update 2026-06-14 Live CRT Terminal Restore Point
- Status: active required restore truth for the physically verified live CRT terminal readout.
- Commit: pending commit created from branch `aps-terminal-live-bg-test` after user-confirmed live data on LCD.
- Physical result:
  - LCD live
  - GUI visible
  - `BG`, `IOB`, `COB`, `INS`, and `SAFETY` are live on the CRT block
  - `ACT` remains intentionally static until a live activity path is wired
- Intent: this supersedes the placeholder-only CRT restore point as the new golden/failsafe source of truth.

## Update 2026-06-14 APS Demo State RAPID Restore Point
- Status: active required restore truth for the APS demo-state CRT readout with `RAPID` safety wording.
- Commit: pending commit created from branch `aps-demo-state-wiring`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260614_105422_aps_demo_state_rapid_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260614_105422_aps_demo_state_rapid_proj_cm55.elf`
- Flash/build evidence:
  - build passed with `CONFIG_DISPLAY=W4P3INCH_DISP`
  - program passed
  - OpenOCD pre/post reset-run both reported healthy `PSE846GPS2DBZC4A` and `CYBOOT_SUCCESS`
- Display behavior:
  - CRT terminal values come from `aps_demo_state_t`
  - the center `mg/dL` card remains on the original dashboard/replay path and can differ from CRT `BG`
  - `APS_SAFETY_REASON_RAPID_FALL` now displays as `RAPID`
- Intent: this supersedes the prior live CRT restore point and is now the required golden/failsafe source of truth for continued APS demo-state work.

## Update 2026-06-14 Virtual Human + Short CRT Labels Restore Point
- Status: active required restore truth for the physically verified `Virtual Human` label plus shortened CRT APS labels.
- Commit: `89bf899`
- Artifacts:
  - `../failsafe/e8_insulin_pump_20260614_134223_virtual_human_label_refine_app_combined.hex`
  - `../failsafe/e8_insulin_pump_20260614_134223_virtual_human_label_refine_proj_cm55.elf`
- Physical result:
  - LCD live
  - GUI visible
  - `Virtual Human` visible above the glucose card
  - CRT labels render as `GLUCOSE`, `INS ONBD`, `CARBS`, `ACTION`, `INS RATE`, `SAFETY`
- Intent: this supersedes the prior `RAPID`-wording-only APS demo-state restore point and is now the required golden/failsafe source of truth.
