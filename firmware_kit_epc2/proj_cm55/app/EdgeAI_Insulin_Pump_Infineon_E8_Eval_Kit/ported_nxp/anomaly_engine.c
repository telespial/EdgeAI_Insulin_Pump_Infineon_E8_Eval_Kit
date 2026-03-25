#include "anomaly_engine.h"

#include <string.h>

#define ANOM_WIN_SAMPLES (50u)       /* 5 s @ 10 Hz */
#define ANOM_TRAIN_SAMPLES (300u)    /* 30 s @ 10 Hz */
#define ANOM_MIN_BASELINE (10u)

typedef struct
{
    int32_t hist[ANOM_WIN_SAMPLES];
    uint16_t hist_idx;
    uint16_t hist_count;
    int64_t sum;
    int64_t sum_sq;

    uint8_t watch_hist[ANOM_WIN_SAMPLES];
    uint8_t minor_hist[ANOM_WIN_SAMPLES];
    uint8_t major_hist[ANOM_WIN_SAMPLES];
    uint16_t vote_idx;
    uint16_t vote_count;
    uint16_t watch_sum;
    uint16_t minor_sum;
    uint16_t major_sum;

    int32_t trained_mean;
    int32_t trained_var;
    bool trained_valid;

    int32_t static_min;
    int32_t static_max;
    int32_t static_warn_lo;
    int32_t static_warn_hi;
    int32_t min_var;
} anomaly_channel_state_t;

typedef struct
{
    anomaly_mode_t mode;
    anomaly_tune_t tune;
    bool adaptive_freeze;
    bool training_active;
    bool trained_ready;
    uint16_t train_samples;
    anomaly_channel_state_t ch[ANOMALY_CH_COUNT];
    anomaly_output_t out;
} anomaly_engine_state_t;

static anomaly_engine_state_t sAnom;

typedef struct
{
    uint16_t watch_mult_x100;
    uint16_t minor_mult_x100;
    uint16_t major_mult_x100;
    uint16_t watch_hits;
    uint16_t minor_hits;
    uint16_t major_hits;
    uint16_t static_warn_pct_lo;
    uint16_t static_warn_pct_hi;
} tune_profile_t;

static tune_profile_t TuneProfile(anomaly_tune_t tune)
{
    tune_profile_t p;
    if (tune == ANOMALY_TUNE_LOOSE)
    {
        p.watch_mult_x100 = 500u; /* 5 sigma */
        p.minor_mult_x100 = 1100u; /* 11 sigma */
        p.major_mult_x100 = 1800u; /* 18 sigma */
        p.watch_hits = 8u;
        p.minor_hits = 5u;
        p.major_hits = 4u;
        p.static_warn_pct_lo = 15u;
        p.static_warn_pct_hi = 85u;
        return p;
    }
    if (tune == ANOMALY_TUNE_STRICT)
    {
        p.watch_mult_x100 = 300u; /* 3 sigma */
        p.minor_mult_x100 = 625u; /* 6.25 sigma */
        p.major_mult_x100 = 900u; /* 9 sigma */
        p.watch_hits = 4u;
        p.minor_hits = 3u;
        p.major_hits = 2u;
        p.static_warn_pct_lo = 25u;
        p.static_warn_pct_hi = 75u;
        return p;
    }

    p.watch_mult_x100 = 400u; /* 4 sigma */
    p.minor_mult_x100 = 900u; /* 9 sigma */
    p.major_mult_x100 = 1600u; /* 16 sigma */
    p.watch_hits = 6u;
    p.minor_hits = 4u;
    p.major_hits = 3u;
    p.static_warn_pct_lo = 20u;
    p.static_warn_pct_hi = 80u;
    return p;
}

static void ResetVotes(anomaly_channel_state_t *c)
{
    memset(c->watch_hist, 0, sizeof(c->watch_hist));
    memset(c->minor_hist, 0, sizeof(c->minor_hist));
    memset(c->major_hist, 0, sizeof(c->major_hist));
    c->vote_idx = 0u;
    c->vote_count = 0u;
    c->watch_sum = 0u;
    c->minor_sum = 0u;
    c->major_sum = 0u;
}

