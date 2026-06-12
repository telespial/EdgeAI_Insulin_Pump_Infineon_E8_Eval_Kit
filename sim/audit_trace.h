#ifndef AUDIT_TRACE_H
#define AUDIT_TRACE_H

#include "safety_supervisor.h"

#include <stdio.h>

void AuditTrace_PrintHeader(FILE *stream);
void AuditTrace_PrintStep(FILE *stream,
                         uint32_t timestamp_s,
                         const predictor_v2_input_t *input,
                         const predictor_v2_output_t *prediction,
                         const aps_controller_output_t *controller_request,
                         const aps_controller_output_t *final_output);
void AuditTrace_FormatSafetyReasons(uint32_t reason_flags, char *buffer, size_t buffer_length);

#endif
