#include "edgeai_insulin_pump_app.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "controller_openaps.h"
#include "cgm_model_runtime.h"
#include "cgm_replay_subject001.h"
#include "predictor_v2.h"
#include "lvgl.h"
#include "safety_supervisor.h"
#include "pump_background_image_rgb565.h"

enum
{
    CGM_GRAPH_POINTS = 32u,
    CGM_REPLAY_STEP_MS = 1400u,
    CGM_REPLAY_SAMPLE_MINUTES = 5u,
    BAR_GRAPH_COUNT = 3u,
};

typedef struct
{
    uint32_t sample_index;
    lv_obj_t *chart;
    lv_chart_series_t *glucose_series;
    lv_chart_series_t *prediction_series;
    lv_obj_t *prediction_label;
    lv_obj_t *prediction_accuracy_label;
    lv_obj_t *glucose_unit_label;
    lv_obj_t *glucose_title_label;
    lv_obj_t *glucose_label;
    lv_obj_t *glucose_shadow_label;
    lv_obj_t *status_labels[BAR_GRAPH_COUNT];
    lv_obj_t *status_bars[BAR_GRAPH_COUNT];
    lv_timer_t *timer;
} cgm_dashboard_t;

static cgm_dashboard_t gDashboard;

#if defined(APP_APS_SIDECAR_DEMO) && (APP_APS_SIDECAR_DEMO == 1)
#define APS_SIDECAR_DEMO_PERIOD_MS 5000u
#define APS_SIDECAR_DEMO_SEQUENCE_LEN 8u

typedef struct
{
    bool initialized;
    uint32_t last_service_ms;
    uint32_t sequence_index;
    lv_obj_t *panel;
    lv_obj_t *status_label;
} aps_sidecar_demo_t;

static aps_sidecar_demo_t gApsSidecar;

static const uint16_t kApsSidecarDemoBgMgdl[APS_SIDECAR_DEMO_SEQUENCE_LEN] = {110u, 125u, 145u, 160u, 150u, 130u, 105u, 90u};
#endif

static const char *action_to_string(aps_action_t action)
{
    switch (action)
    {
        case APS_ACTION_NO_CHANGE:
            return "NO_CHANGE";
        case APS_ACTION_REDUCE_BASAL:
            return "REDUCE";
        case APS_ACTION_SUSPEND_BASAL:
            return "SUSPEND";
        case APS_ACTION_INCREASE_BASAL:
            return "INCREASE";
        case APS_ACTION_CORRECTION_SUGGESTION:
            return "CORRECTION";
        default:
            return "UNKNOWN";
    }
}

#if defined(APP_APS_EMBEDDED_PROBE) && (APP_APS_EMBEDDED_PROBE == 1)
static bool gApsProbeRan;

void ApsEmbeddedProbe_RunOnce(void)
{
    predictor_v2_input_t input = {0};
    predictor_v2_output_t prediction = {0};
    aps_controller_output_t command = {0};
    bool prediction_ok;
    bool controller_ok;
    bool safety_ok;

    input.cgm.epoch_s = 1u;
    input.cgm.age_s = 0u;
    input.cgm.glucose_mgdl = 120u;
    input.cgm.trend_mgdl_min_x100 = 0;
    input.cgm.sqi_pct = 95u;
    input.cgm.sensor_flags = 0u;
    input.cgm.valid = true;

    input.physiology.iob_u = 0.5f;
    input.physiology.insulin_activity_u_per_hr = 0.0f;
    input.physiology.cob_g = 10.0f;
    input.physiology.carb_absorption_g_per_hr = 0.0f;
    input.physiology.basal_u_per_hr = 0.8f;
    input.physiology.insulin_30m_u = 0.0f;
    input.physiology.insulin_120m_u = 0.0f;
    input.physiology.carbs_30m_g = 0.0f;
    input.physiology.carbs_120m_g = 0.0f;
    input.physiology.minutes_since_bolus = 0u;
    input.physiology.minutes_since_meal = 0u;
    input.physiology.activity_state = (uint8_t)ACTIVITY_SEDENTARY;
    input.physiology.activity_confidence_pct = 95u;
    input.physiology.motion_rms_5m = 0.0f;
    input.physiology.motion_rms_15m = 0.0f;
    input.physiology.active_minutes = 0u;
    input.physiology.post_exercise_minutes = 0u;
    input.physiology_present = true;

    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    OpenApsController_Reset();

    prediction_ok = PredictorV2_Update(&input, &prediction);
    controller_ok = prediction_ok && OpenApsController_DetermineBasal(&input, &prediction, &command);
    safety_ok = controller_ok && SafetySupervisor_Apply(1u, &input, &prediction, &command);

    printf("APS probe: BG=120 P15=%u P30=%u P60=%u ACTION=%s SAFETY=0x%08lX OK=%u%u%u\n",
           (unsigned int)prediction.pred_15m_mgdl,
           (unsigned int)prediction.pred_30m_mgdl,
           (unsigned int)prediction.pred_60m_mgdl,
           action_to_string(command.action),
           (unsigned long)command.reason_flags,
           prediction_ok ? 1u : 0u,
           controller_ok ? 1u : 0u,
           safety_ok ? 1u : 0u);

    gApsProbeRan = true;
}
#endif

