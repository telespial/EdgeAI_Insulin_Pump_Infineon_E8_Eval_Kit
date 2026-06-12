#ifndef REPLAY_LOADER_H
#define REPLAY_LOADER_H

#include "aps_types.h"

#include <stddef.h>

#define REPLAY_LOADER_MAX_STEPS 512u

typedef struct
{
    uint32_t now_s;
    predictor_v2_input_t input;
    float insulin_u;
    float basal_u_per_hr;
    float carbs_g;
    int16_t accel_ax_mg;
    int16_t accel_ay_mg;
    int16_t accel_az_mg;
    uint16_t step_count;
    bool has_insulin;
    bool has_basal;
    bool has_carbs;
    bool has_activity;
    bool has_step_count;
} replay_step_t;

typedef struct
{
    replay_step_t steps[REPLAY_LOADER_MAX_STEPS];
    size_t count;
    bool physiology_columns_present;
} replay_dataset_t;

bool ReplayLoader_LoadCsv(const char *path, replay_dataset_t *dataset, char *error, size_t error_len);

#endif
