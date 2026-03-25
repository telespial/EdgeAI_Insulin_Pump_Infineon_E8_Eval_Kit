#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app.h"
#include "board.h"
#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_gt911.h"
#include "fsl_i3c.h"
#include "fsl_lpi2c.h"
#include "fsl_ostimer.h"
#include "fsl_port.h"
#include "gauge_render.h"
#include "power_data_source.h"
#include "replay_trace_generated.h"
#include "cgm_replay_subject001.h"
#include "anomaly_engine.h"
#include "eil_profile.h"
#include "accel4_click.h"
#include "fxls8974cf.h"
#include "ext_flash_recorder.h"

#define TOUCH_I2C LPI2C2
#define TOUCH_I2C_FLEXCOMM_INDEX 2u
#define TOUCH_POINTS 5u
#define TOUCH_INT_PORT PORT4
#define TOUCH_INT_PIN 6u
#define ACCEL_I2C LPI2C3
#define ACCEL_I2C_FLEXCOMM_INDEX 3u
#define TEMP_I3C I3C1
#define TEMP_I3C_INDEX 1u
#define BOARD_TEMP_REG 0x00u
#define TEMP_SENSOR_STATIC_ADDR 0x48u
#define TEMP_SENSOR_DYNAMIC_ADDR 0x08u
#define I3C_BROADCAST_ADDR 0x7Eu
#define I3C_CCC_RSTDAA 0x06u
#define I3C_CCC_SETDASA 0x87u
#define SHIELD_GYRO_ADDR0 0x6Au
#define SHIELD_GYRO_ADDR1 0x6Bu
#define SHIELD_GYRO_REG_WHO_AM_I 0x0Fu
#define SHIELD_GYRO_REG_CTRL1_XL 0x10u
#define SHIELD_GYRO_REG_CTRL2_G 0x11u
#define SHIELD_GYRO_REG_CTRL3_C 0x12u
#define SHIELD_GYRO_REG_OUTX_L_G 0x22u
#define SHIELD_GYRO_REG_OUTX_L_A 0x28u
#define SHIELD_GYRO_REG_FUNC_CFG_ACCESS 0x01u
#define SHIELD_IMU_WHOAMI_LSM6DSO16IS 0x22u
#define SHIELD_IMU_WHOAMI_LSM6DSV16X 0x70u
#define SHIELD_SHUB_FUNC_CFG_ACCESS 0x40u
#define SHIELD_SHUB_REG_SENSOR_HUB_1 0x02u
#define SHIELD_SHUB_REG_MASTER_CONFIG 0x14u
#define SHIELD_SHUB_REG_SLV0_ADD 0x15u
#define SHIELD_SHUB_REG_SLV0_SUBADD 0x16u
#define SHIELD_SHUB_REG_SLV0_CONFIG 0x17u
#define SHIELD_SHUB_REG_DATAWRITE_SLV0 0x21u
#define SHIELD_SHUB_REG_STATUS_MASTER 0x22u
#define SHIELD_SHUB_STATUS_ENDOP 0x01u
#define SHIELD_SHUB_STATUS_NACK_MASK 0x78u
#define SHIELD_SHUB_MASTER_SHUB_PU_EN 0x08u
#define SHIELD_SHUB_MASTER_ON 0x04u
#define SHIELD_SHUB_MASTER_WRITE_ONCE 0x40u
#define SHIELD_SHUB_MASTER_RST_REGS 0x80u
#define SHIELD_LIS2DUXS12_ADDR0 0x18u
#define SHIELD_LIS2DUXS12_ADDR1 0x19u
#define SHIELD_LIS2DUXS12_WHOAMI 0x47u
#define SHIELD_LIS2MDL_ADDR 0x1Eu
#define SHIELD_LIS2MDL_ADDR_ALT 0x1Cu
#define SHIELD_LIS2MDL_REG_WHO_AM_I 0x4Fu
#define SHIELD_LIS2MDL_WHOAMI 0x40u
#define SHIELD_LIS2MDL_REG_CFG_A 0x60u
#define SHIELD_LIS2MDL_REG_CFG_C 0x62u
#define SHIELD_LIS2MDL_REG_OUTX_L 0x68u
#define SHIELD_LPS22DF_ADDR0 0x5Cu
#define SHIELD_LPS22DF_ADDR1 0x5Du
#define SHIELD_LPS22DF_WHOAMI 0xB4u
#define SHIELD_LPS22DF_REG_CTRL1 0x10u
#define SHIELD_LPS22DF_REG_OUT_P_XL 0x28u
#define SHIELD_LPS22DF_PRESS_LSB_PER_HPA 4096
#define SHIELD_SHT40_ADDR0 0x44u
#define SHIELD_SHT40_ADDR1 0x45u
#define SHIELD_SHT40_CMD_MEASURE_LP 0xE0u
#define SHIELD_STTS22H_WHOAMI 0xA0u
#define SHIELD_STTS22H_REG_WHO_AM_I 0x01u
#define SHIELD_STTS22H_REG_CTRL 0x04u
#define SHIELD_STTS22H_REG_TEMP_L 0x06u
#define SHIELD_AUX_POLL_PERIOD_US 150000u

#define TOUCH_POLL_DELAY_US 2000u
#define POWER_TICK_PERIOD_US 1000000u
#define DISPLAY_REFRESH_PERIOD_US 100000u
#define RECPLAY_TICK_PERIOD_US 100000u
#define RECPLAY_WARMUP_MULTIPLIER 1200u
#define RECPLAY_WARMUP_PRESEED_DS (2u * 60u * 60u * 10u)
#define RECPLAY_WARMUP_SCORE_DS (2u * 60u * 60u * 10u)
#define RECPLAY_WARMUP_TARGET_DS (RECPLAY_WARMUP_PRESEED_DS + RECPLAY_WARMUP_SCORE_DS)
/* Guard against replay timestamp discontinuities after preroll handoff. */
#define RECPLAY_POST_WARMUP_MAX_DELTA_DS (10u * 60u * 10u)
/* Global runtime anti-jump guard: never advance elapsed clock by a huge single step. */
#define RUNTIME_MAX_STEP_DS 50u
#define RECPLAY_FULLRUN_MULTIPLIER 120u
#define REPLAY_DEFAULT_STEP_DS 3000u /* 5 minutes between CGM points */
#define ACCEL_BUFFER_SAMPLE_PERIOD_US 10000u
#define GYRO_REFRESH_PERIOD_US 100000u
#define ACCEL_LIVE_PERIOD_US 100000u
#define RUNTIME_CLOCK_PERIOD_US 100000u
#define TEMP_REFRESH_PERIOD_US 100000u
#define ACCEL_TEST_LOG_PERIOD_US 1000000u
#define EDGEAI_TIMEBASE_CRYSTAL_HZ 32768u
#define EDGEAI_TIMEBASE_CAL_WINDOW_US 250000u
#ifndef EDGEAI_ENABLE_ACCEL_TEST_LOG
#define EDGEAI_ENABLE_ACCEL_TEST_LOG 0
#endif
#ifndef EDGEAI_ENABLE_SHIELD_SCAN_DIAG
#define EDGEAI_ENABLE_SHIELD_SCAN_DIAG 0
#endif
#ifndef EDGEAI_ENABLE_SHIELD_SENSOR_SCAN_LOG
#define EDGEAI_ENABLE_SHIELD_SENSOR_SCAN_LOG 0
#endif
#ifndef EDGEAI_I2C_RETRY_COUNT
#define EDGEAI_I2C_RETRY_COUNT 3u
#endif
#ifndef EDGEAI_SENSOR_SCAN_MODE
#define EDGEAI_SENSOR_SCAN_MODE 0
#endif

static gt911_handle_t s_touch_handle;
static bool s_touch_ready = false;
static bool s_touch_was_down = false;
static uint32_t s_touch_recover_backoff = 0u;
static bool s_timebase_ready = false;
static uint32_t s_timebase_hz = EDGEAI_TIMEBASE_CRYSTAL_HZ;
static bool s_timebase_use_raw = false;
static bool s_timebase_use_core_cycle = false;
static uint32_t s_core_cycle_prev = 0u;
static uint64_t s_core_cycle_accum = 0u;
static bool s_touch_i2c_inited = false;
static bool s_accel_i2c_inited = false;
static bool s_accel_ready = false;
static fxls8974_dev_t s_accel_dev;
static int16_t s_accel_raw_x_mg = 0;
static int16_t s_accel_raw_y_mg = 0;
static int16_t s_accel_raw_z_mg = 1000;
static int16_t s_accel_x_mg = 0;
static int16_t s_accel_y_mg = 0;
static int16_t s_accel_z_mg = 1000;
static bool s_shield_gyro_ready = false;
static uint8_t s_shield_gyro_addr = 0u;
static uint8_t s_shield_gyro_who = 0u;
static bool s_shield_use_touch_bus = true;
static bool s_shield_gyro_missing_logged = false;
static bool s_shield_gyro_read_fail_logged = false;
static uint8_t s_shield_gyro_read_fail_streak = 0u;
static int16_t s_ui_gyro_x = 0;
static int16_t s_ui_gyro_y = 0;
static int16_t s_ui_gyro_z = 0;
static int16_t s_live_gyro_x_dps = 0;
static int16_t s_live_gyro_y_dps = 0;
static int16_t s_live_gyro_z_dps = 0;
static bool s_shield_mag_ready = false;
static bool s_shield_baro_ready = false;
static bool s_shield_sht_ready = false;
static bool s_shield_stts_ready = false;
static bool s_shield_aux_init_done = false;
static bool s_shield_mag_use_shub = false;
static bool s_shield_baro_use_shub = false;
static bool s_shield_stts_use_shub = false;
static bool s_shield_mag_use_touch_bus = true;
static uint8_t s_shield_mag_addr = SHIELD_LIS2MDL_ADDR;
static bool s_shield_baro_use_touch_bus = true;
static bool s_shield_sht_use_touch_bus = true;
static bool s_shield_stts_use_touch_bus = true;
static uint8_t s_shield_baro_addr = 0u;
static uint8_t s_shield_sht_addr = 0u;
static uint8_t s_shield_stts_addr = 0u;
static int16_t s_mag_x_mgauss = 0;
static int16_t s_mag_y_mgauss = 0;
static int16_t s_mag_z_mgauss = 0;
static uint8_t s_shield_mag_reprobe_ticks = 0u;
static int16_t s_baro_dhpa = 10132;
static int16_t s_sht_temp_c10 = 250;
static int16_t s_sht_rh_dpct = 500;
static int16_t s_stts_temp_c10 = 250;
static bool s_temp_ready = false;
static bool s_temp_i3c_inited = false;
static uint8_t s_temp_addr = 0u;
static uint8_t s_temp_c = 25u;
static int16_t s_temp_c10 = 250;
static uint16_t s_limit_g_warn_mg = 12000u;
static uint16_t s_limit_g_fail_mg = 15000u;
static int16_t s_limit_temp_lo_c10 = 0;
static int16_t s_limit_temp_hi_c10 = 700;
static uint16_t s_limit_gyro_dps = 500u;
static uint8_t s_log_rate_hz = 10u;

static const uint8_t k_log_rate_options[] = {1u, 5u, 10u, 20u, 30u, 40u, 50u};
static uint16_t s_gyro_peak_dps = 0u;
static i3c_bus_type_t s_temp_bus_type = kI3C_TypeI2C;
static power_sample_t s_frame_sample;
static anomaly_output_t s_anom_out;
static bool BoardTempI3CInit(void);
static bool TouchI2CInit(void);
static bool AccelI2CInit(void);
static void ShieldGyroInit(void);
static uint32_t CoreClockHz(void);
static bool ShieldImuSupportsShub(uint8_t who);
static uint64_t TimebaseNowTicks(void);

typedef struct
{
    uint8_t addr;
    uint8_t reg;
    const char *name;
} diag_probe_t;

static const diag_probe_t kDiagProbes[] = {
    {0x6Au, 0x0Fu, "IMU_A"},
    {0x6Bu, 0x0Fu, "IMU_B"},
    {0x18u, 0x0Fu, "LIS2DUXS12_A"},
    {0x19u, 0x0Fu, "LIS2DUXS12_B"},
    {0x1Eu, 0x4Fu, "LIS2MDL"},
    {0x5Cu, 0x0Fu, "LPS22DF_A"},
    {0x5Du, 0x0Fu, "LPS22DF_B"},
    {0x44u, 0x89u, "SHT4x_CMD"},
    {0x45u, 0x89u, "SHT4x_CMD_ALT"},
    {0x5Du, 0x0Fu, "STTS22H?"},
    {0x3Cu, 0x4Fu, "LIS2MDL_ALT8"},
};

static bool s_accel_log_peak_valid = false;
static int16_t s_accel_log_peak_x_mg = 0;
static int16_t s_accel_log_peak_y_mg = 0;
static int16_t s_accel_log_peak_z_mg = 0;
static bool s_accel_capture_peak_valid = false;
static int16_t s_accel_capture_peak_x_mg = 0;
static int16_t s_accel_capture_peak_y_mg = 0;
static int16_t s_accel_capture_peak_z_mg = 0;
static bool s_gyro_log_peak_valid = false;
static int16_t s_gyro_log_peak_x_mg = 0;
static int16_t s_gyro_log_peak_y_mg = 0;
static int16_t s_gyro_log_peak_z_mg = 0;
static bool s_gyro_capture_peak_valid = false;
static int16_t s_gyro_capture_peak_x_mg = 0;
static int16_t s_gyro_capture_peak_y_mg = 0;
static int16_t s_gyro_capture_peak_z_mg = 0;
static bool s_mag_log_peak_valid = false;
static int16_t s_mag_log_peak_x_mgauss = 0;
static int16_t s_mag_log_peak_y_mgauss = 0;
static int16_t s_mag_log_peak_z_mgauss = 0;
static bool s_mag_capture_peak_valid = false;
static int16_t s_mag_capture_peak_x_mgauss = 0;
static int16_t s_mag_capture_peak_y_mgauss = 0;
static int16_t s_mag_capture_peak_z_mgauss = 0;
static bool s_alert_capture_valid = false;
static uint8_t s_alert_capture_status = AI_STATUS_NORMAL;
static uint8_t s_alert_capture_reason = ALERT_REASON_NORMAL;
static uint16_t s_alert_capture_score = 0u;

static int16_t SelectSignedPeakAbs(int16_t current_peak, int16_t sample)
{
    int32_t p = (current_peak < 0) ? -current_peak : current_peak;
    int32_t s = (sample < 0) ? -sample : sample;
    return (s > p) ? sample : current_peak;
}

static uint8_t AlertSeverity(uint8_t status)
{
    if (status == AI_STATUS_FAULT)
    {
        return 2u;
    }
    if (status == AI_STATUS_WARNING)
    {
        return 1u;
    }
    return 0u;
}

static void UpdateAlertCaptureWindow(const power_sample_t *sample)
{
    uint8_t live_severity;
    uint8_t hold_severity;
    if (sample == NULL)
    {
        return;
    }
    if (!s_alert_capture_valid)
    {
        s_alert_capture_status = sample->ai_status;
        s_alert_capture_reason = sample->alert_reason_code;
        s_alert_capture_score = sample->anomaly_score_pct;
        s_alert_capture_valid = true;
        return;
    }

    live_severity = AlertSeverity(sample->ai_status);
    hold_severity = AlertSeverity(s_alert_capture_status);
    if ((live_severity > hold_severity) ||
        ((live_severity == hold_severity) && (sample->anomaly_score_pct >= s_alert_capture_score)))
    {
        s_alert_capture_status = sample->ai_status;
        s_alert_capture_reason = sample->alert_reason_code;
        s_alert_capture_score = sample->anomaly_score_pct;
    }
}

static void ConsumeAlertCaptureWindow(uint16_t fallback_score,
                                      uint8_t fallback_status,
                                      uint8_t fallback_reason,
                                      uint16_t *score_out,
                                      uint8_t *status_out,
                                      uint8_t *reason_out)
{
    if ((score_out == NULL) || (status_out == NULL) || (reason_out == NULL))
    {
        return;
    }
    if (s_alert_capture_valid)
    {
        *score_out = s_alert_capture_score;
        *status_out = s_alert_capture_status;
        *reason_out = s_alert_capture_reason;
    }
    else
    {
        *score_out = fallback_score;
        *status_out = fallback_status;
        *reason_out = fallback_reason;
    }
    s_alert_capture_valid = false;
    s_alert_capture_status = AI_STATUS_NORMAL;
    s_alert_capture_reason = ALERT_REASON_NORMAL;
    s_alert_capture_score = 0u;
}

static uint16_t PeakAbs3I16(int16_t x, int16_t y, int16_t z)
{
    int32_t ax = (x < 0) ? -x : x;
    int32_t ay = (y < 0) ? -y : y;
    int32_t az = (z < 0) ? -z : z;
    int32_t p = ax;
    if (ay > p)
    {
        p = ay;
    }
    if (az > p)
    {
        p = az;
    }
    if (p < 0)
    {
        p = 0;
    }
    if (p > 32767)
    {
        p = 32767;
    }
    return (uint16_t)p;
}

static uint16_t GetAccelAlertPeakMg(void)
{
    uint16_t peak_mg = PeakAbs3I16(s_accel_raw_x_mg, s_accel_raw_y_mg, s_accel_raw_z_mg);
    if (s_accel_capture_peak_valid)
    {
        uint16_t capture_peak =
            PeakAbs3I16(s_accel_capture_peak_x_mg, s_accel_capture_peak_y_mg, s_accel_capture_peak_z_mg);
        if (capture_peak > peak_mg)
        {
            peak_mg = capture_peak;
        }
    }
    if (s_accel_log_peak_valid)
    {
        uint16_t log_peak = PeakAbs3I16(s_accel_log_peak_x_mg, s_accel_log_peak_y_mg, s_accel_log_peak_z_mg);
        if (log_peak > peak_mg)
        {
            peak_mg = log_peak;
        }
    }
    return peak_mg;
}

static void ResetSignalPeakWindows(void)
{
    s_accel_log_peak_valid = false;
    s_accel_capture_peak_valid = false;
    s_gyro_log_peak_valid = false;
    s_gyro_capture_peak_valid = false;
    s_mag_log_peak_valid = false;
    s_mag_capture_peak_valid = false;
    s_alert_capture_valid = false;
    s_alert_capture_status = AI_STATUS_NORMAL;
    s_alert_capture_reason = ALERT_REASON_NORMAL;
    s_alert_capture_score = 0u;
}

static void UpdateSignalPeakWindows(void)
{
    if (!s_accel_log_peak_valid)
    {
        s_accel_log_peak_x_mg = s_accel_raw_x_mg;
        s_accel_log_peak_y_mg = s_accel_raw_y_mg;
        s_accel_log_peak_z_mg = s_accel_raw_z_mg;
        s_accel_log_peak_valid = true;
    }
    else
    {
        s_accel_log_peak_x_mg = SelectSignedPeakAbs(s_accel_log_peak_x_mg, s_accel_raw_x_mg);
        s_accel_log_peak_y_mg = SelectSignedPeakAbs(s_accel_log_peak_y_mg, s_accel_raw_y_mg);
        s_accel_log_peak_z_mg = SelectSignedPeakAbs(s_accel_log_peak_z_mg, s_accel_raw_z_mg);
    }

    if (!s_accel_capture_peak_valid)
    {
        s_accel_capture_peak_x_mg = s_accel_raw_x_mg;
        s_accel_capture_peak_y_mg = s_accel_raw_y_mg;
        s_accel_capture_peak_z_mg = s_accel_raw_z_mg;
        s_accel_capture_peak_valid = true;
    }
    else
    {
        s_accel_capture_peak_x_mg = SelectSignedPeakAbs(s_accel_capture_peak_x_mg, s_accel_raw_x_mg);
        s_accel_capture_peak_y_mg = SelectSignedPeakAbs(s_accel_capture_peak_y_mg, s_accel_raw_y_mg);
        s_accel_capture_peak_z_mg = SelectSignedPeakAbs(s_accel_capture_peak_z_mg, s_accel_raw_z_mg);
    }

    if (!s_gyro_log_peak_valid)
    {
        s_gyro_log_peak_x_mg = s_live_gyro_x_dps;
        s_gyro_log_peak_y_mg = s_live_gyro_y_dps;
        s_gyro_log_peak_z_mg = s_live_gyro_z_dps;
        s_gyro_log_peak_valid = true;
    }
    else
    {
        s_gyro_log_peak_x_mg = SelectSignedPeakAbs(s_gyro_log_peak_x_mg, s_live_gyro_x_dps);
        s_gyro_log_peak_y_mg = SelectSignedPeakAbs(s_gyro_log_peak_y_mg, s_live_gyro_y_dps);
        s_gyro_log_peak_z_mg = SelectSignedPeakAbs(s_gyro_log_peak_z_mg, s_live_gyro_z_dps);
    }

    if (!s_gyro_capture_peak_valid)
    {
        s_gyro_capture_peak_x_mg = s_live_gyro_x_dps;
        s_gyro_capture_peak_y_mg = s_live_gyro_y_dps;
        s_gyro_capture_peak_z_mg = s_live_gyro_z_dps;
        s_gyro_capture_peak_valid = true;
    }
    else
    {
        s_gyro_capture_peak_x_mg = SelectSignedPeakAbs(s_gyro_capture_peak_x_mg, s_live_gyro_x_dps);
        s_gyro_capture_peak_y_mg = SelectSignedPeakAbs(s_gyro_capture_peak_y_mg, s_live_gyro_y_dps);
        s_gyro_capture_peak_z_mg = SelectSignedPeakAbs(s_gyro_capture_peak_z_mg, s_live_gyro_z_dps);
    }

    if (!s_mag_log_peak_valid)
    {
        s_mag_log_peak_x_mgauss = s_mag_x_mgauss;
        s_mag_log_peak_y_mgauss = s_mag_y_mgauss;
        s_mag_log_peak_z_mgauss = s_mag_z_mgauss;
        s_mag_log_peak_valid = true;
    }
    else
    {
        s_mag_log_peak_x_mgauss = SelectSignedPeakAbs(s_mag_log_peak_x_mgauss, s_mag_x_mgauss);
        s_mag_log_peak_y_mgauss = SelectSignedPeakAbs(s_mag_log_peak_y_mgauss, s_mag_y_mgauss);
        s_mag_log_peak_z_mgauss = SelectSignedPeakAbs(s_mag_log_peak_z_mgauss, s_mag_z_mgauss);
    }

    if (!s_mag_capture_peak_valid)
    {
        s_mag_capture_peak_x_mgauss = s_mag_x_mgauss;
        s_mag_capture_peak_y_mgauss = s_mag_y_mgauss;
        s_mag_capture_peak_z_mgauss = s_mag_z_mgauss;
        s_mag_capture_peak_valid = true;
    }
    else
    {
        s_mag_capture_peak_x_mgauss = SelectSignedPeakAbs(s_mag_capture_peak_x_mgauss, s_mag_x_mgauss);
        s_mag_capture_peak_y_mgauss = SelectSignedPeakAbs(s_mag_capture_peak_y_mgauss, s_mag_y_mgauss);
        s_mag_capture_peak_z_mgauss = SelectSignedPeakAbs(s_mag_capture_peak_z_mgauss, s_mag_z_mgauss);
    }
}