#if defined(APP_APS_SIDECAR_DEMO) && (APP_APS_SIDECAR_DEMO == 1)
static uint16_t aps_sidecar_demo_bg_at(uint32_t index)
{
    return kApsSidecarDemoBgMgdl[index % APS_SIDECAR_DEMO_SEQUENCE_LEN];
}

static void aps_sidecar_build_input(uint32_t sequence_index, predictor_v2_input_t *input)
{
    uint16_t glucose_mgdl = aps_sidecar_demo_bg_at(sequence_index);

    if (input == NULL)
    {
        return;
    }

    *input = (predictor_v2_input_t){0};
    input->cgm.epoch_s = 1000u + sequence_index * 300u;
    input->cgm.age_s = 0u;
    input->cgm.glucose_mgdl = glucose_mgdl;
    input->cgm.trend_mgdl_min_x100 = 0;
    input->cgm.sqi_pct = 95u;
    input->cgm.sensor_flags = 0u;
    input->cgm.valid = true;

    input->physiology.iob_u = 0.5f;
    input->physiology.insulin_activity_u_per_hr = 0.0f;
    input->physiology.cob_g = 10.0f;
    input->physiology.carb_absorption_g_per_hr = 0.0f;
    input->physiology.basal_u_per_hr = 0.8f;
    input->physiology.insulin_30m_u = 0.0f;
    input->physiology.insulin_120m_u = 0.0f;
    input->physiology.carbs_30m_g = 0.0f;
    input->physiology.carbs_120m_g = 0.0f;
    input->physiology.minutes_since_bolus = 0u;
    input->physiology.minutes_since_meal = 0u;
    input->physiology.activity_state = (uint8_t)ACTIVITY_SEDENTARY;
    input->physiology.activity_confidence_pct = 95u;
    input->physiology.motion_rms_5m = 0.0f;
    input->physiology.motion_rms_15m = 0.0f;
    input->physiology.active_minutes = 0u;
    input->physiology.post_exercise_minutes = 0u;
    input->physiology_present = true;
}

static void aps_sidecar_update_label(uint16_t glucose_mgdl,
                                    const predictor_v2_output_t *prediction,
                                    const aps_controller_output_t *command,
                                    bool safety_ok)
{
    char buffer[192];
    const char *safety_text = safety_ok ? "NORMAL" : "BLOCKED";
    const char *action_text = (command != NULL) ? action_to_string(command->action) : "UNKNOWN";
    uint16_t pred_15m = (prediction != NULL) ? prediction->pred_15m_mgdl : glucose_mgdl;
    uint16_t pred_30m = (prediction != NULL) ? prediction->pred_30m_mgdl : glucose_mgdl;
    uint16_t pred_60m = (prediction != NULL) ? prediction->pred_60m_mgdl : glucose_mgdl;

    if (gApsSidecar.status_label == NULL)
    {
        return;
    }

    snprintf(buffer,
             sizeof(buffer),
             "APS DEMO\nBG: %u  P15: %u\nP30: %u  P60: %u\nACT: %s  SAFE: %s",
             (unsigned int)glucose_mgdl,
             (unsigned int)pred_15m,
             (unsigned int)pred_30m,
             (unsigned int)pred_60m,
             action_text,
             safety_text);
    lv_label_set_text(gApsSidecar.status_label, buffer);
}

