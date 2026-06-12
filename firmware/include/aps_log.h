#ifndef APS_LOG_H
#define APS_LOG_H

#include "safety_supervisor.h"

void ApsLog_Decision(uint32_t now_s,
                     const predictor_v2_input_t *input,
                     const predictor_v2_output_t *prediction,
                     const aps_controller_output_t *command);

#endif
