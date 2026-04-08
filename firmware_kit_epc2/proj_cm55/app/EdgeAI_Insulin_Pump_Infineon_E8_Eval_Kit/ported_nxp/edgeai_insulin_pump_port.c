#include "edgeai_insulin_pump_port.h"

#include "medical_display_hal.h"
#include "medical_lcd_layout.h"
#include "pump_bg.h"
#include "cgm_preprocess.h"
#include "power_data_source.h"
#include "../upstream_nxp/src/cgm_replay_subject001.h"
#include "../platform/insulin_platform.h"
#include "../platform/display_hal.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define PORT_FRAME_US 100000u
#define DATA_STREAM_ADVANCE_DS 600u
#define REPLAY_STEP_DS 3000u
#define STREAM_STEP_FRAME_DIV 3u
#define RGB565(r, g, b) (uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | (((uint16_t)(b) & 0xF8u) >> 3))

enum
{
    EDGEAI_BTN_X0 = 193,
    EDGEAI_BTN_Y0 = 8,
    EDGEAI_BTN_X1 = 285,
    EDGEAI_BTN_Y1 = 41
};

enum
{
    EDGEAI_BTN_W = (EDGEAI_BTN_X1 - EDGEAI_BTN_X0 + 1),
    EDGEAI_BTN_H = (EDGEAI_BTN_Y1 - EDGEAI_BTN_Y0 + 1),
    EDGEAI_BTN_RADIUS = 2
};

enum
{
    CHART_X0 = 296,
    CHART_Y0 = 8,
    CHART_X1 = 469,
    CHART_Y1 = 128,
    CHART_RADIUS = 8,
    CHART_MAX_POINTS = 80
};

enum
{
    TERM_X0 = 296,
    TERM_Y0 = 140,
    TERM_X1 = 469,
    TERM_Y1 = 312,
    TERM_RADIUS = 8
};

typedef struct
{
    uint16_t replay_index;
    uint32_t epoch_ds;
    uint16_t glucose_mgdl;
    int16_t trend_x100;
    uint16_t pred_15m_mgdl;
    uint16_t pred_30m_mgdl;
    uint8_t confidence_pct;
    uint8_t sqi_pct;
    int16_t temp_c10;
    uint16_t chart_hist[CHART_MAX_POINTS];
    uint8_t chart_wr;
    uint8_t chart_count;
} cgm_stream_state_t;

static uint16_t s_edgeai_btn_pixels[EDGEAI_BTN_W * EDGEAI_BTN_H];
static uint16_t s_chart_panel_pixels[(CHART_X1 - CHART_X0 + 1) * (CHART_Y1 - CHART_Y0 + 1)];
static uint16_t s_term_panel_pixels[(TERM_X1 - TERM_X0 + 1) * (TERM_Y1 - TERM_Y0 + 1)];

static uint8_t color_r5_to_u8(uint16_t c)
{
    uint8_t v = (uint8_t)((c >> 11) & 0x1Fu);
    return (uint8_t)((v << 3) | (v >> 2));
}

static uint8_t color_g6_to_u8(uint16_t c)
{
    uint8_t v = (uint8_t)((c >> 5) & 0x3Fu);
    return (uint8_t)((v << 2) | (v >> 4));
}

static uint8_t color_b5_to_u8(uint16_t c)
{
    uint8_t v = (uint8_t)(c & 0x1Fu);
    return (uint8_t)((v << 3) | (v >> 2));
}

static uint16_t blend_rgb565(uint16_t dst, uint16_t src, uint8_t opa)
{
    uint16_t inv = (uint16_t)(255u - opa);
    uint16_t r = (uint16_t)(((uint16_t)color_r5_to_u8(dst) * inv + (uint16_t)color_r5_to_u8(src) * opa + 127u) / 255u);
    uint16_t g = (uint16_t)(((uint16_t)color_g6_to_u8(dst) * inv + (uint16_t)color_g6_to_u8(src) * opa + 127u) / 255u);
    uint16_t b = (uint16_t)(((uint16_t)color_b5_to_u8(dst) * inv + (uint16_t)color_b5_to_u8(src) * opa + 127u) / 255u);
    return RGB565(r, g, b);
}

