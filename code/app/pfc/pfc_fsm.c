#include "pfc_fsm.h"
#include "stdint.h"
#include "section.h"
#include "my_math.h"
#include "adc_chk.h"
#include "ctrl_pfc.h"
#include "ctrl_inv.h"
#include "gpio.h"
#include "data_com.h"
#include "pwr_cmd.h"
#include "fault.h"

typedef enum
{
    pfc_fsm_sta_init = 1,
    pfc_fsm_sta_idle,
    pfc_fsm_sta_chk_bus,
    pfc_fsm_sta_wait_bus,
    pfc_fsm_sta_rly_on_passby,
    pfc_fsm_sta_rly_on_inv,
    pfc_fsm_sta_soft_start,
    pfc_fsm_sta_chg_passby,
    pfc_fsm_sta_chg,
    pfc_fsm_sta_dsg,
    pfc_fsm_sta_passby,
    pfc_fsm_sta_stop,
    pfc_fsm_sta_fault,
    pfc_fsm_sta_max,
} pfc_fsm_sta_e;

typedef enum
{
    pfc_fsm_ev_init_is_ok = 1,
    pfc_fsm_ev_ready_chg_passby,
    pfc_fsm_ev_ready_chg,
    pfc_fsm_ev_ready_dsg,
    pfc_fsm_ev_bus_is_ok,
    pfc_fsm_ev_rly_is_on,
    pfc_fsm_ev_ss_is_ok,
    pfc_fsm_ev_trig_ups,
    pfc_fsm_ev_to_stop,
    pfc_fsm_ev_to_idle,
    pfc_fsm_ev_to_fault,
} pfc_fsm_ev_e;

static uint32_t fsm_ev = 0;
static uint32_t init_dly_cnt = 0;
static uint32_t rly_on_grid_dly_cnt = 0;
static uint32_t rly_on_passby_dly_cnt = 0;
static uint8_t is_ups_to_inv_trig = 0;
static uint8_t ups_to_inv_flg = 0;
static uint32_t time_cnt = 0;
static uint8_t is_dsg = 0;

static void pfc_fsm_init_in(void)
{
    fault_clr_bit(FAULT_STA_SAMPLE_ERR);
    gpio_set_to_llc(1);
    init_dly_cnt = 0;
    is_dsg = 0;
}
static void pfc_fsm_init_exe(void)
{
    if (init_dly_cnt < TIME_CNT_400MS_IN_1MS)
    {
        init_dly_cnt++;
    }
    if ((data_com_get_llc_is_alive() == 1) &&
        (init_dly_cnt >= TIME_CNT_400MS_IN_1MS))
    {
        fsm_ev = pfc_fsm_ev_init_is_ok;
    }
}
static uint32_t pfc_fsm_init_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_init_is_ok)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}
static void pfc_fsm_init_out(void) {}

static void pfc_fsm_idle_in(void)
{
    gpio_set_to_llc(1);
    is_dsg = 0;
}
static void pfc_fsm_idle_exe(void)
{
    if ((adc_chk_get_ac_is_ok() == 1) &&
        (pwr_cmd_allow_dsg() == 1))
    {
        fsm_ev = pfc_fsm_ev_ready_chg_passby;
    }
    else if (pwr_cmd_allow_dsg() == 1)
    {
        fsm_ev = pfc_fsm_ev_ready_dsg;
    }
}
static uint32_t pfc_fsm_idle_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_ready_chg_passby)
    {
        return pfc_fsm_sta_chk_bus;
    }
    if (fsm_ev == pfc_fsm_ev_ready_dsg)
    {
        return pfc_fsm_sta_wait_bus;
    }
    return 0;
}
static void pfc_fsm_idle_out(void) {}

static void pfc_fsm_chk_bus_in(void)
{
    gpio_set_to_llc(0);
    time_cnt = 0;
    is_dsg = 1;
}
static void pfc_fsm_chk_bus_exe(void)
{
    time_cnt++;
    if (adc_chk_get_grid_ss_v_bus_is_ok() == 1)
    {
        fsm_ev = pfc_fsm_ev_bus_is_ok;
    }
    if (time_cnt > TIME_CNT_5S_IN_1MS)
    {
        fault_set_bit(FAULT_STA_BUS_GRID_SS_ERR);
    }
    if (fault_check() == 1)
    {
        fsm_ev = pfc_fsm_ev_to_fault;
    }
}
static uint32_t pfc_fsm_chk_bus_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_bus_is_ok)
    {
        return pfc_fsm_sta_rly_on_passby;
    }
    if (fsm_ev == pfc_fsm_ev_to_fault)
    {
        return pfc_fsm_sta_fault;
    }
    return 0;
}
static void pfc_fsm_chk_bus_out(void) {}

