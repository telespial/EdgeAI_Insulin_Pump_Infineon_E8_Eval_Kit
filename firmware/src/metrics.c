#include "metrics.h"

#include <math.h>
#include <string.h>

void Metrics_ComputePrediction(const uint16_t *actual_values,
                               const uint16_t *predicted_values,
                               size_t count,
                               prediction_metrics_t *metrics)
{
    size_t sample_index;
    double absolute_error_sum = 0.0;
    double squared_error_sum = 0.0;
    double absolute_percentage_error_sum = 0.0;

    if (metrics == NULL)
    {
        return;
    }

    memset(metrics, 0, sizeof(*metrics));

    if (actual_values == NULL || predicted_values == NULL || count == 0u)
    {
        return;
    }

    for (sample_index = 0u; sample_index < count; ++sample_index)
    {
        double actual_value = (double)actual_values[sample_index];
        double predicted_value = (double)predicted_values[sample_index];
        double difference = predicted_value - actual_value;

        absolute_error_sum += fabs(difference);
        squared_error_sum += difference * difference;
        if (actual_value > 0.0)
        {
            absolute_percentage_error_sum += fabs(difference / actual_value);
        }
    }

    metrics->mae = (float)(absolute_error_sum / (double)count);
    metrics->rmse = (float)sqrt(squared_error_sum / (double)count);
    metrics->mape = (float)((absolute_percentage_error_sum / (double)count) * 100.0);
}

void Metrics_ComputeClinical(const uint16_t *glucose_values,
                             const uint16_t *predicted_values,
                             const uint32_t *safety_flags,
                             size_t count,
                             clinical_metrics_t *metrics)
{
    size_t sample_index;
    size_t in_range_count = 0u;
    size_t below_range_count = 0u;
    size_t above_range_count = 0u;
    size_t missed_low_count = 0u;
    size_t false_low_warning_count = 0u;
    size_t safety_intervention_count = 0u;

    if (metrics == NULL)
    {
        return;
    }

    memset(metrics, 0, sizeof(*metrics));

    if (glucose_values == NULL || predicted_values == NULL || safety_flags == NULL || count == 0u)
    {
        return;
    }

    for (sample_index = 0u; sample_index < count; ++sample_index)
    {
        uint16_t actual_glucose = glucose_values[sample_index];
        uint16_t predicted_glucose = predicted_values[sample_index];

        if (actual_glucose >= 70u && actual_glucose <= 180u)
        {
            in_range_count++;
        }
        else if (actual_glucose < 70u)
        {
            below_range_count++;
        }
        else
        {
            above_range_count++;
        }

        if (actual_glucose < 70u && predicted_glucose >= 70u)
        {
            missed_low_count++;
        }
        if (actual_glucose >= 90u && predicted_glucose < 70u)
        {
            false_low_warning_count++;
        }
        if (safety_flags[sample_index] != 0u)
        {
            safety_intervention_count++;
        }
    }

    metrics->time_in_range_pct = (float)((100.0 * (double)in_range_count) / (double)count);
    metrics->time_below_range_pct = (float)((100.0 * (double)below_range_count) / (double)count);
    metrics->time_above_range_pct = (float)((100.0 * (double)above_range_count) / (double)count);
    metrics->missed_low_count = (uint32_t)missed_low_count;
    metrics->false_low_warning_count = (uint32_t)false_low_warning_count;
    metrics->safety_intervention_count = (uint32_t)safety_intervention_count;
}
