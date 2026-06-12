#ifndef PREDICTOR_V2_GENERATED_H
#define PREDICTOR_V2_GENERATED_H

#include "aps_types.h"

typedef enum
{
    PREDICTOR_V2_FEATURE_CURRENT_GLUCOSE = 0u,
    PREDICTOR_V2_FEATURE_LAG_1 = 1u,
    PREDICTOR_V2_FEATURE_LAG_2 = 2u,
    PREDICTOR_V2_FEATURE_LAG_3 = 3u,
    PREDICTOR_V2_FEATURE_LAG_6 = 4u,
    PREDICTOR_V2_FEATURE_LAG_9 = 5u,
    PREDICTOR_V2_FEATURE_LAG_12 = 6u,
    PREDICTOR_V2_FEATURE_DELTA_1 = 7u,
    PREDICTOR_V2_FEATURE_DELTA_3 = 8u,
    PREDICTOR_V2_FEATURE_DELTA_6 = 9u,
    PREDICTOR_V2_FEATURE_SLOPE_15M = 10u,
    PREDICTOR_V2_FEATURE_SLOPE_30M = 11u,
    PREDICTOR_V2_FEATURE_SLOPE_60M = 12u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_3 = 13u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_6 = 14u,
    PREDICTOR_V2_FEATURE_ROLL_MEAN_12 = 15u,
    PREDICTOR_V2_FEATURE_ROLL_VAR_6 = 16u,
    PREDICTOR_V2_FEATURE_ROLL_STDDEV_6 = 17u,
    PREDICTOR_V2_FEATURE_VOLATILITY_SCORE = 18u,
    PREDICTOR_V2_FEATURE_IOB = 19u,
    PREDICTOR_V2_FEATURE_COB = 20u,
    PREDICTOR_V2_FEATURE_BASAL_RATE = 21u,
    PREDICTOR_V2_FEATURE_INSULIN_30M = 22u,
    PREDICTOR_V2_FEATURE_INSULIN_120M = 23u,
    PREDICTOR_V2_FEATURE_CARBS_30M = 24u,
    PREDICTOR_V2_FEATURE_CARBS_120M = 25u,
    PREDICTOR_V2_FEATURE_SQI = 26u,
    PREDICTOR_V2_FEATURE_CGM_AGE_S = 27u,
    PREDICTOR_V2_FEATURE_TOD_SIN = 28u,
    PREDICTOR_V2_FEATURE_TOD_COS = 29u,
    PREDICTOR_V2_FEATURE_ACTIVITY_STATE = 30u,
    PREDICTOR_V2_FEATURE_ACTIVITY_CONFIDENCE = 31u,
    PREDICTOR_V2_FEATURE_MOTION_RMS_5M = 32u,
    PREDICTOR_V2_FEATURE_MOTION_RMS_15M = 33u,
    PREDICTOR_V2_FEATURE_ACTIVE_MINUTES = 34u,
    PREDICTOR_V2_FEATURE_POST_EXERCISE_MINUTES = 35u,
    PREDICTOR_V2_FEATURE_COUNT = 36u
} predictor_v2_feature_index_t;

typedef struct
{
    float values[PREDICTOR_V2_FEATURE_COUNT];
    uint64_t valid_mask;
} predictor_v2_feature_vector_t;

typedef enum
{
    PREDICTOR_V2_HORIZON_15M = 0u,
    PREDICTOR_V2_HORIZON_30M = 1u,
    PREDICTOR_V2_HORIZON_60M = 2u
} predictor_v2_horizon_t;

typedef struct
{
    float feature_mean[PREDICTOR_V2_FEATURE_COUNT];
    float feature_scale[PREDICTOR_V2_FEATURE_COUNT];
    float feature_median[PREDICTOR_V2_FEATURE_COUNT];
    float coefficients[PREDICTOR_V2_FEATURE_COUNT];
    float intercept;
    bool valid;
} predictor_v2_generated_model_t;

typedef struct
{
    const predictor_v2_generated_model_t *horizon_15m;
    const predictor_v2_generated_model_t *horizon_30m;
    const predictor_v2_generated_model_t *horizon_60m;
} predictor_v2_generated_bundle_t;

typedef struct
{
    uint16_t prediction_mgdl;
    int16_t delta_mgdl;
    uint8_t confidence_pct;
    uint16_t risk_flags;
    uint16_t status_flags;
    bool used_fallback;
} predictor_v2_horizon_eval_t;

#define PREDICTOR_V2_STATUS_VALID_MODEL (1u << 0)
#define PREDICTOR_V2_STATUS_INVALID_MODEL (1u << 1)
#define PREDICTOR_V2_STATUS_INVALID_FEATURES (1u << 2)
#define PREDICTOR_V2_STATUS_FALLBACK_LINEAR (1u << 3)
#define PREDICTOR_V2_STATUS_LOW_CONFIDENCE (1u << 4)
#define PREDICTOR_V2_STATUS_BAD_SQI (1u << 5)
#define PREDICTOR_V2_STATUS_STALE_CGM (1u << 6)
#define PREDICTOR_V2_STATUS_MISSING_PHYSIOLOGY (1u << 7)

const predictor_v2_generated_bundle_t *PredictorV2Generated_DefaultBundle(void);

#endif
