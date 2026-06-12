#ifndef COB_ENGINE_H
#define COB_ENGINE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t meal_duration_min;
    uint16_t max_meals;
} cob_engine_config_t;

void CobEngine_Init(const cob_engine_config_t *config);
void CobEngine_Reset(void);
bool CobEngine_AddMeal(uint32_t timestamp_s, float carbs_g, uint16_t absorption_min);
bool CobEngine_Update(uint32_t now_s);
float CobEngine_GetCobG(void);

#endif
