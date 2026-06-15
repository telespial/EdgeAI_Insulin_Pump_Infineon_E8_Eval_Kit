#ifndef VIRTUAL_PATIENT_V2_H
#define VIRTUAL_PATIENT_V2_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    VP_SCENARIO_NORMAL = 0,
    VP_SCENARIO_BREAKFAST,
    VP_SCENARIO_EXERCISE,
    VP_SCENARIO_DAWN,
    VP_SCENARIO_LOW_GLUCOSE,
    VP_SCENARIO_RAPID_FALL,
} vp_scenario_t;

typedef struct
{
    uint32_t epoch_s;
    uint32_t step_index;
    uint16_t cycle_step;
    uint16_t bg_mgdl;
    uint16_t target_bg_mgdl;
    uint16_t debug_code;
    vp_scenario_t scenario;
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
void VirtualPatientV2_InitWithScenario(vp_scenario_t scenario);
bool VirtualPatientV2_Step(uint32_t now_s,
                           float delivered_insulin_u_hr,
                           virtual_patient_v2_state_t *state);
uint16_t VirtualPatientV2_GetDebugCode(void);
vp_scenario_t VirtualPatientV2_GetScenario(void);

#endif
