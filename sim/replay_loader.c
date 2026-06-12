#include "replay_loader.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int timestamp;
    int glucose;
    int sqi;
    int trend;
    int carbs;
    int insulin;
    int basal;
} column_index_t;

static void set_error(char *error, size_t error_length, const char *message)
{
    if (error != NULL && error_length > 0u)
    {
        snprintf(error, error_length, "%s", message);
    }
}

static void set_error_row(char *error, size_t error_length, const char *message, size_t row_number)
{
    if (error != NULL && error_length > 0u)
    {
        snprintf(error, error_length, "%s at row %zu", message, row_number);
    }
}

static char *trim(char *text)
{
    while (*text != '\0' && isspace((unsigned char)*text))
    {
        text++;
    }
    char *end = text + strlen(text);
    while (end > text && isspace((unsigned char)end[-1]))
    {
        --end;
    }
    *end = '\0';
    return text;
}

static int parse_int_field(const char *text, int *out)
{
    char *end = NULL;
    long value;

    if (text == NULL || out == NULL)
    {
        return 0;
    }

    value = strtol(text, &end, 10);
    if (end == text || *trim(end) != '\0')
    {
        return 0;
    }
    *out = (int)value;
    return 1;
}

static int parse_float_field(const char *text, float *out)
{
    char *end = NULL;
    float value;

    if (text == NULL || out == NULL)
    {
        return 0;
    }

    value = strtof(text, &end);
    if (end == text || *trim(end) != '\0')
    {
        return 0;
    }
    *out = value;
    return 1;
}

static int find_header_index(char **headers, size_t header_count, const char *name)
{
    size_t index;

    for (index = 0u; index < header_count; ++index)
    {
        if (strcmp(headers[index], name) == 0)
        {
            return (int)index;
        }
    }

    return -1;
}

static size_t split_csv_line(char *line, char **fields, size_t max_fields)
{
    size_t count = 0u;
    char *cursor = line;

    while (cursor != NULL && count < max_fields)
    {
        char *comma = strchr(cursor, ',');
        if (comma != NULL)
        {
            *comma = '\0';
            fields[count++] = trim(cursor);
            cursor = comma + 1;
        }
        else
        {
            fields[count++] = trim(cursor);
            break;
        }
    }

    return count;
}

static const char *field_or_empty(char **fields, size_t field_count, int index)
{
    if (index < 0 || (size_t)index >= field_count)
    {
        return "";
    }
    return fields[index];
}

static int row_has_required_fields(const column_index_t *cols)
{
    return cols->timestamp >= 0 && cols->glucose >= 0;
}

