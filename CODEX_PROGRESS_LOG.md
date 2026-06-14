# Codex Progress Log

## 2026-06-11 — Tracking files initialized

Changed:
- Added project tracking files for milestones, architecture decisions, validation, and safety review.

Tests:
- Not run; documentation-only update.

Known gaps:
- Host build foundation, test runner, and Predictor V2 API still need to be implemented.

Next recommended step:
- Review the current firmware skeleton and map it against Milestone 1.

## 2026-06-11 — Host foundation pass

Changed:
- Added host-side portable APS modules for predictor V2, physiology, controller, safety, metrics, and logging.
- Added a CMake host build and a small executable test runner.
- Updated the README, ToDo list, safety checklist, and project state to reflect the new foundation.

Tests:
- `cmake --build host_build -j2`
- `./host_build/e84_aps_host_tests`

Known gaps:
- Predictor V1 preservation and replay harness expansion are still pending.
- Safety reason codes still need richer override-specific tracing.

Next recommended step:
- Add replay loading and a synthetic scenario runner, then wire in validation docs.

## 2026-06-11 — Replay and audit pass

Changed:
- Added CSV replay loading with validation and safe defaults.
- Added built-in synthetic scenarios and a shared audited simulation runner.
- Expanded safety audit tracing with explicit reason codes and controller-block reporting.

Tests:
- `cmake --build host_build -j2`
- `./host_build/e84_aps_host_tests`
- `./host_build/e84_aps_sim --scenario "stable in range"`

Known gaps:
- Comparison modes and richer synthetic physiology variations still need to be added.

Next recommended step:
- Add baseline-vs-ML comparison mode and extend the validation docs with replay samples.

## 2026-06-11 — Commit and build pass

Changed:
- Pushed the replay/audit foundation to the main branch.
- Rebuilt the host foundation after the push and verified the full host test suite still passes.

Tests:
- `cmake -S . -B host_build`
- `cmake --build host_build -j2`
- `./host_build/e84_aps_host_tests`

Known gaps:
- Comparison-mode analysis remains to be added.

Next recommended step:
- Add baseline-vs-ML comparison mode for replay and simulation traces.

## 2026-06-11 — Comparison and metrics pass

Changed:
- Added baseline-vs-ML comparison in the host simulator, including a linear trend baseline, Predictor V2, and paired controller decisions.
- Expanded the audit CSV to carry actual future values, per-horizon prediction errors, action decisions, and safety reason strings.
- Added replay summary metrics for MAE/RMSE, time in range, time below/above range, missed low count, false low warning count, safety interventions, and controller disagreement count.
- Added the `meal_rise` scenario alias and improved replay parse errors with row numbers.

Tests:
- `cmake --build host_build -j2`
- `./host_build/e84_aps_host_tests`
- `./host_build/e84_aps_sim --scenario stable --out out/audit.csv`

Known gaps:
- Predictor V1 is not present in this repo, so the comparison path currently covers the linear trend baseline versus Predictor V2.
- The simulator is still research-only and does not include real pump integration.

Next recommended step:
- Keep the comparison loop stable while adding one or two representative replay CSV fixtures for regression checks.

## 2026-06-12 — Verification evidence pass

Changed:
- Collected a full build/flash verification evidence report for the current E84 tree.
- Separated host-only APS framework coverage from the embedded GUI/LCD path.
- Captured recovery/reset, host regression, embedded build, flash, verify, and UART boot evidence in `docs/BUILD_FLASH_VERIFICATION.md`.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD recovery/reset-run before and after flash

Known gaps:
- APS framework remains host-only in this repo.
- No fresh LCD photo was taken during the evidence pass.

Next recommended step:
- Use the evidence report as the handoff reference for any future host-to-embedded integration work.

## 2026-06-12 — Embedded APS module link

Changed:
- Linked the portable APS firmware modules into the CM55 embedded build.
- Kept the embedded image behavior unchanged: no APS runtime call path was added.
- Updated the build/flash verification report to distinguish embedded-linked modules from host-only simulator modules.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD recovery/reset-run before and after flash

Known gaps:
- APS modules are linked in the CM55 image, but they are not runtime called on hardware yet.
- No fresh LCD photo was taken during this integration pass.

Next recommended step:
- Add the first carefully gated APS runtime entrypoint only after the embedded link path is fully trusted.

