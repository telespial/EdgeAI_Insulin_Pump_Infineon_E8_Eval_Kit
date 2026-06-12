# Architecture Decisions

## ADR-001 — Research-only system
Decision:
This project is a research/simulation framework only and must not be represented as a medical device or real insulin dosing system.

Reason:
The code is not clinically validated, regulatory reviewed, or safe for real-world therapy.

Status:
Accepted

## ADR-002 — Safety supervisor always overrides controller
Decision:
The safety layer is the final authority before any simulated pump command.

Reason:
Prediction and controller modules can fail; safety must remain deterministic and conservative.

Status:
Accepted

## ADR-003 — Separate predictor horizons
Decision:
The 15m, 30m, and 60m outputs will be modeled independently rather than derived from a single horizon.

Reason:
Independent heads improve calibration and reduce compounding error across horizons.

Status:
Accepted

## ADR-004 — Host-first validation
Decision:
Host-side build, replay, and unit tests will precede any embedded porting work.

Reason:
This minimizes iteration cost and keeps the safety and controller logic testable before MCU integration.

Status:
Accepted

## ADR-005 — CSV replay and scenario parity
Decision:
The replay loader and synthetic scenarios will feed the same simulation runner and audit trace path.

Reason:
Using one execution path keeps CSV replay, synthetic testing, and audit logging behavior aligned.

Status:
Accepted

## ADR-006 — RTOS readiness
Decision:
The APS architecture shall remain RTOS-ready but not RTOS-dependent.

Reason:
This keeps the current research loop stable while allowing a later migration to a safety-oriented RTOS without rewriting predictor, controller, safety, or simulation behavior.

Status:
Accepted

## ADR-007 — Static allocation
Decision:
Firmware modules shall avoid dynamic allocation.

Reason:
Static allocation keeps memory use bounded and deterministic, which is important for both current research work and future safety-oriented runtime plans.

Status:
Accepted

## ADR-008 — Safety priority
Decision:
The Safety Supervisor shall always have the highest scheduling priority.

Reason:
Safety must remain the final authority and be able to override any lower-priority decision before output is committed.

Status:
Accepted

## ADR-009 — Preserve Failsafe Restore Baseline
Decision:
APS functionality shall be layered on top of the last stable Failsafe Restore Point and should not replace existing GUI, graphing, prediction, or platform infrastructure without explicit justification.

Reason:
The APS work is an extension of a working E84 platform, and preserving the proven baseline reduces regression risk while keeping the GUI, graphs, and prediction flow intact.

Status:
Accepted

## ADR-010 — Physiology engine provides predictor context
Decision:
IOB, COB, and activity state shall be computed by deterministic physiology modules and provided to Predictor V2 and controller logic as context features.

Reason:
Separating physiology estimation from prediction and control keeps the architecture deterministic, testable, and easier to evolve without disturbing the GUI or replay framework.

Status:
Accepted
