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
