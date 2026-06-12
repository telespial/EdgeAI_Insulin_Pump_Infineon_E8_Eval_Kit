#include "safety_supervisor.h"

#include <stddef.h>

static aps_safety_config_t g_config =
{
    .min_valid_glucose_mgdl = 40u,
    .max_valid_glucose_mgdl = 400u,
    .low_prediction_threshold_mgdl = 80u,
    .min_sqi_for_increase_pct = 60u,
    .max_cgm_age_s = 600u,
    .max_basal_u_per_hr = 2.0f,
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

static void mark_blocked(aps_controller_output_t *command, aps_action_t original_action, float original_basal, float original_correction)
{
    if (command->action != original_action ||
        command->requested_basal_u_per_hr != original_basal ||
        command->suggested_correction_u != original_correction)
    {
        command->reason_flags |= APS_SAFETY_REASON_CONTROLLER_BLOCKED;
    }
}

void SafetySupervisor_Configure(const aps_safety_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
    }
}

bool SafetySupervisor_Apply(uint32_t now_s,
                            const predictor_v2_input_t *input,
                            const predictor_v2_output_t *prediction,
                            aps_controller_output_t *command)
{
    bool stale_cgm;
    bool impossible_glucose;
    bool low_prediction_15m;
    bool low_prediction_30m;
    bool low_confidence;
    bool bad_sensor_quality;
    bool excessive_iob;
    bool missing_physiology;
    aps_action_t original_action;
    float original_basal;
    float original_correction;

    if (input == NULL || prediction == NULL || command == NULL)
    {
        return false;
    }

    original_action = command->action;
    original_basal = command->requested_basal_u_per_hr;
    original_correction = command->suggested_correction_u;

    stale_cgm = (input->cgm.epoch_s == 0u) || ((now_s - input->cgm.epoch_s) > g_config.max_cgm_age_s);
    impossible_glucose = (input->cgm.glucose_mgdl < g_config.min_valid_glucose_mgdl) ||
                         (input->cgm.glucose_mgdl > g_config.max_valid_glucose_mgdl);
    low_prediction_15m = prediction->pred_15m_mgdl <= g_config.low_prediction_threshold_mgdl;
    low_prediction_30m = prediction->pred_30m_mgdl <= g_config.low_prediction_threshold_mgdl;
    low_confidence = prediction->confidence_pct < g_config.min_sqi_for_increase_pct;
    bad_sensor_quality = (input->cgm.sqi_pct < g_config.min_sqi_for_increase_pct) ||
                         ((prediction->risk_flags & APS_RISK_BAD_SENSOR_QUALITY) != 0u);
    excessive_iob = input->physiology.iob_u >= g_config.max_iob_u;
    missing_physiology = !input->physiology_present;

    if (stale_cgm || impossible_glucose)
    {
        command->action = APS_ACTION_NO_CHANGE;
        command->requested_basal_u_per_hr = 0.0f;
        command->suggested_correction_u = 0.0f;
        if (stale_cgm)
        {
            command->reason_flags |= APS_SAFETY_REASON_STALE_CGM;
        }
        if (impossible_glucose)
        {
            command->reason_flags |= APS_SAFETY_REASON_IMPOSSIBLE_GLUCOSE;
        }
        mark_blocked(command, original_action, original_basal, original_correction);
        return true;
    }

    if (low_prediction_15m)
    {
        command->reason_flags |= APS_SAFETY_REASON_PREDICTED_LOW_15M;
    }
    if (low_prediction_30m)
    {
        command->reason_flags |= APS_SAFETY_REASON_PREDICTED_LOW_30M;
    }
    if ((prediction->risk_flags & APS_RISK_RAPID_FALL) != 0u)
    {
        command->reason_flags |= APS_SAFETY_REASON_RAPID_FALL;
    }
    if (low_confidence)
    {
        command->reason_flags |= APS_SAFETY_REASON_LOW_CONFIDENCE;
    }
    if (bad_sensor_quality)
    {
        command->reason_flags |= APS_SAFETY_REASON_BAD_SQI;
        if (command->action == APS_ACTION_INCREASE_BASAL || command->action == APS_ACTION_CORRECTION_SUGGESTION)
        {
            command->action = APS_ACTION_NO_CHANGE;
            command->requested_basal_u_per_hr = 0.0f;
            command->suggested_correction_u = 0.0f;
        }
    }

    if (low_prediction_15m || low_prediction_30m || ((prediction->risk_flags & APS_RISK_RAPID_FALL) != 0u))
    {
        command->action = APS_ACTION_SUSPEND_BASAL;
        command->requested_basal_u_per_hr = 0.0f;
        command->suggested_correction_u = 0.0f;
    }

    if (low_confidence && (command->action == APS_ACTION_INCREASE_BASAL || command->action == APS_ACTION_CORRECTION_SUGGESTION))
    {
        command->action = APS_ACTION_NO_CHANGE;
        command->requested_basal_u_per_hr = 0.0f;
        command->suggested_correction_u = 0.0f;
    }

    if (excessive_iob)
    {
        command->reason_flags |= APS_SAFETY_REASON_EXCESSIVE_IOB;
        if (command->action == APS_ACTION_INCREASE_BASAL || command->action == APS_ACTION_CORRECTION_SUGGESTION)
        {
            command->action = APS_ACTION_NO_CHANGE;
            command->requested_basal_u_per_hr = 0.0f;
            command->suggested_correction_u = 0.0f;
        }
    }

    if (missing_physiology)
    {
        command->reason_flags |= APS_SAFETY_REASON_MISSING_PHYSIOLOGY;
        if (command->action == APS_ACTION_INCREASE_BASAL || command->action == APS_ACTION_CORRECTION_SUGGESTION)
        {
            command->action = APS_ACTION_NO_CHANGE;
            command->requested_basal_u_per_hr = 0.0f;
            command->suggested_correction_u = 0.0f;
        }
    }

    if ((command->reason_flags & (APS_SAFETY_REASON_STALE_CGM |
                                  APS_SAFETY_REASON_BAD_SQI |
                                  APS_SAFETY_REASON_PREDICTED_LOW_15M |
                                  APS_SAFETY_REASON_PREDICTED_LOW_30M |
                                  APS_SAFETY_REASON_RAPID_FALL |
                                  APS_SAFETY_REASON_EXCESSIVE_IOB |
                                  APS_SAFETY_REASON_IMPOSSIBLE_GLUCOSE |
                                  APS_SAFETY_REASON_LOW_CONFIDENCE |
                                  APS_SAFETY_REASON_MISSING_PHYSIOLOGY)) != 0u)
    {
        mark_blocked(command, original_action, original_basal, original_correction);
    }

    command->requested_basal_u_per_hr = clamp_float(command->requested_basal_u_per_hr,
                                                    0.0f,
                                                    g_config.max_basal_u_per_hr);
    return true;
}
