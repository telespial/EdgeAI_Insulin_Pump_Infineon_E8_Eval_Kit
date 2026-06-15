#include "aps_demo_state.h"
#include "aps_physiology.h"
#include "activity_engine.h"
#include "controller_openaps.h"
#include "cob_engine.h"
#include "metrics.h"
#include "iob_engine.h"
#include "predictor_v2.h"
#include "replay_loader.h"
#include "scenario_runner.h"
#include "safety_supervisor.h"
#include "simulation_runner.h"
#include "virtual_patient_v2.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_OPENAPS_REASON_LOW_PRED (1u << 16)

static int g_failures;

static void require_true(const char *name, int condition)
{
    if (!condition)
    {
        fprintf(stderr, "FAILED: %s\n", name);
        g_failures++;
    }
    else
    {
        printf("PASS: %s\n", name);
    }
}

static predictor_v2_input_t make_input(uint32_t epoch_s,
                                       uint16_t glucose_mgdl,
                                       int16_t trend_mgdl_min_x100,
                                       uint8_t sqi_pct,
                                       uint16_t sensor_flags,
                                       float iob_u,
                                       float cob_g,
                                       bool physiology_present)
{
    predictor_v2_input_t input;

    memset(&input, 0, sizeof(input));
    input.cgm.epoch_s = epoch_s;
    input.cgm.age_s = 0u;
    input.cgm.glucose_mgdl = glucose_mgdl;
    input.cgm.trend_mgdl_min_x100 = trend_mgdl_min_x100;
    input.cgm.sqi_pct = sqi_pct;
    input.cgm.sensor_flags = sensor_flags;
    input.cgm.valid = true;
    input.physiology.iob_u = iob_u;
    input.physiology.cob_g = cob_g;
    input.physiology.basal_u_per_hr = 0.8f;
    input.physiology_present = physiology_present;
    return input;
}

static const predictor_v2_generated_bundle_t *feature_fallback_bundle(void)
{
    static predictor_v2_generated_model_t model;
    static predictor_v2_generated_bundle_t bundle;
    size_t feature_index;

    memset(&model, 0, sizeof(model));
    for (feature_index = 0u; feature_index < PREDICTOR_V2_FEATURE_COUNT; ++feature_index)
    {
        model.feature_scale[feature_index] = 1.0f;
        model.feature_mean[feature_index] = 0.0f;
        model.feature_median[feature_index] = 0.0f;
        model.coefficients[feature_index] = 0.0f;
    }
    model.feature_median[PREDICTOR_V2_FEATURE_LAG_1] = 111.0f;
    model.coefficients[PREDICTOR_V2_FEATURE_LAG_1] = 1.0f;
    model.valid = true;
    bundle.horizon_15m = &model;
    bundle.horizon_30m = &model;
    bundle.horizon_60m = &model;
    return &bundle;
}

static const predictor_v2_generated_bundle_t *invalid_model_bundle(void)
{
    static predictor_v2_generated_model_t model;
    static predictor_v2_generated_bundle_t bundle;
    size_t feature_index;

    memset(&model, 0, sizeof(model));
    for (feature_index = 0u; feature_index < PREDICTOR_V2_FEATURE_COUNT; ++feature_index)
    {
        model.feature_scale[feature_index] = 1.0f;
        model.feature_mean[feature_index] = 0.0f;
        model.feature_median[feature_index] = 0.0f;
        model.coefficients[feature_index] = 0.0f;
    }
    model.feature_scale[PREDICTOR_V2_FEATURE_CURRENT_GLUCOSE] = 0.0f;
    model.valid = true;
    bundle.horizon_15m = &model;
    bundle.horizon_30m = &model;
    bundle.horizon_60m = &model;
    return &bundle;
}

static void clear_predictor_bundle_override(void)
{
    PredictorV2_SetModelBundleForTesting(NULL);
}

static void write_text_file(const char *path, const char *contents)
{
    FILE *file = fopen(path, "w");
    require_true("open temp file", file != NULL);
    if (file != NULL)
    {
        fputs(contents, file);
        fclose(file);
    }
}

static void test_predictor_outputs(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t output;
    uint32_t sample_index;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);

    for (sample_index = 0u; sample_index < 12u; ++sample_index)
    {
        input = make_input(300u * sample_index, (uint16_t)(140u + sample_index), 15, 95u, 0u, 0.3f, 5.0f, true);
        require_true("predictor warm-up update", PredictorV2_Update(&input, &output));
    }

    input = make_input(3600u, 160u, 20, 95u, 0u, 0.5f, 8.0f, true);
    require_true("predictor update", PredictorV2_Update(&input, &output));
    require_true("predictor produces 15m", output.pred_15m_mgdl > 0u);
    require_true("predictor produces 30m", output.pred_30m_mgdl > 0u);
    require_true("predictor produces 60m", output.pred_60m_mgdl > 0u);
    require_true("predictor horizons differ", (output.pred_15m_mgdl != output.pred_30m_mgdl) || (output.pred_30m_mgdl != output.pred_60m_mgdl));
}

