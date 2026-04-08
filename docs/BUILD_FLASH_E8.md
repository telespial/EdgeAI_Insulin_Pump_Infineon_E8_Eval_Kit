# Build and Flash - PSOC Edge E8 (4.3 inch)

## Active Project

- `firmware_kit_epc2/`

## Required Toolchain Environment

```bash
export CY_TOOLS_PATHS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox/tools_3.7
export CY_COMPILER_GCC_ARM_DIR=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/mtb-gcc-arm-eabi/14.2.1/gcc
export CY_TOOL_edgeprotecttools_EXE_ABS=/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools
```

## Build

```bash
cd firmware_kit_epc2
make getlibs
make build TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP -j8
```

## Flash

```bash
make program TOOLCHAIN=GCC_ARM CONFIG_DISPLAY=W4P3INCH_DISP
```

## Troubleshooting

- Ensure board power and debug cable are connected.
- If boot behavior is unexpected, confirm board boot switch settings.
- If display is blank after flash, re-check display config (`W4P3INCH_DISP`) and known working restore tag.
- If combine/sign fails with `EdgeProtect Secure Suite not found`, verify:
  - `CY_TOOL_edgeprotecttools_EXE_ABS` points to a valid `edgeprotecttools` binary.

## Direct Flash Alternative (Installed Progtools Path)

If full Modus `tools_3.7` make tooling is unavailable but programmer tools are installed, you can flash an existing `app_combined.hex` directly:

```bash
/opt/Tools/ModusToolboxProgtools-1.7/openocd/bin/openocd \
  -s /opt/Tools/ModusToolboxProgtools-1.7/openocd/scripts \
  -s firmware_kit_epc2/bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource \
  -c "set QSPI_FLASHLOADER firmware_kit_epc2/bsps/TARGET_APP_KIT_PSE84_EVAL_EPC2/config/GeneratedSource/PSE84_SMIF.FLM" \
  -c "source [find interface/kitprog3.cfg]; transport select swd; source [find target/infineon/pse84xgxs2.cfg]; init; reset init; adapter speed 12000; flash write_image erase firmware_kit_epc2/build/app_combined.hex; verify_image firmware_kit_epc2/build/app_combined.hex; reset run; shutdown;"
```