void ApsSidecar_Service(uint32_t now_ms)
{
    predictor_v2_input_t input;
    predictor_v2_output_t prediction = {0};
    aps_controller_output_t command = {0};
    bool prediction_ok;
    bool controller_ok;
    bool safety_ok;
    uint16_t glucose_mgdl;
    uint32_t elapsed_ms;

    if (!gApsSidecar.initialized)
    {
        return;
    }

    if (gApsSidecar.last_service_ms == 0u)
    {
        elapsed_ms = APS_SIDECAR_DEMO_PERIOD_MS;
    }
    else if (now_ms >= gApsSidecar.last_service_ms)
    {
        elapsed_ms = now_ms - gApsSidecar.last_service_ms;
    }
    else
    {
        elapsed_ms = APS_SIDECAR_DEMO_PERIOD_MS;
    }

    if (elapsed_ms < APS_SIDECAR_DEMO_PERIOD_MS)
    {
        return;
    }

    gApsSidecar.last_service_ms = now_ms;
    glucose_mgdl = aps_sidecar_demo_bg_at(gApsSidecar.sequence_index);
    aps_sidecar_build_input(gApsSidecar.sequence_index, &input);

    prediction_ok = PredictorV2_Update(&input, &prediction);
    controller_ok = prediction_ok && OpenApsController_DetermineBasal(&input, &prediction, &command);
    safety_ok = controller_ok && SafetySupervisor_Apply(now_ms / 1000u, &input, &prediction, &command);

    printf("APS sidecar: BG=%u P15=%u P30=%u P60=%u ACTION=%s SAFE=0x%08lX OK=%u%u%u\n",
           (unsigned int)glucose_mgdl,
           (unsigned int)prediction.pred_15m_mgdl,
           (unsigned int)prediction.pred_30m_mgdl,
           (unsigned int)prediction.pred_60m_mgdl,
           action_to_string(command.action),
           (unsigned long)command.reason_flags,
           prediction_ok ? 1u : 0u,
           controller_ok ? 1u : 0u,
           safety_ok ? 1u : 0u);

    aps_sidecar_update_label(glucose_mgdl, &prediction, &command, safety_ok);
    gApsSidecar.sequence_index = (gApsSidecar.sequence_index + 1u) % APS_SIDECAR_DEMO_SEQUENCE_LEN;
}

void ApsSidecar_Init(void)
{
    lv_obj_t *screen;
    lv_obj_t *panel;
    lv_obj_t *label;
    uint32_t now_ms;

    if (gApsSidecar.initialized)
    {
        return;
    }

    screen = lv_screen_active();
    if (screen == NULL)
    {
        return;
    }

    panel = lv_obj_create(screen);
    if (panel != NULL)
    {
        lv_obj_set_size(panel, 180, 114);
        lv_obj_align(panel, LV_ALIGN_BOTTOM_LEFT, 16, -16);
        lv_obj_set_style_radius(panel, 14, 0);
        lv_obj_set_style_border_width(panel, 2, 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x78D8FF), 0);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x08111A), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_80, 0);
        lv_obj_set_style_pad_left(panel, 10, 0);
        lv_obj_set_style_pad_right(panel, 10, 0);
        lv_obj_set_style_pad_top(panel, 8, 0);
        lv_obj_set_style_pad_bottom(panel, 8, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        label = lv_label_create(panel);
        if (label != NULL)
        {
            gApsSidecar.status_label = label;
            lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label, LV_PCT(100));
            lv_obj_set_style_text_color(label, lv_color_hex(0xEAF6FF), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
        }
        gApsSidecar.panel = panel;
    }

    gApsSidecar.initialized = true;
    PredictorV2_Reset();
    PredictorV2_SetEnabled(true);
    OpenApsController_Reset();
    now_ms = lv_tick_get();
    gApsSidecar.last_service_ms = (now_ms > APS_SIDECAR_DEMO_PERIOD_MS) ? (now_ms - APS_SIDECAR_DEMO_PERIOD_MS) : 0u;
    ApsSidecar_Service(now_ms);
}
#endif