static void test_predictor_feature_vector_construction(void)
{
    predictor_v2_input_t input;
    predictor_v2_feature_vector_t features;
    bool valid;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    input = make_input(300u, 145u, 0, 95u, 0u, 0.2f, 4.0f, true);
    valid = PredictorV2_BuildFeatureVector(&input, &features);
    require_true("feature vector builds", valid);
    require_true("feature current glucose", (uint16_t)features.values[PREDICTOR_V2_FEATURE_CURRENT_GLUCOSE] == 145u);
    require_true("feature lag1 defaults to current", (uint16_t)features.values[PREDICTOR_V2_FEATURE_LAG_1] == 145u);
    require_true("feature lag1 marked invalid", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_LAG_1)) == 0u);
    require_true("feature tod sin finite", features.values[PREDICTOR_V2_FEATURE_TOD_SIN] >= -1.0f && features.values[PREDICTOR_V2_FEATURE_TOD_SIN] <= 1.0f);
    require_true("predictor no dynamic memory", !PredictorV2_UsesDynamicMemory());
}

static void test_predictor_physiology_feature_integration(void)
{
    predictor_v2_input_t input;
    predictor_v2_feature_vector_t features;
    predictor_v2_horizon_eval_t evaluation;
    activity_features_t activity;

    IobEngine_Reset();
    CobEngine_Reset();
    ActivityEngine_Reset();
    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);

    require_true("physiology iob seed", IobEngine_AddBolus(300u, 1.5f));
    require_true("physiology cob seed", CobEngine_AddMeal(300u, 22.0f, 180u));
    require_true("physiology activity seed", ActivityEngine_Update(300u, 700, 0, 0, true, 30u, &activity));
    require_true("physiology activity cool-down", ActivityEngine_Update(360u, 0, 0, 1000, false, 0u, &activity));

    input = make_input(420u, 150u, 12, 95u, 0u, IobEngine_GetIobU(), CobEngine_GetCobG(), true);
    input.physiology.activity_state = (uint8_t)activity.state;
    input.physiology.activity_confidence_pct = activity.confidence_pct;
    input.physiology.motion_rms_5m = activity.motion_rms_5m;
    input.physiology.motion_rms_15m = activity.motion_rms_15m;
    input.physiology.active_minutes = activity.active_minutes;
    input.physiology.post_exercise_minutes = activity.post_exercise_minutes;

    require_true("physiology feature vector builds", PredictorV2_BuildFeatureVector(&input, &features));
    require_true("physiology iob feature present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_IOB)) != 0u);
    require_true("physiology cob feature present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_COB)) != 0u);
    require_true("physiology activity state present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_ACTIVITY_STATE)) != 0u);
    require_true("physiology activity confidence present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_ACTIVITY_CONFIDENCE)) != 0u);
    require_true("physiology active minutes present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_ACTIVE_MINUTES)) != 0u);
    require_true("physiology post exercise present", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_POST_EXERCISE_MINUTES)) != 0u);
    require_true("physiology activity state matches", (uint16_t)features.values[PREDICTOR_V2_FEATURE_ACTIVITY_STATE] == (uint16_t)activity.state);
    require_true("physiology confidence matches", (uint16_t)features.values[PREDICTOR_V2_FEATURE_ACTIVITY_CONFIDENCE] == (uint16_t)activity.confidence_pct);
    require_true("physiology motion 5m bounded", features.values[PREDICTOR_V2_FEATURE_MOTION_RMS_5M] >= 0.0f && features.values[PREDICTOR_V2_FEATURE_MOTION_RMS_5M] <= 200.0f);
    require_true("physiology motion 15m bounded", features.values[PREDICTOR_V2_FEATURE_MOTION_RMS_15M] >= 0.0f && features.values[PREDICTOR_V2_FEATURE_MOTION_RMS_15M] <= 200.0f);
    require_true("physiology eval works", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &evaluation));
    require_true("physiology status not missing", (evaluation.status_flags & PREDICTOR_V2_STATUS_MISSING_PHYSIOLOGY) == 0u);
}

static void test_predictor_missing_physiology_fallback(void)
{
    predictor_v2_input_t input;
    predictor_v2_feature_vector_t features;
    predictor_v2_horizon_eval_t evaluation;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    input = make_input(600u, 152u, 8, 95u, 0u, 0.0f, 0.0f, false);
    input.physiology.activity_state = ACTIVITY_UNKNOWN;
    input.physiology.activity_confidence_pct = 0u;
    input.physiology.motion_rms_5m = 0.0f;
    input.physiology.motion_rms_15m = 0.0f;
    input.physiology.active_minutes = 0u;
    input.physiology.post_exercise_minutes = 0u;

    require_true("missing physiology feature vector", PredictorV2_BuildFeatureVector(&input, &features));
    require_true("missing physiology iob masked", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_IOB)) == 0u);
    require_true("missing physiology activity masked", (features.valid_mask & (1ull << PREDICTOR_V2_FEATURE_ACTIVITY_STATE)) == 0u);
    require_true("missing physiology eval works", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &evaluation));
    require_true("missing physiology status emitted", (evaluation.status_flags & PREDICTOR_V2_STATUS_MISSING_PHYSIOLOGY) != 0u);
}

