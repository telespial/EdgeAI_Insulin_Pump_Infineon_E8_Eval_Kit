#include "edgeai_insulin_pump_app.h"

#include <stdint.h>
#include <stdio.h>

#include "cgm_replay_subject001.h"
#include "lvgl.h"
#include "pump_background_image_rgb565.h"

enum
{
    CGM_GRAPH_POINTS = 32u,
    CGM_REPLAY_STEP_MS = 1400u,
    BAR_GRAPH_COUNT = 3u,
};

typedef struct
{
    uint32_t sample_index;
    lv_obj_t *chart;
    lv_chart_series_t *glucose_series;
    lv_obj_t *glucose_unit_label;
    lv_obj_t *glucose_title_label;
    lv_obj_t *glucose_label;
    lv_obj_t *glucose_shadow_label;
    lv_obj_t *status_bars[BAR_GRAPH_COUNT];
    lv_timer_t *timer;
} cgm_dashboard_t;

static cgm_dashboard_t gDashboard;

static uint16_t replay_glucose_at(uint32_t index)
{
    return kCgmReplaySubject001Mgdl[index % CGM_REPLAY_SUBJECT001_LEN];
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

static void update_status_bars(uint16_t current_mgdl)
{
    uint16_t predicted_mgdl;
    int32_t trend_delta;
    int32_t confidence;
    uint32_t i;

    if (gDashboard.status_bars[0u] == NULL)
    {
        return;
    }

    predicted_mgdl = replay_glucose_at(gDashboard.sample_index + 4u);
    trend_delta = (int32_t)predicted_mgdl - (int32_t)current_mgdl;
    confidence = 100 - (trend_delta < 0 ? -trend_delta : trend_delta);
    confidence = clamp_i32(confidence, 20, 100);

    lv_bar_set_value(gDashboard.status_bars[0u], map_range_i32((int32_t)current_mgdl, 40, 400, 12, 100), LV_ANIM_OFF);
    lv_bar_set_value(gDashboard.status_bars[1u], map_range_i32((int32_t)predicted_mgdl, 40, 400, 12, 100), LV_ANIM_OFF);
    lv_bar_set_value(gDashboard.status_bars[2u], confidence, LV_ANIM_OFF);

    for (i = 0u; i < BAR_GRAPH_COUNT; ++i)
    {
        lv_obj_invalidate(gDashboard.status_bars[i]);
    }
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
    update_glucose_label(current_mgdl);
    update_status_bars(current_mgdl);

    if ((gDashboard.chart != NULL) && (gDashboard.glucose_series != NULL))
    {
        lv_chart_set_next_value(gDashboard.chart, gDashboard.glucose_series, (int32_t)current_mgdl);
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
    }

    gDashboard.sample_index = 0u;
    seed_chart();
    gDashboard.timer = lv_timer_create(dashboard_timer_cb, CGM_REPLAY_STEP_MS, NULL);
}
