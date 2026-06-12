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
