# Virtual Patient V2

## Goal
- Upgrade the APS demo-state source into a deterministic looping physiology model that continuously produces realistic `BG`, `IOB`, `COB`, `ACT`, `INS`, and `SAFE` behavior.

## Files
- `firmware/include/virtual_patient_v2.h`
- `firmware/src/virtual_patient_v2.c`
- `firmware/src/aps_demo_state.c`

## API
- `void VirtualPatientV2_Init(void);`
- `bool VirtualPatientV2_Step(uint32_t now_s, float delivered_insulin_u_hr, virtual_patient_v2_state_t *state);`

## State Fields
- `epoch_s`
- `bg_mgdl`
- `meal_cob_g`
- `insulin_iob_u`
- `insulin_sensitivity`
- `activity_factor`
- `dawn_factor`
- `basal_u_hr`
- `meal_event`
- `bolus_event`

## Scenario
- `5 minute` step cadence
- `48 step` looping cycle
- breakfast meal at step `4` with `60g`
- snack at step `26` with `18g`
- exercise sensitivity window at steps `24..30`
- dawn rise window at steps `38..45`

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
