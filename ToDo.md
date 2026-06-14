# ToDo

## Milestone 0 — Repo Review
- [x] Review existing handoff docs
- [x] Review current firmware skeleton
- [x] Review predictor V1
- [x] Review safety supervisor
- [x] Review simulation harness
- [x] Identify missing build/test wiring

## Milestone 1 — Host Build Foundation
- [x] Add host CMake or Makefile build
- [x] Compile predictor/controller/safety/metrics modules on host
- [x] Add basic test runner
- [x] Add replay data loading or synthetic replay fallback
- [x] Document build/run commands

## Milestone 2 — Predictor V2 Interface
- [ ] Preserve Predictor V1
- [x] Add Predictor V2 input/output structs
- [x] Add independent 15m model output
- [x] Add independent 30m model output
- [x] Add independent 60m model output
- [x] Add IOB/COB/basal/carb feature fields
- [x] Add slope/variance/volatility features
- [x] Add confidence and stale-CGM handling
- [x] Add unit tests
- [x] Add explicit 36-feature Predictor V2 vector
- [x] Add generated model tables for 15m/30m/60m
- [x] Add export-ready horizon evaluator with bounded fallback
- [x] Document the stable feature schema
- [x] Integrate deterministic physiology context into Predictor V2 features

## Milestone 3 — Physiology State
- [x] Add insulin event buffer
- [x] Add carb event buffer
- [x] Add IOB estimate
- [x] Add COB estimate
- [x] Add basal history estimate
- [x] Add no-dynamic-memory implementation
- [x] Add tests for meal/bolus scenarios

## Milestone 4 — OpenAPS-Style Controller
- [x] Add deterministic controller input/output structs
- [x] Add target range handling
- [x] Add basal multiplier output
- [x] Add suspend/reduce/normal/increase decisions
- [x] Add correction suggestion placeholder
- [x] Add reason codes
- [ ] Add tests for rising/falling/stable glucose

## Milestone 5 — Safety Supervisor
- [x] Add stale CGM gate
- [x] Add bad SQI gate
- [x] Add predicted hypo gate
- [x] Add rapid-fall gate
- [x] Add excessive IOB gate
- [x] Add impossible-value gate
- [x] Add low-confidence gate
- [x] Add safety reason codes
- [x] Add tests proving safety overrides controller

## Milestone 6 — Metrics
- [x] Add MAE
- [x] Add RMSE
- [x] Add Time in Range
- [x] Add Time Below Range
- [x] Add Time Above Range
- [x] Add missed-low count
- [x] Add false-low-warning count
- [x] Add safety-intervention count

## Milestone 7 — Simulation Harness
- [x] Add replay simulation
- [x] Add synthetic patient scenario runner
- [x] Add CSV output logs
- [x] Add comparison mode: baseline trend vs Predictor V2
- [x] Add comparison mode: controller without ML vs controller with ML
- [x] Add replay audit CSV with actual/future values and error columns
- [x] Add replay metrics summary for MAE/RMSE, TIR/TBR/TAR, and disagreement counts
- [x] Add committed replay fixtures for regression testing
- [x] Add stable host workflows for configure/build/test/sim/clean
- [x] Add fixture regression matrix runner
- [x] Add numeric regression envelopes for committed fixtures
- [x] Add gold fixture regression lane

## Milestone 8 — E84 Portability
- [ ] Add memory estimate
- [ ] Add CPU estimate
- [ ] Add flash estimate
- [ ] Add M33/M55 task split
- [ ] Identify what does not need Ethos-U55
- [ ] Identify optional future Ethos-U55 model path
- [ ] Avoid malloc/free
- [ ] Avoid POSIX-only dependencies in firmware modules

## Milestone 9 — Documentation
- [x] Update README
- [x] Update architecture docs
- [x] Update safety docs
- [x] Update validation docs
- [x] Add known limitations
- [x] Add research-only warning
- [x] Lock the audit CSV schema in docs
- [x] Document fixture matrix and host regression workflow
- [x] Document numeric envelope validation checks
- [x] Document gold fixture regression lane
- [x] Document Predictor V2 feature schema and export-ready model tables

## Milestone 10 — Final Review
- [x] Verify embedded smoke build, flash, and UART boot sequence
- [ ] Run all tests
- [ ] Confirm all generated code compiles
- [ ] Confirm no real pump-control claims
- [ ] Confirm safety layer always wins
- [ ] Write final summary

## Milestone 13 — Verification Evidence Report
- [x] Collect recovery/reset evidence
- [x] Collect host test and regression evidence
- [x] Collect embedded build/flash/verify evidence
- [x] Document host-only vs embedded-linked modules
- [x] Write `docs/BUILD_FLASH_VERIFICATION.md`

## Milestone 11 — RTOS Readiness Planning
- [ ] Add RTOS migration plan documentation
- [ ] Add OSAL plan documentation
- [ ] Record RTOS-ready architecture decisions
- [ ] Keep the current firmware behavior unchanged
- [ ] Avoid dynamic allocation in firmware modules
- [ ] Preserve regression outputs during future migration work

## Milestone 12 — Physiology Engine V1
- [ ] Add IOB engine module
- [ ] Add COB engine module
- [ ] Add activity engine module
- [ ] Extend host replay/simulation with physiology context
- [ ] Add physiology engine documentation
- [ ] Preserve GUI/graph/prediction baseline

