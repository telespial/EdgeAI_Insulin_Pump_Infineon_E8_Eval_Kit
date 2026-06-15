## Update 2026-06-15 — deferred APS/V2 startup after first stable render

Status:
- `EMBEDDED BUILT`
- `FLASHED`
- `RUNNING ON HARDWARE`

Evidence:
- branch: `vp2-background-on-v1-visible`
- source change:
  - removed immediate `ApsDemoState_Init()` call from `edgeai_insulin_pump_app_start()`
  - replaced live startup seeding with a static placeholder dashboard state
  - added a one-tick arming delay before `dashboard_timer_cb()` starts APS stepping
- `make clean TOOLCHAIN=GCC_ARM` passed
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
- OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- physical LCD result: live / GUI visible / `Virtual Human` values delayed, then running

Interpretation:
- the image now renders one stable placeholder frame before any APS/V2 stepping occurs
- this isolates `VirtualPatientV2` work from first-frame LVGL/display bring-up
- hardware survived the change and confirms the early-start interaction was part of the freeze risk
- current startup delay is longer than desired because the visible data path waits through the placeholder-first arming behavior

## Update 2026-06-15 — VirtualPatientV2 debug-code image confirmed on hardware

Status:
- `HOST TESTED`
- `EMBEDDED BUILT`
- `FLASHED`
- `RUNNING ON HARDWARE`

Evidence:
- branch: `vp2-background-on-v1-visible`
- source HEAD flashed: `83b5d7f`
- `rm -rf host_build && make -f host.mk test` passed
- `rm -rf host_build && make -f host.mk regression` passed
- `make clean TOOLCHAIN=GCC_ARM && make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
- OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- physical LCD result: live / GUI visible / center `MG/DL` shows steady `299`

Interpretation:
- `299` is the final `VirtualPatientV2_Step()` success code, which means the background `V2` runtime is reaching the end of its step function on hardware
- this image did not reproduce the earlier visible freeze during the user check
- visible LCD / CRT / chart remain on the proven `V1` APS display path while the center glucose number is temporarily repurposed as the `V2` debug indicator

# Build/Flash Verification Evidence Report

## Update 2026-06-15 — Background VirtualPatientV2 on V1-visible runtime

Status:
- `HOST TESTED`
- `EMBEDDED BUILT`
- not flashed in this milestone

Evidence:
- `rm -rf host_build && make -f host.mk test` passed
- `rm -rf host_build && make -f host.mk regression` passed
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed

Scope:
- visible LCD / CRT / chart remain driven by `VirtualPatientV1` through `ApsDemoState_Step()`
- `VirtualPatientV2` is initialized at app start and stepped in the background only
- `VirtualPatientV2` was made self-contained so it no longer shares singleton `IobEngine` / `CobEngine` state with the visible V1 path
- the new host test proves visible V1 states still match baseline when background V2 is running in parallel
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
- OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- physical LCD confirmation pending
- until that confirmation arrives, the last separately verified restore point remains `c87802a`

## Update 2026-06-14 — Virtual Patient V2 Build Validation

Status:
- `HOST TESTED`
- `EMBEDDED BUILT`
- not flashed in this milestone

Evidence:
- `make -f host.mk test` passed
- `make -f host.mk regression` passed
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
- artifacts present:
  - `build/app_combined.hex`
  - `proj_cm55/build/APP_KIT_PSE84_EVAL_EPC2/Debug/proj_cm55.elf`

Scope:
- `VirtualPatientV2` replaces `VirtualPatientV1` as the active APS demo-state physiology source
- `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
- OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- physical LCD confirmation pending
- physical LCD truth remains tied to the last separately verified restore point

