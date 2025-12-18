#include "protect.h"
#include "section.h"
#include "adc_chk.h"
#include "fault.h"
#include "my_math.h"
#include "pwm.h"
#include "ctrl_buck.h"
#include "hys_cmp.h"
#include "ctrl_buck.h"

static hys_cmp_t mppt_in_ovp = {0};
static hys_cmp_cfg_t mppt_in_ovp_cfg = {
    .thr = MPPT_IN_OVP_THR,
    .thr_hys = MPPT_IN_OVP_THR,
    .time = MPPT_IN_OVP_TIME,
    .time_hys = MPPT_IN_OVP_TIME_HYS,
    .p_cmp_func = cmp_gt,
    .p_cmp_hys_func = cmp_lt,
};

static hys_cmp_t mppt_l_ocp = {0};
static hys_cmp_cfg_t mppt_l_ocp_cfg = {
    .thr = MPPT_L_OCP_THR,
    .thr_hys = MPPT_L_OCP_THR,
    .time = MPPT_L_OCP_TIME,
    .time_hys = MPPT_L_OCP_TIME_HYS,
    .p_cmp_func = cmp_gt,
    .p_cmp_hys_func = cmp_lt,
};

void protect_init(void)
{
    hys_cmp_init(&mppt_in_ovp,
                 &v_buck_in,
                 &mppt_in_ovp_cfg);

    hys_cmp_init(&mppt_l_ocp,
                 &i_buck_in,
                 &mppt_l_ocp_cfg);
}

void protect_fast_task(void)
{
    if (pwm_get_buck_is_hard_ocp())
    {
        fault_set_bit(FAUTL_STA_MPPT_L_HARD_OCP);
        ctrl_buck_disable();
    }
}

REG_INTERRUPT(0, protect_fast_task)

void protect_slow_task(void)
{
    hys_cmp_func(&mppt_in_ovp);
    hys_cmp_func(&mppt_l_ocp);

    if (mppt_in_ovp.output.is_protect)
    {
        fault_set_bit(FAULT_STA_MPPT_IN_OVP);
    }
    else
    {
        fault_clr_bit(FAULT_STA_MPPT_IN_OVP);
    }

    if (mppt_l_ocp.output.is_protect)
    {
        fault_set_bit(FAULT_STA_MPPT_L_SOFT_OCP);
    }
}

REG_TASK_MS(1, protect_slow_task)
