#ifndef EDGEAI_EV_GAUGE_RENDER_H
#define EDGEAI_EV_GAUGE_RENDER_H

#include <stdbool.h>

#include "power_data_source.h"

#define GAUGE_RENDER_AI_PILL_X0 166
#define GAUGE_RENDER_AI_PILL_Y0 4
#define GAUGE_RENDER_AI_PILL_X1 313
#define GAUGE_RENDER_AI_PILL_Y1 30
#define GAUGE_RENDER_AI_SET_X0 166
#define GAUGE_RENDER_AI_SET_Y0 4
#define GAUGE_RENDER_AI_SET_X1 201
#define GAUGE_RENDER_AI_SET_Y1 30
#define GAUGE_RENDER_AI_HELP_X0 277
#define GAUGE_RENDER_AI_HELP_Y0 4
#define GAUGE_RENDER_AI_HELP_X1 313
#define GAUGE_RENDER_AI_HELP_Y1 30
#define GAUGE_RENDER_PROFILE_X0 351
#define GAUGE_RENDER_PROFILE_Y0 286
#define GAUGE_RENDER_PROFILE_X1 479
#define GAUGE_RENDER_PROFILE_Y1 318
#define GAUGE_RENDER_SET_PANEL_X0 16
#define GAUGE_RENDER_SET_PANEL_Y0 24
#define GAUGE_RENDER_SET_PANEL_X1 464
#define GAUGE_RENDER_SET_PANEL_Y1 308
#define GAUGE_RENDER_SETTINGS_CLOSE_X0 448
#define GAUGE_RENDER_SETTINGS_CLOSE_Y0 8
#define GAUGE_RENDER_SETTINGS_CLOSE_X1 472
#define GAUGE_RENDER_SETTINGS_CLOSE_Y1 32
#define GAUGE_RENDER_SET_MODE_X0 168
#define GAUGE_RENDER_SET_MODE_Y0 54
#define GAUGE_RENDER_SET_MODE_W 108
#define GAUGE_RENDER_SET_MODE_H 24
#define GAUGE_RENDER_SET_MODE_GAP 20
#define GAUGE_RENDER_SET_RUN_X0 168
#define GAUGE_RENDER_SET_RUN_Y0 84
#define GAUGE_RENDER_SET_RUN_W 108
#define GAUGE_RENDER_SET_RUN_H 24
#define GAUGE_RENDER_SET_RUN_GAP 20
#define GAUGE_RENDER_SET_TUNE_X0 162
#define GAUGE_RENDER_SET_TUNE_Y0 114
#define GAUGE_RENDER_SET_TUNE_W 88
#define GAUGE_RENDER_SET_TUNE_H 24
#define GAUGE_RENDER_SET_TUNE_GAP 16
#define GAUGE_RENDER_SET_AI_X0 168
#define GAUGE_RENDER_SET_AI_Y0 144
#define GAUGE_RENDER_SET_AI_W 108
#define GAUGE_RENDER_SET_AI_H 24
#define GAUGE_RENDER_SET_AI_GAP 20
#define GAUGE_RENDER_SET_LIMIT_BTN_X0 168
#define GAUGE_RENDER_SET_LIMIT_BTN_Y0 176
#define GAUGE_RENDER_SET_LIMIT_BTN_W 236
#define GAUGE_RENDER_SET_LIMIT_BTN_H 28
#define GAUGE_RENDER_SET_CLEAR_BTN_X0 168
#define GAUGE_RENDER_SET_CLEAR_BTN_Y0 208
#define GAUGE_RENDER_SET_CLEAR_BTN_W 180
#define GAUGE_RENDER_SET_CLEAR_BTN_H 28
#define GAUGE_RENDER_SET_LOG_Y0 240
#define GAUGE_RENDER_SET_LOG_DEC_X0 168
#define GAUGE_RENDER_SET_LOG_DEC_W 52
#define GAUGE_RENDER_SET_LOG_VAL_X0 228
#define GAUGE_RENDER_SET_LOG_VAL_W 108
#define GAUGE_RENDER_SET_LOG_INC_X0 344
#define GAUGE_RENDER_SET_LOG_INC_W 52
#define GAUGE_RENDER_SET_LOG_H 24
#define GAUGE_RENDER_HELP_PANEL_X0 32
#define GAUGE_RENDER_HELP_PANEL_Y0 24
#define GAUGE_RENDER_HELP_PANEL_X1 448
#define GAUGE_RENDER_HELP_PANEL_Y1 304
#define GAUGE_RENDER_HELP_NEXT_X0 339
#define GAUGE_RENDER_HELP_NEXT_Y0 272
#define GAUGE_RENDER_HELP_NEXT_X1 438
#define GAUGE_RENDER_HELP_NEXT_Y1 296
#define GAUGE_RENDER_LIMIT_PANEL_X0 26
#define GAUGE_RENDER_LIMIT_PANEL_Y0 20
#define GAUGE_RENDER_LIMIT_PANEL_X1 454
#define GAUGE_RENDER_LIMIT_PANEL_Y1 306
#define GAUGE_RENDER_LIMIT_ROW_X0 44
#define GAUGE_RENDER_LIMIT_ROW_W 392
#define GAUGE_RENDER_LIMIT_ROW_H 40
#define GAUGE_RENDER_LIMIT_ROW_GAP 8
#define GAUGE_RENDER_LIMIT_ROW_Y0 62
#define GAUGE_RENDER_LIMIT_MINUS_X0 280
#define GAUGE_RENDER_LIMIT_MINUS_W 68
#define GAUGE_RENDER_LIMIT_PLUS_X0 360
#define GAUGE_RENDER_LIMIT_PLUS_W 68

