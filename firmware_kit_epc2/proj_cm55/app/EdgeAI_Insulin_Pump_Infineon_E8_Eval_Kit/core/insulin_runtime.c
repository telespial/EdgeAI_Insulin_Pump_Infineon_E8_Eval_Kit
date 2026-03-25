#include "insulin_runtime.h"

#include "../model/cgm_model.h"
#include "../platform/insulin_platform.h"

#define HYPO_ALERT_THRESHOLD_MGDL (70u)
#define HYPER_ALERT_THRESHOLD_MGDL (180u)

void insulin_runtime_init(insulin_runtime_t *runtime)
{
    if (runtime == 0)
    {
        return;
    }

    runtime->glucose_mgdl = 110u;
    runtime->pred_15m_mgdl = 110u;
    runtime->pred_30m_mgdl = 110u;
    runtime->ai_enabled = true;
    runtime->alert_hypo = false;
    runtime->alert_hyper = false;

    cgm_trace_buffer_init(&runtime->cgm_trace);
    cgm_trace_buffer_push(&runtime->cgm_trace, runtime->glucose_mgdl);

    cgm_model_init();
}

void insulin_runtime_step(insulin_runtime_t *runtime)
{
    uint16_t glucose_now = 0u;

    if ((runtime == 0) || !insulin_platform_read_cgm_sample(&glucose_now))
    {
        return;
    }

    runtime->glucose_mgdl = glucose_now;
    cgm_trace_buffer_push(&runtime->cgm_trace, glucose_now);

    if (runtime->ai_enabled)
    {
        cgm_model_predict(&runtime->cgm_trace, &runtime->pred_15m_mgdl, &runtime->pred_30m_mgdl);
    }
    else
    {
        runtime->pred_15m_mgdl = glucose_now;
        runtime->pred_30m_mgdl = glucose_now;
    }

    runtime->alert_hypo = (runtime->pred_15m_mgdl <= HYPO_ALERT_THRESHOLD_MGDL);
    runtime->alert_hyper = (runtime->pred_15m_mgdl >= HYPER_ALERT_THRESHOLD_MGDL);
}
