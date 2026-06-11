#include "cgm_model_runtime.h"
#include "cgm_model_generated.h"

#include <math.h>
#include <string.h>

enum
{
    CGM_MODEL_HIST_LEN = 16u,
};

typedef struct
{
    bool enabled;
    bool primed;
    uint16_t glucose_hist[CGM_MODEL_HIST_LEN];
    int16_t trend_hist_x100[CGM_MODEL_HIST_LEN];
    uint8_t sqi_hist[CGM_MODEL_HIST_LEN];
    uint8_t wr;
} cgm_model_state_t;

static cgm_model_state_t gCgmModel;

static float ClampF32(float v, float lo, float hi)
{
    if (v < lo)
    {
        return lo;
    }
    if (v > hi)
    {
        return hi;
    }
    return v;
}

static int32_t ClampI32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo)
    {
        return lo;
    }
    if (v > hi)
    {
        return hi;
    }
    return v;
}

static uint16_t HistGetGlucose(uint8_t lag, uint16_t fallback)
{
    uint8_t idx;

    if (lag >= CGM_MODEL_HIST_LEN)
    {
        return fallback;
    }
    idx = (uint8_t)((gCgmModel.wr + CGM_MODEL_HIST_LEN - lag) % CGM_MODEL_HIST_LEN);
    if ((!gCgmModel.primed) && (idx >= gCgmModel.wr))
    {
        return fallback;
    }
    if (gCgmModel.glucose_hist[idx] == 0u)
    {
        return fallback;
    }
    return gCgmModel.glucose_hist[idx];
}

static float TrainedPredictDelta15Mgdl(const cgm_model_features_t *in)
{
    float features[CGM_MODEL_GENERATED_FEATURE_COUNT];
    float glucose = (float)in->glucose_mgdl;
    float lag1 = (float)HistGetGlucose(1u, in->glucose_mgdl);
    float lag2 = (float)HistGetGlucose(2u, (uint16_t)lroundf(lag1));
    float lag3 = (float)HistGetGlucose(3u, (uint16_t)lroundf(lag2));
    float lag6 = (float)HistGetGlucose(6u, (uint16_t)lroundf(lag3));
    float lag9 = (float)HistGetGlucose(9u, (uint16_t)lroundf(lag6));
    float lag12 = (float)HistGetGlucose(12u, (uint16_t)lroundf(lag9));
    float delta1 = glucose - lag1;
    float delta3 = glucose - lag3;
    float delta6 = glucose - lag6;
    float accel13 = delta1 - delta3;
    float roll3 = (glucose + lag1 + lag2) / 3.0f;
    float roll6 = (glucose + lag1 + lag2 + lag3 + (float)HistGetGlucose(4u, in->glucose_mgdl) + (float)HistGetGlucose(5u, in->glucose_mgdl)) / 6.0f;
    float roll12 = (glucose + lag1 + lag2 + lag3 + (float)HistGetGlucose(4u, in->glucose_mgdl) +
                    (float)HistGetGlucose(5u, in->glucose_mgdl) + lag6 + (float)HistGetGlucose(7u, in->glucose_mgdl) +
                    (float)HistGetGlucose(8u, in->glucose_mgdl) + lag9 + (float)HistGetGlucose(10u, in->glucose_mgdl) +
                    (float)HistGetGlucose(11u, in->glucose_mgdl)) / 12.0f;
    uint32_t sec_day = (in->epoch_ds / 10u) % 86400u;
    float ang = 2.0f * 3.14159265f * ((float)sec_day / 86400.0f);
    float tod_sin = sinf(ang);
    float tod_cos = cosf(ang);
    uint8_t i;
    float acc;
    float v;

    features[0] = glucose;
    features[1] = lag1;
    features[2] = lag2;
    features[3] = lag3;
    features[4] = lag6;
    features[5] = lag9;
    features[6] = lag12;
    features[7] = delta1;
    features[8] = delta3;
    features[9] = delta6;
    features[10] = accel13;
    features[11] = roll3;
    features[12] = roll6;
    features[13] = roll12;
    features[14] = tod_sin;
    features[15] = tod_cos;

    acc = kCgmModelGenIntercept;
    for (i = 0u; i < CGM_MODEL_GENERATED_FEATURE_COUNT; ++i)
    {
        v = features[i];
        if (!isfinite(v))
        {
            v = kCgmModelGenMedian[i];
        }
        if (kCgmModelGenScale[i] != 0.0f)
        {
            v = (v - kCgmModelGenMean[i]) / kCgmModelGenScale[i];
        }
        acc += (kCgmModelGenCoeff[i] * v);
    }

    return acc;
}

