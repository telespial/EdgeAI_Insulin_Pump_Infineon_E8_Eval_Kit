#include "ext_flash_recorder.h"

#include <string.h>

#include "fsl_flexspi_nor_flash.h"

#define EDGEAI_FLEXSPI_INSTANCE (0u)
#define EDGEAI_REC_MAGIC (0x52454335u)  /* "REC5" */
#define EDGEAI_META_MAGIC (0x4D455441u) /* "META" */
#define EDGEAI_UICFG_TAG (0xA6u)
#define EDGEAI_MAX_PAGE_SIZE (1024u)

typedef struct
{
    uint32_t magic;
    uint32_t generation;
    uint32_t reserved0;
    uint32_t reserved1;
} ext_flash_meta_t;

typedef struct
{
    uint32_t magic;
    uint32_t seq;
    uint16_t generation;
    uint16_t reserved0;
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
    uint16_t reserved1;
    uint32_t ts_ds;
} ext_flash_sample_record_t;

typedef union
{
    uint32_t words[EDGEAI_MAX_PAGE_SIZE / 4u];
    uint8_t bytes[EDGEAI_MAX_PAGE_SIZE];
} ext_flash_page_buf_t;

static serial_nor_config_option_t s_norOption = {
    .option0.U = 0xC0000001u,
    .option1.U = 0u,
};

static flexspi_nor_config_t s_norConfig;
static ext_flash_page_buf_t s_pageBuf;
static uint32_t s_regionStart;
static uint32_t s_dataStart;
static uint32_t s_capacityPages;
static uint32_t s_writeIndex;
static uint32_t s_oldestIndex;
static uint32_t s_sampleCount;
static uint32_t s_seq;
static uint32_t s_generation;
static bool s_ready;
static uint32_t s_playStartIndex;
static uint32_t s_playCount;
static uint32_t s_playOffset;
static uint8_t s_ui_mode;
static uint8_t s_ui_tune;
static bool s_ui_run_live;
static bool s_ui_ai_enabled;
static bool s_ui_ai_backend_npu;
static uint16_t s_ui_g_warn_mg;
static uint16_t s_ui_g_fail_mg;
static int16_t s_ui_temp_low_c10;
static int16_t s_ui_temp_high_c10;
static uint16_t s_ui_gyro_limit_dps;
static uint8_t s_ui_log_rate_hz;
static bool s_ui_valid;

static uint32_t ExtFlashRecorder_PackUiSettingsCore(void)
{
    uint32_t gw_d10 = (uint32_t)(s_ui_g_warn_mg / 100u);
    uint32_t gf_d10 = (uint32_t)(s_ui_g_fail_mg / 100u);
    if (gw_d10 > 255u)
    {
        gw_d10 = 255u;
    }
    if (gf_d10 > 255u)
    {
        gf_d10 = 255u;
    }
    return ((uint32_t)EDGEAI_UICFG_TAG << 24) |
           (gw_d10 << 16) |
           (gf_d10 << 8) |
           ((uint32_t)1u << 7) |
           ((s_ui_ai_backend_npu ? 1u : 0u) << 6) |
           (((uint32_t)s_ui_tune & 0x3u) << 4) |
           (((uint32_t)s_ui_mode & 0x3u) << 2) |
           ((s_ui_run_live ? 1u : 0u) << 1) |
           (s_ui_ai_enabled ? 1u : 0u);
}

static uint32_t ExtFlashRecorder_PackUiSettingsLimits(void)
{
    int32_t tl_c = (int32_t)(s_ui_temp_low_c10 / 10) + 40;
    int32_t th_c = (int32_t)(s_ui_temp_high_c10 / 10) + 40;
    uint32_t gy_d10 = (uint32_t)(s_ui_gyro_limit_dps / 10u);
    if (tl_c < 0)
    {
        tl_c = 0;
    }
    if (tl_c > 255)
    {
        tl_c = 255;
    }
    if (th_c < 0)
    {
        th_c = 0;
    }
    if (th_c > 255)
    {
        th_c = 255;
    }
    if (gy_d10 > 255u)
    {
        gy_d10 = 255u;
    }
    return ((uint32_t)gy_d10 << 24) |
           ((uint32_t)th_c << 16) |
           ((uint32_t)tl_c << 8) |
           ((uint32_t)s_ui_log_rate_hz & 0xFFu);
}

