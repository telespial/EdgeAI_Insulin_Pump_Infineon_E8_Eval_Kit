#include "aps_physiology.h"
#include "controller_openaps.h"
#include "metrics.h"
#include "predictor_v2.h"
#include "replay_loader.h"
#include "scenario_runner.h"
#include "safety_supervisor.h"
#include "simulation_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int g_failures;

static void require_true(const char *name, int condition)
{
    if (!condition)
    {
        fprintf(stderr, "FAILED: %s\n", name);
        g_failures++;
    }
    else
    {
        printf("PASS: %s\n", name);
    }
}

static predictor_v2_input_t make_input(uint32_t epoch_s,
                                       uint16_t glucose_mgdl,
                                       int16_t trend_mgdl_min_x100,
                                       uint8_t sqi_pct,
                                       uint16_t sensor_flags,
                                       float iob_u,
                                       float cob_g,
                                       bool physiology_present)
{
    predictor_v2_input_t input;

    memset(&input, 0, sizeof(input));
    input.cgm.epoch_s = epoch_s;
    input.cgm.glucose_mgdl = glucose_mgdl;
    input.cgm.trend_mgdl_min_x100 = trend_mgdl_min_x100;
    input.cgm.sqi_pct = sqi_pct;
    input.cgm.sensor_flags = sensor_flags;
    input.cgm.valid = true;
    input.physiology.iob_u = iob_u;
    input.physiology.cob_g = cob_g;
    input.physiology.basal_u_per_hr = 0.8f;
    input.physiology_present = physiology_present;
    return input;
}

static void write_text_file(const char *path, const char *contents)
{
    FILE *file = fopen(path, "w");
    require_true("open temp file", file != NULL);
    if (file != NULL)
    {
        fputs(contents, file);
        fclose(file);
    }
}

static void test_predictor_outputs(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t output;
    uint32_t sample_index;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);

    for (sample_index = 0u; sample_index < 12u; ++sample_index)
    {
        input = make_input(300u * sample_index, (uint16_t)(140u + sample_index), 15, 95u, 0u, 0.3f, 5.0f, true);
        require_true("predictor warm-up update", PredictorV2_Update(&input, &output));
    }

    input = make_input(3600u, 160u, 20, 95u, 0u, 0.5f, 8.0f, true);
    require_true("predictor update", PredictorV2_Update(&input, &output));
    require_true("predictor produces 15m", output.pred_15m_mgdl > 0u);
    require_true("predictor produces 30m", output.pred_30m_mgdl > 0u);
    require_true("predictor produces 60m", output.pred_60m_mgdl > 0u);
    require_true("predictor horizons differ", (output.pred_15m_mgdl != output.pred_30m_mgdl) || (output.pred_30m_mgdl != output.pred_60m_mgdl));
}

static void configure_modules(void)
{
    openaps_controller_config_t controller_config;
    aps_safety_config_t safety_config;

    OpenApsController_Reset();
    controller_config.target_low_mgdl = 100u;
    controller_config.target_high_mgdl = 120u;
    controller_config.suspend_threshold_mgdl = 80u;
    controller_config.basal_u_per_hr = 0.8f;
    controller_config.max_basal_u_per_hr = 2.0f;
    controller_config.insulin_sensitivity_mgdl_per_u = 50.0f;
    controller_config.carb_ratio_g_per_u = 12.0f;
    controller_config.max_iob_u = 5.0f;
    OpenApsController_Configure(&controller_config);

    safety_config.min_valid_glucose_mgdl = 40u;
    safety_config.max_valid_glucose_mgdl = 400u;
    safety_config.low_prediction_threshold_mgdl = 80u;
    safety_config.min_sqi_for_increase_pct = 60u;
    safety_config.max_cgm_age_s = 600u;
    safety_config.max_basal_u_per_hr = 2.0f;
    safety_config.max_iob_u = 5.0f;
    SafetySupervisor_Configure(&safety_config);
}

static void test_controller_and_safety(void)
{
    predictor_v2_input_t input;
    predictor_v2_output_t prediction;
    aps_controller_output_t command;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    configure_modules();

    input = make_input(4000u, 210u, 40, 95u, 0u, 0.2f, 2.0f, true);
    PredictorV2_Update(&input, &prediction);

    require_true("controller basal decision", OpenApsController_DetermineBasal(&input, &prediction, &command));
    require_true("controller can increase", command.action == APS_ACTION_INCREASE_BASAL || command.action == APS_ACTION_NO_CHANGE);

    input.cgm.sqi_pct = 45u;
    command.action = APS_ACTION_INCREASE_BASAL;
    command.requested_basal_u_per_hr = 1.6f;
    command.suggested_correction_u = 1.0f;
    require_true("safety applies", SafetySupervisor_Apply(4300u, &input, &prediction, &command));
    require_true("safety blocks increase on bad sqi", command.action != APS_ACTION_INCREASE_BASAL);
    require_true("safety reason bad sqi", (command.reason_flags & APS_SAFETY_REASON_BAD_SQI) != 0u);
}

