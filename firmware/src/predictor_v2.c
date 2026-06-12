#include "predictor_v2.h"

#include <math.h>
#include <string.h>

static struct
{
    bool enabled;
    bool primed;
    predictor_v2_input_t history[APS_HISTORY_LEN];
    uint8_t write_index;
    uint8_t sample_count;
} g_predictor_state;

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

static uint16_t clamp_u16_i32(int32_t value, int32_t lower_bound, int32_t upper_bound)
{
    if (value < lower_bound)
    {
        value = lower_bound;
    }
    if (value > upper_bound)
    {
        value = upper_bound;
    }
    return (uint16_t)value;
}

static const predictor_v2_input_t *history_at_lag(uint8_t lag_samples)
{
    uint8_t historical_index;

    if (lag_samples >= g_predictor_state.sample_count || lag_samples >= APS_HISTORY_LEN)
    {
        return NULL;
    }

    historical_index = (uint8_t)((g_predictor_state.write_index + APS_HISTORY_LEN - 1u - lag_samples) % APS_HISTORY_LEN);
    return &g_predictor_state.history[historical_index];
}

static float glucose_at_lag(uint8_t lag_samples, float fallback_glucose)
{
    const predictor_v2_input_t *lagged_sample = history_at_lag(lag_samples);

    if (lagged_sample == NULL || !lagged_sample->cgm.valid || lagged_sample->cgm.glucose_mgdl == 0u)
    {
        return fallback_glucose;
    }

    return (float)lagged_sample->cgm.glucose_mgdl;
}

static float rolling_mean(uint8_t window_samples, float fallback_glucose)
{
    float sum_glucose = 0.0f;
    uint8_t used_samples = 0u;
    uint8_t sample_index;

    for (sample_index = 0u; sample_index < window_samples; ++sample_index)
    {
        const predictor_v2_input_t *historical_sample = history_at_lag(sample_index);
        if (historical_sample != NULL && historical_sample->cgm.valid)
        {
            sum_glucose += (float)historical_sample->cgm.glucose_mgdl;
            used_samples++;
        }
    }

    if (used_samples == 0u)
    {
        return fallback_glucose;
    }

    return sum_glucose / (float)used_samples;
}

static float rolling_std(uint8_t window_samples, float fallback_glucose)
{
    float mean_glucose = rolling_mean(window_samples, fallback_glucose);
    float squared_error_sum = 0.0f;
    uint8_t used_samples = 0u;
    uint8_t sample_index;

    for (sample_index = 0u; sample_index < window_samples; ++sample_index)
    {
        const predictor_v2_input_t *historical_sample = history_at_lag(sample_index);
        if (historical_sample != NULL && historical_sample->cgm.valid)
        {
            float delta = (float)historical_sample->cgm.glucose_mgdl - mean_glucose;
            squared_error_sum += delta * delta;
            used_samples++;
        }
    }

    if (used_samples < 2u)
    {
        return 0.0f;
    }

    return sqrtf(squared_error_sum / (float)(used_samples - 1u));
}

static float slope_mgdl_per_min(uint8_t lag_samples, float current_glucose)
{
    float lagged_glucose = glucose_at_lag(lag_samples, current_glucose);
    float elapsed_minutes = (float)(lag_samples * APS_TICK_SECONDS) / 60.0f;

    if (elapsed_minutes <= 0.0f)
    {
        return 0.0f;
    }

    return (current_glucose - lagged_glucose) / elapsed_minutes;
}

static float time_of_day_sin(uint32_t epoch_s)
{
    float seconds_of_day = (float)(epoch_s % 86400u);
    float angle_radians = 2.0f * 3.14159265f * (seconds_of_day / 86400.0f);
    return sinf(angle_radians);
}

static float time_of_day_cos(uint32_t epoch_s)
{
    float seconds_of_day = (float)(epoch_s % 86400u);
    float angle_radians = 2.0f * 3.14159265f * (seconds_of_day / 86400.0f);
    return cosf(angle_radians);
}

static float horizon_delta_15m(const predictor_v2_input_t *input)
{
    float current_glucose = (float)input->cgm.glucose_mgdl;
    float slope_15m = slope_mgdl_per_min(3u, current_glucose);
    float slope_30m = slope_mgdl_per_min(6u, current_glucose);
    float variance_6 = rolling_std(6u, current_glucose);
    float raw_delta = (slope_15m * 15.0f) * 0.65f
                      + (slope_30m * 15.0f) * 0.20f
                      - (input->physiology.iob_u * 2.8f)
                      + (input->physiology.cob_g * 0.28f)
                      + (input->physiology.insulin_30m_u * -0.12f)
                      + (input->physiology.carbs_30m_g * 0.22f)
                      + (variance_6 * 0.04f)
                      + (time_of_day_sin(input->cgm.epoch_s) * 2.5f);
    return raw_delta;
}

static float horizon_delta_30m(const predictor_v2_input_t *input)
{
    float current_glucose = (float)input->cgm.glucose_mgdl;
    float slope_30m = slope_mgdl_per_min(6u, current_glucose);
    float slope_60m = slope_mgdl_per_min(12u, current_glucose);
    float variance_12 = rolling_std(12u, current_glucose);
    float raw_delta = (slope_30m * 30.0f) * 0.58f
                      + (slope_60m * 30.0f) * 0.24f
                      - (input->physiology.iob_u * 4.4f)
                      + (input->physiology.cob_g * 0.36f)
                      + (input->physiology.insulin_120m_u * -0.04f)
                      + (input->physiology.carbs_120m_g * 0.11f)
                      + (variance_12 * 0.06f)
                      + (time_of_day_cos(input->cgm.epoch_s) * 1.8f);
    return raw_delta;
}

