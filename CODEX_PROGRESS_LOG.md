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
