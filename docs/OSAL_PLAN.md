# OSAL Plan

## Goal
Introduce a small OS abstraction layer so firmware modules can remain agnostic to the underlying runtime while still supporting bare metal today and FreeRTOS, SafeRTOS, and embOS-Safe later.

## Proposed API
```c
aps_task_create();
aps_mutex_lock();
aps_mutex_unlock();
aps_queue_send();
aps_queue_receive();
aps_time_ms();
aps_watchdog_kick();
```

## Rules
- Predictor code must not call RTOS APIs directly.
- Controller code must not call RTOS APIs directly.
- Safety code must not call RTOS APIs directly.
- Firmware modules should only use the OSAL.

## Why this helps
- Bare metal: OSAL can map to direct calls, timers, or no-op wrappers.
- FreeRTOS: OSAL can map to native tasks, queues, mutexes, and tick timing.
- SafeRTOS: OSAL can keep the same application interfaces while changing the runtime backend.
- embOS-Safe: OSAL can preserve application logic and switch only the OS backend implementation.

## Design Notes
- Keep the API minimal and bounded.
- Prefer compile-time configuration over runtime discovery.
- Keep memory ownership explicit and static.
- Make timing and watchdog hooks available without exposing kernel internals.
