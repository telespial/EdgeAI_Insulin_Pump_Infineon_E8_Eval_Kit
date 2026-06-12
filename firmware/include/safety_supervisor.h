#ifndef SAFETY_SUPERVISOR_H
#define SAFETY_SUPERVISOR_H

#include "controller_openaps.h"

typedef struct
{
    uint16_t min_valid_glucose_mgdl;
    uint16_t max_valid_glucose_mgdl;
    uint16_t low_prediction_threshold_mgdl;
    uint8_t min_sqi_for_increase_pct;
    uint32_t max_cgm_age_s;
    float max_basal_u_per_hr;
    float max_iob_u;
} aps_safety_config_t;

void SafetySupervisor_Configure(const aps_safety_config_t *config);
bool SafetySupervisor_Apply(uint32_t now_s,
                            const predictor_v2_input_t *input,
                            const predictor_v2_output_t *prediction,
                            aps_controller_output_t *command);

#endif