static void test_metrics_module(void)
{
    uint16_t actual_values[4] = {100u, 120u, 80u, 140u};
    uint16_t predicted_values[4] = {110u, 118u, 90u, 130u};
    uint32_t safety_flags[4] = {0u, 1u, 0u, 1u};
    prediction_metrics_t prediction_metrics;
    clinical_metrics_t clinical_metrics;

    Metrics_ComputePrediction(actual_values, predicted_values, 4u, &prediction_metrics);
    Metrics_ComputeClinical(actual_values, predicted_values, safety_flags, 4u, &clinical_metrics);

    require_true("metrics mae", prediction_metrics.mae > 0.0f);
    require_true("metrics rmse", prediction_metrics.rmse > 0.0f);
    require_true("metrics clinical range", clinical_metrics.time_in_range_pct > 0.0f);
    require_true("metrics safety count", clinical_metrics.safety_intervention_count == 2u);
}

static void test_replay_loader_success(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_full.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min,carbs_g,insulin_u,basal_u_per_hr\n"
                    "300,110,95,0,0,0,0.8\n"
                    "600,115,95,1,12,0,0.8\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("replay csv loads", ReplayLoader_LoadCsv("test_replay_full.csv", &dataset, error, sizeof(error)));
    require_true("replay csv count", dataset.count == 2u);
    require_true("replay physiology columns present", dataset.physiology_columns_present);
    require_true("replay first timestamp", dataset.steps[0].now_s == 300u);
    remove("test_replay_full.csv");
}

static void test_replay_loader_missing_optional_columns(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_missing.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min\n"
                    "300,210,95,6\n"
                    "600,218,95,7\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("replay missing optional columns loads", ReplayLoader_LoadCsv("test_replay_missing.csv", &dataset, error, sizeof(error)));
    require_true("replay missing columns flagged", !dataset.physiology_columns_present);
    require_true("replay missing columns defaults", dataset.steps[0].insulin_u == 0.0f && dataset.steps[0].carbs_g == 0.0f);
    remove("test_replay_missing.csv");
}