static uint16_t cgm_color_from_confidence(uint8_t conf)
{
    if (conf >= 70u)
    {
        return RGB565(96, 255, 160);
    }
    if (conf >= 45u)
    {
        return RGB565(255, 214, 102);
    }
    return RGB565(255, 118, 118);
}

static uint16_t glucose_color_by_threshold(uint16_t mgdl)
{
    if ((mgdl <= 69u) || (mgdl >= 250u))
    {
        return RGB565(255, 84, 84);
    }
    if ((mgdl <= 79u) || (mgdl >= 181u))
    {
        return RGB565(255, 170, 72);
    }
    return RGB565(88, 255, 120);
}

static bool point_in_round_rect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t radius)
{
    if ((x < 0) || (y < 0) || (x >= w) || (y >= h))
    {
        return false;
    }
    if ((x >= radius) && (x < (w - radius)))
    {
        return true;
    }
    if ((y >= radius) && (y < (h - radius)))
    {
        return true;
    }

    int16_t cx = (x < radius) ? radius : (w - radius - 1);
    int16_t cy = (y < radius) ? radius : (h - radius - 1);
    int16_t dx = (int16_t)(x - cx);
    int16_t dy = (int16_t)(y - cy);
    int32_t rr = (int32_t)radius * (int32_t)radius;
    return ((int32_t)dx * (int32_t)dx + (int32_t)dy * (int32_t)dy) <= rr;
}

static uint16_t sample_bg_px(int16_t x, int16_t y)
{
    if ((x < 0) || (x >= PUMP_BG_WIDTH) || (y < 0) || (y >= PUMP_BG_HEIGHT))
    {
        return 0u;
    }
    return g_pump_bg_rgb565[(size_t)y * PUMP_BG_WIDTH + (size_t)x];
}

static void panel_blend_from_bg(uint16_t *dst,
                                int16_t x0,
                                int16_t y0,
                                int16_t w,
                                int16_t h,
                                int16_t radius,
                                uint16_t fill,
                                uint8_t fill_opa,
                                uint16_t border,
                                uint8_t border_opa,
                                uint16_t sheen,
                                uint8_t sheen_top_h)
{
    for (int16_t y = 0; y < h; ++y)
    {
        for (int16_t x = 0; x < w; ++x)
        {
            uint16_t px = sample_bg_px((int16_t)(x0 + x), (int16_t)(y0 + y));

            if (!point_in_round_rect(x, y, w, h, radius))
            {
                dst[(size_t)y * (size_t)w + (size_t)x] = px;
                continue;
            }

            px = blend_rgb565(px, fill, fill_opa);
            if (y < sheen_top_h)
            {
                uint8_t a = (uint8_t)(((sheen_top_h - y) * 20u) / sheen_top_h);
                px = blend_rgb565(px, sheen, a);
            }

            {
                bool border_px = !point_in_round_rect(x - 1, y, w, h, radius) ||
                                 !point_in_round_rect(x + 1, y, w, h, radius) ||
                                 !point_in_round_rect(x, y - 1, w, h, radius) ||
                                 !point_in_round_rect(x, y + 1, w, h, radius);
                if (border_px)
                {
                    px = blend_rgb565(px, border, border_opa);
                }
            }

            dst[(size_t)y * (size_t)w + (size_t)x] = px;
        }
    }
}

static void push_chart_point(cgm_stream_state_t *s, uint16_t glucose)
{
    s->chart_hist[s->chart_wr] = glucose;
    s->chart_wr = (uint8_t)((s->chart_wr + 1u) % CHART_MAX_POINTS);
    if (s->chart_count < CHART_MAX_POINTS)
    {
        s->chart_count++;
    }
}

