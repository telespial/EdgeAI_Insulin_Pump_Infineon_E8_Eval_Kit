# EdgeAI Smart Pong - Infineon PSOC Edge E8 Evaluation Kit

## Real World AI Learning Demonstration

Compare three control behaviors in the same system:
- `HUMAN`: touch-driven paddle control
- `ALGO`: fixed classic CPU predictor (non-learning baseline)
- `EdgeAI`: NPU-assisted predictor with adaptive runtime tuning

Observe online learning in live play:
- The adaptive controller updates from rally outcomes (hits/misses), then changes paddle speed, anticipation, and tracking noise.

`SKILL` modes allow direct side comparison:
- `AI/ALGO`: left uses adaptive EdgeAI, right uses fixed ALGO
- `ALGO/AI`: left uses fixed ALGO, right uses adaptive EdgeAI
- `2AI`: both sides use adaptive EdgeAI

Validate session learning vs persistent learning:
- `PERSIST ON`: left/right learned profiles carry across new matches and are restored independently from onboard flash after reboot/power cycle.
- `PERSIST OFF`: learned profile is cleared, forcing cold-start behavior.

Demonstrate deployment-style tradeoffs:
- NPU path is used when enabled and available, with CPU fallback always present for robustness.
- Bottom telemetry helps correlate behavior with NPU/fallback timing and latency trends.

## Hardware Components

- Manufacturer: Infineon Technologies
- Name: PSOC Edge E8 Evaluation Kit
- Board target: `KIT_PSE84_EVAL_EPC2` (primary active target in this workspace)

- Manufacturer: Waveshare
- Name: 4.3" Raspberry Pi DSI display (800x480) with touch panel
- Display config: `W4P3INCH_DISP`
- Display support policy: 4.3-inch only in this repository (`CONFIG_DISPLAY=W4P3INCH_DISP` enforced)

- Accelerometer source on E8 platform:
- Bosch BMI270 (board-level integration used by current platform HAL)

## Gameplay Rules

- Objective: first side to selected `MATCH` target (`11`, `100`, or `1K`) wins.
- Scoring: miss on one side gives opponent `+1`.
- Ball motion: reflects from top and bottom arena walls.
- Paddle collision: reflects with hit-position-based angle shaping ("english").
- Serve behavior: each serve starts from center and is directed toward the side that conceded previous point.
- Serve variation: adaptive AI-style serve selection mixes straight/single-axis/diagonal patterns with directional bias away from receiver paddle position.

Match flow:
- `P0` (AI vs AI): winner styling + confetti then auto-reset.
- `P1` and `P2`: winner styling + confetti then `NEW GAME? YES/NO`.

Absolute cap:
- At `999` points on either side, winner styling/confetti are shown and game auto-resets to `0-0` after 30 seconds.

Persistence behavior at cap reset:
- `PERSIST OFF`: reset continues without persisted carry-over.
- `PERSIST ON`: persisted profile remains active and is restored.

Bottom HUD:
- Shows per-side match wins (`WINS`) centered relative to net line.

## Controls

Touch control strips at left/right edges:
- Left strip controls player 1 paddle
- Right strip controls player 2 paddle
- Touch `Y` controls paddle height
- Touch `X` inside strip controls paddle depth

Top bar:
- Reserved for settings UI

Alternate control sources:
- Custom mappings can be routed through platform input HAL (`platform/input_hal.c`).
- Volume events are also routed via platform input HAL (`VOL DN`/`VOL UP`).

## Settings

- `Players`: `0`, `1`, `2`
- `Difficulty`: `1`, `2`, `3`
- `NPU`: `ON`, `OFF`
- `SKILL`: `2AI`, `AI/ALGO`, `ALGO/AI`
- `PERSIST`: `ON`, `OFF`
- `MATCH`: `11`, `100`, `1K` (`1K` uses `999` target)
- `TARGET`: `ON`, `OFF`
- `SPEED++`: `ON`, `OFF`
- `VOL`: `0..100` (`0` = mute, `100` = max)
- `New Game`: immediate score reset and clears `WINS` totals to `000`

Current startup defaults (this workspace):
- `P0`
- `D2`
- `MATCH=100`
- `PERSIST=OFF`
- `VOL=60`

## AI and NPU Implementation

This firmware uses an embedded TensorFlow Lite Micro model path with runtime fallback architecture.

Runtime AI paths:
- CPU analytic intercept predictor (deterministic baseline/fallback)
- NPU-assisted predictor path (when enabled and available)

NPU path control:
- Enabled via project configuration (`CONFIG_EDGEAI_USE_NPU` pathing, where applicable in build profile)

