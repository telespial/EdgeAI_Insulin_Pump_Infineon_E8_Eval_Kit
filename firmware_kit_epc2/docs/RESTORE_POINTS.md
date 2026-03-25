# RESTORE_POINTS

## Current baseline
- Date: `2026-03-25`
- Mode: insulin standalone (`APP_INSULIN_PUMP_MODE=1`)
- Build/flash verified from this repo.

## Restore workflow
```bash
git fetch --all --tags
git checkout <commit-or-tag>
```

Rebuild and reflash using `OPS_RUNBOOK.md`.
