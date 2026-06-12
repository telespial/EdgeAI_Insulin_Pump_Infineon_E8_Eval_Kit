# Required Session Startup

Before making changes in this project:

1. Read this `AGENTS.md`.
2. Read:
   - `docs/PROJECT_STATE.md`
   - `docs/COMMAND_LOG.md`
   - `docs/BUILD_FLASH_VERIFICATION.md`
   - `docs/MUST_READ_LCD_RECOVERY.md` if present
   - `docs/LCD_REGRESSION_3FDD51A.md` if present
3. Identify current branch and commit:
   - `git branch --show-current`
   - `git rev-parse --short HEAD`
4. State whether the task is:
   - host-only
   - embedded build-only
   - embedded runtime
   - flash/programming
   - documentation-only
5. Do not proceed with flash/programming work unless LCD recovery rules are understood.

# E84 APS Project Rules

This repository is an Infineon PSoC Edge E84 insulin pump and artificial pancreas research platform built on top of an existing working GUI/LCD/graph/prediction baseline.

The APS work is an extension of the Failsafe Restore Point, not a replacement for the original display application.

## Critical Rules

1. Preserve the LCD/GUI baseline.
   - The LCD, LVGL, touch, graphing, and existing prediction display are sacred.
   - Do not replace or bypass the GUI/app loop.
   - APS code must run alongside the GUI, not instead of it.

2. Physical LCD observation overrides UART assumptions.
   - Do not claim "LCD working" based only on UART.
   - UART boot success does not prove LCD success.
   - If the user says the LCD is blank, treat that as the source of truth.

3. Follow the LCD recovery procedure before flash testing.
   - Use the documented OpenOCD acquire/reset-run workflow before and after programming.
   - Do not erase flash first.
   - Do not repeatedly reflash blindly.
   - Avoid full-chip/internal RRAM erase unless reset-run and normal program both fail.

4. Clean build required after probe/smoke flag changes.
   - After toggling `APP_APS_EMBEDDED_PROBE`, `APP_APS_SMOKE_TEST`, `APP_INSULIN_PUMP_MODE`, or display config flags, run a full clean rebuild.
   - Stale probe-enabled artifacts previously caused misleading LCD behavior.

5. No APS runtime calls from early boot/main without explicit approval.
   - Do not run Predictor, Controller, Safety, Physiology, or APS probe from early `main()` unless the task explicitly asks for it.
   - Default builds must not print `APS probe:` or run APS logic.
   - APS runtime tests must be compile-flag gated and LCD-safe.

6. No timer-driven APS loops until explicitly approved.
   - Do not create APS timers, while loops, or repeated service loops.
   - Do not starve LVGL, touch, display refresh, or existing app services.

7. Host-tested is not the same as embedded-runtime verified.
   - Classify modules honestly:
     - host tested
     - embedded linked
     - one-shot runtime verified
     - runtime loop verified
   - Do not claim hardware runtime success for host-only modules.

8. Always preserve project lineage.
   - Original E84 platform
   - Failsafe Restore Point
   - GUI/graphs/prediction baseline
   - APS Predictor V2
   - Physiology Engine
   - Controller
   - Safety Supervisor
   - Replay/regression
   - Future RTOS migration

9. Safety supervisor always wins.
   - Predictor output must not directly command insulin.
   - Activity output must not directly command insulin.
   - Controller output must pass through Safety Supervisor.
   - This is research/simulation only, not a medical device.

10. No real pump integration.
    - Do not add real insulin delivery, real pump commands, or real therapy behavior.
    - Simulated pump output only unless the user explicitly starts a separate hardware safety review.

11. Keep firmware Cortex-M portable.
    - No malloc/free in firmware modules.
    - No POSIX dependencies in firmware modules.
    - No host-only file I/O in embedded modules.
    - Keep static/bounded buffers.

12. Run required validation after shared module changes.
    - Run:
      `make -f host.mk test`
      `make -f host.mk regression`
    - For embedded changes, also build with:
      `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
    - Only program hardware after a successful build and LCD-safe reset procedure.

13. Update project evidence docs after meaningful work.
    - Update `docs/COMMAND_LOG.md` with commands.
    - Update `docs/PROJECT_STATE.md` with state changes.
    - Update `CODEX_PROGRESS_LOG.md` after milestones.
    - Update `docs/BUILD_FLASH_VERIFICATION.md` for build/flash/runtime evidence changes.
    - Update `ToDo.md` when milestone status changes.

14. Do not trust stale artifacts.
    - If behavior does not match source code, run `make clean`, rebuild, reprogram, and reset-run before making conclusions.

15. Default branch/homepage rules.
    - The public README should describe the EdgeAI Insulin Pump & Artificial Pancreas Research Platform.
    - Legacy Pong/LVGL/demo material belongs in docs, not as the repository identity.

# Never Again Rules

- Never report LCD success without physical confirmation or explicit user confirmation.
- Never assume UART success means display success.
- Never leave probe-enabled artifacts in the default build.
- Never run APS logic from early boot without explicit approval.
- Never erase flash as a first recovery step.
- Never merge host-only APS claims into embedded-runtime claims.

