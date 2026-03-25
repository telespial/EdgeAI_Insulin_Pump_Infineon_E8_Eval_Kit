#ifndef EDGEAI_INSULIN_PLATFORM_H
#define EDGEAI_INSULIN_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool active;
    bool pressed_edge;
    int16_t x;
    int16_t y;
} insulin_platform_touch_t;

void insulin_platform_init(void);
uint32_t insulin_platform_now_us(void);
void insulin_platform_sleep_until_next_tick_us(uint32_t tick_start_us, uint32_t tick_period_us);
bool insulin_platform_read_cgm_sample(uint16_t *glucose_mgdl);
void insulin_platform_touch_poll(insulin_platform_touch_t *touch);

void insulin_platform_ui_begin(void);
void insulin_platform_ui_draw_summary(uint16_t glucose_mgdl,
                                      uint16_t pred_15m_mgdl,
                                      uint16_t pred_30m_mgdl,
                                      bool alert_hypo,
                                      bool alert_hyper);
void insulin_platform_ui_present(void);

#endif /* EDGEAI_INSULIN_PLATFORM_H */
