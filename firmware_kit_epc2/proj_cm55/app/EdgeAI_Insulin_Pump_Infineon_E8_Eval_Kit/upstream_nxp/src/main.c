#include <stdio.h>
#include <string.h>

#include "medical_lcd_layout.h"

int main(void)
{
    medical_lcd_state_t state;
    memset(&state, 0, sizeof(state));

    state.elapsed_hh = 0u;
    state.elapsed_mm = 0u;
    state.elapsed_ss = 12u;
    state.motor_rpm = 1830u;
    state.motor_current_ma = 285u;
    state.motor_load_pct = 34u;
    state.motor_anomaly_pct = 18u;
    state.pump_rate_ml_h = 22u;
    state.pump_fill_pct = 71u;
    state.pump_active = true;
    state.temp_c10 = 367;
    state.anomaly_score_pct = 12u;
    state.ai_enabled = true;
    state.gyro_angle_deg = 32;
    state.inverted = false;
    state.drop_detected = false;
    state.graph_count = 8u;
    state.graph_points[0] = 80u;
    state.graph_points[1] = 120u;
    state.graph_points[2] = 110u;
    state.graph_points[3] = 140u;
    state.graph_points[4] = 200u;
    state.graph_points[5] = 180u;
    state.graph_points[6] = 150u;
    state.graph_points[7] = 160u;

    MedicalLcd_DrawStaticLayout();
    MedicalLcd_DrawDynamic(&state);

    puts("EdgeAI medical device LCD layout staged.");
    return 0;
}
