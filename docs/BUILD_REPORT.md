# Build Report

> Detailed evidence report: `docs/BUILD_FLASH_VERIFICATION.md`

## Branch
- `embedded-bringup-smoke-test`

## Commit
- `11f430ded92c0127f72c52ff7d69dbdceb75ff42`

## Target
- `APP_KIT_PSE84_EVAL_EPC2`

## Toolchain
- `GCC_ARM`
- ModusToolbox tools path: `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7`
- GCC ARM path: `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc`

## Build Command
```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 \
  DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Flash Command
```bash
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP TARGET=APP_KIT_PSE84_EVAL_EPC2 \
  DEFINES+=APP_INSULIN_PUMP_MODE=1 DEFINES+=APP_SMART_PONG_MODE=0 DEFINES+=APP_APS_SMOKE_TEST=1
```

## Result
- Embedded build: success
- Flash/program: success
- UART boot banner: success
- One-shot APS probe: success
- Observed on UART: exactly one `APS probe:` line per boot, with the LCD remaining alive
- Portable APS modules are now linked into the CM55 image, and the boot-time probe verified the predictor/controller/safety path once
- For the current verification pass, the more complete build/flash evidence is captured in `docs/BUILD_FLASH_VERIFICATION.md`

## Artifact Paths
- `build/app_combined.hex`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.elf`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.hex`
- `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.bin`
- `proj_cm33_s/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_s.elf`
- `proj_cm33_ns/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm33_ns.elf`

## Size Report
- CM55 DTCM_INTERNAL: `207,316 / 262,144`
- CM55 ITCM_INTERNAL: `225,880 / 262,144`
- RRAM: `159,744 / 524,288`
- SOCMEM_RAM: `4,446,208 / 5,242,880`
- SRAM: `399,882 / 1,048,576`
- m55 NVM region: `224,296 / 3,932,160`
- `build/app_combined.hex`: `5,158,776` bytes

## Warnings
- `ClampF32` unused warning in `cgm_model_runtime.c`
- `touchpad_read` unused variable warnings in `lv_port_indev.c`
- RWX load-segment linker warning for `proj_cm33_s.elf`
- `dashboard_timer_cb` is now restored in smoke mode so the GUI/timers remain active while APS prints as a sidecar

## Flash Main
- Flashing merged `main` commit `39f6361` completed successfully.
- The flashed image included Candidate V1 and did not enable the APS probe or smoke path by default.
- UART showed the boot banner and no APS probe line.
- Physical LCD verification was not directly captured from the terminal session.

## Root Cause Hypothesis
- The earlier active smoke loop likely stalled the panel because APS work was driven from the GUI/timer path with repeated `push_sample()` updates and UART prints, effectively turning smoke mode into a competing UI workload.
- The LCD-safe fix restores the baseline GUI timer path and keeps APS output in a lightweight UART-only sidecar so the panel never depends on APS work to refresh.
- A second likely contributor was the CM55 loop sleeping too long when `lv_timer_handler()` had no near-term work scheduled; capping the loop delay to a small value keeps the GUI and touch service alive even when APS mode is quiet.
- The sidecar also needed a pacing fallback because the timer-driven GUI loop alone was not guaranteeing repeated APS service on this port.

## UART Output
```text
APS Research Platform
Mode: Embedded smoke test
Predictor: present
Controller: present
Safety: present
Build: Jun 11 2026 21:43:59
```

## Known Gaps
- Timing instrumentation still reports `timing not available`.
- The GUI timer and APS sidecar now share the display safely; the next validation is to confirm the final smoke step/completion banner on a longer UART watch.
- The boot banner still shows the older build timestamp string in the UART watch window; that stamp should be refreshed or explained in a follow-up.
- No host regression was needed because this pass only changed embedded firmware smoke wiring.

## Restore-Point Recheck
- The CM55 application sources were rolled back to `failsafe-e8-insulin-pump` for LCD revalidation.
- The restore-point rebuild completed successfully and was programmed to the board.
- UART was quiet during the short follow-up window, which is expected for the baseline image; the remaining confirmation is a direct LCD visual check on the board.

## Recovery Closure
- Physical LCD confirmed recovered after clean rebuild and default probe-free flash.
- Default image must not print `APS probe:`.
- A clean rebuild is required after toggling `APP_APS_EMBEDDED_PROBE`.
- UART boot alone is not enough to claim LCD success.

## LVGL Move Accuracy Label Out of Chart Test
- Branch: `move-accuracy-label-out-of-chart`
- Baseline commit: `087abe1`
- Change: `gDashboard.prediction_accuracy_label` parent moved from `chart` to `screen`; same text update path retained
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: pending

## Confirmed LVGL Accuracy Label Reparent Pass
- Physical LCD result: live / GUI visible and accuracy label appears
- Existing `lv_label_set_text()` path remained unchanged
- Fix candidate: keep frequently updated status labels out of the chart subtree

## APS Screen Label Readout V1
- Branch: `aps-screen-label-readout-v1`
- Change: compile-gated `header_buffer` placeholder `APS 110` on existing screen-level `prediction_accuracy_label`
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: blank / dead / frozen
- Recovery image result: exact known-good recovery image restored LCD live / GUI visible
- Conclusion: screen-level label reparenting is not sufficient by itself; arbitrary LVGL text mutation on this path is still unsafe under the current render stack

## APS Placeholder Display V1
- Baseline truth point underneath placeholder work: `9fb4e7c`
- Change: added one static white `Artificial Pancreas` header label plus one static green placeholder readout label as screen children
- Final green placeholder position: `x=30`, `y=311`
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible
- Runtime scope: display-only placeholder, no APS runtime math

## APS Placeholder Promotion
- Approved restore-point commit: `2c8c6f1`
- The placeholder display is now the active golden/failsafe LCD restore truth for the insulin-pump layout.
- Matching `.hex` and `proj_cm55.elf` artifacts were captured in `../failsafe/` for direct recovery use.

## BG-Only Placeholder Step
- Branch: `aps-bg-placeholder-step`
- Change: only the visible `BG` placeholder value changed from `110` to `112`
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible and `BG` shows `112`
- Outcome: first safe single-field placeholder replacement confirmed on hardware

## Placeholder Value Refresh
- Branch: `aps-bg-placeholder-step`
- Change: display-only placeholder block now reads `BG 118`, `IOB 0.9`, `COB 22`, `ACT REST`, `INS 0.7`, `SAFE OK`
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible with the refreshed placeholder values
- Runtime scope: display-only placeholder, no APS runtime math

## Live Terminal BG Test
- Branch: `aps-terminal-live-bg-test`
- Change: colon-formatted terminal labels with `SAFETY`, and `BG` driven live from `current_mgdl`
- Other terminal fields remain placeholder values for this step
- Build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible and CRT block is updating

## Live Terminal Multi-Field Wiring Confirmed
- Branch: `aps-terminal-live-bg-test`
- Change: `IOB:`, `COB:`, `INS:`, and `SAFETY:` are now wired to live runtime sources; `ACT:` remains static by design because no activity sensor/runtime is wired yet
- Host validation: `make -f host.mk test` and `make -f host.mk regression` passed
- OpenOCD pre-reset retry: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`) after one transient `KitProg3` resource-busy attempt
- Physical LCD result: live data is visible on the CRT block
- Scope note: the physically confirmed board state is the source of truth; a redundant local rebuild was intentionally interrupted after confirmation to avoid disturbing the known-good screen

