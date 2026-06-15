#include "virtual_patient_v2.h"

#include <math.h>
#include <string.h>

#include "cob_engine.h"
#include "iob_engine.h"

enum
{
    VP2_STEP_MINUTES = 5u,
    VP2_STEP_SECONDS = VP2_STEP_MINUTES * 60u,
    VP2_CYCLE_STEPS = 72u,
    VP2_BREAKFAST_STEP = 2u,
    VP2_EXERCISE_START_STEP = 32u,
    VP2_EXERCISE_END_STEP = 39u,
    VP2_DAWN_START_STEP = 52u,
    VP2_DAWN_END_STEP = 63u,
};

typedef struct
{
    bool initialized;
    uint32_t last_step_index;
    uint16_t current_bg_mgdl;
    float bg_fractional;
    float last_delivered_insulin_u_hr;
    float background_iob_u;
    float effective_iob_u;
    float effective_cob_g;
    float last_insulin_sensitivity;
    float last_activity_factor;
    float last_dawn_factor;
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
    if (cycle_step < VP2_DAWN_START_STEP || cycle_step > VP2_DAWN_END_STEP)
    {
        return 1.0f;
    }

    return 1.0f + (0.18f * ((float)(cycle_step - VP2_DAWN_START_STEP) /
                            (float)(VP2_DAWN_END_STEP - VP2_DAWN_START_STEP + 1u)));
}

static float activity_factor_for_step(uint32_t cycle_step)
{
    if (cycle_step >= VP2_EXERCISE_START_STEP && cycle_step <= VP2_EXERCISE_END_STEP)
    {
        return 1.35f;
    }

    if (cycle_step >= 64u)
    {
        return 0.94f;
    }

    return 1.0f;
}

static bool apply_cycle_events(uint32_t now_s, uint32_t cycle_step, bool *meal_event)
{
    *meal_event = false;

    if (cycle_step == VP2_BREAKFAST_STEP)
    {
        *meal_event = CobEngine_AddMeal(now_s, 60.0f, 210u);
        return *meal_event;
    }

    return true;
}

static float effective_insulin_sensitivity(float base_sensitivity,
                                           float activity_factor,
                                           float dawn_factor)
{
    return clamp_f32((base_sensitivity * activity_factor) / dawn_factor, 25.0f, 75.0f);
}

static void advance_patient_step(uint32_t now_s, uint32_t step_index)
{
    uint32_t cycle_step = step_index % VP2_CYCLE_STEPS;
    float dawn_factor = dawn_factor_for_step(cycle_step);
    float activity_factor = activity_factor_for_step(cycle_step);
    float basal_u_hr = clamp_f32(g_runtime.last_delivered_insulin_u_hr, 0.0f, 3.0f);
    float insulin_sensitivity = effective_insulin_sensitivity(42.0f, activity_factor, dawn_factor);
    float engine_iob_u;
    float total_iob_u;
    float cob_g;
    float target_bg;
    float delta_bg;
    bool meal_event;
    bool bolus_event = false;

    if (step_index > 0u)
    {
        (void)IobEngine_AddBasal(now_s, basal_u_hr, VP2_STEP_MINUTES);

        if (basal_u_hr > 1.35f)
        {
            float micro_bolus_u = clamp_f32((basal_u_hr - 1.1f) * 0.12f, 0.0f, 0.18f);

            if (micro_bolus_u > 0.01f)
            {
                bolus_event = IobEngine_AddBolus(now_s, micro_bolus_u);
            }
        }
    }

    (void)apply_cycle_events(now_s, cycle_step, &meal_event);
    (void)IobEngine_Update(now_s);
    (void)CobEngine_Update(now_s);

    g_runtime.background_iob_u = clamp_f32(g_runtime.background_iob_u * 0.965f, 0.0f, 1.5f);
    if (step_index == 0u)
    {
        g_runtime.background_iob_u = 0.8f;
    }

    engine_iob_u = IobEngine_GetIobU();
    total_iob_u = clamp_f32(engine_iob_u + g_runtime.background_iob_u, 0.0f, 5.0f);
    cob_g = CobEngine_GetCobG();

    target_bg = 108.0f;
    target_bg += cob_g * 1.05f;
    target_bg += (dawn_factor - 1.0f) * 34.0f;
    target_bg -= total_iob_u * insulin_sensitivity * 0.52f;
    target_bg -= (activity_factor - 1.0f) * 10.0f;
    if (meal_event)
    {
        target_bg += 10.0f;
    }

    target_bg = clamp_f32(target_bg, 70.0f, 210.0f);
    delta_bg = (target_bg - g_runtime.bg_fractional) * 0.23f;
    g_runtime.bg_fractional = clamp_f32(g_runtime.bg_fractional + delta_bg, 65.0f, 240.0f);
    g_runtime.current_bg_mgdl = clamp_u16((int32_t)(g_runtime.bg_fractional + 0.5f), 65u, 240u);
    g_runtime.effective_iob_u = total_iob_u;
    g_runtime.effective_cob_g = clamp_f32(cob_g, 0.0f, 90.0f);
    g_runtime.last_insulin_sensitivity = insulin_sensitivity;
    g_runtime.last_activity_factor = activity_factor;
    g_runtime.last_dawn_factor = dawn_factor;
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
    g_runtime.bg_fractional = 110.0f;
    g_runtime.last_delivered_insulin_u_hr = 0.8f;
    g_runtime.background_iob_u = 0.8f;
    g_runtime.effective_iob_u = 0.8f;
    g_runtime.effective_cob_g = 0.0f;
    g_runtime.last_insulin_sensitivity = 42.0f;
    g_runtime.last_activity_factor = 1.0f;
    g_runtime.last_dawn_factor = 1.0f;
}

bool VirtualPatientV2_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_v2_state_t *state)
{
    uint32_t target_step_index;
    uint32_t step_index;

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
            advance_patient_step(step_index * VP2_STEP_SECONDS, step_index);
        }
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        g_runtime.last_step_index = target_step_index;
    }
    else
    {
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
    }

    state->epoch_s = now_s;
    state->bg_mgdl = g_runtime.current_bg_mgdl;
    state->iob_u = g_runtime.effective_iob_u;
    state->cob_g = g_runtime.effective_cob_g;
    state->insulin_sensitivity = g_runtime.last_insulin_sensitivity;
    state->activity_factor = g_runtime.last_activity_factor;
    state->dawn_factor = g_runtime.last_dawn_factor;
    state->basal_u_hr = g_runtime.last_delivered_insulin_u_hr;
    state->meal_event = g_runtime.last_meal_event;
    state->bolus_event = g_runtime.last_bolus_event;
    return true;
}