static void ExtFlashRecorder_UnpackUiSettings(uint32_t packed_core, uint32_t packed_limits)
{
    if (((packed_core >> 24) & 0xFFu) != EDGEAI_UICFG_TAG)
    {
        s_ui_mode = 0u;
        s_ui_tune = 1u;
        s_ui_run_live = true;
        s_ui_ai_enabled = true;
        s_ui_ai_backend_npu = true;
        s_ui_g_warn_mg = 12000u;
        s_ui_g_fail_mg = 15000u;
        s_ui_temp_low_c10 = 0;
        s_ui_temp_high_c10 = 700;
        s_ui_gyro_limit_dps = 500u;
        s_ui_log_rate_hz = 10u;
        s_ui_valid = false;
        return;
    }

    s_ui_mode = (uint8_t)((packed_core >> 2) & 0x3u);
    s_ui_tune = (uint8_t)((packed_core >> 4) & 0x3u);
    s_ui_run_live = (((packed_core >> 1) & 0x1u) != 0u);
    s_ui_ai_enabled = ((packed_core & 0x1u) != 0u);
    /* Bit7 marks schema with persisted backend bit6. Fall back to NPU for legacy records. */
    s_ui_ai_backend_npu = (((packed_core >> 7) & 0x1u) != 0u) ? ((((packed_core >> 6) & 0x1u) != 0u)) : true;
    s_ui_g_warn_mg = 12000u;
    s_ui_g_fail_mg = 15000u;
    s_ui_temp_low_c10 = 0;
    s_ui_temp_high_c10 = 700;
    s_ui_gyro_limit_dps = 500u;
    s_ui_log_rate_hz = 10u;
    {
        uint32_t gw_d10 = (packed_core >> 16) & 0xFFu;
        uint32_t gf_d10 = (packed_core >> 8) & 0xFFu;
        int32_t tl_c = ((int32_t)((packed_limits >> 8) & 0xFFu)) - 40;
        int32_t th_c = ((int32_t)((packed_limits >> 16) & 0xFFu)) - 40;
        uint32_t gy_d10 = (packed_limits >> 24) & 0xFFu;
        uint32_t log_hz = packed_limits & 0xFFu;
        if ((gw_d10 >= 20u) && (gw_d10 <= 150u))
        {
            s_ui_g_warn_mg = (uint16_t)(gw_d10 * 100u);
        }
        if ((gf_d10 >= 30u) && (gf_d10 <= 160u))
        {
            s_ui_g_fail_mg = (uint16_t)(gf_d10 * 100u);
        }
        if ((tl_c >= -20) && (tl_c <= 30))
        {
            s_ui_temp_low_c10 = (int16_t)(tl_c * 10);
        }
        if ((th_c >= 0) && (th_c <= 100))
        {
            s_ui_temp_high_c10 = (int16_t)(th_c * 10);
        }
        if ((gy_d10 >= 10u) && (gy_d10 <= 200u))
        {
            s_ui_gyro_limit_dps = (uint16_t)(gy_d10 * 10u);
        }
        if ((log_hz == 1u) || (log_hz == 5u) || (log_hz == 10u) || (log_hz == 20u) ||
            (log_hz == 30u) || (log_hz == 40u) || (log_hz == 50u))
        {
            s_ui_log_rate_hz = (uint8_t)log_hz;
        }
    }
    if (s_ui_temp_high_c10 <= s_ui_temp_low_c10)
    {
        s_ui_temp_high_c10 = s_ui_temp_low_c10 + 50;
    }
    if (s_ui_g_fail_mg <= s_ui_g_warn_mg)
    {
        s_ui_g_fail_mg = s_ui_g_warn_mg + 500u;
    }
    s_ui_valid = true;
}