bool ReplayLoader_LoadCsv(const char *path, replay_dataset_t *dataset, char *error, size_t error_length)
{
    FILE *file = NULL;
    char line[512];
    char header_line[512];
    char *fields[16];
    char *headers[16];
    column_index_t cols = {-1, -1, -1, -1, -1, -1, -1};
    uint32_t previous_timestamp = 0u;
    size_t line_number = 0u;

    if (dataset == NULL || path == NULL)
    {
        set_error(error, error_length, "invalid arguments");
        return false;
    }

    memset(dataset, 0, sizeof(*dataset));
    file = fopen(path, "r");
    if (file == NULL)
    {
        set_error(error, error_length, "unable to open CSV file");
        return false;
    }

    if (fgets(header_line, sizeof(header_line), file) == NULL)
    {
        fclose(file);
        set_error(error, error_length, "CSV file is empty");
        return false;
    }
    line_number++;

    size_t header_count = split_csv_line(header_line, headers, 16u);
    cols.timestamp = find_header_index(headers, header_count, "timestamp");
    cols.glucose = find_header_index(headers, header_count, "glucose_mgdl");
    cols.sqi = find_header_index(headers, header_count, "sqi_pct");
    cols.trend = find_header_index(headers, header_count, "trend_mgdl_min");
    cols.carbs = find_header_index(headers, header_count, "carbs_g");
    cols.insulin = find_header_index(headers, header_count, "insulin_u");
    cols.basal = find_header_index(headers, header_count, "basal_u_per_hr");

    if (!row_has_required_fields(&cols))
    {
        fclose(file);
        set_error(error, error_length, "missing required CSV columns");
        return false;
    }

    dataset->physiology_columns_present = (cols.carbs >= 0 && cols.insulin >= 0 && cols.basal >= 0);

    while (fgets(line, sizeof(line), file) != NULL)
    {
        int timestamp = 0;
        int glucose = 0;
        int sqi = 100;
        int trend = 0;
        float carbs = 0.0f;
        float insulin = 0.0f;
        float basal = 0.0f;
        size_t field_count;

        line_number++;
        if (dataset->count >= REPLAY_LOADER_MAX_STEPS)
        {
            fclose(file);
            set_error(error, error_length, "CSV replay exceeded maximum supported rows");
            return false;
        }

        field_count = split_csv_line(line, fields, 16u);

        if (!parse_int_field((char *)field_or_empty(fields, field_count, cols.timestamp), &timestamp) || timestamp <= 0)
        {
            fclose(file);
            set_error_row(error, error_length, "invalid timestamp value", line_number - 1u);
            return false;
        }
        if (!parse_int_field((char *)field_or_empty(fields, field_count, cols.glucose), &glucose) || glucose < 40 || glucose > 400)
        {
            fclose(file);
            set_error_row(error, error_length, "invalid glucose value", line_number - 1u);
            return false;
        }
        if (cols.sqi >= 0 && field_or_empty(fields, field_count, cols.sqi)[0] != '\0')
        {
            if (!parse_int_field((char *)field_or_empty(fields, field_count, cols.sqi), &sqi) || sqi < 0 || sqi > 100)
            {
                fclose(file);
                set_error_row(error, error_length, "invalid sqi value", line_number - 1u);
                return false;
            }
        }
        if (cols.trend >= 0 && field_or_empty(fields, field_count, cols.trend)[0] != '\0')
        {
            if (!parse_int_field((char *)field_or_empty(fields, field_count, cols.trend), &trend))
            {
                fclose(file);
                set_error_row(error, error_length, "invalid trend value", line_number - 1u);
                return false;
            }
        }
        if (cols.carbs >= 0 && field_or_empty(fields, field_count, cols.carbs)[0] != '\0')
        {
            if (!parse_float_field((char *)field_or_empty(fields, field_count, cols.carbs), &carbs) || carbs < 0.0f)
            {
                fclose(file);
                set_error_row(error, error_length, "invalid carbs value", line_number - 1u);
                return false;
            }
        }
        if (cols.insulin >= 0 && field_or_empty(fields, field_count, cols.insulin)[0] != '\0')
        {
            if (!parse_float_field((char *)field_or_empty(fields, field_count, cols.insulin), &insulin) || insulin < 0.0f)
            {
                fclose(file);
                set_error_row(error, error_length, "invalid insulin value", line_number - 1u);
                return false;
            }
        }
        if (cols.basal >= 0 && field_or_empty(fields, field_count, cols.basal)[0] != '\0')
        {
            if (!parse_float_field((char *)field_or_empty(fields, field_count, cols.basal), &basal) || basal < 0.0f)
            {
                fclose(file);
                set_error_row(error, error_length, "invalid basal value", line_number - 1u);
                return false;
            }
        }
        if ((uint32_t)timestamp <= previous_timestamp)
        {
            fclose(file);
            set_error_row(error, error_length, "timestamps must increase monotonically", line_number - 1u);
            return false;
        }

        dataset->steps[dataset->count].now_s = (uint32_t)timestamp;
        dataset->steps[dataset->count].input.cgm.epoch_s = (uint32_t)timestamp;
        dataset->steps[dataset->count].input.cgm.glucose_mgdl = (uint16_t)glucose;
        dataset->steps[dataset->count].input.cgm.trend_mgdl_min_x100 = (int16_t)(trend * 100);
        dataset->steps[dataset->count].input.cgm.sqi_pct = (uint8_t)sqi;
        dataset->steps[dataset->count].input.cgm.sensor_flags = 0u;
        dataset->steps[dataset->count].input.cgm.valid = true;
        dataset->steps[dataset->count].input.physiology.basal_u_per_hr = basal;
        dataset->steps[dataset->count].input.physiology_present = dataset->physiology_columns_present;
        dataset->steps[dataset->count].insulin_u = insulin;
        dataset->steps[dataset->count].carbs_g = carbs;
        dataset->steps[dataset->count].basal_u_per_hr = basal;
        dataset->steps[dataset->count].has_insulin = cols.insulin >= 0;
        dataset->steps[dataset->count].has_carbs = cols.carbs >= 0;
        dataset->steps[dataset->count].has_basal = cols.basal >= 0;

        previous_timestamp = (uint32_t)timestamp;
        dataset->count++;
    }

    fclose(file);
    return dataset->count > 0u;
}
