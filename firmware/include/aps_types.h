#ifndef APS_TYPES_H
#define APS_TYPES_H

#include <stdbool.h>
#include <stdint.h>

#define APS_TICK_SECONDS 300u
#define APS_HISTORY_LEN 64u
#define APS_EVENT_HISTORY_LEN 256u

#define APS_SENSOR_FLAG_STALE_CGM (1u << 0)
#define APS_SENSOR_FLAG_BAD_SQI (1u << 1)
#define APS_SENSOR_FLAG_INVALID (1u << 2)

#define APS_RISK_PRED_LOW_15M (1u << 0)
#define APS_RISK_PRED_LOW_30M (1u << 1)
#define APS_RISK_PRED_LOW_60M (1u << 2)
#define APS_RISK_PRED_HIGH_30M (1u << 3)
#define APS_RISK_PRED_HIGH_60M (1u << 4)
#define APS_RISK_RAPID_FALL (1u << 5)
#define APS_RISK_RAPID_RISE (1u << 6)
#define APS_RISK_LOW_CONFIDENCE (1u << 7)
#define APS_RISK_BAD_SENSOR_QUALITY (1u << 8)
#define APS_RISK_STALE_CGM (1u << 9)

#define APS_SAFETY_REASON_STALE_CGM (1u << 0)
#define APS_SAFETY_REASON_BAD_SQI (1u << 1)
#define APS_SAFETY_REASON_PREDICTED_LOW_15M (1u << 2)
#define APS_SAFETY_REASON_PREDICTED_LOW_30M (1u << 3)
#define APS_SAFETY_REASON_RAPID_FALL (1u << 4)
#define APS_SAFETY_REASON_EXCESSIVE_IOB (1u << 5)
#define APS_SAFETY_REASON_IMPOSSIBLE_GLUCOSE (1u << 6)
#define APS_SAFETY_REASON_LOW_CONFIDENCE (1u << 7)
#define APS_SAFETY_REASON_MISSING_PHYSIOLOGY (1u << 8)
#define APS_SAFETY_REASON_CONTROLLER_BLOCKED (1u << 9)
#define APS_SAFETY_REASON_ALL (APS_SAFETY_REASON_STALE_CGM | \
                               APS_SAFETY_REASON_BAD_SQI | \
                               APS_SAFETY_REASON_PREDICTED_LOW_15M | \
                               APS_SAFETY_REASON_PREDICTED_LOW_30M | \
                               APS_SAFETY_REASON_RAPID_FALL | \
                               APS_SAFETY_REASON_EXCESSIVE_IOB | \
                               APS_SAFETY_REASON_IMPOSSIBLE_GLUCOSE | \
                               APS_SAFETY_REASON_LOW_CONFIDENCE | \
                               APS_SAFETY_REASON_MISSING_PHYSIOLOGY)

typedef struct
{
    uint32_t epoch_s;
    uint32_t age_s;
    uint16_t glucose_mgdl;
    int16_t trend_mgdl_min_x100;
    uint8_t sqi_pct;
    uint16_t sensor_flags;
    bool valid;
} aps_cgm_sample_t;

typedef struct
{
    uint32_t epoch_s;
    float bolus_u;
    float basal_u;
    float carbs_g;
    bool exercise_state;
    bool stress_state;
    bool illness_state;
} aps_event_t;

typedef struct
{
    float iob_u;
    float insulin_activity_u_per_hr;
    float cob_g;
    float carb_absorption_g_per_hr;
    float basal_u_per_hr;
    float insulin_30m_u;
    float insulin_120m_u;
    float carbs_30m_g;
    float carbs_120m_g;
    uint16_t minutes_since_bolus;
    uint16_t minutes_since_meal;
} aps_physiology_state_t;

typedef struct
{
    aps_cgm_sample_t cgm;
    aps_physiology_state_t physiology;
    bool physiology_present;
} predictor_v2_input_t;

typedef struct
{
    uint16_t pred_15m_mgdl;
    uint16_t pred_30m_mgdl;
    uint16_t pred_60m_mgdl;
    int16_t delta_15m_mgdl;
    int16_t delta_30m_mgdl;
    int16_t delta_60m_mgdl;
    uint8_t confidence_pct;
    uint16_t risk_flags;
    uint16_t model_status_flags;
} predictor_v2_output_t;

typedef enum
{
    APS_ACTION_NO_CHANGE = 0,
    APS_ACTION_REDUCE_BASAL,
    APS_ACTION_SUSPEND_BASAL,
    APS_ACTION_INCREASE_BASAL,
    APS_ACTION_CORRECTION_SUGGESTION
} aps_action_t;

typedef struct
{
    aps_action_t action;
    float requested_basal_u_per_hr;
    float suggested_correction_u;
    uint32_t reason_flags;
} aps_controller_output_t;

#endif
