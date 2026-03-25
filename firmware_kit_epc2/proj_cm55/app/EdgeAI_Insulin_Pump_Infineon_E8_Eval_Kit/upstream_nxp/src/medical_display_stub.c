#include "medical_display_hal.h"

#include <stdio.h>

void MedicalHal_BlitRgb565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t *pixels)
{
    (void)x; (void)y; (void)width; (void)height; (void)pixels;
}

void MedicalHal_FillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}

void MedicalHal_DrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}

void MedicalHal_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    (void)x0; (void)y0; (void)x1; (void)y1; (void)color;
}

void MedicalHal_DrawCircle(int16_t cx, int16_t cy, int16_t radius, uint16_t color)
{
    (void)cx; (void)cy; (void)radius; (void)color;
}

void MedicalHal_DrawText(int16_t x, int16_t y, const char *text, uint8_t scale, uint16_t color)
{
    (void)x; (void)y; (void)scale; (void)color;
    (void)text;
}
