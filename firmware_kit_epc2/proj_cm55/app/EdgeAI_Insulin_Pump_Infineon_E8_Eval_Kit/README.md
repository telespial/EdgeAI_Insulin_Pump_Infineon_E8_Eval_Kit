# EdgeAI Insulin Pump - Infineon E8 Eval Kit

This directory contains the active Infineon E84 CM55 port workspace for the insulin pump demo.

## Layout

- `upstream_nxp/`: exact copied NXP source snapshot (reference baseline, do not edit)
- `ported_nxp/`: active Infineon port modules copied from NXP and adapted for portability
- `edgeai_insulin_pump_app.c/.h`: app entry now routed to the `ported_nxp` runtime
- `platform/`: Infineon-side platform hooks used by the port runtime
- `PORTING_PLAN.md`: phased parity plan and dependency map

## Integration Contract

Entry point to call from firmware runtime when this app mode is enabled:

`edgeai_insulin_pump_app_start();`

## Current Status

- NXP source imported and checksum-tracked in `upstream_nxp/`
- First runtime path enabled in `ported_nxp/edgeai_insulin_pump_port.c`
- Portable modules integrated: `power_data_source`, `anomaly_engine`, `cgm_preprocess`, `medical_lcd_layout`
- Display shim layer added (`ported_nxp/par_lcd_s035.c`) for Infineon-side adaptation
- Touch input wired from EPC2 touch HAL into port runtime controls
- Syntax check across all local C sources: PASS

## Touch Controls (Current Port Runtime)

Top touch strip buttons:

- `AI ON/OFF`: toggles AI enable state
- `MODE REPLAY/LIVE`: toggles data source mode
- `PROFILE WIRED/OUTLET`: toggles replay profile
- `RESET`: resets replay hour to `0`