static void ConsumeLogPeaks(int16_t *ax_mg,
                            int16_t *ay_mg,
                            int16_t *az_mg,
                            int16_t *gx,
                            int16_t *gy,
                            int16_t *gz,
                            int16_t *mx,
                            int16_t *my,
                            int16_t *mz)
{
    if ((ax_mg == NULL) || (ay_mg == NULL) || (az_mg == NULL) || (gx == NULL) || (gy == NULL) || (gz == NULL) ||
        (mx == NULL) || (my == NULL) || (mz == NULL))
    {
        return;
    }
    if (s_accel_log_peak_valid)
    {
        *ax_mg = s_accel_log_peak_x_mg;
        *ay_mg = s_accel_log_peak_y_mg;
        *az_mg = s_accel_log_peak_z_mg;
    }
    else
    {
        *ax_mg = s_accel_x_mg;
        *ay_mg = s_accel_y_mg;
        *az_mg = s_accel_z_mg;
    }

    if (s_gyro_log_peak_valid)
    {
        *gx = s_gyro_log_peak_x_mg;
        *gy = s_gyro_log_peak_y_mg;
        *gz = s_gyro_log_peak_z_mg;
    }
    else
    {
        *gx = s_live_gyro_x_dps;
        *gy = s_live_gyro_y_dps;
        *gz = s_live_gyro_z_dps;
    }

    if (s_mag_log_peak_valid)
    {
        *mx = s_mag_log_peak_x_mgauss;
        *my = s_mag_log_peak_y_mgauss;
        *mz = s_mag_log_peak_z_mgauss;
    }
    else
    {
        *mx = s_mag_x_mgauss;
        *my = s_mag_y_mgauss;
        *mz = s_mag_z_mgauss;
    }
    s_accel_log_peak_valid = false;
    s_gyro_log_peak_valid = false;
    s_mag_log_peak_valid = false;
}

static void ConsumeCapturePeaks(int16_t *ax_mg,
                                int16_t *ay_mg,
                                int16_t *az_mg,
                                int16_t *gx,
                                int16_t *gy,
                                int16_t *gz,
                                int16_t *mx,
                                int16_t *my,
                                int16_t *mz)
{
    if ((ax_mg == NULL) || (ay_mg == NULL) || (az_mg == NULL) || (gx == NULL) || (gy == NULL) || (gz == NULL) ||
        (mx == NULL) || (my == NULL) || (mz == NULL))
    {
        return;
    }
    if (s_accel_capture_peak_valid)
    {
        *ax_mg = s_accel_capture_peak_x_mg;
        *ay_mg = s_accel_capture_peak_y_mg;
        *az_mg = s_accel_capture_peak_z_mg;
    }
    else
    {
        *ax_mg = s_accel_x_mg;
        *ay_mg = s_accel_y_mg;
        *az_mg = s_accel_z_mg;
    }

    if (s_gyro_capture_peak_valid)
    {
        *gx = s_gyro_capture_peak_x_mg;
        *gy = s_gyro_capture_peak_y_mg;
        *gz = s_gyro_capture_peak_z_mg;
    }
    else
    {
        *gx = s_live_gyro_x_dps;
        *gy = s_live_gyro_y_dps;
        *gz = s_live_gyro_z_dps;
    }

    if (s_mag_capture_peak_valid)
    {
        *mx = s_mag_capture_peak_x_mgauss;
        *my = s_mag_capture_peak_y_mgauss;
        *mz = s_mag_capture_peak_z_mgauss;
    }
    else
    {
        *mx = s_mag_x_mgauss;
        *my = s_mag_y_mgauss;
        *mz = s_mag_z_mgauss;
    }
    s_accel_capture_peak_valid = false;
    s_gyro_capture_peak_valid = false;
    s_mag_capture_peak_valid = false;
}

static void ClockFromDeciseconds(uint32_t ds_total, uint16_t *hh, uint8_t *mm, uint8_t *ss, uint8_t *ds)
{
    uint32_t sec_total = ds_total / 10u;
    uint32_t rem_ds = ds_total % 10u;
    uint32_t hh_v = sec_total / 3600u;
    uint32_t mm_v = (sec_total % 3600u) / 60u;
    uint32_t ss_v = sec_total % 60u;

    if (hh != NULL)
    {
        *hh = (uint16_t)((hh_v > 9999u) ? 9999u : hh_v);
    }
    if (mm != NULL)
    {
        *mm = (uint8_t)mm_v;
    }
    if (ss != NULL)
    {
        *ss = (uint8_t)ss_v;
    }
    if (ds != NULL)
    {
        *ds = (uint8_t)rem_ds;
    }
}

static void SetRuntimeClockFromDs(uint32_t ds_total)
{
    uint16_t hh;
    uint8_t mm;
    uint8_t ss;
    uint8_t ds;
    ClockFromDeciseconds(ds_total, &hh, &mm, &ss, &ds);
    GaugeRender_SetRuntimeClock(hh, mm, ss, ds, true);
}

static uint8_t ChannelLevelPct(anomaly_level_t lvl)
{
    if (lvl >= ANOMALY_LEVEL_MAJOR)
    {
        return 85u;
    }
    if (lvl >= ANOMALY_LEVEL_MINOR)
    {
        return 45u;
    }
    if (lvl >= ANOMALY_LEVEL_WATCH)
    {
        return 12u;
    }
    return 5u;
}

static uint8_t AlertReasonWarningPriority(uint8_t reason)
{
    switch (reason)
    {
        case ALERT_REASON_ACCEL_WARN:
        case ALERT_REASON_TEMP_WARN:
        case ALERT_REASON_GYRO_WARN:
            return 90u;
        case ALERT_REASON_ERRATIC_MOTION:
            return 80u;
        case ALERT_REASON_INVERTED_WARN:
            return 75u;
        case ALERT_REASON_TILT_WARN:
            return 70u;
        case ALERT_REASON_TEMP_APPROACH_LOW:
        case ALERT_REASON_TEMP_APPROACH_HIGH:
            return 60u;
        case ALERT_REASON_SCORE_WARN:
            return 50u;
        case ALERT_REASON_ANOMALY_WATCH:
            return 40u;
        default:
            return 0u;
    }
}

static void ApplyAnomalyToFrame(power_sample_t *dst)
{
    static bool s_motion_prev_valid = false;
    static int16_t s_prev_ax_mg = 0;
    static int16_t s_prev_ay_mg = 0;
    static int16_t s_prev_az_mg = 0;
    static uint8_t s_warn_candidate_reason = ALERT_REASON_NORMAL;
    static uint64_t s_warn_candidate_since_ticks = 0ull;
    static uint8_t s_fault_candidate_reason = ALERT_REASON_NORMAL;
    static uint64_t s_fault_candidate_since_ticks = 0ull;
    static uint8_t s_alert_hold_status = AI_STATUS_NORMAL;
    static uint8_t s_alert_hold_reason = ALERT_REASON_NORMAL;
    static uint64_t s_alert_hold_until_ticks = 0ull;
    static uint64_t s_adapt_freeze_until_ticks = 0ull;
    static uint64_t s_impact_latch_until_ticks = 0ull;
    const eil_profile_t *profile = EilProfile_Get();
    const uint64_t hold_warn_ticks = (uint64_t)s_timebase_hz * 5ull;
    const uint64_t hold_fault_ticks = (uint64_t)s_timebase_hz * 8ull;
    const uint64_t fault_persist_ticks = (uint64_t)s_timebase_hz / 6ull;      /* ~167 ms */
    const uint64_t warn_limit_persist_ticks = (uint64_t)s_timebase_hz / 4ull; /* ~250 ms */
    const uint64_t warn_predict_persist_ticks = (uint64_t)s_timebase_hz * 7ull / 10ull; /* ~700 ms */
    const uint64_t warn_score_persist_ticks = (uint64_t)s_timebase_hz; /* 1.0 s */
    const uint64_t warn_impact_persist_ticks = 0ull; /* immediate impact warning once detected */
    const uint64_t impact_latch_ticks = (uint64_t)s_timebase_hz * 8ull / 10ull; /* 800 ms */
    const uint64_t adapt_freeze_recover_ticks = (uint64_t)s_timebase_hz * 8ull; /* 8 s stable normal */
    uint8_t ax = ChannelLevelPct(s_anom_out.channel_level[ANOMALY_CH_AX]);
    uint8_t ay = ChannelLevelPct(s_anom_out.channel_level[ANOMALY_CH_AY]);
    uint8_t az = ChannelLevelPct(s_anom_out.channel_level[ANOMALY_CH_AZ]);
    uint8_t tp = ChannelLevelPct(s_anom_out.channel_level[ANOMALY_CH_TEMP]);
    float w_ax = 1.0f;
    float w_ay = 1.0f;
    float w_az = 1.0f;
    float w_tp = 1.0f;
    float weighted_pct;
    float weight_sum;
    uint8_t maxch = ax;
    int32_t abx;
    int32_t aby;
    int32_t abz;
    int32_t dax = 0;
    int32_t day = 0;
    int32_t daz = 0;
    int32_t jerk_peak;
    uint16_t accel_peak_mg;
    bool accel_warn_hit;
    bool accel_fail_hit;
    bool temp_limit_hit;
    bool temp_fail_hit;
    bool gyro_limit_hit;
    bool predicted_inverted_warn;
    bool predicted_tilt_warn;
    bool predicted_temp_approach_low;
    bool predicted_temp_approach_high;
    bool predicted_erratic_motion;
    bool watch_motion_evidence;
    uint16_t gyro_predict_warn_dps;
    uint8_t candidate_status = AI_STATUS_NORMAL;
    uint8_t candidate_reason = ALERT_REASON_NORMAL;
    float score_ratio;
    float score_warn_enter = 0.18f;
    float score_warn_exit = 0.15f;
    float score_fail_enter = 0.35f;
    float score_fail_exit = 0.28f;
    uint64_t warning_persist_ticks = warn_limit_persist_ticks;
    bool warning_matured = false;
    bool fault_matured = false;
    bool freeze_adapt = false;
    uint64_t now_ticks = TimebaseNowTicks();

    if (dst == NULL)
    {
        return;
    }

    if (ay > maxch)
    {
        maxch = ay;
    }
    if (az > maxch)
    {
        maxch = az;
    }
    if (tp > maxch)
    {
        maxch = tp;
    }

    if (profile != NULL)
    {
        w_ax = profile->weight_ax;
        w_ay = profile->weight_ay;
        w_az = profile->weight_az;
        w_tp = profile->weight_temp_c;
    }
    if (w_ax < 0.0f)
    {
        w_ax = 0.0f;
    }
    if (w_ay < 0.0f)
    {
        w_ay = 0.0f;
    }
    if (w_az < 0.0f)
    {
        w_az = 0.0f;
    }
    if (w_tp < 0.0f)
    {
        w_tp = 0.0f;
    }

    weight_sum = w_ax + w_ay + w_az + w_tp;
    if (weight_sum > 0.0f)
    {
        weighted_pct = ((float)ax * w_ax + (float)ay * w_ay + (float)az * w_az + (float)tp * w_tp) / weight_sum;
    }
    else
    {
        weighted_pct = (float)maxch;
    }
    if (weighted_pct < 0.0f)
    {
        weighted_pct = 0.0f;
    }
    if (weighted_pct > 100.0f)
    {
        weighted_pct = 100.0f;
    }

    dst->anomaly_score_pct = (uint16_t)(weighted_pct + 0.5f);
    dst->connector_risk_pct = ax;
    dst->wire_risk_pct = ay;
    dst->thermal_damage_risk_pct = tp;
    dst->degradation_drift_pct = az;
    dst->thermal_risk_s = 0u;
    dst->alert_reason_code = ALERT_REASON_NORMAL;

    abx = (s_accel_x_mg < 0) ? -s_accel_x_mg : s_accel_x_mg;
    aby = (s_accel_y_mg < 0) ? -s_accel_y_mg : s_accel_y_mg;
    abz = (s_accel_z_mg < 0) ? -s_accel_z_mg : s_accel_z_mg;
    accel_peak_mg = GetAccelAlertPeakMg();

    accel_warn_hit = (accel_peak_mg >= s_limit_g_warn_mg);
    accel_fail_hit = (accel_peak_mg >= s_limit_g_fail_mg);
    temp_limit_hit = (s_temp_c10 < s_limit_temp_lo_c10) || (s_temp_c10 > s_limit_temp_hi_c10);
    temp_fail_hit = (s_temp_c10 < (s_limit_temp_lo_c10 - 100)) || (s_temp_c10 > (s_limit_temp_hi_c10 + 100));
    gyro_limit_hit = (s_gyro_peak_dps >= s_limit_gyro_dps);
    if (s_motion_prev_valid)
    {
        dax = (int32_t)s_accel_x_mg - (int32_t)s_prev_ax_mg;
        day = (int32_t)s_accel_y_mg - (int32_t)s_prev_ay_mg;
        daz = (int32_t)s_accel_z_mg - (int32_t)s_prev_az_mg;
        dax = (dax < 0) ? -dax : dax;
        day = (day < 0) ? -day : day;
        daz = (daz < 0) ? -daz : daz;
    }
    jerk_peak = dax;
    if (day > jerk_peak)
    {
        jerk_peak = day;
    }
    if (daz > jerk_peak)
    {
        jerk_peak = daz;
    }

    predicted_inverted_warn = (s_accel_z_mg <= -700) && (abx <= 600) && (aby <= 600);
    predicted_tilt_warn = !predicted_inverted_warn &&
                          ((abx >= 1000) || (aby >= 1000)) &&
                          (abz >= 350) &&
                          (jerk_peak <= 500) &&
                          (s_gyro_peak_dps <= 300) &&
                          !accel_warn_hit;
    predicted_temp_approach_low = !temp_limit_hit && !temp_fail_hit && (s_temp_c10 <= (s_limit_temp_lo_c10 + 30));
    predicted_temp_approach_high = !temp_limit_hit && !temp_fail_hit && (s_temp_c10 >= (s_limit_temp_hi_c10 - 30));
    gyro_predict_warn_dps = (uint16_t)((s_limit_gyro_dps * 7u) / 10u);
    predicted_erratic_motion = !accel_warn_hit &&
                               ((jerk_peak >= 2200) ||
                                (accel_peak_mg >= 2800) ||
                                ((s_gyro_peak_dps >= gyro_predict_warn_dps) && (jerk_peak >= 1400)));
    watch_motion_evidence = (accel_peak_mg >= 1700u) || (jerk_peak >= 500) || (s_gyro_peak_dps >= 140u);
    if (predicted_erratic_motion)
    {
        s_impact_latch_until_ticks = now_ticks + impact_latch_ticks;
    }
    else if (now_ticks < s_impact_latch_until_ticks)
    {
        predicted_erratic_motion = true;
    }
    score_ratio = weighted_pct / 100.0f;
    if (profile != NULL)
    {
        if ((profile->alert_warn > 0.01f) && (profile->alert_warn < 1.0f))
        {
            score_warn_enter = profile->alert_warn;
        }
        if ((profile->alert_fail > score_warn_enter) && (profile->alert_fail < 1.0f))
        {
            score_fail_enter = profile->alert_fail;
        }
    }
    score_warn_exit = score_warn_enter * 0.85f;
    score_fail_exit = score_fail_enter * 0.80f;
    if (score_warn_exit > (score_fail_enter * 0.8f))
    {
        score_warn_exit = score_fail_enter * 0.8f;
    }

    if (accel_fail_hit || temp_fail_hit)
    {
        candidate_status = AI_STATUS_FAULT;
        candidate_reason = accel_fail_hit ? ALERT_REASON_ACCEL_FAIL : ALERT_REASON_TEMP_FAIL;
    }
    else if (accel_warn_hit || temp_limit_hit || gyro_limit_hit)
    {
        candidate_status = AI_STATUS_WARNING;
        if (accel_warn_hit)
        {
            candidate_reason = ALERT_REASON_ACCEL_WARN;
        }
        else if (temp_limit_hit)
        {
            candidate_reason = ALERT_REASON_TEMP_WARN;
        }
        else
        {
            candidate_reason = ALERT_REASON_GYRO_WARN;
        }
    }
    else if (predicted_inverted_warn)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_INVERTED_WARN;
    }
    else if (predicted_tilt_warn)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_TILT_WARN;
    }
    else if (predicted_temp_approach_low)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_TEMP_APPROACH_LOW;
    }
    else if (predicted_temp_approach_high)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_TEMP_APPROACH_HIGH;
    }
    else if (predicted_erratic_motion)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_ERRATIC_MOTION;
    }
    else if (score_ratio >= score_fail_enter)
    {
        candidate_status = AI_STATUS_FAULT;
        candidate_reason = ALERT_REASON_SCORE_FAIL;
    }
    else if ((score_ratio >= score_warn_enter) ||
             ((s_alert_hold_reason == ALERT_REASON_SCORE_WARN) && (score_ratio >= score_warn_exit)) ||
             ((s_alert_hold_reason == ALERT_REASON_SCORE_FAIL) && (score_ratio >= score_fail_exit)))
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_SCORE_WARN;
    }
    else if ((s_anom_out.overall_level >= ANOMALY_LEVEL_WATCH) && watch_motion_evidence)
    {
        candidate_status = AI_STATUS_WARNING;
        candidate_reason = ALERT_REASON_ANOMALY_WATCH;
    }

    dst->ai_status = candidate_status;
    dst->alert_reason_code = candidate_reason;

    /* Persistence gate: require sustained evidence before elevating to warning/fault. */
    if (candidate_status == AI_STATUS_FAULT)
    {
        if (s_fault_candidate_reason != candidate_reason)
        {
            s_fault_candidate_reason = candidate_reason;
            s_fault_candidate_since_ticks = now_ticks;
        }
        fault_matured = (now_ticks - s_fault_candidate_since_ticks) >= fault_persist_ticks;
        if (!fault_matured)
        {
            dst->ai_status = AI_STATUS_NORMAL;
            dst->alert_reason_code = ALERT_REASON_NORMAL;
        }
    }
    else
    {
        s_fault_candidate_reason = ALERT_REASON_NORMAL;
        s_fault_candidate_since_ticks = now_ticks;
    }

    if (candidate_status == AI_STATUS_WARNING)
    {
        switch (candidate_reason)
        {
            case ALERT_REASON_ACCEL_WARN:
            case ALERT_REASON_TEMP_WARN:
            case ALERT_REASON_GYRO_WARN:
                warning_persist_ticks = warn_limit_persist_ticks;
                break;
            case ALERT_REASON_SCORE_WARN:
            case ALERT_REASON_ANOMALY_WATCH:
            case ALERT_REASON_TILT_WARN:
            case ALERT_REASON_INVERTED_WARN:
            case ALERT_REASON_TEMP_APPROACH_LOW:
            case ALERT_REASON_TEMP_APPROACH_HIGH:
                warning_persist_ticks = warn_predict_persist_ticks;
                break;
            case ALERT_REASON_ERRATIC_MOTION:
                warning_persist_ticks = warn_impact_persist_ticks;
                break;
            default:
                warning_persist_ticks = warn_score_persist_ticks;
                break;
        }
        if (s_warn_candidate_reason != candidate_reason)
        {
            s_warn_candidate_reason = candidate_reason;
            s_warn_candidate_since_ticks = now_ticks;
        }
        if ((candidate_reason == ALERT_REASON_ERRATIC_MOTION) && (now_ticks < s_impact_latch_until_ticks))
        {
            warning_matured = true;
        }
        else
        {
            warning_matured = (now_ticks - s_warn_candidate_since_ticks) >= warning_persist_ticks;
        }
        if (!warning_matured)
        {
            dst->ai_status = AI_STATUS_NORMAL;
            dst->alert_reason_code = ALERT_REASON_NORMAL;
        }
    }
    else
    {
        s_warn_candidate_reason = ALERT_REASON_NORMAL;
        s_warn_candidate_since_ticks = now_ticks;
    }

    /* Keep warning/fault visible, while always prioritizing higher-severity alerts. */
    if (dst->ai_status == AI_STATUS_FAULT)
    {
        s_alert_hold_status = dst->ai_status;
        s_alert_hold_reason = dst->alert_reason_code;
        s_alert_hold_until_ticks = now_ticks + hold_fault_ticks;
    }
    else if (dst->ai_status == AI_STATUS_WARNING)
    {
        bool hold_active = (s_alert_hold_status != AI_STATUS_NORMAL) && (now_ticks < s_alert_hold_until_ticks);
        if (!hold_active || (s_alert_hold_status == AI_STATUS_NORMAL))
        {
            s_alert_hold_status = dst->ai_status;
            s_alert_hold_reason = dst->alert_reason_code;
            s_alert_hold_until_ticks = now_ticks + hold_warn_ticks;
        }
        else if (s_alert_hold_status == AI_STATUS_FAULT)
        {
            dst->ai_status = s_alert_hold_status;
            dst->alert_reason_code = s_alert_hold_reason;
        }
        else
        {
            uint8_t live_pri = AlertReasonWarningPriority(dst->alert_reason_code);
            uint8_t hold_pri = AlertReasonWarningPriority(s_alert_hold_reason);
            if ((live_pri > hold_pri) || ((live_pri == hold_pri) && (dst->alert_reason_code != s_alert_hold_reason)))
            {
                s_alert_hold_status = dst->ai_status;
                s_alert_hold_reason = dst->alert_reason_code;
                s_alert_hold_until_ticks = now_ticks + hold_warn_ticks;
            }
            else
            {
                dst->ai_status = s_alert_hold_status;
                dst->alert_reason_code = s_alert_hold_reason;
            }
        }
    }
    else if ((s_alert_hold_status != AI_STATUS_NORMAL) && (now_ticks < s_alert_hold_until_ticks))
    {
        dst->ai_status = s_alert_hold_status;
        dst->alert_reason_code = s_alert_hold_reason;
    }
    else
    {
        s_alert_hold_status = AI_STATUS_NORMAL;
        s_alert_hold_reason = ALERT_REASON_NORMAL;
    }

    /* Freeze adaptive baseline during active alerts, then hold freeze for stable-normal recovery. */
    if (dst->ai_status != AI_STATUS_NORMAL)
    {
        s_adapt_freeze_until_ticks = now_ticks + adapt_freeze_recover_ticks;
        freeze_adapt = true;
    }
    else
    {
        freeze_adapt = (now_ticks < s_adapt_freeze_until_ticks);
    }
    AnomalyEngine_SetAdaptiveFreeze(freeze_adapt);

    dst->ai_fault_flags = 0u;
    s_prev_ax_mg = s_accel_x_mg;
    s_prev_ay_mg = s_accel_y_mg;
    s_prev_az_mg = s_accel_z_mg;
    s_motion_prev_valid = true;
}

static void AccelAxisSelfTestLog(void)
{
#if EDGEAI_ENABLE_ACCEL_TEST_LOG
    int32_t ax = s_accel_x_mg;
    int32_t ay = s_accel_y_mg;
    int32_t az = s_accel_z_mg;
    int32_t abx = (ax < 0) ? -ax : ax;
    int32_t aby = (ay < 0) ? -ay : ay;
    int32_t abz = (az < 0) ? -az : az;
    char axis = 'X';
    int32_t val = ax;

    if (aby > abx)
    {
        axis = 'Y';
        val = ay;
        abx = aby;
    }
    if (abz > abx)
    {
        axis = 'Z';
        val = az;
    }

    PRINTF("ACCEL_TEST,DOM=%c%s,X=%d,Y=%d,Z=%d\r\n",
           axis, (val >= 0) ? "+" : "-", (int)ax, (int)ay, (int)az);
#else
    /* Disabled by default to avoid periodic UART blocking in the render loop. */
#endif
}

