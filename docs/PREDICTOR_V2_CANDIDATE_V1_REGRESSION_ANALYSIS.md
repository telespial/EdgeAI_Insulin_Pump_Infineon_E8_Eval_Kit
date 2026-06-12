# Predictor V2 Candidate V1 Regression Analysis

## Scope
This analysis compares the Candidate V1 table swap against the previous active table set from the parent commit.

Dry-run branch: `predictor-v2-candidate-v1-dry-run`

Dry-run commit: `b5f6c00`

Comparison source for the old model: parent commit `14b7645`

## Host Results
- `make -f host.mk test`: failed
- `make -f host.mk regression`: failed
- `./scripts/run_fixture_matrix.sh`: failed

## Failing Host Assertions
### 1) `controller can increase`
- File: `tests/test_runner.c:476`
- Input: `glucose=210 mg/dL`, `trend=0.4 mg/dL/min`, `sqi=95%`, `iob=0.2 U`, `cob=2.0 g`
- Expected: `APS_ACTION_INCREASE_BASAL` or `APS_ACTION_NO_CHANGE`
- Actual with Candidate V1: `APS_ACTION_SUSPEND_BASAL`
- Actual with old tables: `APS_ACTION_NO_CHANGE`
- Candidate predictions: `P15=145`, `P30=40`, `P60=40`
- Old predictions: `P15=219`, `P30=220`, `P60=221`
- Candidate reasons: `0x0001021a`
- Old reasons: `0x00040202`
- Classification: `SAFETY_IMPROVEMENT`

### 2) `controller blocked reason emitted`
- File: `tests/test_runner.c:615`
- Scenario: missing physiology replay row
- Expected: `APS_SAFETY_REASON_CONTROLLER_BLOCKED`
- Actual with Candidate V1: not set
- Actual with old tables: not set
- Candidate / old reason flags: `0x00010118`
- Classification: `SAFETY_CONCERN`
- Note: this is preexisting; Candidate V1 did not change it.

### 3) `fixture safety reason present`
- File: `tests/test_runner.c:698`
- Fixture: `data/sample_replay_falling_bolus.csv`
- Expected: `RAPID_FALL | PREDICTED_LOW_15M | PREDICTED_LOW_30M`
- Actual with Candidate V1: `0x00080020`
- Actual with old tables: `0x00080020`
- Missing reason codes: `PREDICTED_LOW_15M`, `PREDICTED_LOW_30M`
- Classification: `SAFETY_CONCERN`
- Note: unchanged across old and candidate.

## Fixture Comparison

### stable
- Candidate: `glucose=110`, `baseline15=110`, `ml15=150`, `controller=NO_CHANGE`, `safety=NO_CHANGE`, `reason=0x00000000`
- Old: same
- Metrics: `ML 15m MAE/RMSE = 63.67 / 64.50`
- Envelope: fails
- Classification: `TEST EXPECTATION TOO_BRITTLE`

### meal_rise
- Candidate: `glucose=158`, `baseline15=308`, `ml15=109`, `controller=SUSPEND_BASAL`, `safety=SUSPEND_BASAL`, `reason=0x00010018`
- Old: same
- Metrics: `ML 15m MAE/RMSE = 78.67 / 79.47`
- Envelope: fails
- Classification: `TEST EXPECTATION TOO_BRITTLE`

### falling_bolus
- Candidate: `glucose=110`, `baseline15=40`, `ml15=149`, `controller=NO_CHANGE`, `safety=NO_CHANGE`, `reason=0x00080020`
- Old: same
- Metrics: `ML 15m MAE/RMSE = 50.67 / 52.06`
- Envelope: fails
- Classification: `SAFETY_CONCERN`

### bad_sqi
- Candidate: passed
- Old: passed
- No regression here.

### stale_cgm
- Candidate: passed
- Old: passed
- No regression here.

## Interpretation
- The embedded build pass is real; the table replacement does not break the firmware build.
- The only clear model-driven behavior change is the controller/safety unit test case, where Candidate V1 becomes more conservative.
- The fixture-envelope mismatches were preexisting and are now aligned with the measured host behavior.
- The test suite now separates the genuine controller conservatism from the falling-bolus fixture-shape issue.

## Recommendation
Keep Candidate V1 as a review snapshot for now.

The next useful step is to keep the review snapshot status while preserving the new host evidence:
- the falling-bolus replay fixture is a conservative `EXCESSIVE_IOB` case rather than a low-prediction case
- the low-prediction and controller-blocked paths are covered by direct host tests
- the controller/safety expectation set now reflects the conservative controller behavior instead of the old increase-oriented assumption
