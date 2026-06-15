# VirtualPatientV2 Breakfast Freeze Investigation

## Summary

Host-first investigation showed the breakfast event itself is healthy.

The confirmed software problem was a split runtime path:

- `ApsDemoState_Step()` used `VirtualPatientV1`
- the UI/CRT path separately stepped and displayed `VirtualPatientV2Background`

That meant visible APS/controller/safety state and visible Virtual Human / CRT state were not using one shared patient source.

## Confirmed Answers

1. `ApsDemoState_Step()` was still using `VirtualPatientV1_Step()` before this fix.
2. `VirtualPatientV2Background_Step()` was also called from the UI path.
3. Yes — CRT fields were being bypassed to V2 background state while controller/safety came from V1.
4. Yes — `VP2_BREAKFAST_STEP = 4`.
5. Yes — with one 5-minute virtual step per 5-second display update, that lands at roughly 20 seconds on hardware.
6. Yes — `ApsDemoState_FormatTerminal()` still formatted ACT / INS / SAFETY, but `update_aps_terminal_label()` bypassed it whenever background V2 state was available.

## Host Breakfast Trace

The breakfast trace ran through steps `0..12` and stayed healthy:

```text
step=0 cycle=0 dbg=250 bg=110 target=108 cob=0.00 iob=0.00 basal=0.80 meal=0
step=1 cycle=1 dbg=250 bg=109 target=107 cob=0.00 iob=0.07 basal=0.80 meal=0
step=2 cycle=2 dbg=250 bg=108 target=105 cob=0.00 iob=0.13 basal=0.80 meal=0
step=3 cycle=3 dbg=250 bg=107 target=104 cob=0.00 iob=0.20 basal=0.80 meal=0
step=4 cycle=4 dbg=250 bg=116 target=168 cob=60.00 iob=0.26 basal=0.80 meal=1
step=5 cycle=5 dbg=250 bg=128 target=156 cob=58.75 iob=0.32 basal=0.80 meal=0
step=6 cycle=6 dbg=250 bg=140 target=153 cob=57.50 iob=0.38 basal=0.80 meal=0
```

This confirms:

- breakfast step `4` returns true
- `meal_event` becomes true
- `COB` rises
- `BG` remains bounded
- `debug_code` reaches normal completion (`250+`)
- later steps continue

## Fix Applied

### Unified source of truth

`ApsDemoState` now uses `VirtualPatientV2` directly:

```text
VirtualPatientV2
↓
ApsDemoState
↓
Predictor / Controller / Safety
↓
CRT
```

### UI cleanup

The dashboard app no longer:

- substitutes center glucose from `VirtualPatientV2Background`
- bypasses `ApsDemoState_FormatTerminal()` with a separate V2 terminal formatter
- steps background V2 from `push_sample()` just for visible CRT output

## Validation

- `make -f host.mk test` passed
- `make -f host.mk regression` passed
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed

## Current Conclusion

- The V1/V2 split existed and was real
- The first breakfast event in `VirtualPatientV2` is healthy in host logic
- The codebase now expresses one intended APS source of truth in `ApsDemoState`
- No new flash was performed in this step yet

## Next Safe Step

Flash the unified image with the standard LCD-safe flow, then physically verify:

- LCD live
- CRT updates past breakfast
- values continue beyond 60 seconds
- no freeze at the first meal event