static uint16_t chart_hist_get(const cgm_stream_state_t *s, uint8_t oldest_index)
{
    uint8_t start = (uint8_t)((s->chart_wr + CHART_MAX_POINTS - s->chart_count) % CHART_MAX_POINTS);
    uint8_t idx = (uint8_t)((start + oldest_index) % CHART_MAX_POINTS);
    return s->chart_hist[idx];
}

static void draw_chart_panel(const cgm_stream_state_t *s)
{
    const int16_t w = (CHART_X1 - CHART_X0 + 1);
    const int16_t h = (CHART_Y1 - CHART_Y0 + 1);
    const int16_t inner_x0 = CHART_X0 + 8;
    const int16_t inner_y0 = CHART_Y0 + 20;
    const int16_t inner_x1 = CHART_X1 - 8;
    const int16_t inner_y1 = CHART_Y1 - 10;
    const int16_t inner_w = (inner_x1 - inner_x0 + 1);
    const int16_t inner_h = (inner_y1 - inner_y0 + 1);
    const uint16_t axis = RGB565(150, 206, 236);
    const uint16_t grid = RGB565(90, 150, 190);
    uint16_t line = cgm_color_from_confidence(s->confidence_pct);

    panel_blend_from_bg(s_chart_panel_pixels,
                        CHART_X0,
                        CHART_Y0,
                        w,
                        h,
                        CHART_RADIUS,
                        RGB565(120, 180, 220),
                        34u,
                        RGB565(228, 244, 255),
                        74u,
                        RGB565(224, 244, 255),
                        14u);
    MedicalHal_BlitRgb565(CHART_X0, CHART_Y0, (uint16_t)w, (uint16_t)h, s_chart_panel_pixels);

    MedicalHal_DrawText(CHART_X0 + 10, CHART_Y0 + 6, "CGM STREAM", 1u, RGB565(220, 246, 255));
    for (int16_t gy = 1; gy <= 3; ++gy)
    {
        int16_t y = (int16_t)(inner_y0 + (gy * inner_h) / 4);
        MedicalHal_DrawLine(inner_x0, y, inner_x1, y, grid);
    }
    MedicalHal_DrawRect(inner_x0, inner_y0, inner_x1, inner_y1, axis);

    if (s->chart_count >= 2u)
    {
        int16_t prev_x = inner_x0;
        int16_t prev_y = inner_y1;
        uint8_t points = s->chart_count;
        for (uint8_t i = 0; i < points; ++i)
        {
            uint16_t g = chart_hist_get(s, i);
            int32_t gx = inner_x0 + ((int32_t)i * (inner_w - 1)) / (points - 1);
            int32_t gy = inner_y1 - (((int32_t)g - 40) * (inner_h - 1)) / (400 - 40);
            if (gy < inner_y0)
            {
                gy = inner_y0;
            }
            if (gy > inner_y1)
            {
                gy = inner_y1;
            }
            if (i != 0u)
            {
                MedicalHal_DrawLine(prev_x, prev_y, (int16_t)gx, (int16_t)gy, line);
            }
            prev_x = (int16_t)gx;
            prev_y = (int16_t)gy;
        }
        MedicalHal_FillRect((int16_t)(prev_x - 1), (int16_t)(prev_y - 1), (int16_t)(prev_x + 1), (int16_t)(prev_y + 1), RGB565(240, 255, 255));
    }
}

static uint8_t append_u32(char *dst, uint8_t pos, uint8_t max, uint32_t val)
{
    char tmp[11];
    uint8_t n = 0u;
    if (val == 0u)
    {
        if (pos < max)
        {
            dst[pos++] = '0';
        }
        return pos;
    }
    while ((val > 0u) && (n < sizeof(tmp)))
    {
        tmp[n++] = (char)('0' + (val % 10u));
        val /= 10u;
    }
    while (n > 0u)
    {
        if (pos < max)
        {
            dst[pos++] = tmp[--n];
        }
        else
        {
            --n;
        }
    }
    return pos;
}

static uint8_t append_i32(char *dst, uint8_t pos, uint8_t max, int32_t v)
{
    uint32_t mag = (uint32_t)((v < 0) ? -v : v);
    if ((v < 0) && (pos < max))
    {
        dst[pos++] = '-';
    }
    return append_u32(dst, pos, max, mag);
}

