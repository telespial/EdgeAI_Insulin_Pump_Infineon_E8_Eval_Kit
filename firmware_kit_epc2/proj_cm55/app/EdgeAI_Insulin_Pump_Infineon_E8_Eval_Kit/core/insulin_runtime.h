#ifndef EDGEAI_INSULIN_RUNTIME_H
#define EDGEAI_INSULIN_RUNTIME_H

#include <stdbool.h>
#include <stdint.h>

#include "../data/cgm_trace_buffer.h"

typedef struct
{
    uint16_t glucose_mgdl;
    uint16_t pred_15m_mgdl;
    uint16_t pred_30m_mgdl;
    bool ai_enabled;
    bool alert_hypo;
    bool alert_hyper;
    cgm_trace_buffer_t cgm_trace;
} insulin_runtime_t;

void insulin_runtime_init(insulin_runtime_t *runtime);
void insulin_runtime_step(insulin_runtime_t *runtime);

#endif /* EDGEAI_INSULIN_RUNTIME_H */