## Scope
- Verify what is built, flashed, running on E84 hardware, or host-only.
- No APS logic, GUI/LCD behavior, predictor behavior, or controller/safety behavior was changed for this report.
- The portable APS modules are now embedded linked in `proj_cm55`, and the one-shot boot probe runtime-verified the predictor/controller/safety path on hardware.
- Current repo state at capture time:
  - Branch: `embedded-bringup-smoke-test`
  - Commit: `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
- Recovery closure:
- Physical LCD confirmed recovered after clean rebuild and default probe-free flash.
- Default image must not print `APS probe:`.
- A clean rebuild is required after toggling `APP_APS_EMBEDDED_PROBE`.
- UART boot alone is not enough to claim LCD success.

## Update 2026-06-14 — APS glucose unification candidate
- Source branch: `aps-glucose-unified-display`
- Source base / restore truth point: `aed98c2`
- Purpose: remove replay-array display wiring so the visible glucose paths now follow APS demo / virtual-patient state continuously.
- Visible paths rewired in `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`:
  - CRT `GLUCOSE:` line
  - center `MG/DL` glucose value
  - chart glucose series
- Removed from the visible dashboard path:
  - `cgm_replay_subject001.h`
  - `CgmModel_*`
  - local replay-array/model helper functions
- Build result:
  - `make clean TOOLCHAIN=GCC_ARM` ✅
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` ✅
- Flash result:
  - not yet run for this candidate
- Physical LCD result:
  - pending
- Status:
  - build-verified candidate only; not yet a new hardware-confirmed golden point

## Update 2026-06-12 17:00 PDT
- Flashed merged `main` commit `39f6361` with the Candidate V1 tables included.
- Pre-reset OpenOCD again detected `PSE846GPS2DBZC4A` and reported `CYBOOT_SUCCESS`.
- Post-reset OpenOCD completed a clean `reset run` after programming.
- UART after the flash showed the legacy LVGL demo boot banner and did not show any `APS probe:` line or smoke sequence output.
- The panel was not directly visible in this terminal session, so the run records UART and debugger evidence only.

## Verification Matrix

| System Item | Host Tested | Embedded Built | Flashed | Running on Hardware | Evidence | Notes |
|---|---:|---:|---:|---:|---|---|
| E84 LCD/GUI baseline | no | yes | yes | yes | `make build ...`, `make program ...`, OpenOCD reset-run, UART banner | GUI image stayed alive after flash/reset; no fresh photo was taken in this session. |
| Flash recovery/reset workflow | no | no | no | yes | OpenOCD `init; flash banks; reset run; sleep 2000; shutdown` | Target reported `PSE846GPS2DBZC4A` and `Boot Status : CYBOOT_SUCCESS`. |
| Host APS simulation | yes | no | no | no | `make -f host.mk test`, `make -f host.mk regression` | Host-only APS framework and simulator. |
| Regression fixtures | yes | no | no | no | fixture/gold output from `make -f host.mk regression` | Both fixture and gold matrices passed. |
| IOB engine | yes | yes | yes | no | host test output (`PASS: iob ...`), CM55 link map | Embedded linked, not runtime called. |
| COB engine | yes | yes | yes | no | host test output (`PASS: cob ...`), CM55 link map | Embedded linked, not runtime called. |
| Activity engine | yes | yes | yes | no | host test output (`PASS: activity ...`), CM55 link map | Embedded linked, not runtime called. |
| Controller/safety framework | yes | yes | yes | yes | host test output (`PASS: controller ...`, `PASS: safety ...`), CM55 probe output | Embedded linked, one-shot runtime verified. |
| Audit logging | yes | yes | yes | no | fixture/audit CSV outputs, CM55 link map | Embedded linked, not runtime called. |
| Embedded APS one-shot runtime probe | yes | yes | yes | yes | `make build ...`, `make program ...`, OpenOCD reset-run, UART `APS probe:` line | One-shot probe ran on boot and returned to the existing GUI/LVGL flow; later recovery showed why boot-time APS work must stay off the default image. |

## Evidence Notes