static void pfc_fsm_wait_bus_in(void)
{
    gpio_set_to_llc(1);
    time_cnt = 0;
    is_dsg = 0;
}
static void pfc_fsm_wait_bus_exe(void)
{
    time_cnt++;
    if (adc_chk_get_bus_is_ok() == 1)
    {
        fsm_ev = pfc_fsm_ev_bus_is_ok;
    }
    if (time_cnt > TIME_CNT_5S_IN_1MS)
    {
        fault_set_bit(FAULT_STA_BUS_LLC_SS_ERR);
    }
    if (fault_check() == 1)
    {
        fsm_ev = pfc_fsm_ev_to_fault;
    }
}
static uint32_t pfc_fsm_wait_bus_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_bus_is_ok)
    {
        return pfc_fsm_sta_rly_on_inv;
    }
    if (fsm_ev == pfc_fsm_ev_to_fault)
    {
        return pfc_fsm_sta_fault;
    }
    return 0;
}
static void pfc_fsm_wait_bus_out(void) {}

static void pfc_fsm_rly_on_passby_in(void)
{
    adc_chk_set_rly_on_grid_trig();
    gpio_set_inv_rly(1);
    gpio_set_to_llc(0);
    rly_on_passby_dly_cnt = 0;
}
static void pfc_fsm_rly_on_passby_exe(void)
{
    rly_on_passby_dly_cnt++;
    if (rly_on_passby_dly_cnt > TIME_CNT_50MS_IN_1MS)
    {
        fsm_ev = pfc_fsm_ev_rly_is_on;
    }
}
static uint32_t pfc_fsm_rly_on_passby_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_rly_is_on)
    {
        return pfc_fsm_sta_soft_start;
    }
    return 0;
}
static void pfc_fsm_rly_on_passby_out(void) {}

