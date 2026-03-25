#ifndef EDGEAI_MEDICAL_DISPLAY_HAL_H
#define EDGEAI_MEDICAL_DISPLAY_HAL_H

#include <stdint.h>

/*
 * Hardware abstraction for LCD drawing primitives.
 * Platform firmware should provide these implementations.
 */
void MedicalHal_BlitRgb565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t *pixels);
void MedicalHal_FillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void MedicalHal_DrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void MedicalHal_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void MedicalHal_DrawCircle(int16_t cx, int16_t cy, int16_t radius, uint16_t color);
void MedicalHal_DrawText(int16_t x, int16_t y, const char *text, uint8_t scale, uint16_t color);

#endif