static void test_predictor_invalid_physiology_sanitized(void)
{
    predictor_v2_input_t input;
    predictor_v2_feature_vector_t features;
    predictor_v2_horizon_eval_t evaluation;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    input = make_input(900u, 148u, 10, 95u, 0u, -4.0f, 999.0f, true);
    input.physiology.basal_u_per_hr = -1.0f;
    input.physiology.insulin_30m_u = 50.0f;
    input.physiology.insulin_120m_u = 80.0f;
    input.physiology.carbs_30m_g = -2.0f;
    input.physiology.carbs_120m_g = 999.0f;
    input.physiology.activity_state = 42u;
    input.physiology.activity_confidence_pct = 240u;
    input.physiology.motion_rms_5m = -10.0f;
    input.physiology.motion_rms_15m = 400.0f;
    input.physiology.active_minutes = 5000u;
    input.physiology.post_exercise_minutes = 6000u;

    require_true("invalid physiology feature vector", PredictorV2_BuildFeatureVector(&input, &features));
    require_true("invalid physiology iob clamped", (uint16_t)features.values[PREDICTOR_V2_FEATURE_IOB] == 0u);
    require_true("invalid physiology cob clamped", (uint16_t)features.values[PREDICTOR_V2_FEATURE_COB] == 200u);
    require_true("invalid physiology basal clamped", (uint16_t)features.values[PREDICTOR_V2_FEATURE_BASAL_RATE] == 0u);
    require_true("invalid physiology activity state clamped", (uint16_t)features.values[PREDICTOR_V2_FEATURE_ACTIVITY_STATE] == 0u);
    require_true("invalid physiology motion clamped", (uint16_t)features.values[PREDICTOR_V2_FEATURE_MOTION_RMS_15M] == 200u);
    require_true("invalid physiology eval works", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &evaluation));
    require_true("invalid physiology status emitted", (evaluation.status_flags & PREDICTOR_V2_STATUS_INVALID_FEATURES) != 0u);
}

static void test_predictor_invalid_feature_fallback(void)
{
    predictor_v2_input_t input;
    predictor_v2_horizon_eval_t evaluation;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    PredictorV2_SetModelBundleForTesting(feature_fallback_bundle());

    input = make_input(600u, 150u, 0, 95u, 0u, 0.0f, 0.0f, true);
    require_true("invalid feature input accepted", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &evaluation));
    require_true("invalid features flagged", (evaluation.status_flags & PREDICTOR_V2_STATUS_INVALID_FEATURES) != 0u);
    require_true("median fallback applied", evaluation.prediction_mgdl == 261u);
    require_true("linear fallback not forced", (evaluation.status_flags & PREDICTOR_V2_STATUS_FALLBACK_LINEAR) == 0u);
    clear_predictor_bundle_override();
}

static void test_predictor_independent_horizons(void)
{
    predictor_v2_input_t input;
    predictor_v2_horizon_eval_t eval_15m;
    predictor_v2_horizon_eval_t eval_30m;
    predictor_v2_horizon_eval_t eval_60m;
    predictor_v2_output_t warm_output;
    uint32_t sample_index;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    for (sample_index = 0u; sample_index < 12u; ++sample_index)
    {
        input = make_input(300u * sample_index, (uint16_t)(110u + (sample_index * 2u)), 18, 95u, 0u, 0.2f, 4.0f, true);
        require_true("horizon warm-up", PredictorV2_Update(&input, &warm_output));
    }

    input = make_input(4000u, 160u, 22, 95u, 0u, 0.4f, 6.0f, true);
    require_true("15m horizon eval", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &eval_15m));
    require_true("30m horizon eval", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_30M, &eval_30m));
    require_true("60m horizon eval", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_60M, &eval_60m));
    require_true("horizons differ",
                 eval_15m.prediction_mgdl != eval_30m.prediction_mgdl &&
                 eval_15m.prediction_mgdl != eval_60m.prediction_mgdl &&
                 eval_30m.prediction_mgdl != eval_60m.prediction_mgdl);
}

static void test_predictor_output_bounds(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t output;
    uint32_t sample_index;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    for (sample_index = 0u; sample_index < 12u; ++sample_index)
    {
        input = make_input(300u * sample_index, 380u, 50, 95u, 0u, 0.1f, 0.0f, true);
        require_true("bounds warm-up", PredictorV2_Update(&input, &output));
    }

    input = make_input(5000u, 400u, 80, 95u, 0u, 0.0f, 0.0f, true);
    require_true("bounds update", PredictorV2_Update(&input, &output));
    require_true("pred 15 bounds", output.pred_15m_mgdl >= 40u && output.pred_15m_mgdl <= 400u);
    require_true("pred 30 bounds", output.pred_30m_mgdl >= 40u && output.pred_30m_mgdl <= 400u);
    require_true("pred 60 bounds", output.pred_60m_mgdl >= 40u && output.pred_60m_mgdl <= 400u);
}

static void test_predictor_invalid_model_fallback(void)
{
    predictor_v2_input_t input;
    predictor_v2_horizon_eval_t evaluation;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    PredictorV2_SetModelBundleForTesting(invalid_model_bundle());

    input = make_input(600u, 150u, 20, 95u, 0u, 0.0f, 0.0f, true);
    require_true("invalid model fallback", PredictorV2_EvaluateHorizon(&input, PREDICTOR_V2_HORIZON_15M, &evaluation));
    require_true("invalid model flagged", (evaluation.status_flags & PREDICTOR_V2_STATUS_INVALID_MODEL) != 0u);
    require_true("linear fallback used", (evaluation.status_flags & PREDICTOR_V2_STATUS_FALLBACK_LINEAR) != 0u);
    require_true("baseline prediction preserved", evaluation.prediction_mgdl == 153u);
    clear_predictor_bundle_override();
}

