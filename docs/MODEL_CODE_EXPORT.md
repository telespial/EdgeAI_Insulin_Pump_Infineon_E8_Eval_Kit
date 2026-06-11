# CGM Model Code Export

This file collects the model-related source used by the `EdgeAI_Insulin_Pump_Infineon_E8_Eval_Kit` project in one place for review.

## `cgm_model_runtime.c`

```c
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
```

## `cgm_model_runtime.h`

```c
#ifndef CGM_MODEL_RUNTIME_H
#define CGM_MODEL_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t glucose_mgdl;
    int16_t trend_mgdl_min_x100;
    uint8_t sqi_pct;
    uint16_t sensor_flags;
    uint32_t epoch_ds;
} cgm_model_features_t;

void CgmModel_Reset(void);
void CgmModel_SetEnabled(bool enabled);
bool CgmModel_IsEnabled(void);
bool CgmModel_Predict(const cgm_model_features_t *in,
                      uint16_t *pred_15m_mgdl,
                      uint16_t *pred_30m_mgdl,
                      uint8_t *confidence_pct);

#endif /* CGM_MODEL_RUNTIME_H */
```

## `cgm_model_generated.h`

```c
#ifndef CGM_MODEL_GENERATED_H
#define CGM_MODEL_GENERATED_H

#include <stdint.h>

#define CGM_MODEL_GENERATED_FEATURE_COUNT 16u

/* Auto-generated by tools/train_cgm_compare_export.py */
/* Predicts +15m delta (mg/dL); runtime converts to absolute. */

static const float kCgmModelGenMedian[CGM_MODEL_GENERATED_FEATURE_COUNT] = {
    153.0f, 153.0f, 153.0f, 153.0f, 153.0f, 153.0f,
    153.0f, 0.0f, 0.0f, -1.0f, 0.0f, 152.333333f,
    152.166667f, 151.833333f, 0.00861745638f, -0.042711209f,
};

static const float kCgmModelGenMean[CGM_MODEL_GENERATED_FEATURE_COUNT] = {
    161.508717f, 161.502069f, 161.495715f, 161.461436f, 161.436909f, 161.433067f,
    161.422725f, 0.00664893617f, 0.0472813239f, 0.0718085106f, -0.0406323877f, 161.502167f,
    161.473552f, 161.45315f, 0.00866457187f, -0.0164534069f,
};

static const float kCgmModelGenScale[CGM_MODEL_GENERATED_FEATURE_COUNT] = {
    72.5630091f, 72.5693318f, 72.5765104f, 72.5748472f, 72.5857945f, 72.5904055f,
    72.6006634f, 9.24792083f, 17.6312994f, 29.8260003f, 13.4219453f, 72.2971856f,
    71.8717907f, 70.5941395f, 0.697346053f, 0.71649333f,
};

static const float kCgmModelGenCoeff[CGM_MODEL_GENERATED_FEATURE_COUNT] = {
    0.383436782f, 0.102574745f, -0.552429701f, 0.0605640154f, 0.225603058f, 0.00224054712f,
    -0.019290345f, 0.280862037f, 0.322872767f, 0.157833724f, -0.0420107294f, -0.0221393915f,
    -0.345387833f, 0.146296076f, -0.114660303f, 0.683814933f,
};

static const float kCgmModelGenIntercept = 2.74074935f;

#endif
```

## `cgm_replay_subject001.h`

