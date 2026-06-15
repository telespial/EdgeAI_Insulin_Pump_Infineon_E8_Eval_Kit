# VirtualPatientV2 Scenario Mode LCD Failure

## Status

Open issue. Not root-caused yet.

This is the current blocking embedded-runtime bug for `VirtualPatientV2` scenario-mode work.

## Short Version

The current LCD-good image is the delayed-start runtime checkpoint promoted at commit `b06cdd0`.

That image:

- boots successfully
- keeps the LCD alive
- preserves the current GUI / CRT layout
- uses the deferred APS/V2 startup path with a `250 ms` one-shot arm delay

However, scenario-mode images built with:

```text
APP_VP_SCENARIO_MODE=1..6
```

can still:

- build successfully
- program successfully
- pass OpenOCD pre/post reset-run checks
- report `PSE846GPS2DBZC4A`
- report `CYBOOT_SUCCESS`

and yet still produce:

```text
LCD blank / dead / frozen
```

on physical hardware.

## What Is Confirmed

### Known-good current golden checkpoint

- Branch: `vp2-background-on-v1-visible`
- Commit: `b06cdd0`
- Hardware result:
  - LCD live
  - GUI visible

### Known-good older recovery checkpoints

- `deeb67c`
- `6023494`
- `571fb89`

These remain useful for rollback and comparison.

### Known-bad mode experiments

The following mode-enabled images were physically bad even though build/program/debugger checks looked healthy:

#### Mode 1

```text
APP_VP_SCENARIO_MODE=1
NORMAL
```

Observed result:

- build passed
- program passed
- OpenOCD pre-reset healthy
- OpenOCD post-reset healthy
- physical LCD blank / dead / frozen

#### Mode 2

```text
APP_VP_SCENARIO_MODE=2
BREAKFAST
```

Observed result:

- build passed
- program passed
- OpenOCD pre-reset healthy
- OpenOCD post-reset healthy
- physical LCD blank / dead / frozen

#### Mode 6

```text
APP_VP_SCENARIO_MODE=6
RAPID_FALL
```

Observed result:

- build passed
- program passed
- OpenOCD pre-reset healthy
- OpenOCD post-reset healthy
- physical LCD blank / dead / frozen

## Current Working Hypothesis

This does **not** currently look like:

- a basic toolchain failure
- a programmer/OpenOCD failure
- a stale-artifact-only issue
- a simple UART issue

The failure currently appears more consistent with one of:

1. scenario-mode runtime initialization interacting badly with first-render timing
2. display bring-up being perturbed by when/where scenario-selected V2 work starts
3. a runtime-state path difference between default `BREAKFAST` behavior and explicit compile-time mode selection
4. a code/data-layout side effect introduced by the mode-selection path
5. a hidden dependency on the default no-flag startup sequence

## Important Negative Findings

These are also important because they narrow the search:

- The failed `THINKING` banner experiment was a separate LCD regression and has already been backed out.
- The new current golden image is LCD-safe after shortening the deferred startup arm delay to `250 ms`.
- The problem is **not** explained by OpenOCD health; healthy debugger output does not guarantee a good LCD image.
- The problem is **not** yet proven to be a `VirtualPatientV2` math bug by itself.

## Current Safe Ground

Use this when continuing work:

- Golden restore point: `b06cdd0`
- Keep the older failsafe restore point unchanged
- Do not start from a mode-enabled image
- Do not assume host-pass or build-pass means LCD-safe

## Rules While Investigating

1. Physical LCD result is the source of truth.
2. Use the LCD-safe OpenOCD pre-reset / program / post-reset flow every time.
3. Keep display experiments separate from scenario-mode experiments.
4. Keep one-change-per-image when isolating the mode bug.
5. Do not erase flash first.
6. Preserve rollback to `b06cdd0`.

## Recommended Next Isolation Steps

Only one at a time:

1. compare default no-flag runtime vs explicit `APP_VP_SCENARIO_MODE=2`
2. defer mode selection until after first stable render
3. keep mode compiled in but do not activate it until after delayed startup arm
4. move selected-scenario application out of early init into the existing safe post-render cadence
5. inspect whether compile-time mode path changes static/global layout in a way that affects runtime bring-up

## Related Docs

- `docs/START_HERE.md`
- `docs/PROJECT_STATE.md`
- `docs/BUILD_FLASH_VERIFICATION.md`
- `docs/BUILD_REPORT.md`
- `docs/RESTORE_POINTS.md`
- `docs/VIRTUAL_PATIENT_V2.md`
- `docs/VIRTUAL_PATIENT_V2_BREAKFAST_FREEZE.md`
- `docs/LVGL_DEBUG_PRINTF_FAILURE.md`
