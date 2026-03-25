#ifndef EDGEAI_CGM_TRACE_BUFFER_H
#define EDGEAI_CGM_TRACE_BUFFER_H

#include <stdint.h>

#define CGM_TRACE_BUFFER_SIZE (256u)

typedef struct
{
    uint16_t samples[CGM_TRACE_BUFFER_SIZE];
    uint16_t head;
    uint16_t count;
} cgm_trace_buffer_t;

void cgm_trace_buffer_init(cgm_trace_buffer_t *buffer);
void cgm_trace_buffer_push(cgm_trace_buffer_t *buffer, uint16_t sample_mgdl);

#endif /* EDGEAI_CGM_TRACE_BUFFER_H */
