#include "virtual_patient_v1.h"

#include <math.h>
#include <string.h>

#include "cob_engine.h"
#include "iob_engine.h"

enum
{
    VP_STEP_MINUTES = 5u,
    VP_STEP_SECONDS = VP_STEP_MINUTES * 60u,
    VP_CYCLE_STEPS = 24u,
    VP_MEAL_BREAKFAST_STEP = 2u,
    VP_MEAL_SNACK_STEP = 14u,
    VP_EXERCISE_START_STEP = 17u,
    VP_EXERCISE_END_STEP = 20u,
};

typedef struct
{
    bool initialized;
    uint32_t last_step_index;
    uint16_t current_bg_mgdl;
    float last_delivered_insulin_u_hr;
} virtual_patient_runtime_t;

static virtual_patient_runtime_t g_runtime;

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
    return now_s / VP_STEP_SECONDS;
}

static float activity_factor_for_step(uint32_t cycle_step)
{
    if (cycle_step >= VP_EXERCISE_START_STEP && cycle_step <= VP_EXERCISE_END_STEP)
    {
        return 0.78f;
    }

    return 1.0f;
}

static bool apply_step_events(uint32_t now_s, uint32_t step_index, bool *meal_event)
{
    uint32_t cycle_step = step_index % VP_CYCLE_STEPS;

    *meal_event = false;

    if (cycle_step == VP_MEAL_BREAKFAST_STEP)
    {
        *meal_event = CobEngine_AddMeal(now_s, 34.0f, 180u);
        return *meal_event;
    }

    if (cycle_step == VP_MEAL_SNACK_STEP)
    {
        *meal_event = CobEngine_AddMeal(now_s, 18.0f, 120u);
        return *meal_event;
    }

    return true;
}

static void advance_patient_step(uint32_t now_s, uint32_t step_index)
{
    uint32_t cycle_step = step_index % VP_CYCLE_STEPS;
    float activity_factor = activity_factor_for_step(cycle_step);
    float basal_u_hr = clamp_f32(g_runtime.last_delivered_insulin_u_hr, 0.0f, 3.0f);
    float iob_u;
    float cob_g;
    float target_bg;
    float next_bg;
    bool meal_event;

    if (step_index > 0u)
    {
        (void)IobEngine_AddBasal(now_s, basal_u_hr, VP_STEP_MINUTES);
    }

    (void)apply_step_events(now_s, step_index, &meal_event);
    (void)IobEngine_Update(now_s);
    (void)CobEngine_Update(now_s);

    iob_u = IobEngine_GetIobU();
    cob_g = CobEngine_GetCobG();

    target_bg = 108.0f + (1.55f * cob_g) - (17.5f * iob_u * activity_factor);
    if (meal_event)
    {
        target_bg += 10.0f;
    }
    if (cycle_step >= VP_EXERCISE_START_STEP && cycle_step <= VP_EXERCISE_END_STEP)
    {
        target_bg -= 8.0f;
    }

    target_bg = clamp_f32(target_bg, 70.0f, 210.0f);
    next_bg = ((0.68f * (float)g_runtime.current_bg_mgdl) + (0.32f * target_bg));
    g_runtime.current_bg_mgdl = clamp_u16((int32_t)(next_bg + 0.5f), 60u, 250u);
}

void VirtualPatientV1_Init(void)
{
    IobEngine_Init(NULL);
    CobEngine_Init(NULL);
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.initialized = true;
    g_runtime.current_bg_mgdl = 110u;
    g_runtime.last_delivered_insulin_u_hr = 0.8f;
}

bool VirtualPatientV1_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_state_t *state)
{
    uint32_t target_step_index;
    uint32_t step_index;
    uint32_t cycle_step;

    if (state == NULL)
    {
        return false;
    }

    if (!g_runtime.initialized)
    {
        VirtualPatientV1_Init();
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
            uint32_t step_now_s = step_index * VP_STEP_SECONDS;
            advance_patient_step(step_now_s, step_index);
        }
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        g_runtime.last_step_index = target_step_index;
    }
    else
    {
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
    }

    cycle_step = target_step_index % VP_CYCLE_STEPS;

    state->epoch_s = now_s;
    state->bg_mgdl = g_runtime.current_bg_mgdl;
    state->meal_cob_g = CobEngine_GetCobG();
    state->insulin_iob_u = IobEngine_GetIobU();
    state->activity_factor = activity_factor_for_step(cycle_step);
    state->basal_u_hr = g_runtime.last_delivered_insulin_u_hr;
    state->meal_event = (cycle_step == VP_MEAL_BREAKFAST_STEP) || (cycle_step == VP_MEAL_SNACK_STEP);
    state->bolus_event = false;
    return true;
}