static void TouchDelayMs(uint32_t delay_ms)
{
    SDK_DelayAtLeastUs(delay_ms * 1000u, CoreClockHz());
}

static uint32_t TimebaseCalibrateHz(bool use_raw)
{
    uint64_t t0 = use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
    uint64_t t1;
    uint64_t dt_ticks;
    uint64_t hz64;
    uint32_t core_hz = CoreClockHz();

    SDK_DelayAtLeastUs(EDGEAI_TIMEBASE_CAL_WINDOW_US, core_hz);
    t1 = use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
    dt_ticks = t1 - t0;
    if (dt_ticks == 0u)
    {
        return 0u;
    }

    hz64 = (dt_ticks * 1000000ull) / EDGEAI_TIMEBASE_CAL_WINDOW_US;
    if ((hz64 < 1000ull) || (hz64 > 1000000ull))
    {
        return 0u;
    }
    return (uint32_t)hz64;
}

static bool TimebaseInit(void)
{
    status_t st = CLOCK_SetupOsc32KClocking(kCLOCK_Osc32kToWake);
    const uint32_t nominal_hz = EDGEAI_TIMEBASE_CRYSTAL_HZ;
    const uint32_t min_cfg_hz = 1000u;
    const uint32_t max_cfg_hz = 1000000u;
    const uint32_t min_meas_hz = 1000u;
    const uint32_t max_meas_hz = 1000000u;
    uint32_t cfg_hz;
    uint32_t raw_hz;
    uint32_t dec_hz;
    const char *src = "nominal";

    CLOCK_AttachClk(kXTAL32K2_to_OSTIMER);
    OSTIMER_Init(OSTIMER0);
    cfg_hz = CLOCK_GetOstimerClkFreq();

    s_timebase_use_core_cycle = false;
    s_timebase_use_raw = false;
    s_timebase_hz = nominal_hz;

    /* Keep calibration diagnostics for UART visibility only.
     * Do not use calibrated values for runtime conversion; delay-based calibration
     * can skew significantly depending on clock-tree/transient conditions. */
    raw_hz = TimebaseCalibrateHz(true);
    dec_hz = TimebaseCalibrateHz(false);

    /* Use decimal OSTIMER counter for elapsed math.
     * The raw counter may be Gray-encoded on this device, so raw deltas are not valid. */
    if ((dec_hz >= min_meas_hz) && (dec_hz <= max_meas_hz))
    {
        s_timebase_hz = dec_hz;
        src = "dec_meas";
    }
    else if ((cfg_hz >= min_cfg_hz) && (cfg_hz <= max_cfg_hz))
    {
        s_timebase_hz = cfg_hz;
        src = "cfg";
    }

    PRINTF("TIMEBASE: src=ostimer32k setup=%d cfg=%u raw=%u dec=%u sel=%s mode=%s use_hz=%u\r\n",
           (int)st,
           (unsigned int)cfg_hz,
           (unsigned int)raw_hz,
           (unsigned int)dec_hz,
           src,
           s_timebase_use_raw ? "raw" : "dec",
           (unsigned int)s_timebase_hz);
    s_timebase_ready = true;
    return true;
}

static uint64_t TimebaseNowTicks(void)
{
    if (s_timebase_use_core_cycle)
    {
        uint32_t now = DWT->CYCCNT;
        uint32_t delta = now - s_core_cycle_prev;
        s_core_cycle_prev = now;
        s_core_cycle_accum += (uint64_t)delta;
        return s_core_cycle_accum;
    }
    if (!s_timebase_ready)
    {
        return 0u;
    }
    return s_timebase_use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
}

static void DelayUsByTimebase(uint32_t delay_us)
{
    uint64_t start_ticks;
    uint64_t wait_ticks;

    if (!s_timebase_ready)
    {
        SDK_DelayAtLeastUs(delay_us, CoreClockHz());
        return;
    }

    start_ticks = TimebaseNowTicks();
    wait_ticks = ((uint64_t)delay_us * s_timebase_hz) / 1000000ull;
    if (wait_ticks == 0u)
    {
        wait_ticks = 1u;
    }
    while ((TimebaseNowTicks() - start_ticks) < wait_ticks)
    {
    }
}

static uint32_t CoreClockHz(void)
{
    uint32_t hz = CLOCK_GetCoreSysClkFreq();
    if (hz == 0u)
    {
        hz = SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY;
    }
    return hz;
}

static bool TouchI2CRecover(void)
{
    s_touch_i2c_inited = false;
    LPI2C_MasterDeinit(TOUCH_I2C);
    return TouchI2CInit();
}

static bool TouchI2CInit(void)
{
    uint32_t src_hz;
    lpi2c_master_config_t cfg;

    if (s_touch_i2c_inited)
    {
        return true;
    }

    CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);

    src_hz = CLOCK_GetLPFlexCommClkFreq(TOUCH_I2C_FLEXCOMM_INDEX);
    if (src_hz == 0u)
    {
        PRINTF("TOUCH i2c init failed: FC2 clock=0\r\n");
        return false;
    }

    LPI2C_MasterGetDefaultConfig(&cfg);
    /* Shared bus (GT911 + ST sensor shield): keep conservative speed for stability. */
    cfg.baudRate_Hz = 100000u;
    LPI2C_MasterInit(TOUCH_I2C, &cfg, src_hz);
    s_touch_i2c_inited = true;
    return true;
}

static bool AccelI2CRecover(void)
{
    s_accel_i2c_inited = false;
    LPI2C_MasterDeinit(ACCEL_I2C);
    return AccelI2CInit();
}

static bool AccelI2CInit(void)
{
    uint32_t src_hz;
    lpi2c_master_config_t cfg;

    if (s_accel_i2c_inited)
    {
        return true;
    }

    CLOCK_SetClkDiv(kCLOCK_DivFlexcom3Clk, 1u);
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);

    src_hz = CLOCK_GetLPFlexCommClkFreq(ACCEL_I2C_FLEXCOMM_INDEX);
    if (src_hz == 0u)
    {
        PRINTF("ACCEL i2c init failed: FC3 clock=0\r\n");
        return false;
    }

    LPI2C_MasterGetDefaultConfig(&cfg);
    cfg.baudRate_Hz = 100000u;
    LPI2C_MasterInit(ACCEL_I2C, &cfg, src_hz);
    s_accel_i2c_inited = true;
    return true;
}

static bool ShieldBusTransferWithRetry(bool use_touch_bus, lpi2c_master_transfer_t *xfer)
{
    LPI2C_Type *base = use_touch_bus ? TOUCH_I2C : ACCEL_I2C;
    bool inited = use_touch_bus ? TouchI2CInit() : AccelI2CInit();
    uint32_t attempt;

    if (!inited || (xfer == NULL))
    {
        return false;
    }

    for (attempt = 0u; attempt < EDGEAI_I2C_RETRY_COUNT; attempt++)
    {
        if (LPI2C_MasterTransferBlocking(base, xfer) == kStatus_Success)
        {
            return true;
        }
        if (use_touch_bus)
        {
            (void)TouchI2CRecover();
        }
        else
        {
            (void)AccelI2CRecover();
        }
        SDK_DelayAtLeastUs(300u, CoreClockHz());
    }

    return false;
}

static status_t TouchI2CSend(uint8_t deviceAddress,
                             uint32_t subAddress,
                             uint8_t subaddressSize,
                             const uint8_t *txBuff,
                             uint8_t txBuffSize)
{
    lpi2c_master_transfer_t xfer;
    status_t st;
    uint32_t attempt;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = subAddress;
    xfer.subaddressSize = subaddressSize;
    xfer.data = (uint8_t *)(uintptr_t)txBuff;
    xfer.dataSize = txBuffSize;
    st = kStatus_Fail;
    for (attempt = 0u; attempt < EDGEAI_I2C_RETRY_COUNT; attempt++)
    {
        st = LPI2C_MasterTransferBlocking(TOUCH_I2C, &xfer);
        if (st == kStatus_Success)
        {
            break;
        }
        (void)TouchI2CRecover();
        SDK_DelayAtLeastUs(250u, CoreClockHz());
    }
    return st;
}

static status_t TouchI2CReceive(uint8_t deviceAddress,
                                uint32_t subAddress,
                                uint8_t subaddressSize,
                                uint8_t *rxBuff,
                                uint8_t rxBuffSize)
{
    lpi2c_master_transfer_t xfer;
    status_t st;
    uint32_t attempt;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = subAddress;
    xfer.subaddressSize = subaddressSize;
    xfer.data = rxBuff;
    xfer.dataSize = rxBuffSize;
    st = kStatus_Fail;
    for (attempt = 0u; attempt < EDGEAI_I2C_RETRY_COUNT; attempt++)
    {
        st = LPI2C_MasterTransferBlocking(TOUCH_I2C, &xfer);
        if (st == kStatus_Success)
        {
            break;
        }
        (void)TouchI2CRecover();
        SDK_DelayAtLeastUs(250u, CoreClockHz());
    }
    return st;
}

static bool AccelI2CWrite(uint8_t deviceAddress, uint8_t reg, const uint8_t *txBuff, uint32_t txLen)
{
    lpi2c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1u;
    xfer.data = (uint8_t *)(uintptr_t)txBuff;
    xfer.dataSize = txLen;
    return (LPI2C_MasterTransferBlocking(ACCEL_I2C, &xfer) == kStatus_Success);
}

static bool AccelI2CRead(uint8_t deviceAddress, uint8_t reg, uint8_t *rxBuff, uint32_t rxLen)
{
    lpi2c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1u;
    xfer.data = rxBuff;
    xfer.dataSize = rxLen;
    return (LPI2C_MasterTransferBlocking(ACCEL_I2C, &xfer) == kStatus_Success);
}

static bool ShieldBusRead(bool use_touch_bus, uint8_t deviceAddress, uint8_t reg, uint8_t *rxBuff, uint32_t rxLen)
{
    lpi2c_master_transfer_t xfer;

    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1u;
    xfer.data = rxBuff;
    xfer.dataSize = rxLen;
    return ShieldBusTransferWithRetry(use_touch_bus, &xfer);
}

static bool ShieldBusWriteReg(bool use_touch_bus, uint8_t deviceAddress, uint8_t reg, uint8_t value)
{
    lpi2c_master_transfer_t xfer;

    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1u;
    xfer.data = &value;
    xfer.dataSize = 1u;
    return ShieldBusTransferWithRetry(use_touch_bus, &xfer);
}

static bool ShieldBusProbeAddress(bool use_touch_bus, uint8_t deviceAddress)
{
    lpi2c_master_transfer_t xfer;

    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = 0u;
    xfer.subaddressSize = 0u;
    xfer.data = NULL;
    xfer.dataSize = 0u;
    return ShieldBusTransferWithRetry(use_touch_bus, &xfer);
}

static bool ShieldBusWriteRaw(bool use_touch_bus, uint8_t deviceAddress, const uint8_t *txBuff, uint32_t txLen)
{
    lpi2c_master_transfer_t xfer;

    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = 0u;
    xfer.subaddressSize = 0u;
    xfer.data = (uint8_t *)(uintptr_t)txBuff;
    xfer.dataSize = txLen;
    return ShieldBusTransferWithRetry(use_touch_bus, &xfer);
}

static bool ShieldBusReadRaw(bool use_touch_bus, uint8_t deviceAddress, uint8_t *rxBuff, uint32_t rxLen)
{
    lpi2c_master_transfer_t xfer;

    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = deviceAddress;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = 0u;
    xfer.subaddressSize = 0u;
    xfer.data = rxBuff;
    xfer.dataSize = rxLen;
    return ShieldBusTransferWithRetry(use_touch_bus, &xfer);
}

static bool ShieldShubSetAccess(bool enable)
{
    uint8_t value = enable ? SHIELD_SHUB_FUNC_CFG_ACCESS : 0u;
    return ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_FUNC_CFG_ACCESS, value);
}

static bool ShieldShubResetMaster(uint8_t master_cfg)
{
    if (!ShieldBusWriteReg(s_shield_use_touch_bus,
                           s_shield_gyro_addr,
                           SHIELD_SHUB_REG_MASTER_CONFIG,
                           (uint8_t)(master_cfg | SHIELD_SHUB_MASTER_RST_REGS)))
    {
        return false;
    }
    return ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_MASTER_CONFIG, master_cfg);
}

static bool ShieldShubReadRegs(uint8_t ext_addr7, uint8_t reg, uint8_t *rx, uint8_t len)
{
    uint8_t master_cfg = 0u;
    uint8_t status = 0u;
    bool ok = false;

    if ((rx == NULL) || (len == 0u) || (len > 6u) || !s_shield_gyro_ready || (s_shield_gyro_addr == 0u))
    {
        return false;
    }

    if (!ShieldShubSetAccess(true))
    {
        return false;
    }

    do
    {
        if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_MASTER_CONFIG, &master_cfg, 1u))
        {
            break;
        }
        if (!ShieldShubResetMaster(master_cfg))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_ADD, (uint8_t)((ext_addr7 << 1) | 0x01u)))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_SUBADD, reg))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_CONFIG, (uint8_t)(len & 0x07u)))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus,
                               s_shield_gyro_addr,
                               SHIELD_SHUB_REG_MASTER_CONFIG,
                               (uint8_t)((master_cfg & 0xF0u) | SHIELD_SHUB_MASTER_SHUB_PU_EN | SHIELD_SHUB_MASTER_ON)))
        {
            break;
        }

        for (uint32_t attempt = 0u; attempt < 16u; attempt++)
        {
            if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_STATUS_MASTER, &status, 1u))
            {
                break;
            }
            if ((status & SHIELD_SHUB_STATUS_ENDOP) != 0u)
            {
                break;
            }
            SDK_DelayAtLeastUs(1200u, CoreClockHz());
        }

        if (((status & SHIELD_SHUB_STATUS_ENDOP) == 0u) || ((status & SHIELD_SHUB_STATUS_NACK_MASK) != 0u))
        {
            break;
        }
        if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SENSOR_HUB_1, rx, len))
        {
            break;
        }
        ok = true;
    } while (false);

    (void)ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_MASTER_CONFIG, master_cfg);
    (void)ShieldShubSetAccess(false);
    return ok;
}

static bool ShieldShubWriteReg(uint8_t ext_addr7, uint8_t reg, uint8_t value)
{
    uint8_t master_cfg = 0u;
    uint8_t status = 0u;
    bool ok = false;

    if (!s_shield_gyro_ready || (s_shield_gyro_addr == 0u))
    {
        return false;
    }

    if (!ShieldShubSetAccess(true))
    {
        return false;
    }

    do
    {
        if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_MASTER_CONFIG, &master_cfg, 1u))
        {
            break;
        }
        if (!ShieldShubResetMaster(master_cfg))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_ADD, (uint8_t)(ext_addr7 << 1)))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_SUBADD, reg))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_DATAWRITE_SLV0, value))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_SLV0_CONFIG, 0x00u))
        {
            break;
        }
        if (!ShieldBusWriteReg(s_shield_use_touch_bus,
                               s_shield_gyro_addr,
                               SHIELD_SHUB_REG_MASTER_CONFIG,
                               (uint8_t)((master_cfg & 0xB0u) | SHIELD_SHUB_MASTER_WRITE_ONCE |
                                         SHIELD_SHUB_MASTER_SHUB_PU_EN | SHIELD_SHUB_MASTER_ON)))
        {
            break;
        }

        for (uint32_t attempt = 0u; attempt < 16u; attempt++)
        {
            if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_STATUS_MASTER, &status, 1u))
            {
                break;
            }
            if ((status & SHIELD_SHUB_STATUS_ENDOP) != 0u)
            {
                break;
            }
            SDK_DelayAtLeastUs(1200u, CoreClockHz());
        }

        if (((status & SHIELD_SHUB_STATUS_ENDOP) == 0u) || ((status & SHIELD_SHUB_STATUS_NACK_MASK) != 0u))
        {
            break;
        }
        ok = true;
    } while (false);

    (void)ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_SHUB_REG_MASTER_CONFIG, master_cfg);
    (void)ShieldShubSetAccess(false);
    return ok;
}

static bool ShieldSht40CrcOk(const uint8_t *buf2, uint8_t crc)
{
    uint8_t c = 0xFFu;
    for (uint32_t i = 0u; i < 2u; i++)
    {
        c ^= buf2[i];
        for (uint32_t b = 0u; b < 8u; b++)
        {
            c = (c & 0x80u) ? (uint8_t)((c << 1) ^ 0x31u) : (uint8_t)(c << 1);
        }
    }
    return (c == crc);
}

static void ShieldAuxSetRenderState(void)
{
    GaugeRender_SetMag(s_mag_x_mgauss,
                       s_mag_y_mgauss,
                       s_mag_z_mgauss,
                       s_shield_mag_ready);
    GaugeRender_SetBaro(s_baro_dhpa, s_shield_baro_ready);
    GaugeRender_SetSht(s_sht_temp_c10, s_sht_rh_dpct, s_shield_sht_ready);
    GaugeRender_SetStts(s_stts_temp_c10, s_shield_stts_ready);
}

static void ShieldAuxInit(void)
{
    static const bool buses[2] = {true, false};
    static const char *bus_name[2] = {"FC2", "FC3"};
    static const uint8_t stts_addrs[] = {0x3Cu, 0x3Du, 0x3Eu, 0x3Fu, 0x38u};
    uint8_t who = 0u;

    s_shield_mag_ready = false;
    s_shield_baro_ready = false;
    s_shield_sht_ready = false;
    s_shield_stts_ready = false;
    s_shield_mag_use_shub = false;
    s_shield_baro_use_shub = false;
    s_shield_stts_use_shub = false;
    s_shield_mag_use_touch_bus = true;
    s_shield_mag_addr = SHIELD_LIS2MDL_ADDR;
    s_shield_baro_use_touch_bus = true;
    s_shield_sht_use_touch_bus = true;
    s_shield_stts_use_touch_bus = true;
    s_shield_baro_addr = 0u;
    s_shield_sht_addr = 0u;
    s_shield_stts_addr = 0u;

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        if (ShieldBusRead(buses[bi], SHIELD_LIS2MDL_ADDR, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LIS2MDL_WHOAMI))
        {
            s_shield_mag_use_touch_bus = buses[bi];
            s_shield_mag_addr = SHIELD_LIS2MDL_ADDR;
            s_shield_mag_ready = true;
            break;
        }
        if (ShieldBusRead(buses[bi], SHIELD_LIS2MDL_ADDR_ALT, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LIS2MDL_WHOAMI))
        {
            s_shield_mag_use_touch_bus = buses[bi];
            s_shield_mag_addr = SHIELD_LIS2MDL_ADDR_ALT;
            s_shield_mag_ready = true;
            break;
        }
    }
    if (!s_shield_mag_ready && s_shield_gyro_ready && ShieldImuSupportsShub(s_shield_gyro_who))
    {
        if (ShieldShubReadRegs(SHIELD_LIS2MDL_ADDR, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LIS2MDL_WHOAMI))
        {
            s_shield_mag_addr = SHIELD_LIS2MDL_ADDR;
            s_shield_mag_ready = true;
            s_shield_mag_use_shub = true;
        }
        else if (ShieldShubReadRegs(SHIELD_LIS2MDL_ADDR_ALT, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u) &&
                 (who == SHIELD_LIS2MDL_WHOAMI))
        {
            s_shield_mag_addr = SHIELD_LIS2MDL_ADDR_ALT;
            s_shield_mag_ready = true;
            s_shield_mag_use_shub = true;
        }
    }

    if (s_shield_mag_ready)
    {
        if (s_shield_mag_use_shub)
        {
            (void)ShieldShubWriteReg(s_shield_mag_addr, SHIELD_LIS2MDL_REG_CFG_A, 0x00u);
            (void)ShieldShubWriteReg(s_shield_mag_addr, SHIELD_LIS2MDL_REG_CFG_C, 0x10u);
            PRINTF("SHIELD_MAG ready bus=SHUB addr=0x%02x\r\n", (unsigned int)s_shield_mag_addr);
        }
        else
        {
            (void)ShieldBusWriteReg(s_shield_mag_use_touch_bus, s_shield_mag_addr, SHIELD_LIS2MDL_REG_CFG_A, 0x00u);
            (void)ShieldBusWriteReg(s_shield_mag_use_touch_bus, s_shield_mag_addr, SHIELD_LIS2MDL_REG_CFG_C, 0x10u);
            PRINTF("SHIELD_MAG ready bus=%s addr=0x%02x\r\n",
                   s_shield_mag_use_touch_bus ? bus_name[0] : bus_name[1],
                   (unsigned int)s_shield_mag_addr);
        }
    }
    else
    {
        PRINTF("SHIELD_MAG not_found\r\n");
    }

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        if (ShieldBusRead(buses[bi], SHIELD_LPS22DF_ADDR0, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LPS22DF_WHOAMI))
        {
            s_shield_baro_addr = SHIELD_LPS22DF_ADDR0;
            s_shield_baro_use_touch_bus = buses[bi];
            break;
        }
        if (ShieldBusRead(buses[bi], SHIELD_LPS22DF_ADDR1, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LPS22DF_WHOAMI))
        {
            s_shield_baro_addr = SHIELD_LPS22DF_ADDR1;
            s_shield_baro_use_touch_bus = buses[bi];
            break;
        }
    }
    if ((s_shield_baro_addr == 0u) && s_shield_gyro_ready && ShieldImuSupportsShub(s_shield_gyro_who))
    {
        if (ShieldShubReadRegs(SHIELD_LPS22DF_ADDR0, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) &&
            (who == SHIELD_LPS22DF_WHOAMI))
        {
            s_shield_baro_addr = SHIELD_LPS22DF_ADDR0;
            s_shield_baro_use_shub = true;
        }
        else if (ShieldShubReadRegs(SHIELD_LPS22DF_ADDR1, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) &&
                 (who == SHIELD_LPS22DF_WHOAMI))
        {
            s_shield_baro_addr = SHIELD_LPS22DF_ADDR1;
            s_shield_baro_use_shub = true;
        }
    }

    if (s_shield_baro_addr != 0u)
    {
        if (s_shield_baro_use_shub)
        {
            (void)ShieldShubWriteReg(s_shield_baro_addr, SHIELD_LPS22DF_REG_CTRL1, 0x22u);
        }
        else
        {
            (void)ShieldBusWriteReg(s_shield_baro_use_touch_bus, s_shield_baro_addr, SHIELD_LPS22DF_REG_CTRL1, 0x22u);
        }
        s_shield_baro_ready = true;
        if (s_shield_baro_use_shub)
        {
            PRINTF("SHIELD_BARO ready bus=SHUB addr=0x%02x\r\n", (unsigned int)s_shield_baro_addr);
        }
        else
        {
            PRINTF("SHIELD_BARO ready bus=%s addr=0x%02x\r\n",
                   s_shield_baro_use_touch_bus ? bus_name[0] : bus_name[1],
                   (unsigned int)s_shield_baro_addr);
        }
    }

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        if (ShieldBusProbeAddress(buses[bi], SHIELD_SHT40_ADDR0))
        {
            s_shield_sht_addr = SHIELD_SHT40_ADDR0;
            s_shield_sht_use_touch_bus = buses[bi];
            break;
        }
        if (ShieldBusProbeAddress(buses[bi], SHIELD_SHT40_ADDR1))
        {
            s_shield_sht_addr = SHIELD_SHT40_ADDR1;
            s_shield_sht_use_touch_bus = buses[bi];
            break;
        }
    }
    if (s_shield_sht_addr != 0u)
    {
        s_shield_sht_ready = true;
        PRINTF("SHIELD_SHT ready bus=%s addr=0x%02x\r\n",
               s_shield_sht_use_touch_bus ? bus_name[0] : bus_name[1],
               (unsigned int)s_shield_sht_addr);
    }

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        for (uint32_t i = 0u; i < (sizeof(stts_addrs) / sizeof(stts_addrs[0])); i++)
        {
            if (!ShieldBusRead(buses[bi], stts_addrs[i], SHIELD_STTS22H_REG_WHO_AM_I, &who, 1u))
            {
                continue;
            }
            if ((who == SHIELD_STTS22H_WHOAMI) || ((who != 0x00u) && (who != 0xFFu)))
            {
                s_shield_stts_addr = stts_addrs[i];
                s_shield_stts_use_touch_bus = buses[bi];
                break;
            }
        }
        if (s_shield_stts_addr != 0u)
        {
            break;
        }
    }
    if ((s_shield_stts_addr == 0u) && s_shield_gyro_ready && ShieldImuSupportsShub(s_shield_gyro_who))
    {
        for (uint32_t i = 0u; i < (sizeof(stts_addrs) / sizeof(stts_addrs[0])); i++)
        {
            if (!ShieldShubReadRegs(stts_addrs[i], SHIELD_STTS22H_REG_WHO_AM_I, &who, 1u))
            {
                continue;
            }
            if ((who == SHIELD_STTS22H_WHOAMI) || ((who != 0x00u) && (who != 0xFFu)))
            {
                s_shield_stts_addr = stts_addrs[i];
                s_shield_stts_use_shub = true;
                break;
            }
        }
    }

    if (s_shield_stts_addr != 0u)
    {
        if (s_shield_stts_use_shub)
        {
            (void)ShieldShubWriteReg(s_shield_stts_addr, SHIELD_STTS22H_REG_CTRL, 0x0Cu);
        }
        else
        {
            (void)ShieldBusWriteReg(s_shield_stts_use_touch_bus, s_shield_stts_addr, SHIELD_STTS22H_REG_CTRL, 0x0Cu);
        }
        s_shield_stts_ready = true;
        if (s_shield_stts_use_shub)
        {
            PRINTF("SHIELD_STTS ready bus=SHUB addr=0x%02x\r\n", (unsigned int)s_shield_stts_addr);
        }
        else
        {
            PRINTF("SHIELD_STTS ready bus=%s addr=0x%02x\r\n",
                   s_shield_stts_use_touch_bus ? bus_name[0] : bus_name[1],
                   (unsigned int)s_shield_stts_addr);
        }
    }

    s_shield_aux_init_done = true;
    ShieldAuxSetRenderState();
}

