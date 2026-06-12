#include "predictor_v2_generated.h"

static const predictor_v2_generated_model_t k_model_15m =
{
    .feature_mean = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 20.0f, 10.0f, 12.0f, 1.0f,
        15.0f, 0.8f, 0.5f, 2.0f, 0.0f, 10.0f, 95.0f, 0.0f, 0.0f, 0.0f,
    },
    .feature_scale = {
        40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 20.0f, 20.0f, 20.0f,
        1.5f, 1.5f, 1.5f, 25.0f, 25.0f, 25.0f, 15.0f, 10.0f, 10.0f, 1.0f,
        20.0f, 0.4f, 0.5f, 1.5f, 10.0f, 15.0f, 20.0f, 300.0f, 1.0f, 1.0f,
    },
    .feature_median = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 8.0f, 6.0f, 8.0f, 1.0f,
        10.0f, 0.8f, 0.5f, 2.0f, 0.0f, 8.0f, 95.0f, 0.0f, 0.0f, 1.0f,
    },
    .coefficients = {
        0.4f, 0.6f, 0.4f, 0.2f, 0.0f, 0.0f, 0.0f, 2.4f, 1.2f, 0.6f,
        7.5f, 4.0f, 1.5f, 0.2f, 0.3f, 0.2f, 0.4f, 0.2f, 0.8f, -8.5f,
        3.4f, -1.0f, -2.0f, -0.8f, 1.8f, 0.4f, 0.3f, -0.01f, 2.0f, 1.0f,
    },
    .intercept = 0.0f,
    .valid = true,
};

static const predictor_v2_generated_model_t k_model_30m =
{
    .feature_mean = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 20.0f, 10.0f, 12.0f, 1.0f,
        15.0f, 0.8f, 0.5f, 2.0f, 0.0f, 10.0f, 95.0f, 0.0f, 0.0f, 0.0f,
    },
    .feature_scale = {
        40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 20.0f, 20.0f, 20.0f,
        1.5f, 1.5f, 1.5f, 25.0f, 25.0f, 25.0f, 15.0f, 10.0f, 10.0f, 1.0f,
        20.0f, 0.4f, 0.5f, 1.5f, 10.0f, 15.0f, 20.0f, 300.0f, 1.0f, 1.0f,
    },
    .feature_median = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 8.0f, 6.0f, 8.0f, 1.0f,
        10.0f, 0.8f, 0.5f, 2.0f, 0.0f, 8.0f, 95.0f, 0.0f, 0.0f, 1.0f,
    },
    .coefficients = {
        0.5f, 0.5f, 0.5f, 0.3f, 0.1f, 0.0f, 0.0f, 1.8f, 1.4f, 1.0f,
        4.5f, 7.0f, 2.5f, 0.2f, 0.3f, 0.3f, 0.6f, 0.3f, 0.8f, -10.0f,
        4.1f, -1.1f, -2.1f, -0.9f, 2.0f, 0.5f, 0.3f, -0.02f, 1.5f, 1.5f,
    },
    .intercept = 0.0f,
    .valid = true,
};

static const predictor_v2_generated_model_t k_model_60m =
{
    .feature_mean = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 20.0f, 10.0f, 12.0f, 1.0f,
        15.0f, 0.8f, 0.5f, 2.0f, 0.0f, 10.0f, 95.0f, 0.0f, 0.0f, 0.0f,
    },
    .feature_scale = {
        40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 40.0f, 20.0f, 20.0f, 20.0f,
        1.5f, 1.5f, 1.5f, 25.0f, 25.0f, 25.0f, 15.0f, 10.0f, 10.0f, 1.0f,
        20.0f, 0.4f, 0.5f, 1.5f, 10.0f, 15.0f, 20.0f, 300.0f, 1.0f, 1.0f,
    },
    .feature_median = {
        140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 140.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 140.0f, 140.0f, 140.0f, 8.0f, 6.0f, 8.0f, 1.0f,
        10.0f, 0.8f, 0.5f, 2.0f, 0.0f, 8.0f, 95.0f, 0.0f, 0.0f, 1.0f,
    },
    .coefficients = {
        0.4f, 0.4f, 0.6f, 0.4f, 0.2f, 0.1f, 0.1f, 1.2f, 1.5f, 1.8f,
        2.5f, 4.0f, 8.5f, 0.2f, 0.3f, 0.4f, 0.8f, 0.4f, 0.9f, -11.5f,
        4.5f, -1.2f, -2.2f, -1.0f, 2.2f, 0.5f, 0.3f, -0.03f, 1.0f, 1.6f,
    },
    .intercept = 0.0f,
    .valid = true,
};

static const predictor_v2_generated_bundle_t k_default_bundle =
{
    .horizon_15m = &k_model_15m,
    .horizon_30m = &k_model_30m,
    .horizon_60m = &k_model_60m,
};

const predictor_v2_generated_bundle_t *PredictorV2Generated_DefaultBundle(void)
{
    return &k_default_bundle;
}
