#include "audit_trace.h"
#include "simulation_runner.h"

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
    if (reason_flags == 0u)
    {
        snprintf(buffer, buffer_length, "NONE");
        return;
    }
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

static const char *action_to_string(aps_action_t action)
{
    switch (action)
    {
        case APS_ACTION_NO_CHANGE:
            return "NO_CHANGE";
        case APS_ACTION_REDUCE_BASAL:
            return "REDUCE_BASAL";
        case APS_ACTION_SUSPEND_BASAL:
            return "SUSPEND_BASAL";
        case APS_ACTION_INCREASE_BASAL:
            return "INCREASE_BASAL";
        case APS_ACTION_CORRECTION_SUGGESTION:
            return "CORRECTION_SUGGESTION";
        default:
            return "UNKNOWN";
    }
}

void AuditTrace_PrintHeader(FILE *stream)
{
    if (stream == NULL)
    {
        return;
    }

    fprintf(stream,
            "timestamp,glucose_mgdl,sqi_pct,iob_u,cob_g,baseline_pred_15,baseline_pred_30,baseline_pred_60,ml_pred_15,ml_pred_30,ml_pred_60,actual_15,actual_30,actual_60,baseline_error_15,baseline_error_30,baseline_error_60,ml_error_15,ml_error_30,ml_error_60,controller_without_ml,controller_with_ml,safety_final_action,safety_reason_codes\n");
}

static void format_optional_u16(char *buffer, size_t buffer_length, bool has_value, uint16_t value)
{
    if (buffer == NULL || buffer_length == 0u)
    {
        return;
    }

    if (has_value)
    {
        snprintf(buffer, buffer_length, "%u", value);
    }
    else
    {
        buffer[0] = '\0';
    }
}

static void format_optional_i16(char *buffer, size_t buffer_length, bool has_value, int16_t value)
{
    if (buffer == NULL || buffer_length == 0u)
    {
        return;
    }

    if (has_value)
    {
        snprintf(buffer, buffer_length, "%d", (int)value);
    }
    else
    {
        buffer[0] = '\0';
    }
}

void AuditTrace_PrintStep(FILE *stream, const sim_step_result_t *result)
{
    char reasons[192];
    char actual_15[32];
    char actual_30[32];
    char actual_60[32];
    char baseline_error_15[32];
    char baseline_error_30[32];
    char baseline_error_60[32];
    char ml_error_15[32];
    char ml_error_30[32];
    char ml_error_60[32];

    if (stream == NULL || result == NULL)
    {
        return;
    }

    format_optional_u16(actual_15, sizeof(actual_15), result->has_actual_15m, result->actual_15m_mgdl);
    format_optional_u16(actual_30, sizeof(actual_30), result->has_actual_30m, result->actual_30m_mgdl);
    format_optional_u16(actual_60, sizeof(actual_60), result->has_actual_60m, result->actual_60m_mgdl);
    format_optional_i16(baseline_error_15, sizeof(baseline_error_15), result->has_actual_15m, result->baseline_error_15m_mgdl);
    format_optional_i16(baseline_error_30, sizeof(baseline_error_30), result->has_actual_30m, result->baseline_error_30m_mgdl);
    format_optional_i16(baseline_error_60, sizeof(baseline_error_60), result->has_actual_60m, result->baseline_error_60m_mgdl);
    format_optional_i16(ml_error_15, sizeof(ml_error_15), result->has_actual_15m, result->ml_error_15m_mgdl);
    format_optional_i16(ml_error_30, sizeof(ml_error_30), result->has_actual_30m, result->ml_error_30m_mgdl);
    format_optional_i16(ml_error_60, sizeof(ml_error_60), result->has_actual_60m, result->ml_error_60m_mgdl);
    AuditTrace_FormatSafetyReasons(result->safety_reason_flags, reasons, sizeof(reasons));
    fprintf(stream,
            "%u,%u,%u,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",
            result->timestamp_s,
            result->input.cgm.glucose_mgdl,
            result->input.cgm.sqi_pct,
            result->input.physiology.iob_u,
            result->input.physiology.cob_g,
            result->baseline_prediction.pred_15m_mgdl,
            result->baseline_prediction.pred_30m_mgdl,
            result->baseline_prediction.pred_60m_mgdl,
            result->ml_prediction.pred_15m_mgdl,
            result->ml_prediction.pred_30m_mgdl,
            result->ml_prediction.pred_60m_mgdl,
            actual_15,
            actual_30,
            actual_60,
            baseline_error_15,
            baseline_error_30,
            baseline_error_60,
            ml_error_15,
            ml_error_30,
            ml_error_60,
            action_to_string(result->controller_without_ml.action),
            action_to_string(result->controller_with_ml.action),
            action_to_string(result->safety_final_output.action),
            reasons);
}