static void term_line_glucose(char *out, uint16_t g)
{
    uint8_t p = 0u;
    const char *head = "G ";
    while ((*head != '\0') && (p < 31u))
    {
        out[p++] = *head++;
    }
    p = append_u32(out, p, 31u, g);
    if (p < 31u) out[p++] = 'm';
    if (p < 31u) out[p++] = 'g';
    if (p < 31u) out[p++] = '/';
    if (p < 31u) out[p++] = 'd';
    if (p < 31u) out[p++] = 'L';
    out[p] = '\0';
}

static void term_line_pred(char *out, uint16_t p15, uint16_t p30)
{
    uint8_t p = 0u;
    const char *head = "P15 ";
    while ((*head != '\0') && (p < 31u))
    {
        out[p++] = *head++;
    }
    p = append_u32(out, p, 31u, p15);
    if (p < 31u) out[p++] = ' ';
    if (p < 31u) out[p++] = 'P';
    if (p < 31u) out[p++] = '3';
    if (p < 31u) out[p++] = '0';
    if (p < 31u) out[p++] = ' ';
    p = append_u32(out, p, 31u, p30);
    out[p] = '\0';
}

static void term_line_trend(char *out, int16_t trend_x100)
{
    uint8_t p = 0u;
    int32_t whole = trend_x100 / 100;
    int32_t frac = trend_x100 % 100;
    if (frac < 0)
    {
        frac = -frac;
    }
    out[p++] = 'T';
    out[p++] = ' ';
    p = append_i32(out, p, 31u, whole);
    if (p < 31u) out[p++] = '.';
    if (p < 31u) out[p++] = (char)('0' + ((frac / 10) % 10));
    out[p] = '\0';
}

static void term_line_meta(char *out, uint8_t conf, uint8_t sqi, uint16_t idx)
{
    uint8_t p = 0u;
    out[p++] = 'C';
    out[p++] = ' ';
    p = append_u32(out, p, 31u, conf);
    if (p < 31u) out[p++] = '%';
    if (p < 31u) out[p++] = ' ';
    if (p < 31u) out[p++] = 'S';
    if (p < 31u) out[p++] = ' ';
    p = append_u32(out, p, 31u, sqi);
    if (p < 31u) out[p++] = '%';
    if (p < 31u) out[p++] = ' ';
    if (p < 31u) out[p++] = '#';
    p = append_u32(out, p, 31u, idx);
    out[p] = '\0';
}

static void term_line_temp(char *out, int16_t temp_c10)
{
    uint8_t p = 0u;
    int32_t whole = temp_c10 / 10;
    int32_t frac = temp_c10 % 10;
    if (frac < 0)
    {
        frac = -frac;
    }
    out[p++] = 'T';
    out[p++] = ' ';
    p = append_i32(out, p, 31u, whole);
    if (p < 31u) out[p++] = '.';
    if (p < 31u) out[p++] = (char)('0' + (frac % 10));
    if (p < 31u) out[p++] = 'C';
    out[p] = '\0';
}

static void draw_terminal_panel(const cgm_stream_state_t *s, bool edgeai_enabled)
{
    const int16_t w = (TERM_X1 - TERM_X0 + 1);
    const int16_t h = (TERM_Y1 - TERM_Y0 + 1);
    uint16_t text = edgeai_enabled ? RGB565(170, 255, 196) : RGB565(255, 170, 170);
    char l0[32];
    char l1[32];
    char l2[32];
    char l3[32];
    char l4[32];

    panel_blend_from_bg(s_term_panel_pixels,
                        TERM_X0,
                        TERM_Y0,
                        w,
                        h,
                        TERM_RADIUS,
                        RGB565(114, 170, 214),
                        38u,
                        RGB565(228, 244, 255),
                        72u,
                        RGB565(224, 244, 255),
                        12u);
    MedicalHal_BlitRgb565(TERM_X0, TERM_Y0, (uint16_t)w, (uint16_t)h, s_term_panel_pixels);

    term_line_glucose(l0, s->glucose_mgdl);
    term_line_pred(l1, s->pred_15m_mgdl, s->pred_30m_mgdl);
    term_line_trend(l2, s->trend_x100);
    term_line_meta(l3, s->confidence_pct, s->sqi_pct, s->replay_index);
    term_line_temp(l4, s->temp_c10);

    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 8, edgeai_enabled ? "AI TERM ON" : "AI TERM OFF", 1u, text);
    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 26, l0, 1u, RGB565(218, 245, 255));
    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 40, l1, 1u, RGB565(218, 245, 255));
    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 54, l2, 1u, RGB565(218, 245, 255));
    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 68, l3, 1u, RGB565(218, 245, 255));
    MedicalHal_DrawText(TERM_X0 + 10, TERM_Y0 + 82, l4, 1u, RGB565(218, 245, 255));
}