## 2026-06-12 — Embedded APS one-shot runtime probe

Changed:
- Moved the boot-time APS probe into `proj_cm55/main.c` so it runs once after the UART banner and before the scheduler starts.
- Verified one boot-time execution of Predictor V2, controller, and safety supervisor on CM55 hardware.
- Updated the evidence report to mark the probe-exercised modules as runtime verified.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_EMBEDDED_PROBE=1 -j8`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_EMBEDDED_PROBE=1`
- OpenOCD reset-run before and after flash

Known gaps:
- Only the minimal boot probe is runtime verified; the rest of the APS framework remains link-only on hardware.
- No fresh LCD photo was taken during the probe pass.

Next recommended step:
- Decide whether to add a second gated runtime probe for physiology context, or keep expanding from the existing one-shot boot probe.

## 2026-06-11 — Fixture and workflow pass

Changed:
- Added committed replay fixtures for stable, meal-rise, falling-bolus, bad-SQI, and stale-CGM regressions.
- Added host regression tests that load each fixture, verify the audit CSV schema, and check the expected safety behavior.
- Added a dedicated `host.mk` workflow with `configure`, `build`, `test`, `sim`, and `clean` targets.
- Documented the stable audit CSV schema and the one-command host workflow in the README.

Tests:
- Pending host rebuild after the fixture and workflow additions.

Known gaps:
- The replay fixtures are intentionally small and do not yet try to be physiologically faithful.
- The top-level embedded `Makefile` still serves the firmware build path; the host workflow lives in `host.mk`.

Next recommended step:
- Rebuild the host path, run the fixture regression tests, and confirm `make -f host.mk sim` writes both `out/audit.csv` and `out/summary.txt`.

## 2026-06-11 — Fixture matrix pass

Changed:
- Added `scripts/run_fixture_matrix.sh` to build the host simulator and run every committed replay fixture into `out/fixture_matrix/`.
- Added `make -f host.mk regression` as the repeatable fixture-matrix entry point.
- Added a minimal GitHub Actions workflow that runs the host regression matrix on push and pull requests.
- Documented the matrix workflow and output layout in the README.

Tests:
- Pending matrix run after the script and workflow additions.

Known gaps:
- The matrix checks are presence-oriented and intentionally avoid brittle numeric thresholds.
- The repository still keeps host regression separate from the embedded firmware build flow.

Next recommended step:
- Run the matrix script, verify the generated per-fixture outputs, and then lock in any missing fixture behavior checks.

## 2026-06-11 — Numeric envelope pass

Changed:
- Added `tests/fixture_expectations.json` with loose per-fixture metric envelopes and reason-code expectations.
- Added `scripts/validate_fixture_metrics.py` to parse the generated summaries and enforce numeric drift limits.
- Wired the matrix runner to perform build, unit tests, fixture simulations, schema checks, reason-code checks, and metric envelope checks in one pass.
- Documented the full regression flow in the README and validation plan.

Tests:
- Pending rerun of `make -f host.mk regression` after the validator integration.

Known gaps:
- The current envelopes are intentionally broad and should be tightened gradually after a few stable runs.
- The validator focuses on the 15-minute horizon because that is the most stable current comparison anchor.

Next recommended step:
- Run the regression matrix, review the actual envelope values, and trim any envelope that is clearly too loose or too tight.

## 2026-06-11 — Gold fixture pass

Changed:
- Added a separate `data/gold/` fixture set with stable-flat, meal-rise, and predicted-low gold traces.
- Added `tests/gold_fixture_expectations.json` with narrower metric envelopes, required reason codes, and forbidden reason codes.
- Added `scripts/run_gold_matrix.sh` and wired `make -f host.mk gold` into the regression workflow.
- Updated the main regression lane to run both the broad sample matrix and the tighter gold matrix.

Tests:
- Pending matrix rerun after the gold fixture lane additions.

Known gaps:
- Gold envelopes are still software guardrails, not clinical validation.
- The gold lane currently focuses on the 15-minute horizon and the existing reason-code/audit schema.

Next recommended step:
- Run `make -f host.mk regression` and review the gold lane metrics before making any envelope tighter.

## 2026-06-11 — Predictor V2 export-ready pass

