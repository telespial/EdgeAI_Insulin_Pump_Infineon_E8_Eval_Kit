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

Current foundation status:
- Host tests cover predicted-low, bad-SQI, and conservative override behavior.
- Replay and scenario runs now share one audited simulation path.
