#include "virtual_patient_v2.h"

#include <math.h>
#include <string.h>

#include "cob_engine.h"
#include "iob_engine.h"

enum
{
    VP2_STEP_MINUTES = 5u,
    VP2_STEP_SECONDS = VP2_STEP_MINUTES * 60u,
    VP2_CYCLE_STEPS = 48u,
    VP2_BREAKFAST_STEP = 4u,
    VP2_SNACK_STEP = 26u,
    VP2_EXERCISE_START_STEP = 24u,
    VP2_EXERCISE_END_STEP = 30u,
    VP2_DAWN_START_STEP = 38u,
    VP2_DAWN_END_STEP = 45u,
};

typedef struct
{
    bool initialized;
    uint32_t last_step_index;
    uint16_t current_bg_mgdl;
    float glucose_velocity_mgdl;
    float last_delivered_insulin_u_hr;
    bool last_meal_event;
    bool last_bolus_event;
} virtual_patient_v2_runtime_t;

static virtual_patient_v2_runtime_t g_runtime;

static float clamp_f32(float value, float minimum, float maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}

static uint16_t clamp_u16(int32_t value, uint16_t minimum, uint16_t maximum)
{
    if (value < (int32_t)minimum)
    {
        return minimum;
    }

    if (value > (int32_t)maximum)
    {
        return maximum;
    }

    return (uint16_t)value;
}

static uint32_t step_index_from_now(uint32_t now_s)
{
    return now_s / VP2_STEP_SECONDS;
}

static float dawn_factor_for_step(uint32_t cycle_step)
{
    if (cycle_step >= VP2_DAWN_START_STEP && cycle_step <= VP2_DAWN_END_STEP)
    {
        float progress = (float)(cycle_step - VP2_DAWN_START_STEP) /
                         (float)((VP2_DAWN_END_STEP - VP2_DAWN_START_STEP) + 1u);
        return 1.08f + (0.12f * progress);
    }

    return 1.0f;
}

static float activity_factor_for_step(uint32_t cycle_step)
{
    if (cycle_step >= VP2_EXERCISE_START_STEP && cycle_step <= VP2_EXERCISE_END_STEP)
    {
        float progress = (float)(cycle_step - VP2_EXERCISE_START_STEP) /
                         (float)((VP2_EXERCISE_END_STEP - VP2_EXERCISE_START_STEP) + 1u);
        return 0.82f - (0.08f * progress);
    }

    return 1.0f;
}

static float insulin_sensitivity_for_factors(float activity_factor, float dawn_factor)
{
    float sensitivity = 1.0f;

    sensitivity += (1.0f - activity_factor) * 0.95f;
    sensitivity -= (dawn_factor - 1.0f) * 0.55f;
    return clamp_f32(sensitivity, 0.75f, 1.45f);
}

static bool apply_step_events(uint32_t now_s, uint32_t step_index, bool *meal_event, bool *bolus_event)
{
    uint32_t cycle_step = step_index % VP2_CYCLE_STEPS;

    *meal_event = false;
    *bolus_event = false;

    if (cycle_step == VP2_BREAKFAST_STEP)
    {
        *meal_event = CobEngine_AddMeal(now_s, 60.0f, 240u);
        return *meal_event;
    }

    if (cycle_step == VP2_SNACK_STEP)
    {
        *meal_event = CobEngine_AddMeal(now_s, 18.0f, 90u);
        return *meal_event;
    }

    return true;
}