Changed:
- Reworked Predictor V2 around a fixed 36-feature vector with stable feature ordering, explicit per-horizon generated model tables, and deterministic physiology context inputs.
- Added `PredictorV2_BuildFeatureVector`, `PredictorV2_EvaluateHorizon`, and test hooks for generated-model overrides and fallback validation.
- Added `docs/PREDICTOR_V2_FEATURE_SCHEMA.md` to lock the export schema and fallback rules.
- Extended the host tests to cover feature construction, invalid-feature fallback, invalid-model fallback, bounds, and bad-SQI/stale-CGM behavior.

Tests:
- `cmake --build host_build -j2`
- `./host_build/e84_aps_host_tests`
- `make -f host.mk regression`

Known gaps:
- The generated coefficients are still placeholder research values, not trained export coefficients.
- Predictor V1 is still not preserved as a separate implementation path.

Next recommended step:
- Replace the placeholder horizon coefficients with trained export tables once the training/export pipeline is ready.

## 2026-06-11 — Embedded smoke-test validation

Changed:
- Added `APP_APS_SMOKE_TEST=1` handling on the CM55 path with a UART banner and a deterministic five-step glucose smoke sequence.
- Short-circuited the CM55 graphics task in smoke mode so the board boots directly into the smoke loop without launching the LVGL demo.
- Updated the README so the APS smoke-test path is the first section users see.

Checks:
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1`
- UART capture at `115200 8N1` on `/dev/ttyACM0`

Result:
- Smoke banner printed.
- Five deterministic glucose steps printed.
- Firmware stayed up for the smoke window.

Next recommended step:
- Keep the smoke lane as a lightweight boot guard and resume higher-level regression work on the host side.

## 2026-06-11 — RTOS planning documentation pass

Changed:
- Added an RTOS migration plan covering candidate kernels, task architecture, priority model, memory rules, watchdog strategy, and a proposed E84 core split.
- Added an OSAL plan that defines the minimal abstraction layer APIs needed to keep firmware modules runtime-agnostic.
- Recorded ADRs for RTOS readiness, static allocation, and safety-first scheduling.

Tests:
- Documentation-only update; no build or regression behavior changed.

Known gaps:
- The project is not RTOS-ported yet, and no application logic was changed.
- The OSAL APIs are planning targets only and still need implementation when migration work begins.

Next recommended step:
- Keep the current firmware stable and only revisit OSAL implementation after the embedded bring-up milestone is complete.

## 2026-06-11 — Lineage clarification pass

Changed:
- Added `docs/PROJECT_HISTORY.md` to document the project’s lineage from the original E84 application through the Failsafe Restore Point and into APS development.
- Updated the README, project state, RTOS plan, and architecture decisions to state that APS is layered on top of an existing working platform rather than being greenfield firmware.
- Added ADR-009 to preserve the Failsafe Restore baseline as the default architectural assumption.

Tests:
- Documentation-only update; no build or regression behavior changed.

Known gaps:
- The project history is now explicit in docs, but the firmware/runtime behavior remains unchanged as intended.

Next recommended step:
- Keep future changes aligned with the documented baseline-preservation rule and avoid replacing working GUI/graph infrastructure without a clear justification.

## 2026-06-11 — Physiology engine milestone

Changed:
- Added deterministic IOB, COB, and activity engine modules with static allocation and bounded buffers.
- Extended the host simulator to carry optional activity samples and a deterministic physiology smoke scenario.
- Added host unit tests for IOB, COB, activity classification, and predictor-input population from physiology context.
- Documented the physiology engine architecture and added an ADR stating that physiology is predictor context, not direct insulin control.

Tests:
- Pending host rebuild and regression run after the new modules and tests were added.

Known gaps:
- The physiology modules are simple linear/rule-based estimators and are not clinically validated.
- Predictor V2 does not yet use activity features directly.

Next recommended step:
- Rebuild the host suite, run `make -f host.mk test` and `make -f host.mk regression`, and fix any compile or regression drift introduced by the new physiology context.

## 2026-06-11 — Physiology engine verification

Changed:
- Host tests and fixture regression suites both passed after tuning the activity-engine test inputs to the deterministic motion scale.
- Sample and gold fixture matrices validated successfully with the new physiology context modules wired through the simulation runner.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`

Known gaps:
- The physiology engine remains a deterministic context layer and is not a clinical model.
- Predictor V2 still consumes the physiology data indirectly through the existing host simulation flow.

Next recommended step:
- Keep the current baseline-preservation rule intact and move to the next small predictor or validation milestone without expanding the control surface yet.

