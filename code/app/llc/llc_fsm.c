#include "llc_fsm.h"
#include "section.h"
#include "my_math.h"
#include "adc_chk.h"
#include "llc_hardware.h"
#include "ctrl_llc_dsg.h"
#include "ctrl_llc_chg.h"
#include "data_com.h"
#include "gpio.h"
#include "pwr_on.h"
#include "fault.h"
#include "pwr_cmd.h"
#include "bms_logic.h"

enum
{
    llc_fsm_sta_init = 1,
    llc_fsm_sta_bat_dsg,
    llc_fsm_sta_idle,
    llc_fsm_sta_soft_start,
    llc_fsm_sta_wait_bus,
    llc_fsm_sta_dsg,
    llc_fsm_sta_chg,
    llc_fsm_sta_stop,
    llc_fsm_sta_fault,
};

enum
{
    llc_fsm_ev_init_is_ok = 1,
    llc_fsm_ev_ready_chg,
    llc_fsm_ev_ready_dsg,
    llc_fsm_ev_v_bus_is_ok,
    llc_fsm_ev_to_stop,
    llc_fsm_ev_to_idle,
    llc_fsm_ev_to_fault,
};

static uint8_t is_ups_trig = 0;
static uint32_t fsm_ev;
static uint32_t time_cnt = 0;

static void llc_fsm_init_in(void)
{
}

static void llc_fsm_init_exe(void)
{
    if ((pwr_on_get_aux_is_on() == 1) &&
        (bms_logic_bat_is_dsg() == 1))
    {
        fsm_ev = llc_fsm_ev_init_is_ok;
    }
}

static uint32_t llc_fsm_init_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_init_is_ok)
    {
        return llc_fsm_sta_idle;
    }
    return 0;
}

static void llc_fsm_init_out(void)
{
}

static void llc_fsm_idle_in(void) {}
static void llc_fsm_idle_exe(void)
{
    if (fault_check() == 1)
    {
        return;
    }
    if ((pwr_cmd_get_ac_chg() == 1) &&
        (gpio_get_from_pfc() == 0))
    {
        fsm_ev = llc_fsm_ev_ready_chg;
    }
    else if ((pwr_cmd_get_inv_dsg() == 1) &&
             (gpio_get_from_pfc() == 1))
    {
        if (data_com_get_pfc_is_dsg() == 1)
        {
            is_ups_trig = 1;
        }
        fsm_ev = llc_fsm_ev_ready_dsg;
    }
}
static uint32_t llc_fsm_idle_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_ready_chg)
    {
        return llc_fsm_sta_wait_bus;
    }
    if (fsm_ev == llc_fsm_ev_ready_dsg)
    {
        return llc_fsm_sta_dsg;
    }
    return 0;
}
static void llc_fsm_idle_out(void) {}

static void llc_fsm_soft_start_in(void)
{
}
static void llc_fsm_soft_start_exe(void)
{
}
static uint32_t llc_fsm_soft_start_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_v_bus_is_ok)
    {
        return llc_fsm_sta_dsg;
    }
    return 0;
}
static void llc_fsm_soft_start_out(void) {}

static void llc_fsm_wait_bus_in(void)
{
    time_cnt = 0;
}

static void llc_fsm_wait_bus_exe(void)
{
    time_cnt++;
    if (data_com_get_bus_is_ok())
    {
        fsm_ev = llc_fsm_ev_v_bus_is_ok;
    }
    else if ((gpio_get_from_pfc() == 1) &&
             (pwr_cmd_get_inv_dsg() == 1))
    {
        fsm_ev = llc_fsm_ev_ready_dsg;
    }

    if ((gpio_get_from_pfc() == 1) &&
        (pwr_cmd_get_inv_dsg() == 0))
    {
        fsm_ev = llc_fsm_ev_to_stop;
    }

    if (time_cnt > TIME_CNT_5S_IN_1MS)
    {
        fault_set_bit(FAULT_STA_BUS_PFC_SS_ERR);
    }

    if (fault_check() == 1)
    {
        fsm_ev = llc_fsm_ev_to_fault;
    }
}

static uint32_t llc_fsm_wait_bus_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_v_bus_is_ok)
    {
        return llc_fsm_sta_chg;
    }
    if (fsm_ev == llc_fsm_ev_ready_dsg)
    {
        return llc_fsm_sta_dsg;
    }
    if (fsm_ev == llc_fsm_ev_to_stop)
    {
        return llc_fsm_sta_stop;
    }
    if (fsm_ev == llc_fsm_ev_to_fault)
    {
        return llc_fsm_sta_fault;
    }
    return 0;
}

static void llc_fsm_wait_bus_out(void) {}

