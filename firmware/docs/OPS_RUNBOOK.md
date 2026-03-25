# Operations Runbook

## Preflight
- `./tools/preflight.sh`

## Build
- Configure `- Build Command:` in `docs/PROJECT_STATE.md`
- Run: `./tools/build.sh`

## Flash
- Configure `- Flash Command:` and `- Binary:` in `docs/PROJECT_STATE.md`
- Run: `./tools/flash.sh`

## Build + Flash
- `./tools/build_and_flash.sh`

## Notes
- This project uses ModusToolbox multi-core targets (`proj_cm33_s`, `proj_cm33_ns`, `proj_cm55`).
- Keep all commands and paths local to this repository.