static void draw_center_glucose_readout(const cgm_stream_state_t *s)
{
    char line[24];
    uint8_t p = 0u;
    uint16_t color = glucose_color_by_threshold(s->glucose_mgdl);
    int16_t y = 240;
    int16_t x;

    p = append_u32(line, p, 23u, s->glucose_mgdl);
    if (p < 23u) line[p++] = ' ';
    if (p < 23u) line[p++] = 'm';
    if (p < 23u) line[p++] = 'g';
    if (p < 23u) line[p++] = '/';
    if (p < 23u) line[p++] = 'd';
    if (p < 23u) line[p++] = 'L';
    line[p] = '\0';

    x = (int16_t)(240 - (((int16_t)strlen(line) * 6 * 2) / 2));
    MedicalHal_DrawText(x, y, line, 2u, color);
}

static void cgm_stream_init(cgm_stream_state_t *s, bool edgeai_enabled)
{
    memset(s, 0, sizeof(*s));
    s->glucose_mgdl = (CGM_REPLAY_SUBJECT001_LEN > 0u) ? kCgmReplaySubject001Mgdl[0] : 110u;
    s->pred_15m_mgdl = s->glucose_mgdl;
    s->pred_30m_mgdl = s->glucose_mgdl;
    s->sqi_pct = 85u;
    s->confidence_pct = 60u;
    s->temp_c10 = 250;
    PowerData_Init();
    CgmModel_Reset();
    CgmModel_SetEnabled(edgeai_enabled);
    push_chart_point(s, s->glucose_mgdl);
}

