# RESTORE_POINTS

## Purpose
Known-good checkpoints for fast recovery when development changes break boot, display, or flashing.

## Active Restore Tags
- `golden-e8-insulin-pump`
- `golden-e8-insulin-pump-20260325`
- `failsafe-e8-insulin-pump`
- `failsafe-e8-insulin-pump-20260325`

## Latest Verified Baseline (2026-03-25)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Artifact:
  - `build/app_combined.hex`

## Restore workflow
```bash
git fetch --all --tags
git checkout <commit-or-tag>
```

Rebuild and reflash using `OPS_RUNBOOK.md`.
