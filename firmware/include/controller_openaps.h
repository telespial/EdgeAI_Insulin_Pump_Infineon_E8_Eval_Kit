#ifndef CONTROLLER_OPENAPS_H
#define CONTROLLER_OPENAPS_H

#include "aps_types.h"

typedef struct
{
    uint16_t target_low_mgdl;
    uint16_t target_high_mgdl;
    uint16_t suspend_threshold_mgdl;
    float basal_u_per_hr;
    float max_basal_u_per_hr;
    float insulin_sensitivity_mgdl_per_u;
    float carb_ratio_g_per_u;
    float max_iob_u;
} openaps_controller_config_t;

typedef struct
{
    uint32_t low_predicted_flags;
    uint32_t rapid_trend_flags;
    uint32_t high_predicted_flags;
    uint32_t iob_flags;
    uint32_t sensor_flags;
} openaps_reason_flags_t;

void OpenApsController_Reset(void);
void OpenApsController_Configure(const openaps_controller_config_t *config);
bool OpenApsController_DetermineBasal(const predictor_v2_input_t *input,
                                      const predictor_v2_output_t *prediction,
                                      aps_controller_output_t *output);

#endif
