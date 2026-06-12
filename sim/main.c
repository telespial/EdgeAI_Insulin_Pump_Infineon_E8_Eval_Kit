#include "replay_loader.h"
#include "scenario_runner.h"
#include "simulation_runner.h"

#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(void)
{
    fprintf(stderr, "Usage: e84_aps_sim --replay <csv> | --scenario <name> [--out <audit.csv>]\n");
}

static bool ensure_parent_directories(const char *path, char *error, size_t error_length)
{
    char buffer[1024];
    size_t length;
    size_t index;

    if (path == NULL)
    {
        return true;
    }

    length = strlen(path);
    if (length == 0u || length >= sizeof(buffer))
    {
        if (error != NULL && error_length > 0u)
        {
            snprintf(error, error_length, "invalid output path");
        }
        return false;
    }

    memcpy(buffer, path, length + 1u);
    for (index = 1u; index < length; ++index)
    {
        if (buffer[index] == '/')
        {
            struct stat stat_info;

            buffer[index] = '\0';
            if (stat(buffer, &stat_info) != 0)
            {
                if (mkdir(buffer, 0775) != 0 && errno != EEXIST)
                {
                    if (error != NULL && error_length > 0u)
                    {
                        snprintf(error, error_length, "unable to create output directory: %s", buffer);
                    }
                    return false;
                }
            }
            buffer[index] = '/';
        }
    }

    return true;
}

int main(int argc, char **argv)
{
    const char *mode = NULL;
    const char *value = NULL;
    const char *output_path = NULL;
    FILE *audit_stream = NULL;
    replay_dataset_t dataset;
    simulation_summary_t summary;
    char error[256];
    int arg_index;

    memset(&dataset, 0, sizeof(dataset));
    memset(&summary, 0, sizeof(summary));
    memset(error, 0, sizeof(error));

    for (arg_index = 1; arg_index < argc; ++arg_index)
    {
        if (strcmp(argv[arg_index], "--replay") == 0 && arg_index + 1 < argc)
        {
            mode = "--replay";
            value = argv[++arg_index];
        }
        else if (strcmp(argv[arg_index], "--scenario") == 0 && arg_index + 1 < argc)
        {
            mode = "--scenario";
            value = argv[++arg_index];
        }
        else if (strcmp(argv[arg_index], "--out") == 0 && arg_index + 1 < argc)
        {
            output_path = argv[++arg_index];
        }
        else
        {
            print_usage();
            return 1;
        }
    }

    if (mode == NULL || value == NULL)
    {
        print_usage();
        return 1;
    }

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

    if (!ensure_parent_directories(output_path, error, sizeof(error)))
    {
        fprintf(stderr, "Output path error: %s\n", error);
        return 1;
    }

    if (output_path != NULL)
    {
        audit_stream = fopen(output_path, "w");
        if (audit_stream == NULL)
        {
            fprintf(stderr, "Unable to open audit output: %s\n", output_path);
            return 1;
        }
    }

    if (!SimulationRunner_RunDataset(&dataset, audit_stream != NULL ? audit_stream : stdout, NULL, 0u, NULL, &summary, error, sizeof(error)))
    {
        if (audit_stream != NULL)
        {
            fclose(audit_stream);
        }
        fprintf(stderr, "Simulation failed: %s\n", error);
        return 1;
    }

    if (audit_stream != NULL)
    {
        fclose(audit_stream);
    }

    SimulationRunner_PrintSummary(stderr, &summary);
    return 0;
}
