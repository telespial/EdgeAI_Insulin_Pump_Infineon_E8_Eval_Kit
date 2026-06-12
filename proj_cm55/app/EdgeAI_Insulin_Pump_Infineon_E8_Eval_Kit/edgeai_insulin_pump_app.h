#ifndef EDGEAI_INSULIN_PUMP_APP_H
#define EDGEAI_INSULIN_PUMP_APP_H

void edgeai_insulin_pump_app_start(void);
#if defined(APP_APS_EMBEDDED_PROBE) && (APP_APS_EMBEDDED_PROBE != 0)
void ApsEmbeddedProbe_RunOnce(void);
#endif

#endif