static void pfc_fsm_rly_on_inv_in(void)
{
    gpio_set_to_llc(1);
    gpio_set_inv_rly(1);
    rly_on_grid_dly_cnt = 0;
    is_dsg = 1;
}
static void pfc_fsm_rly_on_inv_exe(void)
{
    rly_on_grid_dly_cnt++;
    if ((rly_on_grid_dly_cnt > TIME_CNT_50MS_IN_1MS) ||
        ((ups_to_inv_flg == 1) &&
         (rly_on_grid_dly_cnt > TIME_CNT_5MS_IN_1MS)))
    {
        fsm_ev = pfc_fsm_ev_rly_is_on;
    }
}
static uint32_t pfc_fsm_rly_on_inv_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_rly_is_on)
    {
        return pfc_fsm_sta_dsg;
    }
    return 0;
}
static void pfc_fsm_rly_on_inv_out(void)
{
    ups_to_inv_flg = 0;
}
static void pfc_fsm_soft_start_in(void)
{
    ctrl_pfc_enable();
    time_cnt = 0;
    is_dsg = 1;
}
static void pfc_fsm_soft_start_exe(void)
{
    time_cnt++;
    if (adc_chk_get_bus_is_ok())
    {
        fsm_ev = pfc_fsm_ev_ss_is_ok;
    }
    if (time_cnt > TIME_CNT_5S_IN_1MS)
    {
        fault_set_bit(FAULT_STA_BUS_PFC_SS_ERR);
    }
    if (fault_check() == 1)
    {
        adc_chk_set_rly_off_grid_trig();
        gpio_set_inv_rly(0);
        fsm_ev = pfc_fsm_ev_to_fault;
    }
}
static uint32_t pfc_fsm_soft_start_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_ss_is_ok)
    {
        return pfc_fsm_sta_chg_passby;
    }
    if (fsm_ev == pfc_fsm_ev_to_fault)
    {
        return pfc_fsm_sta_fault;
    }
    return 0;
}
static void pfc_fsm_soft_start_out(void) {}
static void pfc_fsm_chg_in(void) {}
static void pfc_fsm_chg_exe(void) {}
static uint32_t pfc_fsm_chg_chk(uint32_t fsm_ev)
{
    (void)fsm_ev;
    return 0;
}
static void pfc_fsm_chg_out(void) {}
static void pfc_fsm_chg_passby_in(void)
{
    is_ups_to_inv_trig = 0;
    ups_to_inv_flg = 0;
    is_dsg = 1;
}
static void pfc_fsm_chg_passby_exe(void)
{
    if (adc_chk_get_ac_is_ok() == 0)
    {
        if (pwr_cmd_allow_dsg() == 1)
        {
            is_ups_to_inv_trig = 1;
            ups_to_inv_flg = 1;
            fsm_ev = pfc_fsm_ev_trig_ups;
        }
        else
        {
            fsm_ev = pfc_fsm_ev_to_stop;
        }
    }
    if (fault_check() == 1)
    {
        adc_chk_set_rly_off_grid_trig();
        gpio_set_inv_rly(0);
        fsm_ev = pfc_fsm_ev_to_fault;
    }
}
static uint32_t pfc_fsm_chg_passby_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_trig_ups)
    {
        return pfc_fsm_sta_rly_on_inv;
    }
    if (fsm_ev == pfc_fsm_ev_to_stop)
    {
        return pfc_fsm_sta_stop;
    }
    if (fsm_ev == pfc_fsm_ev_to_fault)
    {
        return pfc_fsm_sta_fault;
    }
    return 0;
}
static void pfc_fsm_chg_passby_out(void)
{
    ctrl_pfc_disable();
    adc_chk_set_rly_off_grid_trig();
    if (ups_to_inv_flg == 0)
    {
        gpio_set_inv_rly(0);
    }
}
static void pfc_fsm_dsg_in(void)
{
    gpio_set_to_llc(1);
    ctrl_inv_enable();
    is_dsg = 1;
}
static void pfc_fsm_dsg_exe(void)
{
    if (pwr_cmd_allow_dsg() == 0)
    {
        fsm_ev = pfc_fsm_ev_to_stop;
    }
    else if (adc_chk_get_ac_is_ok() == 1)
    {
        fsm_ev = pfc_fsm_ev_ready_chg_passby;
    }
    else if(adc_chk_get_bus_is_ok() == 0)
    {
        fault_set_bit(FAULT_STA_BUS_ERR);
    }
    if (fault_check() == 1)
    {
        fsm_ev = pfc_fsm_ev_to_fault;
    }
}
static uint32_t pfc_fsm_dsg_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_ready_chg_passby)
    {
        return pfc_fsm_sta_rly_on_passby;
    }
    if (fsm_ev == pfc_fsm_ev_to_stop)
    {
        return pfc_fsm_sta_stop;
    }
    if (fsm_ev == pfc_fsm_ev_to_fault)
    {
        return pfc_fsm_sta_fault;
    }
    return 0;
}
static void pfc_fsm_dsg_out(void)
{
    ctrl_inv_disable();
}
static void pfc_fsm_passby_in(void) {}
static void pfc_fsm_passby_exe(void) {}
static uint32_t pfc_fsm_passby_chk(uint32_t fsm_ev)
{
    (void)fsm_ev;
    return 0;
}
static void pfc_fsm_passby_out(void) {}
static void pfc_fsm_stop_in(void)
{
    ctrl_inv_disable();
    ctrl_pfc_disable();
    is_dsg = 0;
    adc_chk_set_rly_off_grid_trig();
    gpio_set_inv_rly(0);
}
static void pfc_fsm_stop_exe(void)
{
    fsm_ev = pfc_fsm_ev_to_idle;
}
static uint32_t pfc_fsm_stop_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_idle)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}
static void pfc_fsm_stop_out(void) {}
static void pfc_fsm_fault_in(void)
{
    is_dsg = 0;
    ctrl_pfc_disable();
    ctrl_inv_disable();
    adc_chk_set_rly_off_grid_trig();
    gpio_set_inv_rly(0);
}
static void pfc_fsm_fault_exe(void) {}
static uint32_t pfc_fsm_fault_chk(uint32_t fsm_ev)
{
    (void)fsm_ev;
    return 0;
}
static void pfc_fsm_fault_out(void) {}

