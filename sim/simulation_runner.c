#include "simulation_runner.h"

#include "aps_physiology.h"
#include "activity_engine.h"
#include "controller_openaps.h"
#include "metrics.h"
#include "predictor_v2.h"
#include "safety_supervisor.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool get_future_glucose(const replay_dataset_t *dataset,
                               size_t index,
                               size_t horizon_steps,
                               uint16_t *glucose_mgdl)
{
    size_t future_index;

    if (dataset == NULL || glucose_mgdl == NULL)
    {
        return false;
    }

    future_index = index + horizon_steps;
    if (future_index >= dataset->count)
    {
        return false;
    }

    *glucose_mgdl = dataset->steps[future_index].input.cgm.glucose_mgdl;
    return true;
}

static predictor_v2_output_t compute_baseline_prediction(const predictor_v2_input_t *input)
{
    predictor_v2_output_t baseline_prediction;
    float current_glucose;
    float trend_mgdl_min;

    memset(&baseline_prediction, 0, sizeof(baseline_prediction));
    if (input == NULL)
    {
        return baseline_prediction;
    }

    current_glucose = (float)input->cgm.glucose_mgdl;
    trend_mgdl_min = (float)input->cgm.trend_mgdl_min_x100 / 100.0f;

    baseline_prediction.pred_15m_mgdl = (uint16_t)lroundf(fminf(fmaxf(current_glucose + trend_mgdl_min * 15.0f, 40.0f), 400.0f));
    baseline_prediction.pred_30m_mgdl = (uint16_t)lroundf(fminf(fmaxf(current_glucose + trend_mgdl_min * 30.0f, 40.0f), 400.0f));
    baseline_prediction.pred_60m_mgdl = (uint16_t)lroundf(fminf(fmaxf(current_glucose + trend_mgdl_min * 60.0f, 40.0f), 400.0f));
    baseline_prediction.delta_15m_mgdl = (int16_t)(baseline_prediction.pred_15m_mgdl - input->cgm.glucose_mgdl);
    baseline_prediction.delta_30m_mgdl = (int16_t)(baseline_prediction.pred_30m_mgdl - input->cgm.glucose_mgdl);
    baseline_prediction.delta_60m_mgdl = (int16_t)(baseline_prediction.pred_60m_mgdl - input->cgm.glucose_mgdl);
    baseline_prediction.confidence_pct = input->cgm.sqi_pct;

    if (input->cgm.sqi_pct < 60u)
    {
        baseline_prediction.risk_flags |= APS_RISK_BAD_SENSOR_QUALITY | APS_RISK_LOW_CONFIDENCE;
    }
    if ((input->cgm.sensor_flags & APS_SENSOR_FLAG_STALE_CGM) != 0u)
    {
        baseline_prediction.risk_flags |= APS_RISK_STALE_CGM | APS_RISK_LOW_CONFIDENCE;
    }

    return baseline_prediction;
}

static void update_compact_prediction_samples(uint16_t actual_value,
                                              uint16_t predicted_value,
                                              uint16_t *actual_values,
                                              uint16_t *predicted_values,
                                              size_t *count,
                                              size_t capacity)
{
    if (actual_values == NULL || predicted_values == NULL || count == NULL)
    {
        return;
    }

    if (*count >= capacity)
    {
        return;
    }

    actual_values[*count] = actual_value;
    predicted_values[*count] = predicted_value;
    (*count)++;
}

