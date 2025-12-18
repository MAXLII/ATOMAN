#include "rly_on.h"
#include "my_math.h"

void rly_on_init(rly_on_t *p_str,
                 uint8_t *p_rly_on_trig,
                 uint8_t *p_rly_off_trig,
                 uint8_t *p_is_equal,
                 float *p_freq,
                 float ctrl_freq,
                 float rly_on_time_def,
                 void (*rly_on)(void),
                 void (*rly_off)(void))
{
    p_str->input.p_freq = p_freq;
    p_str->input.p_is_equal = p_is_equal;
    p_str->input.p_rly_off_trig = p_rly_off_trig;
    p_str->input.p_rly_on_trig = p_rly_on_trig;
    p_str->cfg.ctrl_freq = ctrl_freq;
    p_str->cfg.rly_on_time_def = rly_on_time_def;
    p_str->func.rly_off = rly_off;
    p_str->func.rly_on = rly_on;
}

void rly_on_func(rly_on_t *p_str)
{
    switch (p_str->inter.sta)
    {
    case RLY_ON_STA_INIT:
        if ((p_str->func.rly_off == NULL) ||
            (p_str->func.rly_on == NULL))
        {
            return;
        }
        else
        {
            p_str->inter.sta = RLY_ON_STA_IDLE;
            *p_str->input.p_rly_on_trig = 0;
        }
        break;
    case RLY_ON_STA_IDLE:
        if (*p_str->input.p_rly_on_trig == 1)
        {
            *p_str->input.p_rly_on_trig = 0;
            p_str->inter.sta = RLY_ON_STA_WAIT;
        }
        break;
    case RLY_ON_STA_WAIT:
        if (*p_str->input.p_is_equal == 1)
        {
            p_str->inter.sta = RLY_ON_STA_DLY;
            float freq_temp = *p_str->input.p_freq;
            DN_LMT(freq_temp, 10.0f);
            float grid_ts = 1.0f / freq_temp;
            float rly_on_time = p_str->cfg.rly_on_time_def;
            while (p_str->cfg.rly_on_time_def > grid_ts)
            {
                rly_on_time -= grid_ts;
            }
            p_str->inter.dly = (uint32_t)(p_str->cfg.ctrl_freq * (grid_ts - rly_on_time));
        }
        break;
    case RLY_ON_STA_DLY:
        if (p_str->inter.dly_cnt >= p_str->inter.dly)
        {
            p_str->func.rly_on();
            p_str->inter.dly_cnt = 0;
            p_str->inter.sta = RLY_ON_STA_RUN;
            *p_str->input.p_rly_off_trig = 0;
        }
        else
        {
            p_str->inter.dly_cnt++;
        }
        break;
    case RLY_ON_STA_RUN:
        if (*p_str->input.p_rly_off_trig == 1)
        {
            *p_str->input.p_rly_off_trig = 0;
            p_str->func.rly_off();
            p_str->inter.sta = RLY_ON_STA_IDLE;
            *p_str->input.p_rly_on_trig = 0;
        }
        break;
    case RLY_ON_STA_ERR:
        break;
    default:
        break;
    }
}