static uint32_t ExtFlashRecorder_PageAddr(uint32_t page_index)
{
    return s_dataStart + (page_index * s_norConfig.pageSize);
}

static bool ExtFlashRecorder_ReadRawPage(uint32_t addr)
{
    status_t st = FLEXSPI_NorFlash_Read(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, s_pageBuf.words, addr, s_norConfig.pageSize);
    return (st == kStatus_Success);
}

static bool ExtFlashRecorder_ReadRecordAt(uint32_t page_index, ext_flash_sample_record_t *rec)
{
    if ((rec == NULL) || (page_index >= s_capacityPages))
    {
        return false;
    }
    if (!ExtFlashRecorder_ReadRawPage(ExtFlashRecorder_PageAddr(page_index)))
    {
        return false;
    }
    memcpy(rec, s_pageBuf.bytes, sizeof(*rec));
    return (rec->magic == EDGEAI_REC_MAGIC) && (rec->generation == (uint16_t)(s_generation & 0xFFFFu));
}

static bool ExtFlashRecorder_WriteMeta(uint32_t generation)
{
    ext_flash_meta_t meta;
    status_t st;

    meta.magic = EDGEAI_META_MAGIC;
    meta.generation = generation;
    meta.reserved0 = ExtFlashRecorder_PackUiSettingsCore();
    meta.reserved1 = ExtFlashRecorder_PackUiSettingsLimits();

    st = FLEXSPI_NorFlash_Erase(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, s_regionStart, s_norConfig.sectorSize);
    if (st != kStatus_Success)
    {
        return false;
    }

    memset(s_pageBuf.bytes, 0xFF, s_norConfig.pageSize);
    memcpy(s_pageBuf.bytes, &meta, sizeof(meta));
    st = FLEXSPI_NorFlash_ProgramPage(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, s_regionStart, s_pageBuf.words);
    return (st == kStatus_Success);
}

static bool ExtFlashRecorder_ReadMeta(uint32_t *generation_out)
{
    ext_flash_meta_t meta;

    if (generation_out == NULL)
    {
        return false;
    }
    if (!ExtFlashRecorder_ReadRawPage(s_regionStart))
    {
        return false;
    }

    memcpy(&meta, s_pageBuf.bytes, sizeof(meta));
    if (meta.magic != EDGEAI_META_MAGIC)
    {
        return false;
    }

    *generation_out = meta.generation;
    ExtFlashRecorder_UnpackUiSettings(meta.reserved0, meta.reserved1);
    return true;
}

static void ExtFlashRecorder_RecoverState(void)
{
    ext_flash_sample_record_t rec;
    uint32_t i;
    uint32_t valid_count = 0u;
    uint32_t min_seq = 0xFFFFFFFFu;
    uint32_t max_seq = 0u;
    uint32_t min_idx = 0u;
    uint32_t max_idx = 0u;

    s_writeIndex = 0u;
    s_oldestIndex = 0u;
    s_sampleCount = 0u;
    s_seq = 0u;

    for (i = 0u; i < s_capacityPages; i++)
    {
        if (ExtFlashRecorder_ReadRecordAt(i, &rec))
        {
            if (valid_count == 0u)
            {
                min_seq = rec.seq;
                max_seq = rec.seq;
                min_idx = i;
                max_idx = i;
            }
            else
            {
                if (rec.seq < min_seq)
                {
                    min_seq = rec.seq;
                    min_idx = i;
                }
                if (rec.seq > max_seq)
                {
                    max_seq = rec.seq;
                    max_idx = i;
                }
            }
            valid_count++;
        }
    }

    if (valid_count == 0u)
    {
        return;
    }

    s_sampleCount = valid_count;
    s_oldestIndex = min_idx;
    s_writeIndex = (max_idx + 1u) % s_capacityPages;
    s_seq = max_seq + 1u;
}