static void advance_patient_step(uint32_t now_s, uint32_t step_index)
{
    uint32_t cycle_step = step_index % VP2_CYCLE_STEPS;
    float dawn_factor = dawn_factor_for_step(cycle_step);
    float activity_factor = activity_factor_for_step(cycle_step);
    float insulin_sensitivity = insulin_sensitivity_for_factors(activity_factor, dawn_factor);
    float basal_u_hr = clamp_f32(g_runtime.last_delivered_insulin_u_hr, 0.0f, 3.0f);
    float iob_u;
    float cob_g;
    float carb_drive;
    float insulin_drive;
    float dawn_drive;
    float target_bg;
    bool meal_event;
    bool bolus_event;

    g_runtime.last_meal_event = false;
    g_runtime.last_bolus_event = false;

    if (step_index > 0u)
    {
        (void)IobEngine_AddBasal(now_s, basal_u_hr, VP2_STEP_MINUTES);
    }

    (void)apply_step_events(now_s, step_index, &meal_event, &bolus_event);
    (void)IobEngine_Update(now_s);
    (void)CobEngine_Update(now_s);

    iob_u = IobEngine_GetIobU();
    cob_g = CobEngine_GetCobG();
    carb_drive = 0.92f * cob_g;
    insulin_drive = 19.5f * iob_u * insulin_sensitivity;
    dawn_drive = 18.0f * (dawn_factor - 1.0f);

    target_bg = 108.0f + carb_drive - insulin_drive + dawn_drive;
    if (meal_event)
    {
        target_bg += 10.0f;
    }
    if (activity_factor < 1.0f)
    {
        target_bg -= 6.0f;
    }

    target_bg = clamp_f32(target_bg, 75.0f, 215.0f);
    g_runtime.glucose_velocity_mgdl =
        clamp_f32((0.66f * g_runtime.glucose_velocity_mgdl) +
                      (0.16f * (target_bg - (float)g_runtime.current_bg_mgdl)),
                  -18.0f,
                  18.0f);
    g_runtime.current_bg_mgdl = clamp_u16((int32_t)((float)g_runtime.current_bg_mgdl +
                                                    g_runtime.glucose_velocity_mgdl + 0.5f),
                                          60u,
                                          250u);
    g_runtime.last_meal_event = meal_event;
    g_runtime.last_bolus_event = bolus_event;
}

void VirtualPatientV2_Init(void)
{
    IobEngine_Init(NULL);
    CobEngine_Init(NULL);
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.initialized = true;
    g_runtime.current_bg_mgdl = 110u;
    g_runtime.glucose_velocity_mgdl = 0.0f;
    g_runtime.last_delivered_insulin_u_hr = 0.8f;
}

bool VirtualPatientV2_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_v2_state_t *state)
{
    uint32_t target_step_index;
    uint32_t step_index;
    uint32_t cycle_step;
    float dawn_factor;
    float activity_factor;
    float insulin_sensitivity;

    if (state == NULL)
    {
        return false;
    }

    if (!g_runtime.initialized)
    {
        VirtualPatientV2_Init();
    }

    target_step_index = step_index_from_now(now_s);

    if ((g_runtime.last_step_index == 0u) && (now_s == 0u))
    {
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        advance_patient_step(now_s, 0u);
    }
    else if (target_step_index > g_runtime.last_step_index)
    {
        for (step_index = g_runtime.last_step_index + 1u; step_index <= target_step_index; ++step_index)
        {
            uint32_t step_now_s = step_index * VP2_STEP_SECONDS;
            advance_patient_step(step_now_s, step_index);
        }
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        g_runtime.last_step_index = target_step_index;
    }
    else
    {
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
    }

    cycle_step = target_step_index % VP2_CYCLE_STEPS;
    dawn_factor = dawn_factor_for_step(cycle_step);
    activity_factor = activity_factor_for_step(cycle_step);
    insulin_sensitivity = insulin_sensitivity_for_factors(activity_factor, dawn_factor);

    state->epoch_s = now_s;
    state->bg_mgdl = g_runtime.current_bg_mgdl;
    state->meal_cob_g = CobEngine_GetCobG();
    state->insulin_iob_u = IobEngine_GetIobU();
    state->insulin_sensitivity = insulin_sensitivity;
    state->activity_factor = activity_factor;
    state->dawn_factor = dawn_factor;
    state->basal_u_hr = g_runtime.last_delivered_insulin_u_hr;
    state->meal_event = g_runtime.last_meal_event;
    state->bolus_event = g_runtime.last_bolus_event;
    return true;
}
