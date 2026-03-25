#ifndef EDGEAI_ANOMALY_ENGINE_H
#define EDGEAI_ANOMALY_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    ANOMALY_MODE_ADAPTIVE_BASELINE = 0,
    ANOMALY_MODE_TRAINED_MONITOR = 1,
    ANOMALY_MODE_STATIC_LIMITS = 2,
} anomaly_mode_t;

typedef enum
{
    ANOMALY_TUNE_LOOSE = 0,
    ANOMALY_TUNE_NORMAL = 1,
    ANOMALY_TUNE_STRICT = 2,
} anomaly_tune_t;

typedef enum
{
    ANOMALY_LEVEL_IGNORE = 0,
    ANOMALY_LEVEL_WATCH = 1,
    ANOMALY_LEVEL_MINOR = 2,
    ANOMALY_LEVEL_MAJOR = 3,
} anomaly_level_t;

enum
{
    ANOMALY_CH_AX = 0,
    ANOMALY_CH_AY = 1,
    ANOMALY_CH_AZ = 2,
    ANOMALY_CH_TEMP = 3,
    ANOMALY_CH_COUNT = 4,
};

typedef struct
{
    anomaly_mode_t mode;
    anomaly_tune_t tune;
    bool training_active;
    bool trained_ready;
    anomaly_level_t channel_level[ANOMALY_CH_COUNT];
    anomaly_level_t overall_level;
} anomaly_output_t;

void AnomalyEngine_Init(void);
void AnomalyEngine_SetMode(anomaly_mode_t mode);
anomaly_mode_t AnomalyEngine_GetMode(void);
void AnomalyEngine_CycleTune(void);
void AnomalyEngine_SetTune(anomaly_tune_t tune);
anomaly_tune_t AnomalyEngine_GetTune(void);
void AnomalyEngine_StartTraining(void);
void AnomalyEngine_StopTraining(void);
void AnomalyEngine_SetAdaptiveFreeze(bool freeze);
bool AnomalyEngine_GetAdaptiveFreeze(void);
void AnomalyEngine_Update(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, int16_t temp_c10);
void AnomalyEngine_GetOutput(anomaly_output_t *out);

#endif
