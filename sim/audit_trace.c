#include "audit_trace.h"

#include <stdio.h>
#include <string.h>

static void append_reason(char *buffer, size_t buffer_length, const char *reason)
{
    size_t current_length;

    if (buffer == NULL || buffer_length == 0u || reason == NULL)
    {
        return;
    }

    current_length = strlen(buffer);
    if (current_length > 0u && current_length + 1u < buffer_length)
    {
        buffer[current_length++] = '|';
        buffer[current_length] = '\0';
    }

    if (current_length < buffer_length)
    {
        snprintf(buffer + current_length, buffer_length - current_length, "%s", reason);
    }
}

void AuditTrace_FormatSafetyReasons(uint32_t reason_flags, char *buffer, size_t buffer_length)
{
    if (buffer == NULL || buffer_length == 0u)
    {
        return;
    }

    buffer[0] = '\0';
    if ((reason_flags & APS_SAFETY_REASON_STALE_CGM) != 0u) append_reason(buffer, buffer_length, "STALE_CGM");
    if ((reason_flags & APS_SAFETY_REASON_BAD_SQI) != 0u) append_reason(buffer, buffer_length, "BAD_SQI");
    if ((reason_flags & APS_SAFETY_REASON_PREDICTED_LOW_15M) != 0u) append_reason(buffer, buffer_length, "PREDICTED_LOW_15M");
    if ((reason_flags & APS_SAFETY_REASON_PREDICTED_LOW_30M) != 0u) append_reason(buffer, buffer_length, "PREDICTED_LOW_30M");
    if ((reason_flags & APS_SAFETY_REASON_RAPID_FALL) != 0u) append_reason(buffer, buffer_length, "RAPID_FALL");
    if ((reason_flags & APS_SAFETY_REASON_EXCESSIVE_IOB) != 0u) append_reason(buffer, buffer_length, "EXCESSIVE_IOB");
    if ((reason_flags & APS_SAFETY_REASON_IMPOSSIBLE_GLUCOSE) != 0u) append_reason(buffer, buffer_length, "IMPOSSIBLE_GLUCOSE");
    if ((reason_flags & APS_SAFETY_REASON_LOW_CONFIDENCE) != 0u) append_reason(buffer, buffer_length, "LOW_CONFIDENCE");
    if ((reason_flags & APS_SAFETY_REASON_MISSING_PHYSIOLOGY) != 0u) append_reason(buffer, buffer_length, "MISSING_PHYSIOLOGY");
    if ((reason_flags & APS_SAFETY_REASON_CONTROLLER_BLOCKED) != 0u) append_reason(buffer, buffer_length, "CONTROLLER_BLOCKED");
}

void AuditTrace_PrintHeader(FILE *stream)
{
    if (stream == NULL)
    {
        return;
    }

    fprintf(stream,
            "timestamp,glucose,pred15,pred30,pred60,iob,cob,controller_action,controller_basal,final_action,final_basal,safety_reasons\n");
}

void AuditTrace_PrintStep(FILE *stream,
                         uint32_t timestamp_s,
                         const predictor_v2_input_t *input,
                         const predictor_v2_output_t *prediction,
                         const aps_controller_output_t *controller_request,
                         const aps_controller_output_t *final_output)
{
    char reasons[192];

    if (stream == NULL || input == NULL || prediction == NULL || controller_request == NULL || final_output == NULL)
    {
        return;
    }

    AuditTrace_FormatSafetyReasons(final_output->reason_flags, reasons, sizeof(reasons));
    fprintf(stream,
            "%u,%u,%u,%u,%u,%.3f,%.3f,%u,%.3f,%u,%.3f,%s\n",
            timestamp_s,
            input->cgm.glucose_mgdl,
            prediction->pred_15m_mgdl,
            prediction->pred_30m_mgdl,
            prediction->pred_60m_mgdl,
            input->physiology.iob_u,
            input->physiology.cob_g,
            controller_request->action,
            controller_request->requested_basal_u_per_hr,
            final_output->action,
            final_output->requested_basal_u_per_hr,
            reasons);
}