Inference stack:
- TensorFlow Lite Micro runtime
- NPU backend integration path in platform layer

AI behavior:
- Predicts paddle intercept targets `(y_hit, z_hit, t_hit)`
- Difficulty presets apply reaction-time, speed limits, and tracking noise
- `AI ON/OFF` controls NPU-assisted usage; CPU fallback always remains available

Runtime telemetry:
- On-screen telemetry can show NPU invoke rate, fallback rate, last latency, and moving-average latency in AI-driven modes.

## AI Design (ALGO vs EdgeAI)

Goal in mixed `SKILL` modes:
- Compare fixed analytic baseline (`ALGO`) vs adaptive NPU-assisted control (`EdgeAI`) without hidden cadence bias.

Side assignment:
- `AI/ALGO`: left = EdgeAI, right = ALGO
- `ALGO/AI`: left = ALGO, right = EdgeAI
- `2AI`: both = EdgeAI

Control-path design:
- `ALGO` side uses CPU analytic intercept only
- `EdgeAI` side uses model predictions with analytic blend/fallback
- Mixed modes can run EdgeAI target refresh at ALGO-like cadence for fairer side-to-side timing
- Disagreement gate reduces model weight when divergence vs analytic prediction is high

Learning scope:
- Runtime adapts per-side control parameters (`speed_scale`, `noise_scale`, `lead_scale`)
- Model weights remain fixed at runtime (no on-device retraining)
- With `PERSIST ON`, learned side profiles restore independently from flash

## How It Works in This Project

- Build a feature vector from current game state in AI logic
- Run embedded model path through platform NPU/TFLM integration
- Decode output to `(y_hit, z_hit, t_hit)` style signals
- Blend with analytic physics intercept predictor
- Apply confidence/disagreement gate
- Fall back to CPU analytic path when NPU path is unavailable or fails

Important:
- Runtime learning adapts side control parameters over time; it does not retrain model weights.

## Detailed AI Runtime Flow

Per side:
- Compute paddle target `(target_y, target_z)` at bounded update cadence
- Move paddle with frame-to-frame speed limits

In mixed modes:
- `ALGO` side: analytic intercept only
- `EdgeAI` side: model + analytic blend/fallback

Confidence gating:
- Compare model output vs analytic intercept `(y, z, t)`
- If disagreement is high, reduce model contribution toward zero

Online tactical learning:
- Per-side adaptive fields: `speed_scale`, `noise_scale`, `lead_scale`
- Tactical style state and reward-updated style selection from hit/miss outcomes
- Maturity gating keeps early rallies stable, increasing adaptation influence later

Persistence behavior:
- Startup default `PERSIST OFF` (no profile load unless enabled in UI)
- `PERSIST ON`: left/right profiles are stored/restored independently from flash
- Runtime safeguards include decay toward defaults and rollback behavior after sustained regression

## Features

- 3D-look arena with depth cues, wall shading, segmented score digits
- Startup banner: centered two-line `SMART` / `PONG`
- `0P / 1P / 2P` modes
- Ball-speed-linked color behavior
- `0P` accelerometer perturbation of ball trajectory for live outcome nudging
- New-game countdown (`3`, `2`, `1`) on game start
- Fixed-timestep simulation
- Real-time AI/NPU telemetry overlay
- `SKILL` side-selectable adaptive AI vs baseline ALGO (`2AI`, `AI/ALGO`, `ALGO/AI`)
- Optional `SPEED++` progression
- `NEW GAME` clears both score and `WINS`
- AI-driven serve variation
- Event audio path (speaker): wall `226 Hz`, paddle `459 Hz`, point `490 Hz`, plus win tune
- Settings row for volume: `VOL DN  xxx  UP`
- Volume default on boot: `60`
- CapSense volume path implemented in input HAL (I2C status frame + GPIO fallback)
- Competitive AI tuning: higher lead/chase behavior with reduced EdgeAI noise
- Easy-ball lock: close-range returns force analytic intercept (reduced style/noise misses)

## Build and Flash (Infineon E8 / ModusToolbox)

From this `firmware/` folder:

```bash
export CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7
export CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc
export CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools

make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```

Optional helper scripts:

```bash
./tools/build.sh
./tools/flash.sh
./tools/build_and_flash.sh
```

## Project Layout

- `proj_cm55/`: game logic, rendering, platform HAL, AI/NPU integration
- `docs/`: hardware/build/restore/status docs
- `tools/`: build/flash/helper scripts
- `failsafe/` (where present in companion repo paths): pinned restore binaries

## License and Trademarks

This repository includes upstream and vendor components with their own licenses and notices. Review each component directory for specific terms.
