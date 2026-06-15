# Virtual Patient V2

## Goal
- Upgrade the APS demo-state source into a deterministic looping physiology model that continuously produces realistic `BG`, `IOB`, `COB`, `ACT`, `INS`, and `SAFE` behavior.

## Files
- `firmware/include/virtual_patient_v2.h`
- `firmware/src/virtual_patient_v2.c`
- `firmware/src/aps_demo_state.c`

## API
- `void VirtualPatientV2_Init(void);`
- `void VirtualPatientV2_InitWithScenario(vp_scenario_t scenario);`
- `bool VirtualPatientV2_Step(uint32_t now_s, float delivered_insulin_u_hr, virtual_patient_v2_state_t *state);`
- `vp_scenario_t VirtualPatientV2_GetScenario(void);`

## Scenario Engine

`VirtualPatientV2` now supports deterministic compile-time-selectable scenarios:

- `VP_SCENARIO_NORMAL`
- `VP_SCENARIO_BREAKFAST`
- `VP_SCENARIO_EXERCISE`
- `VP_SCENARIO_DAWN`
- `VP_SCENARIO_LOW_GLUCOSE`
- `VP_SCENARIO_RAPID_FALL`

The default remains `VP_SCENARIO_BREAKFAST` so the current visible dashboard behavior does not change unless a scenario flag is explicitly selected.

Supported scenario flags:

- `APP_VP_SCENARIO_NORMAL=1`
- `APP_VP_SCENARIO_EXERCISE=1`
- `APP_VP_SCENARIO_DAWN=1`
- `APP_VP_SCENARIO_LOW=1`
- `APP_VP_SCENARIO_RAPID=1`

## State Fields
- `epoch_s`
- `step_index`
- `cycle_step`
- `bg_mgdl`
- `target_bg_mgdl`
- `debug_code`
- `scenario`
- `meal_cob_g`
- `insulin_iob_u`
- `insulin_sensitivity`
- `activity_factor`
- `dawn_factor`
- `basal_u_hr`
- `meal_event`
- `bolus_event`

## Default Scenario
- `5 minute` step cadence
- `48 step` looping cycle
- breakfast meal at step `4` with `60g`
- snack at step `26` with `18g`
- exercise sensitivity window at steps `24..30`
- dawn rise window at steps `38..45`

## Additional Scenarios

- `NORMAL`
  - near-target baseline
  - two small meals/snacks later in the cycle
  - mostly `HOLD` / mild controller behavior
- `EXERCISE`
  - higher starting BG
  - early carb support
  - stronger activity sensitivity window
- `DAWN`
  - elevated dawn factor early in the cycle
  - mild breakfast later
  - upward glucose drift before controller compensation
- `LOW_GLUCOSE`
  - lower baseline
  - persistent negative target bias
  - rescue carbs later in the cycle
- `RAPID_FALL`
  - early simulated bolus event
  - stronger negative target bias
  - designed to drive high-IOB / falling-glucose behavior

## Model
- meals feed `CobEngine`
- delivered basal feeds `IobEngine`
- `COB` raises BG
- `IOB * insulin_sensitivity` lowers BG
- dawn factor adds upward drift
- activity factor increases sensitivity
- a bounded velocity term smooths the next BG transition

## Bounds
- BG clamped `60..250 mg/dL`
- insulin sensitivity clamped `0.75..1.45`
- delivered basal clamped `0.0..3.0 U/hr`
- no malloc or dynamic allocation

## APS Integration
- `ApsDemoState_Init()` calls `VirtualPatientV2_Init()`
- `ApsDemoState_Step()` calls `VirtualPatientV2_Step()`
- predictor physiology is filled from the V2 state
- controller output is safety-processed
- post-safety requested basal is fed back into the next patient step

## Validation
- `make -f host.mk test`
- `make -f host.mk regression`
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`

## Update 2026-06-15 — Scenario Engine Milestone

- Added `vp_scenario_t` and scenario-aware initialization.
- Preserved the current dashboard default by leaving `BREAKFAST` as the no-flag default.
- Added host coverage proving all six scenarios:
  - stay bounded
  - continue changing over time
  - produce distinct meal / bolus / glucose signatures
- Validation passed:
  - `make -f host.mk test`
  - `make -f host.mk regression`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- No flash performed in this milestone.

## One-Hour Host Summary
- `bg=[107,167]`
- `iob_max=0.17U`
- `cob_max=60.00g`
- `basal_max=2.00U/hr`
- breakfast meal observed

## Limitations
- deterministic research/demo model only
- not medically validated
- no real pump actuation
- no sensor-driven activity yet

## Next Step
- Flash `VirtualPatientV2` with the known LCD-safe procedure and confirm CRT values remain alive beyond the earlier finite demo window.

## Update 2026-06-15 — Breakfast Trace
- `VP2_BREAKFAST_STEP = 4`
- host tracing across steps `0..12` confirmed:
  - step `4` succeeds
  - `meal_event == true`
  - `COB > 0`
  - `debug_code >= 250`
  - subsequent steps continue normally
- This means the first breakfast event does not fail inside `VirtualPatientV2` host logic.