static void test_predictor_bad_sqi_and_stale_fallback(void)
{
    predictor_v2_input_t bad_sqi_input;
    predictor_v2_input_t stale_input;
    predictor_v2_horizon_eval_t bad_sqi_eval;
    predictor_v2_horizon_eval_t stale_eval;
    predictor_v2_output_t warm_output;
    uint32_t sample_index;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    PredictorV2_SetModelBundleForTesting(NULL);
    for (sample_index = 0u; sample_index < 12u; ++sample_index)
    {
        bad_sqi_input = make_input(300u * sample_index, 130u, -5, 95u, 0u, 0.1f, 2.0f, true);
        require_true("fallback warm-up", PredictorV2_Update(&bad_sqi_input, &warm_output));
    }

    bad_sqi_input = make_input(4000u, 130u, -5, 45u, 0u, 0.1f, 2.0f, true);
    require_true("bad sqi fallback eval", PredictorV2_EvaluateHorizon(&bad_sqi_input, PREDICTOR_V2_HORIZON_15M, &bad_sqi_eval));
    require_true("bad sqi fallback used", (bad_sqi_eval.status_flags & PREDICTOR_V2_STATUS_FALLBACK_LINEAR) != 0u);
    require_true("bad sqi status", (bad_sqi_eval.status_flags & PREDICTOR_V2_STATUS_BAD_SQI) != 0u);

    stale_input = make_input(5000u, 130u, -5, 95u, APS_SENSOR_FLAG_STALE_CGM, 0.1f, 2.0f, true);
    stale_input.cgm.age_s = 1200u;
    require_true("stale fallback eval", PredictorV2_EvaluateHorizon(&stale_input, PREDICTOR_V2_HORIZON_15M, &stale_eval));
    require_true("stale fallback used", (stale_eval.status_flags & PREDICTOR_V2_STATUS_FALLBACK_LINEAR) != 0u);
    require_true("stale status", (stale_eval.status_flags & PREDICTOR_V2_STATUS_STALE_CGM) != 0u);
}

static void test_iob_engine(void)
{
    iob_engine_config_t config =
    {
        .bolus_duration_min = 300u,
        .basal_duration_min = 300u,
        .max_events = 16u,
    };

    IobEngine_Init(&config);
    require_true("iob bolus add", IobEngine_AddBolus(60u, 3.0f));
    require_true("iob basal add", IobEngine_AddBasal(60u, 1.2f, 60u));
    require_true("iob update start", IobEngine_Update(60u));
    require_true("iob positive", IobEngine_GetIobU() > 3.9f && IobEngine_GetIobU() < 4.3f);
    require_true("iob decays", IobEngine_Update(60u + (120u * 60u)));
    require_true("iob still positive", IobEngine_GetIobU() > 1.8f && IobEngine_GetIobU() < 2.3f);
    require_true("iob invalid timestamp rejected", !IobEngine_AddBolus(30u, 1.0f));
    require_true("iob negative rejected", !IobEngine_AddBolus(600u, -1.0f));
    require_true("iob expires", IobEngine_Update(60u + (360u * 60u)));
    require_true("iob zero after expiry", IobEngine_GetIobU() == 0.0f);
}

static void test_cob_engine(void)
{
    cob_engine_config_t config =
    {
        .meal_duration_min = 180u,
        .max_meals = 16u,
    };

    CobEngine_Init(&config);
    require_true("cob meal add", CobEngine_AddMeal(120u, 45.0f, 180u));
    require_true("cob update start", CobEngine_Update(120u));
    require_true("cob positive", CobEngine_GetCobG() > 44.0f && CobEngine_GetCobG() <= 45.0f);
    require_true("cob decays", CobEngine_Update(120u + (90u * 60u)));
    require_true("cob still positive", CobEngine_GetCobG() > 20.0f && CobEngine_GetCobG() < 25.0f);
    require_true("cob invalid timestamp rejected", !CobEngine_AddMeal(60u, 10.0f, 180u));
    require_true("cob negative rejected", !CobEngine_AddMeal(600u, -1.0f, 180u));
    require_true("cob expires", CobEngine_Update(120u + (240u * 60u)));
    require_true("cob zero after expiry", CobEngine_GetCobG() == 0.0f);
}

static void test_activity_engine(void)
{
    activity_engine_config_t config =
    {
        .max_samples = 32u,
    };
    activity_features_t features;
    activity_state_t state;

    ActivityEngine_Init(&config);
    require_true("activity low motion", ActivityEngine_Update(60u, 0, 0, 1000, false, 0u, &features));
    state = features.state;
    require_true("activity sedentary or sleep", state == ACTIVITY_SEDENTARY || state == ACTIVITY_SLEEP);
    require_true("activity confidence bounded", features.confidence_pct <= 100u);

    require_true("activity light motion", ActivityEngine_Update(120u, 980, 0, 0, true, 12u, &features));
    require_true("activity light or higher", features.state == ACTIVITY_LIGHT || features.state == ACTIVITY_MODERATE || features.state == ACTIVITY_EXERCISE);

    require_true("activity exercise motion", ActivityEngine_Update(180u, 700, 0, 0, true, 48u, &features));
    require_true("activity exercise reinforcement", ActivityEngine_Update(240u, 650, 0, 0, true, 48u, &features));
    require_true("activity exercise state", features.state == ACTIVITY_EXERCISE);
    require_true("activity confidence range", features.confidence_pct <= 100u);

    require_true("activity post exercise update", ActivityEngine_Update(1200u, 0, 0, 1000, false, 0u, &features));
    require_true("activity post exercise minutes", features.post_exercise_minutes > 0u);
}

