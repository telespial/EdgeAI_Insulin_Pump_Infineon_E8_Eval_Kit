#ifndef EDGEAI_INSULIN_PUMP_APP_H
#define EDGEAI_INSULIN_PUMP_APP_H

#include <stdint.h>

void edgeai_insulin_pump_app_start(void);
void ApsSmoke_Init(void);
void ApsSmoke_Service(uint32_t now_ms);

#endif
