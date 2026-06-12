# OPS_RUNBOOK

## Environment
Use these env vars for reproducible CLI builds:

```bash
export CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7
export CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc
export CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools
```

## Clean Build
```bash
make clean TOOLCHAIN=GCC_ARM
make build TOOLCHAIN=GCC_ARM
```

## Program Kit
```bash
make program TOOLCHAIN=GCC_ARM
```

For release/golden builds in this repo, always use:
```bash
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```

## Toggle Smart Pong Mode
Edit `proj_cm55/Makefile`:
```make
DEFINES+=APP_SMART_PONG_MODE=1
```
Set to `1` for Smart Pong application (current golden). Set to `0` for music-demo fallback.

## Verify Device Detection
During flash logs, confirm:
- device family `PSE84xGxS2`
- detected part like `PSE846GPS2...`
- `Boot Status : CYBOOT_SUCCESS`

## Troubleshooting
- If build cannot find tools, re-export env vars above.
- If flash works but no display, validate hardware jumpers and `CONFIG_DISPLAY`.
- If `CONFIG_DISPLAY` is not `W4P3INCH_DISP`, build is expected to fail by design.

## E84 LCD Recovery / Flash Procedure

When the LCD is dark, use OpenOCD acquire + reset-run first:

```bash
cd /home/user/projects/embedded/codemaster/projects/Infineon/PSOC_EDGE_E8_EVAL/firmware_kit_epc2

OPENOCD=/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd
SCRIPTS=/opt/Tools/ModusToolboxProgtools-1.7/openocd/scripts
QSPIDIR=$PWD/bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource

$OPENOCD -s "$SCRIPTS" -s "$QSPIDIR" \
  -c 'set QSPI_FLASHLOADER ../flm/infineon/pse8xxgp/PSE84_SMIF.FLM' \
  -f interface/kitprog3.cfg \
  -c 'transport select swd' \
  -f target/infineon/pse84xgxs2.cfg \
  -c 'init; flash banks; reset run; sleep 2000; shutdown'
```

Expected healthy signs:
- `Detected Device: PSE846GPS2DBZC4A`
- `Boot Status : CYBOOT_SUCCESS`
- banks visible: `cat1d.cm33.main_ns` and `cat1d.cm33.smif1_ns`

If the LCD is still dark after a valid flash, rerun the command above before erasing anything.
Only consider erase if both recovery reset-run and normal reprogramming fail.