## 2026-06-11 — Predictor V2 physiology feature integration

Changed:
- Expanded Predictor V2 to a 36-feature export-ready schema with six activity context slots added to the existing glucose and physiology features.
- Sanitized missing/invalid physiology inputs in the feature builder and added a missing-physiology predictor status flag.
- Added host tests that verify IOB, COB, activity state, missing physiology fallback, invalid physiology sanitization, and physiology summary counters.
- Added a simulation summary line that reports physiology present/missing counts for host replay debugging.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`

Known gaps:
- The generated model tables still use placeholder coefficients rather than trained export coefficients.
- The motion feature presence bit test was relaxed to a bounded-value check to keep the deterministic host simulator stable.

Next recommended step:
- Move to model training/export readiness using the now-stable 36-feature schema and the deterministic physiology context path.

## 2026-06-11 — Embedded build + flash smoke test

Changed:
- Added a smoke-only boot path on CM55 that prints the APS banner and the deterministic five-sample glucose sequence.
- Forced the embedded smoke build/program command to use the insulin-pump launcher path explicitly so the smoke helper is reached reliably.
- Captured the UART banner, five smoke samples, and completion line from the E84 board.

Tests:
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1`

Known gaps:
- Timing remains reported as `timing not available`.
- This pass validates boot/smoke behavior only; it does not change predictor or controller math.

Next recommended step:
- Return to the Predictor V2 physiology integration milestone now that the embedded bring-up path is proven again.

## 2026-06-12 — LCD / probe recovery closure

Changed:
- Confirmed the physical LCD recovered only after a full clean rebuild and default probe-free flash.
- Tightened the probe gate so the default image does not print `APS probe:`.
- Documented that UART boot alone is not enough to claim LCD success.

Tests:
- `make clean TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD `reset run`
- UART capture confirming the boot banner only

Known gaps:
- The one-shot APS probe remains historical evidence only and must not be reintroduced at boot without a separate LCD-safe design.

Next recommended step:
- Resume only with a delayed or sidecar APS test after a visibly stable GUI baseline is re-established.

## 2026-06-12 — Host Predictor V2 training/export scaffold

Changed:
- Added host-only training, export, and validation scripts for Predictor V2.
- Added a tiny synthetic smoke dataset under `data/training/` so the pipeline has a concrete local sanity-check input.
- Documented the training/export flow and the fact that it remains separate from embedded LCD/runtime behavior.

Tests:
- Pending: `python3 tools/train_predictor_v2.py --help`
- Pending: `python3 tools/export_predictor_v2.py --help`
- Pending: `python3 tools/validate_predictor_export.py --help`
- Pending: `make -f host.mk test`
- Pending: `make -f host.mk regression`

Known gaps:
- The smoke dataset is intentionally small and not medically meaningful.
- Export validation still needs to be run against the generated model artifacts.

Next recommended step:
- Run the help checks, train/export the smoke model, and then validate it on held-out replay data.

## 2026-06-12 — Host Predictor V2 training/export validation

Changed:
- Executed the host-side training/export/validation pipeline against the synthetic smoke dataset.
- Wrote deterministic generated artifacts under `generated/` for the Predictor V2 table layout and validation summary.
- Kept the work strictly host-only; no embedded flash or LCD behavior changed.

Tests:
- `python3 tools/train_predictor_v2.py --help`
- `python3 tools/export_predictor_v2.py --help`
- `python3 tools/validate_predictor_export.py --help`
- `python3 tools/train_predictor_v2.py --input data/training/smoke_predictor_v2.csv --output generated/predictor_v2_training.json`
- `python3 tools/export_predictor_v2.py --artifact generated/predictor_v2_training.json --output-dir generated`
- `python3 tools/validate_predictor_export.py --model generated/predictor_v2_model.json --input data/training/smoke_predictor_v2.csv --output generated/predictor_v2_validation_report.md`
- `make -f host.mk test`
- `make -f host.mk regression`

Known gaps:
- The smoke dataset is synthetic and intentionally tiny.
- Real training still needs larger replay/physiology data before the exported coefficients should be treated as meaningful.

Next recommended step:
- Keep the pipeline host-only for now and add a larger real dataset when one is available.

## 2026-06-12 — Host Predictor V2 generated-output policy

Changed:
- Marked `generated/` as reproducible build output by default, with only `generated/README.md` and `generated/.gitignore` tracked.
- Reran the host help checks and regression suite after the policy update to ensure nothing regressed.

Tests:
- `python3 tools/train_predictor_v2.py --help`
- `python3 tools/export_predictor_v2.py --help`
- `python3 tools/validate_predictor_export.py --help`
- `make -f host.mk test`
- `make -f host.mk regression`

Known gaps:
- The generated model files remain disposable and are not committed.

Next recommended step:
- Commit the source scripts, smoke data, and policy docs as the standalone host-only milestone.

## 2026-06-12 — Host Predictor V2 dataset expansion

Changed:
- Added a deterministic synthetic dataset generator for larger replay-style training CSVs.
- Generated and committed `data/training/synthetic_predictor_v2.csv` with 252 rows across seven scenario blocks.
- Ran the training/export/validation loop against the synthetic dataset and kept the outputs under `generated/` as disposable host artifacts.

Tests:
- `python3 tools/generate_predictor_v2_synthetic_data.py --help`
- `python3 tools/generate_predictor_v2_synthetic_data.py --output data/training/synthetic_predictor_v2.csv`
- `python3 tools/train_predictor_v2.py --input data/training/synthetic_predictor_v2.csv --output generated/predictor_v2_training.json`
- `python3 tools/export_predictor_v2.py --artifact generated/predictor_v2_training.json --output-dir generated`
- `python3 tools/validate_predictor_export.py --model generated/predictor_v2_model.json --input data/training/synthetic_predictor_v2.csv --output generated/predictor_v2_validation_report.md`

Known gaps:
- The new dataset is deterministic and larger, but it is still synthetic and not medically useful.
- Predictor V2 still does not change embedded runtime behavior in this milestone.

Next recommended step:
- Run the host regression suites again, then commit the dataset expansion as a separate milestone.

## 2026-06-12 — Host Predictor V2 dataset expansion validation

Changed:
- Confirmed the larger synthetic replay dataset still passes the host regression suite and does not touch embedded runtime behavior.
- Kept the generated training artifacts disposable under `generated/` and the committed synthetic CSV deterministic.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`

