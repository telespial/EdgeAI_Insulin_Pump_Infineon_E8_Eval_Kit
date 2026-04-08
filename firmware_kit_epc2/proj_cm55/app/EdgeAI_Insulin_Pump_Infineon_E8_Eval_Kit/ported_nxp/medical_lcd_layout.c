#include "medical_lcd_layout.h"

#include "medical_display_hal.h"
#include "pump_bg.h"

void MedicalLcd_DrawStaticLayout(void)
{
    /* Background-only mode requested: no windows, labels, or controls. */
    MedicalHal_BlitRgb565(0, 0, PUMP_BG_WIDTH, PUMP_BG_HEIGHT, g_pump_bg_rgb565);
}

void MedicalLcd_DrawDynamic(const medical_lcd_state_t *state)
{
    (void)state;
    /* Background-only mode requested: intentionally no dynamic overlays. */
}