static void ShieldAuxUpdate(void)
{
    uint8_t raw[6];

    if (!s_shield_aux_init_done)
    {
        ShieldAuxInit();
    }
    if (s_shield_mag_ready)
    {
        bool ok = true;
        for (uint32_t i = 0u; i < 6u; i++)
        {
            if (s_shield_mag_use_shub)
            {
                if (!ShieldShubReadRegs(s_shield_mag_addr, (uint8_t)(SHIELD_LIS2MDL_REG_OUTX_L + i), &raw[i], 1u))
                {
                    ok = false;
                    break;
                }
            }
            else if (!ShieldBusRead(s_shield_mag_use_touch_bus, s_shield_mag_addr, (uint8_t)(SHIELD_LIS2MDL_REG_OUTX_L + i), &raw[i], 1u))
            {
                ok = false;
                break;
            }
        }
        if (ok)
        {
            int16_t mx = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
            int16_t my = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
            int16_t mz = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);
            s_mag_x_mgauss = (int16_t)((mx * 15) / 10);
            s_mag_y_mgauss = (int16_t)((my * 15) / 10);
            s_mag_z_mgauss = (int16_t)((mz * 15) / 10);
        }
        else
        {
            s_shield_mag_ready = false;
            s_shield_mag_reprobe_ticks = 4u;
        }
    }
    else
    {
        if (s_shield_mag_reprobe_ticks > 0u)
        {
            s_shield_mag_reprobe_ticks--;
        }
        else
        {
            /* Re-scan magnetometer path periodically when unavailable. */
            s_shield_aux_init_done = false;
            ShieldAuxInit();
            s_shield_mag_reprobe_ticks = 4u;
        }
    }

    if (s_shield_baro_ready)
    {
        if ((s_shield_baro_use_shub && ShieldShubReadRegs(s_shield_baro_addr, SHIELD_LPS22DF_REG_OUT_P_XL, raw, 5u)) ||
            (!s_shield_baro_use_shub &&
             ShieldBusRead(s_shield_baro_use_touch_bus, s_shield_baro_addr, SHIELD_LPS22DF_REG_OUT_P_XL, raw, 5u)))
        {
            int32_t p_raw = (int32_t)((((uint32_t)raw[2]) << 16) | (((uint32_t)raw[1]) << 8) | raw[0]);
            if ((p_raw & 0x00800000) != 0)
            {
                p_raw |= (int32_t)0xFF000000;
            }
            s_baro_dhpa = (int16_t)((p_raw * 10) / SHIELD_LPS22DF_PRESS_LSB_PER_HPA);
        }
        else
        {
            s_shield_baro_ready = false;
        }
    }

    if (s_shield_sht_ready)
    {
        uint8_t cmd = SHIELD_SHT40_CMD_MEASURE_LP;
        if (ShieldBusWriteRaw(s_shield_sht_use_touch_bus, s_shield_sht_addr, &cmd, 1u))
        {
            uint16_t raw_t;
            uint16_t raw_rh;
            SDK_DelayAtLeastUs(2500u, CoreClockHz());
            if (ShieldBusReadRaw(s_shield_sht_use_touch_bus, s_shield_sht_addr, raw, 6u) &&
                ShieldSht40CrcOk(&raw[0], raw[2]) &&
                ShieldSht40CrcOk(&raw[3], raw[5]))
            {
                raw_t = (uint16_t)(((uint16_t)raw[0] << 8) | raw[1]);
                raw_rh = (uint16_t)(((uint16_t)raw[3] << 8) | raw[4]);
                s_sht_temp_c10 = (int16_t)(((1750 * (int32_t)raw_t) / 65535) - 450);
                s_sht_rh_dpct = (int16_t)(((1250 * (int32_t)raw_rh) / 65535) - 60);
                if (s_sht_rh_dpct < 0)
                {
                    s_sht_rh_dpct = 0;
                }
                if (s_sht_rh_dpct > 1000)
                {
                    s_sht_rh_dpct = 1000;
                }
            }
            else
            {
                s_shield_sht_ready = false;
            }
        }
        else
        {
            s_shield_sht_ready = false;
        }
    }

    if (s_shield_stts_ready)
    {
        if ((s_shield_stts_use_shub && ShieldShubReadRegs(s_shield_stts_addr, SHIELD_STTS22H_REG_TEMP_L, raw, 2u)) ||
            (!s_shield_stts_use_shub &&
             ShieldBusRead(s_shield_stts_use_touch_bus, s_shield_stts_addr, SHIELD_STTS22H_REG_TEMP_L, raw, 2u)))
        {
            int16_t t_raw = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
            s_stts_temp_c10 = (int16_t)(t_raw / 10);
        }
        else
        {
            s_shield_stts_ready = false;
        }
    }

    ShieldAuxSetRenderState();
}

static void __attribute__((unused)) ShieldScanI2CAddresses(bool use_touch_bus, const char *bus_name)
{
    bool found = false;
    PRINTF("SHIELD scan %s addr:", bus_name);
    for (uint8_t addr = 0x08u; addr <= 0x77u; addr++)
    {
        if (ShieldBusProbeAddress(use_touch_bus, addr))
        {
            PRINTF(" 0x%02x", (unsigned int)addr);
            found = true;
        }
    }
    if (!found)
    {
        PRINTF(" none");
    }
    PRINTF("\r\n");
}

static bool __attribute__((unused)) DiagLpi2cInit(LPI2C_Type *base, uint32_t fc_idx, uint32_t baud_hz)
{
    lpi2c_master_config_t cfg;
    uint32_t src_hz;

    if ((base == TOUCH_I2C) && !s_touch_i2c_inited)
    {
        CLOCK_SetClkDiv(kCLOCK_DivFlexcom2Clk, 1u);
        CLOCK_AttachClk(kFRO12M_to_FLEXCOMM2);
    }
    if ((base == ACCEL_I2C) && !s_accel_i2c_inited)
    {
        CLOCK_SetClkDiv(kCLOCK_DivFlexcom3Clk, 1u);
        CLOCK_AttachClk(kFRO12M_to_FLEXCOMM3);
    }

    src_hz = CLOCK_GetLPFlexCommClkFreq(fc_idx);
    if (src_hz == 0u)
    {
        return false;
    }
    LPI2C_MasterGetDefaultConfig(&cfg);
    cfg.baudRate_Hz = baud_hz;
    LPI2C_MasterDeinit(base);
    LPI2C_MasterInit(base, &cfg, src_hz);
    return true;
}

static bool __attribute__((unused)) DiagLpi2cProbeAddr(LPI2C_Type *base, uint8_t addr7)
{
    lpi2c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = addr7;
    xfer.direction = kLPI2C_Write;
    xfer.subaddress = 0u;
    xfer.subaddressSize = 0u;
    xfer.data = NULL;
    xfer.dataSize = 0u;
    return (LPI2C_MasterTransferBlocking(base, &xfer) == kStatus_Success);
}

static bool __attribute__((unused)) DiagLpi2cReadReg(LPI2C_Type *base, uint8_t addr7, uint8_t reg, uint8_t *val)
{
    lpi2c_master_transfer_t xfer;
    if (val == NULL)
    {
        return false;
    }
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kLPI2C_TransferDefaultFlag;
    xfer.slaveAddress = addr7;
    xfer.direction = kLPI2C_Read;
    xfer.subaddress = reg;
    xfer.subaddressSize = 1u;
    xfer.data = val;
    xfer.dataSize = 1u;
    return (LPI2C_MasterTransferBlocking(base, &xfer) == kStatus_Success);
}

static void __attribute__((unused)) DiagScanLpi2cBus(const char *name, LPI2C_Type *base, uint32_t fc_idx)
{
    static const uint32_t bauds[] = {100000u, 400000u};
    for (uint32_t bi = 0u; bi < (sizeof(bauds) / sizeof(bauds[0])); bi++)
    {
        bool any = false;
        if (!DiagLpi2cInit(base, fc_idx, bauds[bi]))
        {
            PRINTF("DIAG %s %luHz: init_failed\r\n", name, (unsigned long)bauds[bi]);
            continue;
        }
        PRINTF("DIAG %s %luHz addr:", name, (unsigned long)bauds[bi]);
        for (uint8_t addr = 0x08u; addr <= 0x77u; addr++)
        {
            if (DiagLpi2cProbeAddr(base, addr))
            {
                PRINTF(" 0x%02x", (unsigned int)addr);
                any = true;
            }
        }
        if (!any)
        {
            PRINTF(" none");
        }
        PRINTF("\r\n");

        for (uint32_t pi = 0u; pi < (sizeof(kDiagProbes) / sizeof(kDiagProbes[0])); pi++)
        {
            uint8_t who = 0u;
            if (DiagLpi2cReadReg(base, kDiagProbes[pi].addr, kDiagProbes[pi].reg, &who))
            {
                PRINTF("DIAG %s %luHz %s addr=0x%02x reg=0x%02x val=0x%02x\r\n",
                       name,
                       (unsigned long)bauds[bi],
                       kDiagProbes[pi].name,
                       (unsigned int)kDiagProbes[pi].addr,
                       (unsigned int)kDiagProbes[pi].reg,
                       (unsigned int)who);
            }
        }
    }
}

static bool __attribute__((unused)) DiagI3CProbeAddrI2C(uint8_t addr7)
{
    i3c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kI3C_TransferDefaultFlag;
    xfer.slaveAddress = addr7;
    xfer.direction = kI3C_Write;
    xfer.subaddress = 0u;
    xfer.subaddressSize = 0u;
    xfer.data = NULL;
    xfer.dataSize = 0u;
    xfer.busType = kI3C_TypeI2C;
    xfer.ibiResponse = kI3C_IbiRespAck;
    return (I3C_MasterTransferBlocking(TEMP_I3C, &xfer) == kStatus_Success);
}

static void ShieldRunDatastreamDiagnostics(void)
{
#if EDGEAI_ENABLE_SHIELD_SCAN_DIAG
    bool any_i3c = false;
    PRINTF("DIAG start shield datastream scan\r\n");
    DiagScanLpi2cBus("FC2", TOUCH_I2C, TOUCH_I2C_FLEXCOMM_INDEX);
    DiagScanLpi2cBus("FC3", ACCEL_I2C, ACCEL_I2C_FLEXCOMM_INDEX);

    if (BoardTempI3CInit())
    {
        PRINTF("DIAG I3C1(I2C-mode) addr:");
        for (uint8_t addr = 0x08u; addr <= 0x77u; addr++)
        {
            if (DiagI3CProbeAddrI2C(addr))
            {
                PRINTF(" 0x%02x", (unsigned int)addr);
                any_i3c = true;
            }
        }
        if (!any_i3c)
        {
            PRINTF(" none");
        }
        PRINTF("\r\n");
    }
    else
    {
        PRINTF("DIAG I3C1 init_failed\r\n");
    }
    PRINTF("DIAG done shield datastream scan\r\n");
#endif
}

static const char *ShieldImuName(uint8_t who)
{
    if (who == SHIELD_IMU_WHOAMI_LSM6DSO16IS)
    {
        return "LSM6DSO16IS";
    }
    if (who == SHIELD_IMU_WHOAMI_LSM6DSV16X)
    {
        return "LSM6DSV16X";
    }
    return "UNKNOWN";
}

static bool ShieldImuSupportsShub(uint8_t who)
{
    return (who == SHIELD_IMU_WHOAMI_LSM6DSO16IS) || (who == SHIELD_IMU_WHOAMI_LSM6DSV16X);
}

