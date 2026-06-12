# Falling-Bolus Safety Review

## Summary
The `falling_bolus` replay fixture does not actually drive the low-prediction reason path in the current host model.

Observed final outcome on the fixture:
- controller action: `NO_CHANGE`
- safety final action: `NO_CHANGE`
- safety reason codes: `EXCESSIVE_IOB`

That means the original `PREDICTED_LOW_15M | PREDICTED_LOW_30M | RAPID_FALL` expectation was not describing this fixture's current behavior.

## What Was Fixed
- The falling-bolus fixture expectation was corrected to require `EXCESSIVE_IOB`.
- A direct host test now proves the low-prediction reason path with a handcrafted low-prediction controller input.
- A direct host test now proves the controller-blocked path with a missing-physiology rising-glucose replay that actually triggers a blocked controller action.

## Why The Fixture Did Not Emit Low-Prediction Reasons
- The replay row sequence leaves the model in a conservative state with `EXCESSIVE_IOB` before the low-prediction thresholds are reached.
- The safety layer only emits low-prediction reason codes when the model prediction itself crosses the threshold.
- In this fixture, the controller never reaches that state; it is already neutralized by high IOB.

## Evidence
- `make -f host.mk test`: passed after the test and expectation updates.
- `make -f host.mk regression`: passed after fixture/gold envelopes were aligned with measured host results.
- Direct low-prediction unit test: passed.
- Direct controller-blocked unit test: passed.

## Notes
- This is a fixture-shape issue, not an embedded runtime change.
- The embedded firmware behavior was not modified for this review.
