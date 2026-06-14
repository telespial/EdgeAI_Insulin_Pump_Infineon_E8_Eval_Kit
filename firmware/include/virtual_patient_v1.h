#ifndef VIRTUAL_PATIENT_V1_H
#define VIRTUAL_PATIENT_V1_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t epoch_s;
    uint16_t bg_mgdl;
    float meal_cob_g;
    float insulin_iob_u;
    float activity_factor;
    float basal_u_hr;
    bool meal_event;
    bool bolus_event;
} virtual_patient_state_t;

void VirtualPatientV1_Init(void);
bool VirtualPatientV1_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_state_t *state);

#endif