bool SimulationRunner_RunDataset(const replay_dataset_t *dataset,
                                 FILE *stream,
                                 sim_step_result_t *results,
                                 size_t results_capacity,
                                 size_t *results_count,
                                 simulation_summary_t *summary,
                                 char *error,
                                 size_t error_length)
{
    size_t index;
    predictor_v2_output_t ml_prediction;
    predictor_v2_output_t baseline_prediction;
    aps_controller_output_t controller_without_ml;
    aps_controller_output_t controller_with_ml;
    aps_controller_output_t final_output;
    aps_physiology_state_t physiology;
    activity_features_t activity_features;
    predictor_v2_input_t input;
    uint16_t clinical_actual_glucose_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t clinical_ml_predicted_15_values[REPLAY_LOADER_MAX_STEPS];
    uint32_t safety_flags[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_actual_15_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_predicted_15_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_actual_30_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_predicted_30_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_actual_60_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t baseline_predicted_60_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_actual_15_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_predicted_15_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_actual_30_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_predicted_30_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_actual_60_values[REPLAY_LOADER_MAX_STEPS];
    uint16_t ml_predicted_60_values[REPLAY_LOADER_MAX_STEPS];
    size_t baseline_count_15 = 0u;
    size_t baseline_count_30 = 0u;
    size_t baseline_count_60 = 0u;
    size_t ml_count_15 = 0u;
    size_t ml_count_30 = 0u;
    size_t ml_count_60 = 0u;
    size_t clinical_count = 0u;
    uint32_t disagreement_count = 0u;

    if (results_count != NULL)
    {
        *results_count = 0u;
    }
    if (summary != NULL)
    {
        memset(summary, 0, sizeof(*summary));
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
    ActivityEngine_Reset();
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
        uint16_t actual_15m = 0u;
        uint16_t actual_30m = 0u;
        uint16_t actual_60m = 0u;
        bool has_actual_15m = get_future_glucose(dataset, index, 3u, &actual_15m);
        bool has_actual_30m = get_future_glucose(dataset, index, 6u, &actual_30m);
        bool has_actual_60m = get_future_glucose(dataset, index, 12u, &actual_60m);
        aps_event_t event;

        memset(&event, 0, sizeof(event));
        event.epoch_s = step->now_s;
        event.bolus_u = step->has_insulin ? step->insulin_u : 0.0f;
        event.basal_u = step->has_basal ? step->basal_u_per_hr : 0.0f;
        event.carbs_g = step->has_carbs ? step->carbs_g : 0.0f;
        ApsPhysiology_AddEvent(&event);
        ApsPhysiology_Update(step->now_s, &physiology);
        if (!ActivityEngine_Update(step->now_s,
                                   step->has_activity ? step->accel_ax_mg : 0,
                                   step->has_activity ? step->accel_ay_mg : 0,
                                   step->has_activity ? step->accel_az_mg : 1000,
                                   step->has_step_count,
                                   step->step_count,
                                   &activity_features))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "activity engine failed at row %zu", index + 1u);
            }
            return false;
        }

        input = step->input;
        input.physiology = physiology;
        input.physiology.basal_u_per_hr = step->has_basal ? step->basal_u_per_hr : physiology.basal_u_per_hr;
        input.physiology.activity_state = (uint8_t)activity_features.state;
        input.physiology.activity_confidence_pct = activity_features.confidence_pct;
        input.physiology.motion_rms_5m = activity_features.motion_rms_5m;
        input.physiology.motion_rms_15m = activity_features.motion_rms_15m;
        input.physiology.active_minutes = activity_features.active_minutes;
        input.physiology.post_exercise_minutes = activity_features.post_exercise_minutes;
        input.physiology_present = step->input.physiology_present && dataset->physiology_columns_present;

        baseline_prediction = compute_baseline_prediction(&input);

        if (!PredictorV2_Update(&input, &ml_prediction))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "predictor update failed at row %zu", index + 1u);
            }
            return false;
        }
        if (!OpenApsController_DetermineBasal(&input, &baseline_prediction, &controller_without_ml))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "baseline controller failed at row %zu", index + 1u);
            }
            return false;
        }
        if (!OpenApsController_DetermineBasal(&input, &ml_prediction, &controller_with_ml))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "controller failed at row %zu", index + 1u);
            }
            return false;
        }

        final_output = controller_with_ml;
        if (!SafetySupervisor_Apply(step->now_s, &input, &ml_prediction, &final_output))
        {
            if (error != NULL && error_length > 0u)
            {
                snprintf(error, error_length, "safety supervisor failed at row %zu", index + 1u);
            }
            return false;
        }

        if (stream != NULL)
        {
            sim_step_result_t result_for_stream;

            memset(&result_for_stream, 0, sizeof(result_for_stream));
            result_for_stream.timestamp_s = step->now_s;
            result_for_stream.input = input;
            result_for_stream.baseline_prediction = baseline_prediction;
            result_for_stream.ml_prediction = ml_prediction;
            result_for_stream.controller_without_ml = controller_without_ml;
            result_for_stream.controller_with_ml = controller_with_ml;
            result_for_stream.safety_final_output = final_output;
            result_for_stream.actual_15m_mgdl = actual_15m;
            result_for_stream.actual_30m_mgdl = actual_30m;
            result_for_stream.actual_60m_mgdl = actual_60m;
            result_for_stream.has_actual_15m = has_actual_15m;
            result_for_stream.has_actual_30m = has_actual_30m;
            result_for_stream.has_actual_60m = has_actual_60m;
            result_for_stream.baseline_error_15m_mgdl = has_actual_15m ? (int16_t)((int32_t)baseline_prediction.pred_15m_mgdl - (int32_t)actual_15m) : 0;
            result_for_stream.baseline_error_30m_mgdl = has_actual_30m ? (int16_t)((int32_t)baseline_prediction.pred_30m_mgdl - (int32_t)actual_30m) : 0;
            result_for_stream.baseline_error_60m_mgdl = has_actual_60m ? (int16_t)((int32_t)baseline_prediction.pred_60m_mgdl - (int32_t)actual_60m) : 0;
            result_for_stream.ml_error_15m_mgdl = has_actual_15m ? (int16_t)((int32_t)ml_prediction.pred_15m_mgdl - (int32_t)actual_15m) : 0;
            result_for_stream.ml_error_30m_mgdl = has_actual_30m ? (int16_t)((int32_t)ml_prediction.pred_30m_mgdl - (int32_t)actual_30m) : 0;
            result_for_stream.ml_error_60m_mgdl = has_actual_60m ? (int16_t)((int32_t)ml_prediction.pred_60m_mgdl - (int32_t)actual_60m) : 0;
            result_for_stream.safety_reason_flags = final_output.reason_flags;
            AuditTrace_PrintStep(stream, &result_for_stream);
        }

        if (results != NULL && index < results_capacity)
        {
            results[index].timestamp_s = step->now_s;
            results[index].input = input;
            results[index].baseline_prediction = baseline_prediction;
            results[index].ml_prediction = ml_prediction;
            results[index].controller_without_ml = controller_without_ml;
            results[index].controller_with_ml = controller_with_ml;
            results[index].safety_final_output = final_output;
            results[index].actual_15m_mgdl = actual_15m;
            results[index].actual_30m_mgdl = actual_30m;
            results[index].actual_60m_mgdl = actual_60m;
            results[index].has_actual_15m = has_actual_15m;
            results[index].has_actual_30m = has_actual_30m;
            results[index].has_actual_60m = has_actual_60m;
            results[index].baseline_error_15m_mgdl = has_actual_15m ? (int16_t)((int32_t)baseline_prediction.pred_15m_mgdl - (int32_t)actual_15m) : 0;
            results[index].baseline_error_30m_mgdl = has_actual_30m ? (int16_t)((int32_t)baseline_prediction.pred_30m_mgdl - (int32_t)actual_30m) : 0;
            results[index].baseline_error_60m_mgdl = has_actual_60m ? (int16_t)((int32_t)baseline_prediction.pred_60m_mgdl - (int32_t)actual_60m) : 0;
            results[index].ml_error_15m_mgdl = has_actual_15m ? (int16_t)((int32_t)ml_prediction.pred_15m_mgdl - (int32_t)actual_15m) : 0;
            results[index].ml_error_30m_mgdl = has_actual_30m ? (int16_t)((int32_t)ml_prediction.pred_30m_mgdl - (int32_t)actual_30m) : 0;
            results[index].ml_error_60m_mgdl = has_actual_60m ? (int16_t)((int32_t)ml_prediction.pred_60m_mgdl - (int32_t)actual_60m) : 0;
            results[index].safety_reason_flags = final_output.reason_flags;
        }

        if (has_actual_15m)
        {
            update_compact_prediction_samples(actual_15m,
                                              baseline_prediction.pred_15m_mgdl,
                                              baseline_actual_15_values,
                                              baseline_predicted_15_values,
                                              &baseline_count_15,
                                              REPLAY_LOADER_MAX_STEPS);
            update_compact_prediction_samples(actual_15m,
                                              ml_prediction.pred_15m_mgdl,
                                              ml_actual_15_values,
                                              ml_predicted_15_values,
                                              &ml_count_15,
                                              REPLAY_LOADER_MAX_STEPS);
        }
        if (has_actual_30m)
        {
            update_compact_prediction_samples(actual_30m,
                                              baseline_prediction.pred_30m_mgdl,
                                              baseline_actual_30_values,
                                              baseline_predicted_30_values,
                                              &baseline_count_30,
                                              REPLAY_LOADER_MAX_STEPS);
            update_compact_prediction_samples(actual_30m,
                                              ml_prediction.pred_30m_mgdl,
                                              ml_actual_30_values,
                                              ml_predicted_30_values,
                                              &ml_count_30,
                                              REPLAY_LOADER_MAX_STEPS);
        }
        if (has_actual_60m)
        {
            update_compact_prediction_samples(actual_60m,
                                              baseline_prediction.pred_60m_mgdl,
                                              baseline_actual_60_values,
                                              baseline_predicted_60_values,
                                              &baseline_count_60,
                                              REPLAY_LOADER_MAX_STEPS);
            update_compact_prediction_samples(actual_60m,
                                              ml_prediction.pred_60m_mgdl,
                                              ml_actual_60_values,
                                              ml_predicted_60_values,
                                              &ml_count_60,
                                              REPLAY_LOADER_MAX_STEPS);
        }

        clinical_actual_glucose_values[clinical_count] = input.cgm.glucose_mgdl;
        clinical_ml_predicted_15_values[clinical_count] = ml_prediction.pred_15m_mgdl;
        safety_flags[clinical_count] = final_output.reason_flags;
        clinical_count++;

        if (controller_without_ml.action != controller_with_ml.action ||
            fabsf(controller_without_ml.requested_basal_u_per_hr - controller_with_ml.requested_basal_u_per_hr) > 0.001f ||
            fabsf(controller_without_ml.suggested_correction_u - controller_with_ml.suggested_correction_u) > 0.001f)
        {
            disagreement_count++;
        }
    }

    if (results_count != NULL)
    {
        *results_count = dataset->count;
    }

    if (summary != NULL)
    {
        summary->sample_count = dataset->count;
        summary->valid_actual_count_15m = baseline_count_15;
        summary->valid_actual_count_30m = baseline_count_30;
        summary->valid_actual_count_60m = baseline_count_60;
        summary->controller_disagreement_count = disagreement_count;
        Metrics_ComputePrediction(baseline_actual_15_values, baseline_predicted_15_values, baseline_count_15, &summary->baseline_15m);
        Metrics_ComputePrediction(baseline_actual_30_values, baseline_predicted_30_values, baseline_count_30, &summary->baseline_30m);
        Metrics_ComputePrediction(baseline_actual_60_values, baseline_predicted_60_values, baseline_count_60, &summary->baseline_60m);
        Metrics_ComputePrediction(ml_actual_15_values, ml_predicted_15_values, ml_count_15, &summary->ml_15m);
        Metrics_ComputePrediction(ml_actual_30_values, ml_predicted_30_values, ml_count_30, &summary->ml_30m);
        Metrics_ComputePrediction(ml_actual_60_values, ml_predicted_60_values, ml_count_60, &summary->ml_60m);
        Metrics_ComputeClinical(clinical_actual_glucose_values, clinical_ml_predicted_15_values, safety_flags, clinical_count, &summary->clinical);
    }

    return true;
}