static void test_scenario_runner_and_audit(void)
{
    const char *scenarios[] = {
        "stable in range",
        "rising after meal",
        "meal_rise",
        "falling after bolus",
        "predicted hypo",
        "stale cgm",
        "bad sqi",
        "high iob",
        "meal + insulin overlap",
    };
    size_t scenario_index;

    for (scenario_index = 0u; scenario_index < sizeof(scenarios) / sizeof(scenarios[0]); ++scenario_index)
    {
        replay_dataset_t dataset;
        char error[128];
        sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
        size_t result_count = 0u;

        memset(&dataset, 0, sizeof(dataset));
        memset(error, 0, sizeof(error));
        require_true("scenario load", ScenarioRunner_Load(scenarios[scenario_index], &dataset, error, sizeof(error)));
        require_true("scenario count", dataset.count > 0u);
        configure_modules();
        require_true("scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
        require_true("scenario result count", result_count == dataset.count);
        require_true("scenario baseline predictions", results[0].baseline_prediction.pred_15m_mgdl > 0u);
        require_true("scenario ML predictions", results[0].ml_prediction.pred_15m_mgdl > 0u);
        require_true("scenario safety output", results[0].safety_final_output.action <= APS_ACTION_CORRECTION_SUGGESTION);
    }
}

static void test_safety_reason_codes(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    size_t result_count = 0u;

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("high iob scenario load", ScenarioRunner_Load("high iob", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("high iob scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
    require_true("high iob reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_EXCESSIVE_IOB) != 0u);
}

static void test_missing_physiology_reason(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    size_t result_count = 0u;

    write_text_file("test_replay_missing_phys.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min\n"
                    "300,210,95,8\n"
                    "600,216,95,8\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("missing phys csv loads", ReplayLoader_LoadCsv("test_replay_missing_phys.csv", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("missing phys scenario runs", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, NULL, error, sizeof(error)));
    require_true("missing phys reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_MISSING_PHYSIOLOGY) != 0u);
    require_true("controller blocked reason emitted", (results[result_count - 1u].safety_final_output.reason_flags & APS_SAFETY_REASON_CONTROLLER_BLOCKED) != 0u);
    remove("test_replay_missing_phys.csv");
}

static void test_simulation_summary_and_audit_header(void)
{
    replay_dataset_t dataset;
    char error[128];
    sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
    simulation_summary_t summary;
    size_t result_count = 0u;
    FILE *audit_file;
    char header_line[512];

    memset(&dataset, 0, sizeof(dataset));
    memset(&summary, 0, sizeof(summary));
    memset(error, 0, sizeof(error));
    require_true("summary scenario load", ScenarioRunner_Load("stable", &dataset, error, sizeof(error)));
    configure_modules();
    require_true("summary scenario run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, &summary, error, sizeof(error)));
    require_true("summary sample count", summary.sample_count == dataset.count);
    require_true("summary metric baseline", summary.baseline_15m.mae >= 0.0f);
    require_true("summary metric ml", summary.ml_15m.mae >= 0.0f);
    require_true("summary disagreement count", summary.controller_disagreement_count <= dataset.count);
    require_true("summary result has actual", results[0].has_actual_15m || dataset.count < 4u);

    audit_file = tmpfile();
    require_true("audit tmpfile", audit_file != NULL);
    if (audit_file != NULL)
    {
        memset(header_line, 0, sizeof(header_line));
        AuditTrace_PrintHeader(audit_file);
        rewind(audit_file);
        require_true("audit header read", fgets(header_line, sizeof(header_line), audit_file) != NULL);
        require_true("audit header baseline column", strstr(header_line, "baseline_pred_15") != NULL);
        require_true("audit header safety column", strstr(header_line, "safety_reason_codes") != NULL);
        fclose(audit_file);
    }
}

static void test_replay_fixture_regressions(void)
{
    const struct
    {
        const char *path;
        const char *name;
        uint32_t required_reason_mask;
        int expect_disagreement;
    } cases[] = {
        {"data/sample_replay_stable.csv", "stable", 0u, 0},
        {"data/sample_replay_meal_rise.csv", "meal_rise", 0u, 1},
        {"data/sample_replay_falling_bolus.csv", "falling_bolus", APS_SAFETY_REASON_PREDICTED_LOW_15M | APS_SAFETY_REASON_PREDICTED_LOW_30M | APS_SAFETY_REASON_RAPID_FALL, 1},
        {"data/sample_replay_bad_sqi.csv", "bad_sqi", APS_SAFETY_REASON_BAD_SQI | APS_SAFETY_REASON_CONTROLLER_BLOCKED, 0},
        {"data/sample_replay_stale_cgm.csv", "stale_cgm", APS_SAFETY_REASON_STALE_CGM | APS_SAFETY_REASON_CONTROLLER_BLOCKED, 0},
    };
    size_t case_index;

    for (case_index = 0u; case_index < sizeof(cases) / sizeof(cases[0]); ++case_index)
    {
        replay_dataset_t dataset;
        char error[128];
        sim_step_result_t results[REPLAY_LOADER_MAX_STEPS];
        simulation_summary_t summary;
        size_t result_count = 0u;
        FILE *audit_file;
        char header_line[512];

        memset(&dataset, 0, sizeof(dataset));
        memset(&summary, 0, sizeof(summary));
        memset(error, 0, sizeof(error));
        require_true("fixture loads", ReplayLoader_LoadCsv(cases[case_index].path, &dataset, error, sizeof(error)));
        require_true("fixture count", dataset.count > 0u);
        configure_modules();
        require_true("fixture run", SimulationRunner_RunDataset(&dataset, NULL, results, REPLAY_LOADER_MAX_STEPS, &result_count, &summary, error, sizeof(error)));
        require_true("fixture result count", result_count == dataset.count);
        require_true("fixture baseline metric present", summary.baseline_15m.mae >= 0.0f);
        require_true("fixture ml metric present", summary.ml_15m.mae >= 0.0f);
        require_true("fixture disagreement present", summary.controller_disagreement_count >= (uint32_t)(cases[case_index].expect_disagreement ? 1 : 0));

        if (cases[case_index].required_reason_mask != 0u)
        {
            require_true("fixture safety reason present",
                         (results[result_count - 1u].safety_final_output.reason_flags & cases[case_index].required_reason_mask) != 0u);
        }

        audit_file = tmpfile();
        require_true("fixture audit tmpfile", audit_file != NULL);
        if (audit_file != NULL)
        {
            memset(header_line, 0, sizeof(header_line));
            AuditTrace_PrintHeader(audit_file);
            rewind(audit_file);
            require_true("fixture audit header", fgets(header_line, sizeof(header_line), audit_file) != NULL);
            require_true("fixture audit column 1", strstr(header_line, "timestamp,glucose_mgdl,sqi_pct") != NULL);
            require_true("fixture audit column 2", strstr(header_line, "baseline_pred_15") != NULL);
            require_true("fixture audit column 3", strstr(header_line, "safety_reason_codes") != NULL);
            fclose(audit_file);
        }
    }
}

static void test_bad_data_rejected_safely(void)
{
    replay_dataset_t dataset;
    char error[128];

    write_text_file("test_replay_bad.csv",
                    "timestamp,glucose_mgdl,sqi_pct,trend_mgdl_min,carbs_g,insulin_u,basal_u_per_hr\n"
                    "0,500,95,0,0,0,0.8\n");

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));
    require_true("bad data rejected", !ReplayLoader_LoadCsv("test_replay_bad.csv", &dataset, error, sizeof(error)));
    remove("test_replay_bad.csv");
}

int main(void)
{
    ApsPhysiology_Reset();
    PredictorV2_SetEnabled(true);
    test_predictor_outputs();
    test_controller_and_safety();
    test_metrics_module();
    test_replay_loader_success();
    test_replay_loader_missing_optional_columns();
    test_scenario_runner_and_audit();
    test_safety_reason_codes();
    test_missing_physiology_reason();
    test_simulation_summary_and_audit_header();
    test_replay_fixture_regressions();
    test_bad_data_rejected_safely();

    if (g_failures != 0)
    {
        fprintf(stderr, "%d test(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }

    printf("All host foundation tests passed\n");
    return EXIT_SUCCESS;
}
