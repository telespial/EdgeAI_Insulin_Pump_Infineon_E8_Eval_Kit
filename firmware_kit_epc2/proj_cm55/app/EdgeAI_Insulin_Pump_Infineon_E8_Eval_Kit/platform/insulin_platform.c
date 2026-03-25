#include "insulin_platform.h"

#include "FreeRTOS.h"
#include "task.h"

#include "time_hal.h"
#include "touch_hal.h"

#define INSULIN_LCD_W 480
#define INSULIN_LCD_H 320

#ifndef INSULIN_PLATFORM_ENABLE_TOUCH
#define INSULIN_PLATFORM_ENABLE_TOUCH 0
#endif

#if INSULIN_PLATFORM_ENABLE_TOUCH
static bool s_touch_prev_active = false;
#endif

void insulin_platform_init(void)
{
    time_hal_init();
#if INSULIN_PLATFORM_ENABLE_TOUCH
    touch_hal_init();
#endif
}

uint32_t insulin_platform_now_us(void)
{
    return time_hal_cycles_to_us(time_hal_cycles());
}

void insulin_platform_sleep_until_next_tick_us(uint32_t tick_start_us, uint32_t tick_period_us)
{
    uint32_t now_us = insulin_platform_now_us();
    uint32_t elapsed_us = now_us - tick_start_us;
    uint32_t remain_us;
    if (elapsed_us >= tick_period_us)
    {
        taskYIELD();
        return;
    }

    remain_us = tick_period_us - elapsed_us;
    if (remain_us >= 1000u)
    {
        TickType_t ticks = pdMS_TO_TICKS(remain_us / 1000u);
        if (ticks == 0)
        {
            ticks = 1;
        }
        vTaskDelay(ticks);
        return;
    }

    time_hal_delay_us(remain_us);
}

bool insulin_platform_read_cgm_sample(uint16_t *glucose_mgdl)
{
    if (glucose_mgdl == 0)
    {
        return false;
    }

    *glucose_mgdl = 110u;
    return true;
}

void insulin_platform_touch_poll(insulin_platform_touch_t *touch)
{
#if INSULIN_PLATFORM_ENABLE_TOUCH
    edgeai_touch_state_t ts;
    bool active;
    int32_t x = 0;
    int32_t y = 0;
#endif

    if (touch == 0)
    {
        return;
    }

    touch->active = false;
    touch->pressed_edge = false;
    touch->x = 0;
    touch->y = 0;

#if !INSULIN_PLATFORM_ENABLE_TOUCH
    return;
#else
    touch_hal_poll(&ts);
    active = (ts.count > 0u) && ts.points[0].active;
    touch->pressed_edge = active && !s_touch_prev_active;
    s_touch_prev_active = active;

    if (!active)
    {
        return;
    }

    x = (int32_t)(ts.points[0].x * (float)(INSULIN_LCD_W - 1));
    y = (int32_t)(ts.points[0].y * (float)(INSULIN_LCD_H - 1));

    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x >= INSULIN_LCD_W)
    {
        x = INSULIN_LCD_W - 1;
    }
    if (y >= INSULIN_LCD_H)
    {
        y = INSULIN_LCD_H - 1;
    }

    touch->active = true;
    touch->x = (int16_t)x;
    touch->y = (int16_t)y;
#endif
}

void insulin_platform_ui_begin(void)
{
}

void insulin_platform_ui_draw_summary(uint16_t glucose_mgdl,
                                      uint16_t pred_15m_mgdl,
                                      uint16_t pred_30m_mgdl,
                                      bool alert_hypo,
                                      bool alert_hyper)
{
    (void)glucose_mgdl;
    (void)pred_15m_mgdl;
    (void)pred_30m_mgdl;
    (void)alert_hypo;
    (void)alert_hyper;
}

void insulin_platform_ui_present(void)
{
}