### LCD / GUI baseline
- Source files involved:
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/pump_background_image_rgb565.c`
  - `proj_cm55/app/EdgeAI_Smart_Pong_demo_Infineon_E8_Eval_Kit/smart_pong_app.c`
  - `proj_cm55/lv_port_disp.c`
  - `proj_cm55/lv_port_indev.c`
- Build target:
  - `proj_cm55`
- Commands used:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
  - OpenOCD reset-run: `init; flash banks; reset run; sleep 2000; shutdown`
- UART evidence:
  - `****************** PSOC Edge MCU: Graphics LVGL Demo ******************`
- LCD evidence:
  - Display remained alive after flash/reset; no new photo was taken in this session.
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### Flash recovery / reset workflow
- Source files involved:
  - `docs/MUST_READ_LCD_RECOVERY.md`
  - `docs/OPS_RUNBOOK.md`
- Build target:
  - N/A
- Command used:
  - OpenOCD reset-run: `init; flash banks; reset run; sleep 2000; shutdown`
- Flash command used:
  - N/A
- UART / LCD evidence:
  - OpenOCD reported `Detected Device: PSE846GPS2DBZC4A`
  - OpenOCD reported `Boot Status : CYBOOT_SUCCESS`
  - Flash banks visible: `cat1d.cm33.main_ns` and `cat1d.cm33.smif1_ns`
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### Host APS simulation
- Source files involved:
  - `firmware/src/aps_log.c`
  - `firmware/src/activity_engine.c`
  - `firmware/src/cob_engine.c`
  - `firmware/src/aps_physiology.c`
  - `firmware/src/controller_openaps.c`
  - `firmware/src/iob_engine.c`
  - `firmware/src/metrics.c`
  - `firmware/src/predictor_v2.c`
  - `firmware/src/predictor_v2_generated.c`
  - `firmware/src/safety_supervisor.c`
  - `sim/audit_trace.c`
  - `sim/replay_loader.c`
  - `sim/scenario_runner.c`
  - `sim/simulation_runner.c`
- Build target:
  - `e84_aps_framework`, `e84_aps_sim`, `e84_aps_host_tests`
- Commands used:
  - `make -f host.mk test`
  - `make -f host.mk regression`
- Test output:
  - `All host foundation tests passed`
  - `Fixture metric validation passed`
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### Regression fixtures
- Source files involved:
  - `tests/fixture_expectations.json`
  - `tests/gold_fixture_expectations.json`
  - `scripts/run_fixture_matrix.sh`
  - `scripts/run_gold_matrix.sh`
- Build target:
  - Host simulator / host tests
- Commands used:
  - `make -f host.mk regression`
- Regression output:
  - `fixture ok: stable`
  - `fixture ok: meal_rise`
  - `fixture ok: falling_bolus`
  - `fixture ok: bad_sqi`
  - `fixture ok: stale_cgm`
  - `gold fixture ok: gold_meal_rise`
  - `gold fixture ok: gold_predicted_low`
  - `gold fixture ok: gold_stable_flat`
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### IOB / COB / Activity / Controller / Safety / Audit
- Source files involved:
  - `firmware/src/iob_engine.c`
  - `firmware/src/cob_engine.c`
  - `firmware/src/activity_engine.c`
  - `firmware/src/controller_openaps.c`
  - `firmware/src/safety_supervisor.c`
  - `firmware/src/metrics.c`
  - `firmware/src/aps_physiology.c`
  - `firmware/src/predictor_v2.c`
  - `firmware/src/predictor_v2_generated.c`
  - `firmware/src/aps_log.c`
  - `sim/audit_trace.c`
- Build target:
  - Host library `e84_aps_framework`
- Embedded build target:
  - `proj_cm55`
- Commands used:
  - `make -f host.mk test`
  - `make -f host.mk regression`
- Embedded build / flash commands:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Evidence:
  - Host tests emitted `PASS` lines for IOB, COB, activity, controller decisions, safety gates, and audit schema checks.
  - CM55 build linked the portable APS sources without adding a runtime APS call site.
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### Embedded APS link
- Source files involved:
  - `firmware/src/iob_engine.c`
  - `firmware/src/cob_engine.c`
  - `firmware/src/activity_engine.c`
  - `firmware/src/aps_physiology.c`
  - `firmware/src/predictor_v2.c`
  - `firmware/src/predictor_v2_generated.c`
  - `firmware/src/controller_openaps.c`
  - `firmware/src/safety_supervisor.c`
  - `firmware/src/metrics.c`
  - `firmware/src/aps_log.c`
- Build target:
  - `proj_cm55`
- Commands used:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- Flash command used:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- UART evidence:
  - `****************** PSOC Edge MCU: Graphics LVGL Demo ******************`
- LCD / GUI evidence:
  - LCD stayed alive after flash and OpenOCD reset-run; GUI remained on the baseline demo.
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`
- Build size:
  - `build/app_combined.hex`: `5,206,276` bytes
  - `m55_nvm`: `250,496 / 3,932,160`
  - No meaningful delta was observed versus the prior captured build summary in the command log.