static void cgm_stream_step(cgm_stream_state_t *s, bool edgeai_enabled)
{
    cgm_model_features_t feat;
    uint32_t total_trace_ds;
    uint32_t base_idx;
    uint32_t frac_ds;
    uint16_t g_now;
    uint16_t g_prev = s->glucose_mgdl;
    uint16_t g0;
    uint16_t g1;
    int32_t interp;
    int32_t delta;
    uint16_t p15 = s->glucose_mgdl;
    uint16_t p30 = s->glucose_mgdl;
    uint8_t conf = 0u;
    bool ok;
    const power_sample_t *ps;

    if (CGM_REPLAY_SUBJECT001_LEN == 0u)
    {
        return;
    }

    total_trace_ds = (uint32_t)((CGM_REPLAY_SUBJECT001_LEN - 1u) * REPLAY_STEP_DS);
    s->epoch_ds += DATA_STREAM_ADVANCE_DS;
    if (total_trace_ds != 0u)
    {
        s->epoch_ds %= total_trace_ds;
    }

    base_idx = s->epoch_ds / REPLAY_STEP_DS;
    frac_ds = s->epoch_ds % REPLAY_STEP_DS;
    if (base_idx >= CGM_REPLAY_SUBJECT001_LEN)
    {
        base_idx = 0u;
    }
    s->replay_index = (uint16_t)base_idx;

    g0 = kCgmReplaySubject001Mgdl[base_idx];
    g1 = kCgmReplaySubject001Mgdl[(base_idx + 1u) % CGM_REPLAY_SUBJECT001_LEN];
    interp = (int32_t)g0 + (((int32_t)g1 - (int32_t)g0) * (int32_t)frac_ds) / (int32_t)REPLAY_STEP_DS;
    if (interp < 40)
    {
        interp = 40;
    }
    if (interp > 400)
    {
        interp = 400;
    }
    g_now = (uint16_t)interp;
    delta = (int32_t)g_now - (int32_t)g_prev;

    s->glucose_mgdl = g_now;
    s->trend_x100 = (int16_t)((delta * 60000) / (int32_t)DATA_STREAM_ADVANCE_DS);
    s->sqi_pct = (uint8_t)(85u - (uint8_t)((s->replay_index % 8u) * 3u));

    PowerData_Tick();
    ps = PowerData_Get();
    if (ps != NULL)
    {
        s->temp_c10 = (int16_t)((int16_t)ps->temp_c * 10);
    }

    feat.glucose_mgdl = s->glucose_mgdl;
    feat.trend_mgdl_min_x100 = s->trend_x100;
    feat.sqi_pct = s->sqi_pct;
    feat.sensor_flags = 0u;
    feat.epoch_ds = s->epoch_ds;

    CgmModel_SetEnabled(edgeai_enabled);
    ok = CgmModel_Predict(&feat, &p15, &p30, &conf);
    if (ok)
    {
        s->pred_15m_mgdl = p15;
        s->pred_30m_mgdl = p30;
        s->confidence_pct = conf;
    }
    else
    {
        s->pred_15m_mgdl = s->glucose_mgdl;
        s->pred_30m_mgdl = s->glucose_mgdl;
        s->confidence_pct = 0u;
    }

    push_chart_point(s, s->glucose_mgdl);
}

static void draw_edgeai_button(bool enabled)
{
    const uint16_t btn_fill = RGB565(143, 211, 255);  /* 0x8FD3FF */
    const uint16_t btn_border = RGB565(230, 244, 255); /* 0xE6F4FF */
    const uint16_t btn_shadow = RGB565(107, 190, 255); /* 0x6BBEFF */
    const uint16_t btn_highlight = RGB565(222, 242, 255);
    const uint16_t text_color = enabled ? RGB565(88, 255, 120) : RGB565(255, 84, 84);
    const uint8_t fill_opa = enabled ? 46u : 56u;
    const uint8_t border_opa = enabled ? 68u : 88u;
    const uint8_t shadow_opa = enabled ? 34u : 42u;
    const char *label = enabled ? "AI ON" : "AI OFF";
    const uint8_t text_scale = 2u;
    int16_t label_w = (int16_t)(((int32_t)strlen(label) * 6) * (int32_t)text_scale);
    int16_t label_h = (int16_t)(7 * text_scale);
    int16_t text_x = (int16_t)(EDGEAI_BTN_X0 + ((EDGEAI_BTN_W - label_w) / 2));
    int16_t text_y = (int16_t)(EDGEAI_BTN_Y0 + ((EDGEAI_BTN_H - label_h) / 2));

    for (int16_t y = 0; y < EDGEAI_BTN_H; ++y)
    {
        size_t src_off = (size_t)(EDGEAI_BTN_Y0 + y) * PUMP_BG_WIDTH + (size_t)EDGEAI_BTN_X0;
        memcpy(&s_edgeai_btn_pixels[(size_t)y * EDGEAI_BTN_W], &g_pump_bg_rgb565[src_off], (size_t)EDGEAI_BTN_W * sizeof(uint16_t));
    }

    for (int16_t y = 0; y < EDGEAI_BTN_H; ++y)
    {
        for (int16_t x = 0; x < EDGEAI_BTN_W; ++x)
        {
            uint16_t *px = &s_edgeai_btn_pixels[(size_t)y * EDGEAI_BTN_W + (size_t)x];

            if (!point_in_round_rect(x, y, EDGEAI_BTN_W, EDGEAI_BTN_H, EDGEAI_BTN_RADIUS))
            {
                continue;
            }

            *px = blend_rgb565(*px, btn_fill, fill_opa);

            /* Inner blue glow to emulate the HUD glass depth. */
            int16_t dx = (x < (EDGEAI_BTN_W / 2)) ? x : (int16_t)(EDGEAI_BTN_W - 1 - x);
            int16_t dy = (y < (EDGEAI_BTN_H / 2)) ? y : (int16_t)(EDGEAI_BTN_H - 1 - y);
            int16_t edge_d = (dx < dy) ? dx : dy;
            if (edge_d < 5)
            {
                uint8_t edge_alpha = (uint8_t)(((5 - edge_d) * shadow_opa) / 5);
                *px = blend_rgb565(*px, btn_shadow, edge_alpha);
            }

            /* Top sheen pass for liquid-glass feel. */
            if (y < 12)
            {
                uint8_t sheen = (uint8_t)(((12 - y) * 18u) / 12u);
                *px = blend_rgb565(*px, btn_highlight, sheen);
            }

            /* Border stroke with same rounded geometry. */
            bool border_px = !point_in_round_rect(x - 1, y, EDGEAI_BTN_W, EDGEAI_BTN_H, EDGEAI_BTN_RADIUS) ||
                             !point_in_round_rect(x + 1, y, EDGEAI_BTN_W, EDGEAI_BTN_H, EDGEAI_BTN_RADIUS) ||
                             !point_in_round_rect(x, y - 1, EDGEAI_BTN_W, EDGEAI_BTN_H, EDGEAI_BTN_RADIUS) ||
                             !point_in_round_rect(x, y + 1, EDGEAI_BTN_W, EDGEAI_BTN_H, EDGEAI_BTN_RADIUS);
            if (border_px)
            {
                *px = blend_rgb565(*px, btn_border, border_opa);
            }
        }
    }

    MedicalHal_BlitRgb565(EDGEAI_BTN_X0, EDGEAI_BTN_Y0, EDGEAI_BTN_W, EDGEAI_BTN_H, s_edgeai_btn_pixels);
    MedicalHal_DrawText(text_x, text_y, label, text_scale, text_color);
}

