#include "activity_engine.h"

#include <math.h>
#include <string.h>

typedef struct
{
    uint32_t timestamp_s;
    float motion;
    uint16_t step_count;
    bool has_step_count;
    bool valid;
} activity_sample_t;

static activity_engine_config_t g_config =
{
    .max_samples = 32u,
};

static activity_sample_t g_samples[32u];
static uint16_t g_sample_count;
static uint16_t g_write_index;
static uint32_t g_last_update_s;
static activity_features_t g_features;
static activity_state_t g_previous_state = ACTIVITY_UNKNOWN;
static uint16_t g_low_motion_minutes;
static uint16_t g_active_minutes;
static uint16_t g_post_exercise_minutes;

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

static float calculate_motion(int16_t ax_mg, int16_t ay_mg, int16_t az_mg)
{
    float ax = (float)ax_mg;
    float ay = (float)ay_mg;
    float az = (float)az_mg;
    float magnitude = sqrtf((ax * ax) + (ay * ay) + (az * az));
    float motion = fabsf(magnitude - 1000.0f);

    return clamp_float(motion, 0.0f, 2000.0f);
}

static float window_rms(uint32_t now_s, uint32_t window_s)
{
    uint32_t index;
    float sum_squares = 0.0f;
    uint32_t count = 0u;

    for (index = 0u; index < g_sample_count; ++index)
    {
        const activity_sample_t *sample = &g_samples[index];
        if (!sample->valid || sample->timestamp_s > now_s || (now_s - sample->timestamp_s) > window_s)
        {
            continue;
        }
        sum_squares += sample->motion * sample->motion;
        count++;
    }

    if (count == 0u)
    {
        return 0.0f;
    }

    return sqrtf(sum_squares / (float)count);
}

static activity_state_t classify_state(float motion_rms_5m, float motion_rms_15m, bool has_step_count)
{
    float adjusted_motion = motion_rms_5m;

    if (has_step_count && adjusted_motion > 15.0f)
    {
        adjusted_motion += 5.0f;
    }

    if (motion_rms_15m < 12.0f && motion_rms_5m < 12.0f)
    {
        return (g_low_motion_minutes >= 20u) ? ACTIVITY_SLEEP : ACTIVITY_SEDENTARY;
    }
    if (adjusted_motion < 30.0f)
    {
        return ACTIVITY_LIGHT;
    }
    if (adjusted_motion < 65.0f)
    {
        return ACTIVITY_MODERATE;
    }
    return ACTIVITY_EXERCISE;
}

static uint8_t compute_confidence(activity_state_t state, float motion_rms_5m, float motion_rms_15m, bool has_step_count)
{
    float confidence = 50.0f;

    switch (state)
    {
        case ACTIVITY_SLEEP:
        case ACTIVITY_SEDENTARY:
            confidence = 80.0f - clamp_float(motion_rms_15m, 0.0f, 20.0f);
            break;
        case ACTIVITY_LIGHT:
            confidence = 65.0f + (20.0f - clamp_float(fabsf(motion_rms_5m - 20.0f), 0.0f, 20.0f));
            break;
        case ACTIVITY_MODERATE:
            confidence = 75.0f + (25.0f - clamp_float(fabsf(motion_rms_5m - 50.0f), 0.0f, 25.0f));
            break;
        case ACTIVITY_EXERCISE:
            confidence = 85.0f + (15.0f - clamp_float(fabsf(motion_rms_5m - 90.0f), 0.0f, 15.0f));
            break;
        case ACTIVITY_UNKNOWN:
        default:
            confidence = 40.0f;
            break;
    }

    if (has_step_count)
    {
        confidence += 5.0f;
    }

    return (uint8_t)clamp_float(confidence, 0.0f, 100.0f);
}

static uint16_t delta_minutes(uint32_t now_s, uint32_t last_s)
{
    uint32_t delta_s;

    if (last_s == 0u || now_s <= last_s)
    {
        return 1u;
    }

    delta_s = now_s - last_s;
    return (uint16_t)((delta_s + 30u) / 60u);
}

