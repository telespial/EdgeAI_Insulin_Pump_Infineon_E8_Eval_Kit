# RESTORE_POINTS

## Purpose
Known-good checkpoints for fast recovery when development changes break boot, display, or flashing.

## Tagged Restore Points (Repo-Level)
- `failsafe-e8-smart-pong`
  - Meaning: Last known-stable Smart Pong restore point for rapid recovery.
- `golden-e8-smart-pong`
  - Meaning: Current development base for Smart Pong feature work.

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