```c
#ifndef CGM_REPLAY_SUBJECT001_H
#define CGM_REPLAY_SUBJECT001_H

#include <stdint.h>

#define CGM_REPLAY_SUBJECT001_LEN 1438u

static const uint16_t kCgmReplaySubject001Mgdl[CGM_REPLAY_SUBJECT001_LEN] = {
    185u, 178u, 169u, 176u, 173u, 169u, 166u, 160u, 157u, 151u, 148u, 144u, 142u, 142u, 140u, 140u, 
    139u, 135u, 135u, 135u, 137u, 140u, 144u, 148u, 149u, 149u, 149u, 148u, 144u, 142u, 140u, 137u, 
    135u, 133u, 135u, 137u, 140u, 144u, 149u, 153u, 157u, 157u, 158u, 158u, 158u, 160u, 164u, 167u, 
    171u, 176u, 180u, 185u, 187u, 191u, 194u, 194u, 194u, 194u, 193u, 191u, 187u, 184u, 184u, 180u, 
    178u, 180u, 185u, 193u, 200u, 207u, 214u, 216u, 220u, 220u, 218u, 216u, 214u, 211u, 207u, 205u, 
    202u, 198u, 196u, 194u, 191u, 187u, 184u, 180u, 176u, 175u, 173u, 171u, 171u, 173u, 176u, 178u, 
    180u, 184u, 184u, 184u, 180u, 178u, 175u, 171u, 167u, 164u, 158u, 155u, 151u, 148u, 144u, 142u, 
    140u, 140u, 142u, 142u, 142u, 142u, 140u, 139u, 135u, 133u, 131u, 128u, 126u, 124u, 124u, 122u, 
    122u, 121u, 119u, 117u, 115u, 113u, 112u, 110u, 106u, 104u, 103u, 101u, 99u, 97u, 95u, 94u, 
    92u, 92u, 94u, 97u, 103u, 106u, 110u, 110u, 110u, 112u, 115u, 121u, 128u, 135u, 144u, 151u, 
    157u, 160u, 162u, 164u, 166u, 166u, 167u, 167u, 167u, 167u, 166u, 166u, 164u, 162u, 162u, 160u, 
    160u, 158u, 158u, 160u, 160u, 166u, 166u, 169u, 178u, 187u, 200u, 212u, 227u, 239u, 250u, 257u, 
    259u, 259u, 257u, 256u, 254u, 256u, 257u, 259u, 261u, 265u, 265u, 265u, 263u, 259u, 257u, 252u, 
    248u, 243u, 241u, 239u, 238u, 234u, 230u, 227u, 221u, 218u, 214u, 212u, 211u, 207u, 205u, 202u, 
    198u, 194u, 191u, 187u, 184u, 182u, 182u, 184u, 185u, 187u, 191u, 193u, 194u, 196u, 198u, 202u, 
    203u, 203u, 203u, 202u, 198u, 196u, 193u, 189u, 185u, 182u, 178u, 178u, 176u, 176u, 175u, 175u, 
    171u, 171u, 169u, 166u, 164u, 162u, 160u, 169u, 158u, 157u, 158u, 160u, 166u, 171u, 178u, 187u, 
    194u, 203u, 214u, 223u, 230u, 236u, 239u, 241u, 239u, 238u, 234u, 230u, 225u, 220u, 214u, 209u, 
    205u, 202u, 200u, 200u, 198u, 198u, 196u, 198u, 194u, 193u, 189u, 189u, 189u, 191u, 194u, 202u, 
    211u, 221u, 238u, 254u, 270u, 286u, 301u, 311u, 319u, 322u, 324u, 324u, 324u, 324u, 324u, 322u, 
    319u, 315u, 310u, 304u, 301u, 295u, 292u, 288u, 286u, 284u, 283u, 279u, 270u, 277u, 274u, 274u, 
    274u, 279u, 286u, 295u, 304u, 311u, 319u, 320u, 322u, 322u, 322u, 320u, 319u, 315u, 313u, 310u, 
    304u, 301u, 295u, 290u, 284u, 279u, 274u, 266u, 261u, 254u, 254u, 248u, 241u, 238u, 232u, 225u, 
    220u, 214u, 207u, 203u, 198u, 194u, 189u, 185u, 180u, 175u, 169u, 164u, 158u, 153u, 149u, 144u, 
    139u, 133u, 126u, 121u, 113u, 106u, 101u, 95u, 92u, 86u, 83u, 79u, 76u, 74u, 70u, 68u, 
    65u, 63u, 61u, 58u, 56u, 54u, 52u, 50u, 49u, 47u, 47u, 45u, 43u, 43u, 43u, 43u, 
    45u, 45u, 45u, 45u, 45u, 45u, 45u, 43u, 43u, 43u, 43u, 41u, 41u, 41u, 40u, 40u, 
    40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 
    41u, 41u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 40u, 41u, 45u, 50u, 56u, 
    61u, 63u, 65u, 63u, 61u, 61u, 59u, 59u, 61u, 63u, 67u, 72u, 68u, 70u, 74u, 76u, 
    79u, 86u, 94u, 101u, 110u, 117u, 126u, 135u, 144u, 153u, 160u, 167u, 175u, 178u, 182u, 184u, 
    184u, 182u, 180u, 178u, 176u, 175u, 171u, 171u, 158u, 171u, 171u, 175u, 178u, 184u, 189u, 196u, 
    202u, 203u, 205u, 209u, 212u, 216u, 220u, 223u, 225u, 225u, 227u, 229u, 230u, 234u, 238u, 241u, 
    247u, 250u, 252u, 256u, 256u, 254u, 254u, 254u, 257u, 261u, 270u, 279u, 288u, 299u, 310u, 317u, 
    324u, 329u, 337u, 344u, 353u, 364u, 376u, 389u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 
    400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 
    400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 400u, 398u, 391u, 383u, 
    378u, 374u, 369u, 367u, 365u, 362u, 358u, 353u, 347u, 340u, 337u, 333u, 331u, 329u, 329u, 328u, 
    328u, 328u, 328u, 337u, 328u, 328u, 328u, 326u, 324u, 322u, 320u, 315u, 311u, 308u, 304u, 301u, 
    301u, 299u, 301u, 302u, 304u, 310u, 311u, 315u, 315u, 313u, 310u, 306u, 302u, 299u, 295u, 293u, 
    292u, 292u, 292u, 292u, 293u, 293u, 292u, 292u, 288u, 286u, 284u, 279u, 277u, 274u, 270u, 268u, 
    266u, 265u, 265u, 263u, 261u, 261u, 261u, 259u, 257u, 256u, 254u, 250u, 248u, 247u, 243u, 241u, 
    239u, 238u, 234u, 232u, 230u, 229u, 225u, 221u, 216u, 212u, 207u, 203u, 200u, 196u, 194u, 193u, 
    193u, 193u, 189u, 189u, 187u, 185u, 184u, 180u, 178u, 175u, 171u, 166u, 162u, 157u, 153u, 151u, 
    149u, 148u, 144u, 144u, 142u, 142u, 140u, 139u, 135u, 133u, 131u, 130u, 128u, 126u, 124u, 113u, 
    122u, 121u, 117u, 117u, 119u, 124u, 131u, 140u, 149u, 158u, 162u, 162u, 162u, 158u, 153u, 149u, 
    148u, 146u, 144u, 144u, 142u, 139u, 133u, 128u, 121u, 113u, 106u, 101u, 95u, 92u, 86u, 85u, 
    81u, 77u, 76u, 74u, 76u, 68u, 77u, 81u, 86u, 94u, 99u, 106u, 113u, 121u, 128u, 133u, 
    139u, 140u, 139u, 133u, 126u, 117u, 110u, 103u, 95u, 88u, 83u, 77u, 74u, 70u, 68u, 67u, 
    65u, 61u, 59u, 58u, 56u, 52u, 50u, 49u, 61u, 47u, 45u, 45u, 45u, 47u, 47u, 49u, 
    52u, 56u, 59u, 67u, 72u, 79u, 88u, 97u, 106u, 113u, 121u, 126u, 131u, 137u, 140u, 142u, 
    144u, 148u, 151u, 155u, 158u, 160u, 162u, 162u, 162u, 162u, 162u, 164u, 167u, 169u, 171u, 173u, 
    175u, 175u, 176u, 178u, 178u, 176u, 176u, 175u, 176u, 176u, 176u, 176u, 176u, 185u, 175u, 175u, 
    175u, 175u, 175u, 175u, 175u, 173u, 171u, 169u, 169u, 169u, 171u, 173u, 175u, 175u, 175u, 175u, 
    173u, 171u, 167u, 167u, 166u, 167u, 169u, 171u, 175u, 178u, 185u, 191u, 200u, 211u, 241u, 223u, 
    238u, 254u, 270u, 290u, 310u, 329u, 347u, 362u, 373u, 378u, 382u, 383u, 380u, 378u, 376u, 374u, 
    374u, 376u, 376u, 374u, 374u, 373u, 371u, 374u, 376u, 380u, 383u, 387u, 389u, 392u, 394u, 396u, 
    398u, 398u, 400u, 400u, 396u, 392u, 387u, 382u, 378u, 356u, 371u, 367u, 360u, 355u, 347u, 342u, 
    337u, 329u, 324u, 319u, 313u, 306u, 302u, 297u, 293u, 288u, 284u, 281u, 277u, 272u, 268u, 265u, 
    259u, 256u, 254u, 250u, 248u, 247u, 243u, 241u, 239u, 238u, 234u, 232u, 229u, 225u, 221u, 220u, 
    216u, 212u, 211u, 207u, 203u, 202u, 198u, 196u, 194u, 193u, 189u, 187u, 185u, 184u, 182u, 180u, 
    178u, 176u, 173u, 171u, 169u, 166u, 164u, 160u, 158u, 157u, 153u, 151u, 151u, 149u, 148u, 146u, 
    144u, 144u, 142u, 140u, 139u, 137u, 133u, 131u, 130u, 126u, 124u, 122u, 121u, 119u, 117u, 115u, 
    115u, 113u, 113u, 113u, 113u, 113u, 112u, 112u, 110u, 112u, 113u, 121u, 130u, 142u, 158u, 176u, 
    193u, 211u, 225u, 238u, 247u, 252u, 254u, 254u, 250u, 247u, 243u, 241u, 239u, 236u, 230u, 221u, 
    212u, 202u, 191u, 180u, 171u, 162u, 153u, 148u, 140u, 133u, 126u, 117u, 110u, 101u, 92u, 85u, 
    79u, 74u, 72u, 67u, 72u, 74u, 77u, 83u, 88u, 94u, 95u, 97u, 97u, 95u, 92u, 90u, 
    86u, 83u, 79u, 76u, 74u, 70u, 68u, 68u, 67u, 65u, 65u, 65u, 63u, 65u, 61u, 63u, 
    63u, 65u, 68u, 72u, 76u, 83u, 88u, 97u, 108u, 117u, 128u, 139u, 144u, 151u, 155u, 157u, 
    155u, 153u, 151u, 148u, 142u, 139u, 133u, 130u, 130u, 130u, 131u, 137u, 140u, 146u, 149u, 153u, 
    157u, 157u, 155u, 151u, 148u, 142u, 139u, 139u, 139u, 140u, 142u, 146u, 148u, 149u, 149u, 148u, 
    144u, 142u, 139u, 133u, 131u, 130u, 128u, 126u, 126u, 126u, 126u, 126u, 128u, 130u, 133u, 135u, 
    135u, 135u, 135u, 133u, 131u, 130u, 128u, 128u, 130u, 133u, 140u, 153u, 167u, 238u, 187u, 205u, 
    223u, 238u, 248u, 256u, 257u, 257u, 256u, 250u, 247u, 239u, 234u, 230u, 223u, 220u, 212u, 205u, 
    173u, 198u, 193u, 185u, 184u, 184u, 185u, 189u, 193u, 194u, 196u, 196u, 196u, 198u, 202u, 205u, 
    214u, 223u, 232u, 241u, 250u, 254u, 256u, 254u, 250u, 248u, 247u, 248u, 250u, 252u, 254u, 248u, 
    252u, 248u, 243u, 239u, 238u, 236u, 234u, 234u, 234u, 232u, 230u, 229u, 223u, 220u, 216u, 212u, 
    211u, 207u, 205u, 205u, 203u, 205u, 205u, 207u, 209u, 209u, 205u, 203u, 200u, 196u, 193u, 189u, 
    187u, 185u, 184u, 180u, 178u, 175u, 171u, 169u, 167u, 166u, 166u, 166u, 166u, 166u, 164u, 162u, 
    162u, 160u, 158u, 157u, 157u, 153u, 151u, 149u, 148u, 144u, 140u, 137u, 135u, 131u, 130u, 130u, 
    128u, 126u, 126u, 124u, 122u, 122u, 121u, 119u, 117u, 115u, 112u, 110u, 104u, 103u, 99u, 97u, 
    95u, 94u, 95u, 95u, 97u, 99u, 99u, 97u, 97u, 95u, 94u, 92u, 90u, 92u, 94u, 101u, 
    95u, 112u, 128u, 146u, 164u, 180u, 196u, 207u, 216u, 221u, 225u, 225u, 221u, 216u, 205u, 194u, 
    184u, 171u, 162u, 153u, 144u, 135u, 126u, 117u, 110u, 103u, 95u, 92u, 86u, 81u, 76u, 72u, 
    70u, 63u, 70u, 74u, 77u, 85u, 94u, 101u, 106u, 113u, 117u, 121u, 121u, 121u, 115u, 112u, 
    108u, 104u, 104u, 103u, 103u, 103u, 103u, 101u, 101u, 97u, 95u, 94u, 92u, 90u, 88u, 86u, 
    86u, 86u, 85u, 83u, 79u, 76u, 72u, 67u, 63u, 59u, 59u, 59u, 61u, 67u, 74u, 81u, 
    86u, 94u, 97u, 99u, 101u, 103u, 104u, 108u, 112u, 113u, 117u, 122u, 126u, 133u, 139u, 146u, 
    151u, 158u, 166u, 171u, 175u, 176u, 176u, 176u, 176u, 178u, 182u, 184u, 185u, 185u, 187u, 189u, 
    193u, 193u, 194u, 196u, 198u, 202u, 205u, 211u, 216u, 221u, 225u, 225u, 221u, 218u
};

#endif
```