static void ResetHistory(anomaly_channel_state_t *c)
{
    memset(c->hist, 0, sizeof(c->hist));
    c->hist_idx = 0u;
    c->hist_count = 0u;
    c->sum = 0;
    c->sum_sq = 0;
}

static void PushHistory(anomaly_channel_state_t *c, int32_t v)
{
    if (c->hist_count < ANOM_WIN_SAMPLES)
    {
        c->hist[c->hist_idx] = v;
        c->sum += v;
        c->sum_sq += (int64_t)v * (int64_t)v;
        c->hist_count++;
        c->hist_idx = (uint16_t)((c->hist_idx + 1u) % ANOM_WIN_SAMPLES);
        return;
    }

    {
        int32_t old = c->hist[c->hist_idx];
        c->hist[c->hist_idx] = v;
        c->sum += (int64_t)v - (int64_t)old;
        c->sum_sq += (int64_t)v * (int64_t)v - (int64_t)old * (int64_t)old;
        c->hist_idx = (uint16_t)((c->hist_idx + 1u) % ANOM_WIN_SAMPLES);
    }
}

static void PushVotes(anomaly_channel_state_t *c, bool watch, bool minor, bool major)
{
    if (c->vote_count < ANOM_WIN_SAMPLES)
    {
        c->watch_hist[c->vote_idx] = watch ? 1u : 0u;
        c->minor_hist[c->vote_idx] = minor ? 1u : 0u;
        c->major_hist[c->vote_idx] = major ? 1u : 0u;
        c->watch_sum += (watch ? 1u : 0u);
        c->minor_sum += (minor ? 1u : 0u);
        c->major_sum += (major ? 1u : 0u);
        c->vote_count++;
        c->vote_idx = (uint16_t)((c->vote_idx + 1u) % ANOM_WIN_SAMPLES);
        return;
    }

    c->watch_sum = (uint16_t)(c->watch_sum - c->watch_hist[c->vote_idx] + (watch ? 1u : 0u));
    c->minor_sum = (uint16_t)(c->minor_sum - c->minor_hist[c->vote_idx] + (minor ? 1u : 0u));
    c->major_sum = (uint16_t)(c->major_sum - c->major_hist[c->vote_idx] + (major ? 1u : 0u));
    c->watch_hist[c->vote_idx] = watch ? 1u : 0u;
    c->minor_hist[c->vote_idx] = minor ? 1u : 0u;
    c->major_hist[c->vote_idx] = major ? 1u : 0u;
    c->vote_idx = (uint16_t)((c->vote_idx + 1u) % ANOM_WIN_SAMPLES);
}

static anomaly_level_t VotesToLevel(const anomaly_channel_state_t *c, uint16_t w_hits, uint16_t m_hits, uint16_t M_hits)
{
    if (c->major_sum >= M_hits)
    {
        return ANOMALY_LEVEL_MAJOR;
    }
    if (c->minor_sum >= m_hits)
    {
        return ANOMALY_LEVEL_MINOR;
    }
    if (c->watch_sum >= w_hits)
    {
        return ANOMALY_LEVEL_WATCH;
    }
    return ANOMALY_LEVEL_IGNORE;
}

static int32_t ComputeVar(const anomaly_channel_state_t *c)
{
    int64_t n;
    int64_t mean;
    int64_t var;

    if (c->hist_count == 0u)
    {
        return c->min_var;
    }

    n = c->hist_count;
    mean = c->sum / n;
    var = (c->sum_sq / n) - (mean * mean);
    if (var < (int64_t)c->min_var)
    {
        var = c->min_var;
    }
    if (var > 0x7FFFFFFFll)
    {
        var = 0x7FFFFFFFll;
    }
    return (int32_t)var;
}

static int32_t ComputeMean(const anomaly_channel_state_t *c)
{
    if (c->hist_count == 0u)
    {
        return 0;
    }
    return (int32_t)(c->sum / c->hist_count);
}

