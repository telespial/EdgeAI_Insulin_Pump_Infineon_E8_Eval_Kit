# LCD Recovery Active Incident

## Date
- 2026-06-13

## Recovery Context
- Branch before recovery: `mini-terminal-aps-demo`
- Commit before recovery: `087abe1`
- Working-tree note: incident-doc edits were stashed as `lcd incident docs before 087abe1 recovery` so the exact baseline recovery could proceed from a clean tree.

## Known-Good Baseline Tested
- Commit flashed: `087abe1`
- Checkout state during recovery: detached `HEAD` at `087abe1`

## Commands Used
- Clean build:
  - `make clean`
  - `make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- LCD-safe reset before program:
  - OpenOCD `init; flash banks; reset run; sleep 2000; shutdown`
- Program:
  - `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- LCD-safe reset after program:
  - OpenOCD `init; flash banks; reset run; sleep 2000; shutdown`

## OpenOCD Result
- Pre-program acquire/reset succeeded.
- Post-program acquire/reset succeeded.
- Target reported `PSE846GPS2DBZC4A`.
- Boot status reported `CYBOOT_SUCCESS`.
- Flash banks were visible.

## UART Result
- Short `/dev/ttyACM0` capture was inconclusive and appeared garbled, so UART is not being used as LCD proof for this incident.

## Physical LCD Result
- Confirmed on; GUI visible.

## Smart Pong Baseline
- Not tested in this recovery pass.

## Current Suspected Root Cause
- Still unconfirmed.
- This incident does not prove the original mini-terminal change was the sole cause of the blank panel, but it does prove the exact known-good pump baseline can be rebuilt and reflashed back to a healthy LCD state.
- If a future blank-panel incident survives this exact baseline recovery path, the next comparison should be the original Smart Pong / LCD bring-up image versus this pump baseline.

## Recovery Conclusion
- Recovery commit: `087abe1`
- No APS flags were used in the recovery build or program path.
- OpenOCD remained healthy throughout recovery, reporting `PSE846GPS2DBZC4A` and `CYBOOT_SUCCESS`.
- UART was not used as evidence because the short capture was garbled and inconclusive.
- Conclusion: known-good LCD baseline restored.