static uint16_t replay_glucose_at(uint32_t index)
{
    return kCgmReplaySubject001Mgdl[index % CGM_REPLAY_SUBJECT001_LEN];
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum);

static lv_color_t glucose_status_color(uint16_t glucose_mgdl)
{
    if ((glucose_mgdl < 70u) || (glucose_mgdl > 250u))
    {
        return lv_color_hex(0xFF5A5A);
    }
    if ((glucose_mgdl < 80u) || (glucose_mgdl > 180u))
    {
        return lv_color_hex(0xFFD45A);
    }
    return lv_color_hex(0x58E07C);
}

static lv_color_t metric_bar_color(int32_t percent)
{
    if (percent < 85)
    {
        return lv_color_hex(0xFF5A5A);
    }
    if (percent < 90)
    {
        return lv_color_hex(0xFFD45A);
    }
    return lv_color_hex(0x58E07C);
}

static void style_prediction_label(lv_obj_t *label)
{
    if (label == NULL)
    {
        return;
    }

    lv_obj_set_style_text_color(label, lv_color_hex(0x79D8FF), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_shadow_color(label, lv_color_hex(0xFF4A6A), 0);
    lv_obj_set_style_shadow_opa(label, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(label, 8, 0);
    lv_obj_set_style_shadow_ofs_x(label, 0, 0);
    lv_obj_set_style_shadow_ofs_y(label, 0, 0);
}

static int16_t estimate_trend_x100(uint32_t index)
{
    int32_t current;
    int32_t delta;
    int32_t accum = 0;
    uint32_t count = 0u;
    uint32_t lag;

    if (index == 0u)
    {
        return 0;
    }

    current = (int32_t)replay_glucose_at(index);
    for (lag = 1u; lag <= 3u; ++lag)
    {
        if (index < lag)
        {
            break;
        }

        delta = current - (int32_t)replay_glucose_at(index - lag);
        accum += (delta * 100) / ((int32_t)lag * (int32_t)CGM_REPLAY_SAMPLE_MINUTES);
        count++;
    }

    if (count == 0u)
    {
        return 0;
    }

    return (int16_t)clamp_i32(accum / (int32_t)count, -2500, 2500);
}

static bool predict_glucose_from_model(uint32_t sample_index,
                                       uint16_t current_mgdl,
                                       uint16_t *predicted_15m_mgdl,
                                       uint8_t *confidence_pct)
{
    cgm_model_features_t features;
    uint16_t pred_30m_mgdl;
    uint8_t model_confidence;

    if ((predicted_15m_mgdl == NULL) || (confidence_pct == NULL))
    {
        return false;
    }

    features.glucose_mgdl = current_mgdl;
    features.trend_mgdl_min_x100 = estimate_trend_x100(sample_index);
    features.sqi_pct = 92u;
    features.sensor_flags = 0u;
    features.epoch_ds = sample_index * (uint32_t)CGM_REPLAY_SAMPLE_MINUTES * 600u;

    if (CgmModel_Predict(&features, predicted_15m_mgdl, &pred_30m_mgdl, &model_confidence))
    {
        *confidence_pct = model_confidence;
        return true;
    }

    *predicted_15m_mgdl = replay_glucose_at(sample_index + 4u);
    *confidence_pct = 70u;
    return false;
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum)
    {
        return minimum;
    }

    if (value > maximum)
    {
        return maximum;
    }

    return value;
}

static int32_t map_range_i32(int32_t value, int32_t in_min, int32_t in_max, int32_t out_min, int32_t out_max)
{
    int32_t clamped = clamp_i32(value, in_min, in_max);

    return out_min + ((clamped - in_min) * (out_max - out_min)) / (in_max - in_min);
}

static int32_t calculate_prediction_accuracy_percent(uint16_t current_mgdl, uint16_t predicted_mgdl)
{
    int32_t accuracy;

    accuracy = 100 - ((int32_t)abs((int32_t)predicted_mgdl - (int32_t)current_mgdl) * 100) / 150;
    return clamp_i32(accuracy, 0, 100);
}

static void update_status_bars(uint16_t current_mgdl, uint16_t predicted_mgdl, uint8_t confidence_pct)
{
    int32_t accuracy;
    uint32_t i;

    if (gDashboard.status_bars[0u] == NULL)
    {
        return;
    }

    accuracy = calculate_prediction_accuracy_percent(current_mgdl, predicted_mgdl);

    lv_bar_set_value(gDashboard.status_bars[0u], map_range_i32((int32_t)current_mgdl, 40, 400, 12, 100), LV_ANIM_OFF);
    lv_bar_set_value(gDashboard.status_bars[1u], confidence_pct, LV_ANIM_OFF);
    lv_bar_set_value(gDashboard.status_bars[2u], accuracy, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(gDashboard.status_bars[1u], metric_bar_color(confidence_pct), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(gDashboard.status_bars[2u], metric_bar_color(accuracy), LV_PART_INDICATOR);

    for (i = 0u; i < BAR_GRAPH_COUNT; ++i)
    {
        lv_obj_invalidate(gDashboard.status_bars[i]);
    }
}

static void update_chart_colors(uint16_t current_mgdl)
{
    if ((gDashboard.chart == NULL) || (gDashboard.glucose_series == NULL) || (gDashboard.prediction_series == NULL))
    {
        return;
    }

    lv_chart_set_series_color(gDashboard.chart, gDashboard.glucose_series, glucose_status_color(current_mgdl));
    lv_chart_set_series_color(gDashboard.chart, gDashboard.prediction_series, lv_color_hex(0x4CC7FF));
}

static void update_glucose_label(uint16_t current_mgdl)
{
    char buffer[48];

    if (gDashboard.glucose_label == NULL)
    {
        return;
    }

    snprintf(buffer, sizeof(buffer), "%u", (unsigned int)current_mgdl);
    if (gDashboard.glucose_shadow_label != NULL)
    {
        lv_label_set_text(gDashboard.glucose_shadow_label, buffer);
    }
    lv_label_set_text(gDashboard.glucose_label, buffer);
}

static void push_sample(uint16_t current_mgdl)
{
    uint16_t predicted_mgdl = current_mgdl;
    uint8_t confidence_pct = 0u;
    int32_t accuracy_pct;
    char header_buffer[64];

    (void)predict_glucose_from_model(gDashboard.sample_index, current_mgdl, &predicted_mgdl, &confidence_pct);
    accuracy_pct = calculate_prediction_accuracy_percent(current_mgdl, predicted_mgdl);
    update_glucose_label(current_mgdl);
    update_status_bars(current_mgdl, predicted_mgdl, confidence_pct);
    if (gDashboard.prediction_accuracy_label != NULL)
    {
        snprintf(header_buffer, sizeof(header_buffer), "%d%%", (int)accuracy_pct);
        lv_label_set_text(gDashboard.prediction_accuracy_label, header_buffer);
    }

    if ((gDashboard.chart != NULL) && (gDashboard.glucose_series != NULL))
    {
        lv_chart_set_next_value(gDashboard.chart, gDashboard.glucose_series, (int32_t)current_mgdl);
        if (gDashboard.prediction_series != NULL)
        {
            lv_chart_set_next_value(gDashboard.chart, gDashboard.prediction_series, (int32_t)predicted_mgdl);
        }
        update_chart_colors(current_mgdl);
        lv_chart_refresh(gDashboard.chart);
    }

    gDashboard.sample_index++;
}

static void seed_chart(void)
{
    uint32_t i;
    uint32_t seed_count = CGM_GRAPH_POINTS;

    if (seed_count > CGM_REPLAY_SUBJECT001_LEN)
    {
        seed_count = CGM_REPLAY_SUBJECT001_LEN;
    }

    for (i = 0u; i < seed_count; ++i)
    {
        push_sample(replay_glucose_at(i));
    }
}

static void dashboard_timer_cb(lv_timer_t *timer)
{
    uint32_t index = gDashboard.sample_index % CGM_REPLAY_SUBJECT001_LEN;

    (void)timer;
    push_sample(replay_glucose_at(index));
#if defined(APP_APS_SIDECAR_DEMO) && (APP_APS_SIDECAR_DEMO == 1)
    ApsSidecar_Service(lv_tick_get());
#endif
}

void edgeai_insulin_pump_app_start(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *image;
    lv_obj_t *panel;
    lv_obj_t *row;
    lv_obj_t *chart;
    lv_obj_t *bar;
    lv_obj_t *label;

    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    image = lv_image_create(screen);
    lv_image_set_src(image, &edgeai_pump_background_image);
    lv_obj_center(image);

    panel = lv_obj_create(screen);
    if (panel != NULL)
    {
        lv_obj_set_size(panel, 168, 96);
        lv_obj_align(panel, LV_ALIGN_CENTER, 0, 178);
        lv_obj_set_style_radius(panel, 16, 0);
        lv_obj_set_style_border_width(panel, 2, 0);
        lv_obj_set_style_border_color(panel, lv_color_hex(0x78D8FF), 0);
        lv_obj_set_style_bg_color(panel, lv_color_hex(0x08111A), 0);
        lv_obj_set_style_bg_opa(panel, LV_OPA_70, 0);
        lv_obj_set_style_pad_left(panel, 10, 0);
        lv_obj_set_style_pad_right(panel, 10, 0);
        lv_obj_set_style_pad_top(panel, 8, 0);
        lv_obj_set_style_pad_bottom(panel, 8, 0);
        lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

        row = lv_obj_create(panel);
        if (row != NULL)
        {
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_align(row, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(row, 0, 0);
            lv_obj_set_style_pad_all(row, 0, 0);
            lv_obj_set_style_pad_column(row, 4, 0);
            lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

            label = lv_label_create(row);
            if (label != NULL)
            {
                gDashboard.glucose_label = label;
                lv_label_set_text(label, "0");
                lv_obj_set_style_text_color(label, lv_color_hex(0xEAF6FF), 0);
                lv_obj_set_style_text_font(label, &lv_font_montserrat_42, 0);
            }

            label = lv_label_create(row);
            if (label != NULL)
            {
                gDashboard.glucose_unit_label = label;
                lv_label_set_text(label, "MG/DL");
                lv_obj_set_style_text_color(label, lv_color_hex(0xC7EFFF), 0);
                lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
            }
        }

        label = lv_label_create(panel);
        if (label != NULL)
        {
            gDashboard.glucose_title_label = label;
            lv_label_set_text(label, "GLUCOSE LEVEL");
            lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 52);
            lv_obj_set_style_text_color(label, lv_color_hex(0xC7EFFF), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
        }

        gDashboard.glucose_shadow_label = NULL;
        update_glucose_label(replay_glucose_at(0u));
    }

    for (uint32_t index = 0u; index < BAR_GRAPH_COUNT; ++index)
    {
        static const lv_coord_t bar_y_positions[BAR_GRAPH_COUNT] = {249, 279, 309};
        static const char *bar_texts[BAR_GRAPH_COUNT] = {NULL, "CONF", "ACC"};

        if (bar_texts[index] != NULL)
        {
            label = lv_label_create(screen);
            if (label != NULL)
            {
                gDashboard.status_labels[index] = label;
                lv_label_set_text(label, bar_texts[index]);
                lv_obj_set_pos(label, 580, bar_y_positions[index] - 2);
                lv_obj_set_style_text_color(label, lv_color_hex(0xC7EFFF), 0);
                lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
            }
        }

        bar = lv_bar_create(screen);
        if (bar != NULL)
        {
            gDashboard.status_bars[index] = bar;
            lv_obj_set_size(bar, 142, 14);
            lv_obj_set_pos(bar, 620, bar_y_positions[index]);
            lv_bar_set_range(bar, 0, 100);
            lv_bar_set_value(bar, 60, LV_ANIM_OFF);
            lv_obj_set_style_radius(bar, 8, LV_PART_MAIN);
            lv_obj_set_style_radius(bar, 8, LV_PART_INDICATOR);
            lv_obj_set_style_border_width(bar, 0, LV_PART_MAIN);
            lv_obj_set_style_pad_all(bar, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x0A1620), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(bar, LV_OPA_10, LV_PART_MAIN);
            lv_obj_set_style_bg_color(bar, lv_color_hex(0x46DFFF), LV_PART_INDICATOR);
            lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
        }
    }

    chart = lv_chart_create(screen);
    if (chart != NULL)
    {
        gDashboard.chart = chart;
        lv_obj_set_size(chart, 238, 128);
        lv_obj_align(chart, LV_ALIGN_BOTTOM_RIGHT, -16, -16);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        lv_chart_set_point_count(chart, CGM_GRAPH_POINTS);
        lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 50, 390);
        lv_chart_set_div_line_count(chart, 4, 4);
        lv_obj_set_style_radius(chart, 14, 0);
        lv_obj_set_style_border_width(chart, 2, 0);
        lv_obj_set_style_border_color(chart, lv_color_hex(0x78D8FF), 0);
        lv_obj_set_style_bg_color(chart, lv_color_hex(0x08111A), 0);
        lv_obj_set_style_bg_opa(chart, LV_OPA_70, 0);
        lv_obj_set_style_pad_left(chart, 10, 0);
        lv_obj_set_style_pad_right(chart, 10, 0);
        lv_obj_set_style_pad_top(chart, 10, 0);
        lv_obj_set_style_pad_bottom(chart, 8, 0);
        gDashboard.glucose_series = lv_chart_add_series(chart, lv_color_hex(0x6AE0FF), LV_CHART_AXIS_PRIMARY_Y);
        if (gDashboard.glucose_series != NULL)
        {
            lv_chart_set_all_value(chart, gDashboard.glucose_series, 100);
        }
        gDashboard.prediction_series = lv_chart_add_series(chart, lv_color_hex(0x4CC7FF), LV_CHART_AXIS_PRIMARY_Y);
        if (gDashboard.prediction_series != NULL)
        {
            lv_chart_set_all_value(chart, gDashboard.prediction_series, 100);
        }
        label = lv_label_create(chart);
        if (label != NULL)
        {
            gDashboard.prediction_label = label;
            lv_label_set_text(label, "EdgeAI Prediction");
            lv_obj_align(label, LV_ALIGN_TOP_LEFT, 18, -10);
            style_prediction_label(label);
        }

        label = lv_label_create(chart);
        if (label != NULL)
        {
            gDashboard.prediction_accuracy_label = label;
            lv_label_set_text(label, "0%");
            lv_obj_align_to(label, gDashboard.prediction_label, LV_ALIGN_OUT_RIGHT_MID, 18, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(0x79D8FF), 0);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
            lv_obj_set_style_shadow_color(label, lv_color_hex(0xFF4A6A), 0);
            lv_obj_set_style_shadow_opa(label, LV_OPA_COVER, 0);
            lv_obj_set_style_shadow_width(label, 8, 0);
            lv_obj_set_style_shadow_ofs_x(label, 0, 0);
            lv_obj_set_style_shadow_ofs_y(label, 0, 0);
        }
    }

    CgmModel_Reset();
    CgmModel_SetEnabled(true);
    gDashboard.sample_index = 0u;
    seed_chart();
    gDashboard.timer = lv_timer_create(dashboard_timer_cb, CGM_REPLAY_STEP_MS, NULL);
#if defined(APP_APS_SIDECAR_DEMO) && (APP_APS_SIDECAR_DEMO == 1)
    ApsSidecar_Init();
#endif
}
