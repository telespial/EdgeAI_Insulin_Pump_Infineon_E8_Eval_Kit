#ifndef EDGEAI_CGM_MODEL_H
#define EDGEAI_CGM_MODEL_H

#include <stdint.h>

#include "../data/cgm_trace_buffer.h"

void cgm_model_init(void);
void cgm_model_predict(const cgm_trace_buffer_t *trace, uint16_t *pred_15m_mgdl, uint16_t *pred_30m_mgdl);

#endif /* EDGEAI_CGM_MODEL_H */
