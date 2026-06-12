#include "cob_engine.h"

#include <math.h>
#include <string.h>

typedef struct
{
    uint32_t timestamp_s;
    float carbs_g;
    uint16_t duration_min;
    bool valid;
} cob_event_t;

static cob_engine_config_t g_config =
{
    .meal_duration_min = 180u,
    .max_meals = 32u,
};

static cob_event_t g_meals[32u];
static uint16_t g_write_index;
static uint16_t g_meal_count;
static uint32_t g_last_update_s;
static uint32_t g_last_meal_s;
static float g_cob_g;

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

static bool add_meal(uint32_t timestamp_s, float carbs_g, uint16_t duration_min)
{
    if (timestamp_s == 0u || carbs_g < 0.0f || isnan(carbs_g) || isinf(carbs_g))
    {
        return false;
    }
    if (timestamp_s < g_last_meal_s || timestamp_s < g_last_update_s)
    {
        return false;
    }

    g_meals[g_write_index].timestamp_s = timestamp_s;
    g_meals[g_write_index].carbs_g = carbs_g;
    g_meals[g_write_index].duration_min = duration_min;
    g_meals[g_write_index].valid = true;
    g_write_index = (uint16_t)((g_write_index + 1u) % g_config.max_meals);
    if (g_meal_count < g_config.max_meals)
    {
        g_meal_count++;
    }
    g_last_meal_s = timestamp_s;
    return true;
}

void CobEngine_Init(const cob_engine_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
        if (g_config.max_meals == 0u || g_config.max_meals > (uint16_t)(sizeof(g_meals) / sizeof(g_meals[0])))
        {
            g_config.max_meals = (uint16_t)(sizeof(g_meals) / sizeof(g_meals[0]));
        }
        if (g_config.meal_duration_min == 0u)
        {
            g_config.meal_duration_min = 180u;
        }
    }
    CobEngine_Reset();
}

void CobEngine_Reset(void)
{
    memset(g_meals, 0, sizeof(g_meals));
    g_write_index = 0u;
    g_meal_count = 0u;
    g_last_update_s = 0u;
    g_last_meal_s = 0u;
    g_cob_g = 0.0f;
}

bool CobEngine_AddMeal(uint32_t timestamp_s, float carbs_g, uint16_t absorption_min)
{
    if (absorption_min == 0u)
    {
        absorption_min = g_config.meal_duration_min;
    }
    return add_meal(timestamp_s, carbs_g, absorption_min);
}

bool CobEngine_Update(uint32_t now_s)
{
    uint32_t event_index;
    uint16_t write_index = 0u;
    uint16_t kept_events = 0u;
    float cob_g = 0.0f;

    if (now_s < g_last_update_s)
    {
        return false;
    }

    g_last_update_s = now_s;
    for (event_index = 0u; event_index < g_meal_count; ++event_index)
    {
        const cob_event_t *event = &g_meals[event_index];
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

        g_meals[write_index++] = *event;
        kept_events++;
        cob_g += event->carbs_g * fraction;
    }

    while (write_index < g_config.max_meals)
    {
        memset(&g_meals[write_index], 0, sizeof(g_meals[write_index]));
        write_index++;
    }

    g_meal_count = kept_events;
    g_write_index = kept_events % g_config.max_meals;
    g_cob_g = clamp_nonnegative(cob_g);
    return true;
}

float CobEngine_GetCobG(void)
{
    return g_cob_g;
}
