#include "hys_cmp.h"
#include "my_math.h"
#include "string.h"

void hys_cmp_init(hys_cmp_t *p_str,
                  float *p_val,
                  hys_cmp_cfg_t *p_cfg)
{
    p_str->input.p_val = p_val;
    p_str->inter.p_cfg = p_cfg;
    p_str->inter.cnt = 0;
    p_str->output.is_protect = 0;
}

void hys_cmp_func(hys_cmp_t *p_str)
{
    if ((p_str->input.p_val == NULL) ||
        (p_str->inter.p_cfg == NULL))
    {
        return;
    }

    if ((p_str->output.is_protect == 0) &&
        (p_str->inter.p_cfg->p_cmp_func != NULL))
    {
        if (p_str->inter.p_cfg->p_cmp_func(*p_str->input.p_val, p_str->inter.p_cfg->thr))
        {
            p_str->inter.cnt++;
            if (p_str->inter.cnt > p_str->inter.p_cfg->time)
            {
                p_str->output.is_protect = 1;
                p_str->inter.cnt = 0;
            }
        }
        else
        {
            DN_CNT(p_str->inter.cnt);
        }
    }
    else if ((p_str->output.is_protect == 1) &&
             (p_str->inter.p_cfg->p_cmp_hys_func != NULL))
    {
        if (p_str->inter.p_cfg->p_cmp_hys_func(*p_str->input.p_val, p_str->inter.p_cfg->thr_hys))
        {
            p_str->inter.cnt++;
            if (p_str->inter.cnt > p_str->inter.p_cfg->time_hys)
            {
                p_str->output.is_protect = 0;
                p_str->inter.cnt = 0;
            }
        }
        else
        {
            DN_CNT(p_str->inter.cnt);
        }
    }
}

uint8_t cmp_gt(float val, float thr)
{
    return val > thr;
}

uint8_t cmp_lt(float val, float thr)
{
    return val < thr;
}
