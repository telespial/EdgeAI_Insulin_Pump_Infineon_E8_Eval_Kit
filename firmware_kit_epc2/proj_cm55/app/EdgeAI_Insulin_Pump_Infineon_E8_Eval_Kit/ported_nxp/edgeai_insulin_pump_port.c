#include "edgeai_insulin_pump_port.h"

#include <string.h>

#include "anomaly_engine.h"
#include "cgm_preprocess.h"
#include "medical_display_hal.h"
#include "medical_lcd_layout.h"
#include "power_data_source.h"
#include "../platform/insulin_platform.h"
#include "../platform/display_hal.h"

#define PORT_FRAME_US 100000u
#define UI_BTN_Y0 0
#define UI_BTN_Y1 22

typedef struct
{
    int16_t x0;
    int16_t x1;
} ui_btn_range_t;

static const ui_btn_range_t kBtnAi = {8, 108};
static const ui_btn_range_t kBtnMode = {118, 248};
static const ui_btn_range_t kBtnProfile = {258, 388};
static const ui_btn_range_t kBtnReset = {398, 472};

static bool point_in_btn(int16_t x, int16_t y, const ui_btn_range_t *btn)
{
    return (btn != 0) && (x >= btn->x0) && (x <= btn->x1) && (y >= UI_BTN_Y0) && (y <= UI_BTN_Y1);
}

static void draw_btn(const ui_btn_range_t *btn, const char *label, uint16_t fill, uint16_t edge, uint16_t text)
{
    if ((btn == 0) || (label == 0))
    {
        return;
    }

    MedicalHal_FillRect(btn->x0, UI_BTN_Y0, btn->x1, UI_BTN_Y1, fill);
    MedicalHal_DrawRect(btn->x0, UI_BTN_Y0, btn->x1, UI_BTN_Y1, edge);
    MedicalHal_DrawText((int16_t)(btn->x0 + 4), (int16_t)(UI_BTN_Y0 + 7), label, 1u, text);
}

static void draw_controls(bool ai_enabled, power_data_source_mode_t mode, power_replay_profile_t profile)
{
    const uint16_t bg = 0x0000u;
    const uint16_t edge = 0xFFFFu;
    const uint16_t txt = 0xFFFFu;
    const uint16_t on = 0x07E0u;
    const uint16_t off = 0x39E7u;
    const uint16_t mode_replay = 0x051Fu;
    const uint16_t profile_outlet = 0xFD20u;

    MedicalHal_FillRect(0, 0, 479, 24, bg);
    draw_btn(&kBtnAi, ai_enabled ? "AI ON" : "AI OFF", ai_enabled ? on : off, edge, txt);
    draw_btn(&kBtnMode, (mode == POWER_DATA_SOURCE_REPLAY) ? "MODE REPLAY" : "MODE LIVE", (mode == POWER_DATA_SOURCE_REPLAY) ? mode_replay : off, edge, txt);
    draw_btn(&kBtnProfile, (profile == POWER_REPLAY_PROFILE_OUTLET) ? "PROFILE OUTLET" : "PROFILE WIRED", (profile == POWER_REPLAY_PROFILE_OUTLET) ? profile_outlet : off, edge, txt);
    draw_btn(&kBtnReset, "RESET", off, edge, txt);
}

static void map_power_to_lcd(const power_sample_t *sample,
                             const anomaly_output_t *anom,
                             bool ai_enabled,
                             medical_lcd_state_t *state)
{
    uint8_t graph_idx;

    state->motor_rpm = (uint16_t)(1200u + (sample->current_mA / 6u));
    state->motor_current_ma = sample->current_mA;
    state->motor_load_pct = (uint16_t)((sample->power_mW > 0u) ? ((sample->power_mW / 150u) % 100u) : 0u);
    state->motor_anomaly_pct = sample->anomaly_score_pct;
    state->pump_rate_ml_h = (uint16_t)(10u + (sample->current_mA / 300u));
    state->pump_fill_pct = sample->soc_pct;
    state->pump_active = (sample->current_mA > 200u);
    state->temp_c10 = (int16_t)(sample->temp_c * 10);
    state->anomaly_score_pct = sample->anomaly_score_pct;
    state->ai_enabled = ai_enabled;
    state->gyro_angle_deg = (int16_t)(((sample->elapsed_charge_sim_s / 4u) % 360u));
    state->inverted = (anom->overall_level >= ANOMALY_LEVEL_MAJOR);
    state->drop_detected = (anom->overall_level >= ANOMALY_LEVEL_MINOR);

    state->graph_count = 16u;
    for (graph_idx = 0u; graph_idx < state->graph_count; graph_idx++)
    {
        uint16_t base = (uint16_t)(sample->power_mW / 50u);
        state->graph_points[graph_idx] = (uint16_t)(base + (graph_idx * 6u));
    }
}