bool ExtFlashRecorder_Init(void)
{
    status_t st;
    uint32_t totalSize;
    uint32_t regionSize;
    uint32_t meta_generation = 0u;

    s_ready = false;
    s_writeIndex = 0u;
    s_oldestIndex = 0u;
    s_sampleCount = 0u;
    s_seq = 0u;
    s_generation = 1u;
    s_playStartIndex = 0u;
    s_playCount = 0u;
    s_playOffset = 0u;
    s_ui_mode = 0u;
    s_ui_tune = 1u;
    s_ui_run_live = true;
    s_ui_ai_enabled = true;
    s_ui_ai_backend_npu = true;
    s_ui_g_warn_mg = 12000u;
    s_ui_g_fail_mg = 15000u;
    s_ui_temp_low_c10 = 0;
    s_ui_temp_high_c10 = 700;
    s_ui_gyro_limit_dps = 500u;
    s_ui_log_rate_hz = 10u;
    s_ui_valid = false;
    memset(&s_norConfig, 0, sizeof(s_norConfig));

    st = FLEXSPI_NorFlash_GetConfig(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, &s_norOption);
    if (st != kStatus_Success)
    {
        return false;
    }

    st = FLEXSPI_NorFlash_Init(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig);
    if (st != kStatus_Success)
    {
        return false;
    }

    totalSize = s_norConfig.memConfig.sflashA1Size;
    if ((totalSize == 0u) || (s_norConfig.pageSize == 0u) || (s_norConfig.sectorSize == 0u))
    {
        return false;
    }
    if ((s_norConfig.pageSize > EDGEAI_MAX_PAGE_SIZE) || ((s_norConfig.pageSize % 4u) != 0u))
    {
        return false;
    }
    if (sizeof(ext_flash_sample_record_t) > s_norConfig.pageSize)
    {
        return false;
    }

    regionSize = totalSize / 2u;
    regionSize -= (regionSize % s_norConfig.sectorSize);
    if (regionSize <= s_norConfig.sectorSize)
    {
        return false;
    }

    s_regionStart = totalSize - regionSize;
    s_dataStart = s_regionStart + s_norConfig.sectorSize; /* reserve first sector for metadata */
    s_capacityPages = (regionSize - s_norConfig.sectorSize) / s_norConfig.pageSize;
    if (s_capacityPages == 0u)
    {
        return false;
    }

    if (ExtFlashRecorder_ReadMeta(&meta_generation))
    {
        s_generation = (meta_generation == 0u) ? 1u : meta_generation;
    }
    else
    {
        s_generation = 1u;
        if (!ExtFlashRecorder_WriteMeta(s_generation))
        {
            return false;
        }
    }

    s_ready = true;
    ExtFlashRecorder_RecoverState();
    return true;
}

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
                                     uint32_t ts_ds)
{
    ext_flash_sample_record_t rec;
    status_t st;
    uint32_t write_addr;

    if (!s_ready || (s_capacityPages == 0u))
    {
        return false;
    }

    if (s_writeIndex >= s_capacityPages)
    {
        s_writeIndex = 0u;
    }

    write_addr = ExtFlashRecorder_PageAddr(s_writeIndex);
    if ((write_addr % s_norConfig.sectorSize) == 0u)
    {
        st = FLEXSPI_NorFlash_Erase(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, write_addr, s_norConfig.sectorSize);
        if (st != kStatus_Success)
        {
            return false;
        }
    }

    rec.magic = EDGEAI_REC_MAGIC;
    rec.seq = s_seq++;
    rec.generation = (uint16_t)(s_generation & 0xFFFFu);
    rec.reserved0 = anomaly_score_pct;
    rec.ax_mg = ax_mg;
    rec.ay_mg = ay_mg;
    rec.az_mg = az_mg;
    rec.gx_mdps = gx_mdps;
    rec.gy_mdps = gy_mdps;
    rec.gz_mdps = gz_mdps;
    rec.temp_c10 = temp_c10;
    rec.mag_x_mgauss = mag_x_mgauss;
    rec.mag_y_mgauss = mag_y_mgauss;
    rec.mag_z_mgauss = mag_z_mgauss;
    rec.baro_dhpa = baro_dhpa;
    rec.sht_temp_c10 = sht_temp_c10;
    rec.sht_rh_dpct = sht_rh_dpct;
    rec.stts_temp_c10 = stts_temp_c10;
    rec.reserved1 = (((uint16_t)alert_status & 0xFFu) << 8) | ((uint16_t)alert_reason_code & 0xFFu);
    rec.ts_ds = ts_ds;

    memset(s_pageBuf.bytes, 0xFF, s_norConfig.pageSize);
    memcpy(s_pageBuf.bytes, &rec, sizeof(rec));
    st = FLEXSPI_NorFlash_ProgramPage(EDGEAI_FLEXSPI_INSTANCE, &s_norConfig, write_addr, s_pageBuf.words);
    if (st != kStatus_Success)
    {
        return false;
    }

    if (s_sampleCount == 0u)
    {
        s_oldestIndex = s_writeIndex;
    }

    s_writeIndex = (s_writeIndex + 1u) % s_capacityPages;
    if (s_sampleCount < s_capacityPages)
    {
        s_sampleCount++;
    }
    else
    {
        s_oldestIndex = s_writeIndex;
    }

    return true;
}

