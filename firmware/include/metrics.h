#ifndef METRICS_H
#define METRICS_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    float mae;
    float rmse;
    float mape;
} prediction_metrics_t;

typedef struct
{
    float time_in_range_pct;
    float time_below_range_pct;
    float time_above_range_pct;
    uint32_t missed_low_count;
    uint32_t false_low_warning_count;
    uint32_t safety_intervention_count;
} clinical_metrics_t;

void Metrics_ComputePrediction(const uint16_t *actual_values,
                               const uint16_t *predicted_values,
                               size_t count,
                               prediction_metrics_t *metrics);

void Metrics_ComputeClinical(const uint16_t *glucose_values,
                             const uint16_t *predicted_values,
                             const uint32_t *safety_flags,
                             size_t count,
                             clinical_metrics_t *metrics);

#endif