static void ShieldSensorScanLog(void)
{
#if EDGEAI_ENABLE_SHIELD_SENSOR_SCAN_LOG
    static const bool buses[2] = {true, false};
    static const char *bus_name[2] = {"FC2", "FC3"};
    static const uint8_t stts_addrs[] = {0x3Cu, 0x3Du, 0x3Eu, 0x3Fu, 0x38u};
    uint8_t who = 0u;

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        bool use_touch_bus = buses[bi];
        ShieldScanI2CAddresses(use_touch_bus, bus_name[bi]);
        if (ShieldBusRead(use_touch_bus, SHIELD_LIS2DUXS12_ADDR0, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) ||
            ShieldBusRead(use_touch_bus, SHIELD_LIS2DUXS12_ADDR1, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD probe %s LIS2DUXS12 who=0x%02x %s\r\n",
                   bus_name[bi],
                   (unsigned int)who,
                   (who == SHIELD_LIS2DUXS12_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD probe %s LIS2DUXS12 not_found\r\n", bus_name[bi]);
        }

        if (ShieldBusRead(use_touch_bus, SHIELD_LPS22DF_ADDR0, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) ||
            ShieldBusRead(use_touch_bus, SHIELD_LPS22DF_ADDR1, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD probe %s LPS22DF who=0x%02x %s\r\n",
                   bus_name[bi],
                   (unsigned int)who,
                   (who == SHIELD_LPS22DF_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD probe %s LPS22DF not_found\r\n", bus_name[bi]);
        }

        if (ShieldBusRead(use_touch_bus, SHIELD_LIS2MDL_ADDR, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD probe %s LIS2MDL who=0x%02x %s\r\n",
                   bus_name[bi],
                   (unsigned int)who,
                   (who == SHIELD_LIS2MDL_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD probe %s LIS2MDL not_found\r\n", bus_name[bi]);
        }

        if (ShieldBusRead(use_touch_bus, SHIELD_LIS2MDL_ADDR_ALT, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD probe %s LIS2MDL_ALT who=0x%02x %s\r\n",
                   bus_name[bi],
                   (unsigned int)who,
                   (who == SHIELD_LIS2MDL_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD probe %s LIS2MDL_ALT not_found\r\n", bus_name[bi]);
        }

        if (ShieldBusProbeAddress(use_touch_bus, SHIELD_SHT40_ADDR0) || ShieldBusProbeAddress(use_touch_bus, SHIELD_SHT40_ADDR1))
        {
            PRINTF("SHIELD probe %s SHT4x present\r\n", bus_name[bi]);
        }
        else
        {
            PRINTF("SHIELD probe %s SHT4x not_found\r\n", bus_name[bi]);
        }

        {
            bool stts_found = false;
            for (uint32_t i = 0u; i < (sizeof(stts_addrs) / sizeof(stts_addrs[0])); i++)
            {
                if (ShieldBusRead(use_touch_bus, stts_addrs[i], SHIELD_STTS22H_REG_WHO_AM_I, &who, 1u))
                {
                    PRINTF("SHIELD probe %s STTS22H addr=0x%02x who=0x%02x %s\r\n",
                           bus_name[bi],
                           (unsigned int)stts_addrs[i],
                           (unsigned int)who,
                           (who == SHIELD_STTS22H_WHOAMI) ? "ok" : "candidate");
                    stts_found = true;
                    break;
                }
            }
            if (!stts_found)
            {
                PRINTF("SHIELD probe %s STTS22H not_found\r\n", bus_name[bi]);
            }
        }
    }

    if (!s_shield_gyro_ready)
    {
        ShieldGyroInit();
    }
    if (s_shield_gyro_ready && ShieldImuSupportsShub(s_shield_gyro_who))
    {
        static const uint8_t stts_addrs[] = {0x3Cu, 0x3Du, 0x3Eu, 0x3Fu, 0x38u};
        uint8_t who = 0u;
        bool stts_found = false;

        if (ShieldShubReadRegs(SHIELD_LIS2MDL_ADDR, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u) ||
            ShieldShubReadRegs(SHIELD_LIS2MDL_ADDR_ALT, SHIELD_LIS2MDL_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD_SHUB probe LIS2MDL who=0x%02x %s\r\n",
                   (unsigned int)who,
                   (who == SHIELD_LIS2MDL_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD_SHUB probe LIS2MDL not_found\r\n");
        }

        if (ShieldShubReadRegs(SHIELD_LPS22DF_ADDR0, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u) ||
            ShieldShubReadRegs(SHIELD_LPS22DF_ADDR1, SHIELD_GYRO_REG_WHO_AM_I, &who, 1u))
        {
            PRINTF("SHIELD_SHUB probe LPS22DF who=0x%02x %s\r\n",
                   (unsigned int)who,
                   (who == SHIELD_LPS22DF_WHOAMI) ? "ok" : "unexpected");
        }
        else
        {
            PRINTF("SHIELD_SHUB probe LPS22DF not_found\r\n");
        }

        for (uint32_t i = 0u; i < (sizeof(stts_addrs) / sizeof(stts_addrs[0])); i++)
        {
            if (ShieldShubReadRegs(stts_addrs[i], SHIELD_STTS22H_REG_WHO_AM_I, &who, 1u))
            {
                PRINTF("SHIELD_SHUB probe STTS22H addr=0x%02x who=0x%02x %s\r\n",
                       (unsigned int)stts_addrs[i],
                       (unsigned int)who,
                       (who == SHIELD_STTS22H_WHOAMI) ? "ok" : "candidate");
                stts_found = true;
                break;
            }
        }
        if (!stts_found)
        {
            PRINTF("SHIELD_SHUB probe STTS22H not_found\r\n");
        }
    }
    else if (s_shield_gyro_ready)
    {
        PRINTF("SHIELD_SHUB probe skipped: unsupported imu who=0x%02x\r\n", (unsigned int)s_shield_gyro_who);
    }
    else
    {
        PRINTF("SHIELD_SHUB probe skipped: gyro_not_ready\r\n");
    }
#endif
}

#if EDGEAI_SENSOR_SCAN_MODE
static int SensorScanModeMain(void)
{
    uint32_t core_hz;

    BOARD_InitHardware();
    core_hz = CLOCK_GetCoreSysClkFreq();
    if (core_hz == 0u)
    {
        core_hz = CoreClockHz();
    }

    PRINTF("SENSOR_SCAN_MODE active\r\n");
    PRINTF("Tip: remove LCD, keep shield attached, then reboot this scan image.\r\n");

    for (;;)
    {
        ShieldRunDatastreamDiagnostics();
        ShieldSensorScanLog();
        SDK_DelayAtLeastUs(2000000u, core_hz);
    }

    return 0;
}
#endif

static void TouchConfigIntPin(gt911_int_pin_mode_t mode)
{
    CLOCK_EnableClock(kCLOCK_Port4);

    port_pin_config_t cfg = {
        .pullSelect = kPORT_PullDown,
        .pullValueSelect = kPORT_LowPullResistor,
        .slewRate = kPORT_FastSlewRate,
        .passiveFilterEnable = kPORT_PassiveFilterDisable,
        .openDrainEnable = kPORT_OpenDrainDisable,
        .driveStrength = kPORT_LowDriveStrength,
#if defined(FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1) && FSL_FEATURE_PORT_HAS_DRIVE_STRENGTH1
        .driveStrength1 = kPORT_NormalDriveStrength,
#endif
        .mux = kPORT_MuxAlt0,
        .inputBuffer = kPORT_InputBufferEnable,
        .invertInput = kPORT_InputNormal,
        .lockRegister = kPORT_UnlockRegister,
    };

    switch (mode)
    {
        case kGT911_IntPinPullUp:
            cfg.pullSelect = kPORT_PullUp;
            break;
        case kGT911_IntPinPullDown:
            cfg.pullSelect = kPORT_PullDown;
            break;
        case kGT911_IntPinInput:
            cfg.pullSelect = kPORT_PullDisable;
            break;
        default:
            break;
    }

    PORT_SetPinConfig(TOUCH_INT_PORT, TOUCH_INT_PIN, &cfg);
}

static void TouchConfigResetPin(bool pullUp)
{
    (void)pullUp;
}

static void TouchInit(void)
{
    gt911_config_t touch_cfg = {
        .I2C_SendFunc = TouchI2CSend,
        .I2C_ReceiveFunc = TouchI2CReceive,
        .timeDelayMsFunc = TouchDelayMs,
        .intPinFunc = TouchConfigIntPin,
        .pullResetPinFunc = TouchConfigResetPin,
        .touchPointNum = TOUCH_POINTS,
        .i2cAddrMode = kGT911_I2cAddrMode1,
        .intTrigMode = kGT911_IntFallingEdge,
    };
    status_t st;

    s_touch_ready = false;
    s_touch_was_down = false;
    s_touch_recover_backoff = 0u;

    if (!TouchI2CInit())
    {
        return;
    }

    st = GT911_Init(&s_touch_handle, &touch_cfg);
    if (st != kStatus_Success)
    {
        /* Fallback: some panels come up strapped to 0x5D. */
        touch_cfg.i2cAddrMode = kGT911_I2cAddrMode0;
        st = GT911_Init(&s_touch_handle, &touch_cfg);
    }
    if (st == kStatus_Success)
    {
        s_touch_ready = true;
        PRINTF("TOUCH ready: GT911 (%u x %u)\r\n",
               (unsigned int)s_touch_handle.resolutionX,
               (unsigned int)s_touch_handle.resolutionY);
    }
    else
    {
        PRINTF("TOUCH init failed: GT911 status=%d\r\n", (int)st);
    }
}

static void __attribute__((unused)) AccelInit(void)
{
    static const uint8_t addrs[2] = {ACCEL4_CLICK_I2C_ADDR0, ACCEL4_CLICK_I2C_ADDR1};
    uint8_t who = 0u;

    s_accel_ready = false;
    s_accel_dev.addr7 = 0u;
    s_accel_dev.write = AccelI2CWrite;
    s_accel_dev.read = AccelI2CRead;

    if (!AccelI2CInit())
    {
        GaugeRender_SetAccel(0, 0, 1000, false);
        GaugeRender_SetGyro(0, 0, 0, false);
        return;
    }

    for (uint32_t i = 0u; i < 2u; i++)
    {
        s_accel_dev.addr7 = addrs[i];
        if (fxls8974_read_whoami(&s_accel_dev, &who) && (who == FXLS8974_WHO_AM_I_VALUE))
        {
            s_accel_ready = true;
            break;
        }
    }

    if (!s_accel_ready)
    {
        PRINTF("ACCEL not found (WHO_AM_I=0x%02x)\r\n", (unsigned int)who);
        GaugeRender_SetAccel(0, 0, 1000, false);
        GaugeRender_SetGyro(0, 0, 0, false);
        return;
    }

    (void)fxls8974_set_active(&s_accel_dev, false);
    (void)fxls8974_set_fsr(&s_accel_dev, FXLS8974_FSR_4G);
    (void)fxls8974_set_active(&s_accel_dev, true);
    PRINTF("ACCEL ready addr=0x%02x\r\n", (unsigned int)s_accel_dev.addr7);
}

static void __attribute__((unused)) AccelUpdate(void)
{
    fxls8974_sample_t raw;
    int32_t x_mg;
    int32_t y_mg;
    int32_t z_mg;
    int16_t filt_x;
    int16_t filt_y;
    int16_t filt_z;

    if (!s_accel_ready)
    {
        return;
    }

    if (!fxls8974_read_sample_12b(&s_accel_dev, &raw))
    {
        return;
    }

    /* FXLS8974 raw is 12-bit signed. For +/-4g, 1 LSB ~= 1.953 mg. */
    x_mg = ((int32_t)raw.x * 125) / 64;
    y_mg = ((int32_t)raw.y * 125) / 64;
    z_mg = ((int32_t)raw.z * 125) / 64;
    s_accel_raw_x_mg = (int16_t)x_mg;
    s_accel_raw_y_mg = (int16_t)y_mg;
    s_accel_raw_z_mg = (int16_t)z_mg;

    /* Simple low-pass to keep the gyro view stable. */
    filt_x = (int16_t)((s_accel_x_mg * 3 + x_mg) / 4);
    filt_y = (int16_t)((s_accel_y_mg * 3 + y_mg) / 4);
    filt_z = (int16_t)((s_accel_z_mg * 3 + z_mg) / 4);

    /* Requested mapping change: swap X and Y axes globally. */
    s_accel_x_mg = filt_y;
    s_accel_y_mg = filt_x;
    s_accel_z_mg = filt_z;
}

static void ShieldGyroInit(void)
{
    static const bool buses[2] = {true, false};
    static const char *bus_name[2] = {"FC2", "FC3"};
    static const uint8_t addrs[2] = {SHIELD_GYRO_ADDR0, SHIELD_GYRO_ADDR1};
    uint8_t who = 0u;

    s_shield_gyro_ready = false;
    s_shield_gyro_addr = 0u;
    s_shield_gyro_who = 0u;
    s_shield_use_touch_bus = true;
    s_ui_gyro_x = 0;
    s_ui_gyro_y = 0;
    s_ui_gyro_z = 0;

    for (uint32_t bi = 0u; bi < 2u; bi++)
    {
        bool use_touch_bus = buses[bi];
        for (uint32_t i = 0u; i < 2u; i++)
        {
            if (!ShieldBusRead(use_touch_bus, addrs[i], SHIELD_GYRO_REG_WHO_AM_I, &who, 1u))
            {
                continue;
            }
            if ((who == SHIELD_IMU_WHOAMI_LSM6DSO16IS) || (who == SHIELD_IMU_WHOAMI_LSM6DSV16X))
            {
                s_shield_gyro_ready = true;
                s_shield_gyro_addr = addrs[i];
                s_shield_gyro_who = who;
                s_shield_use_touch_bus = use_touch_bus;
                PRINTF("SHIELD_GYRO bus=%s\r\n", bus_name[bi]);
                break;
            }
        }
        if (s_shield_gyro_ready)
        {
            break;
        }
    }
    if (!s_shield_gyro_ready)
    {
        if (!s_shield_gyro_missing_logged)
        {
            PRINTF("SHIELD_GYRO not found (last WHO_AM_I=0x%02x)\r\n", (unsigned int)who);
            s_shield_gyro_missing_logged = true;
        }
        return;
    }
    s_shield_gyro_missing_logged = false;
    s_shield_gyro_read_fail_logged = false;
    s_shield_gyro_read_fail_streak = 0u;

    /* LSM6-family setup: BDU + auto-increment, accel ODR=104Hz FS=4g, gyro ODR=104Hz FS=2000 dps. */
    if (!ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_CTRL3_C, 0x44u) ||
        !ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_CTRL1_XL, 0x48u) ||
        !ShieldBusWriteReg(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_CTRL2_G, 0x4Cu))
    {
        s_shield_gyro_ready = false;
        PRINTF("SHIELD_GYRO cfg failed addr=0x%02x\r\n", (unsigned int)s_shield_gyro_addr);
        return;
    }
    PRINTF("SHIELD_GYRO ready addr=0x%02x who=0x%02x (%s)\r\n",
           (unsigned int)s_shield_gyro_addr,
           (unsigned int)s_shield_gyro_who,
           ShieldImuName(s_shield_gyro_who));
}

static void ShieldGyroUpdate(void)
{
    uint8_t raw_g[6];
    uint8_t raw_a[6];
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;
    uint16_t gx_dps;
    uint16_t gy_dps;
    uint16_t gz_dps;
    int16_t gx_dps_signed;
    int16_t gy_dps_signed;
    int16_t gz_dps_signed;
    int32_t accel_mg_per_lsb_x1000;
    int32_t ax_mg;
    int32_t ay_mg;
    int32_t az_mg;
    int16_t filt_ax;
    int16_t filt_ay;
    int16_t filt_az;

    if (!s_shield_gyro_ready)
    {
        ShieldGyroInit();
        if (!s_shield_gyro_ready)
        {
            s_accel_raw_x_mg = 0;
            s_accel_raw_y_mg = 0;
            s_accel_raw_z_mg = 1000;
            GaugeRender_SetAccel(0, 0, 1000, false);
            GaugeRender_SetLinearAccel(0, 0, 1000, false);
            GaugeRender_SetGyro(0, 0, 0, false);
            s_live_gyro_x_dps = 0;
            s_live_gyro_y_dps = 0;
            s_live_gyro_z_dps = 0;
            return;
        }
    }

    if (!s_shield_gyro_ready)
    {
        s_accel_raw_x_mg = 0;
        s_accel_raw_y_mg = 0;
        s_accel_raw_z_mg = 1000;
        GaugeRender_SetAccel(0, 0, 1000, false);
        GaugeRender_SetLinearAccel(0, 0, 1000, false);
        GaugeRender_SetGyro(0, 0, 0, false);
        s_live_gyro_x_dps = 0;
        s_live_gyro_y_dps = 0;
        s_live_gyro_z_dps = 0;
        return;
    }

    /* Keep gyro read in path, but do not fail accel update if it errors. */
    if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_OUTX_L_G, raw_g, sizeof(raw_g)))
    {
        if (!s_shield_gyro_read_fail_logged)
        {
            PRINTF("SHIELD_GYRO read fail (gyro) addr=0x%02x\r\n", (unsigned int)s_shield_gyro_addr);
            s_shield_gyro_read_fail_logged = true;
        }
    }

    if (!ShieldBusRead(s_shield_use_touch_bus, s_shield_gyro_addr, SHIELD_GYRO_REG_OUTX_L_A, raw_a, sizeof(raw_a)))
    {
        if (s_shield_gyro_read_fail_streak < 255u)
        {
            s_shield_gyro_read_fail_streak++;
        }
        if (s_shield_gyro_read_fail_streak >= 5u)
        {
            s_shield_gyro_ready = false;
        }
        if (!s_shield_gyro_read_fail_logged)
        {
            PRINTF("SHIELD_GYRO read fail (accel) addr=0x%02x\r\n", (unsigned int)s_shield_gyro_addr);
            s_shield_gyro_read_fail_logged = true;
        }
        return;
    }
    s_shield_gyro_read_fail_streak = 0u;
    s_shield_gyro_read_fail_logged = false;

    gx_raw = (int16_t)(((uint16_t)raw_g[1] << 8) | raw_g[0]);
    gy_raw = (int16_t)(((uint16_t)raw_g[3] << 8) | raw_g[2]);
    gz_raw = (int16_t)(((uint16_t)raw_g[5] << 8) | raw_g[4]);
    gx_dps_signed = (int16_t)((gx_raw * 70) / 1000);
    gy_dps_signed = (int16_t)((gy_raw * 70) / 1000);
    gz_dps_signed = (int16_t)((gz_raw * 70) / 1000);
    s_live_gyro_x_dps = gx_dps_signed;
    s_live_gyro_y_dps = gy_dps_signed;
    s_live_gyro_z_dps = gz_dps_signed;
    gx_dps = (uint16_t)(((gx_raw < 0 ? -gx_raw : gx_raw) * 70) / 1000);
    gy_dps = (uint16_t)(((gy_raw < 0 ? -gy_raw : gy_raw) * 70) / 1000);
    gz_dps = (uint16_t)(((gz_raw < 0 ? -gz_raw : gz_raw) * 70) / 1000);
    s_gyro_peak_dps = gx_dps;
    if (gy_dps > s_gyro_peak_dps)
    {
        s_gyro_peak_dps = gy_dps;
    }
    if (gz_dps > s_gyro_peak_dps)
    {
        s_gyro_peak_dps = gz_dps;
    }

    ax_raw = (int16_t)(((uint16_t)raw_a[1] << 8) | raw_a[0]);
    ay_raw = (int16_t)(((uint16_t)raw_a[3] << 8) | raw_a[2]);
    az_raw = (int16_t)(((uint16_t)raw_a[5] << 8) | raw_a[4]);

    /* LSM6DSV16X reads ~2x high with 0.122 mg/LSB on this setup; use 0.061 mg/LSB. */
    accel_mg_per_lsb_x1000 = (s_shield_gyro_who == SHIELD_IMU_WHOAMI_LSM6DSV16X) ? 61 : 122;
    ax_mg = ((int32_t)ax_raw * accel_mg_per_lsb_x1000) / 1000;
    ay_mg = ((int32_t)ay_raw * accel_mg_per_lsb_x1000) / 1000;
    az_mg = ((int32_t)az_raw * accel_mg_per_lsb_x1000) / 1000;
    s_accel_raw_x_mg = (int16_t)ax_mg;
    s_accel_raw_y_mg = (int16_t)ay_mg;
    s_accel_raw_z_mg = (int16_t)az_mg;

    /* Stable absolute tilt mapping from accel only (no gyro boost, no axis swap). */
    filt_ax = (int16_t)((s_accel_x_mg * 3 + ax_mg) / 4);
    filt_ay = (int16_t)((s_accel_y_mg * 3 + ay_mg) / 4);
    filt_az = (int16_t)((s_accel_z_mg * 3 + az_mg) / 4);
    s_accel_x_mg = filt_ax;
    s_accel_y_mg = filt_ay;
    s_accel_z_mg = filt_az;

    /* UI orientation mapping: board is rotated relative to display, so swap X/Y for sphere motion. */
    s_ui_gyro_x = filt_ay;
    s_ui_gyro_y = filt_ax;
    s_ui_gyro_z = filt_az;
    GaugeRender_SetLinearAccel(s_accel_raw_x_mg, s_accel_raw_y_mg, s_accel_raw_z_mg, true);
    GaugeRender_SetAccel(s_ui_gyro_x, s_ui_gyro_y, s_ui_gyro_z, true);
    GaugeRender_SetGyro((int16_t)((gx_raw * 7) / 10),
                        (int16_t)((gy_raw * 7) / 10),
                        (int16_t)((gz_raw * 7) / 10),
                        true);
}

static bool BoardTempReadRaw(uint8_t addr, i3c_bus_type_t bus_type, uint8_t *raw2)
{
    i3c_master_transfer_t xfer;
    memset(&xfer, 0, sizeof(xfer));
    xfer.flags = kI3C_TransferDefaultFlag;
    xfer.slaveAddress = addr;
    xfer.direction = kI3C_Read;
    xfer.subaddress = BOARD_TEMP_REG;
    xfer.subaddressSize = 1u;
    xfer.data = raw2;
    xfer.dataSize = 2u;
    xfer.busType = bus_type;
    xfer.ibiResponse = kI3C_IbiRespAck;
    return (I3C_MasterTransferBlocking(TEMP_I3C, &xfer) == kStatus_Success);
}

static status_t BoardTempAssignDynamicAddress(void)
{
    i3c_master_transfer_t xfer;
    uint8_t ccc;
    uint8_t payload;

    /* RSTDAA to clear stale dynamic addresses. */
    memset(&xfer, 0, sizeof(xfer));
    ccc = I3C_CCC_RSTDAA;
    xfer.slaveAddress = I3C_BROADCAST_ADDR;
    xfer.direction = kI3C_Write;
    xfer.busType = kI3C_TypeI3CSdr;
    xfer.data = &ccc;
    xfer.dataSize = 1u;
    xfer.flags = kI3C_TransferDefaultFlag;
    if (I3C_MasterTransferBlocking(TEMP_I3C, &xfer) != kStatus_Success)
    {
        return kStatus_Fail;
    }

    /* SETDASA command phase. */
    memset(&xfer, 0, sizeof(xfer));
    ccc = I3C_CCC_SETDASA;
    xfer.slaveAddress = I3C_BROADCAST_ADDR;
    xfer.direction = kI3C_Write;
    xfer.busType = kI3C_TypeI3CSdr;
    xfer.data = &ccc;
    xfer.dataSize = 1u;
    xfer.flags = kI3C_TransferNoStopFlag;
    if (I3C_MasterTransferBlocking(TEMP_I3C, &xfer) != kStatus_Success)
    {
        return kStatus_Fail;
    }

    /* SETDASA payload phase: assign dynamic address to static 0x48 target. */
    memset(&xfer, 0, sizeof(xfer));
    payload = (uint8_t)(TEMP_SENSOR_DYNAMIC_ADDR << 1);
    xfer.slaveAddress = TEMP_SENSOR_STATIC_ADDR;
    xfer.direction = kI3C_Write;
    xfer.busType = kI3C_TypeI3CSdr;
    xfer.data = &payload;
    xfer.dataSize = 1u;
    xfer.flags = kI3C_TransferDefaultFlag;
    return I3C_MasterTransferBlocking(TEMP_I3C, &xfer);
}

static bool BoardTempDecodeC10(const uint8_t *raw, int32_t *temp_c10_out)
{
    int16_t raw12;
    int32_t temp_c10;

    if ((raw == NULL) || (temp_c10_out == NULL))
    {
        return false;
    }

    raw12 = (int16_t)((((uint16_t)raw[0] << 8) | raw[1]) >> 4);
    if ((raw12 & 0x0800) != 0)
    {
        raw12 |= (int16_t)0xF000;
    }
    /* P3T1755 is 12-bit, 0.0625C/LSB (1/16 C) => 0.625 tenth-C/LSB. */
    temp_c10 = ((int32_t)raw12 * 5) / 8;

    /* Reject clearly invalid values to avoid latching wrong targets. */
    if ((temp_c10 < -400) || (temp_c10 > 1250))
    {
        return false;
    }

    *temp_c10_out = temp_c10;
    return true;
}

static bool BoardTempDecodeC(const uint8_t *raw, int32_t *temp_c_out)
{
    int32_t temp_c10;
    if ((raw == NULL) || (temp_c_out == NULL))
    {
        return false;
    }
    if (!BoardTempDecodeC10(raw, &temp_c10))
    {
        return false;
    }
    *temp_c_out = temp_c10 / 10;
    return true;
}

static bool BoardTempProbeAddr(uint8_t addr, i3c_bus_type_t bus_type, int32_t *temp_c_out)
{
    uint8_t raw_a[2];
    uint8_t raw_b[2];
    int32_t ta;
    int32_t tb;
    int32_t dt;

    if ((temp_c_out == NULL) || !BoardTempReadRaw(addr, bus_type, raw_a))
    {
        return false;
    }
    if (!BoardTempDecodeC(raw_a, &ta))
    {
        return false;
    }
    if (!BoardTempReadRaw(addr, bus_type, raw_b) || !BoardTempDecodeC(raw_b, &tb))
    {
        *temp_c_out = ta;
        return true;
    }

    dt = ta - tb;
    if (dt < 0)
    {
        dt = -dt;
    }
    if (dt > 8)
    {
        return false;
    }

    *temp_c_out = tb;
    return true;
}

static bool BoardTempI3CInit(void)
{
    i3c_master_config_t cfg;
    uint32_t src_hz;

    if (s_temp_i3c_inited)
    {
        return true;
    }

    src_hz = CLOCK_GetI3cClkFreq(TEMP_I3C_INDEX);
    if (src_hz == 0u)
    {
        PRINTF("TEMP i3c init failed: I3C1 clock=0\r\n");
        return false;
    }

    I3C_MasterGetDefaultConfig(&cfg);
    cfg.baudRate_Hz.i2cBaud = 100000u;
    cfg.baudRate_Hz.i3cPushPullBaud = 1000000u;
    cfg.baudRate_Hz.i3cOpenDrainBaud = 125000u;
    cfg.enableOpenDrainStop = false;
    cfg.disableTimeout = true;
    I3C_MasterInit(TEMP_I3C, &cfg, src_hz);
    s_temp_i3c_inited = true;
    return true;
}

static void BoardTempInit(void)
{
    int32_t temp_c;
    int32_t temp_c10;
    uint8_t addr;
    status_t dyn_status;

    s_temp_ready = false;
    PRINTF("TEMP wiring: I3C1_PUR=P1_11 I3C1_SDA=P1_16 I3C1_SCL=P1_17\r\n");
    if (!BoardTempI3CInit())
    {
        GaugeRender_SetBoardTempC10(s_temp_c10, false);
        return;
    }

    dyn_status = BoardTempAssignDynamicAddress();
    if ((dyn_status == kStatus_Success) &&
        BoardTempProbeAddr(TEMP_SENSOR_DYNAMIC_ADDR, kI3C_TypeI3CSdr, &temp_c))
    {
        uint8_t raw[2] = {0u, 0u};
        uint16_t temp_f = (uint16_t)(((uint16_t)temp_c * 9u) / 5u + 32u);
        s_temp_addr = TEMP_SENSOR_DYNAMIC_ADDR;
        s_temp_bus_type = kI3C_TypeI3CSdr;
        s_temp_c = (uint8_t)temp_c;
        if (BoardTempReadRaw(TEMP_SENSOR_DYNAMIC_ADDR, kI3C_TypeI3CSdr, raw) &&
            BoardTempDecodeC10(raw, &temp_c10))
        {
            s_temp_c10 = (int16_t)temp_c10;
        }
        else
        {
            s_temp_c10 = (int16_t)s_temp_c * 10;
        }
        s_temp_ready = true;
        if (BoardTempReadRaw(TEMP_SENSOR_DYNAMIC_ADDR, kI3C_TypeI3CSdr, raw))
        {
            PRINTF("TEMP init raw=0x%02x 0x%02x -> %dC/%uF\r\n",
                   (unsigned int)raw[0],
                   (unsigned int)raw[1],
                   (int)s_temp_c,
                   (unsigned int)temp_f);
        }
        PRINTF("TEMP ready dyn=0x%02x bus=i3c T=%dC\r\n", (unsigned int)s_temp_addr, (int)s_temp_c);
    }

    /* Fallback to legacy I2C mode if dynamic mapping fails. */
    if (!s_temp_ready)
    {
        s_temp_bus_type = kI3C_TypeI2C;
        for (addr = 0x48u; addr <= 0x4Bu; addr++)
        {
            PRINTF("TEMP legacy probe addr=0x%02x\r\n", (unsigned int)addr);
            if (BoardTempProbeAddr(addr, kI3C_TypeI2C, &temp_c))
            {
                s_temp_addr = addr;
                s_temp_c = (uint8_t)temp_c;
                s_temp_c10 = (int16_t)s_temp_c * 10;
                s_temp_ready = true;
                break;
            }
        }
    }

    if (!s_temp_ready)
    {
        PRINTF("TEMP sensor not found on I3C1 legacy targets\r\n");
    }
    else
    {
        PRINTF("TEMP ready addr=0x%02x bus=%s T=%dC\r\n",
               (unsigned int)s_temp_addr,
               (s_temp_bus_type == kI3C_TypeI3CSdr) ? "i3c" : "i2c",
               (int)s_temp_c);
    }
    GaugeRender_SetBoardTempC10(s_temp_c10, s_temp_ready);
}

static void BoardTempUpdate(void)
{
    uint8_t raw[2];
    int32_t temp_c;
    int32_t temp_c10;

    if (!s_temp_ready)
    {
        GaugeRender_SetBoardTempC10(s_temp_c10, false);
        return;
    }
    if (!BoardTempReadRaw(s_temp_addr, s_temp_bus_type, raw))
    {
        GaugeRender_SetBoardTempC10(s_temp_c10, false);
        return;
    }
    if (!BoardTempDecodeC(raw, &temp_c) || !BoardTempDecodeC10(raw, &temp_c10))
    {
        GaugeRender_SetBoardTempC10(s_temp_c10, false);
        return;
    }
    if (temp_c < 0)
    {
        temp_c = 0;
    }
    if (temp_c > 99)
    {
        temp_c = 99;
    }
    s_temp_c = (uint8_t)temp_c;
    s_temp_c10 = (int16_t)temp_c10;
    GaugeRender_SetBoardTempC10(s_temp_c10, true);
}

static const power_sample_t *GetFrameSample(void)
{
    const power_sample_t *src = PowerData_Get();
    if (src == NULL)
    {
        return NULL;
    }
    s_frame_sample = *src;
    s_frame_sample.temp_c = s_temp_c;
    ApplyAnomalyToFrame(&s_frame_sample);
    UpdateAlertCaptureWindow(&s_frame_sample);
    return &s_frame_sample;
}

static bool TouchGetPoint(int32_t *x_out, int32_t *y_out)
{
    touch_point_t points[TOUCH_POINTS];
    uint8_t point_count = TOUCH_POINTS;
    const touch_point_t *selected = NULL;
    int32_t x;
    int32_t y;
    int32_t res_x;
    status_t st = kStatus_Fail;

    if ((x_out == NULL) || (y_out == NULL))
    {
        return false;
    }

    if (!s_touch_ready)
    {
        if (++s_touch_recover_backoff >= 200u)
        {
            s_touch_recover_backoff = 0u;
            TouchInit();
        }
        return false;
    }

    for (uint32_t attempt = 0u; attempt < 6u; attempt++)
    {
        point_count = TOUCH_POINTS;
        st = GT911_GetMultiTouch(&s_touch_handle, &point_count, points);
        if (st == kStatus_Success)
        {
            break;
        }
        if (st != kStatus_TOUCHPANEL_NotReady)
        {
            break;
        }
        SDK_DelayAtLeastUs(800u, CoreClockHz());
    }
    if (st != kStatus_Success)
    {
        if (++s_touch_recover_backoff >= 20u)
        {
            s_touch_recover_backoff = 0u;
            (void)TouchI2CRecover();
            TouchInit();
        }
        return false;
    }
    s_touch_recover_backoff = 0u;

    for (uint8_t i = 0u; i < point_count; i++)
    {
        if (points[i].valid && (points[i].touchID == 0u))
        {
            selected = &points[i];
            break;
        }
    }

    if (selected == NULL)
    {
        for (uint8_t i = 0u; i < point_count; i++)
        {
            if (points[i].valid)
            {
                selected = &points[i];
                break;
            }
        }
    }

    if (selected == NULL)
    {
        return false;
    }

    res_x = (s_touch_handle.resolutionX > 0u) ? (int32_t)s_touch_handle.resolutionX : 480;
    x = (int32_t)selected->y;
    y = res_x - (int32_t)selected->x;

    if (x < 0) x = 0;
    if (x > 479) x = 479;
    if (y < 0) y = 0;
    if (y > 319) y = 319;

    *x_out = x;
    *y_out = y;
    return true;
}

static bool TouchInRectExpanded(int32_t x, int32_t y, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t pad_x, int32_t pad_y)
{
    if (x0 > x1)
    {
        int32_t t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1)
    {
        int32_t t = y0;
        y0 = y1;
        y1 = t;
    }
    x0 -= pad_x;
    x1 += pad_x;
    y0 -= pad_y;
    y1 += pad_y;
    return (x >= x0) && (x <= x1) && (y >= y0) && (y <= y1);
}

static bool TouchInAiSet(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_AI_SET_X0, GAUGE_RENDER_AI_SET_Y0, GAUGE_RENDER_AI_SET_X1, GAUGE_RENDER_AI_SET_Y1, 14, 10);
}

static bool TouchInAiHelp(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_AI_HELP_X0, GAUGE_RENDER_AI_HELP_Y0, GAUGE_RENDER_AI_HELP_X1, GAUGE_RENDER_AI_HELP_Y1, 14, 10);
}

static bool __attribute__((unused)) TouchInSettingsPanel(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_SET_PANEL_X0, GAUGE_RENDER_SET_PANEL_Y0, GAUGE_RENDER_SET_PANEL_X1, GAUGE_RENDER_SET_PANEL_Y1, 4, 4);
}

static bool TouchInHelpPanel(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_HELP_PANEL_X0, GAUGE_RENDER_HELP_PANEL_Y0, GAUGE_RENDER_HELP_PANEL_X1, GAUGE_RENDER_HELP_PANEL_Y1, 4, 4);
}

static bool TouchInSettingsClose(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_SETTINGS_CLOSE_X0, GAUGE_RENDER_SETTINGS_CLOSE_Y0, GAUGE_RENDER_SETTINGS_CLOSE_X1, GAUGE_RENDER_SETTINGS_CLOSE_Y1, 12, 10);
}

