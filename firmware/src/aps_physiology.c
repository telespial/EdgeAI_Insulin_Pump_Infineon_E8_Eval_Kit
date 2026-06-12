#include "aps_physiology.h"

#include <string.h>

static aps_physiology_config_t g_config =
{
    .insulin_action_hours = 5.0f,
    .carb_action_hours = 3.0f,
    .default_basal_u_per_hr = 0.8f,
};

static aps_event_t g_events[APS_EVENT_HISTORY_LEN];
static uint16_t g_write_index;
static uint16_t g_event_count;

static float clamp_min(float value, float lower_bound)
{
    return value < lower_bound ? lower_bound : value;
}

static float remaining_fraction(float age_hours, float action_hours)
{
    if (age_hours < 0.0f || action_hours <= 0.0f || age_hours >= action_hours)
    {
        return 0.0f;
    }
    return 1.0f - (age_hours / action_hours);
}

void ApsPhysiology_Reset(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_write_index = 0u;
    g_event_count = 0u;
}

void ApsPhysiology_Configure(const aps_physiology_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
    }
}

void ApsPhysiology_AddEvent(const aps_event_t *event)
{
    if (event == NULL)
    {
        return;
    }

    g_events[g_write_index] = *event;
    g_write_index = (uint16_t)((g_write_index + 1u) % APS_EVENT_HISTORY_LEN);
    if (g_event_count < APS_EVENT_HISTORY_LEN)
    {
        g_event_count++;
    }
}

void ApsPhysiology_Update(uint32_t epoch_s, aps_physiology_state_t *state)
{
    uint16_t event_index;

    if (state == NULL)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->basal_u_per_hr = g_config.default_basal_u_per_hr;
    state->minutes_since_bolus = 0xFFFFu;
    state->minutes_since_meal = 0xFFFFu;

    for (event_index = 0u; event_index < g_event_count; ++event_index)
    {
        const aps_event_t *event = &g_events[event_index];
        uint32_t age_seconds;
        float age_hours;
        float age_minutes;
        float insulin_units;
        float carb_grams;
        float insulin_remaining;
        float carb_remaining;

        if (event->epoch_s == 0u || event->epoch_s > epoch_s)
        {
            continue;
        }

        age_seconds = epoch_s - event->epoch_s;
        age_hours = (float)age_seconds / 3600.0f;
        age_minutes = (float)age_seconds / 60.0f;

        insulin_units = event->bolus_u + event->basal_u;
        carb_grams = event->carbs_g;

        if (insulin_units > 0.0f)
        {
            insulin_remaining = remaining_fraction(age_hours, g_config.insulin_action_hours);
            state->iob_u += insulin_units * insulin_remaining;
            state->insulin_activity_u_per_hr += insulin_units / g_config.insulin_action_hours;
            if (age_seconds <= 1800u)
            {
                state->insulin_30m_u += insulin_units;
            }
            if (age_seconds <= 7200u)
            {
                state->insulin_120m_u += insulin_units;
            }
            if (event->bolus_u > 0.0f && age_minutes < (float)state->minutes_since_bolus)
            {
                state->minutes_since_bolus = (uint16_t)age_minutes;
            }
        }

        if (carb_grams > 0.0f)
        {
            carb_remaining = remaining_fraction(age_hours, g_config.carb_action_hours);
            state->cob_g += carb_grams * carb_remaining;
            state->carb_absorption_g_per_hr += carb_grams / g_config.carb_action_hours;
            if (age_seconds <= 1800u)
            {
                state->carbs_30m_g += carb_grams;
            }
            if (age_seconds <= 7200u)
            {
                state->carbs_120m_g += carb_grams;
            }
            if (age_minutes < (float)state->minutes_since_meal)
            {
                state->minutes_since_meal = (uint16_t)age_minutes;
            }
        }
    }

    state->iob_u = clamp_min(state->iob_u, 0.0f);
    state->cob_g = clamp_min(state->cob_g, 0.0f);
}
