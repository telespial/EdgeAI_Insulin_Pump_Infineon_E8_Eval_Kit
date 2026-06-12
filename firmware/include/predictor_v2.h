#ifndef PREDICTOR_V2_H
#define PREDICTOR_V2_H

#include "aps_types.h"
#include "predictor_v2_generated.h"

void PredictorV2_Reset(void);
void PredictorV2_SetEnabled(bool enabled);
bool PredictorV2_IsEnabled(void);
const predictor_v2_generated_bundle_t *PredictorV2_GetDefaultModelBundle(void);
void PredictorV2_SetModelBundleForTesting(const predictor_v2_generated_bundle_t *bundle);
bool PredictorV2_BuildFeatureVector(const predictor_v2_input_t *input, predictor_v2_feature_vector_t *features);
bool PredictorV2_EvaluateHorizon(const predictor_v2_input_t *input,
                                 predictor_v2_horizon_t horizon,
                                 predictor_v2_horizon_eval_t *evaluation);
bool PredictorV2_UsesDynamicMemory(void);
bool PredictorV2_Update(const predictor_v2_input_t *input, predictor_v2_output_t *output);

#endif