void SimulationRunner_PrintSummary(FILE *stream, const simulation_summary_t *summary)
{
    if (stream == NULL || summary == NULL)
    {
        return;
    }

    fprintf(stream, "Replay summary\n");
    fprintf(stream, "  samples: %zu\n", summary->sample_count);
    fprintf(stream, "  baseline 15m MAE/RMSE: %.2f / %.2f\n", summary->baseline_15m.mae, summary->baseline_15m.rmse);
    fprintf(stream, "  baseline 30m MAE/RMSE: %.2f / %.2f\n", summary->baseline_30m.mae, summary->baseline_30m.rmse);
    fprintf(stream, "  baseline 60m MAE/RMSE: %.2f / %.2f\n", summary->baseline_60m.mae, summary->baseline_60m.rmse);
    fprintf(stream, "  ML 15m MAE/RMSE: %.2f / %.2f\n", summary->ml_15m.mae, summary->ml_15m.rmse);
    fprintf(stream, "  ML 30m MAE/RMSE: %.2f / %.2f\n", summary->ml_30m.mae, summary->ml_30m.rmse);
    fprintf(stream, "  ML 60m MAE/RMSE: %.2f / %.2f\n", summary->ml_60m.mae, summary->ml_60m.rmse);
    fprintf(stream, "  time in range: %.1f%%\n", summary->clinical.time_in_range_pct);
    fprintf(stream, "  time below range: %.1f%%\n", summary->clinical.time_below_range_pct);
    fprintf(stream, "  time above range: %.1f%%\n", summary->clinical.time_above_range_pct);
    fprintf(stream, "  missed low count: %u\n", summary->clinical.missed_low_count);
    fprintf(stream, "  false low warning count: %u\n", summary->clinical.false_low_warning_count);
    fprintf(stream, "  safety intervention count: %u\n", summary->clinical.safety_intervention_count);
    fprintf(stream, "  controller disagreement count: %u\n", summary->controller_disagreement_count);
}
