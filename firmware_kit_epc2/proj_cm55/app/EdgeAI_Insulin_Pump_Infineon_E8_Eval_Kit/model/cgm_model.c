#include "cgm_model.h"

void cgm_model_init(void)
{
}

void cgm_model_predict(const cgm_trace_buffer_t *trace, uint16_t *pred_15m_mgdl, uint16_t *pred_30m_mgdl)
{
    uint16_t last = 110u;

    if ((trace != 0) && (trace->count > 0u))
    {
        last = trace->samples[(trace->head + CGM_TRACE_BUFFER_SIZE - 1u) % CGM_TRACE_BUFFER_SIZE];
    }

    if (pred_15m_mgdl != 0)
    {
        *pred_15m_mgdl = last;
    }

    if (pred_30m_mgdl != 0)
    {
        *pred_30m_mgdl = last;
    }
}
