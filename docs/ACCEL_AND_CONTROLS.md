# Accelerometer and Controls

## Current Output Mapping

In `platform/input_hal.c` the exported accel outputs are:
- `accel_ax = sensor X`
- `accel_ay = -sensor Y`

This reflects latest hardware tuning done on-device.

## Game Nudge Usage (P0)

In `game/game.c` the P0 ball vertical nudge uses mapped `Y` and current sign inversion:
- `g->ball.vy -= ay * k * dt;`

## Why This Matters

The mapping/sign were tuned to ensure:
- Left/right roll does not unintentionally drive the wrong vertical direction
- Practical playability on the mounted E8 board orientation

## If Controls Feel Reversed

Adjust one sign only:
- Flip `accel_ay` sign in `platform/input_hal.c` **or**
- Flip P0 nudge sign in `game/game.c`

Do not flip both at once unless intentionally remapping behavior.
