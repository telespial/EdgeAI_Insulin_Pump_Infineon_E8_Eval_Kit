#ifndef SCENARIO_RUNNER_H
#define SCENARIO_RUNNER_H

#include "replay_loader.h"

bool ScenarioRunner_Load(const char *scenario_name, replay_dataset_t *dataset, char *error, size_t error_len);

#endif
