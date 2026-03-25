# START_HERE

## Goal
Build and flash the standalone insulin pump demo on E8 Eval Kit EPC2.

## Read order
1. `../README.md`
2. `PROJECT_STATE.md`
3. `OPS_RUNBOOK.md`
4. `HARDWARE_SETUP.md`
5. `COMMAND_LOG.md`
6. `TODO.md`

## Source of truth
- Runtime entry: `../proj_cm55/main.c`
- Insulin app: `../proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/`
- Display profile: `../common.mk` (`CONFIG_DISPLAY=W4P3INCH_DISP`)
