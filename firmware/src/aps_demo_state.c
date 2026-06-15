#include "aps_demo_state.h"

#include <stdio.h>
#include <string.h>

#include "cob_engine.h"
#include "controller_openaps.h"
#include "iob_engine.h"
#include "predictor_v2.h"
#include "safety_supervisor.h"
#include "virtual_patient_v2.h"

enum
{
    APS_DEMO_MINUTES_PER_STEP = 5u,
};

typedef struct
{
    bool initialized;
    uint32_t last_now_s;
    uint32_t last_step_index;
    uint16_t last_bg_mgdl;
    float last_delivered_insulin_u_hr;
} aps_demo_runtime_t;

static aps_demo_runtime_t g_runtime;

static uint32_t step_index_from_now(uint32_t now_s)
{
    return now_s / (APS_DEMO_MINUTES_PER_STEP * 60u);
}

static const char *action_text_from_action(aps_action_t action)
{
    switch (action)
    {
        case APS_ACTION_NO_CHANGE:
            return "HOLD";
        case APS_ACTION_INCREASE_BASAL:
            return "INCR";
        case APS_ACTION_REDUCE_BASAL:
            return "RED";
        case APS_ACTION_SUSPEND_BASAL:
            return "STOP";
        case APS_ACTION_CORRECTION_SUGGESTION:
            return "CORR";
        default:
            return "ERR";
    }
}

static const char *safety_text_from_flags(uint32_t safety_flags)
{
    if ((safety_flags & (APS_SAFETY_REASON_PREDICTED_LOW_15M | APS_SAFETY_REASON_PREDICTED_LOW_30M)) != 0u)
    {
        return "LOW";
    }

    if ((safety_flags & APS_SAFETY_REASON_EXCESSIVE_IOB) != 0u)
    {
        return "IOB";
    }

    if ((safety_flags & APS_SAFETY_REASON_BAD_SQI) != 0u)
    {
        return "SQI";
    }

    if ((safety_flags & APS_SAFETY_REASON_STALE_CGM) != 0u)
    {
        return "STALE";
    }

    if ((safety_flags & APS_SAFETY_REASON_RAPID_FALL) != 0u)
    {
        return "RAPID";
    }

    return "NORM";
}

static void format_tenths(char *buffer, size_t buffer_size, float value)
{
    int32_t tenths;
    int32_t whole;
    int32_t fractional;

    if ((buffer == NULL) || (buffer_size == 0u))
    {
        return;
    }

    tenths = (int32_t)((value >= 0.0f) ? ((value * 10.0f) + 0.5f) : ((value * 10.0f) - 0.5f));
    whole = tenths / 10;
    fractional = tenths % 10;
    if (fractional < 0)
    {
        fractional = -fractional;
    }
    snprintf(buffer, buffer_size, "%ld.%ld", (long)whole, (long)fractional);
}

bool ApsDemoState_Init(void)
{
    VirtualPatientV2_Init();
    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    OpenApsController_Reset();

    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.initialized = true;
    g_runtime.last_bg_mgdl = 110u;
    g_runtime.last_delivered_insulin_u_hr = 0.8f;
    return true;
}

