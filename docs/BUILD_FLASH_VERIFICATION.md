# Build/Flash Verification Evidence Report

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
