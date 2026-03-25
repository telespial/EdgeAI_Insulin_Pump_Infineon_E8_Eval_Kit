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
cd /home/user/projects/embedded/codemaster/projects/Infineon/PSOC_EDGE_E8_EVAL/firmware_kit_epc2
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
  - Example known-good path:
    `/home/user/toolchains/infineon/ModusToolbox_local/opt/Tools/ModusToolbox-Edge-Protect-Security-Suite-1.6.1/tools/edgeprotecttools/bin/edgeprotecttools`
