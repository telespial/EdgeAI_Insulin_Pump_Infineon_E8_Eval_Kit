# RTOS Migration Plan

## Purpose
This APS research platform should remain RTOS-ready so it can later support deterministic scheduling, task isolation, watchdog supervision, portability, easier scaling, and future safety certification paths without changing current predictor, controller, safety, or simulation behavior.

## Candidate RTOS Platforms

### FreeRTOS
- Advantages: already aligned with the current firmware style, broad ecosystem, lightweight, easy prototype path.
- Disadvantages: not a safety-certified target by itself, and still requires careful OSAL discipline for deterministic behavior.
- Likely fit: best near-term prototype environment and the easiest migration bridge.

### SafeRTOS
- Advantages: safety-oriented lineage and strong fit for constrained, deterministic embedded systems.
- Disadvantages: smaller ecosystem, licensing considerations, and more migration overhead than a prototype RTOS.
- Likely fit: future safety-oriented deployment candidate if certification direction tightens.

### embOS-Safe
- Advantages: safety-focused design, efficient scheduling, and a good fit for hard real-time embedded control.
- Disadvantages: commercial licensing and less overlap with the current FreeRTOS-based code path.
- Likely fit: strong future candidate if the project moves toward safety-oriented embedded certification.

### VxWorks Cert Edition
- Advantages: mature certified RTOS ecosystem and strong tooling for high-assurance systems.
- Disadvantages: heavier integration footprint and more complexity than this project likely needs for the current research phase.
- Likely fit: only if the project later needs a larger certified platform and a broader systems toolchain.

## Proposed Task Architecture

### Logical tasks
- CGM Input Task
- Physiology Task: IOB, COB, activity state
- Predictor Task: 15m, 30m, 60m forecasts
- Controller Task: OpenAPS-style logic
- Safety Supervisor Task: final authority
- Audit/Logging Task
- Watchdog/Health Task

### Recommended update rates
- CGM Input Task: every replay/sample tick
- Physiology Task: every sample tick, after CGM input
- Predictor Task: every sample tick
- Controller Task: every sample tick
- Safety Supervisor Task: every sample tick, last in the decision chain
- Audit/Logging Task: every sample tick or buffered batch
- Watchdog/Health Task: periodic heartbeat, independent of the main control loop

## Priority Model
- Highest: Safety Supervisor
- High: CGM Input
- Medium: Predictor, Controller
- Low: Logging
- Background: Diagnostics

Safety must run first because it is the final authority and must be able to override any lower-priority action before output is committed.

## Memory Rules
- No `malloc()` or `free()` in firmware modules
- Static allocation only
- Bounded queues only
- Bounded buffers only
- Deterministic execution paths only

## Communication Model
Recommended communication primitives:
- Message queues
- Shared state structures
- Double-buffered data exchange

Modules should not share mutable internal state directly unless it is behind a well-defined OSAL or protected exchange object. Predictor, controller, and safety should consume inputs and emit outputs through bounded interfaces rather than reaching into each other’s internals.

## Watchdog Strategy
- Task heartbeat monitoring
- Loop execution timing
- Stuck-task detection
- Safe fallback state

The watchdog should confirm that each critical task is still making forward progress and that the system falls back conservatively if a heartbeat is missed.

## Future Multi-Core Mapping

### Proposed E84 split
- CM33 Secure: security, watchdog, safety supervision
- CM33 Nonsecure: communications, diagnostics
- CM55: predictor, controller, physiology

### Why this split
- Safety supervision belongs with the most trusted execution context.
- Communications and diagnostics can remain isolated from the safety authority.
- Predictor/controller/physiology logic stays together so the research loop remains coherent and easier to profile.

## Migration Principle
The project should be RTOS-ready but not RTOS-dependent. The current firmware behavior, regression outputs, and simulation loop should remain stable while the RTOS plan matures.
