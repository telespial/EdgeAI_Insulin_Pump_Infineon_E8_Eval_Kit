#include "medical_display_hal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../platform/display_hal.h"
#include "text5x7.h"

#define LCD_W 480
#define LCD_H 320

static uint16_t s_scale_row_buf[EDGEAI_LCD_W];

static inline bool in_bounds(int32_t x, int32_t y)
{
    return (x >= 0) && (x < LCD_W) && (y >= 0) && (y < LCD_H);
}

static inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
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

static inline int32_t map_x0(int32_t x)
{
    return (x * EDGEAI_LCD_W) / LCD_W;
}

static inline int32_t map_y0(int32_t y)
{
    return (y * EDGEAI_LCD_H) / LCD_H;
}

static inline int32_t map_x1(int32_t x)
{
    return (((x + 1) * EDGEAI_LCD_W) - 1) / LCD_W;
}

static inline int32_t map_y1(int32_t y)
{
    return (((y + 1) * EDGEAI_LCD_H) - 1) / LCD_H;
}

static inline void put_px(int32_t x, int32_t y, uint16_t color)
{
    int32_t dx0;
    int32_t dy0;
    int32_t dx1;
    int32_t dy1;

    if (!in_bounds(x, y))
    {
        return;
    }

    dx0 = map_x0(x);
    dy0 = map_y0(y);
    dx1 = map_x1(x);
    dy1 = map_y1(y);
    display_hal_fill_rect(dx0, dy0, dx1, dy1, color);
}

void MedicalHal_BlitRgb565(int16_t x, int16_t y, uint16_t width, uint16_t height, const uint16_t *pixels)
{
    int32_t src_w;
    int32_t src_h;
    int32_t vx0;
    int32_t vy0;
    int32_t vx1;
    int32_t vy1;
    int32_t cx0;
    int32_t cy0;
    int32_t cx1;
    int32_t cy1;
    int32_t sx_off;
    int32_t sy_off;
    int32_t clip_w;
    int32_t clip_h;
    int32_t dx0;
    int32_t dy0;
    int32_t dx1;
    int32_t dy1;
    int32_t dst_w;
    int32_t dst_h;
    int32_t dy;

    if ((pixels == 0) || (width == 0u) || (height == 0u))
    {
        return;
    }

    src_w = (int32_t)width;
    src_h = (int32_t)height;
    vx0 = (int32_t)x;
    vy0 = (int32_t)y;
    vx1 = vx0 + src_w - 1;
    vy1 = vy0 + src_h - 1;

    cx0 = clamp_i32(vx0, 0, LCD_W - 1);
    cy0 = clamp_i32(vy0, 0, LCD_H - 1);
    cx1 = clamp_i32(vx1, 0, LCD_W - 1);
    cy1 = clamp_i32(vy1, 0, LCD_H - 1);
    if ((cx1 < cx0) || (cy1 < cy0))
    {
        return;
    }

    sx_off = cx0 - vx0;
    sy_off = cy0 - vy0;
    clip_w = (cx1 - cx0) + 1;
    clip_h = (cy1 - cy0) + 1;

    dx0 = map_x0(cx0);
    dy0 = map_y0(cy0);
    dx1 = map_x1(cx1);
    dy1 = map_y1(cy1);
    dst_w = (dx1 - dx0) + 1;
    dst_h = (dy1 - dy0) + 1;

    if ((dst_w <= 0) || (dst_h <= 0) || (dst_w > EDGEAI_LCD_W))
    {
        return;
    }

    for (dy = 0; dy < dst_h; dy++)
    {
        int32_t sy = sy_off + ((dy * clip_h) / dst_h);
        int32_t y_dst = dy0 + dy;
        int32_t dx;
        const uint16_t *src_row = &pixels[(size_t)sy * (size_t)src_w];

        for (dx = 0; dx < dst_w; dx++)
        {
            int32_t sx = sx_off + ((dx * clip_w) / dst_w);
            s_scale_row_buf[dx] = src_row[sx];
        }

        display_hal_blit_rect(dx0, y_dst, dx1, y_dst, s_scale_row_buf);
    }
}

