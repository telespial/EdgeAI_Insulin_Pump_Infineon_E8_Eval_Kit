#ifndef APS_DEMO_STATE_H
#define APS_DEMO_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "aps_types.h"

typedef struct
{
    uint16_t bg_mgdl;
    float iob_u;
    float cob_g;
    aps_action_t action;
    float insulin_u_hr;
    uint32_t safety_flags;
    const char *action_text;
    const char *safe_text;
} aps_demo_state_t;

bool ApsDemoState_Init(void);
bool ApsDemoState_Step(uint32_t now_s, aps_demo_state_t *state);
bool ApsDemoState_FormatTerminal(const aps_demo_state_t *state, char *buffer, size_t buffer_size);

#endif
