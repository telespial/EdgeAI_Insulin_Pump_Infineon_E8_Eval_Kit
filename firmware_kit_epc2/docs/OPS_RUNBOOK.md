# OPS_RUNBOOK

## Bootstrap Local Modus Links (One-Time)
```bash
# 1) Full Modus tools package (3.7) extracted under:
# /home/user/toolchains/infineon/ModusToolbox_local_extract
# Link into expected location:
mkdir -p /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox
ln -sfn /home/user/toolchains/infineon/ModusToolbox_local_extract/opt/Tools/ModusToolbox/tools_3.7 \
  /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7

# 2) Edge Protect Security Suite extracted under:
# /home/user/toolchains/infineon/ModusToolbox_edgeprotect_extract
ln -sfn /home/user/toolchains/infineon/ModusToolbox_edgeprotect_extract/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1 \
  /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1

# 3) Modus tools package on Linux may not include modus-shell wrappers.
# Add wrapper links expected by mtbquery:
mkdir -p /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7/modus-shell/bin
ln -sfn /usr/bin/make /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7/modus-shell/bin/make
ln -sfn /usr/bin/git  /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7/modus-shell/bin/git
ln -sfn /bin/bash     /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7/modus-shell/bin/bash
```

## Environment
```bash
# Auto-detect full Modus build tools folder (tools_X.Y).
export CY_TOOLS_PATHS="$(ls -d /home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_* /opt/Tools/ModusToolbox/tools_* $HOME/ModusToolbox/tools_* 2>/dev/null | sort | tail -n1)"

# GCC location used in this workspace.
export CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc

# EdgeProtect CLI required by combine-sign postbuild.
export CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools

echo "CY_TOOLS_PATHS=${CY_TOOLS_PATHS}"
```

## Build
```bash
make clean TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
```

## Program
```bash
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```

## Direct Flash Fallback (No full tools_X.Y)
```bash
/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd \
  -s /opt/Tools/ModusToolboxProgtools-1.7/openocd/scripts \
  -s bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource \
  -c "set QSPI_FLASHLOADER PSE84_SMIF.FLM" \
  -c "source [find interface/kitprog3.cfg]; transport select swd; source [find target/infineon/pse84xgxs2.cfg]; init; reset init; adapter speed 12000; flash write_image erase build/app_combined.hex; verify_image build/app_combined.hex; reset run; shutdown;"
```

## Flash Rule
- Always flash only after a successful build in the current source state.
