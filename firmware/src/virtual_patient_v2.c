#include "virtual_patient_v2.h"

#include <string.h>

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
    VP2_MAX_INSULIN_EVENTS = 32u,
    VP2_MAX_MEAL_EVENTS = 32u,
    VP2_INSULIN_DURATION_MIN = 300u,
};

typedef struct
{
    uint32_t timestamp_s;
    float units_u;
    uint16_t duration_min;
    bool valid;
} vp2_insulin_event_t;

typedef struct
{
    uint32_t timestamp_s;
    float carbs_g;
    uint16_t duration_min;
    bool valid;
} vp2_meal_event_t;

typedef struct
{
    bool initialized;
    uint32_t last_step_index;
    uint16_t debug_code;
    uint16_t current_bg_mgdl;
    uint16_t current_target_bg_mgdl;
    float glucose_velocity_mgdl;
    float last_delivered_insulin_u_hr;
    bool last_meal_event;
    bool last_bolus_event;
    vp2_insulin_event_t insulin_events[VP2_MAX_INSULIN_EVENTS];
    vp2_meal_event_t meal_events[VP2_MAX_MEAL_EVENTS];
    uint16_t insulin_write_index;
    uint16_t insulin_event_count;
    uint16_t meal_write_index;
    uint16_t meal_event_count;
    uint32_t last_insulin_event_s;
    uint32_t last_meal_event_s;
    uint32_t last_iob_update_s;
    uint32_t last_cob_update_s;
    float insulin_iob_u;
    float meal_cob_g;
} virtual_patient_v2_runtime_t;

static virtual_patient_v2_runtime_t g_runtime;

static void set_debug_code(uint16_t code)
{
    g_runtime.debug_code = code;
}

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