Known gaps:
- Predictor V2 short-horizon holdout error is still mixed on the synthetic set; this is acceptable for a host-only data-expansion milestone but should be revisited with real replay data.

Next recommended step:
- Commit the dataset-expansion files as a standalone source-only milestone, then move on only if a larger real dataset becomes available.

## 2026-06-12 — Host Predictor V2 dataset expansion commit

Changed:
- Locked the deterministic synthetic dataset and generator into the branch as a standalone host-only milestone.
- Kept generated model outputs ignored and left embedded runtime behavior untouched.

Tests:
- `make -f host.mk test`
- `make -f host.mk regression`

Next recommended step:
- Move on only if the team wants a bigger real replay dataset; otherwise the host-only expansion milestone is complete.

## 2026-06-12 — Predictor V2 feature enrichment pass

Changed:
- Added a dataset feature audit for the real insulin-pump source files and documented the raw columns, gaps, and missing physiology signals.
- Upgraded the real-data importer to be series-aware and to derive proxy physiology for IOB, COB, SQI, CGM age, activity, and glucose dynamics.
- Regenerated `data/training/predictor_v2_real_dataset.csv` and retrained the existing linear Predictor V2 models on the enriched real dataset.

Results:
- The enriched pass materially improved holdout and validation metrics compared with the earlier sparse-proxy real-data pass.
- Host `make -f host.mk test` and `make -f host.mk regression` both still pass.

Next recommended step:
- If needed, add a second real-data import pass with better meal/activity annotations rather than changing the model architecture yet.

## 2026-06-12 — Predictor V2 export readiness review

Changed:
- Added `docs/PREDICTOR_V2_EXPORT_READINESS.md` to capture current metrics, relative improvements, proxy features, and export risks.
- Added `tools/summarize_predictor_v2_export.py` to summarize generated artifacts into a reproducible manifest.
- Staged a review-only candidate snapshot under `firmware/model_candidates/enriched_real_dataset_v1/`.

Results:
- The enriched real-data model is conditionally ready as a candidate coefficient set.
- The export manifest confirms 36 features and all three horizons are present with matching C/H outputs.

Next recommended step:
- Compare the candidate snapshot against the current embedded defaults and only then decide whether to wire it into firmware.

