# Virtual Patient V2

## Goal
Upgrade the earlier deterministic demo generator into a more physiology-driven artificial patient that continuously produces bounded:

- BG
- IOB
- COB
- activity effect
- insulin sensitivity shifts
- dawn-phenomenon shifts

without changing the LCD rendering path.

## Files
- `firmware/include/virtual_patient_v2.h`
- `firmware/src/virtual_patient_v2.c`

## API
- `void VirtualPatientV2_Init(void);`
- `bool VirtualPatientV2_Step(uint32_t now_s, float delivered_insulin_u_hr, virtual_patient_v2_state_t *state);`

## State
- `epoch_s`
- `bg_mgdl`
- `iob_u`
- `cob_g`
- `insulin_sensitivity`
- `activity_factor`
- `dawn_factor`
- `basal_u_hr`
- `meal_event`
- `bolus_event`

## Model Architecture

### Meal Path
`meal event -> COB engine -> glucose rise`

- Breakfast is injected as a deterministic `60 g` meal.
- COB is not hardcoded in the formatter; it is produced by `CobEngine`.
- Remaining COB drives upward BG pressure until absorbed.

### Insulin Path
`controller-delivered basal -> IOB engine -> glucose fall`

- The patient consumes the prior controller-delivered insulin rate.
- That insulin is fed into `IobEngine` every 5-minute step.
- When delivered basal rises high enough, a small deterministic micro-bolus surrogate is added so the artificial patient can show a clearer IOB response during meal compensation.

### Activity Path
`exercise phase -> activity_factor -> higher insulin sensitivity`

- Exercise is modeled as a deterministic phase window.
- During exercise, `activity_factor` increases above baseline.
- Higher activity increases effective insulin sensitivity and slightly lowers BG pressure.

### Dawn Path
`time-of-day phase -> dawn_factor -> rising glucose pressure`

- A dawn window raises `dawn_factor`.
- Dawn reduces effective insulin sensitivity and adds upward BG pressure.

## Scenario Phases

### Phase 1 — Baseline
- BG starts near `110 mg/dL`
- IOB starts near `0.8 U`
- COB starts at `0 g`

### Phase 2 — Breakfast
- A deterministic breakfast event occurs early in the cycle
- COB rises immediately
- BG begins climbing

### Phase 3 — Controller Response
- Higher BG encourages a stronger controller basal request
- Delivered insulin feeds back into patient state
- IOB grows from the insulin path

### Phase 4 — Recovery
- As COB declines and insulin effect accumulates, BG trends back toward target

### Phase 5 — Exercise
- Activity factor rises
- Effective insulin sensitivity increases
- Controller decisions occur against a more insulin-sensitive patient

### Phase 6 — Night / Dawn
- Dawn factor rises
- BG pressure drifts upward
- Insulin sensitivity is partially reduced

### Phase 7 — Continuous Loop
- The scenario does not terminate after 20–30 seconds
- Meals, activity, and dawn effects repeat deterministically
- Engines and patient state continue evolving without malloc or reset thrash

## Current Cycle Shape
- Step cadence: `300 s` (5 minutes) per patient step
- Cycle length: `72` steps
- Breakfast step: `2`
- Exercise window: steps `32..39`
- Dawn window: steps `52..63`

## Example One-Hour Summary
Using a simple host probe with adaptive delivered insulin:

- `0 min`: `BG 106`, `IOB 0.80`, `COB 0.00`
- `10 min`: `BG 116`, `IOB 0.82`, `COB 60.00`, meal begins
- `30 min`: `BG 138`, `IOB 0.72`, `COB 54.29`
- `60 min`: `BG 143`, `IOB 0.60`, `COB 45.71`

Interpretation:
- breakfast drives a plausible glucose rise,
- COB decays steadily instead of sticking at zero,
- IOB remains bounded and alive,
- patient state does not run out after the opening window.

## Integration
`ApsDemoState_Step()` now consumes `VirtualPatientV2` instead of `VirtualPatientV1`.

CRT values continue to map from `aps_demo_state_t`:

- `GLUCOSE:` -> `state.bg_mgdl`
- `INS ONBD:` -> `state.iob_u`
- `CARBS:` -> `state.cob_g`
- `ACTION:` -> controller action text
- `INS RATE:` -> controller requested basal after safety
- `SAFETY:` -> safety text derived from flags

No LVGL object creation or CRT layout changes are required for this upgrade.

## Validation

### Host
Passed:
- `make -f host.mk test`
- `make -f host.mk regression`

New host assertions cover:
- 10-minute continuation
- 30-minute continuation
- 1-hour continuation
- bounded BG/IOB/COB/insulin values
- activity and dawn phases
- no NaN / dead state

### Embedded
Passed build-only:
- `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`

No flash was performed in this milestone.

## Scope / Safety
- Deterministic
- No malloc
- No real pump control
- No medical claim
- Research/demo physiology only