void CgmModel_Reset(void)
{
    memset(&gCgmModel, 0, sizeof(gCgmModel));
}

void CgmModel_SetEnabled(bool enabled)
{
    gCgmModel.enabled = enabled;
}

bool CgmModel_IsEnabled(void)
{
    return gCgmModel.enabled;
}

bool CgmModel_Predict(const cgm_model_features_t *in,
                      uint16_t *pred_15m_mgdl,
                      uint16_t *pred_30m_mgdl,
                      uint8_t *confidence_pct)
{
    int32_t trend_x100;
    int32_t pred15;
    int32_t pred30;
    int32_t pred15_trained;
    int32_t pred15_linear;
    int32_t pred30_trained;
    int32_t pred30_linear;
    int32_t alpha_x100;
    uint8_t q;

    if ((in == NULL) || (pred_15m_mgdl == NULL) || (pred_30m_mgdl == NULL) || (confidence_pct == NULL))
    {
        return false;
    }

    gCgmModel.glucose_hist[gCgmModel.wr] = in->glucose_mgdl;
    gCgmModel.trend_hist_x100[gCgmModel.wr] = in->trend_mgdl_min_x100;
    gCgmModel.sqi_hist[gCgmModel.wr] = in->sqi_pct;
    gCgmModel.wr = (uint8_t)((gCgmModel.wr + 1u) % CGM_MODEL_HIST_LEN);
    if (gCgmModel.wr == 0u)
    {
        gCgmModel.primed = true;
    }
    if (!gCgmModel.enabled)
    {
        return false;
    }

    trend_x100 = (int32_t)in->trend_mgdl_min_x100;
    pred15_linear = (int32_t)in->glucose_mgdl + ((trend_x100 * 15) / 100);
    pred30_linear = (int32_t)in->glucose_mgdl + ((trend_x100 * 30) / 100);
    pred15_trained = (int32_t)in->glucose_mgdl + (int32_t)lroundf(TrainedPredictDelta15Mgdl(in));
    pred30_trained = (int32_t)in->glucose_mgdl + (2 * (pred15_trained - (int32_t)in->glucose_mgdl));

    if (in->sqi_pct >= 70u)
    {
        alpha_x100 = 75;
    }
    else if (in->sqi_pct >= 50u)
    {
        alpha_x100 = 55;
    }
    else
    {
        alpha_x100 = 35;
    }
    if ((in->sensor_flags & (1u << 1)) != 0u || (in->sensor_flags & (1u << 2)) != 0u)
    {
        alpha_x100 = 25;
    }

    pred15 = ((alpha_x100 * pred15_trained) + ((100 - alpha_x100) * pred15_linear)) / 100;
    pred30 = ((alpha_x100 * pred30_trained) + ((100 - alpha_x100) * pred30_linear)) / 100;

    pred15 = ClampI32(pred15, 40, 400);
    pred30 = ClampI32(pred30, 40, 400);

    q = in->sqi_pct;
    if ((in->sensor_flags & ((1u << 1) | (1u << 2) | (1u << 3))) != 0u)
    {
        q = (uint8_t)((q > 40u) ? (q - 40u) : 0u);
    }

    *pred_15m_mgdl = (uint16_t)pred15;
    *pred_30m_mgdl = (uint16_t)pred30;
    *confidence_pct = q;
    return true;
}