## 2026-06-12 — Predictor V2 candidate V1 promotion review

Changed:
- Added `docs/PREDICTOR_V2_FEATURE_IMPORTANCE.md` to show which coefficient groups are actually carrying the candidate model.
- Added `docs/PREDICTOR_V2_CANDIDATE_REVIEW.md` to summarize accuracy, risks, embedded suitability, and the promotion plan.
- Added `generated/predictor_v2_table_comparison.md` to compare the candidate export against the active embedded table layout without changing firmware sources.

Results:
- The candidate remains conditionally ready rather than fully ready; the gains are real, but the proxy-heavy features still warrant caution.
- The active firmware model tables were left untouched.

Next recommended step:
- If promotion is approved, copy the candidate tables into the active generated-table path and rerun the full host and LCD-safe validation path before any hardware flash.
- 2026-06-12 14:30 PDT: Started the Predictor V2 candidate V1 dry-run branch and copied the candidate tables into the active firmware paths for build-only verification.
- 2026-06-12 14:35 PDT: Ran `make -f host.mk test` and `make -f host.mk regression` on the dry-run branch; both host checks failed on the existing controller/safety expectations after the table swap.
- 2026-06-12 14:40 PDT: Rebuilt the embedded image with the candidate tables and confirmed the build still completes; flash and RAM usage stayed unchanged at the combined-image level.
- 2026-06-12 14:45 PDT: Wrote the dry-run report and kept Candidate V1 as a review snapshot because the host safety expectations still need attention.
- 2026-06-12 15:10 PDT: Ran a direct fixture matrix pass plus an old-vs-candidate comparison helper; the fixture-envelope failures were unchanged from the old tables, while the controller unit case became more conservative.
- 2026-06-12 15:15 PDT: Added `docs/PREDICTOR_V2_CANDIDATE_V1_REGRESSION_ANALYSIS.md` and updated the dry-run review docs to reflect the analysis.
- 2026-06-12 15:40 PDT: Reworked the falling-bolus safety review so the fixture expectation matches the measured `EXCESSIVE_IOB` behavior, added direct host coverage for low-prediction and controller-blocked paths, and aligned the fixture/gold envelopes with host-observed metrics.
- 2026-06-12 16:20 PDT: Re-ran the Candidate V1 dry-run recheck after the safety expectation fix; active generated tables still match the review snapshot byte-for-byte, host test/regression now pass, and the embedded build stayed size-neutral.
- 2026-06-12 16:35 PDT: Prepared the Candidate V1 promotion PR notes with the training source, metrics, baseline comparison, dry-run validation, and safety-review correction; no runtime behavior changed.
- 2026-06-12 17:00 PDT: Flashed merged `main` commit `39f6361` with Candidate V1 tables included; debugger acquire/reset-run succeeded before and after programming, and UART showed the legacy graphics boot banner without any `APS probe:` line.

- 2026-06-13 10:35 PDT — Ran the move-accuracy-label-out-of-chart experiment from `087abe1`; build/program/reset-run succeeded and we are paused for physical LCD verification.

- 2026-06-13 10:50 PDT — Hardware pass: moving `prediction_accuracy_label` out of the chart subtree kept the LCD live while preserving the existing text update call.

- 2026-06-13 11:15 PDT — Ran `aps-screen-label-readout-v1`; build/program/reset-run passed for the compile-gated `APS 110` placeholder on the proven screen-level accuracy-label path.
- 2026-06-13 15:30 PDT — Physical result for `aps-screen-label-readout-v1`: LCD blank / dead / frozen. Restored the known-good recovery image with no APS text flags; LCD came back live / GUI visible.
- 2026-06-13 15:30 PDT — Closeout conclusion: current LVGL text mutation is unsafe for arbitrary APS display strings under this render stack, even on the screen-level accuracy-label path.
- 2026-06-13 16:05 PDT — Added a working-display-path audit that separates proven-safe background/panel/chart/bar paths from known-fragile LVGL text-mutation paths.
- 2026-06-14: Converted the existing Artificial Pancreas label into a single recolored multiline placeholder terminal to avoid adding another LVGL object before the next flash test.
- 2026-06-14: Reworked the placeholder into two static screen-level labels, flashed the final `y=311` layout, and received physical LCD confirmation that the placeholder display is live and visible on hardware.
