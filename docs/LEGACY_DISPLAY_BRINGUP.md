# Legacy Display Bring-Up

This document preserves the early display and touch bring-up details that were useful while the E84 platform was being validated.

## Purpose

The repository previously used an LCD/touch demo path as proof-of-life infrastructure during bring-up. That path helped validate the display, touch, and boot pipeline before the APS research platform took shape.

## Legacy Display Details

- Waveshare 4.3-inch Raspberry Pi DSI 800x480 pixel display
- Waveshare 7-inch Raspberry Pi DSI LCD C 1024x600 pixel display
- 10.1 inch 1024x600 TFT display (WF101JTYAHMNB0)

## Legacy Three-Project Structure

The original embedded example used:

- CM33 secure
- CM33 non-secure
- CM55

The display bring-up path configured the LCD and touch stack before the APS research workloads were layered on top.

## Legacy Hardware Notes

- BOOT SW must be in the HIGH/ON position
- J20 and J21 must be in the tristate/not connected (NC) position for the E84 evaluation kit
- The 4.3-inch display is the default legacy panel
- The 7-inch and 10.1-inch panels require alternate `CONFIG_DISPLAY` selections

## Historical Reference Commands

```bash
CONFIG_DISPLAY=W4P3INCH_DISP
CONFIG_DISPLAY=WS7P0DSI_RPI_DISP
CONFIG_DISPLAY=WF101JTYAHMNB0_DISP
```

This file is intentionally historical. It exists so the repository landing page can stay focused on the APS research platform while the legacy bring-up details remain available for reference.
