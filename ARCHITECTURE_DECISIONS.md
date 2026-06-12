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
