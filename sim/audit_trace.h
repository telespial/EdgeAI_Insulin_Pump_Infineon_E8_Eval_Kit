#ifndef AUDIT_TRACE_H
#define AUDIT_TRACE_H

#include "safety_supervisor.h"

typedef struct sim_step_result_t sim_step_result_t;

#include <stdio.h>

void AuditTrace_PrintHeader(FILE *stream);
void AuditTrace_PrintStep(FILE *stream, const sim_step_result_t *result);
void AuditTrace_FormatSafetyReasons(uint32_t reason_flags, char *buffer, size_t buffer_length);

#endif
