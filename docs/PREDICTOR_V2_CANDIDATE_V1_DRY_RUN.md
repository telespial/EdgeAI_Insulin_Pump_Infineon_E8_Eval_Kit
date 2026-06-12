# Predictor V2 Candidate V1 Dry Run

## Scope
Build-only promotion rehearsal for Candidate V1.

No flash was performed.
No LCD/GUI/runtime behavior was changed.

## Branch
- `predictor-v2-candidate-v1-dry-run`

## Source State
- Active firmware tables were replaced with the candidate snapshot from `firmware/model_candidates/enriched_real_dataset_v1/`
- Active runtime logic was left unchanged

## Host Validation
- `make -f host.mk test`: failed
- `make -f host.mk regression`: failed

Observed failing checks:
- `controller can increase`
- `controller blocked reason emitted`
- `fixture safety reason present`

These failures appeared after the table swap and should be treated as a host-side expectation mismatch until proven otherwise.

## Embedded Build
- `make clean`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`

Result:
- build completed successfully
- no embedded compile or link errors
- no runtime flash was performed

## Size Comparison
- Baseline `build/app_combined.hex`: `5206276`
- Candidate V1 `build/app_combined.hex`: `5206276`
- Baseline `proj_cm55.elf`: `6355484`
- Candidate V1 `proj_cm55.elf`: `6355484`

Delta:
- flash: `0`
- RAM: `0`
- combined image size: `0`

## Risk Assessment
- Candidate V1 is build-compatible with the active embedded firmware layout
- The embedded image size did not change
- Host regression failures mean the candidate still needs behavioral review before promotion
- No hardware flash was performed, so LCD behavior was not revalidated in this step

## Decision
KEEP AS REVIEW SNAPSHOT

The candidate tables build cleanly, but the host test/regression mismatch is enough to keep this snapshot out of active firmware for now.
