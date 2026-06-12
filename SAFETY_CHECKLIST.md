# Safety Checklist

- [x] Controller cannot bypass safety supervisor
- [x] Bad SQI prevents aggressive insulin recommendation
- [x] Stale CGM prevents aggressive insulin recommendation
- [x] Predicted low causes reduce/suspend
- [x] Rapid fall causes reduce/suspend
- [x] Excessive IOB blocks correction recommendation
- [x] Impossible glucose values are rejected
- [x] Low confidence falls back to conservative mode
- [x] All safety interventions emit reason codes
- [x] Tests prove safety overrides controller
- [x] Audit CSV records final safety action and reason codes for every step
- [ ] Physiology context is advisory only and does not directly command insulin
- [x] Predictor V2 accepts physiology context without bypassing safety checks

Current foundation status:
- Host tests cover predicted-low, bad-SQI, and conservative override behavior.
- Replay and scenario runs now share one audited simulation path.
- Baseline-vs-ML comparison mode is now available in the host simulator.
- Replay fixtures now lock down stale-CGM, bad-SQI, meal-rise, and falling-bolus regressions.
