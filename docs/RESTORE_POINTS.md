# Restore Points (Golden)

## Current Golden
- Tag: `golden-e8-insulin-pump-20260325`
- Date: `2026-03-25`
- Verified on hardware with build + flash.

## Restore
```bash
git fetch --tags
git checkout golden-e8-insulin-pump-20260325
```

## Validation rule
Only create/move golden tags after successful build and flash on target hardware.
