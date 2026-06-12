#ifndef IOB_ENGINE_H
#define IOB_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t bolus_duration_min;
    uint16_t basal_duration_min;
    uint16_t max_events;
} iob_engine_config_t;

void IobEngine_Init(const iob_engine_config_t *config);
void IobEngine_Reset(void);
bool IobEngine_AddBolus(uint32_t timestamp_s, float bolus_u);
bool IobEngine_AddBasal(uint32_t timestamp_s, float basal_u_per_hr, uint16_t duration_min);
bool IobEngine_Update(uint32_t now_s);
float IobEngine_GetIobU(void);

#endif
