#include "medical_lcd_layout.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "medical_display_hal.h"
#include "pump_bg.h"

#define RGB565(r, g, b) (uint16_t)((((uint16_t)(r) & 0xF8u) << 8) | (((uint16_t)(g) & 0xFCu) << 3) | (((uint16_t)(b) & 0xF8u) >> 3))

enum
{
    SCREEN_W = 480,
    SCREEN_H = 320,

    RIGHT_X = 324,
    GRAPH_X = 328,
    GRAPH_Y = 24,
    GRAPH_W = 148,
    GRAPH_H = 94,
    TERM_X = 328,
    TERM_Y = 124,
    TERM_W = 148,
    TERM_H = 172
};

static const uint16_t kPanel = RGB565(6, 16, 28);
static const uint16_t kPanelEdge = RGB565(80, 180, 255);
static const uint16_t kText = RGB565(220, 245, 255);
static const uint16_t kAccent = RGB565(96, 255, 170);
static const uint16_t kWarn = RGB565(255, 200, 70);
static const uint16_t kFault = RGB565(255, 96, 96);
static const uint16_t kBgShade = RGB565(0, 12, 22);

static void DrawFrame(void)
{
    MedicalHal_DrawRect(8, 8, SCREEN_W - 9, SCREEN_H - 9, kPanelEdge);
    MedicalHal_DrawRect(RIGHT_X, 12, SCREEN_W - 10, SCREEN_H - 12, kPanelEdge);
    MedicalHal_FillRect(GRAPH_X, GRAPH_Y, GRAPH_X + GRAPH_W, GRAPH_Y + GRAPH_H, kPanel);
    MedicalHal_FillRect(TERM_X, TERM_Y, TERM_X + TERM_W, TERM_Y + TERM_H, kPanel);
    MedicalHal_DrawRect(GRAPH_X, GRAPH_Y, GRAPH_X + GRAPH_W, GRAPH_Y + GRAPH_H, kPanelEdge);
    MedicalHal_DrawRect(TERM_X, TERM_Y, TERM_X + TERM_W, TERM_Y + TERM_H, kPanelEdge);
}

static void DrawMotorOverlay(const medical_lcd_state_t *state)
{
    char line[28];
    const int16_t base_x = 18;
    const int16_t base_y = 38;
    const int16_t bar_x0 = 16;
    const int16_t bar_y0 = 102;
    const int16_t bar_w = 152;
    int16_t fill_w = (int16_t)((state->motor_anomaly_pct * (uint16_t)(bar_w - 4)) / 100u);
    uint16_t status_color = state->motor_anomaly_pct >= 65u ? kFault : (state->motor_anomaly_pct >= 35u ? kWarn : kAccent);

    if (fill_w < 0)
    {
        fill_w = 0;
    }

    MedicalHal_FillRect(10, 30, 178, 122, kBgShade);
    MedicalHal_DrawRect(10, 30, 178, 122, kPanelEdge);
    MedicalHal_DrawText(base_x, base_y, "MOTOR", 1, kText);
    (void)snprintf(line, sizeof(line), "RPM %4u", (unsigned)state->motor_rpm);
    MedicalHal_DrawText(base_x, base_y + 16, line, 1, kText);
    (void)snprintf(line, sizeof(line), "CUR %4umA", (unsigned)state->motor_current_ma);
    MedicalHal_DrawText(base_x, base_y + 30, line, 1, kText);
    (void)snprintf(line, sizeof(line), "LOAD %3u%%", (unsigned)state->motor_load_pct);
    MedicalHal_DrawText(base_x, base_y + 44, line, 1, kText);
    MedicalHal_DrawText(base_x, base_y + 58, "ANOM", 1, status_color);
    MedicalHal_DrawRect(bar_x0, bar_y0, bar_x0 + bar_w, bar_y0 + 14, kPanelEdge);
    MedicalHal_FillRect(bar_x0 + 2, bar_y0 + 2, bar_x0 + 2 + fill_w, bar_y0 + 12, status_color);
}

