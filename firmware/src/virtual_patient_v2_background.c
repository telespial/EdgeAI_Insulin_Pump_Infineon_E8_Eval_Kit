#include "virtual_patient_v2_background.h"

#include <string.h>

typedef struct
{
    bool initialized;
    bool has_state;
    uint32_t last_now_s;
    float last_delivered_insulin_u_hr;
    uint16_t last_debug_code;
    virtual_patient_v2_state_t last_state;
} virtual_patient_v2_background_runtime_t;

static virtual_patient_v2_background_runtime_t g_background_runtime;

bool VirtualPatientV2Background_Init(void)
{
    memset(&g_background_runtime, 0, sizeof(g_background_runtime));
    VirtualPatientV2_Init();
    g_background_runtime.last_debug_code = VirtualPatientV2_GetDebugCode();
    g_background_runtime.initialized = true;
    return true;
}

bool VirtualPatientV2Background_Step(uint32_t now_s, float delivered_insulin_u_hr)
{
    if (!g_background_runtime.initialized)
    {
        if (!VirtualPatientV2Background_Init())
        {
            return false;
        }
    }

    if (!VirtualPatientV2_Step(now_s, delivered_insulin_u_hr, &g_background_runtime.last_state))
    {
        g_background_runtime.last_debug_code = VirtualPatientV2_GetDebugCode();
        return false;
    }

    g_background_runtime.last_now_s = now_s;
    g_background_runtime.last_delivered_insulin_u_hr = delivered_insulin_u_hr;
    g_background_runtime.last_debug_code = VirtualPatientV2_GetDebugCode();
    g_background_runtime.has_state = true;
    return true;
}

bool VirtualPatientV2Background_GetState(virtual_patient_v2_state_t *state)
{
    if ((state == NULL) || !g_background_runtime.has_state)
    {
        return false;
    }

    *state = g_background_runtime.last_state;
    return true;
}

uint16_t VirtualPatientV2Background_GetDebugCode(void)
{
    return g_background_runtime.last_debug_code;
}