bool ExtFlashRecorder_AppendSample(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, uint8_t temp_c)
{
    return ExtFlashRecorder_AppendSampleEx(ax_mg,
                                           ay_mg,
                                           az_mg,
                                           0,
                                           0,
                                           0,
                                           (int16_t)temp_c * 10,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0,
                                           0u,
                                           0u,
                                           0u,
                                           0u);
}

bool ExtFlashRecorder_IsReady(void)
{
    return s_ready;
}

bool ExtFlashRecorder_ClearAll(void)
{
    if (!s_ready)
    {
        return false;
    }

    s_generation++;
    if (s_generation == 0u)
    {
        s_generation = 1u;
    }
    if (!ExtFlashRecorder_WriteMeta(s_generation))
    {
        return false;
    }

    s_writeIndex = 0u;
    s_oldestIndex = 0u;
    s_sampleCount = 0u;
    s_seq = 0u;
    s_playStartIndex = 0u;
    s_playCount = 0u;
    s_playOffset = 0u;
    return true;
}

bool ExtFlashRecorder_StartPlayback(void)
{
    if (!s_ready || (s_sampleCount == 0u) || (s_capacityPages == 0u))
    {
        return false;
    }

    s_playCount = s_sampleCount;
    s_playStartIndex = s_oldestIndex;
    s_playOffset = 0u;
    return true;
}

bool ExtFlashRecorder_ReadNextSample(ext_flash_sample_t *sample)
{
    ext_flash_sample_record_t rec;
    uint32_t page_index;
    int32_t temp_c_rounded;

    if (!s_ready || (sample == NULL) || (s_playCount == 0u) || (s_capacityPages == 0u))
    {
        return false;
    }

    page_index = (s_playStartIndex + s_playOffset) % s_capacityPages;
    if (!ExtFlashRecorder_ReadRecordAt(page_index, &rec))
    {
        return false;
    }

    sample->seq = rec.seq;
    sample->ts_ds = rec.ts_ds;
    sample->ax_mg = rec.ax_mg;
    sample->ay_mg = rec.ay_mg;
    sample->az_mg = rec.az_mg;
    sample->gx_mdps = rec.gx_mdps;
    sample->gy_mdps = rec.gy_mdps;
    sample->gz_mdps = rec.gz_mdps;
    sample->temp_c10 = rec.temp_c10;
    sample->mag_x_mgauss = rec.mag_x_mgauss;
    sample->mag_y_mgauss = rec.mag_y_mgauss;
    sample->mag_z_mgauss = rec.mag_z_mgauss;
    sample->baro_dhpa = rec.baro_dhpa;
    sample->sht_temp_c10 = rec.sht_temp_c10;
    sample->sht_rh_dpct = rec.sht_rh_dpct;
    sample->stts_temp_c10 = rec.stts_temp_c10;
    sample->anomaly_score_pct = rec.reserved0;
    sample->alert_status = (uint8_t)((rec.reserved1 >> 8) & 0xFFu);
    sample->alert_reason_code = (uint8_t)(rec.reserved1 & 0xFFu);

    temp_c_rounded = (sample->temp_c10 >= 0) ? (sample->temp_c10 + 5) : (sample->temp_c10 - 5);
    temp_c_rounded /= 10;
    if (temp_c_rounded < 0)
    {
        temp_c_rounded = 0;
    }
    if (temp_c_rounded > 99)
    {
        temp_c_rounded = 99;
    }
    sample->temp_c = (uint8_t)temp_c_rounded;

    s_playOffset++;
    if (s_playOffset >= s_playCount)
    {
        s_playOffset = 0u;
    }
    return true;
}