REG_FSM(pfc_fsm, pfc_fsm_sta_init, fsm_ev,
        FSM_ENTRY(pfc_fsm_sta_init,
                  pfc_fsm_init_in,
                  pfc_fsm_init_exe,
                  pfc_fsm_init_chk,
                  pfc_fsm_init_out),

        FSM_ENTRY(pfc_fsm_sta_idle,
                  pfc_fsm_idle_in,
                  pfc_fsm_idle_exe,
                  pfc_fsm_idle_chk,
                  pfc_fsm_idle_out),

        FSM_ENTRY(pfc_fsm_sta_chk_bus,
                  pfc_fsm_chk_bus_in,
                  pfc_fsm_chk_bus_exe,
                  pfc_fsm_chk_bus_chk,
                  pfc_fsm_chk_bus_out),

        FSM_ENTRY(pfc_fsm_sta_wait_bus,
                  pfc_fsm_wait_bus_in,
                  pfc_fsm_wait_bus_exe,
                  pfc_fsm_wait_bus_chk,
                  pfc_fsm_wait_bus_out),

        FSM_ENTRY(pfc_fsm_sta_rly_on_passby,
                  pfc_fsm_rly_on_passby_in,
                  pfc_fsm_rly_on_passby_exe,
                  pfc_fsm_rly_on_passby_chk,
                  pfc_fsm_rly_on_passby_out),

        FSM_ENTRY(pfc_fsm_sta_rly_on_inv,
                  pfc_fsm_rly_on_inv_in,
                  pfc_fsm_rly_on_inv_exe,
                  pfc_fsm_rly_on_inv_chk,
                  pfc_fsm_rly_on_inv_out),

        FSM_ENTRY(pfc_fsm_sta_soft_start,
                  pfc_fsm_soft_start_in,
                  pfc_fsm_soft_start_exe,
                  pfc_fsm_soft_start_chk,
                  pfc_fsm_soft_start_out),

        FSM_ENTRY(pfc_fsm_sta_chg_passby,
                  pfc_fsm_chg_passby_in,
                  pfc_fsm_chg_passby_exe,
                  pfc_fsm_chg_passby_chk,
                  pfc_fsm_chg_passby_out),

        FSM_ENTRY(pfc_fsm_sta_chg,
                  pfc_fsm_chg_in,
                  pfc_fsm_chg_exe,
                  pfc_fsm_chg_chk,
                  pfc_fsm_chg_out),

        FSM_ENTRY(pfc_fsm_sta_dsg,
                  pfc_fsm_dsg_in,
                  pfc_fsm_dsg_exe,
                  pfc_fsm_dsg_chk,
                  pfc_fsm_dsg_out),

        FSM_ENTRY(pfc_fsm_sta_passby,
                  pfc_fsm_passby_in,
                  pfc_fsm_passby_exe,
                  pfc_fsm_passby_chk,
                  pfc_fsm_passby_out),

        FSM_ENTRY(pfc_fsm_sta_stop,
                  pfc_fsm_stop_in,
                  pfc_fsm_stop_exe,
                  pfc_fsm_stop_chk,
                  pfc_fsm_stop_out),

        FSM_ENTRY(pfc_fsm_sta_fault,
                  pfc_fsm_fault_in,
                  pfc_fsm_fault_exe,
                  pfc_fsm_fault_chk,
                  pfc_fsm_fault_out))

uint8_t pfc_fsm_get_is_ups_to_inv_trig(void)
{
    uint8_t temp = is_ups_to_inv_trig;
    is_ups_to_inv_trig = 0;
    return temp;
}

REG_SHELL_VAR(PFC_FSM_STA, FSM_GET_STATE(pfc_fsm), SHELL_UINT32, pfc_fsm_sta_max, 0, NULL, SHELL_STA_AUTO)

uint8_t pfc_fsm_get_is_dsg(void)
{
    return is_dsg;
}

static void ctrl_grid_rly_on_test(DEC_MY_PRINTF)
{
    (void)my_printf;
    adc_chk_set_rly_on_grid_trig();
}

static void ctrl_grid_rly_off_test(DEC_MY_PRINTF)
{
    (void)my_printf;
    adc_chk_set_rly_off_grid_trig();
}

REG_SHELL_CMD(GRID_RLY_ON, ctrl_grid_rly_on_test)
REG_SHELL_CMD(GRID_RLY_OFF, ctrl_grid_rly_off_test)

static void ctrl_gpio_grid_rly_on_test(DEC_MY_PRINTF)
{
    (void)my_printf;
    gpio_set_grid_rly(1);
}

static void ctrl_gpio_grid_rly_off_test(DEC_MY_PRINTF)
{
    (void)my_printf;
    gpio_set_grid_rly(0);
}

REG_SHELL_CMD(GPIO_GRID_RLY_ON, ctrl_gpio_grid_rly_on_test)
REG_SHELL_CMD(GPIO_GRID_RLY_OFF, ctrl_gpio_grid_rly_off_test)

