#ifndef EDGEAI_INSULIN_PUMP_APP_H
#define EDGEAI_INSULIN_PUMP_APP_H

#include <stdint.h>

void edgeai_insulin_pump_app_start(void);
#if defined(APP_APS_SIDECAR_DEMO) && (APP_APS_SIDECAR_DEMO == 1)
void ApsSidecar_Init(void);
void ApsSidecar_Service(uint32_t now_ms);
#endif
#if defined(APP_APS_EMBEDDED_PROBE) && (APP_APS_EMBEDDED_PROBE == 1)
void ApsEmbeddedProbe_RunOnce(void);
#endif

#endif