static void configure_modules(void)
{
    openaps_controller_config_t controller_config;
    aps_safety_config_t safety_config;

    OpenApsController_Reset();
    controller_config.target_low_mgdl = 100u;
    controller_config.target_high_mgdl = 120u;
    controller_config.suspend_threshold_mgdl = 80u;
    controller_config.basal_u_per_hr = 0.8f;
    controller_config.max_basal_u_per_hr = 2.0f;
    controller_config.insulin_sensitivity_mgdl_per_u = 50.0f;
    controller_config.carb_ratio_g_per_u = 12.0f;
    controller_config.max_iob_u = 5.0f;
    OpenApsController_Configure(&controller_config);

    safety_config.min_valid_glucose_mgdl = 40u;
    safety_config.max_valid_glucose_mgdl = 400u;
    safety_config.low_prediction_threshold_mgdl = 80u;
    safety_config.min_sqi_for_increase_pct = 60u;
    safety_config.max_cgm_age_s = 600u;
    safety_config.max_basal_u_per_hr = 2.0f;
    safety_config.max_iob_u = 5.0f;
    SafetySupervisor_Configure(&safety_config);
}

static void test_controller_and_safety(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t prediction;
    aps_controller_output_t command;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    configure_modules();

    input = make_input(4000u, 210u, 40, 95u, 0u, 0.2f, 2.0f, true);
    PredictorV2_Update(&input, &prediction);

    require_true("controller basal decision", OpenApsController_DetermineBasal(&input, &prediction, &command));
    require_true("controller conservative decision", command.action == APS_ACTION_SUSPEND_BASAL);
    require_true("controller low reason emitted",
                 (command.reason_flags & TEST_OPENAPS_REASON_LOW_PRED) != 0u);

    input.cgm.sqi_pct = 45u;
    command.action = APS_ACTION_INCREASE_BASAL;
    command.requested_basal_u_per_hr = 1.6f;
    command.suggested_correction_u = 1.0f;
    require_true("safety applies", SafetySupervisor_Apply(4300u, &input, &prediction, &command));
    require_true("safety blocks increase on bad sqi", command.action != APS_ACTION_INCREASE_BASAL);
    require_true("safety reason bad sqi", (command.reason_flags & APS_SAFETY_REASON_BAD_SQI) != 0u);
}

static void test_metrics_module(void)
{
    uint16_t actual_values[4] = {100u, 120u, 80u, 140u};
    uint16_t predicted_values[4] = {110u, 118u, 90u, 130u};
    uint32_t safety_flags[4] = {0u, 1u, 0u, 1u};
    prediction_metrics_t prediction_metrics;
    clinical_metrics_t clinical_metrics;

    Metrics_ComputePrediction(actual_values, predicted_values, 4u, &prediction_metrics);
    Metrics_ComputeClinical(actual_values, predicted_values, safety_flags, 4u, &clinical_metrics);

    require_true("metrics mae", prediction_metrics.mae > 0.0f);
    require_true("metrics rmse", prediction_metrics.rmse > 0.0f);
    require_true("metrics clinical range", clinical_metrics.time_in_range_pct > 0.0f);
    require_true("metrics safety count", clinical_metrics.safety_intervention_count == 2u);
}

static void test_replay_loader_success(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_full.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min,carbs_g,insulin_u,basal_u_per_hr\n"
                    "300,110,95,0,0,0,0.8\n"
                    "600,115,95,1,12,0,0.8\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("replay csv loads", ReplayLoader_LoadCsv("test_replay_full.csv", &dataset, error, sizeof(error)));
    require_true("replay csv count", dataset.count == 2u);
    require_true("replay physiology columns present", dataset.physiology_columns_present);
    require_true("replay first timestamp", dataset.steps[0].now_s == 300u);
    remove("test_replay_full.csv");
}

static void test_replay_loader_missing_optional_columns(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_missing.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min\n"
                    "300,210,95,6\n"
                    "600,218,95,7\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("replay missing optional columns loads", ReplayLoader_LoadCsv("test_replay_missing.csv", &dataset, error, sizeof(error)));
    require_true("replay missing columns flagged", !dataset.physiology_columns_present);
    require_true("replay missing columns defaults", dataset.steps[0].insulin_u == 0.0f && dataset.steps[0].carbs_g == 0.0f);
    remove("test_replay_missing.csv");
}

