# APS Runtime Truth Table

## Purpose
Capture what the current embedded APS demo is actually doing, what values are live, and what is still only research/demo quality.

## Current Milestone
- Branch: `aps-glucose-unified-display`
- Hardware truth:
  - LCD live
  - GUI visible
  - CRT updating
  - unified glucose path updating beyond the earlier five-point freeze window

## Live Runtime Pipeline
```text
VirtualPatientV1
-> ApsDemoState_Step
-> PredictorV2_Update
-> OpenApsController_DetermineBasal
-> SafetySupervisor_Apply
-> Dashboard / CRT formatting
```

## Field Truth Table

| Visible field | Current source | Live on hardware | Notes |
|---|---|---:|---|
| CRT `GLUCOSE:` | `aps_demo_state_t.bg_mgdl` | yes | Sourced from `VirtualPatientV1` through `ApsDemoState_Step()` |
| CRT `INS ONBD:` | `aps_demo_state_t.iob_u` | yes | From `IobEngine` state carried by the APS pipeline |
| CRT `CARBS:` | `aps_demo_state_t.cob_g` | yes | From `CobEngine` state carried by the APS pipeline |
| CRT `ACTION:` | `aps_demo_state_t.action_text` | yes | Derived from controller output |
| CRT `INS RATE:` | `aps_demo_state_t.insulin_u_hr` | yes | Requested basal after controller/safety path |
| CRT `SAFETY:` | `aps_demo_state_t.safe_text` | yes | Derived from safety flags |
| center `MG/DL` card | APS demo-state glucose path | yes | Now intentionally aligned with the CRT/chart glucose source |
| chart glucose line | APS demo-state glucose path | yes | No longer directly fed by the older replay-array display path |
| prediction score / confidence display | existing dashboard path | yes | Present as UI signal, not a medical confidence claim |

## What Is Actually Running
- `PredictorV2_Update()` runs each APS dashboard step.
- `OpenApsController_DetermineBasal()` runs each APS dashboard step after predictor output is available.
- `SafetySupervisor_Apply()` runs after controller output and can change the final delivered recommendation.
- The displayed CRT values are not hardcoded placeholders in the current milestone.

## What Is Not Yet Proven
- Medical validity
- Sensor-driven real-world accuracy
- Long-duration robustness across many scenario classes
- Clinically meaningful prediction accuracy on-board

## Safe Claim
The current system is a **live embedded APS research/demo integration** with active predictor, controller, safety, and virtual-patient plumbing.

## Unsafe Claim
It would be inaccurate to claim that the current embedded predictor/controller behavior is medically validated or production-ready.

## Next Recommended Phase
1. Preserve this milestone as the golden/failsafe restore point.
2. Add longer-duration host/runtime checks around the unified APS path.
3. Improve virtual-patient realism without changing the proven display path.
