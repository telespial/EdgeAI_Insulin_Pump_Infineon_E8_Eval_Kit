# Current Status And Completion Plan

## Purpose
This document is the detailed status snapshot for the current E84 APS/LCD codebase. It captures:

- what is physically verified on hardware,
- what is only source-built or source-pushed,
- what the active restore points are,
- what the current architecture is,
- what is still incomplete.

This is intended to be the easiest single-file handoff for the next coding session.

## Repository Snapshot
- Repo path: `Infineon/PSOC_EDGE_E8_EVAL/firmware_kit_epc2`
- Active branch at time of writing: `aps-demo-state-wiring`
- Current `HEAD`: `d931204`
- Most recent source checkpoint commit:
  - `aed98c2` — `save live battery restore candidate`
- Most recent log-only follow-up commit:
  - `d931204` — `log live battery restore candidate tag`

## Hardware / Platform Context
- Board: `KIT_PSE84_EVAL_EPC2`
- Detected MCU during healthy flash/reset:
  - `PSE846GPS2DBZC4A`
- Healthy OpenOCD signs:
  - `PSE846GPS2DBZC4A`
  - `CYBOOT_SUCCESS`
- Display profile:
  - `CONFIG_DISPLAY=W4P3INCH_DISP`

## Current Architecture Split
The screen currently shows two different glucose-related paths:

### Original Dashboard Path
This is the older, already-working GUI path:
- large center `mg/dL` display,
- graph,
- prediction title/score area,
- existing dashboard timer and replay path.

This path is still alive and still drives the original middle glucose display.

### APS CRT / Demo-State Path
This is the newer artificial-pancreas path:
- lower-left CRT-style APS block,
- `Virtual Human` label,
- virtual patient driven APS state,
- controller + safety supervisor integration,
- live CRT values for APS-related fields.

Important:
- the large center `mg/dL` value and the CRT `GLUCOSE:` value can differ,
- that is expected right now,
- because they come from different runtime paths.

## Current UI State

### Physically Verified LCD/UI State
These items have been physically confirmed on hardware in the current development line:

- LCD boots and GUI renders
- `Virtual Human` label is visible
- CRT block is visible
- CRT labels are currently:
  - `GLUCOSE:`
  - `INS ONBD:`
  - `CARBS:`
  - `ACTION:`
  - `INS RATE:`
  - `SAFETY:`
- CRT values are live from the APS demo-state / virtual-patient path
- WiFi indicator bar is visible on the upper-right
- Static battery bar graphic was physically confirmed as visually correct

### Current Upper-Right Indicator State

#### WiFi Bar
- Implemented as a screen-level LVGL bar
- Proven live update path
- Current geometry in source:
  - `32x84 @ (683,97)` for the WiFi bar in the currently documented live layout sequence

#### Battery Bar
Two separate states now exist:

1. **Physically verified static-battery state**
   - battery bar object visible
   - no backend/live update call
   - this is the last physically confirmed battery-bar state
   - commit: `0e058a4`
   - tag: `e84-golden-static-battery-2026-06-14`

2. **Live-battery source candidate**
   - same battery bar object and geometry
   - existing `update_battery_bar(gDashboard.sample_index)` call restored
   - build passed
   - program passed
   - OpenOCD pre/post reset-run passed
   - pushed to GitHub
   - **physical LCD result is still pending**
   - source checkpoint commit: `aed98c2`
   - branch tip: `d931204`
   - tag: `e84-live-battery-restore-candidate-2026-06-14`

## APS Runtime State

### What Is Real And Live Now
The CRT block is no longer using hardcoded placeholders for its main APS values.

The APS readout path is wired through the APS demo-state pipeline.

### Current APS Value Sources
- `GLUCOSE:` from APS demo-state / virtual patient state
- `INS ONBD:` from `IobEngine`
- `CARBS:` from `CobEngine`
- `ACTION:` from controller action mapping
- `INS RATE:` from controller output after safety limiting
- `SAFETY:` from safety flag mapping

### Current Safety Text Behavior
- safety text is live
- the rapid-fall condition was renamed from `FALL` to `RAPID`
- this avoids confusing a rapid-fall state with a generic system fault

## Virtual Patient State
Virtual Patient V1 is integrated and physically demonstrated in the APS CRT path.

### What It Currently Does
- replaces the old finite demo sequence
- continuously evolves BG over time
- feeds the APS demo-state pipeline
- keeps CRT values changing beyond the first short demo window

### What It Is Not Yet
- not a medically realistic patient simulator
- not a full certified closed-loop system
- not connected to any real pump hardware

## What Has Been Proven Safe

### Proven Safe On Hardware
- baseline LCD boot + GUI
- APS CRT block on current approved layout path
- `Virtual Human` label
- shortened CRT labels
- live CRT values from APS demo-state / virtual patient
- WiFi bar live display path
- static battery bar graphic path

### Proven Unsafe / Fragile
These were major discoveries and remain important constraints:

- adding new LVGL objects in the wrong path can blank the LCD
- mutating LVGL text on certain fragile paths can blank the LCD
- chart-child label invalidation was especially dangerous
- arbitrary LVGL text experiments were repeatedly unsafe
- the display stack is still fragile enough that new display work must stay extremely conservative