static bool TouchInHelpClose(int32_t x, int32_t y)
{
    int32_t bx1 = GAUGE_RENDER_HELP_PANEL_X1 - 8;
    int32_t bx0 = bx1 - 22;
    int32_t by0 = GAUGE_RENDER_HELP_PANEL_Y0 + 8;
    int32_t by1 = by0 + 22;
    return TouchInRectExpanded(x, y, bx0, by0, bx1, by1, 12, 10);
}

static bool TouchInHelpNext(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_HELP_NEXT_X0, GAUGE_RENDER_HELP_NEXT_Y0, GAUGE_RENDER_HELP_NEXT_X1, GAUGE_RENDER_HELP_NEXT_Y1, 12, 10);
}

static bool __attribute__((unused)) TouchInSettingsModeIndex(int32_t x, int32_t y, uint8_t idx)
{
    int32_t x0 = GAUGE_RENDER_SET_MODE_X0 + (int32_t)idx * (GAUGE_RENDER_SET_MODE_W + GAUGE_RENDER_SET_MODE_GAP);
    int32_t x1 = x0 + GAUGE_RENDER_SET_MODE_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_MODE_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_MODE_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 8, 3);
}

static bool __attribute__((unused)) TouchInSettingsRunIndex(int32_t x, int32_t y, uint8_t idx)
{
    int32_t x0 = GAUGE_RENDER_SET_RUN_X0 + (int32_t)idx * (GAUGE_RENDER_SET_RUN_W + GAUGE_RENDER_SET_RUN_GAP);
    int32_t x1 = x0 + GAUGE_RENDER_SET_RUN_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_RUN_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_RUN_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 8, 3);
}

static bool __attribute__((unused)) TouchInSettingsTuneIndex(int32_t x, int32_t y, uint8_t idx)
{
    int32_t x0 = GAUGE_RENDER_SET_TUNE_X0 + (int32_t)idx * (GAUGE_RENDER_SET_TUNE_W + GAUGE_RENDER_SET_TUNE_GAP);
    int32_t x1 = x0 + GAUGE_RENDER_SET_TUNE_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_TUNE_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_TUNE_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 8, 3);
}

static bool __attribute__((unused)) TouchInSettingsAiIndex(int32_t x, int32_t y, uint8_t idx)
{
    int32_t x0 = GAUGE_RENDER_SET_AI_X0 + (int32_t)idx * (GAUGE_RENDER_SET_AI_W + GAUGE_RENDER_SET_AI_GAP);
    int32_t x1 = x0 + GAUGE_RENDER_SET_AI_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_AI_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_AI_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 8, 3);
}

static bool __attribute__((unused)) TouchInSettingsLimitsButton(int32_t x, int32_t y)
{
    int32_t x0 = GAUGE_RENDER_SET_LIMIT_BTN_X0;
    int32_t x1 = x0 + GAUGE_RENDER_SET_LIMIT_BTN_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_LIMIT_BTN_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_LIMIT_BTN_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 10, 8);
}

static bool __attribute__((unused)) TouchInSettingsClearButton(int32_t x, int32_t y)
{
    int32_t x0 = GAUGE_RENDER_SET_CLEAR_BTN_X0;
    int32_t x1 = x0 + GAUGE_RENDER_SET_CLEAR_BTN_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_CLEAR_BTN_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_CLEAR_BTN_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 10, 8);
}

static bool __attribute__((unused)) TouchInSettingsLogDec(int32_t x, int32_t y)
{
    int32_t x0 = GAUGE_RENDER_SET_LOG_DEC_X0;
    int32_t x1 = x0 + GAUGE_RENDER_SET_LOG_DEC_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_LOG_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_LOG_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 12, 10);
}

static bool __attribute__((unused)) TouchInSettingsLogInc(int32_t x, int32_t y)
{
    int32_t x0 = GAUGE_RENDER_SET_LOG_INC_X0;
    int32_t x1 = x0 + GAUGE_RENDER_SET_LOG_INC_W - 1;
    int32_t y0 = GAUGE_RENDER_SET_LOG_Y0;
    int32_t y1 = y0 + GAUGE_RENDER_SET_LOG_H - 1;
    return TouchInRectExpanded(x, y, x0, y0, x1, y1, 12, 10);
}

static uint8_t ClampLogRateHz(uint8_t hz)
{
    for (uint32_t i = 0u; i < (sizeof(k_log_rate_options) / sizeof(k_log_rate_options[0])); i++)
    {
        if (hz == k_log_rate_options[i])
        {
            return hz;
        }
    }
    return 10u;
}

static uint8_t __attribute__((unused)) NextLogRateHz(uint8_t hz, bool increase)
{
    uint32_t count = (uint32_t)(sizeof(k_log_rate_options) / sizeof(k_log_rate_options[0]));
    uint32_t idx = 0u;
    hz = ClampLogRateHz(hz);
    for (idx = 0u; idx < count; idx++)
    {
        if (k_log_rate_options[idx] == hz)
        {
            break;
        }
    }
    if (idx >= count)
    {
        idx = 2u; /* 10Hz default index */
    }
    if (increase)
    {
        if (idx + 1u < count)
        {
            idx++;
        }
    }
    else if (idx > 0u)
    {
        idx--;
    }
    return k_log_rate_options[idx];
}

static bool TouchInLimitsPanel(int32_t x, int32_t y)
{
    return TouchInRectExpanded(x, y, GAUGE_RENDER_LIMIT_PANEL_X0, GAUGE_RENDER_LIMIT_PANEL_Y0, GAUGE_RENDER_LIMIT_PANEL_X1, GAUGE_RENDER_LIMIT_PANEL_Y1, 4, 4);
}

static bool TouchInLimitsClose(int32_t x, int32_t y)
{
    int32_t bx1 = GAUGE_RENDER_LIMIT_PANEL_X1 - 8;
    int32_t bx0 = bx1 - 22;
    int32_t by0 = GAUGE_RENDER_LIMIT_PANEL_Y0 + 8;
    int32_t by1 = by0 + 22;
    return TouchInRectExpanded(x, y, bx0, by0, bx1, by1, 12, 10);
}

static bool TouchInLimitsAdjust(int32_t x, int32_t y, uint8_t *idx_out, bool *increase_out)
{
    for (uint8_t idx = 0u; idx < 5u; idx++)
    {
        int32_t y0 = GAUGE_RENDER_LIMIT_ROW_Y0 + (int32_t)idx * (GAUGE_RENDER_LIMIT_ROW_H + GAUGE_RENDER_LIMIT_ROW_GAP);
        int32_t y1 = y0 + GAUGE_RENDER_LIMIT_ROW_H - 1;
        int32_t minus_x0 = GAUGE_RENDER_LIMIT_MINUS_X0;
        int32_t minus_x1 = minus_x0 + GAUGE_RENDER_LIMIT_MINUS_W - 1;
        int32_t plus_x0 = GAUGE_RENDER_LIMIT_PLUS_X0;
        int32_t plus_x1 = plus_x0 + GAUGE_RENDER_LIMIT_PLUS_W - 1;

        if ((y < y0) || (y > y1))
        {
            continue;
        }

        if (TouchInRectExpanded(x, y, minus_x0, y0, minus_x1, y1, 8, 6))
        {
            *idx_out = idx;
            *increase_out = false;
            return true;
        }
        if (TouchInRectExpanded(x, y, plus_x0, y0, plus_x1, y1, 8, 6))
        {
            *idx_out = idx;
            *increase_out = true;
            return true;
        }
    }

    return false;
}

static void SaveUiSettingsIfReady(bool ext_flash_ok,
                                  anomaly_mode_t mode,
                                  anomaly_tune_t tune,
                                  bool run_live,
                                  bool ai_enabled,
                                  bool ai_backend_npu,
                                  uint16_t g_warn_mg,
                                  uint16_t g_fail_mg,
                                  int16_t temp_low_c10,
                                  int16_t temp_high_c10,
                                  uint16_t gyro_limit_dps,
                                  uint8_t log_rate_hz)
{
    if (!ext_flash_ok)
    {
        return;
    }
    if (!ExtFlashRecorder_SaveUiSettings((uint8_t)mode,
                                         (uint8_t)tune,
                                         run_live,
                                         ai_enabled,
                                         ai_backend_npu,
                                         g_warn_mg,
                                         g_fail_mg,
                                         temp_low_c10,
                                         temp_high_c10,
                                         gyro_limit_dps,
                                         log_rate_hz))
    {
        PRINTF("UI_CFG_SAVE: failed\r\n");
    }
}

static void ConfigurePlaybackWarmup(bool playback_active,
                                    bool live_mode,
                                    bool *warmup_active,
                                    bool *warmup_complete)
{
    if ((warmup_active == NULL) || (warmup_complete == NULL))
    {
        return;
    }

    if (!playback_active)
    {
        *warmup_active = false;
        GaugeRender_SetWarmupThinking(false);
        return;
    }

    if (live_mode && !(*warmup_complete))
    {
        if (!(*warmup_active))
        {
            PRINTF("AI_WARMUP: start target=%u ds speed=%ux\r\n",
                   (unsigned int)RECPLAY_WARMUP_TARGET_DS,
                   (unsigned int)RECPLAY_WARMUP_MULTIPLIER);
            PRINTF("AI_WARMUP: phases preseed=%u ds score=%u ds\r\n",
                   (unsigned int)RECPLAY_WARMUP_PRESEED_DS,
                   (unsigned int)RECPLAY_WARMUP_SCORE_DS);
            /* Start each warmup run with a clean model-score window. */
            GaugeRender_ResetPredictionMetrics();
        }
        *warmup_active = true;
        GaugeRender_SetWarmupThinking(true);
    }
    else
    {
        *warmup_active = false;
        GaugeRender_SetWarmupThinking(false);
    }
}

static bool PreloadDefaultReplayToExtFlash(bool ext_flash_ok)
{
    uint32_t i;
    uint32_t ts_ds = 0u;

    if (!ext_flash_ok)
    {
        return false;
    }

    PRINTF("EXT_FLASH_PRELOAD: start count=%u step_ds=%u\r\n",
           (unsigned int)REPLAY_TRACE_DEFAULT_LEN,
           (unsigned int)REPLAY_DEFAULT_STEP_DS);

    if (!ExtFlashRecorder_ClearAll())
    {
        PRINTF("EXT_FLASH_PRELOAD: clear_failed\r\n");
        return false;
    }

    for (i = 0u; i < REPLAY_TRACE_DEFAULT_LEN; i++)
    {
        const replay_trace_point_t *p = &kReplayTrace_Default[i];
        int16_t ax = (int16_t)((int32_t)p->current_mA / 20);
        int16_t ay = (int16_t)((int32_t)p->power_mW / 5);
        int16_t az = (int16_t)(((int32_t)p->voltage_mV - 24000) / 2);
        int16_t temp_c10 = (int16_t)((int16_t)p->temp_c * 10);
        uint16_t score = (uint16_t)((int32_t)p->soc_pct * 100 / 100);

        if (!ExtFlashRecorder_AppendSampleEx(ax,
                                             ay,
                                             az,
                                             0,
                                             0,
                                             0,
                                             temp_c10,
                                             0,
                                             0,
                                             0,
                                             10130,
                                             temp_c10,
                                             450,
                                             temp_c10,
                                             score,
                                             AI_STATUS_NORMAL,
                                             ALERT_REASON_NORMAL,
                                             ts_ds))
        {
            PRINTF("EXT_FLASH_PRELOAD: append_failed idx=%u\r\n", (unsigned int)i);
            return false;
        }
        ts_ds += REPLAY_DEFAULT_STEP_DS;
    }

    PRINTF("EXT_FLASH_PRELOAD: done count=%u duration_ds=%u\r\n",
           (unsigned int)REPLAY_TRACE_DEFAULT_LEN,
           (unsigned int)((REPLAY_TRACE_DEFAULT_LEN > 0u) ? ((REPLAY_TRACE_DEFAULT_LEN - 1u) * REPLAY_DEFAULT_STEP_DS) : 0u));
    return true;
}

static void ApplyLimitAdjustment(uint8_t lim_idx, bool increase)
{
    switch (lim_idx)
    {
        case 0u: /* G warn */
            if (increase)
            {
                if (s_limit_g_warn_mg < 15000u)
                {
                    s_limit_g_warn_mg += 500u;
                }
            }
            else if (s_limit_g_warn_mg > 2000u)
            {
                s_limit_g_warn_mg -= 500u;
            }
            if (s_limit_g_fail_mg <= s_limit_g_warn_mg)
            {
                s_limit_g_fail_mg = (uint16_t)(s_limit_g_warn_mg + 500u);
            }
            break;
        case 1u: /* G fail */
            if (increase)
            {
                if (s_limit_g_fail_mg < 16000u)
                {
                    s_limit_g_fail_mg += 500u;
                }
            }
            else if (s_limit_g_fail_mg > 3000u)
            {
                s_limit_g_fail_mg -= 500u;
            }
            if (s_limit_g_fail_mg <= s_limit_g_warn_mg)
            {
                s_limit_g_warn_mg = (uint16_t)(s_limit_g_fail_mg - 500u);
            }
            break;
        case 2u: /* Temp low */
            if (increase)
            {
                if (s_limit_temp_lo_c10 < 300)
                {
                    s_limit_temp_lo_c10 += 50;
                }
            }
            else if (s_limit_temp_lo_c10 > -200)
            {
                s_limit_temp_lo_c10 -= 50;
            }
            if (s_limit_temp_lo_c10 >= s_limit_temp_hi_c10)
            {
                s_limit_temp_hi_c10 = s_limit_temp_lo_c10 + 50;
            }
            break;
        case 3u: /* Temp high */
            if (increase)
            {
                if (s_limit_temp_hi_c10 < 1000)
                {
                    s_limit_temp_hi_c10 += 50;
                }
            }
            else if (s_limit_temp_hi_c10 > (s_limit_temp_lo_c10 + 50))
            {
                s_limit_temp_hi_c10 -= 50;
            }
            if (s_limit_temp_hi_c10 <= s_limit_temp_lo_c10)
            {
                s_limit_temp_hi_c10 = s_limit_temp_lo_c10 + 50;
            }
            break;
        default: /* Gyro */
            if (increase)
            {
                if (s_limit_gyro_dps < 2000u)
                {
                    s_limit_gyro_dps += 100u;
                }
            }
            else if (s_limit_gyro_dps > 100u)
            {
                s_limit_gyro_dps -= 100u;
            }
            break;
    }

    GaugeRender_SetLimitInfo(s_limit_g_warn_mg, s_limit_g_fail_mg, s_limit_temp_lo_c10, s_limit_temp_hi_c10, s_limit_gyro_dps);
}

