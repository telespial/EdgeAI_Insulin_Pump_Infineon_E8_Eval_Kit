# APS CRT Real State Wiring

## Scope
- Replace CRT placeholder values with APS demo-state values while preserving the proven LCD-safe CRT layout.

## Source Mapping
- `BG` -> `aps_demo_state_t.bg_mgdl`
- `IOB` -> `aps_demo_state_t.iob_u`
- `COB` -> `aps_demo_state_t.cob_g`
- `ACT` -> `aps_demo_state_t.action_text`
- `INS` -> `aps_demo_state_t.insulin_u_hr`
- `SAFETY` -> `aps_demo_state_t.safe_text`

## Embedded Call Path
- `edgeai_insulin_pump_app_start()`
  - calls `ApsDemoState_Init()`
- `dashboard_timer_cb()`
  - calls `push_sample()`
- `push_sample()`
  - calls `update_aps_terminal_label()`
- `update_aps_terminal_label()`
  - computes `now_s`
  - calls `ApsDemoState_Step(now_s, &state)`
  - calls `ApsDemoState_FormatTerminal(&state, buffer, sizeof(buffer))`
  - updates the existing CRT terminal label

## Host Validation
- `make -f host.mk test`
- `make -f host.mk regression`

## Safety Notes
- No new LVGL objects
- No CRT layout change
- No direct hardcoded medical placeholder values in the display formatter
- No real insulin actuation
- The board LCD remains the physical source of truth after flashing
