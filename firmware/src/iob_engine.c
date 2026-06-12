#include "iob_engine.h"

#include <math.h>
#include <string.h>

typedef struct
{
    uint32_t timestamp_s;
    float units_u;
    uint16_t duration_min;
    bool valid;
} iob_event_t;

static iob_engine_config_t g_config =
{
    .bolus_duration_min = 300u,
    .basal_duration_min = 300u,
    .max_events = 32u,
};

static iob_event_t g_events[32u];
static uint16_t g_write_index;
static uint16_t g_event_count;
static uint32_t g_last_update_s;
static uint32_t g_last_event_s;
static float g_iob_u;

static float clamp_nonnegative(float value)
{
    return value < 0.0f ? 0.0f : value;
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

static bool add_event(uint32_t timestamp_s, float units_u, uint16_t duration_min)
{
    if (timestamp_s == 0u || units_u < 0.0f || isnan(units_u) || isinf(units_u))
    {
        return false;
    }
    if (timestamp_s < g_last_event_s || timestamp_s < g_last_update_s)
    {
        return false;
    }

    g_events[g_write_index].timestamp_s = timestamp_s;
    g_events[g_write_index].units_u = units_u;
    g_events[g_write_index].duration_min = duration_min;
    g_events[g_write_index].valid = true;
    g_write_index = (uint16_t)((g_write_index + 1u) % g_config.max_events);
    if (g_event_count < g_config.max_events)
    {
        g_event_count++;
    }
    g_last_event_s = timestamp_s;
    return true;
}

void IobEngine_Init(const iob_engine_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
        if (g_config.max_events == 0u || g_config.max_events > (uint16_t)(sizeof(g_events) / sizeof(g_events[0])))
        {
            g_config.max_events = (uint16_t)(sizeof(g_events) / sizeof(g_events[0]));
        }
        if (g_config.bolus_duration_min == 0u)
        {
            g_config.bolus_duration_min = 300u;
        }
        if (g_config.basal_duration_min == 0u)
        {
            g_config.basal_duration_min = 300u;
        }
    }
    IobEngine_Reset();
}

void IobEngine_Reset(void)
{
    memset(g_events, 0, sizeof(g_events));
    g_write_index = 0u;
    g_event_count = 0u;
    g_last_update_s = 0u;
    g_last_event_s = 0u;
    g_iob_u = 0.0f;
}

bool IobEngine_AddBolus(uint32_t timestamp_s, float bolus_u)
{
    return add_event(timestamp_s, bolus_u, g_config.bolus_duration_min);
}

bool IobEngine_AddBasal(uint32_t timestamp_s, float basal_u_per_hr, uint16_t duration_min)
{
    float units_u;

    if (duration_min == 0u)
    {
        duration_min = g_config.basal_duration_min;
    }
    units_u = basal_u_per_hr * ((float)duration_min / 60.0f);
    return add_event(timestamp_s, units_u, duration_min);
}

bool IobEngine_Update(uint32_t now_s)
{
    uint32_t event_index;
    uint16_t write_index;
    uint16_t kept_events = 0u;
    float iob_u = 0.0f;

    if (now_s < g_last_update_s)
    {
        return false;
    }

    g_last_update_s = now_s;
    write_index = 0u;
    for (event_index = 0u; event_index < g_event_count; ++event_index)
    {
        const iob_event_t *event = &g_events[event_index];
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

        g_events[write_index++] = *event;
        kept_events++;
        iob_u += event->units_u * fraction;
    }

    while (write_index < g_config.max_events)
    {
        memset(&g_events[write_index], 0, sizeof(g_events[write_index]));
        write_index++;
    }

    g_event_count = kept_events;
    g_write_index = kept_events % g_config.max_events;
    g_iob_u = clamp_nonnegative(iob_u);
    return true;
}

float IobEngine_GetIobU(void)
{
    return g_iob_u;
}
