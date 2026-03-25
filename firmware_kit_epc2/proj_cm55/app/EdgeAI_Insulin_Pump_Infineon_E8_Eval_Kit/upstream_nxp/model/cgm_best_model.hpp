#pragma once

#include <cstddef>

namespace cgm_best {

constexpr std::size_t kFeatureCount = 22u;
// 0: epoch_s
// 1: glucose_mgdl
// 2: insulin_fast_u
// 3: insulin_slow_u
// 4: insulin_total_u
// 5: glucose_mgdl_lag1
// 6: glucose_mgdl_lag2
// 7: glucose_mgdl_lag3
// 8: insulin_total_u_lag1
// 9: insulin_total_u_lag2
// 10: insulin_total_u_lag3
// 11: insulin_fast_u_lag1
// 12: insulin_fast_u_lag2
// 13: insulin_fast_u_lag3
// 14: insulin_slow_u_lag1
// 15: insulin_slow_u_lag2
// 16: insulin_slow_u_lag3
// 17: glucose_delta1
// 18: glucose_delta2
// 19: glucose_accel
// 20: glucose_roll3
// 21: glucose_roll6

// Returns regression value or binary probability depending on model task.
float PredictScore(const float *features, std::size_t len);

// Returns true if score corresponds to classification probability.
bool IsClassification();

} // namespace cgm_best