static bool point_in_rect(int16_t x, int16_t y, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
    return (x >= x0) && (x <= x1) && (y >= y0) && (y <= y1);
}

static void render_screen(bool edgeai_enabled, const cgm_stream_state_t *stream)
{
    MedicalLcd_DrawStaticLayout();
    draw_edgeai_button(edgeai_enabled);
    if (stream != NULL)
    {
        draw_chart_panel(stream);
        draw_terminal_panel(stream, edgeai_enabled);
        draw_center_glucose_readout(stream);
    }
    display_hal_present_frame();
}

void edgeai_insulin_pump_port_start(void)
{
    insulin_platform_touch_t touch;
    bool edgeai_enabled = true;
    cgm_stream_state_t stream;
    uint8_t stream_frame_div = 0u;

    insulin_platform_init();

    if (!display_hal_init())
    {
        for (;;)
        {
        }
    }

    display_hal_fill(0x0000u);
    cgm_stream_init(&stream, edgeai_enabled);
    render_screen(edgeai_enabled, &stream);

    for (;;)
    {
        uint32_t tick_start_us = insulin_platform_now_us();
        insulin_platform_touch_poll(&touch);

        if (touch.pressed_edge &&
            point_in_rect(touch.x, touch.y, EDGEAI_BTN_X0, EDGEAI_BTN_Y0, EDGEAI_BTN_X1, EDGEAI_BTN_Y1))
        {
            edgeai_enabled = !edgeai_enabled;
            CgmModel_SetEnabled(edgeai_enabled);
            render_screen(edgeai_enabled, &stream);
        }

        stream_frame_div++;
        if (stream_frame_div >= STREAM_STEP_FRAME_DIV)
        {
            stream_frame_div = 0u;
            cgm_stream_step(&stream, edgeai_enabled);
            render_screen(edgeai_enabled, &stream);
        }

        insulin_platform_sleep_until_next_tick_us(tick_start_us, PORT_FRAME_US);
    }
}
