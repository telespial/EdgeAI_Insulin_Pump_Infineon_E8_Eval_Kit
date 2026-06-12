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

## Milestone 10 — Final Review
- [ ] Run all tests
- [ ] Confirm all generated code compiles
- [ ] Confirm no real pump-control claims
- [ ] Confirm safety layer always wins
- [ ] Write final summary
