# Predictor V2 Feature Importance

This review ranks the trained enriched-real-data candidate by absolute coefficient magnitude. All coefficients are from the standardized linear model, so the ranking reflects relative contribution inside each horizon rather than raw unit scale.

## 15m

### Top 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `rolling_mean_6` | -101.463775 | glucose history |
| 2 | `rolling_mean_3` | -34.684266 | glucose history |
| 3 | `motion_rms_5m` | -32.949288 | activity |
| 4 | `lag_1` | 23.208053 | glucose history |
| 5 | `current_glucose` | 22.911995 | glucose history |
| 6 | `motion_rms_15m` | 22.360079 | activity |
| 7 | `lag_3` | 22.042794 | glucose history |
| 8 | `lag_6` | 21.145602 | glucose history |
| 9 | `rolling_mean_12` | 15.577010 | glucose history |
| 10 | `lag_12` | 14.927660 | glucose history |
| 11 | `slope_60m` | 12.713769 | glucose dynamics |
| 12 | `lag_2` | 8.059832 | glucose history |
| 13 | `volatility_score` | 6.932400 | glucose dynamics |
| 14 | `carbs_30m` | -5.972021 | COB |
| 15 | `lag_9` | 5.579871 | glucose history |
| 16 | `delta_6` | 4.718842 | glucose dynamics |
| 17 | `slope_30m` | 4.718842 | glucose dynamics |
| 18 | `slope_15m` | 4.022942 | glucose dynamics |
| 19 | `delta_3` | 4.022942 | glucose dynamics |
| 20 | `rolling_stddev_6` | 4.016668 | glucose dynamics |

### Bottom 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `insulin_30m` | 0.002992 | IOB |
| 2 | `iob` | -0.029053 | IOB |
| 3 | `active_minutes` | -0.136885 | activity |
| 4 | `insulin_120m` | 0.205447 | IOB |
| 5 | `sqi` | 0.219510 | quality/confidence |
| 6 | `basal_rate` | 0.258400 | IOB |
| 7 | `activity_state` | -0.319928 | activity |
| 8 | `tod_cos` | 0.372850 | time features |
| 9 | `post_exercise_minutes` | 0.416051 | activity |
| 10 | `carbs_120m` | -0.770146 | COB |
| 11 | `cob` | -0.770146 | COB |
| 12 | `tod_sin` | -0.818568 | time features |
| 13 | `cgm_age_s` | 1.297285 | quality/confidence |
| 14 | `delta_1` | -2.699233 | glucose dynamics |
| 15 | `rolling_variance_6` | 3.102574 | glucose dynamics |
| 16 | `activity_confidence` | 3.461321 | activity |
| 17 | `rolling_stddev_6` | 4.016668 | glucose dynamics |
| 18 | `delta_3` | 4.022942 | glucose dynamics |
| 19 | `slope_15m` | 4.022942 | glucose dynamics |
| 20 | `slope_30m` | 4.718842 | glucose dynamics |

### Near-Zero Features
| Feature | Coefficient |
|---|---:|
| `insulin_30m` | 0.002992 |
| `iob` | -0.029053 |
| `active_minutes` | -0.136885 |
| `insulin_120m` | 0.205447 |
| `sqi` | 0.219510 |
| `basal_rate` | 0.258400 |
| `activity_state` | -0.319928 |
| `tod_cos` | 0.372850 |
| `post_exercise_minutes` | 0.416051 |

### Group Ranking
| Group | Sum | Max | Count |
|---|---:|---:|---:|
| glucose history | 269.600856 | 101.463775 | 10 |
| glucose dynamics | 46.948213 | 12.713769 | 9 |
| time features | 1.191418 | 0.818568 | 2 |
| IOB | 0.495892 | 0.258400 | 4 |
| COB | 7.512312 | 5.972021 | 3 |
| activity | 59.643553 | 32.949288 | 6 |
| quality/confidence | 1.516795 | 1.297285 | 2 |

### What Helps
- `glucose history` is the dominant signal group at every horizon.
- `glucose dynamics` is the next strongest group and helps the model follow rising/falling patterns.
- `activity` contributes noticeably, mainly through motion proxies, but less than the glucose history terms.
- `COB` matters more than `IOB` in this candidate, largely because the imported real data still lacks direct meal annotations.
- `time features` and `quality/confidence` are present but small; they help conditioning more than raw accuracy.
- `IOB` is useful but comparatively modest, suggesting the decay proxy is acting as a secondary stabilizer rather than the main predictor.

