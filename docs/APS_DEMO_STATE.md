# APS Demo State

## Purpose
- Provide one deterministic APS demo-state pipeline for the CRT readout without adding LVGL objects or hardcoded medical values in the display formatter.

## APIs
- `bool ApsDemoState_Init(void);`
- `bool ApsDemoState_Step(uint32_t now_s, aps_demo_state_t *state);`
- `bool ApsDemoState_FormatTerminal(const aps_demo_state_t *state, char *buffer, size_t buffer_size);`

## State Fields
- `bg_mgdl`
- `iob_u`
- `cob_g`
- `action`
- `insulin_u_hr`
- `safety_flags`
- `action_text`
- `safe_text`

## Deterministic Input
- `ApsDemoState_Step()` now consumes `VirtualPatientV2` output instead of a finite hand-typed BG sequence or the earlier simpler `VirtualPatientV1`.
- Step cadence remains `5 minutes` per state step.
- `VirtualPatientV2` continuously generates bounded BG, COB, IOB, insulin sensitivity, activity factor, dawn factor, and basal context.
- SQI remains `95%`.
- CGM age remains `0 s`.

## Pipeline
- `VirtualPatientV2` produces BG / COB / IOB / activity / dawn context
- `predictor_v2` evaluates the virtual-patient-derived physiology state
- `controller_openaps` computes action / requested basal
- `safety_supervisor` post-processes the controller result
- delivered insulin feeds back into `VirtualPatientV2`
- CRT formatting reads only from `aps_demo_state_t`

## Mapping
- `NO_CHANGE` -> `HOLD`
- `INCREASE_BASAL` -> `INCR`
- `REDUCE_BASAL` -> `RED`
- `SUSPEND_BASAL` -> `STOP`
- `CORRECTION_SUGGESTION` -> `CORR`

- predicted low -> `LOW`
- excessive IOB -> `IOB`
- bad SQI -> `SQI`
- stale CGM -> `STALE`
- rapid fall -> `RAPID`
- otherwise -> `NORM`

## Notes
- No malloc
- No real pump output
- No hardware insulin command
- CRT fallback text is error-only; normal display data is sourced from `aps_demo_state_t`
- The large center `mg/dL` card still follows the original dashboard path and can differ from CRT `BG:`.
