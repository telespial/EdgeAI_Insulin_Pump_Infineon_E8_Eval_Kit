# Flash Main `39f6361` Report

## Flashed Commit

- `39f6361` — `document merge sequence cleanup`
- This `main` tip includes the docs-only README cleanup and the merged Predictor V2 Candidate V1 tables.

## Commands

- Build: `make clean && make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Program: `make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP`
- OpenOCD pre-reset: `init; flash banks; reset run; sleep 2000; shutdown`
- OpenOCD post-reset: `init; flash banks; reset run; sleep 2000; shutdown`

## Results

- Build result: passed
- Program result: passed
- OpenOCD pre-reset result: device detected, `PSE846GPS2DBZC4A`, `CYBOOT_SUCCESS`, flash banks visible
- OpenOCD post-reset result: device detected again and reset-run completed cleanly
- UART output summary: boot banner appeared; no `APS probe:` line; no smoke sequence output
- Candidate V1 tables present: yes
- APS probe/smoke default-disabled: yes

## LCD / GUI

- UART shows the legacy graphics LVGL demo banner during boot.
- I could not directly observe the physical LCD panel from the terminal session, so this run does not claim a direct visual confirmation.
- The flashed image built and reset successfully, and the UART evidence does not show any APS probe or smoke activity.

## Notes

- No code changes were added for this milestone.
- No APS runtime probe or smoke mode was enabled by default.
- The flashed image corresponds to the merged `main` commit only.
