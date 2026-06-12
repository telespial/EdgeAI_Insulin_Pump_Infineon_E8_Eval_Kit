#ifndef SIMULATION_RUNNER_H
#define SIMULATION_RUNNER_H

#include "audit_trace.h"
#include "metrics.h"
#include "scenario_runner.h"

typedef struct sim_step_result_t
{
    uint32_t timestamp_s;
    predictor_v2_input_t input;
    predictor_v2_output_t ml_prediction;
    predictor_v2_output_t baseline_prediction;
    aps_controller_output_t controller_without_ml;
    aps_controller_output_t controller_with_ml;
    aps_controller_output_t safety_final_output;
    uint16_t actual_15m_mgdl;
    uint16_t actual_30m_mgdl;
    uint16_t actual_60m_mgdl;
    bool has_actual_15m;
    bool has_actual_30m;
    bool has_actual_60m;
    int16_t baseline_error_15m_mgdl;
    int16_t baseline_error_30m_mgdl;
    int16_t baseline_error_60m_mgdl;
    int16_t ml_error_15m_mgdl;
    int16_t ml_error_30m_mgdl;
    int16_t ml_error_60m_mgdl;
    uint32_t safety_reason_flags;
} sim_step_result_t;

typedef struct
{
    prediction_metrics_t baseline_15m;
    prediction_metrics_t baseline_30m;
    prediction_metrics_t baseline_60m;
    prediction_metrics_t ml_15m;
    prediction_metrics_t ml_30m;
    prediction_metrics_t ml_60m;
    clinical_metrics_t clinical;
    uint32_t controller_disagreement_count;
    size_t sample_count;
    size_t valid_actual_count_15m;
    size_t valid_actual_count_30m;
    size_t valid_actual_count_60m;
} simulation_summary_t;

bool SimulationRunner_RunDataset(const replay_dataset_t *dataset,
                                 FILE *stream,
                                 sim_step_result_t *results,
                                 size_t results_capacity,
                                 size_t *results_count,
                                 simulation_summary_t *summary,
                                 char *error,
                                 size_t error_length);

void SimulationRunner_PrintSummary(FILE *stream, const simulation_summary_t *summary);

#endif
