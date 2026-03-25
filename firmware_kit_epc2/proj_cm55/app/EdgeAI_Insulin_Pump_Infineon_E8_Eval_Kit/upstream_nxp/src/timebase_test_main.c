#include <stdbool.h>
#include <stdint.h>

#include "app.h"
#include "board.h"
#include "fsl_clock.h"
#include "fsl_debug_console.h"
#include "fsl_ostimer.h"
#include "gauge_render.h"
#include "power_data_source.h"

#define TIMEBASE_HZ 32768u
#define TIMEBASE_CAL_WINDOW_US 250000u
#define RUNTIME_CLOCK_PERIOD_US 100000u
#define DISPLAY_REFRESH_PERIOD_US 100000u
#define POWER_TICK_PERIOD_US 1000000u

static void ClockFromDeciseconds(uint32_t ds_total, uint8_t *hh, uint8_t *mm, uint8_t *ss, uint8_t *ds)
{
    uint32_t sec_total = ds_total / 10u;
    uint32_t rem_ds = ds_total % 10u;
    uint32_t hh_v = (sec_total / 3600u) % 24u;
    uint32_t mm_v = (sec_total % 3600u) / 60u;
    uint32_t ss_v = sec_total % 60u;

    *hh = (uint8_t)hh_v;
    *mm = (uint8_t)mm_v;
    *ss = (uint8_t)ss_v;
    *ds = (uint8_t)rem_ds;
}

static uint32_t TimebaseCalibrateHz(bool use_raw)
{
    uint64_t t0 = use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
    uint64_t t1;
    uint64_t dt_ticks;
    uint64_t hz64;
    uint32_t core_hz = CLOCK_GetCoreSysClkFreq();
    if (core_hz == 0u)
    {
        core_hz = SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY;
    }

    SDK_DelayAtLeastUs(TIMEBASE_CAL_WINDOW_US, core_hz);
    t1 = use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
    dt_ticks = t1 - t0;
    if (dt_ticks == 0u)
    {
        return 0u;
    }

    hz64 = (dt_ticks * 1000000ull) / TIMEBASE_CAL_WINDOW_US;
    if ((hz64 < 1000ull) || (hz64 > 1000000ull))
    {
        return 0u;
    }
    return (uint32_t)hz64;
}

int main(void)
{
    bool lcd_ok;
    uint32_t runtime_elapsed_ds = 0u;
    uint32_t runtime_accum_us = 0u;
    uint32_t render_accum_us = 0u;
    uint32_t power_accum_us = 0u;
    uint32_t timebase_hz = TIMEBASE_HZ;
    bool timebase_use_raw = false;
    uint64_t prev_ticks;
    uint64_t us_rem = 0u;
    status_t st;
    uint32_t cfg_hz;
    uint32_t raw_hz;
    uint32_t dec_hz;
    uint32_t err_raw;
    uint32_t err_dec;

    BOARD_InitHardware();
    lcd_ok = GaugeRender_Init();
    PowerData_Init();

    GaugeRender_SetRuntimeClock(0u, 0u, 0u, 0u, true);
    GaugeRender_SetHelpVisible(false);
    GaugeRender_SetSettingsVisible(false);
    GaugeRender_SetRecordMode(false);
    GaugeRender_SetPlayhead(0u, false);

    st = CLOCK_SetupOsc32KClocking(kCLOCK_Osc32kToWake);
    CLOCK_AttachClk(kXTAL32K2_to_OSTIMER);
    OSTIMER_Init(OSTIMER0);
    cfg_hz = CLOCK_GetOstimerClkFreq();
    raw_hz = TimebaseCalibrateHz(true);
    dec_hz = TimebaseCalibrateHz(false);
    if (cfg_hz != 0u)
    {
        err_raw = (raw_hz > cfg_hz) ? (raw_hz - cfg_hz) : (cfg_hz - raw_hz);
        err_dec = (dec_hz > cfg_hz) ? (dec_hz - cfg_hz) : (cfg_hz - dec_hz);
        timebase_use_raw = (raw_hz != 0u) && ((dec_hz == 0u) || (err_raw <= err_dec));
    }
    else
    {
        timebase_use_raw = (raw_hz != 0u);
    }

    if (timebase_use_raw)
    {
        timebase_hz = raw_hz;
    }
    else if (dec_hz != 0u)
    {
        timebase_hz = dec_hz;
    }
    else if (cfg_hz != 0u)
    {
        timebase_hz = cfg_hz;
    }
    prev_ticks = timebase_use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);

    PRINTF("TIMEBASE_TEST: OSC32K setup=%d cfg=%uHz raw=%uHz dec=%uHz mode=%s use=%uHz\\r\\n",
           (int)st,
           (unsigned int)cfg_hz,
           (unsigned int)raw_hz,
           (unsigned int)dec_hz,
           timebase_use_raw ? "raw" : "dec",
           (unsigned int)timebase_hz);
    PRINTF("TIMEBASE_TEST: elapsed clock running\\r\\n");

    for (;;)
    {
        uint64_t now_ticks = timebase_use_raw ? OSTIMER_GetCurrentTimerRawValue(OSTIMER0) : OSTIMER_GetCurrentTimerValue(OSTIMER0);
        uint64_t dt_ticks = now_ticks - prev_ticks;
        uint64_t us_num = (dt_ticks * 1000000ull) + us_rem;
        uint32_t elapsed_loop_us = (uint32_t)(us_num / timebase_hz);

        prev_ticks = now_ticks;
        us_rem = us_num % timebase_hz;

        if (elapsed_loop_us == 0u)
        {
            elapsed_loop_us = 1u;
        }
        if (elapsed_loop_us > 250000u)
        {
            elapsed_loop_us = 250000u;
        }

        runtime_accum_us += elapsed_loop_us;
        render_accum_us += elapsed_loop_us;
        power_accum_us += elapsed_loop_us;

        while (runtime_accum_us >= RUNTIME_CLOCK_PERIOD_US)
        {
            uint8_t hh, mm, ss, ds;
            runtime_accum_us -= RUNTIME_CLOCK_PERIOD_US;
            runtime_elapsed_ds++;
            ClockFromDeciseconds(runtime_elapsed_ds, &hh, &mm, &ss, &ds);
            GaugeRender_SetRuntimeClock(hh, mm, ss, ds, true);
        }

        if (power_accum_us >= POWER_TICK_PERIOD_US)
        {
            power_accum_us -= POWER_TICK_PERIOD_US;
            PowerData_Tick();
        }

        if (lcd_ok && (render_accum_us >= DISPLAY_REFRESH_PERIOD_US))
        {
            const power_sample_t *sample = PowerData_Get();
            render_accum_us -= DISPLAY_REFRESH_PERIOD_US;
            if (sample != NULL)
            {
                GaugeRender_DrawFrame(sample, true, PowerData_GetReplayProfile());
            }
        }
    }

    return 0;
}
