# Porting Plan - EdgeAI Insulin Pump on Infineon E84 (EPC2)

## Source Of Truth
Exact source baseline to port:

- `/home/user/projects/embedded/codemaster/projects/Infineon/PSOC_EDGE_E8_EVAL/EdgeAI_CGM_Insulin_Pump_demo_NXP_FRDM-MCXN947`

Imported snapshot inside this app:

- `upstream_nxp/src/`
- `upstream_nxp/model/`
- `upstream_nxp/SOURCE_SNAPSHOT.sha256`

Verification status:

- file-level diff check against source: PASS
- snapshot checksum manifest generated: PASS

## Porting Strategy
1. Keep medical runtime logic, model code, scoring, and UI logic functionally identical.
2. Replace board/driver dependencies with Infineon EPC2 platform adapters.
3. Integrate into CM55 app flow in `firmware_kit_epc2/proj_cm55`.
4. Iterate on build-first, then hardware bring-up.

## Dependency Split
Likely requires platform adaptation (NXP SDK specific):

- `edgeai_medical_device_demo.c`
- `ext_flash_recorder.c`
- `gauge_render.c`
- `gauge_style.c`
- `par_lcd_s035.c`
- `real_telemetry.c`
- `text5x7.c`
- `timebase_test_main.c`

Likely portable with minimal changes:

- `cgm_preprocess.*`
- `anomaly_engine.*`
- `eil_profile.*`
- `medical_lcd_layout.*`
- replay/data/model generated headers and constants

## Execution Phases
1. Build a compatibility HAL layer for display/touch/time/storage/sensors.
Status: in progress (`ported_nxp/par_lcd_s035.c` shim and `medical_display_hal` path wired).
2. Port rendering path to EPC2 display pipeline (4.3-inch DSI).
3. Port touch/settings flow and replay timing loop.
4. Reconnect sensor paths (BMI270 + available EPC2 sensors) with safe fallbacks.
5. Validate prediction, score windows, and alert behavior against source traces.

## Working Rule
All functional parity checks are evaluated against the imported `upstream_nxp` snapshot.
