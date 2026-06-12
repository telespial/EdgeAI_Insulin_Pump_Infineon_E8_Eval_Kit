# Project History

The Artificial Pancreas Research Platform is being developed on top of an existing operational Infineon PSoC Edge E84 application.

The project did not begin as an APS application.

The original platform already provided:

- Embedded firmware infrastructure
- GUI framework
- Visualization framework
- Prediction framework
- E84 multi-core project structure
- Build and deployment flow

The APS architecture is intentionally layered on top of this proven baseline.

The current branch should always be considered an extension of the last stable Failsafe Restore Point.

## Architectural Principle

Preserve the working GUI and graphing platform.

APS functionality should integrate with the existing application rather than replacing it.

## Evolution Path

Original Platform  
↓  
Failsafe Restore Point  
↓  
GUI Completed  
↓  
Graphs Completed  
↓  
Prediction System Completed  
↓  
APS Predictor V2  
↓  
Controller  
↓  
Safety Supervisor  
↓  
Replay Validation  
↓  
Regression Framework  
↓  
Future RTOS Migration

## Future Direction

The final platform should contain:

1. Existing GUI
2. Existing Graphs
3. Existing Prediction Views
4. APS Predictor V2
5. Physiology Engine
6. Controller
7. Safety Supervisor
8. Replay Validation
9. Future RTOS Layer
