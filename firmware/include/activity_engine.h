#ifndef ACTIVITY_ENGINE_H
#define ACTIVITY_ENGINE_H

#include "aps_types.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t max_samples;
} activity_engine_config_t;

void ActivityEngine_Init(const activity_engine_config_t *config);
void ActivityEngine_Reset(void);
bool ActivityEngine_Update(uint32_t timestamp_s,
                           int16_t ax_mg,
                           int16_t ay_mg,
                           int16_t az_mg,
                           bool has_step_count,
                           uint16_t step_count,
                           activity_features_t *features);
const activity_features_t *ActivityEngine_GetFeatures(void);

#endif