void ActivityEngine_Init(const activity_engine_config_t *config)
{
    if (config != NULL)
    {
        g_config = *config;
        if (g_config.max_samples == 0u || g_config.max_samples > (uint16_t)(sizeof(g_samples) / sizeof(g_samples[0])))
        {
            g_config.max_samples = (uint16_t)(sizeof(g_samples) / sizeof(g_samples[0]));
        }
    }

    ActivityEngine_Reset();
}

void ActivityEngine_Reset(void)
{
    memset(g_samples, 0, sizeof(g_samples));
    memset(&g_features, 0, sizeof(g_features));
    g_sample_count = 0u;
    g_write_index = 0u;
    g_last_update_s = 0u;
    g_previous_state = ACTIVITY_UNKNOWN;
    g_low_motion_minutes = 0u;
    g_active_minutes = 0u;
    g_post_exercise_minutes = 0u;
}

bool ActivityEngine_Update(uint32_t timestamp_s,
                           int16_t ax_mg,
                           int16_t ay_mg,
                           int16_t az_mg,
                           bool has_step_count,
                           uint16_t step_count,
                           activity_features_t *features)
{
    activity_sample_t sample;
    uint16_t elapsed_minutes;
    uint32_t index;
    float motion_rms_5m;
    float motion_rms_15m;
    activity_state_t state;

    if (timestamp_s == 0u || timestamp_s < g_last_update_s)
    {
        return false;
    }

    sample.timestamp_s = timestamp_s;
    sample.motion = calculate_motion(ax_mg, ay_mg, az_mg);
    sample.step_count = step_count;
    sample.has_step_count = has_step_count;
    sample.valid = true;

    g_samples[g_write_index] = sample;
    g_write_index = (uint16_t)((g_write_index + 1u) % g_config.max_samples);
    if (g_sample_count < g_config.max_samples)
    {
        g_sample_count++;
    }

    motion_rms_5m = window_rms(timestamp_s, 5u * 60u);
    motion_rms_15m = window_rms(timestamp_s, 15u * 60u);
    state = classify_state(motion_rms_5m, motion_rms_15m, has_step_count);
    elapsed_minutes = delta_minutes(timestamp_s, g_last_update_s);

    if (state == ACTIVITY_EXERCISE)
    {
        g_active_minutes = (uint16_t)((uint32_t)g_active_minutes + elapsed_minutes);
        g_post_exercise_minutes = 0u;
        g_low_motion_minutes = 0u;
    }
    else
    {
        if (g_previous_state == ACTIVITY_EXERCISE && g_post_exercise_minutes < 120u)
        {
            uint32_t post_minutes = (uint32_t)g_post_exercise_minutes + elapsed_minutes;
            g_post_exercise_minutes = (uint16_t)((post_minutes > 120u) ? 120u : post_minutes);
        }
        if (state == ACTIVITY_SEDENTARY || state == ACTIVITY_SLEEP)
        {
            uint32_t low_minutes = (uint32_t)g_low_motion_minutes + elapsed_minutes;
            g_low_motion_minutes = (uint16_t)((low_minutes > 60u) ? 60u : low_minutes);
        }
        else
        {
            g_low_motion_minutes = 0u;
            g_active_minutes = (uint16_t)((uint32_t)g_active_minutes + elapsed_minutes);
        }
    }

    g_last_update_s = timestamp_s;
    g_previous_state = state;
    g_features.state = state;
    g_features.motion_rms_5m = motion_rms_5m;
    g_features.motion_rms_15m = motion_rms_15m;
    g_features.active_minutes = g_active_minutes;
    g_features.post_exercise_minutes = g_post_exercise_minutes;
    g_features.confidence_pct = compute_confidence(state, motion_rms_5m, motion_rms_15m, has_step_count);

    if (features != NULL)
    {
        *features = g_features;
    }

    return true;
}

const activity_features_t *ActivityEngine_GetFeatures(void)
{
    return &g_features;
}
