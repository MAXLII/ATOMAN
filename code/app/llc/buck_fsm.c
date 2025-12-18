#include "buck_fsm.h"
#include "section.h"
#include "my_math.h"
#include "gpio.h"
#include "adc_chk.h"
#include "ctrl_buck.h"
#include "data_com.h"
#include "pwr_on.h"
#include "fault.h"
#include "pwr_cmd.h"
#include "bms_logic.h"

enum
{
    buck_fsm_sta_init = 1,
    buck_fsm_sta_idle,
    buck_fsm_sta_soft_start,
    buck_fsm_sta_main_rly_dly,
    buck_fsm_sta_chg,
    buck_fsm_sta_stop,
    buck_fsm_sta_fault,
};

enum
{
    buck_fsm_ev_init_is_ok = 1,
    buck_fsm_ev_ready_chg,
    buck_fsm_ev_soft_start_is_ok,
    buck_fsm_ev_main_rly_is_ok,
    buck_fsm_ev_chkout_chg,
    buck_fsm_ev_fault,
    buck_fsm_ev_to_idle,
};

static uint32_t fsm_ev;
static uint32_t dly_cnt = 0;

void buck_fsm_init_in(void)
{
    dly_cnt = 0;
}
void buck_fsm_init_exe(void)
{
    if ((pwr_on_get_aux_is_on() == 1) &&
        (bms_logic_bat_is_dsg() == 1))
    {
        fsm_ev = buck_fsm_ev_init_is_ok;
    }
}
uint32_t buck_fsm_init_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_init_is_ok)
    {
        return buck_fsm_sta_idle;
    }
    return 0;
}
void buck_fsm_init_out(void) {}
void buck_fsm_idle_in(void)
{
}
void buck_fsm_idle_exe(void)
{
    if ((adc_chk_get_pv_in_is_ok() == 1) &&
        (pwr_cmd_get_pv_chg() == 1) &&
        (fault_check() == 0))
    {
        fsm_ev = buck_fsm_ev_ready_chg;
    }
}
uint32_t buck_fsm_idle_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_ready_chg)
    {
        return buck_fsm_sta_soft_start;
    }
    return 0;
}
void buck_fsm_idle_out(void) {}
void buck_fsm_soft_start_in(void)
{
    dly_cnt = 0;
}
void buck_fsm_soft_start_exe(void)
{
    dly_cnt++;
    if (dly_cnt > TIME_CNT_1S_IN_1MS) // 上升到93%
    {
        fsm_ev = buck_fsm_ev_soft_start_is_ok;
    }
    else if (adc_chk_get_pv_in_is_ok() == 0)
    {
        fsm_ev = buck_fsm_ev_to_idle;
    }
}
uint32_t buck_fsm_soft_start_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_soft_start_is_ok)
    {
        return buck_fsm_sta_main_rly_dly;
    }
    if (fsm_ev == buck_fsm_ev_to_idle)
    {
        return buck_fsm_sta_idle;
    }
    return 0;
}
void buck_fsm_soft_start_out(void)
{
}
void buck_fsm_buck_main_rly_dly_in(void)
{
    gpio_set_buck_main_rly(1);
    dly_cnt = 0;
}
void buck_fsm_buck_main_rly_dly_exe(void)
{
    dly_cnt++;
    if (dly_cnt > TIME_CNT_10MS_IN_1MS)
    {
        fsm_ev = buck_fsm_ev_main_rly_is_ok;
    }
}
uint32_t buck_fsm_buck_main_rly_dly_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_main_rly_is_ok)
    {
        return buck_fsm_sta_chg;
    }
    return 0;
}
void buck_fsm_buck_main_rly_dly_out(void) {}
void buck_fsm_chg_in(void)
{
    ctrl_buck_enable();
}
void buck_fsm_chg_exe(void)
{
    if ((pwr_cmd_get_pv_chg() == 0) ||
        (adc_chk_get_pv_in_is_ok() == 0))
    {
        fsm_ev = buck_fsm_ev_chkout_chg;
    }
    if (fault_check() == 1)
    {
        fsm_ev = buck_fsm_ev_fault;
    }
}
uint32_t buck_fsm_chg_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_chkout_chg)
    {
        return buck_fsm_sta_stop;
    }
    if (fsm_ev == buck_fsm_ev_fault)
    {
        return buck_fsm_sta_fault;
    }
    return 0;
}
void buck_fsm_chg_out(void)
{
    gpio_set_buck_main_rly(0);
    ctrl_buck_disable();
}
void buck_fsm_stop_in(void) {}
void buck_fsm_stop_exe(void)
{
    fsm_ev = buck_fsm_ev_to_idle;
}
uint32_t buck_fsm_stop_chk(uint32_t fsm_ev)
{
    if (fsm_ev == buck_fsm_ev_to_idle)
    {
        return buck_fsm_sta_idle;
    }
    return 0;
}
void buck_fsm_stop_out(void) {}
void buck_fsm_fault_in(void) {}
void buck_fsm_fault_exe(void) {}
uint32_t buck_fsm_fault_chk(uint32_t fsm_ev)
{
    (void)fsm_ev;
    return 0;
}
void buck_fsm_fault_out(void) {}

REG_FSM(buck_fsm, buck_fsm_sta_init, fsm_ev,
        FSM_ENTRY(buck_fsm_sta_init,
                  buck_fsm_init_in,
                  buck_fsm_init_exe,
                  buck_fsm_init_chk,
                  buck_fsm_init_out),
        FSM_ENTRY(buck_fsm_sta_idle,
                  buck_fsm_idle_in,
                  buck_fsm_idle_exe,
                  buck_fsm_idle_chk,
                  buck_fsm_idle_out),
        FSM_ENTRY(buck_fsm_sta_soft_start,
                  buck_fsm_soft_start_in,
                  buck_fsm_soft_start_exe,
                  buck_fsm_soft_start_chk,
                  buck_fsm_soft_start_out),
        FSM_ENTRY(buck_fsm_sta_main_rly_dly,
                  buck_fsm_buck_main_rly_dly_in,
                  buck_fsm_buck_main_rly_dly_exe,
                  buck_fsm_buck_main_rly_dly_chk,
                  buck_fsm_buck_main_rly_dly_out),
        FSM_ENTRY(buck_fsm_sta_chg,
                  buck_fsm_chg_in,
                  buck_fsm_chg_exe,
                  buck_fsm_chg_chk,
                  buck_fsm_chg_out),
        FSM_ENTRY(buck_fsm_sta_stop,
                  buck_fsm_stop_in,
                  buck_fsm_stop_exe,
                  buck_fsm_stop_chk,
                  buck_fsm_stop_out),
        FSM_ENTRY(buck_fsm_sta_fault,
                  buck_fsm_fault_in,
                  buck_fsm_fault_exe,
                  buck_fsm_fault_chk,
                  buck_fsm_fault_out))

REG_SHELL_VAR(BUCK_FSM_STA, FSM_GET_STATE(buck_fsm), SHELL_UINT32, 0xFF, 0, NULL, SHELL_STA_NULL)