void edgeai_insulin_pump_port_start(void)
{
    power_sample_t const *sample;
    anomaly_output_t anom;
    cgm_preprocess_t cgm;
    medical_lcd_state_t state;
    insulin_platform_touch_t touch;
    bool ai_enabled = true;
    power_data_source_mode_t mode = POWER_DATA_SOURCE_REPLAY;
    power_replay_profile_t profile = POWER_REPLAY_PROFILE_WIRED;

    memset(&state, 0, sizeof(state));
    insulin_platform_init();

    if (!display_hal_init())
    {
        for (;;)
        {
        }
    }

    display_hal_fill(0x0000u);

    PowerData_Init();
    PowerData_SetMode(mode);
    PowerData_SetReplayProfile(profile);
    PowerData_SetAiAssistEnabled(ai_enabled);
    AnomalyEngine_Init();
    CgmPreprocess_InitDefault(&cgm);
    CgmModel_SetEnabled(ai_enabled);

    MedicalLcd_DrawStaticLayout();
    draw_controls(ai_enabled, mode, profile);
    display_hal_present_frame();

    for (;;)
    {
        uint32_t tick_start_us = insulin_platform_now_us();

        insulin_platform_touch_poll(&touch);
        if (touch.pressed_edge)
        {
            if (point_in_btn(touch.x, touch.y, &kBtnAi))
            {
                ai_enabled = !ai_enabled;
                CgmModel_SetEnabled(ai_enabled);
                PowerData_SetAiAssistEnabled(ai_enabled);
            }
            else if (point_in_btn(touch.x, touch.y, &kBtnMode))
            {
                mode = (mode == POWER_DATA_SOURCE_REPLAY) ? POWER_DATA_SOURCE_LIVE_OVERRIDE : POWER_DATA_SOURCE_REPLAY;
                PowerData_SetMode(mode);
            }
            else if (point_in_btn(touch.x, touch.y, &kBtnProfile))
            {
                profile = (profile == POWER_REPLAY_PROFILE_WIRED) ? POWER_REPLAY_PROFILE_OUTLET : POWER_REPLAY_PROFILE_WIRED;
                PowerData_SetReplayProfile(profile);
            }
            else if (point_in_btn(touch.x, touch.y, &kBtnReset))
            {
                PowerData_SetReplayHour(0u);
            }
        }

        PowerData_Tick();
        sample = PowerData_Get();

        AnomalyEngine_Update((int16_t)sample->current_mA,
                             (int16_t)sample->power_mW,
                             (int16_t)sample->voltage_mV,
                             (int16_t)(sample->temp_c * 10));
        AnomalyEngine_GetOutput(&anom);

        state.elapsed_hh = (uint16_t)((sample->elapsed_charge_sim_s / 3600u) % 10000u);
        state.elapsed_mm = (uint8_t)((sample->elapsed_charge_sim_s / 60u) % 60u);
        state.elapsed_ss = (uint8_t)(sample->elapsed_charge_sim_s % 60u);
        state.elapsed_ds = (uint8_t)((sample->elapsed_charge_s * 10u) % 10u);

        map_power_to_lcd(sample, &anom, ai_enabled, &state);
        MedicalLcd_DrawDynamic(&state);
        draw_controls(ai_enabled, mode, profile);
        display_hal_present_frame();

        insulin_platform_sleep_until_next_tick_us(tick_start_us, PORT_FRAME_US);
    }
}
