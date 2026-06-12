#include "predictor_v2.h"

#include <math.h>
#include <string.h>

enum
{
    PREDICTOR_V2_STALE_CGM_THRESHOLD_S = 600u,
    PREDICTOR_V2_MIN_PRED_MGDL = 40u,
    PREDICTOR_V2_MAX_PRED_MGDL = 400u,
    PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD = 60u,
};

static struct
{
    bool enabled;
    bool primed;
    predictor_v2_input_t history[APS_HISTORY_LEN];
    uint8_t write_index;
    uint8_t sample_count;
    const predictor_v2_generated_bundle_t *bundle_override;
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

static int16_t clamp_i16_i32(int32_t value, int32_t lower_bound, int32_t upper_bound)
{
    if (value < lower_bound)
    {
        value = lower_bound;
    }
    if (value > upper_bound)
    {
        value = upper_bound;
    }
    return (int16_t)value;
}

static float sanitize_feature_float(float value, float lower_bound, float upper_bound, float fallback_value, bool *valid)
{
    if (valid != NULL)
    {
        *valid = true;
    }

    if (!isfinite(value))
    {
        if (valid != NULL)
        {
            *valid = false;
        }
        return fallback_value;
    }

    if (value < lower_bound)
    {
        if (valid != NULL)
        {
            *valid = false;
        }
        return lower_bound;
    }
    if (value > upper_bound)
    {
        if (valid != NULL)
        {
            *valid = false;
        }
        return upper_bound;
    }

    return value;
}

static float sanitize_activity_state_feature(uint8_t value, bool *valid)
{
    if (valid != NULL)
    {
        *valid = (value <= (uint8_t)ACTIVITY_SLEEP);
    }

    if (value > (uint8_t)ACTIVITY_SLEEP)
    {
        return (float)ACTIVITY_UNKNOWN;
    }

    return (float)value;
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

static bool sample_is_valid(const predictor_v2_input_t *sample)
{
    return sample != NULL && sample->cgm.valid && sample->cgm.glucose_mgdl >= PREDICTOR_V2_MIN_PRED_MGDL &&
           sample->cgm.glucose_mgdl <= PREDICTOR_V2_MAX_PRED_MGDL;
}

static float glucose_at_lag(uint8_t lag_samples, float fallback_glucose, bool *valid)
{
    const predictor_v2_input_t *lagged_sample = history_at_lag(lag_samples);

    if (valid != NULL)
    {
        *valid = false;
    }

    if (!sample_is_valid(lagged_sample))
    {
        return fallback_glucose;
    }

    if (valid != NULL)
    {
        *valid = true;
    }

    return (float)lagged_sample->cgm.glucose_mgdl;
}

static float rolling_mean(uint8_t window_samples, float fallback_glucose, uint32_t *valid_count)
{
    float sum_glucose = 0.0f;
    uint8_t used_samples = 0u;
    uint8_t sample_index;

    for (sample_index = 0u; sample_index < window_samples; ++sample_index)
    {
        const predictor_v2_input_t *historical_sample = history_at_lag(sample_index);
        if (sample_is_valid(historical_sample))
        {
            sum_glucose += (float)historical_sample->cgm.glucose_mgdl;
            used_samples++;
        }
    }

    if (valid_count != NULL)
    {
        *valid_count = used_samples;
    }

    if (used_samples == 0u)
    {
        return fallback_glucose;
    }

    return sum_glucose / (float)used_samples;
}

static float rolling_variance(uint8_t window_samples, float fallback_glucose, uint32_t *valid_count)
{
    float mean_glucose = rolling_mean(window_samples, fallback_glucose, valid_count);
    float squared_error_sum = 0.0f;
    uint8_t used_samples = 0u;
    uint8_t sample_index;

    for (sample_index = 0u; sample_index < window_samples; ++sample_index)
    {
        const predictor_v2_input_t *historical_sample = history_at_lag(sample_index);
        if (sample_is_valid(historical_sample))
        {
            float delta = (float)historical_sample->cgm.glucose_mgdl - mean_glucose;
            squared_error_sum += delta * delta;
            used_samples++;
        }
    }

    if (valid_count != NULL)
    {
        *valid_count = used_samples;
    }

    if (used_samples < 2u)
    {
        return 0.0f;
    }

    return squared_error_sum / (float)(used_samples - 1u);
}

static float rolling_stddev(uint8_t window_samples, float fallback_glucose, uint32_t *valid_count)
{
    return sqrtf(rolling_variance(window_samples, fallback_glucose, valid_count));
}

static float slope_mgdl_per_min(uint8_t lag_samples, float current_glucose, bool *valid)
{
    float lagged_glucose = glucose_at_lag(lag_samples, current_glucose, valid);
    float elapsed_minutes = (float)(lag_samples * APS_TICK_SECONDS) / 60.0f;

    if (elapsed_minutes <= 0.0f || (valid != NULL && !*valid))
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

static float current_trend_mgdl_min(const predictor_v2_input_t *input)
{
    if (input == NULL)
    {
        return 0.0f;
    }
    return (float)input->cgm.trend_mgdl_min_x100 / 100.0f;
}

static void feature_set(predictor_v2_feature_vector_t *features,
                        predictor_v2_feature_index_t index,
                        float value,
                        bool valid)
{
    if (features == NULL)
    {
        return;
    }

    features->values[index] = value;
    if (valid)
    {
        features->valid_mask |= (1ull << (uint32_t)index);
    }
}

static bool model_is_valid(const predictor_v2_generated_model_t *model)
{
    size_t index;

    if (model == NULL || !model->valid)
    {
        return false;
    }

    for (index = 0u; index < PREDICTOR_V2_FEATURE_COUNT; ++index)
    {
        if (!isfinite(model->feature_mean[index]) ||
            !isfinite(model->feature_scale[index]) ||
            !isfinite(model->feature_median[index]) ||
            !isfinite(model->coefficients[index]) ||
            model->feature_scale[index] <= 0.0f)
        {
            return false;
        }
    }

    return isfinite(model->intercept);
}

static const predictor_v2_generated_model_t *select_horizon_model(const predictor_v2_generated_bundle_t *bundle,
                                                                  predictor_v2_horizon_t horizon)
{
    if (bundle == NULL)
    {
        return NULL;
    }

    switch (horizon)
    {
        case PREDICTOR_V2_HORIZON_15M:
            return bundle->horizon_15m;
        case PREDICTOR_V2_HORIZON_30M:
            return bundle->horizon_30m;
        case PREDICTOR_V2_HORIZON_60M:
            return bundle->horizon_60m;
        default:
            return NULL;
    }
}

static uint8_t estimate_confidence_pct(const predictor_v2_input_t *input, const predictor_v2_feature_vector_t *features, bool used_fallback)
{
    uint8_t confidence_pct = 0u;
    float rolling_stddev_6 = 0.0f;
    float volatility_score = 0.0f;

    if (input != NULL)
    {
        confidence_pct = input->cgm.sqi_pct;
    }

    if ((input != NULL) && ((input->cgm.sensor_flags & APS_SENSOR_FLAG_BAD_SQI) != 0u))
    {
        confidence_pct = (uint8_t)((confidence_pct > 35u) ? (confidence_pct - 35u) : 0u);
    }
    if ((input != NULL) && (((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u) || input->cgm.age_s > PREDICTOR_V2_STALE_CGM_THRESHOLD_S))
    {
        confidence_pct = 0u;
    }
    if ((input != NULL) && !input->physiology_present && confidence_pct > 5u)
    {
        confidence_pct = (uint8_t)(confidence_pct - 5u);
    }
    if (features != NULL)
    {
        rolling_stddev_6 = features->values[PREDICTOR_V2_FEATURE_ROLL_STDDEV_6];
        volatility_score = features->values[PREDICTOR_V2_FEATURE_VOLATILITY_SCORE];
    }
    if (rolling_stddev_6 > 30.0f && confidence_pct > 15u)
    {
        confidence_pct = (uint8_t)(confidence_pct - 15u);
    }
    if (volatility_score > 35.0f && confidence_pct > 10u)
    {
        confidence_pct = (uint8_t)(confidence_pct - 10u);
    }
    if (used_fallback && confidence_pct > 10u)
    {
        confidence_pct = (uint8_t)(confidence_pct - 10u);
    }

    return confidence_pct;
}

static float baseline_delta_for_horizon(const predictor_v2_input_t *input, predictor_v2_horizon_t horizon)
{
    float trend_mgdl_min = current_trend_mgdl_min(input);

    switch (horizon)
    {
        case PREDICTOR_V2_HORIZON_15M:
            return trend_mgdl_min * 15.0f;
        case PREDICTOR_V2_HORIZON_30M:
            return trend_mgdl_min * 30.0f;
        case PREDICTOR_V2_HORIZON_60M:
            return trend_mgdl_min * 60.0f;
        default:
            return 0.0f;
    }
}

static bool should_force_fallback(const predictor_v2_input_t *input,
                                  const predictor_v2_generated_model_t *model,
                                  uint16_t *status_flags)
{
    bool bad_sensor_quality = false;
    bool stale_cgm = false;

    if (input != NULL)
    {
        if (!input->physiology_present)
        {
            if (status_flags != NULL)
            {
                *status_flags |= PREDICTOR_V2_STATUS_MISSING_PHYSIOLOGY;
            }
        }
        bad_sensor_quality = (input->cgm.sqi_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD) ||
                             ((input->cgm.sensor_flags & APS_SENSOR_FLAG_BAD_SQI) != 0u) ||
                             ((input->cgm.sensor_flags & APS_SENSOR_FLAG_INVALID) != 0u);
        stale_cgm = ((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u) ||
                    (input->cgm.age_s > PREDICTOR_V2_STALE_CGM_THRESHOLD_S);
    }

    if (status_flags != NULL)
    {
        if (bad_sensor_quality)
        {
            *status_flags |= PREDICTOR_V2_STATUS_BAD_SQI;
        }
        if (stale_cgm)
        {
            *status_flags |= PREDICTOR_V2_STATUS_STALE_CGM;
        }
        if (!model_is_valid(model))
        {
            *status_flags |= PREDICTOR_V2_STATUS_INVALID_MODEL;
        }
    }

    return bad_sensor_quality || stale_cgm || !model_is_valid(model);
}

static float evaluate_standardized_model(const predictor_v2_generated_model_t *model,
                                         const predictor_v2_feature_vector_t *features,
                                         uint16_t *status_flags)
{
    size_t index;
    float score = 0.0f;

    if (status_flags == NULL)
    {
        return 0.0f;
    }

    if (!model_is_valid(model))
    {
        *status_flags |= PREDICTOR_V2_STATUS_INVALID_MODEL;
        return 0.0f;
    }

    score = model->intercept;
    *status_flags |= PREDICTOR_V2_STATUS_VALID_MODEL;
    for (index = 0u; index < PREDICTOR_V2_FEATURE_COUNT; ++index)
    {
        float raw_value = features->values[index];
        float standardized_value;

        if ((features->valid_mask & (1ull << index)) == 0u || !isfinite(raw_value))
        {
            raw_value = model->feature_median[index];
            *status_flags |= PREDICTOR_V2_STATUS_INVALID_FEATURES;
        }

        standardized_value = (raw_value - model->feature_mean[index]) / model->feature_scale[index];
        score += model->coefficients[index] * standardized_value;
    }

    return score;
}

static void compute_risk_flags_for_prediction(const predictor_v2_input_t *input,
                                              uint16_t prediction_mgdl,
                                              int16_t delta_mgdl,
                                              uint8_t confidence_pct,
                                              uint16_t *risk_flags,
                                              predictor_v2_horizon_t horizon)
{
    if (risk_flags == NULL)
    {
        return;
    }

    if (prediction_mgdl < 80u)
    {
        if (horizon == PREDICTOR_V2_HORIZON_15M)
        {
            *risk_flags |= APS_RISK_PRED_LOW_15M;
        }
        else if (horizon == PREDICTOR_V2_HORIZON_30M)
        {
            *risk_flags |= APS_RISK_PRED_LOW_30M;
        }
        else
        {
            *risk_flags |= APS_RISK_PRED_LOW_60M;
        }
    }
    if (prediction_mgdl > 180u)
    {
        if (horizon == PREDICTOR_V2_HORIZON_30M)
        {
            *risk_flags |= APS_RISK_PRED_HIGH_30M;
        }
        else if (horizon == PREDICTOR_V2_HORIZON_60M)
        {
            *risk_flags |= APS_RISK_PRED_HIGH_60M;
        }
    }
    if (delta_mgdl < -20)
    {
        *risk_flags |= APS_RISK_RAPID_FALL;
    }
    if (delta_mgdl > 20)
    {
        *risk_flags |= APS_RISK_RAPID_RISE;
    }
    if (confidence_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD)
    {
        *risk_flags |= APS_RISK_LOW_CONFIDENCE;
    }
    if (input != NULL && input->cgm.sqi_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD)
    {
        *risk_flags |= APS_RISK_BAD_SENSOR_QUALITY;
    }
    if (input != NULL && (((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u) || input->cgm.age_s > PREDICTOR_V2_STALE_CGM_THRESHOLD_S))
    {
        *risk_flags |= APS_RISK_STALE_CGM;
    }
}

static bool evaluate_horizon_from_features(const predictor_v2_input_t *input,
                                           const predictor_v2_feature_vector_t *features,
                                           predictor_v2_horizon_t horizon,
                                           predictor_v2_horizon_eval_t *evaluation)
{
    const predictor_v2_generated_bundle_t *bundle;
    const predictor_v2_generated_model_t *model;
    float current_glucose;
    float delta_mgdl;
    float prediction_mgdl;
    uint16_t status_flags = 0u;
    bool force_fallback;

    if (input == NULL || features == NULL || evaluation == NULL || !sample_is_valid(input))
    {
        return false;
    }

    memset(evaluation, 0, sizeof(*evaluation));
    bundle = (g_predictor_state.bundle_override != NULL) ? g_predictor_state.bundle_override : PredictorV2_GetDefaultModelBundle();
    model = select_horizon_model(bundle, horizon);

    current_glucose = (float)input->cgm.glucose_mgdl;
    force_fallback = should_force_fallback(input, model, &status_flags);

    if (!force_fallback)
    {
        delta_mgdl = evaluate_standardized_model(model, features, &status_flags);
        prediction_mgdl = clamp_float(current_glucose + delta_mgdl,
                                      (float)PREDICTOR_V2_MIN_PRED_MGDL,
                                      (float)PREDICTOR_V2_MAX_PRED_MGDL);
        evaluation->used_fallback = false;
    }
    else
    {
        delta_mgdl = baseline_delta_for_horizon(input, horizon);
        prediction_mgdl = clamp_float(current_glucose + delta_mgdl,
                                      (float)PREDICTOR_V2_MIN_PRED_MGDL,
                                      (float)PREDICTOR_V2_MAX_PRED_MGDL);
        evaluation->used_fallback = true;
        status_flags |= PREDICTOR_V2_STATUS_FALLBACK_LINEAR;
    }

    evaluation->prediction_mgdl = (uint16_t)lroundf(prediction_mgdl);
    evaluation->delta_mgdl = clamp_i16_i32((int32_t)lroundf((float)evaluation->prediction_mgdl - current_glucose), -400, 400);
    evaluation->confidence_pct = estimate_confidence_pct(input, features, evaluation->used_fallback);
    evaluation->status_flags = status_flags;

    if (evaluation->confidence_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD)
    {
        evaluation->status_flags |= PREDICTOR_V2_STATUS_LOW_CONFIDENCE;
    }

    compute_risk_flags_for_prediction(input,
                                      evaluation->prediction_mgdl,
                                      evaluation->delta_mgdl,
                                      evaluation->confidence_pct,
                                      &evaluation->risk_flags,
                                      horizon);

    return true;
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

const predictor_v2_generated_bundle_t *PredictorV2_GetDefaultModelBundle(void)
{
    return PredictorV2Generated_DefaultBundle();
}

void PredictorV2_SetModelBundleForTesting(const predictor_v2_generated_bundle_t *bundle)
{
    g_predictor_state.bundle_override = bundle;
}

bool PredictorV2_UsesDynamicMemory(void)
{
    return false;
}

bool PredictorV2_BuildFeatureVector(const predictor_v2_input_t *input, predictor_v2_feature_vector_t *features)
{
    bool lag1_valid = false;
    bool lag2_valid = false;
    bool lag3_valid = false;
    bool lag6_valid = false;
    bool lag9_valid = false;
    bool lag12_valid = false;
    float current_glucose;
    float lag1;
    float lag2;
    float lag3;
    float lag6;
    float lag9;
    float lag12;
    float delta1;
    float delta3;
    float delta6;
    float slope15;
    float slope30;
    float slope60;
    uint32_t roll_count_3 = 0u;
    uint32_t roll_count_6 = 0u;
    uint32_t roll_count_12 = 0u;
    float mean3;
    float mean6;
    float mean12;
    float var6;
    float stddev6;
    float volatility_score;
    float cgm_age_s;

    if (input == NULL || features == NULL || !sample_is_valid(input))
    {
        return false;
    }

    memset(features, 0, sizeof(*features));

    current_glucose = (float)input->cgm.glucose_mgdl;
    lag1 = glucose_at_lag(1u, current_glucose, &lag1_valid);
    lag2 = glucose_at_lag(2u, current_glucose, &lag2_valid);
    lag3 = glucose_at_lag(3u, current_glucose, &lag3_valid);
    lag6 = glucose_at_lag(6u, current_glucose, &lag6_valid);
    lag9 = glucose_at_lag(9u, current_glucose, &lag9_valid);
    lag12 = glucose_at_lag(12u, current_glucose, &lag12_valid);
    delta1 = current_glucose - lag1;
    delta3 = current_glucose - lag3;
    delta6 = current_glucose - lag6;
    slope15 = slope_mgdl_per_min(3u, current_glucose, &lag3_valid);
    slope30 = slope_mgdl_per_min(6u, current_glucose, &lag6_valid);
    slope60 = slope_mgdl_per_min(12u, current_glucose, &lag12_valid);
    mean3 = rolling_mean(3u, current_glucose, &roll_count_3);
    mean6 = rolling_mean(6u, current_glucose, &roll_count_6);
    mean12 = rolling_mean(12u, current_glucose, &roll_count_12);
    var6 = rolling_variance(6u, current_glucose, &roll_count_6);
    stddev6 = sqrtf(var6);
    volatility_score = stddev6 + (fabsf(delta3) * 0.25f) + (fabsf(delta6) * 0.125f);
    cgm_age_s = (float)input->cgm.age_s;

    feature_set(features, PREDICTOR_V2_FEATURE_CURRENT_GLUCOSE, current_glucose, true);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_1, lag1, lag1_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_2, lag2, lag2_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_3, lag3, lag3_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_6, lag6, lag6_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_9, lag9, lag9_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_LAG_12, lag12, lag12_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_DELTA_1, delta1, lag1_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_DELTA_3, delta3, lag3_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_DELTA_6, delta6, lag6_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_SLOPE_15M, slope15, lag3_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_SLOPE_30M, slope30, lag6_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_SLOPE_60M, slope60, lag12_valid);
    feature_set(features, PREDICTOR_V2_FEATURE_ROLL_MEAN_3, mean3, roll_count_3 > 0u);
    feature_set(features, PREDICTOR_V2_FEATURE_ROLL_MEAN_6, mean6, roll_count_6 > 0u);
    feature_set(features, PREDICTOR_V2_FEATURE_ROLL_MEAN_12, mean12, roll_count_12 > 0u);
    feature_set(features, PREDICTOR_V2_FEATURE_ROLL_VAR_6, var6, roll_count_6 > 1u);
    feature_set(features, PREDICTOR_V2_FEATURE_ROLL_STDDEV_6, stddev6, roll_count_6 > 1u);
    feature_set(features, PREDICTOR_V2_FEATURE_VOLATILITY_SCORE, volatility_score, roll_count_6 > 1u);
    {
        bool iob_valid = input->physiology_present && isfinite(input->physiology.iob_u) && input->physiology.iob_u >= 0.0f && input->physiology.iob_u <= 20.0f;
        bool cob_valid = input->physiology_present && isfinite(input->physiology.cob_g) && input->physiology.cob_g >= 0.0f && input->physiology.cob_g <= 200.0f;
        bool basal_valid = input->physiology_present && isfinite(input->physiology.basal_u_per_hr) && input->physiology.basal_u_per_hr >= 0.0f && input->physiology.basal_u_per_hr <= 5.0f;
        bool insulin_30_valid = input->physiology_present && isfinite(input->physiology.insulin_30m_u) && input->physiology.insulin_30m_u >= 0.0f && input->physiology.insulin_30m_u <= 20.0f;
        bool insulin_120_valid = input->physiology_present && isfinite(input->physiology.insulin_120m_u) && input->physiology.insulin_120m_u >= 0.0f && input->physiology.insulin_120m_u <= 40.0f;
        bool carbs_30_valid = input->physiology_present && isfinite(input->physiology.carbs_30m_g) && input->physiology.carbs_30m_g >= 0.0f && input->physiology.carbs_30m_g <= 200.0f;
        bool carbs_120_valid = input->physiology_present && isfinite(input->physiology.carbs_120m_g) && input->physiology.carbs_120m_g >= 0.0f && input->physiology.carbs_120m_g <= 300.0f;
        bool activity_state_valid = input->physiology_present && input->physiology.activity_state <= (uint8_t)ACTIVITY_SLEEP;
        bool activity_confidence_valid = input->physiology_present && input->physiology.activity_confidence_pct <= 100u;
        bool motion_5_valid = input->physiology_present && isfinite(input->physiology.motion_rms_5m) && input->physiology.motion_rms_5m >= 0.0f && input->physiology.motion_rms_5m <= 200.0f;
        bool motion_15_valid = input->physiology_present && isfinite(input->physiology.motion_rms_15m) && input->physiology.motion_rms_15m >= 0.0f && input->physiology.motion_rms_15m <= 200.0f;
        bool active_minutes_valid = input->physiology_present && input->physiology.active_minutes <= 1440u;
        bool post_exercise_valid = input->physiology_present && input->physiology.post_exercise_minutes <= 1440u;

        feature_set(features, PREDICTOR_V2_FEATURE_IOB, sanitize_feature_float(input->physiology.iob_u, 0.0f, 20.0f, 0.0f, NULL), iob_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_COB, sanitize_feature_float(input->physiology.cob_g, 0.0f, 200.0f, 0.0f, NULL), cob_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_BASAL_RATE, sanitize_feature_float(input->physiology.basal_u_per_hr, 0.0f, 5.0f, 0.0f, NULL), basal_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_INSULIN_30M, sanitize_feature_float(input->physiology.insulin_30m_u, 0.0f, 20.0f, 0.0f, NULL), insulin_30_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_INSULIN_120M, sanitize_feature_float(input->physiology.insulin_120m_u, 0.0f, 40.0f, 0.0f, NULL), insulin_120_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_CARBS_30M, sanitize_feature_float(input->physiology.carbs_30m_g, 0.0f, 200.0f, 0.0f, NULL), carbs_30_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_CARBS_120M, sanitize_feature_float(input->physiology.carbs_120m_g, 0.0f, 300.0f, 0.0f, NULL), carbs_120_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_SQI, (float)input->cgm.sqi_pct, true);
        feature_set(features, PREDICTOR_V2_FEATURE_CGM_AGE_S, cgm_age_s, true);
        feature_set(features, PREDICTOR_V2_FEATURE_TOD_SIN, time_of_day_sin(input->cgm.epoch_s), true);
        feature_set(features, PREDICTOR_V2_FEATURE_TOD_COS, time_of_day_cos(input->cgm.epoch_s), true);
        feature_set(features, PREDICTOR_V2_FEATURE_ACTIVITY_STATE, sanitize_activity_state_feature(input->physiology.activity_state, NULL), activity_state_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_ACTIVITY_CONFIDENCE, sanitize_feature_float((float)input->physiology.activity_confidence_pct, 0.0f, 100.0f, 0.0f, NULL), activity_confidence_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_MOTION_RMS_5M, sanitize_feature_float(input->physiology.motion_rms_5m, 0.0f, 200.0f, 0.0f, NULL), motion_5_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_MOTION_RMS_15M, sanitize_feature_float(input->physiology.motion_rms_15m, 0.0f, 200.0f, 0.0f, NULL), motion_15_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_ACTIVE_MINUTES, sanitize_feature_float((float)input->physiology.active_minutes, 0.0f, 1440.0f, 0.0f, NULL), active_minutes_valid);
        feature_set(features, PREDICTOR_V2_FEATURE_POST_EXERCISE_MINUTES, sanitize_feature_float((float)input->physiology.post_exercise_minutes, 0.0f, 1440.0f, 0.0f, NULL), post_exercise_valid);
    }

    return true;
}

bool PredictorV2_EvaluateHorizon(const predictor_v2_input_t *input,
                                 predictor_v2_horizon_t horizon,
                                 predictor_v2_horizon_eval_t *evaluation)
{
    predictor_v2_feature_vector_t features;

    if (!PredictorV2_BuildFeatureVector(input, &features))
    {
        return false;
    }

    return evaluate_horizon_from_features(input, &features, horizon, evaluation);
}

bool PredictorV2_Update(const predictor_v2_input_t *input, predictor_v2_output_t *output)
{
    predictor_v2_feature_vector_t features;
    predictor_v2_horizon_eval_t eval_15m;
    predictor_v2_horizon_eval_t eval_30m;
    predictor_v2_horizon_eval_t eval_60m;
    bool valid_input;

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

    valid_input = PredictorV2_BuildFeatureVector(input, &features);
    if (!valid_input)
    {
        return false;
    }

    if (!evaluate_horizon_from_features(input, &features, PREDICTOR_V2_HORIZON_15M, &eval_15m) ||
        !evaluate_horizon_from_features(input, &features, PREDICTOR_V2_HORIZON_30M, &eval_30m) ||
        !evaluate_horizon_from_features(input, &features, PREDICTOR_V2_HORIZON_60M, &eval_60m))
    {
        return false;
    }

    output->pred_15m_mgdl = eval_15m.prediction_mgdl;
    output->pred_30m_mgdl = eval_30m.prediction_mgdl;
    output->pred_60m_mgdl = eval_60m.prediction_mgdl;
    output->delta_15m_mgdl = eval_15m.delta_mgdl;
    output->delta_30m_mgdl = eval_30m.delta_mgdl;
    output->delta_60m_mgdl = eval_60m.delta_mgdl;
    output->confidence_pct = eval_15m.confidence_pct;
    output->risk_flags = eval_15m.risk_flags | eval_30m.risk_flags | eval_60m.risk_flags;
    output->model_status_flags = eval_15m.status_flags | eval_30m.status_flags | eval_60m.status_flags;

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
    if (output->confidence_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD)
    {
        output->risk_flags |= APS_RISK_LOW_CONFIDENCE;
        output->model_status_flags |= PREDICTOR_V2_STATUS_LOW_CONFIDENCE;
    }
    if (input->cgm.sqi_pct < PREDICTOR_V2_LOW_CONFIDENCE_THRESHOLD)
    {
        output->risk_flags |= APS_RISK_BAD_SENSOR_QUALITY;
    }
    if ((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u || input->cgm.age_s > PREDICTOR_V2_STALE_CGM_THRESHOLD_S)
    {
        output->risk_flags |= APS_RISK_STALE_CGM;
    }

    return true;
}
