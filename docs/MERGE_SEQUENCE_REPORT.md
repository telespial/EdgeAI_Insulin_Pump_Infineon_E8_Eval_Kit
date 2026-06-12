# Merge Sequence Report

## Docs-Only Merge

- Branch merged: `update-main-aps-readme` into `main`
- Resulting `main` commit after the docs merge: `6f6417f`
- Outcome: the repository homepage now presents the EdgeAI Insulin Pump & Artificial Pancreas Research Platform identity
- Legacy Smart Pong / LVGL demo identity text is no longer present in `README.md`
- Legacy display bring-up history remains documented in `docs/LEGACY_DISPLAY_BRINGUP.md`

## PR #1 Merge

- Branch merged: `predictor-v2-candidate-v1-dry-run` into `main`
- Candidate branch tip before the merge: `5606c5c`
- Resulting `main` merge commit: `74c8491`
- Outcome: the Predictor V2 Candidate V1 generated tables and supporting host-side firmware/model work are now merged on `main`

## Validation

- Host tests: passed with `make -f host.mk test`
- Host regression: passed with `make -f host.mk regression`
- Embedded build: passed with `make clean && make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8`
- Size delta: `0`
- Flash/programming: not performed

## Notes

- `README.md` on `main` now starts with the APS research platform title.
- No LCD/GUI, APS runtime, controller, or safety behavior changes were introduced by the merge sequence itself.
- The candidate merge was validated before merge and revalidated on the merged tree.