## Current Restore Points

### Last Physically Verified Battery-Bar Restore Point
- Commit: `0e058a4`
- Meaning:
  - WiFi bar live
  - battery bar visible as static-only graphic
  - LCD physically verified

### Live Battery Restore Candidate
- Source checkpoint commit: `aed98c2`
- Branch tip: `d931204`
- Tag: `e84-live-battery-restore-candidate-2026-06-14`
- Meaning:
  - battery live update call is restored in source
  - build/program/OpenOCD passed
  - physical LCD confirmation still needed

### APS / Virtual Human Restore Line
- Commit already documented in restore docs:
  - `89bf899`
- Meaning:
  - `Virtual Human`
  - shortened CRT labels
  - physically verified LCD-safe APS display line

## Current Code State In `edgeai_insulin_pump_app.c`

### Present Objects / Paths Of Interest
- `gDashboard.prediction_accuracy_label`
- `gDashboard.wifi_bar`
- `gDashboard.battery_bar`

### Current Functions Of Interest
- `update_wifi_bar(uint32_t sample_index)`
- `update_battery_bar(uint32_t sample_index)`
- CRT formatting/update path inside the main sample push/update flow

### Current Behavior In Source
- WiFi bar updates each sample
- battery bar update call is currently enabled again in source
- replay label is present as:
  - `Replay: 60X`
- `Virtual Human` label is present
- CRT fallback/error text is present
- CRT default visible text block is present

## Validation Status

### Host Validation
Previously confirmed for APS runtime work:
- `make -f host.mk test`
- `make -f host.mk regression`

These were already passing for the APS demo-state / virtual patient milestones before the current battery-display work.

### Embedded Validation
For the live battery candidate:
- build passed
- program passed
- OpenOCD pre-reset passed
- OpenOCD post-reset passed

### Missing Validation
Still missing for the live battery candidate:
- physical LCD confirmation
- confirmation that:
  - LCD is live
  - GUI is visible
  - CRT remains updating
  - battery bar updates without freezing the rest of the screen

## What Is Completed

### Completed Display Work
- approved prediction title/score layout changes
- `Artificial Pancreas` title work
- CRT block placement and readability work
- `Virtual Human` label placement
- shortened CRT labels
- `Replay: 60X` graph label work
- WiFi bar placement and visible live path
- static battery bar visual path

### Completed APS Runtime Work
- APS demo-state wiring
- controller/safety mapping into CRT fields
- virtual patient v1 integration
- live CRT updates beyond 60 seconds

## What Still Needs To Be Completed

### 1. Confirm Live Battery Candidate On Physical Hardware
This is the most immediate incomplete item.

Need physical confirmation for the current live-battery source candidate:
- LCD live / GUI visible
- battery bar visibly updating
- no frozen data
- no LCD blanking

If this fails:
- restore `0e058a4`

### 2. Clean Up Restore Documentation
`docs/RESTORE_POINTS.md` is partially stale and inconsistent with the current APS line.

It still contains older wording like:
- pending commits that are no longer pending
- older golden/failsafe descriptions mixed with newer APS states

This should be consolidated so restore truth is crystal clear.

### 3. Reconcile Source Truth vs Physical Truth
Right now we have:
- physically verified static-battery restore point
- source-pushed live-battery candidate

That distinction should remain explicit until hardware confirms the live-battery candidate.

### 4. Add Replay-Speed Setting Architecture
The user wants multiple replay rates and real-time/realtime-like scaling behavior.

Current status:
- replay label says `Replay: 60X`
- replay cadence is effectively set for one 5-minute sample every 5 seconds

Still needed:
- configurable replay-rate backend setting
- user-facing settings path later
- propagation of replay rate into battery drain behavior

### 5. Decide Next Safe UI Enhancements
Only after the battery candidate is physically confirmed.

Likely next safe work:
- keep using already-proven objects only
- avoid new LVGL object proliferation
- continue conservative screen-level updates only

### 6. Long-Term APS Work
Still not complete:
- richer virtual patient realism
- user-adjustable replay rate
- better alignment between CRT APS state and center dashboard state
- activity path if hardware/input source is added later
- safer consolidated status/restore docs

## Recommended Immediate Next Steps

### Highest Priority
1. Physically verify the current live-battery candidate on the board
2. If good:
   - promote `aed98c2` / `d931204` as the next physically verified golden/failsafe point
   - update restore docs accordingly
3. If bad:
   - immediately restore `0e058a4`
   - record exact failure mode

### Next After Battery Confirmation
1. clean and reconcile `docs/RESTORE_POINTS.md`
2. create one concise branch/status summary in `README.md` or an APS status doc if desired
3. plan replay-speed backend setting milestone

## Risks / Constraints To Remember
- Physical LCD observation is the source of truth
- UART/OpenOCD success does not prove LCD success
- LVGL mutation remains fragile
- avoid new display objects unless there is a very strong reason
- preserve rollback points before each risky visual/runtime change

## Short Version
We are in a strong place overall:

- APS CRT values are live
- virtual patient is integrated
- `Virtual Human` UI is working
- WiFi bar is working
- static battery bar is working

The one unresolved item right now is:

- **live battery bar wiring is pushed and flashed, but not yet physically confirmed**

That is the next truth gate.
