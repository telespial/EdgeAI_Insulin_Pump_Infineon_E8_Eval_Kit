# APS Display Milestone Plan

## Current Restore Point
- Active golden/failsafe restore point: `2c8c6f1`
- Physical result:
  - LCD live
  - GUI visible
  - white `Artificial Pancreas` header visible
  - green placeholder block visible

## Guardrails
- Do not touch the existing GUI structure.
- Do not add new LVGL objects.
- Do not add arbitrary LVGL text mutations outside the already proven placeholder path.
- Replace only one already-visible placeholder value at a time.
- Each field gets its own build, flash, and physical LCD confirmation.
- Do not introduce Predictor V2, controller, safety, or physiology runtime until the display path is proven stable with deterministic values.
- Keep display experiments and APS runtime experiments separate.

## Recommended Sequence
1. `BG` only
2. `IOB`
3. `COB`
4. `ACT`
5. `INS`
6. `SAFE`

## First Milestone
### BG-Only Placeholder Replacement
- Goal: change only the visible `BG` placeholder value to a deterministic number.
- Leave `IOB`, `COB`, `ACT`, `INS`, and `SAFE` unchanged.
- If the LCD stays live and the BG value renders correctly, commit that step as the first safe readout milestone.

## Stop Rule
- If any single-field replacement causes LCD blanking, freezing, or misrendering:
  - stop immediately
  - restore `2c8c6f1`
  - document the failure
  - do not advance to the next field

## Success Criteria Per Step
- Build passes
- Program passes
- OpenOCD pre/post reset-run stays healthy
- Physical LCD remains live
- Only the intended single placeholder value changes

## Deferred Until Later
- Real Predictor V2 outputs
- Real controller outputs
- Real safety outputs
- Any rotating/multi-line dynamic status logic
- Any timer changes
- Any additional display elements
