#include "controller_openaps.h"

#include <math.h>
#include <string.h>

#define OPENAPS_REASON_LOW_PRED (1u << 16)
#define OPENAPS_REASON_RAPID_FALL (1u << 17)
#define OPENAPS_REASON_HIGH_PRED (1u << 18)
#define OPENAPS_REASON_HIGH_IOB (1u << 19)
#define OPENAPS_REASON_BAD_SENSOR (1u << 20)

static openaps_controller_config_t g_config =
{
    .target_low_mgdl = 100u,
    .target_high_mgdl = 120u,
    .suspend_threshold_mgdl = 80u,
    .basal_u_per_hr = 0.8f,
    .max_basal_u_per_hr = 2.0f,
    .insulin_sensitivity_mgdl_per_u = 50.0f,
    .carb_ratio_g_per_u = 12.0f,
    .max_iob_u = 5.0f,
};

static float clamp_float(float value, float lower_bound, float upper_bound)
{
    if (value < lower_bound)
    {
        return lower_bound;
    }
    if (value > upper_bound)
    {
        return upper_bound;
    }
    return value;
}

void OpenApsController_Reset(void)
{
}

void OpenApsController_Configure(const openaps_controller_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
    }
}

bool OpenApsController_DetermineBasal(const predictor_v2_input_t *input,
                                      const predictor_v2_output_t *prediction,
                                      aps_controller_output_t *output)
{
    uint16_t conservative_prediction;

    if (input == NULL || prediction == NULL || output == NULL)
    {
        return false;
    }

    memset(output, 0, sizeof(*output));
    output->action = APS_ACTION_NO_CHANGE;
    output->requested_basal_u_per_hr = g_config.basal_u_per_hr;

    conservative_prediction = prediction->pred_15m_mgdl;
    if (prediction->pred_30m_mgdl < conservative_prediction)
    {
        conservative_prediction = prediction->pred_30m_mgdl;
    }
    if (prediction->pred_60m_mgdl < conservative_prediction)
    {
        conservative_prediction = prediction->pred_60m_mgdl;
    }

    if (conservative_prediction <= g_config.suspend_threshold_mgdl)
    {
        output->action = APS_ACTION_SUSPEND_BASAL;
        output->requested_basal_u_per_hr = 0.0f;
        output->reason_flags |= OPENAPS_REASON_LOW_PRED;
        return true;
    }

    if ((prediction->risk_flags & APS_RISK_RAPID_FALL) != 0u)
    {
        output->action = APS_ACTION_REDUCE_BASAL;
        output->requested_basal_u_per_hr = g_config.basal_u_per_hr * 0.25f;
        output->reason_flags |= OPENAPS_REASON_RAPID_FALL;
        return true;
    }

    if (input->physiology.iob_u >= g_config.max_iob_u)
    {
        output->action = APS_ACTION_NO_CHANGE;
        output->requested_basal_u_per_hr = g_config.basal_u_per_hr;
        output->reason_flags |= OPENAPS_REASON_HIGH_IOB;
        return true;
    }

    if (input->cgm.sqi_pct < 60u || prediction->confidence_pct < 60u)
    {
        output->action = APS_ACTION_NO_CHANGE;
        output->requested_basal_u_per_hr = g_config.basal_u_per_hr;
        output->reason_flags |= OPENAPS_REASON_BAD_SENSOR;
        return true;
    }

    if (prediction->pred_30m_mgdl > g_config.target_high_mgdl && input->cgm.glucose_mgdl > g_config.target_high_mgdl)
    {
        float excess_glucose = (float)prediction->pred_30m_mgdl - (float)g_config.target_high_mgdl;
        float additional_basal = excess_glucose / g_config.insulin_sensitivity_mgdl_per_u;
        output->action = APS_ACTION_INCREASE_BASAL;
        output->requested_basal_u_per_hr = clamp_float(g_config.basal_u_per_hr + additional_basal,
                                                        0.0f,
                                                        g_config.max_basal_u_per_hr);
        output->reason_flags |= OPENAPS_REASON_HIGH_PRED;
        return true;
    }

    if (conservative_prediction < g_config.target_low_mgdl)
    {
        output->action = APS_ACTION_REDUCE_BASAL;
        output->requested_basal_u_per_hr = g_config.basal_u_per_hr * 0.50f;
        output->reason_flags |= OPENAPS_REASON_LOW_PRED;
        return true;
    }

    return true;
}
