# EdgeAI Insulin Pump & Artificial Pancreas Research Platform

Embedded glucose prediction, physiology-aware insulin pump control, safety supervision, and E84 edge-AI deployment.

## Project Overview

This repository is an Infineon PSoC Edge E84 embedded artificial pancreas and insulin pump research platform. It combines replay-driven simulation, deterministic physiology context, prediction, control, safety supervision, and embedded visualization on the E84 multi-core device family.

The project is intentionally research-oriented. It preserves the last stable Failsafe Restore Point as the baseline and layers APS functionality on top of the working E84 application rather than replacing it.

## Project Lineage

This project extends the last stable Failsafe Restore Point:

Original E84 firmware  
→ GUI  
→ Graphs  
→ Prediction display  
→ APS Predictor V2  
→ Physiology Engine  
→ Controller  
→ Safety Supervisor  
→ Replay/Regression  
→ Future RTOS migration

Early LCD/touch demo code existed only as bring-up infrastructure for the display and touch path. It is not the identity of this repository.

## Branch Status

- Current homepage: `main`
- Active APS development branch: `codex/bar-graph-tuning`
- The active APS implementation and regression work remain on `codex/bar-graph-tuning` until embedded smoke testing is complete.

## What the Platform Does

- CGM-style glucose input
- 15/30/60 minute prediction
- IOB engine
- COB engine
- activity engine
- OpenAPS-style controller
- safety supervisor
- replay simulation
- audit CSV output
- regression fixtures
- gold regression fixtures
- E84 smoke-test path

## Architecture

```text
CGM / Replay / Synthetic Input
→ Physiology Engine
→ Predictor V2
→ Controller
→ Safety Supervisor
→ GUI / Graphs / Audit Logs / Simulated Pump Output
```

## Embedded Target

The embedded target is the Infineon PSoC Edge E84 family:

- CM33 secure
- CM33 non-secure
- CM55

Supported kits:

- `KIT_PSE84_AI`
- `KIT_PSE84_EVAL_EPC2`

## Host Development Workflow

```bash
make -f host.mk test
make -f host.mk sim
make -f host.mk gold
make -f host.mk regression
make -f host.mk clean
```

## Embedded Smoke Test

Current smoke-test path under validation:

```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Validation

Validation covers:

- sample fixture matrix
- gold fixture matrix
- metric envelopes
- reason-code checks
- audit CSV schema
- host regression

## Safety Model

- The safety supervisor always wins.
- Activity does not directly command insulin.
- Predictor does not directly command insulin.
- Output is simulated pump output only.
- There is no real therapy use here.

## Documentation Index

- `docs/PROJECT_HISTORY.md`
- `docs/LEGACY_DISPLAY_BRINGUP.md`
- `docs/PHYSIOLOGY_ENGINE.md`
- `docs/PREDICTOR_V2_FEATURE_SCHEMA.md`
- `docs/RTOS_MIGRATION_PLAN.md`
- `docs/OSAL_PLAN.md`
- `docs/PROJECT_STATE.md`
- `VALIDATION_PLAN.md`
- `SAFETY_CHECKLIST.md`
- `ARCHITECTURE_DECISIONS.md`

## Research Disclaimer

This is a research and simulation platform only. It is not a medical device, not clinically validated, not FDA cleared, and must not be used for insulin dosing or patient care.
