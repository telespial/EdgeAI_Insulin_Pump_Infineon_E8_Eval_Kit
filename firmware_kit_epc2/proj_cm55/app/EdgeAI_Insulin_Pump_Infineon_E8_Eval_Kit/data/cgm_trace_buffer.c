#include "cgm_trace_buffer.h"

void cgm_trace_buffer_init(cgm_trace_buffer_t *buffer)
{
    uint16_t i = 0u;

    if (buffer == 0)
    {
        return;
    }

    for (i = 0u; i < CGM_TRACE_BUFFER_SIZE; ++i)
    {
        buffer->samples[i] = 0u;
    }

    buffer->head = 0u;
    buffer->count = 0u;
}

void cgm_trace_buffer_push(cgm_trace_buffer_t *buffer, uint16_t sample_mgdl)
{
    if (buffer == 0)
    {
        return;
    }

    buffer->samples[buffer->head] = sample_mgdl;
    buffer->head = (uint16_t)((buffer->head + 1u) % CGM_TRACE_BUFFER_SIZE);

    if (buffer->count < CGM_TRACE_BUFFER_SIZE)
    {
        buffer->count++;
    }
}
