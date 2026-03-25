# EdgeAI CGM Insulin Pump Demo (NXP FRDM-MCXN947)

Firmware demo for an embedded CGM + insulin-pump UX on FRDM-MCXN947 with a model-assisted prediction layer, replay benchmarking, and safety-gated alerting.

This project introduces a new method of AI model generation delivered as a Visual Studio Code extension for embedded systems.

The extension automatically builds, trains, and benchmarks multiple lightweight model types such as anomaly detection, decision logic enhancement, and predictive models. It then compares performance metrics and exports the optimal solution as a compact Embedded Intelligence Layer in production-ready C++.
The generated layer integrates cleanly into existing firmware. A firmware engineer simply connects system inputs and output handshakes to the intelligence layer without restructuring host firmware or altering core control logic.

This architecture is designed to improve accuracy, prediction capability, and system-level decision making while preserving deterministic behavior and production stability. The AI layer operates as an isolated enhancement, reducing regression risk after deployment and protecting field reliability.

For more info contact: rmhaberkern@gmail.com

![insulin_pump](https://github.com/user-attachments/assets/1033010d-696f-45b3-b7d1-bed7aa6203d0)

## Hardware Platform And Sensor BOM

- Main board:
  - `FRDM-MCXN947` (NXP Freedom development board)
  - MCU: `MCXN947`
- Display stack:
  - 3.5-inch 320x480 parallel TFT module used by this project (`PAR-S035` integration path)
  - LCD controller IC: `ST7796S`
  - Touch controller IC: `GT911`
- On-board/primary motion sensor:
  - `FXLS8974CF` (NXP accelerometer)
- ST sensor-shield path used by runtime auto-detect (optional, when connected):
  - IMU: `LSM6DSO16IS` or `LSM6DSV16X`
  - Magnetometer: `LIS2MDL`
  - Barometric pressure: `LPS22DF`
  - Temperature: `STTS22H`
  - Humidity/temperature companion: `SHT40`
  - Additional accel path support: `LIS2DUXS12`

Notes:
- The firmware auto-detects available shield sensors at runtime and uses whichever supported set is present.
- For activity state and motion context, the main fusion path uses accelerometer + gyro + barometric trend signals.

## Safety Notice

This repository is an engineering prototype and evaluation demo. It is **not** a cleared or approved medical device and must not be used for clinical treatment decisions.

## What This Project Demonstrates

- Real-time embedded UI for a pump-style cockpit.
- CGM preprocessing and short-horizon prediction (`+15m`, `+30m` display).
- Prediction-driven hypo/hyper warning integration.
- Replay-driven benchmarking with on-device scoring.
- Clear separation between host firmware controls and model inference logic.

## Embedded Intelligence Layer (EIL) Architecture

The Embedded Intelligence Layer is the model/inference subsystem that sits on top of deterministic host firmware.

### Host Firmware Responsibilities

- Sensor ingestion and data integrity checks.
- Timebase, replay control, storage, and deterministic scheduling.
- UI rendering and user interaction.
- Hard gating and safe fallback behavior (AI enable/disable, prediction validity gates).

### EIL Responsibilities

- Feature preparation from CGM stream state.
- Model inference for short-horizon predictions.
- Confidence-linked prediction behavior.
- Prediction signal handoff to warning/overlay logic.

### Why this split improves ease-of-use and safety

- Firmware remains debuggable and deterministic even when models change.
- Model upgrades can be evaluated without rewriting control/timing code.
- Safety checks remain in firmware, independent of model internals.
- Operators can disable AI while preserving core UI/runtime functionality.

## Model Summary (Current Runtime)

- Runtime focus: CGM prediction support.
- Primary scored horizon: `+15m`.
- Display-only secondary horizon: `+30m`.
- On-screen center format:
  - `PRED 15M:xxx 30M:xxx`

Scoring policy (current baseline):
- scoring window uses the intended warmup split:
  - first 2 hours: pre-eval (no final score)
  - next 2 hours: model scoring window
- score is model-eligible only (`model_ok` with confidence gate), not fallback-trend-only paths
- `MODEL SCORE` is realism-weighted:
  - 50% tolerance-hit rate (from `+15m` evaluated horizon)
  - 35% MAE quality mapping
  - 15% model confidence
- tolerance window for hit scoring: `±5%` (minimum 3 mg/dL)
- minimum steady eval count required before showing score
- `+30m` remains displayed and contributes to MAE blend, but does not drive hit-rate numerator/denominator

## Human Activity States And Real-World Integration

The runtime computes human activity intensity states:
- `REST`
- `LIGHT`
- `MODERATE`
- `ACTIVE`
- `HEAVY`

These states are derived from motion and context, then used as control signals for dosing-assist logic and warnings.

### Sensor Inputs Driving Activity Classification

- Accelerometer:
  - body motion magnitude and orientation stability
  - cadence-like movement signatures
- Gyroscope:
  - angular motion and dynamic posture changes
  - movement burst characterization
- Barometric pressure:
  - vertical movement cues (up/down elevation trends, stair-like transitions)
  - suppression of false “effort” under vibration-only conditions

### How To Integrate Into A Real Prototype

In a production-oriented prototype, feed activity state into prediction and dosing as explicit features:
- current activity state and confidence
- short rolling history of activity transitions
- vertical-motion context from barometric trend
- transport-context gate (e.g., suppress false effort during vehicle vibration)

Recommended architecture:
1. Run sensor fusion and activity-state detection at fixed cadence in host firmware.
2. Publish activity state/confidence into model feature buffer.
3. Apply hard safety gates in firmware (SQI/fault/rate limits) independent of model outputs.
4. Use model output only for assistive modulation, never as the sole control authority.

### Optional ST AI Biosensor Offload Path

For lower host compute load, an external smart biosensor/AI IMU path can precompute:
- activity class
- movement confidence
- event flags (step-like bursts, posture changes)

This offloads part of feature extraction/classification from the MCU/NPU, reducing CPU budget and simplifying host-side pipelines. The host firmware still remains the safety authority for gating, limits, and final actuation policy.

### Vehicle And Non-Human Motion Filtering

Yes, the runtime includes transport-context filtering to suppress false activity escalation from non-human high-speed movement (for example, car/air vibration or transit motion).

Current behavior:
- Transport mode and confidence are estimated from fused accel/gyro/barometric context.
- Vehicle-like signatures are prevented from being treated as true human exertion.
- Dose modulation is damped in vehicle contexts (`CAR`, `AIR`) so vibration does not over-reduce insulin.
- Barometric trend is used as a consistency signal to distinguish elevation transitions from pure vibration.

Design intent:
- Keep alerts and activity state realistic during motion/transport.
- Avoid overreacting to high-frequency vibration that is not metabolic effort.
- Preserve firmware safety gates as final authority even when transport filtering is active.

## Runtime Flow

1. Boot and initialize runtime + display.
2. Load/replay dataset path and CGM processing chain.
3. Run model inference on schedule.
4. Render predictions and warning state.
5. Evaluate prediction quality over replay and show score/MAE indicators.

## Build and Flash

### Prerequisites

- Linux host
- Arm GNU toolchain
- LinkServer installed and accessible in `PATH`
- FRDM-MCXN947 connected through MCU-Link

### Standard Commands

```bash
./scripts/preflight.sh
./scripts/build.sh
./scripts/flash.sh
```

One-step:

```bash
./scripts/build_and_flash.sh
```

Direct board tooling:

```bash
./tools/build_frdmmcxn947.sh debug
./tools/flash_frdmmcxn947.sh
```

## Active Restore Baseline

- Golden: `GOLDEN-2026-02-25-R11`
- Failsafe: `FAILSAFE-2026-02-25-R11`

Artifacts:
- `failsafe/edgeai_medical_device_demo_cm33_core0_golden_2026-02-25-R11.bin`
- `failsafe/edgeai_medical_device_demo_cm33_core0_failsafe_2026-02-25-R11.bin`

Failsafe restore:

```bash
./tools/flash_failsafe.sh failsafe/edgeai_medical_device_demo_cm33_core0_failsafe_2026-02-25-R11.bin
```

## Controls

- Timeline area: `LIVE`, `PLAY/STOP`
- `REC` control: recording workflow
- `*`: settings
- `?`: help

## Key Files

- `src/edgeai_medical_device_demo.c`
  runtime orchestration, replay/timing, firmware control flow
- `src/cgm_preprocess.c`
  CGM preprocessing + prediction interface
- `src/gauge_render.c`
  LCD composition, overlays, prediction text, score visuals
- `src/ext_flash_recorder.c`
  external flash recording/replay persistence
- `docs/PROJECT_STATE.md`
  authoritative change and verification history
- `docs/OPS_RUNBOOK.md`
  operational command baseline

## Repository Layout

- `src/` firmware source
- `docs/` state/runbook/history/TODO
- `scripts/` common workflow commands
- `tools/` board/toolchain helpers
- `failsafe/` staged restore binaries
- `model/` model export artifacts
- `mcuxsdk_ws/` generated build workspace

## License

MIT (`LICENSE`)
