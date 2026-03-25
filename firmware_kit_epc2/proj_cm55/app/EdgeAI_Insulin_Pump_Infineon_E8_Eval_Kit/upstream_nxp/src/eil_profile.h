#ifndef EDGEAI_EIL_PROFILE_H
#define EDGEAI_EIL_PROFILE_H

typedef struct
{
    float alert_warn;
    float alert_fail;
    float weight_ax;
    float weight_ay;
    float weight_az;
    float weight_temp_c;
} eil_profile_t;

const eil_profile_t *EilProfile_Get(void);
const char *EilProfile_GetModelName(void);
const char *EilProfile_GetModelVersion(void);
const char *EilProfile_GetExtensionVersion(void);

#endif /* EDGEAI_EIL_PROFILE_H */