static void test_scenario_runner_and_audit(void)
{
    const char *scenarios[] = {
        "stable in range",
        "rising after meal",
        "meal_rise",
        "falling after bolus",
        "predicted hypo",
        "stale cgm",
        "bad sqi",
        "high iob",
        "meal + insulin overlap",
        "physiology smoke",
    };
    size_t scenario_index;

    for (scenario_index = 0u; scenario_index < sizeof(scenarios) / sizeof(scenarios[0]); ++scenario_index)
    {
        replay_dataset_t dataset;
        char error[128];
        sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
        size_t result_count = 0u;

        memset(&dataset, 0, sizeof(dataset));
        memset(error, 0, sizeof(error));
        require_true("scenario load", ScenarioRunner_Load(scenarios[scenario_index], &dataset, error, sizeof(error)));
        require_true("scenario count", dataset.count > 0u);
        configure_modules();
        require_true("scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
        require_true("scenario result count", result_count == dataset.count);
        require_true("scenario baseline predictions", results[0].baseline_prediction.pred_15m_mgdl > 0u);
        require_true("scenario ML predictions", results[0].ml_prediction.pred_15m_mgdl > 0u);
        require_true("scenario safety output", results[0].safety_final_output.action <= APS_ACTION_CORRECTION_SUGGESTION);
        if (strcmp(scenarios[scenario_index], "physiology smoke") == 0)
        {
            require_true("physiology smoke activity populated", results[0].input.physiology.activity_state != ACTIVITY_UNKNOWN);
            require_true("physiology smoke active minutes", results[result_count - 1u].input.physiology.active_minutes > 0u);
        }
    }
}

static void test_safety_reason_codes(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    size_t result_count = 0u;

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("high iob scenario load", ScenarioRunner_Load("high iob", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("high iob scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
    require_true("high iob reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_EXCESSIVE_IOB) != 0u);
}

static void test_predicted_low_reason_codes(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t prediction;
    aps_controller_output_t command;

    memset(&input, 0, sizeof(input));
    memset(&prediction, 0, sizeof(prediction));
    memset(&command, 0, sizeof(command));

    input.cgm.epoch_s = 600u;
    input.cgm.glucose_mgdl = 112u;
    input.cgm.trend_mgdl_min_x100 = -24;
    input.cgm.sqi_pct = 95u;
    input.cgm.valid = true;
    input.physiology_present = true;

    prediction.pred_15m_mgdl = 76u;
    prediction.pred_30m_mgdl = 72u;
    prediction.pred_60m_mgdl = 90u;
    prediction.confidence_pct = 95u;

    command.action = APS_ACTION_INCREASE_BASAL;
    command.requested_basal_u_per_hr = 1.2f;
    command.suggested_correction_u = 0.5f;

    configure_modules();
    require_true("predicted low safety applies", SafetySupervisor_Apply(600u, &input, &prediction, &command));
    require_true("predicted low 15 emitted", (command.reason_flags & APS_SAFETY_REASON_PREDICTED_LOW_15M) != 0u);
    require_true("predicted low 30 emitted", (command.reason_flags & APS_SAFETY_REASON_PREDICTED_LOW_30M) != 0u);
    require_true("predicted low action suspended", command.action == APS_ACTION_SUSPEND_BASAL);
}

static void test_controller_blocked_reason(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    size_t result_count = 0u;

    write_text_file("test_replay_missing_phys.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min\n"
                    "300,120,95,4\n"
                    "600,122,95,4\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("missing phys csv loads", ReplayLoader_LoadCsv("test_replay_missing_phys.csv", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("missing phys scenario runs", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
    require_true("missing phys reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_MISSING_PHYSIOLOGY) != 0u);
    require_true("controller blocked reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_CONTROLLER_BLOCKED) != 0u);
    remove("test_replay_missing_phys.csv");
}

static void test_simulation_summary_and_audit_header(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    simulation_summary_t summary;
    size_t result_count = 0u;
    FILE *audit_file;
    char header_line[512];

    memset(&dataset, 0, sizeof(dataset));
    memset(&summary, 0, sizeof(summary));
    memset(error, 0, sizeof(error));
    require_true("summary scenario load", ScenarioRunner_Load("stable", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("summary scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, &summary, error, sizeof(error)));
    require_true("summary sample count", summary.sample_count == dataset.count);
    require_true("summary metric baseline", summary.baseline_15m.mae >= 0.0f);
    require_true("summary metric ml", summary.ml_15m.mae >= 0.0f);
    require_true("summary disagreement count", summary.controller_disagreement_count <= dataset.count);
    require_true("summary physiology present count", summary.physiology_present_count == dataset.count);
    require_true("summary physiology missing count", summary.physiology_missing_count == 0u);
    require_true("summary result has actual", results[0].has_actual_15m || dataset.count < 4u);

    audit_file = tmpfile();
    require_true("audit tmpfile", audit_file != NULL);
    if (audit_file != NULL)
    {
        memset(header_line, 0, sizeof(header_line));
        AuditTrace_PrintHeader(audit_file);
        rewind(audit_file);
        require_true("audit header read", fgets(header_line, sizeof(header_line), audit_file) != NULL);
        require_true("audit header baseline column", strstr(header_line, "baseline_pred_15") != NULL);
        require_true("audit header safety column", strstr(header_line, "safety_reason_codes") != NULL);
        fclose(audit_file);
    }
}

static void test_replay_fixture_regressions(void)
{
    const struct
    {
        const char *path;
        const char *name;
        uint32_t required_reason_mask;
        int expect_disagreement;
    } cases[] = {
        {"data/sample_replay_stable.csv", "stable", 0u, 0},
        {"data/sample_replay_meal_rise.csv", "meal_rise", 0u, 1},
        {"data/sample_replay_falling_bolus.csv", "falling_bolus", APS_SAFETY_REASON_EXCESSIVE_IOB, 1},
        {"data/sample_replay_bad_sqi.csv", "bad_sqi", APS_SAFETY_REASON_BAD_SQI | APS_SAFETY_REASON_CONTROLLER_BLOCKED, 0},
        {"data/sample_replay_stale_cgm.csv", "stale_cgm", APS_SAFETY_REASON_STALE_CGM | APS_SAFETY_REASON_CONTROLLER_BLOCKED, 0},
    };
    size_t case_index;

    for (case_index = 0u; case_index < sizeof(cases) / sizeof(cases[0]); ++case_index)
    {
        replay_dataset_t dataset;
        char error[128];
        sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
        simulation_summary_t summary;
        size_t result_count = 0u;
        FILE *audit_file;
        char header_line[512];

        memset(&dataset, 0, sizeof(dataset));
        memset(&summary, 0, sizeof(summary));
        memset(error, 0, sizeof(error));
        require_true("fixture loads", ReplayLoader_LoadCsv(cases[case_index].path, &dataset, error, sizeof(error)));
        require_true("fixture count", dataset.count > 0u);
        configure_modules();
        require_true("fixture run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, &summary, error, sizeof(error)));
        require_true("fixture result count", result_count == dataset.count);
        require_true("fixture baseline metric present", summary.baseline_15m.mae >= 0.0f);
        require_true("fixture ml metric present", summary.ml_15m.mae >= 0.0f);
        require_true("fixture disagreement present", summary.controller_disagreement_count >= (uint32_t)(cases[case_index].expect_disagreement ? 1 : 0));

        if (cases[case_index].required_reason_mask != 0u)
        {
            require_true("fixture safety reason present",
                         (results[result_count - 1u].safety_final_output.reason_flags & cases[case_index].required_reason_mask) != 0u);
        }

        audit_file = tmpfile();
        require_true("fixture audit tmpfile", audit_file != NULL);
        if (audit_file != NULL)
        {
            memset(header_line, 0, sizeof(header_line));
            AuditTrace_PrintHeader(audit_file);
            rewind(audit_file);
            require_true("fixture audit header", fgets(header_line, sizeof(header_line), audit_file) != NULL);
            require_true("fixture audit column 1", strstr(header_line, "timestamp,glucose_mgdl,sqi_pct") != NULL);
            require_true("fixture audit column 2", strstr(header_line, "baseline_pred_15") != NULL);
            require_true("fixture audit column 3", strstr(header_line, "safety_reason_codes") != NULL);
            fclose(audit_file);
        }
    }
}

static void test_bad_data_rejected_safely(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_bad.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min,carbs_g,insulin_u,basal_u_per_hr\n"
                    "0,500,95,0,0,0,0.8\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("bad data rejected", !ReplayLoader_LoadCsv("test_replay_bad.csv", &dataset, error, sizeof(error)));
    remove("test_replay_bad.csv");
}

static void test_physiology_feature_population(void)
{
    predictor_v2_input_t input;
    predictor_v2_feature_vector_t features;
    activity_features_t activity;

    IobEngine_Reset();
    CobEngine_Reset();
    ActivityEngine_Reset();
    require_true("iob seed", IobEngine_AddBolus(300u, 2.0f));
    require_true("cob seed", CobEngine_AddMeal(300u, 30.0f, 180u));
    require_true("iob update seed", IobEngine_Update(300u));
    require_true("cob update seed", CobEngine_Update(300u));
    require_true("activity seed", ActivityEngine_Update(300u, 700, 0, 0, true, 30u, &activity));
    require_true("activity seed reinforce", ActivityEngine_Update(360u, 650, 0, 0, true, 36u, &activity));
    require_true("activity seed reinforce 2", ActivityEngine_Update(420u, 680, 0, 0, true, 42u, &activity));

    memset(&input, 0, sizeof(input));
    input.cgm.epoch_s = 300u;
    input.cgm.glucose_mgdl = 140u;
    input.cgm.trend_mgdl_min_x100 = 10;
    input.cgm.sqi_pct = 95u;
    input.cgm.valid = true;
    input.physiology.iob_u = IobEngine_GetIobU();
    input.physiology.cob_g = CobEngine_GetCobG();
    input.physiology.activity_state = (uint8_t)activity.state;
    input.physiology.activity_confidence_pct = activity.confidence_pct;
    input.physiology.motion_rms_5m = activity.motion_rms_5m;
    input.physiology.motion_rms_15m = activity.motion_rms_15m;
    input.physiology.active_minutes = activity.active_minutes;
    input.physiology.post_exercise_minutes = activity.post_exercise_minutes;
    input.physiology_present = true;

    require_true("physiology copied iob", input.physiology.iob_u > 0.0f);
    require_true("physiology copied cob", input.physiology.cob_g > 0.0f);
    require_true("physiology copied activity", input.physiology.activity_state != ACTIVITY_UNKNOWN);
    require_true("physiology copied activity confidence", input.physiology.activity_confidence_pct > 0u);
    require_true("physiology feature vector still builds", PredictorV2_BuildFeatureVector(&input, &features));
}

static void test_aps_demo_state_pipeline(void)
{
    aps_demo_state_t state0;
    aps_demo_state_t state1;
    aps_demo_state_t state2;
    aps_demo_state_t state6;
    aps_demo_state_t state12;
    char buffer[160];

    memset(&state0, 0, sizeof(state0));
    memset(&state1, 0, sizeof(state1));
    memset(&state2, 0, sizeof(state2));
    memset(&state6, 0, sizeof(state6));
    memset(&state12, 0, sizeof(state12));
    memset(buffer, 0, sizeof(buffer));

    require_true("aps demo init", ApsDemoState_Init());
    require_true("aps demo step 0", ApsDemoState_Step(0u, &state0));
    require_true("aps demo step 1", ApsDemoState_Step(300u, &state1));
    require_true("aps demo step 2", ApsDemoState_Step(600u, &state2));
    require_true("aps demo step 6", ApsDemoState_Step(1800u, &state6));
    require_true("aps demo step 12", ApsDemoState_Step(3600u, &state12));
    require_true("aps demo bg changes", state0.bg_mgdl != state1.bg_mgdl);
    require_true("aps demo iob changes", state1.iob_u != state2.iob_u);
    require_true("aps demo cob becomes active", state6.cob_g > 0.0f);
    require_true("aps demo action text", state2.action_text != NULL && strlen(state2.action_text) > 0u);
    require_true("aps demo insulin generated", state2.insulin_u_hr >= 0.0f);
    require_true("aps demo safety text", state2.safe_text != NULL && strlen(state2.safe_text) > 0u);
    require_true("aps demo continues bg", state12.bg_mgdl >= 60u && state12.bg_mgdl <= 250u);
    require_true("aps demo continues iob", state12.iob_u >= 0.0f && state12.iob_u <= 5.0f);
    require_true("aps demo continues cob", state12.cob_g >= 0.0f && state12.cob_g <= 80.0f);
    require_true("aps demo format terminal", ApsDemoState_FormatTerminal(&state2, buffer, sizeof(buffer)));
    require_true("aps demo format bg sourced", strstr(buffer, "GLUCOSE: ") != NULL);
    require_true("aps demo format insulin sourced", strstr(buffer, "INS RATE: ") != NULL);
    require_true("aps demo format safety sourced", strstr(buffer, "SAFETY: ") != NULL);
}

static void test_virtual_patient_v2_continuous(void)
{
    uint32_t step_index;
    float delivered_insulin_u_hr = 0.8f;
    float previous_iob = -1.0f;
    float previous_cob = -1.0f;
    bool saw_bg_change = false;
    bool saw_iob_change = false;
    bool saw_cob_change = false;
    bool saw_values_after_30s = false;
    bool saw_exercise_window = false;
    bool saw_meal_event = false;
    bool saw_dawn_window = false;
    virtual_patient_v2_state_t state;

    memset(&state, 0, sizeof(state));
    VirtualPatientV2_Init();

    for (step_index = 0u; step_index < 72u; ++step_index)
    {
        uint32_t now_s = step_index * 300u;

        require_true("virtual patient step", VirtualPatientV2_Step(now_s, delivered_insulin_u_hr, &state));
        require_true("virtual patient bg bounded", state.bg_mgdl >= 60u && state.bg_mgdl <= 250u);
        require_true("virtual patient iob bounded", state.insulin_iob_u >= 0.0f && state.insulin_iob_u <= 5.0f);
        require_true("virtual patient cob bounded", state.meal_cob_g >= 0.0f && state.meal_cob_g <= 80.0f);
        require_true("virtual patient insulin bounded", state.basal_u_hr >= 0.0f && state.basal_u_hr <= 3.0f);
        require_true("virtual patient activity bounded", state.activity_factor >= 0.5f && state.activity_factor <= 1.0f);
        require_true("virtual patient sensitivity bounded", state.insulin_sensitivity >= 0.7f && state.insulin_sensitivity <= 1.5f);
        require_true("virtual patient dawn bounded", state.dawn_factor >= 1.0f && state.dawn_factor <= 1.25f);
        require_true("virtual patient no nan iob", state.insulin_iob_u == state.insulin_iob_u);
        require_true("virtual patient no nan cob", state.meal_cob_g == state.meal_cob_g);

        if (step_index > 0u && state.bg_mgdl != 110u)
        {
            saw_bg_change = true;
        }
        if (step_index > 0u && previous_iob >= 0.0f && fabsf(previous_iob - state.insulin_iob_u) > 0.01f)
        {
            saw_iob_change = true;
        }
        if (step_index > 0u && previous_cob >= 0.0f && fabsf(previous_cob - state.meal_cob_g) > 0.01f)
        {
            saw_cob_change = true;
        }
        if (step_index >= 12u && (state.insulin_iob_u > 0.0f || state.meal_cob_g > 0.0f))
        {
            saw_values_after_30s = true;
        }
        if (state.meal_event)
        {
            saw_meal_event = true;
        }
        if (state.activity_factor < 1.0f)
        {
            saw_exercise_window = true;
        }
        if (state.dawn_factor > 1.0f)
        {
            saw_dawn_window = true;
        }

        previous_iob = state.insulin_iob_u;
        previous_cob = state.meal_cob_g;
        delivered_insulin_u_hr = (state.bg_mgdl > 150u) ? 2.0f : 0.8f;
    }

    require_true("virtual patient bg changes", saw_bg_change);
    require_true("virtual patient iob changes", saw_iob_change);
    require_true("virtual patient cob changes", saw_cob_change);
    require_true("virtual patient values after 30s", saw_values_after_30s);
    require_true("virtual patient meal event", saw_meal_event);
    require_true("virtual patient exercise window", saw_exercise_window);
    require_true("virtual patient dawn window", saw_dawn_window);
}

int main(void)
{
    ApsPhysiology_Reset();
    PredictorV2_SetEnabled(true);
    PredictorV2_SetModelBundleForTesting(NULL);
    test_predictor_outputs();
    test_predictor_feature_vector_construction();
    test_predictor_physiology_feature_integration();
    test_predictor_missing_physiology_fallback();
    test_predictor_invalid_physiology_sanitized();
    test_predictor_invalid_feature_fallback();
    test_predictor_independent_horizons();
    test_predictor_output_bounds();
    test_predictor_invalid_model_fallback();
    test_predictor_bad_sqi_and_stale_fallback();
    test_iob_engine();
    test_cob_engine();
    test_activity_engine();
    test_controller_and_safety();
    test_metrics_module();
    test_replay_loader_success();
    test_replay_loader_missing_optional_columns();
    test_scenario_runner_and_audit();
    test_safety_reason_codes();
    test_predicted_low_reason_codes();
    test_controller_blocked_reason();
    test_simulation_summary_and_audit_header();
    test_replay_fixture_regressions();
    test_bad_data_rejected_safely();
    test_physiology_feature_population();
    test_aps_demo_state_pipeline();
    test_virtual_patient_v2_continuous();

    if (g_failures != 0)
    {
        fprintf(stderr, "%d test(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }

    printf("All host foundation tests passed\n");
    return EXIT_SUCCESS;
}
