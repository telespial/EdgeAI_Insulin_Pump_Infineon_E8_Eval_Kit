#ifndef PREDICTOR_V2_H
#define PREDICTOR_V2_H

#include "aps_types.h"

void PredictorV2_Reset(void);
void PredictorV2_SetEnabled(bool enabled);
bool PredictorV2_IsEnabled(void);
bool PredictorV2_Update(const predictor_v2_input_t *input, predictor_v2_output_t *output);

#endif