bool ApsDemoState_Step(uint32_t now_s, aps_demo_state_t *state)
{
    uint32_t step_index;
    virtual_patient_v2_state_t patient_state = {0};
    predictor_v2_input_t input = {0};
    predictor_v2_output_t prediction = {0};
    aps_controller_output_t command = {0};

    if (state == NULL)
    {
        return false;
    }

    if (!g_runtime.initialized)
    {
        if (!ApsDemoState_Init())
        {
            return false;
        }
    }

    step_index = step_index_from_now(now_s);
    if (!VirtualPatientV2_Step(now_s, g_runtime.last_delivered_insulin_u_hr, &patient_state))
    {
        return false;
    }

    input.cgm.epoch_s = now_s;
    input.cgm.age_s = 0u;
    input.cgm.glucose_mgdl = patient_state.bg_mgdl;
    if (step_index == 0u)
    {
        input.cgm.trend_mgdl_min_x100 = 0;
    }
    else
    {
        int32_t delta = (int32_t)patient_state.bg_mgdl - (int32_t)g_runtime.last_bg_mgdl;
        input.cgm.trend_mgdl_min_x100 = (int16_t)((delta * 100) / APS_DEMO_MINUTES_PER_STEP);
    }
    input.cgm.sqi_pct = 95u;
    input.cgm.sensor_flags = 0u;
    input.cgm.valid = true;

    input.physiology.iob_u = patient_state.insulin_iob_u;
    input.physiology.insulin_activity_u_per_hr = 0.0f;
    input.physiology.cob_g = patient_state.meal_cob_g;
    input.physiology.carb_absorption_g_per_hr = 0.0f;
    input.physiology.basal_u_per_hr = patient_state.basal_u_hr;
    input.physiology.insulin_30m_u = 0.0f;
    input.physiology.insulin_120m_u = 0.0f;
    input.physiology.carbs_30m_g = 0.0f;
    input.physiology.carbs_120m_g = 0.0f;
    input.physiology.minutes_since_bolus = 0u;
    input.physiology.minutes_since_meal = 0u;
    input.physiology.activity_state = (uint8_t)((patient_state.activity_factor < 1.0f) ? ACTIVITY_EXERCISE : ACTIVITY_SEDENTARY);
    input.physiology.activity_confidence_pct = 95u;
    input.physiology.motion_rms_5m = (patient_state.activity_factor < 1.0f) ? 0.65f : 0.05f;
    input.physiology.motion_rms_15m = (patient_state.activity_factor < 1.0f) ? 0.55f : 0.04f;
    input.physiology.active_minutes = (patient_state.activity_factor < 1.0f) ? 20u : 0u;
    input.physiology.post_exercise_minutes = (patient_state.activity_factor < 1.0f) ? 5u : 0u;
    input.physiology_present = true;

    if (!PredictorV2_Update(&input, &prediction))
    {
        return false;
    }

    if (!OpenApsController_DetermineBasal(&input, &prediction, &command))
    {
        return false;
    }

    if (!SafetySupervisor_Apply(now_s, &input, &prediction, &command))
    {
        return false;
    }

    state->bg_mgdl = patient_state.bg_mgdl;
    state->iob_u = patient_state.insulin_iob_u;
    state->cob_g = patient_state.meal_cob_g;
    state->action = command.action;
    state->insulin_u_hr = command.requested_basal_u_per_hr;
    state->safety_flags = command.reason_flags;
    state->action_text = action_text_from_action(command.action);
    state->safe_text = safety_text_from_flags(command.reason_flags);

    g_runtime.last_now_s = now_s;
    g_runtime.last_step_index = step_index;
    g_runtime.last_bg_mgdl = patient_state.bg_mgdl;
    g_runtime.last_delivered_insulin_u_hr = command.requested_basal_u_per_hr;
    return true;
}

bool ApsDemoState_FormatTerminal(const aps_demo_state_t *state, char *buffer, size_t buffer_size)
{
    char iob_buffer[16];
    char cob_buffer[16];
    char insulin_buffer[16];

    if ((state == NULL) || (buffer == NULL) || (buffer_size == 0u))
    {
        return false;
    }

    format_tenths(iob_buffer, sizeof(iob_buffer), state->iob_u);
    format_tenths(cob_buffer, sizeof(cob_buffer), state->cob_g);
    format_tenths(insulin_buffer, sizeof(insulin_buffer), state->insulin_u_hr);

    snprintf(buffer,
             buffer_size,
             "GLUCOSE: %3u\n"
             "INS ONBD: %s\n"
             "CARBS: %s\n"
             "ACTION: %s\n"
             "INS RATE: %s\n"
             "SAFETY: %s",
             (unsigned int)state->bg_mgdl,
             iob_buffer,
             cob_buffer,
             state->action_text,
             insulin_buffer,
             state->safe_text);
    return true;
}