bool GaugeRender_Init(void);
void GaugeRender_DrawFrame(const power_sample_t *sample, bool ai_enabled, power_replay_profile_t profile);
void GaugeRender_SetAccel(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, bool valid);
void GaugeRender_SetLinearAccel(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, bool valid);
void GaugeRender_SetGyro(int16_t gx_dps, int16_t gy_dps, int16_t gz_dps, bool valid);
void GaugeRender_SetMag(int16_t mx_mgauss, int16_t my_mgauss, int16_t mz_mgauss, bool valid);
void GaugeRender_SetBaro(int16_t pressure_dhpa, bool valid);
void GaugeRender_SetSht(int16_t temp_c10, int16_t rh_dpct, bool valid);
void GaugeRender_SetStts(int16_t temp_c10, bool valid);
void GaugeRender_SetBoardTempC(uint8_t temp_c, bool valid);
void GaugeRender_SetBoardTempC10(int16_t temp_c10, bool valid);
void GaugeRender_SetProfileInfo(const char *model_name, const char *model_version, const char *extension_version);
void GaugeRender_SetLogRateHz(uint8_t hz);
void GaugeRender_SetRuntimeClock(uint16_t hh, uint8_t mm, uint8_t ss, uint8_t ds, bool valid);
void GaugeRender_SetLimitInfo(uint16_t g_warn_mg,
                              uint16_t g_fail_mg,
                              int16_t temp_low_c10,
                              int16_t temp_high_c10,
                              uint16_t gyro_limit_dps);
void GaugeRender_SetAnomalyInfo(uint8_t mode,
                                uint8_t tune,
                                bool training_active,
                                bool trained_ready,
                                uint8_t level_ax,
                                uint8_t level_ay,
                                uint8_t level_az,
                                uint8_t level_temp,
                                uint8_t overall_level);
void GaugeRender_SetHelpVisible(bool visible);
void GaugeRender_SetHelpPage(uint8_t page);
void GaugeRender_NextHelpPage(void);
void GaugeRender_SetSettingsVisible(bool visible);
void GaugeRender_SetLimitsVisible(bool visible);
bool GaugeRender_IsLimitsVisible(void);
void GaugeRender_SetLiveBannerMode(bool enabled);
bool GaugeRender_IsLiveBannerMode(void);
void GaugeRender_SetAiBackendNpu(bool use_npu);
void GaugeRender_SetWarmupThinking(bool enabled);
void GaugeRender_PrimePredictionScore(void);
void GaugeRender_ResetPredictionMetrics(void);
void GaugeRender_IngestReplayCgmSample(uint32_t ts_ds, uint16_t glucose_mgdl, bool valid);
void GaugeRender_DrawGyroFast(void);
bool GaugeRender_HandleTouch(int32_t x, int32_t y, bool pressed);
uint8_t GaugeRender_GetTimelineHour(void);
bool GaugeRender_IsRecordMode(void);
void GaugeRender_SetPlayhead(uint8_t position_0_to_99, bool valid);
void GaugeRender_SetRecordMode(bool record_mode);
bool GaugeRender_IsRecordConfirmActive(void);
bool GaugeRender_ConsumeRecordStartRequest(void);
bool GaugeRender_ConsumeRecordStopRequest(void);
void GaugeRender_RequestClearFlashConfirm(void);
bool GaugeRender_ConsumeClearFlashRequest(void);
void GaugeRender_RequestModalRedraw(void);

#endif
