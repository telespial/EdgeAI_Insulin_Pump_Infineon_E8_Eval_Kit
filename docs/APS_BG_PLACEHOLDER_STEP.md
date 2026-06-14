# APS BG Placeholder Step

## Status
- Branch: `aps-bg-placeholder-step`
- Base restore point: `2c8c6f1`
- Planning baseline before code change: `472a22a`
- Physical LCD result: confirmed live / GUI visible and `BG` shows `112`

## Exact Visible Change
- Changed only the green placeholder line:
  - from `BG   110`
  - to `BG   112`

## What Stayed Unchanged
- `IOB  1.2`
- `COB   18`
- `ACT HOLD`
- `INS  0.8`
- `SAFE NORM`
- No new LVGL objects
- No new timers
- No Predictor V2, controller, safety, or physiology runtime calls

## Verification
- Build: passed
- Program: passed
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD: live / GUI visible and `BG` shows `112`

## Outcome
- This is the first safe single-field APS placeholder readout step.
- The next allowed display-only step is `IOB`-only placeholder replacement.
