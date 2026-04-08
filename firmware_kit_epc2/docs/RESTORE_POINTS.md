# RESTORE_POINTS

## Purpose
Known-good checkpoints for fast recovery when development changes break boot, display, or flashing.

## Active Restore Tags
- `golden-e8-insulin-pump`
- `golden-e8-insulin-pump-20260408`
- `failsafe-e8-insulin-pump`
- `failsafe-e8-insulin-pump-20260408`

## Latest Verified Baseline (2026-04-08)
- Board: `PSE846GPS2DBZC4A` (Rev `B0`)
- Git commit: `f05711e`
- Build:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Flash:
  - OpenOCD with `QSPI_FLASHLOADER=PSE84_SMIF.FLM` (see `OPS_RUNBOOK.md`)
- Artifact:
  - `build/app_combined.hex`
- Restore artifacts:
  - `failsafe/restore_points/golden_20260408_095131_app_combined.hex`
  - `failsafe/restore_points/failsafe_20260408_095131_app_combined.hex`
  - `failsafe/restore_points/20260408_095131_sha256.txt`

## Restore workflow
```bash
git fetch --all --tags
git checkout <commit-or-tag>
```

Rebuild and reflash using `OPS_RUNBOOK.md`.
