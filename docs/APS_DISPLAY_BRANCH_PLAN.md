# APS Display Branch Plan

## Base Commit
- Branch from: `2c8c6f1`
- Reason: this is the current physically verified golden/failsafe restore point.

## Branch Strategy
- Create one branch per single-field placeholder experiment.
- Do not stack unverified display changes.
- Do not mix display experiments with APS runtime experiments.

## Planned Branches
1. `aps-bg-placeholder-step`
   - Scope: deterministic `BG` placeholder value only
2. `aps-iob-placeholder-step`
   - Scope: deterministic `IOB` placeholder value only
3. `aps-cob-placeholder-step`
   - Scope: deterministic `COB` placeholder value only
4. `aps-act-placeholder-step`
   - Scope: deterministic `ACT` placeholder value only
5. `aps-ins-placeholder-step`
   - Scope: deterministic `INS` placeholder value only
6. `aps-safe-placeholder-step`
   - Scope: deterministic `SAFE` placeholder value only

## Rules For Every Branch
- Branch from the last physically verified LCD-safe commit only.
- Change one field only.
- Build, flash, and physically verify before any commit promotion.
- If it fails, restore `2c8c6f1` and close out with docs only.
- If it passes, commit that branch result and decide whether it becomes the next branch base.

## Runtime Separation
- Do not start Predictor V2/controller/safety runtime branches until the placeholder display path is proven stable field-by-field.
- Future runtime branches should start only after the last display-only placeholder milestone is physically verified.