static float horizon_delta_60m(const predictor_v2_input_t *input)
{
    float current_glucose = (float)input->cgm.glucose_mgdl;
    float slope_60m = slope_mgdl_per_min(12u, current_glucose);
    float variance_12 = rolling_std(12u, current_glucose);
    float raw_delta = (slope_60m * 60.0f) * 0.74f
                      - (input->physiology.iob_u * 6.6f)
                      + (input->physiology.cob_g * 0.24f)
                      + (input->physiology.insulin_120m_u * -0.06f)
                      + (input->physiology.carbs_120m_g * 0.07f)
                      + (variance_12 * 0.08f)
                      + (time_of_day_sin(input->cgm.epoch_s) * 2.0f);
    return raw_delta;
}

static uint8_t confidence_from_input(const predictor_v2_input_t *input, float current_glucose)
{
    uint8_t confidence_pct = input->cgm.sqi_pct;
    float variability_6 = rolling_std(6u, current_glucose);

    if (!g_predictor_state.primed && confidence_pct > 60u)
    {
        confidence_pct = 60u;
    }
    if (variability_6 > 30.0f && confidence_pct > 20u)
    {
        confidence_pct = (uint8_t)(confidence_pct - 20u);
    }
    if ((input->cgm.sensor_flags & APS_SENSOR_FLAG_BAD_SQI) != 0u)
    {
        confidence_pct = (uint8_t)((confidence_pct > 30u) ? (confidence_pct - 30u) : 0u);
    }
    if ((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u)
    {
        confidence_pct = 0u;
    }
    return confidence_pct;
}

void PredictorV2_Reset(void)
{
    memset(&g_predictor_state, 0, sizeof(g_predictor_state));
}

void PredictorV2_SetEnabled(bool enabled)
{
    g_predictor_state.enabled = enabled;
}

bool PredictorV2_IsEnabled(void)
{
    return g_predictor_state.enabled;
}

bool PredictorV2_Update(const predictor_v2_input_t *input, predictor_v2_output_t *output)
{
    float current_glucose;
    float delta_15m;
    float delta_30m;
    float delta_60m;
    uint8_t confidence_pct;

    if (input == NULL || output == NULL || !input->cgm.valid)
    {
        return false;
    }

    g_predictor_state.history[g_predictor_state.write_index] = *input;
    g_predictor_state.write_index = (uint8_t)((g_predictor_state.write_index + 1u) % APS_HISTORY_LEN);
    if (g_predictor_state.sample_count < APS_HISTORY_LEN)
    {
        g_predictor_state.sample_count++;
    }
    if (g_predictor_state.sample_count >= 12u)
    {
        g_predictor_state.primed = true;
    }

    memset(output, 0, sizeof(*output));

    if (!g_predictor_state.enabled)
    {
        return false;
    }

    current_glucose = (float)input->cgm.glucose_mgdl;
    delta_15m = horizon_delta_15m(input);
    delta_30m = horizon_delta_30m(input);
    delta_60m = horizon_delta_60m(input);

    output->delta_15m_mgdl = (int16_t)lroundf(delta_15m);
    output->delta_30m_mgdl = (int16_t)lroundf(delta_30m);
    output->delta_60m_mgdl = (int16_t)lroundf(delta_60m);
    output->pred_15m_mgdl = clamp_u16_i32((int32_t)lroundf(current_glucose + delta_15m), 40, 400);
    output->pred_30m_mgdl = clamp_u16_i32((int32_t)lroundf(current_glucose + delta_30m), 40, 400);
    output->pred_60m_mgdl = clamp_u16_i32((int32_t)lroundf(current_glucose + delta_60m), 40, 400);

    confidence_pct = confidence_from_input(input, current_glucose);
    output->confidence_pct = confidence_pct;

    if (output->pred_15m_mgdl < 80u)
    {
        output->risk_flags |= APS_RISK_PRED_LOW_15M;
    }
    if (output->pred_30m_mgdl < 80u)
    {
        output->risk_flags |= APS_RISK_PRED_LOW_30M;
    }
    if (output->pred_60m_mgdl < 80u)
    {
        output->risk_flags |= APS_RISK_PRED_LOW_60M;
    }
    if (output->pred_30m_mgdl > 180u)
    {
        output->risk_flags |= APS_RISK_PRED_HIGH_30M;
    }
    if (output->pred_60m_mgdl > 180u)
    {
        output->risk_flags |= APS_RISK_PRED_HIGH_60M;
    }
    if (output->delta_15m_mgdl < -20)
    {
        output->risk_flags |= APS_RISK_RAPID_FALL;
    }
    if (output->delta_15m_mgdl > 20)
    {
        output->risk_flags |= APS_RISK_RAPID_RISE;
    }
    if (output->confidence_pct < 60u)
    {
        output->risk_flags |= APS_RISK_LOW_CONFIDENCE;
    }
    if (input->cgm.sqi_pct < 60u)
    {
        output->risk_flags |= APS_RISK_BAD_SENSOR_QUALITY;
    }
    if ((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u || !input->cgm.valid)
    {
        output->risk_flags |= APS_RISK_STALE_CGM;
    }

    return true;
}
