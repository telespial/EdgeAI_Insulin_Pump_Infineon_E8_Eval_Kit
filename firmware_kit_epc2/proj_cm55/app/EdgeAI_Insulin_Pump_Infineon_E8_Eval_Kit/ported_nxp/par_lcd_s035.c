#include "par_lcd_s035.h"

#include "medical_display_hal.h"

#define LCD_W 480
#define LCD_H 320

static int32_t clip_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo)
    {
        return lo;
    }
    if (v > hi)
    {
        return hi;
    }
    return v;
}

bool par_lcd_s035_init(void)
{
    return true;
}

void par_lcd_s035_fill(uint16_t rgb565)
{
    MedicalHal_FillRect(0, 0, LCD_W - 1, LCD_H - 1, rgb565);
}

void par_lcd_s035_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t rgb565)
{
    if ((x1 < x0) || (y1 < y0))
    {
        return;
    }

    x0 = clip_i32(x0, 0, LCD_W - 1);
    x1 = clip_i32(x1, 0, LCD_W - 1);
    y0 = clip_i32(y0, 0, LCD_H - 1);
    y1 = clip_i32(y1, 0, LCD_H - 1);

    MedicalHal_FillRect((int16_t)x0, (int16_t)y0, (int16_t)x1, (int16_t)y1, rgb565);
}

void par_lcd_s035_blit_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t *rgb565)
{
    uint16_t w;
    uint16_t h;

    if ((rgb565 == 0) || (x1 < x0) || (y1 < y0))
    {
        return;
    }

    x0 = clip_i32(x0, 0, LCD_W - 1);
    x1 = clip_i32(x1, 0, LCD_W - 1);
    y0 = clip_i32(y0, 0, LCD_H - 1);
    y1 = clip_i32(y1, 0, LCD_H - 1);

    w = (uint16_t)(x1 - x0 + 1);
    h = (uint16_t)(y1 - y0 + 1);
    MedicalHal_BlitRgb565((int16_t)x0, (int16_t)y0, w, h, rgb565);
}

void par_lcd_s035_draw_filled_circle(int32_t cx, int32_t cy, int32_t r, uint16_t rgb565)
{
    MedicalHal_DrawCircle((int16_t)cx, (int16_t)cy, (int16_t)r, rgb565);
}

void par_lcd_s035_draw_silver_ball(int32_t cx, int32_t cy, int32_t r,
                                   uint32_t phase, uint8_t glint,
                                   int32_t spin_sin_q14, int32_t spin_cos_q14)
{
    (void)phase;
    (void)glint;
    (void)spin_sin_q14;
    (void)spin_cos_q14;
    MedicalHal_DrawCircle((int16_t)cx, (int16_t)cy, (int16_t)r, 0xC618u);
}

void par_lcd_s035_draw_ball_shadow(int32_t cx, int32_t cy, int32_t r, uint32_t alpha_max)
{
    (void)alpha_max;
    if (r > 1)
    {
        MedicalHal_DrawCircle((int16_t)cx, (int16_t)(cy + (r / 2)), (int16_t)(r - 1), 0x18E3u);
    }
}
