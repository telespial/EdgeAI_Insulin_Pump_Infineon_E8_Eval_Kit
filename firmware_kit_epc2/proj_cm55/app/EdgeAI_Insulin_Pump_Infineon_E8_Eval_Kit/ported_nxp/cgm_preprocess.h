#ifndef CGM_PREPROCESS_H
#define CGM_PREPROCESS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t adc_counts;
    float ref_mv;
    float temp_c;
} cgm_raw_sample_t;

typedef struct
{
    bool output_ready;
    float glucose_mgdl;
    float glucose_filtered_mgdl;
    float trend_mgdl_min;
    float sensor_current_na;
    uint8_t sqi_pct;
    float adaptive_alpha;
    float calibration_age_days;
    float drift_comp_mgdl;
    bool calibration_stale;
    bool drift_warn;
    bool sensitivity_change;
    float lag_comp_mgdl;
    float lag_gain_applied;
    uint16_t sensor_flags;
    bool prediction_blocked;
    bool hold_last;
} cgm_preprocess_output_t;

enum
{
    CGM_FLAG_SATURATION = (1u << 0),
    CGM_FLAG_DROPOUT = (1u << 1),
    CGM_FLAG_IMPLAUSIBLE_ROC = (1u << 2),
    CGM_FLAG_TEMP_OUT_OF_RANGE = (1u << 3),
    CGM_FLAG_CAL_STALE = (1u << 4),
    CGM_FLAG_DRIFT_WARN = (1u << 5),
};

typedef struct
{
    float adc_counts_to_na;
    float adc_offset_counts;
    float linear_k2;
    float ref_nominal_mv;
    float anti_alias_alpha;
    uint8_t decimation_factor;
    bool notch_enable;
    float notch_f0_hz;
    float notch_q;
    float sample_rate_hz;
    float cal_gain_mgdl_per_na;
    float cal_offset_mgdl;
    float temp_coeff_mgdl_per_c;
    float temp_ref_c;
    float filter_alpha_min;
    float filter_alpha_max;
    float noise_ref_na;
    float trend_alpha_min;
    float trend_alpha_max;
    float calibration_stale_days;
    float drift_alpha;
    float drift_limit_mgdl;
    float drift_warn_mgdl;
    float sensitivity_short_alpha;
    float sensitivity_long_alpha;
    float sensitivity_ratio_low;
    float sensitivity_ratio_high;
    uint8_t sensitivity_hold_samples;
    bool lag_enable;
    float lag_tau_s;
    float lag_gain_max;
    float lag_gain_min;
    float lag_comp_limit_mgdl;
} cgm_preprocess_config_t;

typedef struct
{
    cgm_preprocess_config_t cfg;
    bool aa_primed;
    float aa_state;
    float decim_sum;
    uint8_t decim_count;
    float median_hist[3];
    uint8_t median_fill;
    float notch_b0;
    float notch_b1;
    float notch_b2;
    float notch_a1;
    float notch_a2;
    float notch_x1;
    float notch_x2;
    float notch_y1;
    float notch_y2;
    bool trend_primed;
    float prev_glucose_mgdl;
    float prev_glucose_filt_mgdl;
    float prev_trend_mgdl_min;
    float noise_ema_na;
    float cal_gain_runtime;
    float cal_offset_runtime;
    float calibration_age_days;
    float drift_state_mgdl;
    float sensitivity_short_ema;
    float sensitivity_long_ema;
    uint8_t sensitivity_counter;
    bool sensitivity_change_active;
    float prev_sensor_current_na;
    float lag_state_mgdl;
    float prev_kinetic_mgdl;
} cgm_preprocess_t;

typedef struct
{
    uint16_t glucose_mgdl;
    int16_t trend_mgdl_min_x100;
    uint8_t sqi_pct;
    uint16_t sensor_flags;
    uint32_t epoch_ds;
} cgm_model_features_t;

void CgmPreprocess_Init(cgm_preprocess_t *st, const cgm_preprocess_config_t *cfg);
void CgmPreprocess_InitDefault(cgm_preprocess_t *st);
void CgmPreprocess_SetCalibration(cgm_preprocess_t *st, float gain_mgdl_per_na, float offset_mgdl);
void CgmPreprocess_ResetCalibrationAge(cgm_preprocess_t *st);
void CgmPreprocess_Push(cgm_preprocess_t *st,
                        const cgm_raw_sample_t *sample,
                        cgm_preprocess_output_t *out);

void CgmModel_Reset(void);
void CgmModel_SetEnabled(bool enabled);
bool CgmModel_IsEnabled(void);
bool CgmModel_Predict(const cgm_model_features_t *in,
                      uint16_t *pred_15m_mgdl,
                      uint16_t *pred_30m_mgdl,
                      uint8_t *confidence_pct);

#endif /* CGM_PREPROCESS_H */
