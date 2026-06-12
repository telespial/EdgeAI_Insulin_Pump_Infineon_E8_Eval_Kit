#include "simulation_runner.h"

#include "aps_physiology.h"
#include "controller_openaps.h"
#include "metrics.h"
#include "predictor_v2.h"
#include "safety_supervisor.h"

#include <stdio.h>
#include <string.h>

bool SimulationRunner_RunDataset(const replay_dataset_t *dataset,
                                 FILE *stream,
                                 sim_step_result_t *results,
                                 size_t results_capacity,
                                 size_t *results_count,
                                 char *error,
                                 size_t error_length)
{
    size_t index;
    predictor_v2_output_t prediction;
    aps_controller_output_t controller_request;
    aps_controller_output_t final_output;
    aps_physiology_state_t physiology;
    predictor_v2_input_t input;

    if (results_count != NULL)
    {
        *results_count = 0u;
    }

    if (dataset == NULL || dataset->count == 0u)
    {
        if (error != NULL && error_length > 0u)
        {
            snprintf(error, error_length, "empty dataset");
        }
        return false;
    }

    ApsPhysiology_Reset();
    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    OpenApsController_Reset();

    if (stream != NULL)
    {
        AuditTrace_PrintHeader(stream);
    }

    for (index = 0u; index < dataset->count; ++index)
    {
        const replay_step_t *step = &dataset->steps[index];
        aps_event_t event;

        memset(&event, 0, sizeof(event));
        event.epoch_s = step->now_s;
        event.bolus_u = step->has_insulin ? step->insulin_u : 0.0f;
        event.basal_u = step->has_basal ? step->basal_u_per_hr : 0.0f;
        event.carbs_g = step->has_carbs ? step->carbs_g : 0.0f;
        ApsPhysiology_AddEvent(&event);
        ApsPhysiology_Update(step->now_s, &physiology);

        input = step->input;
        input.physiology = physiology;
        input.physiology.basal_u_per_hr = step->has_basal ? step->basal_u_per_hr : physiology.basal_u_per_hr;
        input.physiology_present = step->input.physiology_present && dataset->physiology_columns_present;

        if (!PredictorV2_Update(&input, &prediction))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "predictor update failed at row %zu", index + 1u);
            }
            return false;
        }
        if (!OpenApsController_DetermineBasal(&input, &prediction, &controller_request))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "controller failed at row %zu", index + 1u);
            }
            return false;
        }

        final_output = controller_request;
        if (!SafetySupervisor_Apply(step->now_s, &input, &prediction, &final_output))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "safety supervisor failed at row %zu", index + 1u);
            }
            return false;
        }

        if (stream != NULL)
        {
            AuditTrace_PrintStep(stream, step->now_s, &input, &prediction, &controller_request, &final_output);
        }

        if (results != NULL && index < results_capacity)
        {
            results[index].timestamp_s = step->now_s;
            results[index].input = input;
            results[index].prediction = prediction;
            results[index].controller_request = controller_request;
            results[index].final_output = final_output;
        }
    }

    if (results_count != NULL)
    {
        *results_count = dataset->count;
    }

    return true;
}
