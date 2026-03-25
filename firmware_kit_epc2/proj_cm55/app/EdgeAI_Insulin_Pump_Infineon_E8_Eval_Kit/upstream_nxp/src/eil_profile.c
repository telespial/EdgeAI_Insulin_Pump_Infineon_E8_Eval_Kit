#include "eil_profile.h"
#include "eil_profile_generated.h"

static const eil_profile_t s_profile = {
    .alert_warn = EIL_ALERT_WARN,
    .alert_fail = EIL_ALERT_FAIL,
    .weight_ax = EIL_WEIGHT_ACCEL_X,
    .weight_ay = EIL_WEIGHT_ACCEL_Y,
    .weight_az = EIL_WEIGHT_ACCEL_Z,
    .weight_temp_c = EIL_WEIGHT_TEMP_C,
};

const eil_profile_t *EilProfile_Get(void)
{
    return &s_profile;
}

const char *EilProfile_GetModelName(void)
{
    return EIL_MODEL_NAME;
}

const char *EilProfile_GetModelVersion(void)
{
    return EIL_MODEL_VERSION;
}

const char *EilProfile_GetExtensionVersion(void)
{
    return EIL_EXTENSION_VERSION;
}