## 30m

### Top 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `rolling_mean_3` | -344.003232 | glucose history |
| 2 | `rolling_mean_6` | -134.826366 | glucose history |
| 3 | `lag_2` | 98.404877 | glucose history |
| 4 | `lag_1` | 77.383544 | glucose history |
| 5 | `current_glucose` | 76.521925 | glucose history |
| 6 | `lag_3` | 74.103198 | glucose history |
| 7 | `lag_6` | 71.505207 | glucose history |
| 8 | `lag_12` | 53.409963 | glucose history |
| 9 | `motion_rms_5m` | -37.765079 | activity |
| 10 | `slope_60m` | 36.778030 | glucose dynamics |
| 11 | `motion_rms_15m` | 28.736616 | activity |
| 12 | `rolling_mean_12` | 18.208128 | glucose history |
| 13 | `slope_30m` | 13.395472 | glucose dynamics |
| 14 | `delta_6` | 13.395472 | glucose dynamics |
| 15 | `slope_15m` | 11.188851 | glucose dynamics |
| 16 | `delta_3` | 11.188850 | glucose dynamics |
| 17 | `carbs_30m` | -10.285350 | COB |
| 18 | `volatility_score` | 10.181623 | glucose dynamics |
| 19 | `delta_1` | -7.847372 | glucose dynamics |
| 20 | `rolling_variance_6` | 6.575230 | glucose dynamics |

### Bottom 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `basal_rate` | 0.174186 | IOB |
| 2 | `insulin_120m` | 0.336348 | IOB |
| 3 | `sqi` | -0.348671 | quality/confidence |
| 4 | `insulin_30m` | 0.424390 | IOB |
| 5 | `rolling_stddev_6` | -0.428715 | glucose dynamics |
| 6 | `iob` | -0.446838 | IOB |
| 7 | `activity_state` | -0.590694 | activity |
| 8 | `post_exercise_minutes` | 0.737329 | activity |
| 9 | `active_minutes` | -0.755686 | activity |
| 10 | `tod_cos` | 1.088787 | time features |
| 11 | `cob` | -1.557906 | COB |
| 12 | `carbs_120m` | -1.557906 | COB |
| 13 | `tod_sin` | -2.136744 | time features |
| 14 | `cgm_age_s` | 2.158918 | quality/confidence |
| 15 | `lag_9` | 3.896379 | glucose history |
| 16 | `activity_confidence` | 5.043327 | activity |
| 17 | `rolling_variance_6` | 6.575230 | glucose dynamics |
| 18 | `delta_1` | -7.847372 | glucose dynamics |
| 19 | `volatility_score` | 10.181623 | glucose dynamics |
| 20 | `carbs_30m` | -10.285350 | COB |

### Near-Zero Features
| Feature | Coefficient |
|---|---:|
| `basal_rate` | 0.174186 |
| `insulin_120m` | 0.336348 |
| `sqi` | -0.348671 |
| `insulin_30m` | 0.424390 |
| `rolling_stddev_6` | -0.428715 |
| `iob` | -0.446838 |

### Group Ranking
| Group | Sum | Max | Count |
|---|---:|---:|---:|
| glucose history | 952.262819 | 344.003232 | 10 |
| glucose dynamics | 110.979616 | 36.778030 | 9 |
| time features | 3.225532 | 2.136744 | 2 |
| IOB | 1.381762 | 0.446838 | 4 |
| COB | 13.401163 | 10.285350 | 3 |
| activity | 73.628732 | 37.765079 | 6 |
| quality/confidence | 2.507589 | 2.158918 | 2 |

### What Helps
- `glucose history` is the dominant signal group at every horizon.
- `glucose dynamics` is the next strongest group and helps the model follow rising/falling patterns.
- `activity` contributes noticeably, mainly through motion proxies, but less than the glucose history terms.
- `COB` matters more than `IOB` in this candidate, largely because the imported real data still lacks direct meal annotations.
- `time features` and `quality/confidence` are present but small; they help conditioning more than raw accuracy.
- `IOB` is useful but comparatively modest, suggesting the decay proxy is acting as a secondary stabilizer rather than the main predictor.

