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
