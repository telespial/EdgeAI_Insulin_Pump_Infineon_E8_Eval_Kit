#include "insulin_ui.h"

#include "../platform/insulin_platform.h"

void insulin_ui_init(insulin_ui_t *ui)
{
    if (ui == 0)
    {
        return;
    }

    ui->reserved = 0;
    insulin_platform_ui_begin();
}

void insulin_ui_render(insulin_ui_t *ui, const insulin_runtime_t *runtime)
{
    (void)ui;

    if (runtime == 0)
    {
        return;
    }

    insulin_platform_ui_draw_summary(runtime->glucose_mgdl,
                                     runtime->pred_15m_mgdl,
                                     runtime->pred_30m_mgdl,
                                     runtime->alert_hypo,
                                     runtime->alert_hyper);
    insulin_platform_ui_present();
}