## 60m

### Top 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `rolling_mean_3` | -282.877928 | glucose history |
| 2 | `rolling_mean_6` | -172.403510 | glucose history |
| 3 | `lag_2` | 75.510407 | glucose history |
| 4 | `lag_1` | 75.107084 | glucose history |
| 5 | `current_glucose` | 74.475091 | glucose history |
| 6 | `lag_3` | 71.998061 | glucose history |
| 7 | `lag_6` | 68.777477 | glucose history |
| 8 | `motion_rms_15m` | 61.673452 | activity |
| 9 | `lag_12` | 53.096098 | glucose history |
| 10 | `motion_rms_5m` | -44.314970 | activity |
| 11 | `slope_60m` | 34.011658 | glucose dynamics |
| 12 | `rolling_stddev_6` | -32.198239 | glucose dynamics |
| 13 | `carbs_30m` | -21.017578 | COB |
| 14 | `rolling_mean_12` | 18.145559 | glucose history |
| 15 | `volatility_score` | 16.973886 | glucose dynamics |
| 16 | `slope_30m` | 15.220604 | glucose dynamics |
| 17 | `delta_6` | 15.220604 | glucose dynamics |
| 18 | `rolling_variance_6` | 13.224063 | glucose dynamics |
| 19 | `slope_15m` | 11.460326 | glucose dynamics |
| 20 | `delta_3` | 11.460326 | glucose dynamics |

### Bottom 20
| Rank | Feature | Coefficient | Group |
|---|---|---:|---|
| 1 | `cgm_age_s` | 0.278043 | quality/confidence |
| 2 | `insulin_30m` | 0.396385 | IOB |
| 3 | `post_exercise_minutes` | 0.468170 | activity |
| 4 | `basal_rate` | 0.602255 | IOB |
| 5 | `activity_state` | -0.635298 | activity |
| 6 | `insulin_120m` | 0.661253 | IOB |
| 7 | `cob` | -2.023302 | COB |
| 8 | `carbs_120m` | -2.023302 | COB |
| 9 | `active_minutes` | -2.126677 | activity |
| 10 | `iob` | -2.285375 | IOB |
| 11 | `tod_cos` | 2.606640 | time features |
| 12 | `activity_confidence` | 4.130650 | activity |
| 13 | `lag_9` | 4.132084 | glucose history |
| 14 | `sqi` | -4.202638 | quality/confidence |
| 15 | `tod_sin` | -5.397115 | time features |
| 16 | `delta_1` | -5.740880 | glucose dynamics |
| 17 | `delta_3` | 11.460326 | glucose dynamics |
| 18 | `slope_15m` | 11.460326 | glucose dynamics |
| 19 | `rolling_variance_6` | 13.224063 | glucose dynamics |
| 20 | `delta_6` | 15.220604 | glucose dynamics |

### Near-Zero Features
| Feature | Coefficient |
|---|---:|
| `cgm_age_s` | 0.278043 |
| `insulin_30m` | 0.396385 |
| `post_exercise_minutes` | 0.468170 |

### Group Ranking
| Group | Sum | Max | Count |
|---|---:|---:|---:|
| glucose history | 896.523298 | 282.877928 | 10 |
| glucose dynamics | 155.510584 | 34.011658 | 9 |
| time features | 8.003754 | 5.397115 | 2 |
| IOB | 3.945268 | 2.285375 | 4 |
| COB | 25.064182 | 21.017578 | 3 |
| activity | 113.349216 | 61.673452 | 6 |
| quality/confidence | 4.480681 | 4.202638 | 2 |

### What Helps
- `glucose history` is the dominant signal group at every horizon.
- `glucose dynamics` is the next strongest group and helps the model follow rising/falling patterns.
- `activity` contributes noticeably, mainly through motion proxies, but less than the glucose history terms.
- `COB` matters more than `IOB` in this candidate, largely because the imported real data still lacks direct meal annotations.
- `time features` and `quality/confidence` are present but small; they help conditioning more than raw accuracy.
- `IOB` is useful but comparatively modest, suggesting the decay proxy is acting as a secondary stabilizer rather than the main predictor.
