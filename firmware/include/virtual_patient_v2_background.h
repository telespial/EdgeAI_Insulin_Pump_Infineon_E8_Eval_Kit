#ifndef VIRTUAL_PATIENT_V2_BACKGROUND_H
#define VIRTUAL_PATIENT_V2_BACKGROUND_H

#include <stdbool.h>
#include <stdint.h>

#include "virtual_patient_v2.h"

bool VirtualPatientV2Background_Init(void);
bool VirtualPatientV2Background_Step(uint32_t now_s, float delivered_insulin_u_hr);
bool VirtualPatientV2Background_GetState(virtual_patient_v2_state_t *state);
uint16_t VirtualPatientV2Background_GetDebugCode(void);

#endif
