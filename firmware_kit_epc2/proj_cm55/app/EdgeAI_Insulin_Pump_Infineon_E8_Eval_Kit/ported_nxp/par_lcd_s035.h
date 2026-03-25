#pragma once

#include <stdbool.h>
#include <stdint.h>

bool par_lcd_s035_init(void);
void par_lcd_s035_fill(uint16_t rgb565);
void par_lcd_s035_blit_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t *rgb565);
void par_lcd_s035_draw_filled_circle(int32_t cx, int32_t cy, int32_t r, uint16_t rgb565);
void par_lcd_s035_fill_rect(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t rgb565);
void par_lcd_s035_draw_silver_ball(int32_t cx, int32_t cy, int32_t r,
                                   uint32_t phase, uint8_t glint,
                                   int32_t spin_sin_q14, int32_t spin_cos_q14);
void par_lcd_s035_draw_ball_shadow(int32_t cx, int32_t cy, int32_t r, uint32_t alpha_max);
