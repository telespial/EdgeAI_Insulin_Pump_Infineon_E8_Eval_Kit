#include "scenario_runner.h"

#include <stdio.h>
#include <string.h>

static void clear_dataset(replay_dataset_t *dataset)
{
    memset(dataset, 0, sizeof(*dataset));
}

static replay_step_t make_step(uint32_t now_s,
                               uint16_t glucose_mgdl,
                               int16_t trend_mgdl_min_x100,
                               uint8_t sqi_pct,
                               uint16_t sensor_flags,
                               float carbs_g,
                               float insulin_u,
                               float basal_u_per_hr,
                               bool physiology_present,
                               uint32_t cgm_epoch_s)
{
    replay_step_t step;

    memset(&step, 0, sizeof(step));
    step.now_s = now_s;
    step.input.cgm.epoch_s = cgm_epoch_s;
    step.input.cgm.glucose_mgdl = glucose_mgdl;
    step.input.cgm.trend_mgdl_min_x100 = trend_mgdl_min_x100;
    step.input.cgm.sqi_pct = sqi_pct;
    step.input.cgm.sensor_flags = sensor_flags;
    step.input.cgm.valid = true;
    step.input.physiology.basal_u_per_hr = basal_u_per_hr;
    step.input.physiology_present = physiology_present;
    step.carbs_g = carbs_g;
    step.insulin_u = insulin_u;
    step.basal_u_per_hr = basal_u_per_hr;
    step.has_carbs = true;
    step.has_insulin = true;
    step.has_basal = true;
    return step;
}

static bool append_step(replay_dataset_t *dataset, const replay_step_t *step)
{
    if (dataset->count >= REPLAY_LOADER_MAX_STEPS)
    {
        return false;
    }
    dataset->steps[dataset->count++] = *step;
    return true;
}

static void set_common_good_input(replay_step_t *step)
{
    step->input.physiology_present = true;
}

static bool build_stable_in_range(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 12u; ++index)
    {
        replay_step_t step = make_step(300u * (index + 1u), 110u, 0, 95u, 0u, 0.0f, 0.0f, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_rising_after_meal(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 12u; ++index)
    {
        uint16_t glucose = (uint16_t)(118u + (index * 8u));
        replay_step_t step = make_step(300u * (index + 1u), glucose, 12, 95u, 0u, (index == 0u) ? 45.0f : 0.0f, 0.0f, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_falling_after_bolus(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 12u; ++index)
    {
        uint16_t glucose = (uint16_t)(165u - (index * 7u));
        replay_step_t step = make_step(300u * (index + 1u), glucose, -18, 95u, 0u, 0.0f, (index == 0u) ? 4.0f : 0.0f, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_predicted_hypo(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 12u; ++index)
    {
        uint16_t glucose = (uint16_t)(96u - (index * 4u));
        replay_step_t step = make_step(300u * (index + 1u), glucose, -25, 95u, 0u, 0.0f, 0.0f, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_stale_cgm(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 8u; ++index)
    {
        uint32_t now_s = 300u * (index + 1u);
        replay_step_t step = make_step(now_s, 132u, 0, 95u, APS_SENSOR_FLAG_STALE_CGM, 0.0f, 0.0f, 0.8f, true, now_s - 1200u);
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_bad_sqi(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 10u; ++index)
    {
        replay_step_t step = make_step(300u * (index + 1u), 198u, 10, 42u, APS_SENSOR_FLAG_BAD_SQI, 0.0f, 0.0f, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_high_iob(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 10u; ++index)
    {
        float iob = (index == 0u) ? 6.0f : 5.8f;
        replay_step_t step = make_step(300u * (index + 1u), 224u, 18, 95u, 0u, 0.0f, iob, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

static bool build_meal_and_insulin_overlap(replay_dataset_t *dataset)
{
    uint32_t index;
    clear_dataset(dataset);
    dataset->physiology_columns_present = true;
    for (index = 0u; index < 12u; ++index)
    {
        float carbs = (index == 0u) ? 55.0f : 0.0f;
        float insulin = (index == 0u) ? 3.5f : 0.0f;
        replay_step_t step = make_step(300u * (index + 1u), (uint16_t)(128u + (index * 5u)), 8, 95u, 0u, carbs, insulin, 0.8f, true, 300u * (index + 1u));
        set_common_good_input(&step);
        if (!append_step(dataset, &step))
        {
            return false;
        }
    }
    return true;
}

bool ScenarioRunner_Load(const char *scenario_name, replay_dataset_t *dataset, char *error, size_t error_length)
{
    if (scenario_name == NULL || dataset == NULL)
    {
        if (error != NULL && error_length > 0u)
        {
            snprintf(error, error_length, "invalid arguments");
        }
        return false;
    }

    if (strcmp(scenario_name, "stable in range") == 0 || strcmp(scenario_name, "stable") == 0)
    {
        return build_stable_in_range(dataset);
    }
    if (strcmp(scenario_name, "rising after meal") == 0)
    {
        return build_rising_after_meal(dataset);
    }
    if (strcmp(scenario_name, "falling after bolus") == 0)
    {
        return build_falling_after_bolus(dataset);
    }
    if (strcmp(scenario_name, "predicted hypo") == 0)
    {
        return build_predicted_hypo(dataset);
    }
    if (strcmp(scenario_name, "stale cgm") == 0)
    {
        return build_stale_cgm(dataset);
    }
    if (strcmp(scenario_name, "bad sqi") == 0)
    {
        return build_bad_sqi(dataset);
    }
    if (strcmp(scenario_name, "high iob") == 0)
    {
        return build_high_iob(dataset);
    }
    if (strcmp(scenario_name, "meal + insulin overlap") == 0)
    {
        return build_meal_and_insulin_overlap(dataset);
    }

    if (error != NULL && error_length > 0u)
    {
        snprintf(error, error_length, "unknown scenario: %s", scenario_name);
    }
    return false;
}
