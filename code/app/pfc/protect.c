#include "protect.h"
#include "hys_cmp.h"
#include "section.h"
#include "adc_chk.h"
#include "fault.h"

hys_cmp_t ac_out_ocp[AC_OUT_OCP_LV_MAX] = {0};

hys_cmp_cfg_t ac_out_ocp_cfg[AC_OUT_OCP_LV_MAX] = {
    {
        .thr = AC_OUT_OCP_LV1_THR,
        .thr_hys = AC_OUT_OCP_LV1_THR_HYS,
        .time = AC_OUT_OCP_LV1_TIME,
        .time_hys = AC_OUT_OCP_LV1_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_OCP_LV2_THR,
        .thr_hys = AC_OUT_OCP_LV2_THR_HYS,
        .time = AC_OUT_OCP_LV2_TIME,
        .time_hys = AC_OUT_OCP_LV2_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_OCP_LV3_THR,
        .thr_hys = AC_OUT_OCP_LV3_THR_HYS,
        .time = AC_OUT_OCP_LV3_TIME,
        .time_hys = AC_OUT_OCP_LV3_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
};

hys_cmp_t ac_out_actopp[AC_OUT_ACTOPP_LV_MAX] = {0};

hys_cmp_cfg_t ac_out_actopp_cfg[AC_OUT_ACTOPP_LV_MAX] = {
    {
        .thr = AC_OUT_ACTOPP_LV1_THR,
        .thr_hys = AC_OUT_ACTOPP_LV1_THR_HYS,
        .time = AC_OUT_ACTOPP_LV1_TIME,
        .time_hys = AC_OUT_ACTOPP_LV1_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_ACTOPP_LV2_THR,
        .thr_hys = AC_OUT_ACTOPP_LV2_THR_HYS,
        .time = AC_OUT_ACTOPP_LV2_TIME,
        .time_hys = AC_OUT_ACTOPP_LV2_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_ACTOPP_LV3_THR,
        .thr_hys = AC_OUT_ACTOPP_LV3_THR_HYS,
        .time = AC_OUT_ACTOPP_LV3_TIME,
        .time_hys = AC_OUT_ACTOPP_LV3_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
};

hys_cmp_t ac_out_ttlopp[AC_OUT_TTLOPP_LV_MAX] = {0};

hys_cmp_cfg_t ac_out_ttlopp_cfg[AC_OUT_TTLOPP_LV_MAX] = {
    {
        .thr = AC_OUT_TTLOPP_LV1_THR,
        .thr_hys = AC_OUT_TTLOPP_LV1_THR_HYS,
        .time = AC_OUT_TTLOPP_LV1_TIME,
        .time_hys = AC_OUT_TTLOPP_LV1_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_TTLOPP_LV2_THR,
        .thr_hys = AC_OUT_TTLOPP_LV2_THR_HYS,
        .time = AC_OUT_TTLOPP_LV2_TIME,
        .time_hys = AC_OUT_TTLOPP_LV2_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
    {
        .thr = AC_OUT_TTLOPP_LV3_THR,
        .thr_hys = AC_OUT_TTLOPP_LV3_THR_HYS,
        .time = AC_OUT_TTLOPP_LV3_TIME,
        .time_hys = AC_OUT_TTLOPP_LV3_TIME_HYS,
        .p_cmp_func = cmp_gt,
        .p_cmp_hys_func = cmp_lt,
    },
};

void protect_init(void)
{
    for (uint8_t i = 0; i < AC_OUT_OCP_LV_MAX; i++)
    {
        hys_cmp_init(&ac_out_ocp[i],
                     &cal_rms_i_out.output.rms,
                     &ac_out_ocp_cfg[i]);
    }

    for (uint8_t i = 0; i < AC_OUT_ACTOPP_LV_MAX; i++)
    {
        hys_cmp_init(&ac_out_actopp[i],
                     &ac_out_pwr_rms,
                     &ac_out_actopp_cfg[i]);
    }
    for (uint8_t i = 0; i < AC_OUT_TTLOPP_LV_MAX; i++)
    {
        hys_cmp_init(&ac_out_ttlopp[i],
                     &ac_out_pwr_total,
                     &ac_out_ttlopp_cfg[i]);
    }
}

void protect_fast_task(void)
{
}

void protect_slow_task(void)
{
    for (uint8_t i = 0; i < AC_OUT_OCP_LV_MAX; i++)
    {
        hys_cmp_func(&ac_out_ocp[i]);
        if (ac_out_ocp[0].output.is_protect == 1)
        {
            fault_set_bit(FAULT_STA_AC_OUT_OCP);
        }
    }

    for (uint8_t i = 0; i < AC_OUT_ACTOPP_LV_MAX; i++)
    {
        hys_cmp_func(&ac_out_actopp[i]);
        if (ac_out_actopp[0].output.is_protect == 1)
        {
            fault_set_bit(FAULT_STA_AC_OUT_ACTOPP);
        }
    }

    for (uint8_t i = 0; i < AC_OUT_TTLOPP_LV_MAX; i++)
    {
        hys_cmp_func(&ac_out_ttlopp[i]);
        if (ac_out_ttlopp[0].output.is_protect == 1)
        {
            fault_set_bit(FAULT_STA_AC_OUT_TTLOPP);
        }
    }
}

REG_TASK_MS(1, protect_slow_task)
