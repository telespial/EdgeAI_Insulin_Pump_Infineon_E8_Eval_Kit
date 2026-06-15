# START_HERE

## Project Identity

This repository is the active PSoC Edge E84 insulin pump / artificial pancreas research platform.

It is **not** currently a Smart Pong bring-up repo in practice, even though older Smart Pong material still exists in project history and legacy docs.

## Current Hardware Truth

Current LCD-good golden restore point:

- branch: `vp2-background-on-v1-visible`
- commit: `b06cdd0`

Current physical result at that point:

- LCD live
- GUI visible
- delayed APS/V2 startup path active
- `250 ms` one-shot arm delay before normal `5000 ms` runtime cadence

Current major open bug:

- explicit `APP_VP_SCENARIO_MODE=1..6` images can pass build/program/OpenOCD checks but still blank the LCD on hardware
- see `VP2_MODE_LCD_FAILURE.md`

## Mandatory Read Order

Read these first for every session:

1. `../AGENTS.md`
2. `PROJECT_STATE.md`
3. `COMMAND_LOG.md`
4. `BUILD_FLASH_VERIFICATION.md`
5. `OPS_RUNBOOK.md`
6. `RESTORE_POINTS.md`
7. `VP2_MODE_LCD_FAILURE.md`

Then read as needed:

8. `VIRTUAL_PATIENT_V2.md`
9. `VIRTUAL_PATIENT_V2_BREAKFAST_FREEZE.md`
10. `KNOWN_ISSUES.md`
11. `UNSAFE_PATTERNS.md`
12. `TODO.md`

## What Repo You Are In

Active repo root:

- `Infineon/PSOC_EDGE_E8_EVAL/firmware_kit_epc2/`

Key code areas:

- runtime entry: `proj_cm55/main.c`
- active insulin-pump app: `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/`
- display port: `proj_cm55/lv_port_disp.c`
- LVGL config: `proj_cm55/lv_conf.h`
- active APS / physiology modules: `firmware/src/`

## Rules That Matter Most

1. Physical LCD observation overrides UART.
2. Do not treat build/program success as proof of a good image.
3. Use the LCD-safe OpenOCD pre-reset / program / post-reset workflow.
4. Do not add random LVGL object/text mutations on this display path.
5. Keep display experiments separate from APS/runtime experiments.
6. Preserve rollback to the current golden point before risky changes.
7. Update `PROJECT_STATE.md` and `COMMAND_LOG.md` after meaningful work.

## Current Safe / Unsafe Summary

### Safe right now

- reflashing the current golden image
- working from commit `b06cdd0`
- using the delayed-start runtime path
- preserving the current LCD / CRT layout

### Unsafe / blocked right now

- explicit scenario-mode images without more isolation
- assuming OpenOCD health means LCD health
- reintroducing the `THINKING` banner path
- arbitrary LVGL display mutations without physical verification

## Best Next Debug Focus

If continuing from the current state, the next highest-value task is:

```text
isolate why APP_VP_SCENARIO_MODE images blank the LCD
```

Start with:

- `VP2_MODE_LCD_FAILURE.md`
- `PROJECT_STATE.md`
- `BUILD_FLASH_VERIFICATION.md`

## Legacy Note

Older Smart Pong and early LVGL bring-up material is retained for historical context only.

If you need that lineage, use:

- `SMART_PONG_PORT_PLAN.md`
- `LEGACY_DISPLAY_BRINGUP.md`
- `PROJECT_HISTORY.md`
