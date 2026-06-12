#ifndef APS_PHYSIOLOGY_H
#define APS_PHYSIOLOGY_H

#include "aps_types.h"

typedef struct
{
    float insulin_action_hours;
    float carb_action_hours;
    float default_basal_u_per_hr;
} aps_physiology_config_t;

void ApsPhysiology_Reset(void);
void ApsPhysiology_Configure(const aps_physiology_config_t *config);
void ApsPhysiology_AddEvent(const aps_event_t *event);
void ApsPhysiology_Update(uint32_t epoch_s, aps_physiology_state_t *state);

#endif