### Embedded APS one-shot runtime probe
- Source files involved:
  - `proj_cm55/main.c`
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`
  - `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.h`
  - `firmware/src/predictor_v2.c`
  - `firmware/src/controller_openaps.c`
  - `firmware/src/safety_supervisor.c`
- Build target:
  - `proj_cm55`
- Commands used:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_EMBEDDED_PROBE=1 -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_EMBEDDED_PROBE=1`
  - OpenOCD reset-run: `init; flash banks; reset run; sleep 2500; shutdown`
- Runtime call path:
  - `main()` prints the boot banner, then calls `ApsEmbeddedProbe_RunOnce()`, which calls `PredictorV2_Reset()`, `PredictorV2_SetEnabled(true)`, `OpenApsController_Reset()`, `PredictorV2_Update(...)`, `OpenApsController_DetermineBasal(...)`, and `SafetySupervisor_Apply(...)` once.
- UART evidence:
  - `APS probe: BG=120 P15=125 P30=126 P60=127 ACTION=NO_CHANGE SAFETY=0x00000000 OK=111`
- LCD / GUI evidence:
  - LCD stayed alive after flash and reset-run; GUI remained on the baseline demo.
- Probe execution:
  - Single-reset capture shows one `APS probe:` line; the earlier program step also produced a normal boot, which is why the combined capture showed one line per boot.
- Commit / branch:
  - `11f430ded92c0127f72c52ff7d69dbdceb75ff42`
  - `embedded-bringup-smoke-test`

### LCD / probe recovery closure
- Bad commit:
  - `3fdd51a`
- Last LCD-good commit before probe:
  - `cbe7b58`
- Recovery branch:
  - `recover-lcd-after-aps-probe`
- Symptom:
  - LCD blank after probe-enabled image and stale artifact reuse during the first baseline flash attempt.
- Cause hypothesis:
  - Stale probe-enabled build artifacts and the risk of running APS work too early from `main()`.
- Recovery action:
  - Tightened `APP_APS_EMBEDDED_PROBE == 1` gating.
  - Ran `make clean` with the documented ModusToolbox environment.
  - Rebuilt the default image from scratch.
  - Reflashed and ran OpenOCD `reset run`.
  - Physically confirmed the LCD recovered after the clean rebuild and probe-free flash.
- New rule:
  - No APS runtime call from boot or `main()` until a LCD-safe delayed or sidecar execution model is designed.
  - Clean rebuild is required after toggling `APP_APS_EMBEDDED_PROBE`.
  - Physical LCD verification is required before claiming success.

## Firmware Map

### Requested APS modules
| File | Classification | Evidence |
|---|---|---|
| `firmware/src/iob_engine.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; host tests compiled and passed. |
| `firmware/src/cob_engine.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; host tests compiled and passed. |
| `firmware/src/activity_engine.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; host tests compiled and passed. |
| `firmware/src/aps_physiology.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; no runtime APS call site added. |
| `firmware/src/predictor_v2.c` | embedded linked, one-shot runtime verified | Present in host build and now linked into `proj_cm55/Makefile`; exercised once by the boot-time probe. |
| `firmware/src/predictor_v2_generated.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; generated tables were not altered. |
| `firmware/src/controller_openaps.c` | embedded linked, one-shot runtime verified | Present in host build and now linked into `proj_cm55/Makefile`; exercised once by the boot-time probe. |
| `firmware/src/safety_supervisor.c` | embedded linked, one-shot runtime verified | Present in host build and now linked into `proj_cm55/Makefile`; exercised once by the boot-time probe. |
| `firmware/src/metrics.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; metrics behavior was not changed. |
| `firmware/src/aps_log.c` | embedded linked, not runtime called | Present in host build and now linked into `proj_cm55/Makefile`; logging remains a no-op on target. |
| `sim/audit_trace.c` | host only | Present in `CMakeLists.txt`; audit schema checks passed on host; not referenced from embedded subprojects. |
| `sim/simulation_runner.c` | host only | Present in `CMakeLists.txt`; host simulator built and regression ran; not referenced from embedded subprojects. |
| `sim/replay_loader.c` | host only | Present in `CMakeLists.txt`; host replay/regression used it; not referenced from embedded subprojects. |
| `sim/scenario_runner.c` | host only | Present in `CMakeLists.txt`; host replay/regression used it; not referenced from embedded subprojects. |