static anomaly_level_t EvalSigma(anomaly_channel_state_t *c, int32_t sample, bool use_trained)
{
    tune_profile_t tune = TuneProfile(sAnom.tune);
    int32_t mean;
    int32_t var;
    int64_t d;
    int64_t d2;
    bool watch = false;
    bool minor = false;
    bool major = false;

    if (use_trained)
    {
        if (!c->trained_valid)
        {
            PushVotes(c, false, false, false);
            return ANOMALY_LEVEL_IGNORE;
        }
        mean = c->trained_mean;
        var = c->trained_var;
    }
    else
    {
        if (c->hist_count < ANOM_MIN_BASELINE)
        {
            PushVotes(c, false, false, false);
            return ANOMALY_LEVEL_IGNORE;
        }
        mean = ComputeMean(c);
        var = ComputeVar(c);
    }

    d = (int64_t)sample - (int64_t)mean;
    d2 = d * d;

    if (d2 * 100ll >= (int64_t)var * (int64_t)tune.major_mult_x100)
    {
        major = true;
        minor = true;
        watch = true;
    }
    else if (d2 * 100ll >= (int64_t)var * (int64_t)tune.minor_mult_x100)
    {
        minor = true;
        watch = true;
    }
    else if (d2 * 100ll >= (int64_t)var * (int64_t)tune.watch_mult_x100)
    {
        watch = true;
    }

    PushVotes(c, watch, minor, major);
    return VotesToLevel(c, tune.watch_hits, tune.minor_hits, tune.major_hits);
}

static anomaly_level_t EvalStatic(anomaly_channel_state_t *c, int32_t sample)
{
    tune_profile_t tune = TuneProfile(sAnom.tune);
    int32_t range = c->static_max - c->static_min;
    int32_t warn_lo = c->static_min + (range * (int32_t)tune.static_warn_pct_lo) / 100;
    int32_t warn_hi = c->static_min + (range * (int32_t)tune.static_warn_pct_hi) / 100;
    bool minor = false;
    bool major = false;

    if ((sample < c->static_min) || (sample > c->static_max))
    {
        major = true;
        minor = true;
    }
    else if ((sample <= warn_lo) || (sample >= warn_hi))
    {
        minor = true;
    }

    PushVotes(c, false, minor, major);
    return VotesToLevel(c, 1u, 1u, 1u);
}

static void BuildDefaults(void)
{
    memset(&sAnom, 0, sizeof(sAnom));
    sAnom.mode = ANOMALY_MODE_ADAPTIVE_BASELINE;
    sAnom.tune = ANOMALY_TUNE_NORMAL;

    sAnom.ch[ANOMALY_CH_AX].min_var = 2500;  /* 50 mg sigma floor */
    sAnom.ch[ANOMALY_CH_AY].min_var = 2500;
    sAnom.ch[ANOMALY_CH_AZ].min_var = 2500;
    sAnom.ch[ANOMALY_CH_TEMP].min_var = 9;   /* 0.3 C sigma floor (c10 units) */

    sAnom.ch[ANOMALY_CH_AX].static_min = -2000;
    sAnom.ch[ANOMALY_CH_AX].static_max = 2000;
    sAnom.ch[ANOMALY_CH_AY].static_min = -2000;
    sAnom.ch[ANOMALY_CH_AY].static_max = 2000;
    sAnom.ch[ANOMALY_CH_AZ].static_min = 0;
    sAnom.ch[ANOMALY_CH_AZ].static_max = 2000;
    sAnom.ch[ANOMALY_CH_TEMP].static_min = 0;
    sAnom.ch[ANOMALY_CH_TEMP].static_max = 850;

    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        int32_t range = sAnom.ch[i].static_max - sAnom.ch[i].static_min;
        sAnom.ch[i].static_warn_lo = sAnom.ch[i].static_min + (range * 20) / 100;
        sAnom.ch[i].static_warn_hi = sAnom.ch[i].static_min + (range * 80) / 100;
    }
}

void AnomalyEngine_Init(void)
{
    BuildDefaults();
}

void AnomalyEngine_SetMode(anomaly_mode_t mode)
{
    sAnom.mode = mode;
    sAnom.adaptive_freeze = false;
    sAnom.training_active = false;
    sAnom.trained_ready = false;
    sAnom.train_samples = 0u;
    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        ResetVotes(&sAnom.ch[i]);
        ResetHistory(&sAnom.ch[i]);
        sAnom.ch[i].trained_valid = false;
    }
}