bool ExtFlashRecorder_GetPlaybackInfo(uint32_t *offset, uint32_t *count)
{
    if (!s_ready || (offset == NULL) || (count == NULL))
    {
        return false;
    }

    *offset = s_playOffset;
    *count = s_playCount;
    return true;
}

bool ExtFlashRecorder_GetRecordInfo(uint32_t *count)
{
    if (!s_ready || (count == NULL))
    {
        return false;
    }

    *count = s_sampleCount;
    return true;
}

bool ExtFlashRecorder_GetUsageInfo(uint32_t *used_count, uint32_t *capacity_count)
{
    if (!s_ready || (used_count == NULL) || (capacity_count == NULL))
    {
        return false;
    }

    *used_count = s_sampleCount;
    *capacity_count = s_capacityPages;
    return true;
}

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
                                     uint8_t log_rate_hz)
{
    if (!s_ready)
    {
        return false;
    }

    s_ui_mode = mode & 0x3u;
    s_ui_tune = tune & 0x3u;
    s_ui_run_live = run_live;
    s_ui_ai_enabled = ai_enabled;
    s_ui_ai_backend_npu = ai_backend_npu;
    s_ui_g_warn_mg = g_warn_mg;
    s_ui_g_fail_mg = g_fail_mg;
    s_ui_temp_low_c10 = temp_low_c10;
    s_ui_temp_high_c10 = temp_high_c10;
    s_ui_gyro_limit_dps = gyro_limit_dps;
    s_ui_log_rate_hz = log_rate_hz;
    s_ui_valid = true;
    return ExtFlashRecorder_WriteMeta(s_generation);
}

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
                                    bool *valid)
{
    if ((mode == NULL) || (tune == NULL) || (run_live == NULL) || (ai_enabled == NULL) || (ai_backend_npu == NULL) ||
        (g_warn_mg == NULL) ||
        (g_fail_mg == NULL) ||
        (temp_low_c10 == NULL) || (temp_high_c10 == NULL) || (gyro_limit_dps == NULL) || (log_rate_hz == NULL) ||
        (valid == NULL))
    {
        return false;
    }
    if (!s_ready)
    {
        return false;
    }

    *mode = s_ui_mode;
    *tune = s_ui_tune;
    *run_live = s_ui_run_live;
    *ai_enabled = s_ui_ai_enabled;
    *ai_backend_npu = s_ui_ai_backend_npu;
    *g_warn_mg = s_ui_g_warn_mg;
    *g_fail_mg = s_ui_g_fail_mg;
    *temp_low_c10 = s_ui_temp_low_c10;
    *temp_high_c10 = s_ui_temp_high_c10;
    *gyro_limit_dps = s_ui_gyro_limit_dps;
    *log_rate_hz = s_ui_log_rate_hz;
    *valid = s_ui_valid;
    return true;
}