static void llc_fsm_dsg_in(void)
{
    ctrl_llc_dsg_enable();
}
static void llc_fsm_dsg_exe(void)
{
    if (gpio_get_from_pfc() == 0)
    {
        if (pwr_cmd_get_ac_chg() == 1)
        {
            fsm_ev = llc_fsm_ev_ready_chg;
        }
        else
        {
            fsm_ev = llc_fsm_ev_to_stop;
        }
    }
    else if (pwr_cmd_get_inv_dsg() == 0)
    {
        fsm_ev = llc_fsm_ev_to_stop;
    }
    if (fault_check() == 1)
    {
        fsm_ev = llc_fsm_ev_to_fault;
    }
}
static uint32_t llc_fsm_dsg_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_ready_chg)
    {
        return llc_fsm_sta_wait_bus;
    }
    if (fsm_ev == llc_fsm_ev_to_stop)
    {
        return llc_fsm_sta_stop;
    }
    if (fsm_ev == llc_fsm_ev_to_fault)
    {
        return llc_fsm_sta_fault;
    }
    return 0;
}
static void llc_fsm_dsg_out(void)
{
    ctrl_llc_dsg_disable();
}

static void llc_fsm_chg_in(void)
{
    ctrl_llc_chg_enable();
}
static void llc_fsm_chg_exe(void)
{
    if (gpio_get_from_pfc() == 1) // 先判断是不是要切放电
    {
        if (data_com_get_pfc_is_dsg() == 1)
        {
            is_ups_trig = 1;
            fsm_ev = llc_fsm_ev_ready_dsg;
        }
        else
        {
            fsm_ev = llc_fsm_ev_to_stop;
        }
    }
    else if (pwr_cmd_get_ac_chg() == 0) // 再判断是不是不允许充电
    {
        fsm_ev = llc_fsm_ev_to_stop;
    }
    if (fault_check() == 1)
    {
        fsm_ev = llc_fsm_ev_to_fault;
    }
}
static uint32_t llc_fsm_chg_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_ready_dsg)
    {
        return llc_fsm_sta_dsg;
    }
    if (fsm_ev == llc_fsm_ev_to_fault)
    {
        return llc_fsm_sta_fault;
    }
    if (fsm_ev == llc_fsm_ev_to_stop)
    {
        return llc_fsm_sta_stop;
    }
    return 0;
}
static void llc_fsm_chg_out(void)
{
    ctrl_llc_chg_disable();
}

static void llc_fsm_stop_in(void) {}
static void llc_fsm_stop_exe(void)
{
    fsm_ev = llc_fsm_ev_to_idle;
}
static uint32_t llc_fsm_stop_chk(uint32_t fsm_ev)
{
    if (fsm_ev == llc_fsm_ev_to_idle)
    {
        return llc_fsm_sta_idle;
    }
    return 0;
}
static void llc_fsm_stop_out(void) {}

static void llc_fsm_fault_in(void) {}
static void llc_fsm_fault_exe(void) {}
static uint32_t llc_fsm_fault_chk(uint32_t fsm_ev)
{
    (void)fsm_ev;
    return 0;
}
static void llc_fsm_fault_out(void) {}

REG_FSM(llc_fsm, llc_fsm_sta_init, fsm_ev,
        FSM_ENTRY(llc_fsm_sta_init,
                  llc_fsm_init_in,
                  llc_fsm_init_exe,
                  llc_fsm_init_chk,
                  llc_fsm_init_out),

        FSM_ENTRY(llc_fsm_sta_idle,
                  llc_fsm_idle_in,
                  llc_fsm_idle_exe,
                  llc_fsm_idle_chk,
                  llc_fsm_idle_out),

        FSM_ENTRY(llc_fsm_sta_soft_start,
                  llc_fsm_soft_start_in,
                  llc_fsm_soft_start_exe,
                  llc_fsm_soft_start_chk,
                  llc_fsm_soft_start_out),

        FSM_ENTRY(llc_fsm_sta_wait_bus,
                  llc_fsm_wait_bus_in,
                  llc_fsm_wait_bus_exe,
                  llc_fsm_wait_bus_chk,
                  llc_fsm_wait_bus_out),

        FSM_ENTRY(llc_fsm_sta_dsg,
                  llc_fsm_dsg_in,
                  llc_fsm_dsg_exe,
                  llc_fsm_dsg_chk,
                  llc_fsm_dsg_out),

        FSM_ENTRY(llc_fsm_sta_chg,
                  llc_fsm_chg_in,
                  llc_fsm_chg_exe,
                  llc_fsm_chg_chk,
                  llc_fsm_chg_out),

        FSM_ENTRY(llc_fsm_sta_stop,
                  llc_fsm_stop_in,
                  llc_fsm_stop_exe,
                  llc_fsm_stop_chk,
                  llc_fsm_stop_out),

        FSM_ENTRY(llc_fsm_sta_fault,
                  llc_fsm_fault_in,
                  llc_fsm_fault_exe,
                  llc_fsm_fault_chk,
                  llc_fsm_fault_out))

uint8_t llc_fsm_get_is_ups_trig(void)
{
    uint8_t temp = is_ups_trig;
    is_ups_trig = 0;
    return temp;
}

void llc_fsm_task(void)
{
}

REG_TASK_MS(1, llc_fsm_task)