int main(void)
{
#if EDGEAI_SENSOR_SCAN_MODE
    return SensorScanModeMain();
#else
    bool ai_enabled = true;
    bool ai_use_npu = true;
    bool lcd_ok;
    bool ext_flash_ok;
    bool help_visible = false;
    bool settings_visible = false;
    bool limits_visible = false;
    bool ui_block_touch = false;
    bool train_armed_idle = false;
    bool record_mode;
    bool prev_record_mode;
    bool prev_trained_ready;
    bool ui_settings_dirty = false;
    bool playback_active = false;
    bool playback_warmup_active = false;
    bool playback_warmup_complete = false;
    bool playback_run_complete = false;
    bool playback_prev_ts_valid = false;
    bool playback_prev_src_ts_valid = false;
    uint32_t playback_prev_src_ts_ds = 0u;
    uint32_t playback_prev_ts_ds = 0u;
    uint32_t playback_period_us = RECPLAY_TICK_PERIOD_US;
    uint32_t playback_rt_anchor_ds = 0u;
    uint64_t playback_rt_anchor_ticks = 0u;
    uint32_t playback_cgm_index = 0u;
    uint8_t playback_read_fail_streak = 0u;
    ext_flash_sample_t playback_sample;
    uint32_t data_tick_accum_us = 0u;
    uint32_t recplay_tick_accum_us = 0u;
    uint32_t render_tick_accum_us = 0u;
    uint32_t accel_sample_tick_accum_us = 0u;
    uint32_t gyro_tick_accum_us = 0u;
    uint32_t accel_live_tick_accum_us = 0u;
    uint32_t runtime_clock_tick_accum_us = 0u;
    uint32_t log_tick_accum_us = 0u;
    uint32_t temp_tick_accum_us = 0u;
    uint32_t shield_aux_tick_accum_us = 0u;
    uint32_t accel_test_tick_accum_us = 0u;
    uint32_t runtime_elapsed_ds = 0u;
    uint32_t runtime_displayed_ds = UINT32_MAX;
    uint32_t rec_elapsed_ds = 0u;
    uint32_t play_off = 0u;
    uint32_t play_cnt = 0u;
    uint32_t rec_cnt = 0u;
    const power_sample_t *sample;
    anomaly_mode_t anom_mode;
    anomaly_tune_t anom_tune;
    uint8_t saved_mode = 0u;
    uint8_t saved_tune = 0u;
    bool saved_run_live = true;
    bool saved_ai = true;
    bool saved_ai_backend_npu = true;
    uint16_t saved_g_warn_mg = 12000u;
    uint16_t saved_g_fail_mg = 15000u;
    int16_t saved_temp_lo_c10 = 0;
    int16_t saved_temp_hi_c10 = 700;
    uint16_t saved_gyro_limit_dps = 500u;
    uint8_t saved_log_rate_hz = 10u;
    bool saved_valid = false;
    uint64_t time_prev_ticks = 0u;
    uint64_t time_us_rem = 0u;
    uint64_t runtime_clock_start_ticks = 0u;

    BOARD_InitHardware();
    ext_flash_ok = ExtFlashRecorder_Init();
    PRINTF("EXT_FLASH_REC: %s\r\n", ext_flash_ok ? "ready" : "init_failed");

    lcd_ok = GaugeRender_Init();
    PRINTF("EV dash LCD: %s\r\n", lcd_ok ? "ready" : "init_failed");
    GaugeRender_SetProfileInfo(EilProfile_GetModelName(), EilProfile_GetModelVersion(), EilProfile_GetExtensionVersion());

    PowerData_Init();
    AnomalyEngine_Init();
    if (ext_flash_ok &&
        ExtFlashRecorder_GetUiSettings(&saved_mode,
                                       &saved_tune,
                                       &saved_run_live,
                                       &saved_ai,
                                       &saved_ai_backend_npu,
                                       &saved_g_warn_mg,
                                       &saved_g_fail_mg,
                                       &saved_temp_lo_c10,
                                       &saved_temp_hi_c10,
                                       &saved_gyro_limit_dps,
                                       &saved_log_rate_hz,
                                       &saved_valid) &&
        saved_valid)
    {
        if (saved_mode > (uint8_t)ANOMALY_MODE_TRAINED_MONITOR)
        {
            saved_mode = (uint8_t)ANOMALY_MODE_ADAPTIVE_BASELINE;
        }
        if (saved_tune > (uint8_t)ANOMALY_TUNE_STRICT)
        {
            saved_tune = (uint8_t)ANOMALY_TUNE_NORMAL;
        }
        if (saved_g_warn_mg < 500u)
        {
            saved_g_warn_mg = 500u;
        }
        if (saved_g_warn_mg > 15000u)
        {
            saved_g_warn_mg = 15000u;
        }
        if (saved_g_fail_mg < 1000u)
        {
            saved_g_fail_mg = 1000u;
        }
        if (saved_g_fail_mg > 16000u)
        {
            saved_g_fail_mg = 16000u;
        }
        if (saved_g_fail_mg <= saved_g_warn_mg)
        {
            saved_g_fail_mg = saved_g_warn_mg + 500u;
            if (saved_g_fail_mg > 16000u)
            {
                saved_g_fail_mg = 16000u;
            }
        }
        if (saved_temp_lo_c10 < -200)
        {
            saved_temp_lo_c10 = -200;
        }
        if (saved_temp_hi_c10 > 1000)
        {
            saved_temp_hi_c10 = 1000;
        }
        if (saved_temp_hi_c10 <= saved_temp_lo_c10)
        {
            saved_temp_hi_c10 = saved_temp_lo_c10 + 50;
        }
        if (saved_gyro_limit_dps < 100u)
        {
            saved_gyro_limit_dps = 100u;
        }
        if (saved_gyro_limit_dps > 2000u)
        {
            saved_gyro_limit_dps = 2000u;
        }
        saved_log_rate_hz = ClampLogRateHz(saved_log_rate_hz);
        /* Force default AI ON at boot even if previous persisted setting was OFF. */
        ai_enabled = true;
        ai_use_npu = saved_ai_backend_npu;
        s_limit_g_warn_mg = saved_g_warn_mg;
        s_limit_g_fail_mg = saved_g_fail_mg;
        s_limit_temp_lo_c10 = saved_temp_lo_c10;
        s_limit_temp_hi_c10 = saved_temp_hi_c10;
        s_limit_gyro_dps = saved_gyro_limit_dps;
        s_log_rate_hz = saved_log_rate_hz;
        AnomalyEngine_SetMode((anomaly_mode_t)saved_mode);
        AnomalyEngine_SetTune((anomaly_tune_t)saved_tune);
        GaugeRender_SetLiveBannerMode(saved_run_live);
        PRINTF("UI_CFG_LOAD: mode=%u tune=%u run=%u ai=%u backend=%s gw=%u gf=%u tl=%d th=%d gy=%u log=%uHz\r\n",
               (unsigned int)saved_mode,
               (unsigned int)saved_tune,
               saved_run_live ? 1u : 0u,
               ai_enabled ? 1u : 0u,
               saved_ai_backend_npu ? "NPU" : "MCU",
               (unsigned int)s_limit_g_warn_mg,
               (unsigned int)s_limit_g_fail_mg,
               (int)(s_limit_temp_lo_c10 / 10),
               (int)(s_limit_temp_hi_c10 / 10),
               (unsigned int)s_limit_gyro_dps,
               (unsigned int)s_log_rate_hz);
    }
    GaugeRender_SetLogRateHz(s_log_rate_hz);
    PowerData_SetAiAssistEnabled(ai_enabled);
    anom_mode = AnomalyEngine_GetMode();
    anom_tune = AnomalyEngine_GetTune();

    if (ext_flash_ok)
    {
        bool preloaded = PreloadDefaultReplayToExtFlash(ext_flash_ok);
        if (preloaded)
        {
            SaveUiSettingsIfReady(ext_flash_ok,
                                  anom_mode,
                                  anom_tune,
                                  GaugeRender_IsLiveBannerMode(),
                                  ai_enabled,
                                  ai_use_npu,
                                  s_limit_g_warn_mg,
                                  s_limit_g_fail_mg,
                                  s_limit_temp_lo_c10,
                                  s_limit_temp_hi_c10,
                                  s_limit_gyro_dps,
                                  s_log_rate_hz);
        }
    }

    ShieldGyroInit();
    ShieldRunDatastreamDiagnostics();
    ShieldSensorScanLog();
    TouchInit();
    BoardTempInit();
    BoardTempUpdate();
    ShieldGyroUpdate();
    ShieldAuxInit();
    (void)TimebaseInit();
    time_prev_ticks = TimebaseNowTicks();
    runtime_clock_start_ticks = time_prev_ticks;
    GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
    GaugeRender_SetHelpVisible(false);
    GaugeRender_SetSettingsVisible(false);
    GaugeRender_SetLimitsVisible(false);
    GaugeRender_SetWarmupThinking(false);
    GaugeRender_SetLiveBannerMode(saved_valid ? saved_run_live : true);
    GaugeRender_SetAiBackendNpu(ai_use_npu);
    GaugeRender_SetLimitInfo(s_limit_g_warn_mg,
                             s_limit_g_fail_mg,
                             s_limit_temp_lo_c10,
                             s_limit_temp_hi_c10,
                             s_limit_gyro_dps);
    GaugeRender_SetRecordMode(false);
    if (GaugeRender_IsLiveBannerMode())
    {
        /* Boot into preroll visual mode: background-only until warmup handoff logic finalizes. */
        GaugeRender_SetWarmupThinking(true);
    }
    memset(&s_anom_out, 0, sizeof(s_anom_out));
    AnomalyEngine_GetOutput(&s_anom_out);
    GaugeRender_SetAnomalyInfo((uint8_t)s_anom_out.mode,
                               (uint8_t)s_anom_out.tune,
                               s_anom_out.training_active,
                               s_anom_out.trained_ready,
                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AX],
                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AY],
                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AZ],
                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_TEMP],
                               (uint8_t)s_anom_out.overall_level);
    sample = GetFrameSample();
    if (lcd_ok && (sample != NULL))
    {
        GaugeRender_DrawFrame(sample, ai_enabled, PowerData_GetReplayProfile());
    }
    prev_record_mode = GaugeRender_IsRecordMode();
    prev_trained_ready = s_anom_out.trained_ready;
    if (!prev_record_mode)
    {
        playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
        playback_cgm_index = 0u;
        playback_run_complete = false;
        ConfigurePlaybackWarmup(playback_active, GaugeRender_IsLiveBannerMode(), &playback_warmup_active, &playback_warmup_complete);
        playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
        playback_period_us = RECPLAY_TICK_PERIOD_US;
        runtime_elapsed_ds = 0u;
        runtime_displayed_ds = UINT32_MAX;
        runtime_clock_start_ticks = TimebaseNowTicks();
        GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
        PRINTF("EXT_FLASH_PLAY: %s\r\n", playback_active ? "ready" : "no_data");
        if (playback_active && ExtFlashRecorder_GetPlaybackInfo(&play_off, &play_cnt))
        {
            PRINTF("EXT_FLASH_PLAY_INFO: offset=%u count=%u\r\n", (unsigned int)play_off, (unsigned int)play_cnt);
        }
    }

    PRINTF("EV dash app ready\r\n");

    for (;;)
    {
        int32_t tx = 0;
        int32_t ty = 0;
        uint32_t recplay_hz = (uint32_t)ClampLogRateHz(s_log_rate_hz);
        uint32_t recplay_period_us = RECPLAY_TICK_PERIOD_US;
        bool modal_active;
        bool modal_active_now;
        bool pressed = TouchGetPoint(&tx, &ty);
        bool in_set;
        bool in_help;
        bool opened_modal_this_touch = false;
        bool modal_overlay_visible = (help_visible || settings_visible || limits_visible);
        bool touch_press_edge;
        bool timeline_changed = GaugeRender_HandleTouch(tx, ty, pressed);
        bool record_start_request;
        bool record_stop_request;
        bool clear_flash_request;
        if (recplay_hz > 0u)
        {
            recplay_period_us = 1000000u / recplay_hz;
            if (recplay_period_us == 0u)
            {
                recplay_period_us = RECPLAY_TICK_PERIOD_US;
            }
        }
        if (playback_active && GaugeRender_IsLiveBannerMode() && !playback_warmup_active)
        {
            recplay_period_us = playback_period_us;
            if (recplay_period_us == 0u)
            {
                recplay_period_us = RECPLAY_TICK_PERIOD_US;
            }
        }

        modal_active = GaugeRender_IsRecordConfirmActive();
        in_set = pressed && !modal_active && TouchInAiSet(tx, ty);
        in_help = pressed && !modal_active && TouchInAiHelp(tx, ty);

        if (help_visible || settings_visible || limits_visible)
        {
            ui_block_touch = false;
        }

        if (ui_block_touch)
        {
            if (!pressed)
            {
                ui_block_touch = false;
            }
            in_set = false;
            in_help = false;
        }

        touch_press_edge = pressed && !s_touch_was_down;

        /* Immediate open path for top-row modal buttons.
         * Avoid depending on edge-trigger state that can miss taps on some touch scans. */
        if (pressed && !modal_active && !help_visible && !settings_visible && !limits_visible)
        {
            bool opened_modal = false;
            if (in_set)
            {
                settings_visible = true;
                help_visible = false;
                limits_visible = false;
                GaugeRender_SetHelpVisible(false);
                GaugeRender_SetLimitsVisible(false);
                GaugeRender_SetSettingsVisible(true);
                opened_modal = true;
                opened_modal_this_touch = true;
            }
            else if (in_help)
            {
                help_visible = true;
                settings_visible = false;
                limits_visible = false;
                GaugeRender_SetSettingsVisible(false);
                GaugeRender_SetLimitsVisible(false);
                GaugeRender_SetHelpPage(0u);
                GaugeRender_SetHelpVisible(true);
                opened_modal = true;
                opened_modal_this_touch = true;
                PRINTF("UI_HELP,ON\r\n");
            }
            if (opened_modal && lcd_ok)
            {
                GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
            }
        }

        if (pressed &&
            !opened_modal_this_touch &&
            !modal_active &&
            (!ui_block_touch || modal_overlay_visible) &&
            touch_press_edge)
        {
            bool redraw_ui = false;

            if (help_visible)
            {
                if (TouchInHelpClose(tx, ty))
                {
                    help_visible = false;
                    GaugeRender_SetHelpVisible(false);
                    redraw_ui = true;
                }
                else if (TouchInHelpNext(tx, ty) || in_help)
                {
                    GaugeRender_NextHelpPage();
                    redraw_ui = true;
                }
                else if (!TouchInHelpPanel(tx, ty))
                {
                    help_visible = false;
                    GaugeRender_SetHelpVisible(false);
                    redraw_ui = true;
                }
            }
            else if (limits_visible)
            {
                bool handled_limits = false;
                uint8_t lim_idx = 0u;
                bool increase = false;

                if (TouchInLimitsClose(tx, ty))
                {
                    limits_visible = false;
                    GaugeRender_SetLimitsVisible(false);
                    redraw_ui = true;
                    handled_limits = true;
                }
                else if (TouchInLimitsAdjust(tx, ty, &lim_idx, &increase))
                {
                    ApplyLimitAdjustment(lim_idx, increase);
                    ui_settings_dirty = true;
                    redraw_ui = true;
                    handled_limits = true;
                }

                if (!handled_limits && (in_set || !TouchInLimitsPanel(tx, ty)))
                {
                    limits_visible = false;
                    GaugeRender_SetLimitsVisible(false);
                    redraw_ui = true;
                }
            }
            else if (settings_visible)
            {
                bool settings_changed = false;
                if (TouchInSettingsClose(tx, ty))
                {
                    settings_visible = false;
                    GaugeRender_SetSettingsVisible(false);
                    redraw_ui = true;
                }
                else if (TouchInSettingsModeIndex(tx, ty, 0u))
                {
                    ai_enabled = false;
                    PowerData_SetAiAssistEnabled(ai_enabled);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsModeIndex(tx, ty, 1u))
                {
                    ai_enabled = true;
                    PowerData_SetAiAssistEnabled(ai_enabled);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsRunIndex(tx, ty, 0u))
                {
                    GaugeRender_SetLiveBannerMode(false);
                    ConfigurePlaybackWarmup(playback_active,
                                            GaugeRender_IsLiveBannerMode(),
                                            &playback_warmup_active,
                                            &playback_warmup_complete);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsRunIndex(tx, ty, 1u))
                {
                    GaugeRender_SetLiveBannerMode(true);
                    ConfigurePlaybackWarmup(playback_active,
                                            GaugeRender_IsLiveBannerMode(),
                                            &playback_warmup_active,
                                            &playback_warmup_complete);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsTuneIndex(tx, ty, 0u))
                {
                    AnomalyEngine_SetTune(ANOMALY_TUNE_LOOSE);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsTuneIndex(tx, ty, 1u))
                {
                    AnomalyEngine_SetTune(ANOMALY_TUNE_NORMAL);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsTuneIndex(tx, ty, 2u))
                {
                    AnomalyEngine_SetTune(ANOMALY_TUNE_STRICT);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsAiIndex(tx, ty, 0u))
                {
                    ai_use_npu = false;
                    GaugeRender_SetAiBackendNpu(ai_use_npu);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsAiIndex(tx, ty, 1u))
                {
                    ai_use_npu = true;
                    GaugeRender_SetAiBackendNpu(ai_use_npu);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsLimitsButton(tx, ty))
                {
                    settings_visible = false;
                    limits_visible = true;
                    GaugeRender_SetSettingsVisible(false);
                    GaugeRender_SetLimitsVisible(true);
                    redraw_ui = true;
                }
                else if (TouchInSettingsClearButton(tx, ty))
                {
                    GaugeRender_RequestClearFlashConfirm();
                    redraw_ui = true;
                }
                else if (TouchInSettingsLogDec(tx, ty))
                {
                    s_log_rate_hz = NextLogRateHz(s_log_rate_hz, false);
                    GaugeRender_SetLogRateHz(s_log_rate_hz);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (TouchInSettingsLogInc(tx, ty))
                {
                    s_log_rate_hz = NextLogRateHz(s_log_rate_hz, true);
                    GaugeRender_SetLogRateHz(s_log_rate_hz);
                    settings_changed = true;
                    redraw_ui = true;
                }
                else if (!TouchInSettingsPanel(tx, ty))
                {
                    settings_visible = false;
                    GaugeRender_SetSettingsVisible(false);
                    redraw_ui = true;
                }

                if (settings_changed)
                {
                    GaugeRender_RequestModalRedraw();
                    ui_settings_dirty = true;
                }

                if (redraw_ui)
                {
                    AnomalyEngine_GetOutput(&s_anom_out);
                    GaugeRender_SetAnomalyInfo((uint8_t)s_anom_out.mode,
                                               (uint8_t)s_anom_out.tune,
                                               s_anom_out.training_active,
                                               s_anom_out.trained_ready,
                                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AX],
                                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AY],
                                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AZ],
                                               (uint8_t)s_anom_out.channel_level[ANOMALY_CH_TEMP],
                                               (uint8_t)s_anom_out.overall_level);
                    if (lcd_ok)
                    {
                        GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
                    }
                }
            }
            else if (in_set)
            {
                settings_visible = true;
                help_visible = false;
                limits_visible = false;
                GaugeRender_SetHelpVisible(false);
                GaugeRender_SetLimitsVisible(false);
                GaugeRender_SetSettingsVisible(true);
                redraw_ui = true;
            }
            else if (in_help)
            {
                help_visible = true;
                settings_visible = false;
                limits_visible = false;
                GaugeRender_SetSettingsVisible(false);
                GaugeRender_SetLimitsVisible(false);
                GaugeRender_SetHelpPage(0u);
                GaugeRender_SetHelpVisible(true);
                redraw_ui = true;
                PRINTF("UI_HELP,ON\r\n");
            }

            if (redraw_ui && lcd_ok)
            {
                GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
            }
        }

        if (timeline_changed && lcd_ok)
        {
            GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
        }
        if (timeline_changed && !GaugeRender_IsRecordMode() && !GaugeRender_IsRecordConfirmActive() && !train_armed_idle)
        {
            playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
            playback_cgm_index = 0u;
            playback_run_complete = false;
            ConfigurePlaybackWarmup(playback_active,
                                    GaugeRender_IsLiveBannerMode(),
                                    &playback_warmup_active,
                                    &playback_warmup_complete);
            playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
            playback_period_us = RECPLAY_TICK_PERIOD_US;
            runtime_elapsed_ds = 0u;
            runtime_displayed_ds = UINT32_MAX;
            runtime_clock_start_ticks = TimebaseNowTicks();
            GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
            PRINTF("EXT_FLASH_PLAY: %s\r\n", playback_active ? "restart" : "no_data");
            if (playback_active && ExtFlashRecorder_GetPlaybackInfo(&play_off, &play_cnt))
            {
                PRINTF("EXT_FLASH_PLAY_INFO: offset=%u count=%u\r\n", (unsigned int)play_off, (unsigned int)play_cnt);
            }
        }
        record_start_request = GaugeRender_ConsumeRecordStartRequest();
        if (record_start_request)
        {
            bool cleared = ext_flash_ok && ExtFlashRecorder_ClearAll();
            if (cleared)
            {
                GaugeRender_SetLiveBannerMode(false);
                GaugeRender_SetRecordMode(true);
                playback_active = false;
                playback_run_complete = false;
                ConfigurePlaybackWarmup(false, false, &playback_warmup_active, &playback_warmup_complete);
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = RECPLAY_TICK_PERIOD_US;
                train_armed_idle = false;
                runtime_elapsed_ds = 0u;
                rec_elapsed_ds = 0u;
                ResetSignalPeakWindows();
                runtime_displayed_ds = UINT32_MAX;
                runtime_clock_start_ticks = TimebaseNowTicks();
                GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                if (anom_mode == ANOMALY_MODE_TRAINED_MONITOR)
                {
                    AnomalyEngine_StartTraining();
                }
                PRINTF("EXT_FLASH_REC: cleared_start\r\n");
                GaugeRender_SetPlayhead(99u, true);
            }
            else
            {
                GaugeRender_SetRecordMode(false);
                playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
                playback_cgm_index = 0u;
                playback_run_complete = false;
                ConfigurePlaybackWarmup(playback_active,
                                        GaugeRender_IsLiveBannerMode(),
                                        &playback_warmup_active,
                                        &playback_warmup_complete);
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = RECPLAY_TICK_PERIOD_US;
                train_armed_idle = (anom_mode == ANOMALY_MODE_TRAINED_MONITOR);
                runtime_elapsed_ds = 0u;
                ResetSignalPeakWindows();
                runtime_displayed_ds = UINT32_MAX;
                runtime_clock_start_ticks = TimebaseNowTicks();
                GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                PRINTF("EXT_FLASH_REC: clear_failed\r\n");
            }
            if (lcd_ok)
            {
                GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
            }
        }
        record_stop_request = GaugeRender_ConsumeRecordStopRequest();
        if (record_stop_request)
        {
            GaugeRender_SetRecordMode(false);
            train_armed_idle = (anom_mode == ANOMALY_MODE_TRAINED_MONITOR) && !GaugeRender_IsLiveBannerMode();
            runtime_elapsed_ds = 0u;
            runtime_displayed_ds = UINT32_MAX;
            runtime_clock_start_ticks = TimebaseNowTicks();
            GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
            PRINTF("EXT_FLASH_REC: stop_confirmed\r\n");
            if (lcd_ok)
            {
                GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
            }
        }
        clear_flash_request = GaugeRender_ConsumeClearFlashRequest();
        if (clear_flash_request)
        {
            bool cleared = ext_flash_ok && ExtFlashRecorder_ClearAll();
            playback_active = false;
            ConfigurePlaybackWarmup(false, false, &playback_warmup_active, &playback_warmup_complete);
            playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
            playback_period_us = RECPLAY_TICK_PERIOD_US;
            GaugeRender_SetPlayhead(99u, false);
            runtime_elapsed_ds = 0u;
            rec_elapsed_ds = 0u;
            runtime_displayed_ds = UINT32_MAX;
            runtime_clock_start_ticks = TimebaseNowTicks();
            GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
            PRINTF("EXT_FLASH_MANUAL_CLEAR: %s\r\n", cleared ? "ok" : "failed");
            if (lcd_ok)
            {
                GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
            }
        }
        if (ui_settings_dirty && !help_visible && !settings_visible && !limits_visible)
        {
            SaveUiSettingsIfReady(ext_flash_ok,
                                  anom_mode,
                                  AnomalyEngine_GetTune(),
                                  GaugeRender_IsLiveBannerMode(),
                                  ai_enabled,
                                  ai_use_npu,
                                  s_limit_g_warn_mg,
                                  s_limit_g_fail_mg,
                                  s_limit_temp_lo_c10,
                                  s_limit_temp_hi_c10,
                                  s_limit_gyro_dps,
                                  s_log_rate_hz);
            ui_settings_dirty = false;
        }
        s_touch_was_down = pressed;
        modal_active_now = GaugeRender_IsRecordConfirmActive() || help_visible || settings_visible || limits_visible;
        record_mode = GaugeRender_IsRecordMode();
        if (record_mode != prev_record_mode)
        {
            if (!record_mode)
            {
                if (anom_mode == ANOMALY_MODE_TRAINED_MONITOR)
                {
                    if (s_anom_out.trained_ready)
                    {
                        AnomalyEngine_StopTraining();
                    }
                }
                playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
                playback_cgm_index = 0u;
                playback_run_complete = false;
                ConfigurePlaybackWarmup(playback_active,
                                        GaugeRender_IsLiveBannerMode(),
                                        &playback_warmup_active,
                                        &playback_warmup_complete);
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = RECPLAY_TICK_PERIOD_US;
                runtime_elapsed_ds = 0u;
                runtime_displayed_ds = UINT32_MAX;
                runtime_clock_start_ticks = TimebaseNowTicks();
                GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                PRINTF("EXT_FLASH_PLAY: %s\r\n", playback_active ? "ready" : "no_data");
                if (playback_active && ExtFlashRecorder_GetPlaybackInfo(&play_off, &play_cnt))
                {
                    PRINTF("EXT_FLASH_PLAY_INFO: offset=%u count=%u\r\n", (unsigned int)play_off, (unsigned int)play_cnt);
                }
            }
            else
            {
                GaugeRender_SetLiveBannerMode(false);
                if (anom_mode == ANOMALY_MODE_TRAINED_MONITOR)
                {
                    AnomalyEngine_StartTraining();
                }
                playback_active = false;
                playback_run_complete = false;
                ConfigurePlaybackWarmup(false, false, &playback_warmup_active, &playback_warmup_complete);
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = RECPLAY_TICK_PERIOD_US;
                runtime_elapsed_ds = 0u;
                rec_elapsed_ds = 0u;
                ResetSignalPeakWindows();
                runtime_displayed_ds = UINT32_MAX;
                runtime_clock_start_ticks = TimebaseNowTicks();
                GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                PRINTF("EXT_FLASH_REC: active\r\n");
                if (ext_flash_ok && ExtFlashRecorder_GetRecordInfo(&rec_cnt))
                {
                    PRINTF("EXT_FLASH_REC_INFO: count=%u\r\n", (unsigned int)rec_cnt);
                }
                GaugeRender_SetPlayhead(99u, true);
            }
            prev_record_mode = record_mode;
        }

        {
            uint32_t elapsed_loop_us;
            if (s_timebase_ready)
            {
                uint64_t now_ticks = TimebaseNowTicks();
                uint64_t dt_ticks = now_ticks - time_prev_ticks;
                uint64_t us_num = (dt_ticks * 1000000ull) + time_us_rem;
                time_prev_ticks = now_ticks;
                elapsed_loop_us = (uint32_t)(us_num / s_timebase_hz);
                time_us_rem = us_num % s_timebase_hz;
            }
            else
            {
                elapsed_loop_us = TOUCH_POLL_DELAY_US;
            }

            if (elapsed_loop_us == 0u)
            {
                elapsed_loop_us = TOUCH_POLL_DELAY_US;
            }
            if (elapsed_loop_us > 250000u)
            {
                elapsed_loop_us = 250000u;
            }

            runtime_clock_tick_accum_us += elapsed_loop_us;
            data_tick_accum_us += elapsed_loop_us;
            recplay_tick_accum_us += elapsed_loop_us;
            render_tick_accum_us += elapsed_loop_us;
            accel_sample_tick_accum_us += elapsed_loop_us;
            gyro_tick_accum_us += elapsed_loop_us;
            accel_live_tick_accum_us += elapsed_loop_us;
            temp_tick_accum_us += elapsed_loop_us;
            shield_aux_tick_accum_us += elapsed_loop_us;
            accel_test_tick_accum_us += elapsed_loop_us;
            log_tick_accum_us += elapsed_loop_us;

            if (data_tick_accum_us > (POWER_TICK_PERIOD_US * 2u))
            {
                data_tick_accum_us = POWER_TICK_PERIOD_US * 2u;
            }
            if (recplay_tick_accum_us > (recplay_period_us * 2u))
            {
                recplay_tick_accum_us = recplay_period_us * 2u;
            }
            if (render_tick_accum_us > (DISPLAY_REFRESH_PERIOD_US * 2u))
            {
                render_tick_accum_us = DISPLAY_REFRESH_PERIOD_US * 2u;
            }
            if (accel_sample_tick_accum_us > (ACCEL_BUFFER_SAMPLE_PERIOD_US * 4u))
            {
                accel_sample_tick_accum_us = ACCEL_BUFFER_SAMPLE_PERIOD_US * 4u;
            }
            if (temp_tick_accum_us > (TEMP_REFRESH_PERIOD_US * 2u))
            {
                temp_tick_accum_us = TEMP_REFRESH_PERIOD_US * 2u;
            }
            if (shield_aux_tick_accum_us > (SHIELD_AUX_POLL_PERIOD_US * 2u))
            {
                shield_aux_tick_accum_us = SHIELD_AUX_POLL_PERIOD_US * 2u;
            }
            if (accel_test_tick_accum_us > (ACCEL_TEST_LOG_PERIOD_US * 2u))
            {
                accel_test_tick_accum_us = ACCEL_TEST_LOG_PERIOD_US * 2u;
            }
            if (log_tick_accum_us > 2000000u)
            {
                log_tick_accum_us = 2000000u;
            }
        }

        while (accel_sample_tick_accum_us >= ACCEL_BUFFER_SAMPLE_PERIOD_US)
        {
            accel_sample_tick_accum_us -= ACCEL_BUFFER_SAMPLE_PERIOD_US;
            if (!train_armed_idle)
            {
                ShieldGyroUpdate();
                UpdateSignalPeakWindows();
            }
        }

        while (gyro_tick_accum_us >= GYRO_REFRESH_PERIOD_US)
        {
            gyro_tick_accum_us -= GYRO_REFRESH_PERIOD_US;
            if (lcd_ok)
            {
                if (!train_armed_idle)
                {
                    if (!modal_active_now)
                    {
                        GaugeRender_DrawGyroFast();
                    }
                }
            }
        }

        while (accel_live_tick_accum_us >= ACCEL_LIVE_PERIOD_US)
        {
            accel_live_tick_accum_us -= ACCEL_LIVE_PERIOD_US;
            /* Gyro/accel live update now runs in the gyro refresh loop above. */
        }

        while (shield_aux_tick_accum_us >= SHIELD_AUX_POLL_PERIOD_US)
        {
            shield_aux_tick_accum_us -= SHIELD_AUX_POLL_PERIOD_US;
            if (!train_armed_idle)
            {
                ShieldAuxUpdate();
            }
        }

        while (runtime_clock_tick_accum_us >= RUNTIME_CLOCK_PERIOD_US)
        {
            runtime_clock_tick_accum_us -= RUNTIME_CLOCK_PERIOD_US;
            if (playback_warmup_active &&
                (runtime_displayed_ds != UINT32_MAX) &&
                (runtime_displayed_ds >= RECPLAY_WARMUP_TARGET_DS))
            {
                uint32_t handoff_ds = RECPLAY_WARMUP_TARGET_DS;
                playback_warmup_active = false;
                playback_warmup_complete = true;
                GaugeRender_SetWarmupThinking(false);
                GaugeRender_PrimePredictionScore();
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = (uint32_t)((uint64_t)REPLAY_DEFAULT_STEP_DS * 100000ull);
                playback_rt_anchor_ds = handoff_ds;
                playback_rt_anchor_ticks = TimebaseNowTicks();
                runtime_elapsed_ds = handoff_ds;
                runtime_displayed_ds = handoff_ds;
                SetRuntimeClockFromDs(handoff_ds);
                PRINTF("AI_WARMUP: force_complete at %u ds from display timeline\r\n",
                       (unsigned int)handoff_ds);
            }
            if (!record_mode && !playback_active && !playback_run_complete)
            {
                if (s_timebase_ready && (s_timebase_hz != 0u))
                {
                    uint64_t now_ticks = TimebaseNowTicks();
                    uint64_t dt_ticks = (now_ticks >= runtime_clock_start_ticks) ? (now_ticks - runtime_clock_start_ticks) : 0u;
                    runtime_elapsed_ds = (uint32_t)((dt_ticks * 10ull) / s_timebase_hz);
                }
                else
                {
                    runtime_elapsed_ds++;
                }

                if ((runtime_displayed_ds != UINT32_MAX) && (runtime_elapsed_ds < runtime_displayed_ds))
                {
                    /* Enforce monotonic runtime display outside explicit mode resets. */
                    runtime_elapsed_ds = runtime_displayed_ds;
                }
                if ((runtime_displayed_ds != UINT32_MAX) &&
                    (runtime_elapsed_ds > (runtime_displayed_ds + RUNTIME_MAX_STEP_DS)))
                {
                    runtime_elapsed_ds = runtime_displayed_ds + RUNTIME_MAX_STEP_DS;
                }
                if (runtime_elapsed_ds != runtime_displayed_ds)
                {
                    uint16_t ch;
                    uint8_t cm, cs, cds;
                    ClockFromDeciseconds(runtime_elapsed_ds, &ch, &cm, &cs, &cds);
                    GaugeRender_SetRuntimeClock(ch, cm, cs, cds, true);
                    runtime_displayed_ds = runtime_elapsed_ds;
                }
            }
            else if (playback_active && GaugeRender_IsLiveBannerMode() && !playback_warmup_active && playback_prev_ts_valid)
            {
                uint32_t rt_ds = playback_rt_anchor_ds;
                if (s_timebase_ready && (s_timebase_hz != 0u))
                {
                    uint64_t now_ticks = TimebaseNowTicks();
                    uint64_t dt_ticks = (now_ticks >= playback_rt_anchor_ticks) ? (now_ticks - playback_rt_anchor_ticks) : 0u;
                    rt_ds += (uint32_t)((dt_ticks * 10ull) / s_timebase_hz);
                }
                else
                {
                    rt_ds += 1u;
                }
                if ((runtime_displayed_ds != UINT32_MAX) && (rt_ds < runtime_displayed_ds))
                {
                    rt_ds = runtime_displayed_ds;
                }
                if ((runtime_displayed_ds != UINT32_MAX) &&
                    (rt_ds > (runtime_displayed_ds + RUNTIME_MAX_STEP_DS)))
                {
                    rt_ds = runtime_displayed_ds + RUNTIME_MAX_STEP_DS;
                }
                if (rt_ds != runtime_displayed_ds)
                {
                    uint16_t ch;
                    uint8_t cm, cs, cds;
                    ClockFromDeciseconds(rt_ds, &ch, &cm, &cs, &cds);
                    GaugeRender_SetRuntimeClock(ch, cm, cs, cds, true);
                    runtime_displayed_ds = rt_ds;
                }
            }
        }

        {
            uint32_t log_period_us = 100000u;
            uint32_t log_hz = (uint32_t)ClampLogRateHz(s_log_rate_hz);
            const power_sample_t *log_sample = GetFrameSample();
            int16_t log_ax_mg;
            int16_t log_ay_mg;
            int16_t log_az_mg;
            int16_t log_gx;
            int16_t log_gy;
            int16_t log_gz;
            int16_t log_mx;
            int16_t log_my;
            int16_t log_mz;
            if (log_hz > 0u)
            {
                log_period_us = 1000000u / log_hz;
            }
            while (log_tick_accum_us >= log_period_us)
            {
                log_tick_accum_us -= log_period_us;
                ConsumeLogPeaks(&log_ax_mg, &log_ay_mg, &log_az_mg, &log_gx, &log_gy, &log_gz, &log_mx, &log_my, &log_mz);
                PRINTF("LOG,%uHZ,AX=%d,AY=%d,AZ=%d,GX=%d,GY=%d,GZ=%d,MX=%d,MY=%d,MZ=%d,T=%d.%dC,P=%d.%dHPA,AL=%u,AS=%u,RC=%u,SC=%u\r\n",
                       (unsigned int)log_hz,
                       (int)log_ax_mg,
                       (int)log_ay_mg,
                       (int)log_az_mg,
                       (int)log_gx,
                       (int)log_gy,
                       (int)log_gz,
                       (int)log_mx,
                       (int)log_my,
                       (int)log_mz,
                       (int)(s_temp_c10 / 10),
                       (int)(s_temp_c10 < 0 ? -s_temp_c10 : s_temp_c10) % 10,
                       (int)(s_baro_dhpa / 10),
                       (int)(s_baro_dhpa < 0 ? -s_baro_dhpa : s_baro_dhpa) % 10,
                       (unsigned int)s_anom_out.overall_level,
                       (unsigned int)((log_sample != NULL) ? log_sample->ai_status : s_frame_sample.ai_status),
                       (unsigned int)((log_sample != NULL) ? log_sample->alert_reason_code : s_frame_sample.alert_reason_code),
                       (unsigned int)((log_sample != NULL) ? log_sample->anomaly_score_pct : s_frame_sample.anomaly_score_pct));
            }
        }

        if (data_tick_accum_us >= POWER_TICK_PERIOD_US)
        {
            data_tick_accum_us -= POWER_TICK_PERIOD_US;
            PowerData_Tick();
        }

        while (recplay_tick_accum_us >= recplay_period_us)
        {
            recplay_tick_accum_us -= recplay_period_us;
            if (ext_flash_ok && record_mode && !GaugeRender_IsLiveBannerMode())
            {
                GaugeRender_IngestReplayCgmSample(0u, 0u, false);
                const power_sample_t *record_sample = GetFrameSample();
                int16_t rec_ax_mg;
                int16_t rec_ay_mg;
                int16_t rec_az_mg;
                int16_t rec_gx;
                int16_t rec_gy;
                int16_t rec_gz;
                int16_t rec_mx;
                int16_t rec_my;
                int16_t rec_mz;
                uint16_t rec_score;
                uint8_t rec_status;
                uint8_t rec_reason;
                if (s_timebase_ready && (s_timebase_hz != 0u))
                {
                    uint64_t now_ticks = TimebaseNowTicks();
                    uint64_t dt_ticks = (now_ticks >= runtime_clock_start_ticks) ? (now_ticks - runtime_clock_start_ticks) : 0u;
                    rec_elapsed_ds = (uint32_t)((dt_ticks * 10ull) / s_timebase_hz);
                }
                else
                {
                    rec_elapsed_ds++;
                }
                if ((runtime_displayed_ds != UINT32_MAX) && (rec_elapsed_ds < runtime_displayed_ds))
                {
                    rec_elapsed_ds = runtime_displayed_ds;
                }
                ConsumeCapturePeaks(&rec_ax_mg, &rec_ay_mg, &rec_az_mg, &rec_gx, &rec_gy, &rec_gz, &rec_mx, &rec_my, &rec_mz);
                rec_score = (record_sample != NULL) ? record_sample->anomaly_score_pct : s_frame_sample.anomaly_score_pct;
                rec_status = (record_sample != NULL) ? record_sample->ai_status : s_frame_sample.ai_status;
                rec_reason = (record_sample != NULL) ? record_sample->alert_reason_code : s_frame_sample.alert_reason_code;
                ConsumeAlertCaptureWindow(rec_score, rec_status, rec_reason, &rec_score, &rec_status, &rec_reason);

                if (!ExtFlashRecorder_AppendSampleEx(rec_ax_mg,
                                                     rec_ay_mg,
                                                     rec_az_mg,
                                                     rec_gx,
                                                     rec_gy,
                                                     rec_gz,
                                                     s_temp_c10,
                                                     rec_mx,
                                                     rec_my,
                                                     rec_mz,
                                                     s_baro_dhpa,
                                                     s_sht_temp_c10,
                                                     s_sht_rh_dpct,
                                                     s_stts_temp_c10,
                                                     rec_score,
                                                     rec_status,
                                                     rec_reason,
                                                     rec_elapsed_ds))
                {
                    PRINTF("EXT_FLASH_REC: write_failed\r\n");
                }
                else
                {
                    uint16_t ch;
                    uint8_t cm, cs, cds;
                    ClockFromDeciseconds(rec_elapsed_ds, &ch, &cm, &cs, &cds);
                    GaugeRender_SetRuntimeClock(ch, cm, cs, cds, true);
                    runtime_displayed_ds = rec_elapsed_ds;
                    GaugeRender_SetPlayhead(99u, true);
                }
            }
            else if (playback_active)
            {
                uint32_t replay_steps = playback_warmup_active ? RECPLAY_WARMUP_MULTIPLIER : RECPLAY_FULLRUN_MULTIPLIER;
                if (!playback_warmup_active && GaugeRender_IsLiveBannerMode())
                {
                    replay_steps = 1u;
                }
                if (replay_steps == 0u)
                {
                    replay_steps = 1u;
                }
                for (uint32_t replay_step = 0u; replay_step < replay_steps; replay_step++)
                {
                    if (ExtFlashRecorder_ReadNextSample(&playback_sample))
                    {
                        playback_read_fail_streak = 0u;
                        if (CGM_REPLAY_SUBJECT001_LEN > 0u)
                        {
                            uint16_t replay_glucose = kCgmReplaySubject001Mgdl[playback_cgm_index % CGM_REPLAY_SUBJECT001_LEN];
                            GaugeRender_IngestReplayCgmSample(playback_sample.ts_ds, replay_glucose, true);
                            playback_cgm_index++;
                        }
                        /* In LIVE playback mode, replay drives timeline only.
                         * Sensor visuals (ball/activity/IMU traces) remain live from actual device sensors. */
                        {
                            uint16_t ch;
                            uint8_t cm, cs, cds;
                            uint32_t play_ds;
                            if (!playback_prev_src_ts_valid)
                            {
                                play_ds = (runtime_displayed_ds == UINT32_MAX) ? 0u : runtime_displayed_ds;
                            }
                            else
                            {
                                uint32_t delta_ds = 1u;
                                if (playback_sample.ts_ds > playback_prev_src_ts_ds)
                                {
                                    delta_ds = playback_sample.ts_ds - playback_prev_src_ts_ds;
                                }
                                if (!playback_warmup_active && (delta_ds > RECPLAY_POST_WARMUP_MAX_DELTA_DS))
                                {
                                    delta_ds = RECPLAY_POST_WARMUP_MAX_DELTA_DS;
                                }
                                play_ds = (runtime_displayed_ds == UINT32_MAX) ? delta_ds : (runtime_displayed_ds + delta_ds);
                            }
                            playback_prev_src_ts_ds = playback_sample.ts_ds;
                            playback_prev_src_ts_valid = true;
                            if ((runtime_displayed_ds != UINT32_MAX) && (play_ds < runtime_displayed_ds))
                            {
                                /* Avoid display-clock stalls on duplicate/non-monotonic replay timestamps. */
                                play_ds = runtime_displayed_ds + 1u;
                            }
                            if (!playback_warmup_active &&
                                (runtime_displayed_ds != UINT32_MAX) &&
                                (play_ds > (runtime_displayed_ds + RUNTIME_MAX_STEP_DS)))
                            {
                                play_ds = runtime_displayed_ds + RUNTIME_MAX_STEP_DS;
                            }
                            ClockFromDeciseconds(play_ds, &ch, &cm, &cs, &cds);
                            GaugeRender_SetRuntimeClock(ch, cm, cs, cds, true);
                            runtime_displayed_ds = play_ds;
                            if (playback_warmup_active && (play_ds >= RECPLAY_WARMUP_TARGET_DS))
                            {
                                play_ds = RECPLAY_WARMUP_TARGET_DS;
                                runtime_displayed_ds = play_ds;
                                SetRuntimeClockFromDs(play_ds);
                                playback_warmup_active = false;
                                playback_warmup_complete = true;
                                GaugeRender_SetWarmupThinking(false);
                                GaugeRender_PrimePredictionScore();
                                PRINTF("AI_WARMUP: complete at %u ds, switching to full-run scoring\r\n",
                                       (unsigned int)play_ds);
                                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                                playback_period_us = (uint32_t)((uint64_t)REPLAY_DEFAULT_STEP_DS * 100000ull);
                                playback_rt_anchor_ds = play_ds;
                                playback_rt_anchor_ticks = TimebaseNowTicks();
                                break;
                            }
                            if (GaugeRender_IsLiveBannerMode() && !playback_warmup_active)
                            {
                                if (playback_prev_ts_valid && (play_ds > playback_prev_ts_ds))
                                {
                                    uint32_t ds_delta = play_ds - playback_prev_ts_ds;
                                    uint64_t period64 = (uint64_t)ds_delta * 100000ull;
                                    if (period64 < 100000ull)
                                    {
                                        period64 = 100000ull;
                                    }
                                    if (period64 > 600000000ull)
                                    {
                                        period64 = 600000000ull;
                                    }
                                    playback_period_us = (uint32_t)period64;
                                }
                                playback_prev_ts_ds = play_ds;
                                playback_prev_ts_valid = true;
                                playback_rt_anchor_ds = play_ds;
                                playback_rt_anchor_ticks = TimebaseNowTicks();
                            }
                        }
                        if (ExtFlashRecorder_GetPlaybackInfo(&play_off, &play_cnt) && (play_cnt > 0u))
                        {
                            uint32_t pos = (play_off * 100u) / play_cnt;
                            if (pos > 99u)
                            {
                                pos = 99u;
                            }
                            GaugeRender_SetPlayhead((uint8_t)pos, true);
                        }
                    }
                    else
                    {
                        if (GaugeRender_IsLiveBannerMode())
                        {
                            playback_read_fail_streak++;
                            if (playback_read_fail_streak < 3u)
                            {
                                PRINTF("EXT_FLASH_PLAY: read_retry streak=%u\r\n", (unsigned int)playback_read_fail_streak);
                            }
                            else
                            {
                                bool restarted = ext_flash_ok && ExtFlashRecorder_StartPlayback();
                                if (restarted)
                                {
                                    playback_active = true;
                                    playback_run_complete = false;
                                    playback_cgm_index = 0u;
                                    playback_prev_ts_valid = false;
                                    playback_prev_src_ts_valid = false;
                                    playback_period_us = RECPLAY_TICK_PERIOD_US;
                                    ConfigurePlaybackWarmup(playback_active,
                                                            GaugeRender_IsLiveBannerMode(),
                                                            &playback_warmup_active,
                                                            &playback_warmup_complete);
                                    playback_read_fail_streak = 0u;
                                    PRINTF("EXT_FLASH_PLAY: recovered_after_read_fail\r\n");
                                }
                                else
                                {
                                    playback_active = false;
                                    playback_run_complete = true;
                                    playback_warmup_active = false;
                                    GaugeRender_IngestReplayCgmSample(0u, 0u, false);
                                    GaugeRender_SetWarmupThinking(false);
                                    GaugeRender_PrimePredictionScore();
                                    GaugeRender_SetPlayhead(100u, true);
                                    playback_prev_ts_valid = false;
                                    playback_prev_src_ts_valid = false;
                                    playback_period_us = RECPLAY_TICK_PERIOD_US;
                                    playback_read_fail_streak = 0u;
                                    PRINTF("EXT_FLASH_PLAY: complete_final_score_hold\r\n");
                                }
                            }
                        }
                        else
                        {
                            playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
                            playback_cgm_index = 0u;
                            playback_run_complete = false;
                            ConfigurePlaybackWarmup(playback_active,
                                                    GaugeRender_IsLiveBannerMode(),
                                                    &playback_warmup_active,
                                                    &playback_warmup_complete);
                            if (playback_active)
                            {
                                runtime_displayed_ds = UINT32_MAX;
                                GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                                playback_period_us = RECPLAY_TICK_PERIOD_US;
                                PRINTF("EXT_FLASH_PLAY: loop_restart\r\n");
                            }
                            else
                            {
                                if ((anom_mode == ANOMALY_MODE_TRAINED_MONITOR) && s_anom_out.training_active &&
                                    !s_anom_out.trained_ready)
                                {
                                    AnomalyEngine_StopTraining();
                                    PRINTF("AI_TRAIN: replay_unavailable\r\n");
                                }
                                else
                                {
                                    PRINTF("EXT_FLASH_PLAY: read_failed\r\n");
                                }
                            }
                        }
                        break;
                    }
                }
            }
            else if (ext_flash_ok && !record_mode && GaugeRender_IsLiveBannerMode() && !playback_run_complete)
            {
                /* If playback dropped out, keep trying to restart in LIVE mode. */
                playback_active = ExtFlashRecorder_StartPlayback();
                playback_run_complete = false;
                ConfigurePlaybackWarmup(playback_active,
                                        GaugeRender_IsLiveBannerMode(),
                                        &playback_warmup_active,
                                        &playback_warmup_complete);
                if (playback_active)
                {
                    playback_cgm_index = 0u;
                    runtime_displayed_ds = UINT32_MAX;
                    GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
                    playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                    playback_period_us = RECPLAY_TICK_PERIOD_US;
                    PRINTF("EXT_FLASH_PLAY: retry_restart\r\n");
                }
            }
            else
            {
                GaugeRender_IngestReplayCgmSample(0u, 0u, false);
            }

            AnomalyEngine_Update(s_accel_x_mg, s_accel_y_mg, s_accel_z_mg, s_temp_c10);
            AnomalyEngine_GetOutput(&s_anom_out);
            if (!playback_run_complete && !prev_trained_ready && s_anom_out.trained_ready)
            {
                /* Automatically promote to LIVE once model fit is complete on-board. */
                GaugeRender_SetLiveBannerMode(true);
                GaugeRender_SetRecordMode(false);
                playback_active = ext_flash_ok && ExtFlashRecorder_StartPlayback();
                playback_cgm_index = 0u;
                playback_run_complete = false;
                ConfigurePlaybackWarmup(playback_active, true, &playback_warmup_active, &playback_warmup_complete);
                playback_prev_ts_valid = false;
                playback_prev_src_ts_valid = false;
                playback_period_us = RECPLAY_TICK_PERIOD_US;
                AnomalyEngine_StopTraining();
                SaveUiSettingsIfReady(ext_flash_ok,
                                      anom_mode,
                                      AnomalyEngine_GetTune(),
                                      true,
                                      ai_enabled,
                                      ai_use_npu,
                                      s_limit_g_warn_mg,
                                      s_limit_g_fail_mg,
                                      s_limit_temp_lo_c10,
                                      s_limit_temp_hi_c10,
                                      s_limit_gyro_dps,
                                      s_log_rate_hz);
                PRINTF("AI_TRAIN: complete_live\r\n");
            }
            prev_trained_ready = s_anom_out.trained_ready;
            GaugeRender_SetAnomalyInfo((uint8_t)s_anom_out.mode,
                                       (uint8_t)s_anom_out.tune,
                                       s_anom_out.training_active,
                                       s_anom_out.trained_ready,
                                       (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AX],
                                       (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AY],
                                       (uint8_t)s_anom_out.channel_level[ANOMALY_CH_AZ],
                                       (uint8_t)s_anom_out.channel_level[ANOMALY_CH_TEMP],
                                       (uint8_t)s_anom_out.overall_level);
        }

        if (temp_tick_accum_us >= TEMP_REFRESH_PERIOD_US)
        {
            temp_tick_accum_us -= TEMP_REFRESH_PERIOD_US;
            if (!train_armed_idle)
            {
                BoardTempUpdate();
            }
        }

        if (accel_test_tick_accum_us >= ACCEL_TEST_LOG_PERIOD_US)
        {
            accel_test_tick_accum_us -= ACCEL_TEST_LOG_PERIOD_US;
            if (s_accel_ready)
            {
                AccelAxisSelfTestLog();
            }
        }

        if (lcd_ok && (render_tick_accum_us >= DISPLAY_REFRESH_PERIOD_US))
        {
            render_tick_accum_us -= DISPLAY_REFRESH_PERIOD_US;
            GaugeRender_DrawFrame(GetFrameSample(), ai_enabled, PowerData_GetReplayProfile());
        }

        DelayUsByTimebase(TOUCH_POLL_DELAY_US);
    }
#endif
}
