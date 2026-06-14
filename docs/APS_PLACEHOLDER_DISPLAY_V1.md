# APS Placeholder Display V1

## Status
- Source baseline: `9fb4e7c` (`finalize artificial pancreas truth-point layout`)
- Physical LCD result: confirmed live / GUI visible
- Scope: display-only placeholder layout

## What Changed
- Kept the existing LCD-safe dashboard baseline intact.
- Kept `gDashboard.prediction_accuracy_label` on the screen-level safe path from the earlier LCD fix.
- Added one static white `Artificial Pancreas` header label.
- Added one static green multiline placeholder readout label below it.
- Final placeholder block position moved to `x=30`, `y=311`.

## Placeholder Content
```text
Artificial Pancreas
BG   110
IOB  1.2
COB   18
ACT HOLD
INS  0.8
SAFE NORM
```

## Important Constraints
- No APS runtime math is running.
- No Predictor V2, controller, safety, or physiology calls were added.
- No arbitrary dynamic APS text mutation was introduced.
- This remains a research/demo display placeholder only.

## Physical Verification
- Build: passed
- Program: passed
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- LCD: live / GUI visible

## Current Interpretation
- This placeholder layout is now a candidate success beyond `9fb4e7c`.
- It should only become the next promoted restore point after intentional commit/tag/push handling.