anomaly_mode_t AnomalyEngine_GetMode(void)
{
    return sAnom.mode;
}

void AnomalyEngine_CycleTune(void)
{
    sAnom.tune = (anomaly_tune_t)(((uint32_t)sAnom.tune + 1u) % 3u);
    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        ResetVotes(&sAnom.ch[i]);
    }
}

void AnomalyEngine_SetTune(anomaly_tune_t tune)
{
    if ((uint32_t)tune > (uint32_t)ANOMALY_TUNE_STRICT)
    {
        tune = ANOMALY_TUNE_NORMAL;
    }
    sAnom.tune = tune;
    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        ResetVotes(&sAnom.ch[i]);
    }
}

anomaly_tune_t AnomalyEngine_GetTune(void)
{
    return sAnom.tune;
}

void AnomalyEngine_StartTraining(void)
{
    if (sAnom.mode != ANOMALY_MODE_TRAINED_MONITOR)
    {
        return;
    }
    sAnom.training_active = true;
    sAnom.trained_ready = false;
    sAnom.train_samples = 0u;
    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        ResetVotes(&sAnom.ch[i]);
        ResetHistory(&sAnom.ch[i]);
        sAnom.ch[i].trained_valid = false;
    }
}

void AnomalyEngine_StopTraining(void)
{
    sAnom.training_active = false;
}

void AnomalyEngine_SetAdaptiveFreeze(bool freeze)
{
    sAnom.adaptive_freeze = freeze;
}

bool AnomalyEngine_GetAdaptiveFreeze(void)
{
    return sAnom.adaptive_freeze;
}

void AnomalyEngine_Update(int16_t ax_mg, int16_t ay_mg, int16_t az_mg, int16_t temp_c10)
{
    int32_t sample[ANOMALY_CH_COUNT];
    anomaly_level_t overall = ANOMALY_LEVEL_IGNORE;

    sample[ANOMALY_CH_AX] = ax_mg;
    sample[ANOMALY_CH_AY] = ay_mg;
    sample[ANOMALY_CH_AZ] = az_mg;
    sample[ANOMALY_CH_TEMP] = temp_c10;

    for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
    {
        anomaly_channel_state_t *c = &sAnom.ch[i];
        anomaly_level_t level = ANOMALY_LEVEL_IGNORE;

        if (sAnom.mode == ANOMALY_MODE_STATIC_LIMITS)
        {
            level = EvalStatic(c, sample[i]);
        }
        else if (sAnom.mode == ANOMALY_MODE_TRAINED_MONITOR)
        {
            if (sAnom.training_active)
            {
                PushHistory(c, sample[i]);
                ResetVotes(c);
                level = ANOMALY_LEVEL_IGNORE;
            }
            else
            {
                level = EvalSigma(c, sample[i], true);
            }
        }
        else
        {
            level = EvalSigma(c, sample[i], false);
            if (!sAnom.adaptive_freeze)
            {
                PushHistory(c, sample[i]);
            }
        }

        sAnom.out.channel_level[i] = level;
        if (level > overall)
        {
            overall = level;
        }
    }

    if ((sAnom.mode == ANOMALY_MODE_TRAINED_MONITOR) && sAnom.training_active)
    {
        sAnom.train_samples++;
        if (sAnom.train_samples >= ANOM_TRAIN_SAMPLES)
        {
            sAnom.training_active = false;
            sAnom.trained_ready = true;
            for (uint32_t i = 0u; i < ANOMALY_CH_COUNT; i++)
            {
                sAnom.ch[i].trained_mean = ComputeMean(&sAnom.ch[i]);
                sAnom.ch[i].trained_var = ComputeVar(&sAnom.ch[i]);
                sAnom.ch[i].trained_valid = true;
                ResetVotes(&sAnom.ch[i]);
            }
        }
    }

    sAnom.out.mode = sAnom.mode;
    sAnom.out.tune = sAnom.tune;
    sAnom.out.training_active = sAnom.training_active;
    sAnom.out.trained_ready = sAnom.trained_ready;
    sAnom.out.overall_level = overall;
}

void AnomalyEngine_GetOutput(anomaly_output_t *out)
{
    if (out == NULL)
    {
        return;
    }
    *out = sAnom.out;
}
