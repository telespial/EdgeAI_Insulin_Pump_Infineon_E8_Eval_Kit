#ifndef VIRTUAL_PATIENT_V2_H
#define VIRTUAL_PATIENT_V2_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t epoch_s;
    uint16_t bg_mgdl;
    float meal_cob_g;
    float insulin_iob_u;
    float insulin_sensitivity;
    float activity_factor;
    float dawn_factor;
    float basal_u_hr;
    bool meal_event;
    bool bolus_event;
} virtual_patient_v2_state_t;

void VirtualPatientV2_Init(void);
bool VirtualPatientV2_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_v2_state_t *state);
uint16_t VirtualPatientV2_GetDebugCode(void);

#endif