### Embedded GUI files
| File | Classification | Evidence |
|---|---|---|
| `proj_cm55/app/EdgeAI_Smart_Pong_demo_Infineon_E8_Eval_Kit/smart_pong_app.c` | embedded linked / runtime called | Built in `proj_cm55`; GUI boot banner observed over UART. |
| `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c` | embedded linked / runtime called | Built in `proj_cm55`; flashed and kept alive through reset-run. |
| `proj_cm55/lv_port_disp.c` | embedded linked | Built in `proj_cm55`. |
| `proj_cm55/lv_port_indev.c` | embedded linked | Built in `proj_cm55`. |
| `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/pump_background_image_rgb565.c` | embedded linked | Built in `proj_cm55`. |

## Remaining Gaps
- No fresh LCD photo was captured during this evidence pass.
- Only the one-shot boot probe is runtime verified on hardware; the rest of the APS framework remains link-only.
- The verification evidence is complete for the current board/repo state, but it does not claim broader embedded APS loop behavior on hardware.

## Pending Flash Verification — Move Accuracy Label Out of Chart
- Branch: `move-accuracy-label-out-of-chart`
- Exact source baseline: `087abe1`
- Build command: `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program command: `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD reset-run before program: passed (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD reset-run after program: passed (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: pending

## Confirmed Flash Verification — Move Accuracy Label Out of Chart
- Physical LCD result: live / GUI visible and accuracy label appears
- Build/program/OpenOCD all passed
- Likely root cause: chart-child label invalidation under chart refresh/full-render/VG-Lite

## Failed Flash Verification — APS Screen Label Readout V1
- Branch: `aps-screen-label-readout-v1`
- Build command: `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_SCREEN_LABEL_READOUT=1 -j8`
- Program command: `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_APS_SCREEN_LABEL_READOUT=1`
- OpenOCD reset-run before program: passed (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD reset-run after program: passed (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: blank / dead / frozen

## Confirmed Recovery — APS Screen Label Readout V1 Failure
- Recovery baseline: known-good source restored with no APS text flags
- Recovery build/program/OpenOCD: passed
- Recovery physical result: LCD live / GUI visible
- Conclusion: no APS math and no new LVGL objects were involved; arbitrary LVGL text mutation remains unsafe on this render stack

## Confirmed Flash Verification — APS Placeholder Display V1
- Branch at flash time: `numeric-only-aps-source-test`
- Source baseline under the placeholder: `9fb4e7c`
- Build/program/OpenOCD all passed
- OpenOCD remained healthy before and after programming with `PSE846GPS2DBZC4A` and `CYBOOT_SUCCESS`
- Physical LCD result: live / GUI visible with the white `Artificial Pancreas` header and green placeholder readout block
- Runtime scope: display-only placeholder, no APS runtime math

## Pending Flash Verification — BG-Only Placeholder Experiment
- Branch: `aps-bg-placeholder-step`
- Source baseline: `2c8c6f1` restore point plus docs-only planning commit `472a22a`
- Visible change: `BG   110` → `BG   112`
- All other placeholder fields remain unchanged
- Runtime scope: display-only, no APS runtime math

## Confirmed Flash Verification — BG-Only Placeholder Experiment
- Branch: `aps-bg-placeholder-step`
- Visible change: `BG   110` → `BG   112`
- Build/program/OpenOCD all passed
- OpenOCD remained healthy before and after programming with `PSE846GPS2DBZC4A` and `CYBOOT_SUCCESS`
- Physical LCD result: live / GUI visible and `BG` shows `112`
- Runtime scope: display-only, no APS runtime math

## Pending Flash Verification — IOB-Only Placeholder Experiment
- Branch: `aps-bg-placeholder-step`
- Visible change: `IOB  1.2` → `IOB  1.3`
- All other placeholder fields remain unchanged from the last physically verified state
- Runtime scope: display-only, no APS runtime math

## Pending Flash Verification — Multi-Field Placeholder Shortcut
- Branch: `aps-bg-placeholder-step`
- Visible state before flash:
  - `BG   112`
  - `IOB  1.2`
  - `COB   24`
  - `ACT WALK`
  - `INS  0.9`
  - `SAFE LOW`
- Notes:
  - The earlier `IOB`-only rebuild was interrupted before flash.
  - This multi-field update is a user-directed shortcut.
- Runtime scope: display-only, no APS runtime math

## Pending Flash Verification — Placeholder Value Refresh
- Branch: `aps-bg-placeholder-step`
- Visible state before flash:
  - `BG   118`
  - `IOB  0.9`
  - `COB   22`
  - `ACT REST`
  - `INS  0.7`
  - `SAFE OK`
- Notes:
  - No new image has been built or flashed from this refresh yet.
  - Runtime scope remains display-only with no APS runtime math.

## Confirmed Flash Verification — Placeholder Value Refresh
- Branch: `aps-bg-placeholder-step`
- Build: passed
- Program: passed
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Flashed visible state should now be:
  - `BG   118`
  - `IOB  0.9`
  - `COB   22`
  - `ACT REST`
  - `INS  0.7`
  - `SAFE OK`
- Runtime scope: display-only, no APS runtime math
- Physical LCD result: live / GUI visible with the refreshed placeholder values

## Confirmed Flash Verification — Live Terminal BG Test
- Branch: `aps-terminal-live-bg-test`
- Visible state after flash:
  - `BG:` becomes live from the existing dashboard sample path
  - `IOB: 0.9`
  - `COB: 22`
  - `ACT: REST`
  - `INS: 0.7`
  - `SAFETY: OK`
- Build: passed
- Program: passed
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible and CRT block is updating
- Notes:
  - No new LVGL objects were added.
  - No Predictor V2/controller/safety runtime output is being added to the terminal yet.

## Confirmed Flash Verification — Live Terminal Multi-Field Wiring
- Branch: `aps-terminal-live-bg-test`
- Visible state now confirmed on hardware:
  - `BG:` live from dashboard sample path
  - `IOB:` live from IOB engine
  - `COB:` live from COB engine
  - `ACT:` remains static because no live activity feed is wired yet
  - `INS:` live from the controller request after safety limiting
  - `SAFETY:` live from safety/controller flags
- Host validation: `make -f host.mk test` and `make -f host.mk regression` passed
- OpenOCD: pre-reset retry healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`) after one transient `KitProg3` resource-busy attempt
- Physical LCD result: user confirmed `data is live!`
- Notes:
  - No new LVGL objects were added.
  - Rollback targets remain `471925b` for live `BG:` only and `909af0d` for the fully static placeholder restore point.
  - A redundant follow-up rebuild/program attempt was stopped after physical confirmation so the known-good board state stayed undisturbed.

## Pending Flash Verification — APS Demo State Wiring
- Branch: `aps-demo-state-wiring`
- Expected visible state after flash:
  - `BG:` from `aps_demo_state_t.bg_mgdl`
  - `IOB:` from `aps_demo_state_t.iob_u`
  - `COB:` from `aps_demo_state_t.cob_g`
  - `ACT:` from controller action mapping
  - `INS:` from `aps_demo_state_t.insulin_u_hr`
  - `SAFETY:` from `aps_demo_state_t.safe_text`
- Host validation: `make -f host.mk test` and `make -f host.mk regression` passed
- Embedded build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Physical LCD result: pending
- Notes:
  - No new LVGL objects were added.
  - CRT formatting now reads from `aps_demo_state_t` instead of hardcoded display placeholders.

## Confirmed Flash Verification — APS Demo State Wiring
- Branch: `aps-demo-state-wiring`
- Visible state confirmed on hardware:
  - `BG:` from `aps_demo_state_t.bg_mgdl`
  - `IOB:` from `aps_demo_state_t.iob_u`
  - `COB:` from `aps_demo_state_t.cob_g`
  - `ACT:` from controller action mapping
  - `INS:` from `aps_demo_state_t.insulin_u_hr`
  - `SAFETY:` from `aps_demo_state_t.safe_text`
- Build: passed
- Program: passed
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible and pancreas data appears to be coming from its own APS source
- Notes:
  - The large center `mg/dL` display still uses the original dashboard path and can differ from the CRT `BG:` line.
  - `APS_SAFETY_REASON_RAPID_FALL` display text is being updated from `FALL` to `RAPID`.

## Pending Flash Verification — Replay Rate Graph Label Candidate
- Branch: `aps-demo-state-wiring`
- Golden/failsafe restore target preserved: `e1ba009`
- Expected visible state after flash:
  - graph retains the existing `EdgeAI Prediction` and large score layout
  - a new static graph annotation reads `Replay:` and `60X`
  - replay advances one 5-minute sample every `5000 ms`
- Embedded build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: pending
- OpenOCD pre/post reset-run: pending
- Physical LCD result: pending

## Confirmed Flash Verification — Virtual Patient V1
- Branch: `aps-demo-state-wiring`
- Build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: passed with `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible with CRT values changing beyond 60 seconds
- Confirmed data path:
  - `BG:` from `VirtualPatientV1` through `aps_demo_state_t`
  - `IOB:` from `IobEngine`
  - `COB:` from `CobEngine`
  - `ACT:` from controller action mapping
  - `INS:` from controller output after safety
  - `SAFETY:` from safety flag mapping
- Notes:
  - Large center `mg/dL` display remains on the original dashboard path and can differ from CRT `BG:`
  - This is the current continuous APS CRT restore candidate before the `Virtual Human` label addition

## Confirmed Flash Verification — Virtual Human + Short CRT Labels
- Branch: `aps-demo-state-wiring`
- Build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: passed with `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result: live / GUI visible with `Virtual Human` above the glucose card and shortened CRT labels
- Notes:
  - CRT live values continue using the APS demo-state + virtual patient pipeline
  - This is the current active golden/failsafe display restore point

## Pending Physical Confirmation — Live Battery Update Candidate
- Branch: `aps-demo-state-wiring`
- Source baseline: static-battery golden restore point `0e058a4` plus the restored `update_battery_bar(gDashboard.sample_index)` call in `push_sample()`
- Build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: passed with `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Expected visible state:
  - WiFi bar still live on the proven path
  - battery bar still at `32x84 @ (725,97)`
  - battery bar now updates from the existing backend helper again
- Physical LCD result: pending

## Confirmed Flash Verification — Unified APS Glucose + Freeze Fix
- Branch: `aps-glucose-unified-display`
- Source change:
  - unified visible glucose paths onto the APS demo-state / virtual-patient pipeline
  - fixed startup chart seeding so the live APS runtime does not begin dozens of steps ahead
  - added an explicit UART breadcrumb if a timer APS step fails
- Runtime path verified in source:
  - `VirtualPatientV1_Step() -> ApsDemoState_Step() -> PredictorV2_Update() -> OpenApsController_DetermineBasal() -> SafetySupervisor_Apply()`
- Embedded build: passed
- Program/OpenOCD result:
  - the current board image was physically confirmed by the user as live and updating after LCD-safe flash/program/reset-run
  - debugger health signs remained `PSE846GPS2DBZC4A` and `CYBOOT_SUCCESS`
- Physical LCD result:
  - live / GUI visible
  - CRT values updating
  - glucose/chart path continues beyond the earlier five-point freeze window
- Notes:
  - Predictor V2 is active in the live loop
  - controller and safety are active in the live loop
  - this is a research/demo integration milestone, not a medical validation claim

## Confirmed Flash Verification — Debug printf removal recovery
- Branch: `virtual-patient-v2`
- Source change:
  - reverted uncommitted LVGL hot-path UART debug / isolation changes from `proj_cm55/main.c` and `proj_cm55/app/EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit/edgeai_insulin_pump_app.c`
- Build: passed with `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: passed with `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD pre-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- OpenOCD post-reset: healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result:
  - LCD live / GUI visible
  - user then observed a visible freeze again after around seven steps
- Notes:
  - this confirms hot-path debug instrumentation was unsafe for LCD bring-up
  - this does not clear the underlying visible-freeze issue on the clean runtime image

## Pending Physical Confirmation — VirtualPatientV2 expose image
- Branch: `vp2-background-on-v1-visible`
- Source baseline: golden/failsafe `571fb89` plus uncommitted V2 expose instrumentation
- Source change:
  - center `MG/DL` value now reads background `VirtualPatientV2` glucose instead of final debug code
  - CRT terminal now prefers background `VirtualPatientV2` state and exposes:
    - `GLUCOSE`
    - `STEP`
    - `CARBS`
    - `INS ONBD`
    - `TARGET`
    - `DEBUG`
- Host validation:
  - `rm -rf host_build && make -f host.mk test` passed
  - `rm -rf host_build && make -f host.mk regression` passed
- Embedded validation:
  - `make clean TOOLCHAIN=GCC_ARM && make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
  - OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
  - OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
- Physical LCD result: pending
- Purpose:
  - expose advancing `V2` state on already-proven display paths to see whether the freeze clusters around a specific patient phase or step

## Update 2026-06-15 — V2 Breakfast Freeze Host Fix
- Branch: `vp2-background-on-v1-visible`
- Source change:
  - migrated `ApsDemoState` from `VirtualPatientV1` to `VirtualPatientV2`
  - removed the active CRT/center-glucose display bypass that rendered separate `VirtualPatientV2Background` state
- Root cause found in source:
  - visible APS/controller/safety state and visible CRT state were previously sourced from different patient runtimes
- Host validation:
  - `make -f host.mk test` passed
  - `make -f host.mk regression` passed
  - breakfast trace for steps `0..12` passed
- Embedded validation:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
- Flash / hardware validation:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP` passed
  - OpenOCD pre-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
  - OpenOCD post-reset healthy: `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`
  - physical result: LCD live / GUI visible and CRT updates past breakfast

## Update 2026-06-15 — VirtualPatientV2 Scenario Engine
- Branch: `vp2-background-on-v1-visible`
- Source change:
  - added deterministic scenario selection to `VirtualPatientV2`
  - added support for `NORMAL`, `BREAKFAST`, `EXERCISE`, `DAWN`, `LOW_GLUCOSE`, and `RAPID_FALL`
  - preserved `BREAKFAST` as the default no-flag visible runtime
  - simplified selection to a single compile-time mode value `APP_VP_SCENARIO_MODE=1..6`
- Host validation:
  - `make -f host.mk test` passed
  - `make -f host.mk regression` passed
- Embedded validation:
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8` passed
- Flash / hardware validation:
  - not run in this milestone

## Failed Flash Verification — VirtualPatientV2 rapid scenario image
- Branch: `vp2-background-on-v1-visible`
- Source HEAD flashed: `20c23a1`
- Build/program configuration:
  - `make clean TOOLCHAIN=GCC_ARM`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_VP_SCENARIO_MODE=6 -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_VP_SCENARIO_MODE=6`
- OpenOCD result:
  - pre-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
  - post-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result:
  - blank / dead / frozen
- Notes:
  - the rapid-fall scenario image is not a safe hardware point even though build/program/reset all reported healthy

## Confirmed Recovery — exact runtime restore after rapid scenario failure
- Restore source:
  - exact commit `deeb67c`
  - detached worktree `/tmp/e84-deeb67c-restore`
- Recovery build/program configuration:
  - `make clean TOOLCHAIN=GCC_ARM`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD result:
  - pre-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
  - post-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result:
  - live / GUI visible
- Notes:
  - `deeb67c` is the latest re-confirmed LCD-live recovery image after the rapid-scenario failure

## Failed Flash Verification — VirtualPatientV2 mode-1 single-mode image
- Branch: `vp2-background-on-v1-visible`
- Source HEAD flashed: `20c23a1`
- Build/program configuration:
  - `make clean TOOLCHAIN=GCC_ARM`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_VP_SCENARIO_MODE=1 -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP DEFINES+=APP_VP_SCENARIO_MODE=1`
- OpenOCD result:
  - pre-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
  - post-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result:
  - blank / dead / frozen
- Notes:
  - the single-mode selector refactor passed host/build validation but the mode-1 hardware image is not a safe LCD point
  - physical truth remains the exact restore commit `deeb67c`

## Confirmed Recovery — exact runtime restore after mode-1 single-mode failure
- Restore source:
  - exact commit `deeb67c`
  - detached worktree `/tmp/e84-deeb67c-restore`
- Recovery build/program configuration:
  - `make clean TOOLCHAIN=GCC_ARM`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD result:
  - pre-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
  - post-reset healthy (`PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`)
- Physical LCD result:
  - pending user confirmation
