#include "replay_loader.h"
#include "scenario_runner.h"
#include "simulation_runner.h"

#include <stdio.h>
#include <string.h>

static void print_usage(void)
{
    fprintf(stderr, "Usage: e84_aps_sim --replay <csv> | --scenario <name>\n");
}

int main(int argc, char **argv)
{
    const char *mode = NULL;
    const char *value = NULL;
    replay_dataset_t dataset;
    char error[256];

    memset(&dataset, 0, sizeof(dataset));
    memset(error, 0, sizeof(error));

    if (argc != 3)
    {
        print_usage();
        return 1;
    }

    mode = argv[1];
    value = argv[2];

    if (strcmp(mode, "--replay") == 0)
    {
        if (!ReplayLoader_LoadCsv(value, &dataset, error, sizeof(error)))
        {
            fprintf(stderr, "Replay load failed: %s\n", error);
            return 1;
        }
    }
    else if (strcmp(mode, "--scenario") == 0)
    {
        if (!ScenarioRunner_Load(value, &dataset, error, sizeof(error)))
        {
            fprintf(stderr, "Scenario load failed: %s\n", error);
            return 1;
        }
    }
    else
    {
        print_usage();
        return 1;
    }

    if (!SimulationRunner_RunDataset(&dataset, stdout, NULL, 0u, NULL, error, sizeof(error)))
    {
        fprintf(stderr, "Simulation failed: %s\n", error);
        return 1;
    }

    return 0;
}
