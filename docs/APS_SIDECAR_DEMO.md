# APS Sidecar Demo

This milestone adds an LCD-safe APS sidecar that runs alongside the existing GUI after the dashboard is already alive.

## Goals

- Keep the existing LCD/GUI flow intact.
- Run Predictor V2, the controller, and the safety supervisor on a slow demo cadence.
- Update only a small on-screen status panel and a compact UART line.
- Stay compile-flag gated by `APP_APS_SIDECAR_DEMO=1`.

## Runtime Shape

- Entry point: `ApsSidecar_Init()`
- Service loop: `ApsSidecar_Service(uint32_t now_ms)`
- Call site: the existing cooperative dashboard timer in `edgeai_insulin_pump_app.c`
- Cadence: roughly every 5 seconds

## Demo Input

The sidecar walks a deterministic glucose sequence:

- 110
- 125
- 145
- 160
- 150
- 130
- 105
- 90

The physiology inputs are fixed for the demo and are not connected to real pump control.

## Safety Rules

- No boot-time APS execution.
- No blocking loop.
- No high-rate timer.
- No pump output.
- No dynamic allocation.
- No graph redraws from the sidecar.

## Expected Output

The sidecar prints a compact UART line and updates a small APS status panel with the current BG, predictions, action, and safety state.

## Build Flag

Enable only when explicitly requested:

```text
APP_APS_SIDECAR_DEMO=1
```

The default image keeps the sidecar disabled.

For the current demo build, the cooperative insulin-pump GUI path must also be enabled:

```text
APP_INSULIN_PUMP_MODE=1
```

Without the pump GUI path active, the sidecar service never gets called.