## APS Demo State Wiring
- Branch: `aps-demo-state-wiring`
- Restore base: `efc1b0c`
- Change: CRT values are now sourced through `aps_demo_state_t` and the new `ApsDemoState_Init() / ApsDemoState_Step()` APIs instead of ad-hoc terminal placeholder/runtime formatting.
- Host validation: `make -f host.mk test` and `make -f host.mk regression` passed
- Embedded build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible with CRT values coming from the APS demo-state pipeline
- Display note: the large center `mg/dL` card still follows the original dashboard/replay path, so it can differ from the CRT `BG:` line by design at this milestone
- Safety text note: `APS_SAFETY_REASON_RAPID_FALL` is being relabeled from `FALL` to `RAPID` for clearer operator wording
- Scope note: no new LVGL objects or CRT layout changes were introduced in this milestone

## Replay Rate Graph Label Candidate
- Branch: `aps-demo-state-wiring`
- Golden/failsafe baseline preserved at `e1ba009` before this change
- Change:
  - replay timer interval set to `5000 ms`
  - static `Replay:` / `60X` graph annotation added as a screen-level label
- Build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: not run yet
- Physical LCD result: pending
- Scope note: this candidate changes display text placement and replay cadence only; APS runtime sourcing is unchanged

## Virtual Patient V1
- Branch: `aps-demo-state-wiring`
- Change:
  - added `VirtualPatientV1` looping patient source
  - replaced the finite APS demo input sequence
  - fed controller-delivered insulin back into the next patient step
- Host validation: `make -f host.mk test` passed
- Regression validation: `make -f host.mk regression` passed
- Embedded build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible and CRT values continue changing beyond 60 seconds
- Runtime note: CRT is now continuously driven by real APS demo-state plumbing plus a deterministic looping virtual patient

## Virtual Human + Short CRT Labels
- Branch: `aps-demo-state-wiring`
- Change:
  - added static blue `Virtual Human` label above the glucose card
  - shortened CRT labels to `INS ONBD`, `CARBS`, and `INS RATE`
  - nudged `Virtual Human` upward by roughly one-half text height in the final accepted layout
- Embedded build: passed
- Program: passed
- OpenOCD pre/post reset-run: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible with the accepted `Virtual Human` + shortened CRT label layout
- Scope note: APS runtime plumbing is unchanged; this is a display wording/layout refinement only

## Unified APS Glucose + Freeze Fix
- Branch: `aps-glucose-unified-display`
- Change:
  - unified the visible glucose paths so CRT, center `MG/DL`, and chart glucose all read from the APS demo-state / virtual-patient source
  - fixed dashboard startup seeding so the chart no longer advances deep into the APS sequence before the timer starts
  - added a timer-step UART breadcrumb if `ApsDemoState_Step()` fails
- Host validation:
  - host assertions needed to be updated for the current CRT wording (`GLUCOSE:` and `INS RATE:`)
- Embedded build: current source builds cleanly with `CONFIG_DISPLAY=W4P3INCH_DISP`
- Physical LCD result: live / GUI visible and the earlier five-point freeze window no longer matches observed behavior on the board
- Scope note:
  - Predictor V2, controller, and safety are running in the live APS pipeline
  - this is still a research/demo integration state rather than a clinically validated system