static float remaining_fraction(uint32_t age_s, uint16_t duration_min)
{
    uint32_t duration_s;

    if (duration_min == 0u)
    {
        return 0.0f;
    }

    duration_s = (uint32_t)duration_min * 60u;
    if (age_s >= duration_s)
    {
        return 0.0f;
    }

    return 1.0f - ((float)age_s / (float)duration_s);
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

static bool add_insulin_event(uint32_t timestamp_s, float units_u, uint16_t duration_min)
{
    if (timestamp_s == 0u || units_u < 0.0f)
    {
        return false;
    }
    if (timestamp_s < g_runtime.last_insulin_event_s || timestamp_s < g_runtime.last_iob_update_s)
    {
        return false;
    }

    g_runtime.insulin_events[g_runtime.insulin_write_index].timestamp_s = timestamp_s;
    g_runtime.insulin_events[g_runtime.insulin_write_index].units_u = units_u;
    g_runtime.insulin_events[g_runtime.insulin_write_index].duration_min = duration_min;
    g_runtime.insulin_events[g_runtime.insulin_write_index].valid = true;
    g_runtime.insulin_write_index = (uint16_t)((g_runtime.insulin_write_index + 1u) % VP2_MAX_INSULIN_EVENTS);
    if (g_runtime.insulin_event_count < VP2_MAX_INSULIN_EVENTS)
    {
        g_runtime.insulin_event_count++;
    }
    g_runtime.last_insulin_event_s = timestamp_s;
    return true;
}

static bool add_meal_event(uint32_t timestamp_s, float carbs_g, uint16_t duration_min)
{
    if (timestamp_s == 0u || carbs_g < 0.0f)
    {
        return false;
    }
    if (timestamp_s < g_runtime.last_meal_event_s || timestamp_s < g_runtime.last_cob_update_s)
    {
        return false;
    }

    g_runtime.meal_events[g_runtime.meal_write_index].timestamp_s = timestamp_s;
    g_runtime.meal_events[g_runtime.meal_write_index].carbs_g = carbs_g;
    g_runtime.meal_events[g_runtime.meal_write_index].duration_min = duration_min;
    g_runtime.meal_events[g_runtime.meal_write_index].valid = true;
    g_runtime.meal_write_index = (uint16_t)((g_runtime.meal_write_index + 1u) % VP2_MAX_MEAL_EVENTS);
    if (g_runtime.meal_event_count < VP2_MAX_MEAL_EVENTS)
    {
        g_runtime.meal_event_count++;
    }
    g_runtime.last_meal_event_s = timestamp_s;
    return true;
}

static void update_iob(uint32_t now_s)
{
    uint16_t write_index = 0u;
    uint16_t kept_events = 0u;
    uint32_t event_index;
    float iob_u = 0.0f;

    if (now_s < g_runtime.last_iob_update_s)
    {
        set_debug_code(171u);
        return;
    }

    set_debug_code(172u);
    g_runtime.last_iob_update_s = now_s;
    for (event_index = 0u; event_index < g_runtime.insulin_event_count; ++event_index)
    {
        const vp2_insulin_event_t *event = &g_runtime.insulin_events[event_index];
        uint32_t age_s;
        float fraction;

        if (!event->valid || event->timestamp_s > now_s)
        {
            continue;
        }

        age_s = now_s - event->timestamp_s;
        fraction = remaining_fraction(age_s, event->duration_min);
        if (fraction <= 0.0f)
        {
            continue;
        }

        g_runtime.insulin_events[write_index++] = *event;
        kept_events++;
        iob_u += event->units_u * fraction;
    }

    while (write_index < VP2_MAX_INSULIN_EVENTS)
    {
        memset(&g_runtime.insulin_events[write_index], 0, sizeof(g_runtime.insulin_events[write_index]));
        write_index++;
    }

    g_runtime.insulin_event_count = kept_events;
    g_runtime.insulin_write_index = kept_events % VP2_MAX_INSULIN_EVENTS;
    g_runtime.insulin_iob_u = clamp_f32(iob_u, 0.0f, 6.0f);
    set_debug_code(179u);
}

static void update_cob(uint32_t now_s)
{
    uint16_t write_index = 0u;
    uint16_t kept_events = 0u;
    uint32_t event_index;
    float cob_g = 0.0f;

    if (now_s < g_runtime.last_cob_update_s)
    {
        set_debug_code(181u);
        return;
    }

    set_debug_code(182u);
    g_runtime.last_cob_update_s = now_s;
    for (event_index = 0u; event_index < g_runtime.meal_event_count; ++event_index)
    {
        const vp2_meal_event_t *event = &g_runtime.meal_events[event_index];
        uint32_t age_s;
        float fraction;

        if (!event->valid || event->timestamp_s > now_s)
        {
            continue;
        }

        age_s = now_s - event->timestamp_s;
        fraction = remaining_fraction(age_s, event->duration_min);
        if (fraction <= 0.0f)
        {
            continue;
        }

        g_runtime.meal_events[write_index++] = *event;
        kept_events++;
        cob_g += event->carbs_g * fraction;
    }

    while (write_index < VP2_MAX_MEAL_EVENTS)
    {
        memset(&g_runtime.meal_events[write_index], 0, sizeof(g_runtime.meal_events[write_index]));
        write_index++;
    }

    g_runtime.meal_event_count = kept_events;
    g_runtime.meal_write_index = kept_events % VP2_MAX_MEAL_EVENTS;
    g_runtime.meal_cob_g = clamp_f32(cob_g, 0.0f, 120.0f);
    set_debug_code(189u);
}

static bool apply_step_events(uint32_t now_s, uint32_t step_index, bool *meal_event, bool *bolus_event)
{
    uint32_t cycle_step = step_index % VP2_CYCLE_STEPS;

    *meal_event = false;
    *bolus_event = false;
    set_debug_code(160u);

    if (cycle_step == VP2_BREAKFAST_STEP)
    {
        set_debug_code(161u);
        *meal_event = add_meal_event(now_s, 60.0f, 240u);
        return *meal_event;
    }

    if (cycle_step == VP2_SNACK_STEP)
    {
        set_debug_code(162u);
        *meal_event = add_meal_event(now_s, 18.0f, 90u);
        return *meal_event;
    }

    set_debug_code(169u);
    return true;
}

static void advance_patient_step(uint32_t now_s, uint32_t step_index)
{
    uint32_t cycle_step = step_index % VP2_CYCLE_STEPS;
    float dawn_factor = dawn_factor_for_step(cycle_step);
    float activity_factor = activity_factor_for_step(cycle_step);
    float insulin_sensitivity = insulin_sensitivity_for_factors(activity_factor, dawn_factor);
    float basal_u_hr = clamp_f32(g_runtime.last_delivered_insulin_u_hr, 0.0f, 3.0f);
    float carb_drive;
    float insulin_drive;
    float dawn_drive;
    float target_bg;
    bool meal_event;
    bool bolus_event;

    g_runtime.last_meal_event = false;
    g_runtime.last_bolus_event = false;
    set_debug_code(150u);

    if (step_index > 0u)
    {
        set_debug_code(151u);
        (void)add_insulin_event(now_s,
                                basal_u_hr * ((float)VP2_STEP_MINUTES / 60.0f),
                                VP2_INSULIN_DURATION_MIN);
    }

    set_debug_code(155u);
    (void)apply_step_events(now_s, step_index, &meal_event, &bolus_event);
    set_debug_code(170u);
    update_iob(now_s);
    set_debug_code(180u);
    update_cob(now_s);

    set_debug_code(190u);
    carb_drive = 0.92f * g_runtime.meal_cob_g;
    insulin_drive = 19.5f * g_runtime.insulin_iob_u * insulin_sensitivity;
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
    g_runtime.current_target_bg_mgdl = clamp_u16((int32_t)(target_bg + 0.5f), 60u, 250u);
    set_debug_code(210u);
    g_runtime.glucose_velocity_mgdl =
        clamp_f32((0.66f * g_runtime.glucose_velocity_mgdl) +
                      (0.16f * (target_bg - (float)g_runtime.current_bg_mgdl)),
                  -18.0f,
                  18.0f);
    set_debug_code(220u);
    g_runtime.current_bg_mgdl = clamp_u16((int32_t)((float)g_runtime.current_bg_mgdl +
                                                    g_runtime.glucose_velocity_mgdl + 0.5f),
                                          60u,
                                          250u);
    g_runtime.last_meal_event = meal_event;
    g_runtime.last_bolus_event = bolus_event;
    set_debug_code(230u);
}

void VirtualPatientV2_Init(void)
{
    memset(&g_runtime, 0, sizeof(g_runtime));
    g_runtime.initialized = true;
    g_runtime.debug_code = 100u;
    g_runtime.current_bg_mgdl = 110u;
    g_runtime.current_target_bg_mgdl = 110u;
    g_runtime.glucose_velocity_mgdl = 0.0f;
    g_runtime.last_delivered_insulin_u_hr = 0.8f;
    set_debug_code(101u);
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
        set_debug_code(900u);
        return false;
    }

    if (!g_runtime.initialized)
    {
        set_debug_code(102u);
        VirtualPatientV2_Init();
    }

    set_debug_code(110u);
    target_step_index = step_index_from_now(now_s);

    if ((g_runtime.last_step_index == 0u) && (now_s == 0u))
    {
        set_debug_code(120u);
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        advance_patient_step(now_s, 0u);
        g_runtime.last_step_index = target_step_index;
    }
    else if (target_step_index > g_runtime.last_step_index)
    {
        set_debug_code(130u);
        for (step_index = g_runtime.last_step_index + 1u; step_index <= target_step_index; ++step_index)
        {
            uint32_t step_now_s = step_index * VP2_STEP_SECONDS;
            set_debug_code((uint16_t)(140u + (step_index % 10u)));
            advance_patient_step(step_now_s, step_index);
        }
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
        g_runtime.last_step_index = target_step_index;
    }
    else
    {
        set_debug_code(135u);
        g_runtime.last_delivered_insulin_u_hr = clamp_f32(delivered_insulin_u_hr, 0.0f, 3.0f);
    }

    set_debug_code(240u);
    cycle_step = target_step_index % VP2_CYCLE_STEPS;
    dawn_factor = dawn_factor_for_step(cycle_step);
    activity_factor = activity_factor_for_step(cycle_step);
    insulin_sensitivity = insulin_sensitivity_for_factors(activity_factor, dawn_factor);

    set_debug_code(250u);
    state->epoch_s = now_s;
    state->step_index = target_step_index;
    state->cycle_step = (uint16_t)cycle_step;
    state->bg_mgdl = g_runtime.current_bg_mgdl;
    state->target_bg_mgdl = g_runtime.current_target_bg_mgdl;
    state->debug_code = g_runtime.debug_code;
    state->meal_cob_g = g_runtime.meal_cob_g;
    state->insulin_iob_u = g_runtime.insulin_iob_u;
    state->insulin_sensitivity = insulin_sensitivity;
    state->activity_factor = activity_factor;
    state->dawn_factor = dawn_factor;
    state->basal_u_hr = g_runtime.last_delivered_insulin_u_hr;
    state->meal_event = g_runtime.last_meal_event;
    state->bolus_event = g_runtime.last_bolus_event;
    set_debug_code(299u);
    return true;
}

uint16_t VirtualPatientV2_GetDebugCode(void)
{
    return g_runtime.debug_code;
}
