# Validation Plan

## Predictor Validation
- Replay historical CGM traces through the predictor.
- Compare 15m, 30m, and 60m outputs against labels.
- Compare baseline trend predictions against Predictor V2 on the same replay rows.
- Measure MAE, RMSE, and low-miss rate.

Pass criteria:
- No NaN or Inf outputs.
- Each horizon remains within safe clamp limits.
- 30m and 60m are independently generated.

## Controller Validation
- Run stable, rising, falling, meal, and bolus scenarios.
- Verify controller action matches expected conservative behavior.

Pass criteria:
- No unsafe increase during low-confidence or rapid-fall cases.
- Corrections remain simulation-only.

## Safety Validation
- Inject stale CGM, bad SQI, and impossible values.
- Verify the safety supervisor overrides controller output.

Pass criteria:
- Safety always wins over controller recommendations.
- Unsafe requests are reduced, clamped, or blocked.

## Replay Validation
- Run the Python replay harness against saved sample data.
- Confirm deterministic CSV outputs.
- Validate parser errors for impossible timestamps and glucose values.
- Validate missing optional CSV columns use safe defaults.
- Confirm the audit CSV includes actual future values, prediction errors, controller decisions, and reason codes.
- Run the committed `data/sample_replay_*.csv` fixtures through the host simulator.
- Run the full fixture matrix script to ensure every committed replay stays covered.
- The matrix now performs build, unit tests, fixture simulations, schema checks, reason-code checks, and numeric metric envelope checks.
- Run the gold fixture matrix to lock down a smaller, higher-signal regression lane.

Pass criteria:
- Same inputs produce same outputs.
- Logs contain prediction, controller, and safety fields.
- Invalid replay rows are rejected with clear errors.

## Synthetic Scenario Validation
- Generate synthetic meals, boluses, and activity cases.
- Compare controller behavior across scenarios.
- Confirm each built-in scenario reaches the intended safety branch.
- Exercise the `meal_rise` alias alongside the long-form scenario name.

Pass criteria:
- Low glucose leads to reduce/suspend behavior.
- High IOB blocks aggressive recommendations.
- Scenario runs emit auditable safety reason codes.

## Host Unit Tests
- Test predictor feature extraction.
- Test physiology accumulation.
- Test controller action selection.
- Test safety override logic.
- Test replay summary metrics and audit CSV header coverage.
- Test every committed replay fixture for expected safety behavior.
- Test the scripted fixture matrix output and failure conditions.

Pass criteria:
- Tests pass consistently on the host platform.

## Regression Tests
- Lock a small set of canonical replay traces.
- Re-run them after every meaningful change.
- Keep the broader sample matrix and the narrower gold matrix separate so each catches different drift modes.

Pass criteria:
- No regression in safety behavior.
- Prediction metrics do not unexpectedly degrade.

## E84 Embedded Smoke Tests Later
- Build the firmware for the E84 target.
- Run replay-only mode first.
- Verify logs and watchdog behavior.

Pass criteria:
- Firmware boots reliably.
- No real pump integration is introduced.
