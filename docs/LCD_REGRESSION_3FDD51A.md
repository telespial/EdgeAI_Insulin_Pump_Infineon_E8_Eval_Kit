# LCD Regression Report: `3fdd51a`

## Summary
- Bad commit: `3fdd51a` (`add one-shot aps boot probe`)
- Last LCD-good commit before the probe: `cbe7b58`
- Recovery branch: `recover-lcd-after-aps-probe`
- Physical LCD confirmed recovered after clean rebuild and default probe-free flash.

## Symptom
- The LCD went blank after the probe-enabled image was introduced.
- The first recovery attempt was misleading because stale probe-enabled artifacts were still being flashed.
- UART-only evidence was not enough to claim LCD success.

## Cause Hypothesis
- Stale probe-enabled build artifacts likely survived the first flash attempt.
- Boot-time APS execution from `main()` was also a risk because it ran before the display path was fully trusted again.

## Recovery Action
- Tightened the probe gate to `APP_APS_EMBEDDED_PROBE == 1`.
- Ran a full `make clean` with the documented ModusToolbox environment.
- Rebuilt the default image from scratch.
- Reflashed the probe-free image.
- Ran OpenOCD `reset run`.
- Physically confirmed the LCD recovered.

## New Rules
- Do not call APS runtime modules from boot or `main()` until a LCD-safe delayed or sidecar execution model is designed.
- Clean rebuild is required after toggling `APP_APS_EMBEDDED_PROBE`.
- Physical LCD verification is required before claiming success.
- UART boot alone is not enough to claim LCD success.

## Notes
- The recovery branch is intentionally left in place so the evidence trail stays intact.
- This regression is treated as a process issue as well as a firmware issue.
