#ifndef EDGEAI_EXT_FLASH_RECORDER_H
#define EDGEAI_EXT_FLASH_RECORDER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t seq;
    uint32_t ts_ds;
    int16_t ax_mg;
    int16_t ay_mg;
    int16_t az_mg;
    int16_t gx_mdps;
    int16_t gy_mdps;
    int16_t gz_mdps;
    int16_t temp_c10;
    int16_t mag_x_mgauss;
    int16_t mag_y_mgauss;
    int16_t mag_z_mgauss;
    int16_t baro_dhpa;
    int16_t sht_temp_c10;
    int16_t sht_rh_dpct;
    int16_t stts_temp_c10;
    uint16_t anomaly_score_pct;
    uint8_t alert_status;
    uint8_t alert_reason_code;
    uint8_t temp_c;
} ext_flash_sample_t;

bool ExtFlashRecorder_Init(void);
bool ExtFlashRecorder_AppendSample(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint8_t temp_c);
bool ExtFlashRecorder_AppendSampleEx(int16_t ax_mg,
                                     int16_t ay_mg,
                                     int16_t az_mg,
                                     int16_t gx_mdps,
                                     int16_t gy_mdps,
                                     int16_t gz_mdps,
                                     int16_t temp_c10,
                                     int16_t mag_x_mgauss,
                                     int16_t mag_y_mgauss,
                                     int16_t mag_z_mgauss,
                                     int16_t baro_dhpa,
                                     int16_t sht_temp_c10,
                                     int16_t sht_rh_dpct,
                                     int16_t stts_temp_c10,
                                     uint16_t anomaly_score_pct,
                                     uint8_t alert_status,
                                     uint8_t alert_reason_code,
                                     uint32_t ts_ds);
bool ExtFlashRecorder_IsReady(void);
bool ExtFlashRecorder_ClearAll(void);
bool ExtFlashRecorder_StartPlayback(void);
bool ExtFlashRecorder_ReadNextSample(ext_flash_sample_t *sample);
bool ExtFlashRecorder_GetPlaybackInfo(uint32_t *offset, uint32_t *count);
bool ExtFlashRecorder_GetRecordInfo(uint32_t *count);
bool ExtFlashRecorder_GetUsageInfo(uint32_t *used_count, uint32_t *capacity_count);
bool ExtFlashRecorder_SaveUiSettings(uint8_t mode,
                                     uint8_t tune,
                                     bool run_live,
                                     bool ai_enabled,
                                     bool ai_backend_npu,
                                     uint16_t g_warn_mg,
                                     uint16_t g_fail_mg,
                                     int16_t temp_low_c10,
                                     int16_t temp_high_c10,
                                     uint16_t gyro_limit_dps,
                                     uint8_t log_rate_hz);
bool ExtFlashRecorder_GetUiSettings(uint8_t *mode,
                                    uint8_t *tune,
                                    bool *run_live,
                                    bool *ai_enabled,
                                    bool *ai_backend_npu,
                                    uint16_t *g_warn_mg,
                                    uint16_t *g_fail_mg,
                                    int16_t *temp_low_c10,
                                    int16_t *temp_high_c10,
                                    uint16_t *gyro_limit_dps,
                                    uint8_t *log_rate_hz,
                                    bool *valid);

#endif
