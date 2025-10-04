#include "cal_rms.h"
#include "string.h"
#include "math.h"

// 重置计算状态
void reset_calculation(cal_rms_t *str)
{
    str->inter.square_sum = 0.0f;
    str->inter.cnt = 0;
}

// 设置错误状态
static void set_error_state(cal_rms_t *str)
{
    str->output.freq = 0.0f;
    str->output.rms = 0.0f;
    str->output.is_cal = 0;
    str->output.is_run = 0;
    reset_calculation(str);
    str->inter.sta = CAL_RMS_STA_IDLE;
}

// 执行RMS计算
static void perform_rms_calculation(cal_rms_t *str)
{
    if (str->inter.cnt == 0)
    {
        set_error_state(str);
        return;
    }

    str->output.freq = 1.0f / (str->cfg.ts * str->inter.cnt);
    str->output.rms = sqrtf(str->inter.square_sum / str->inter.cnt);
    str->output.is_cal = 1;
    reset_calculation(str);
}

// 处理正半周期
static void process_positive_half_cycle(cal_rms_t *str)
{
    str->inter.square_sum += *str->input.p_val * *str->input.p_val;
    str->inter.cnt++;

    if (str->inter.cnt > str->cfg.cnt_max)
    {
        set_error_state(str);
    }
    else if (*str->input.p_val < -str->cfg.cross_thr)
    {
        str->inter.sta = CAL_RMS_STA_IS_NEG;
    }
}

// 处理负半周期
static void process_negative_half_cycle(cal_rms_t *str)
{
    str->inter.square_sum += *str->input.p_val * *str->input.p_val;
    str->inter.cnt++;

    if (str->inter.cnt > str->cfg.cnt_max)
    {
        set_error_state(str);
    }
    else if (*str->input.p_val > str->cfg.cross_thr)
    {
        str->inter.sta = CAL_RMS_STA_CAL;
    }
}

void cal_rms_init(cal_rms_t *str,
                  CAL_RMS_ROLE_E role,
                  float ts,
                  float cross_thr,
                  float *p_val,
                  uint8_t *p_is_cal,
                  uint8_t *p_is_run)
{
    memset(str, 0, sizeof(cal_rms_t));
    str->input.p_val = p_val;
    str->input.p_is_cal = p_is_cal;
    str->input.p_is_run = p_is_run;
    str->cfg.role = role;
    str->cfg.ts = ts;
    str->cfg.cross_thr = cross_thr;
    str->cfg.cnt_max = (int)(1.0f / (CAL_RMS_FREQ_MIN * ts));
}

void cal_rms_master_run(cal_rms_t *str)
{
    if (str->cfg.role != CAL_RMS_MASTER)
    {
        return;
    }

    str->output.is_cal = 0; // 默认未完成计算

    switch (str->inter.sta)
    {
    case CAL_RMS_STA_IDLE:
        str->output.is_run = 0;
        if (*str->input.p_val > str->cfg.cross_thr)
        {
            reset_calculation(str);
            str->inter.sta = CAL_RMS_STA_IS_POS;
            str->output.is_run = 1;
        }
        break;

    case CAL_RMS_STA_IS_POS:
        process_positive_half_cycle(str);
        break;

    case CAL_RMS_STA_IS_NEG:
        process_negative_half_cycle(str);
        break;

    case CAL_RMS_STA_CAL:
        perform_rms_calculation(str);
        // 继续处理当前样本
        str->inter.square_sum += *str->input.p_val * *str->input.p_val;
        str->inter.cnt++;
        str->inter.sta = CAL_RMS_STA_IS_POS; // 准备下一周期
        break;
    }
}

void cal_rms_slave_run(cal_rms_t *str)
{
    if ((str->cfg.role != CAL_RMS_SLAVE) ||
        (str->input.p_is_cal == NULL) ||
        (str->input.p_is_run == NULL))
    {
        return;
    }

    switch (str->inter.sta)
    {
    case CAL_RMS_STA_IDLE:
        str->output.is_run = 0;
        if (*str->input.p_is_run)
        {
            str->output.is_run = 1;
            str->inter.sta = CAL_RMS_STA_CAL;
        }
        break;

    case CAL_RMS_STA_CAL:
        str->inter.square_sum += *str->input.p_val * *str->input.p_val;
        str->inter.cnt++;

        if (*str->input.p_is_cal)
        {
            perform_rms_calculation(str);
        }

        if (str->inter.cnt > str->cfg.cnt_max)
        {
            set_error_state(str);
        }

        if (!*str->input.p_is_run)
        {
            reset_calculation(str);
            str->inter.sta = CAL_RMS_STA_IDLE;
        }
        break;
    }
}