static void DrawPumpOverlay(const medical_lcd_state_t *state)
{
    char line[30];
    const int16_t panel_x0 = 16;
    const int16_t panel_y0 = 178;
    const int16_t panel_x1 = 178;
    const int16_t panel_y1 = 300;
    const int16_t tank_x0 = 124;
    const int16_t tank_y0 = 196;
    const int16_t tank_x1 = 166;
    const int16_t tank_y1 = 288;
    const int16_t tank_h = tank_y1 - tank_y0 - 2;
    int16_t fill_h = (int16_t)((state->pump_fill_pct * (uint16_t)tank_h) / 100u);

    if (fill_h < 0)
    {
        fill_h = 0;
    }

    MedicalHal_FillRect(panel_x0, panel_y0, panel_x1, panel_y1, kBgShade);
    MedicalHal_DrawRect(panel_x0, panel_y0, panel_x1, panel_y1, kPanelEdge);
    MedicalHal_DrawText(panel_x0 + 8, panel_y0 + 8, "PUMP", 1, kText);
    (void)snprintf(line, sizeof(line), "RATE %3uMLH", (unsigned)state->pump_rate_ml_h);
    MedicalHal_DrawText(panel_x0 + 8, panel_y0 + 24, line, 1, kText);
    (void)snprintf(line, sizeof(line), "FILL %3u%%", (unsigned)state->pump_fill_pct);
    MedicalHal_DrawText(panel_x0 + 8, panel_y0 + 40, line, 1, kText);
    MedicalHal_DrawText(panel_x0 + 8,
                        panel_y0 + 56,
                        state->pump_active ? "STATE PUMPING" : "STATE IDLE",
                        1,
                        state->pump_active ? kAccent : kWarn);

    MedicalHal_DrawRect(tank_x0, tank_y0, tank_x1, tank_y1, kPanelEdge);
    MedicalHal_FillRect(tank_x0 + 1, tank_y0 + 1, tank_x1 - 1, tank_y1 - 1, RGB565(4, 20, 32));
    MedicalHal_FillRect(tank_x0 + 2, tank_y1 - 1 - fill_h, tank_x1 - 2, tank_y1 - 2, kAccent);
    if (state->pump_active)
    {
        MedicalHal_DrawLine(108, 236, 122, 236, kAccent);
        MedicalHal_DrawLine(108, 228, 122, 228, kAccent);
        MedicalHal_DrawLine(108, 244, 122, 244, kAccent);
    }
}

static void DrawHumanOverlay(const medical_lcd_state_t *state)
{
    char line[24];
    const int16_t cx = 240;
    const int16_t cy = 156;
    const int16_t ring_r = 84;
    float rad = ((float)state->gyro_angle_deg * 3.14159265f) / 180.0f;
    int16_t px = (int16_t)(cx + (int16_t)(cosf(rad) * (float)(ring_r - 6)));
    int16_t py = (int16_t)(cy - (int16_t)(sinf(rad) * (float)(ring_r - 6)));

    MedicalHal_DrawCircle(cx, cy, ring_r, kPanelEdge);
    MedicalHal_DrawLine(cx, cy, px, py, kAccent);
    MedicalHal_DrawCircle(px, py, 4, kAccent);

    (void)snprintf(line, sizeof(line), "SIM  %3dD", (int)state->gyro_angle_deg);
    MedicalHal_FillRect(188, 244, 318, 308, kBgShade);
    MedicalHal_DrawRect(188, 244, 318, 308, kPanelEdge);
    MedicalHal_DrawText(196, 254, line, 1, kText);
    MedicalHal_DrawText(196, 270, state->inverted ? "INV YES" : "INV NO", 1, state->inverted ? kWarn : kAccent);
    MedicalHal_DrawText(196, 286, state->drop_detected ? "DROP YES" : "DROP NO", 1, state->drop_detected ? kFault : kAccent);
}