void MedicalHal_FillRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int32_t vx0 = clamp_i32((int32_t)x0, 0, LCD_W - 1);
    int32_t vy0 = clamp_i32((int32_t)y0, 0, LCD_H - 1);
    int32_t vx1 = clamp_i32((int32_t)x1, 0, LCD_W - 1);
    int32_t vy1 = clamp_i32((int32_t)y1, 0, LCD_H - 1);
    int32_t dx0;
    int32_t dy0;
    int32_t dx1;
    int32_t dy1;

    if ((vx1 < vx0) || (vy1 < vy0))
    {
        return;
    }

    dx0 = map_x0(vx0);
    dy0 = map_y0(vy0);
    dx1 = map_x1(vx1);
    dy1 = map_y1(vy1);
    display_hal_fill_rect(dx0, dy0, dx1, dy1, color);
}

void MedicalHal_DrawRect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    display_hal_fill_rect((int32_t)x0, (int32_t)y0, (int32_t)x1, (int32_t)y0, color);
    display_hal_fill_rect((int32_t)x0, (int32_t)y1, (int32_t)x1, (int32_t)y1, color);
    display_hal_fill_rect((int32_t)x0, (int32_t)y0, (int32_t)x0, (int32_t)y1, color);
    display_hal_fill_rect((int32_t)x1, (int32_t)y0, (int32_t)x1, (int32_t)y1, color);
}

void MedicalHal_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int32_t x = x0;
    int32_t y = y0;
    int32_t tx1 = x1;
    int32_t ty1 = y1;
    int32_t dx = (tx1 > x) ? (tx1 - x) : (x - tx1);
    int32_t sx = (x < tx1) ? 1 : -1;
    int32_t dy = (ty1 > y) ? (y - ty1) : (ty1 - y);
    int32_t sy = (y < ty1) ? 1 : -1;
    int32_t err = dx + dy;

    for (;;)
    {
        put_px(x, y, color);
        if ((x == tx1) && (y == ty1))
        {
            break;
        }

        if ((2 * err) >= dy)
        {
            err += dy;
            x += sx;
        }
        if ((2 * err) <= dx)
        {
            err += dx;
            y += sy;
        }
    }
}

void MedicalHal_DrawCircle(int16_t cx, int16_t cy, int16_t radius, uint16_t color)
{
    int32_t x;
    int32_t y;
    int32_t err;

    if (radius <= 0)
    {
        return;
    }

    x = radius;
    y = 0;
    err = 0;

    while (x >= y)
    {
        put_px(cx + x, cy + y, color);
        put_px(cx + y, cy + x, color);
        put_px(cx - y, cy + x, color);
        put_px(cx - x, cy + y, color);
        put_px(cx - x, cy - y, color);
        put_px(cx - y, cy - x, color);
        put_px(cx + y, cy - x, color);
        put_px(cx + x, cy - y, color);

        y++;
        err += 1 + (2 * y);
        if ((2 * (err - x)) + 1 > 0)
        {
            x--;
            err += 1 - (2 * x);
        }
    }
}

void MedicalHal_DrawText(int16_t x, int16_t y, const char *text, uint8_t scale, uint16_t color)
{
    uint8_t out_scale;

    if (text == 0)
    {
        return;
    }

    out_scale = (uint8_t)(((uint32_t)scale * (uint32_t)EDGEAI_LCD_H + (uint32_t)LCD_H - 1u) / (uint32_t)LCD_H);
    if (out_scale == 0u)
    {
        out_scale = 1u;
    }

    edgeai_text5x7_draw_scaled_no_present(map_x0((int32_t)x), map_y0((int32_t)y), (int32_t)out_scale, text, color);
}
