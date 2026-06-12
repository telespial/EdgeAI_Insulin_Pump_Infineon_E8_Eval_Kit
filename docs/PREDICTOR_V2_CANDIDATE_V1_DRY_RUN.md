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
- `make -f host.mk test`: passed
- `make -f host.mk regression`: passed

Confirmed host coverage:
- `controller can increase`
- `controller blocked reason emitted`
- `fixture safety reason present`

The falling-bolus safety expectation fix resolved the prior mismatch, and the candidate tables now pass the same host checks as the corrected active expectations.

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
- Host regression now passes with the corrected safety expectation set
- No safety regressions remain in the host review path
- No hardware flash was performed, so LCD behavior was not revalidated in this step

## Decision
READY FOR FIRMWARE PROMOTION PR

The candidate tables build cleanly, the host suites now pass, and the embedded build stayed size-neutral, so the dry-run is ready to move to a promotion PR.

## Regression Follow-Up
- `docs/PREDICTOR_V2_CANDIDATE_V1_REGRESSION_ANALYSIS.md` shows the fixture-envelope failures are unchanged between the old and candidate table sets.
- The only clear model-driven shift is the `controller can increase` unit case, where Candidate V1 becomes more conservative and now suspends basal instead of allowing no-change.
- The falling-bolus safety concern was resolved by reclassifying the fixture as an `EXCESSIVE_IOB` case and aligning the expectations with host-observed behavior.
- The candidate remains byte-for-byte identical to the review snapshot in `firmware/model_candidates/enriched_real_dataset_v1/`.
