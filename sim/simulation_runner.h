#ifndef SIMULATION_RUNNER_H
#define SIMULATION_RUNNER_H

#include "audit_trace.h"
#include "scenario_runner.h"

typedef struct
{
    uint32_t timestamp_s;
    predictor_v2_input_t input;
    predictor_v2_output_t prediction;
    aps_controller_output_t controller_request;
    aps_controller_output_t final_output;
} sim_step_result_t;

bool SimulationRunner_RunDataset(const replay_dataset_t *dataset,
                                 FILE *stream,
                                 sim_step_result_t *results,
                                 size_t results_capacity,
                                 size_t *results_count,
                                 char *error,
                                 size_t error_length);

#endif
