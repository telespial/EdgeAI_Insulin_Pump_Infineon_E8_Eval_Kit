#ifndef EDGEAI_MEDICAL_LCD_LAYOUT_H
#define EDGEAI_MEDICAL_LCD_LAYOUT_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t elapsed_hh;
    uint8_t elapsed_mm;
    uint8_t elapsed_ss;
    uint8_t elapsed_ds;

    uint16_t motor_rpm;
    uint16_t motor_current_ma;
    uint16_t motor_load_pct;
    uint8_t motor_anomaly_pct;
    uint16_t pump_rate_ml_h;
    uint8_t pump_fill_pct;
    bool pump_active;

    int16_t temp_c10;
    uint8_t anomaly_score_pct;
    bool ai_enabled;
    int16_t gyro_angle_deg;
    bool inverted;
    bool drop_detected;

    uint16_t graph_points[96];
    uint8_t graph_count;
} medical_lcd_state_t;

void MedicalLcd_DrawStaticLayout(void);
void MedicalLcd_DrawDynamic(const medical_lcd_state_t *state);

#endif