## Milestone 13 — Embedded Smoke-Test Validation
- [x] Build the E84 firmware with `APP_APS_SMOKE_TEST=1`
- [x] Flash the three-core image to `APP_KIT_PSE84_EVAL_EPC2`
- [x] Verify the APS UART banner
- [x] Verify the five-step deterministic smoke sequence
- [x] Confirm low-glucose safety response in smoke mode

## Milestone 14 — Embedded APS Module Link
- [x] Link portable APS modules into `proj_cm55`
- [x] Keep APS runtime execution out of the hardware boot path
- [x] Reflash the CM55 image and confirm the LCD/GUI stays alive
- [x] Refresh the verification evidence report

## Milestone 15 — Embedded APS One-Shot Runtime Probe
- [x] Add a gated one-shot APS probe entrypoint
- [x] Call the probe once at boot after the UART banner
- [x] Verify the probe on CM55 hardware
- [x] Keep the GUI/LCD alive during probe boot
- [ ] Add a second physiology-context probe only if needed

## Milestone 16 — LCD / Probe Recovery Closure
- [x] Force a clean rebuild after changing `APP_APS_EMBEDDED_PROBE`
- [x] Reflash the default probe-free image
- [x] Confirm the UART boot banner appears without `APS probe:`
- [x] Confirm the physical LCD recovered after the clean rebuild and probe-free flash
- [x] Document that UART boot alone is not enough to claim LCD success
- [x] Commit the recovery branch with the final evidence trail

## Milestone 17 — Predictor V2 Training / Export Pipeline
- [x] Add a host-side training script for Predictor V2
- [x] Add a host-side export script for generated C/H artifacts
- [x] Add a host-side validation script for held-out replay data
- [x] Add a synthetic smoke CSV for pipeline sanity checks
- [x] Document the training/export workflow and smoke-data limitations
- [x] Run help checks, train the smoke model, and validate the export

## Milestone 18 — Predictor V2 Dataset Expansion
- [x] Add a deterministic synthetic dataset generator
- [x] Generate a larger committed synthetic training CSV
- [x] Train/export/validate on the expanded synthetic dataset
- [x] Re-run host regression suites
- [x] Commit the dataset expansion as its own milestone

## Milestone 19 — Predictor V2 Feature Enrichment
- [x] Audit the raw insulin-pump dataset columns and quality issues
- [x] Derive series-aware proxy physiology features from the real dataset
- [x] Retrain/export/validate Predictor V2 on the enriched real dataset
- [x] Keep the work host-only and leave embedded runtime behavior untouched

## Milestone 20 — Predictor V2 Export Readiness Review
- [x] Generate an export manifest for the enriched candidate model
- [x] Write an export-readiness report with risks and recommendation
- [x] Stage a review-only candidate snapshot under `firmware/model_candidates/`
- [x] Keep the active embedded firmware tables unchanged
## Milestone 21 — Predictor V2 Candidate V1 Dry Run
- [x] Swap the active tables to the candidate snapshot on a dry-run branch
- [x] Verify the embedded build still links
- [x] Measure image-size delta against the baseline
- [x] Document the host regression mismatch that blocks promotion
- [x] Keep the candidate as a review snapshot for now

## Milestone 22 — Candidate V1 Regression Analysis
- [x] Capture exact failing assertions
- [x] Compare old vs candidate behavior on the same cases
- [x] Classify the behavior shift and fixture failures
- [x] Keep Candidate V1 out of firmware promotion for now

## Milestone 23 — Falling-Bolus Safety Gap Review
- [x] Reclassify the falling-bolus replay fixture as an `EXCESSIVE_IOB` case
- [x] Add direct host tests for the low-prediction and controller-blocked paths
- [x] Align fixture and gold envelopes with the measured host behavior
- [x] Keep Candidate V1 as a review snapshot

## Milestone 24 — Candidate V1 Dry-Run Recheck
- [x] Confirm the active generated tables still match the candidate snapshot
- [x] Re-run host test/regression after the safety expectation fix
- [x] Rebuild the embedded image without flashing hardware
- [x] Measure the image size delta and confirm it stays at zero
- [x] Move the dry-run verdict to `READY FOR FIRMWARE PROMOTION PR`

## Milestone 25 — Candidate V1 Promotion PR Preparation
- [x] Write the PR-ready promotion summary
- [x] Record validation, safety-review, and size-delta evidence
- [x] Keep the branch ready for GitHub PR creation
- [x] Avoid any local merge or runtime change
- [ ] Capture a direct physical LCD photo/video for the merged `39f6361` image

- [ ] Record the physical LCD result for `move-accuracy-label-out-of-chart` before deciding whether chart-subtree invalidation is the root cause.

- [ ] Next safest APS display step: avoid arbitrary LVGL text mutation and use pre-rendered APS background art or existing numeric/bar paths only.
- [ ] Try the next display experiment by preserving the original numeric/percent-shaped format on an existing safe path instead of introducing arbitrary APS text.

- [x] Record the physical LCD result for `aps-screen-label-readout-v1` before attempting any richer APS text rotation.
- [x] Record the physical LCD result for the static `Artificial Pancreas` placeholder display candidate.
- [x] Intentionally promote the confirmed placeholder layout to the next golden/failsafe restore point after commit/tag/push review.
- [x] Write the next APS display milestone plan and branch plan before touching firmware again.
- [x] Run `BG`-only placeholder replacement as the next single-field display experiment from `2c8c6f1`.
- [x] Replace the remaining terminal placeholders with live `IOB`, `COB`, `INS`, and `SAFETY` values on the proven CRT path.

- [ ] Promote the live multi-field CRT terminal state to the next golden/failsafe restore point if the user wants to lock it in.
