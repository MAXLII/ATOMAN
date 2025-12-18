#include "chk_grid.h"

void chk_grid_init(chk_grid_t *p_str,
                   float *p_rms,
                   float *p_freq,
                   uint32_t judge_time,
                   uint32_t abnormale_time,
                   float rms_normal_max,
                   float rms_normal_min,
                   float rms_abnormal_max,
                   float rms_abnormal_min,
                   float freq_normal_max,
                   float freq_normal_min,
                   float freq_abnormal_max,
                   float freq_abnormal_min)
{
    p_str->input.p_rms = p_rms;
    p_str->input.p_freq = p_freq;
    p_str->cfg.judge_time = judge_time;
    p_str->cfg.abnormale_time = abnormale_time;
    p_str->cfg.rms.normal.max = rms_normal_max;
    p_str->cfg.rms.normal.min = rms_normal_min;
    p_str->cfg.rms.abnormal.max = rms_abnormal_max;
    p_str->cfg.rms.abnormal.min = rms_abnormal_min;
    p_str->cfg.freq.normal.max = freq_normal_max;
    p_str->cfg.freq.normal.min = freq_normal_min;
    p_str->cfg.freq.abnormal.max = freq_abnormal_max;
    p_str->cfg.freq.abnormal.min = freq_abnormal_min;

    p_str->inter.is_ok_cnt = 0;
    p_str->output.is_ok = 0;
}

void chk_grid_func(chk_grid_t *p_str)
{
    if (p_str->output.is_ok == 0)
    {
        if ((*p_str->input.p_rms < p_str->cfg.rms.normal.max) &&
            (*p_str->input.p_rms > p_str->cfg.rms.normal.min) &&
            (*p_str->input.p_freq < p_str->cfg.freq.normal.max) &&
            (*p_str->input.p_freq > p_str->cfg.freq.normal.min))
        {
            p_str->inter.is_ok_cnt++;
        }
        else
        {
            p_str->inter.is_ok_cnt = 0;
        }
        if (p_str->inter.is_ok_cnt > p_str->cfg.judge_time)
        {
            p_str->output.is_ok = 1;
            p_str->inter.is_ok_cnt = 0;
        }
    }
    else
    {
        if ((*p_str->input.p_rms > p_str->cfg.rms.abnormal.max) ||
            (*p_str->input.p_rms < p_str->cfg.rms.abnormal.min) ||
            (*p_str->input.p_freq > p_str->cfg.freq.abnormal.max) ||
            (*p_str->input.p_freq < p_str->cfg.freq.abnormal.min))
        {
            p_str->inter.abnormal_cnt++;
            if (p_str->inter.abnormal_cnt > p_str->cfg.abnormale_time)
            {
                p_str->output.is_ok = 0;
                p_str->inter.is_ok_cnt = 0;
                p_str->inter.abnormal_cnt = 0;
            }
        }
        else
        {
            if (p_str->inter.abnormal_cnt)
            {
                p_str->inter.abnormal_cnt--;
            }
        }
    }
}

void chk_grid_reset(chk_grid_t *p_str)
{
    p_str->output.is_ok = 0;
    p_str->inter.abnormal_cnt = 0;
    p_str->inter.is_ok_cnt = 0;
}