static void DrawHeader(const medical_lcd_state_t *state)
{
    char elapsed[24];
    (void)snprintf(elapsed,
                   sizeof(elapsed),
                   "%04u:%02u:%02u",
                   (unsigned)state->elapsed_hh,
                   (unsigned)state->elapsed_mm,
                   (unsigned)state->elapsed_ss);

    MedicalHal_DrawText(166, 8, "EdgeAI", 2, kText);
    MedicalHal_DrawText(186, 286, elapsed, 2, kAccent);
}

static void DrawGraph(const medical_lcd_state_t *state)
{
    uint8_t i;

    MedicalHal_DrawText(GRAPH_X + 8, GRAPH_Y + 6, "GRAPH", 1, kText);
    MedicalHal_DrawLine(GRAPH_X + 8, GRAPH_Y + GRAPH_H - 14, GRAPH_X + GRAPH_W - 8, GRAPH_Y + GRAPH_H - 14, kPanelEdge);

    for (i = 1u; i < state->graph_count; i++)
    {
        int16_t x0 = (int16_t)(GRAPH_X + 10 + ((i - 1u) * (GRAPH_W - 20)) / 95u);
        int16_t x1 = (int16_t)(GRAPH_X + 10 + (i * (GRAPH_W - 20)) / 95u);
        int16_t y0 = (int16_t)(GRAPH_Y + GRAPH_H - 16 - ((state->graph_points[i - 1u] * (GRAPH_H - 24)) / 1000u));
        int16_t y1 = (int16_t)(GRAPH_Y + GRAPH_H - 16 - ((state->graph_points[i] * (GRAPH_H - 24)) / 1000u));
        MedicalHal_DrawLine(x0, y0, x1, y1, kAccent);
    }
}

static void DrawTerminal(const medical_lcd_state_t *state)
{
    char line[40];

    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 6, "TERMINAL", 1, kText);

    (void)snprintf(line, sizeof(line), "M RPM  %4u", (unsigned)state->motor_rpm);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 26, line, 1, kText);

    (void)snprintf(line, sizeof(line), "M CUR  %4u mA", (unsigned)state->motor_current_ma);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 42, line, 1, kText);

    (void)snprintf(line, sizeof(line), "M LOAD %3u %%", (unsigned)state->motor_load_pct);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 58, line, 1, kText);

    (void)snprintf(line, sizeof(line), "PUMP   %3u MLH", (unsigned)state->pump_rate_ml_h);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 74, line, 1, kText);

    (void)snprintf(line,
                   sizeof(line),
                   "TEMP   %d.%d C",
                   (int)(state->temp_c10 / 10),
                   (int)((state->temp_c10 < 0 ? -state->temp_c10 : state->temp_c10) % 10));
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 90, line, 1, kText);

    (void)snprintf(line, sizeof(line), "SCORE  %3u %%", (unsigned)state->anomaly_score_pct);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 106, line, 1, kText);

    (void)snprintf(line, sizeof(line), "ANOM   %3u %%", (unsigned)state->motor_anomaly_pct);
    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 122, line, 1, state->motor_anomaly_pct >= 65u ? kFault : kText);

    MedicalHal_DrawText(TERM_X + 8, TERM_Y + 138, state->ai_enabled ? "AI     ON" : "AI     OFF", 1, state->ai_enabled ? kAccent : kWarn);
}

void MedicalLcd_DrawStaticLayout(void)
{
    medical_lcd_state_t init_state;

    (void)memset(&init_state, 0, sizeof(init_state));
    init_state.ai_enabled = true;

    MedicalHal_BlitRgb565(0, 0, PUMP_BG_WIDTH, PUMP_BG_HEIGHT, g_pump_bg_rgb565);
    DrawFrame();
    DrawMotorOverlay(&init_state);
    DrawPumpOverlay(&init_state);
    DrawHumanOverlay(&init_state);
    DrawHeader(&init_state);
}

void MedicalLcd_DrawDynamic(const medical_lcd_state_t *state)
{
    if (state == NULL)
    {
        return;
    }

    DrawHeader(state);
    DrawMotorOverlay(state);
    DrawPumpOverlay(state);
    DrawHumanOverlay(state);
    DrawGraph(state);
    DrawTerminal(state);
}
