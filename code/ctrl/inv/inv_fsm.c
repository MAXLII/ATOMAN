#include "inv_fsm.h"
#include "inv_cfg.h"
#include "my_math.h"

static inv_fsm_hal_t *p_hal = NULL;
static inv_fsm_cmd_e fsm_cmd = inv_fsm_cmd_null;
static uint32_t init_dly = 0U;
static uint32_t inv_rly_on_dly = 0U;
static inv_fsm_ev_e fsm_ev = inv_fsm_ev_null;

void inv_fsm_set_cmd(inv_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

void inv_fsm_set_p_hal(inv_fsm_hal_t *p)
{
    p_hal = p;
}

static inv_fsm_cmd_e inv_fsm_get_cmd(void)
{
    inv_fsm_cmd_e cmd = fsm_cmd;
    fsm_cmd = inv_fsm_cmd_null;
    return cmd;
}

static void inv_fsm_init_in(void)
{
    init_dly = 0U;
    PLECS_LOG("inv_fsm enter init\n");
}

static void inv_fsm_init_exe(void)
{
    if (init_dly < TIME_CNT_200MS_IN_1MS)
    {
        init_dly++;
    }
    else
    {
        if ((p_hal != NULL) &&
            (inv_cfg_is_ready() == 1U))
        {
            PLECS_LOG("inv_fsm init ready, goto idle\n");
            fsm_ev = inv_fsm_ev_to_idle;
        }
        else
        {
            PLECS_LOG("inv_fsm init dep invalid\n");
        }
    }
}

static uint32_t inv_fsm_init_chk(uint32_t event)
{
    if (event == inv_fsm_ev_to_idle)
    {
        return inv_fsm_sta_idle;
    }

    return 0U;
}

static void inv_fsm_init_out(void)
{
    PLECS_LOG("inv_fsm leave init\n");
}

static void inv_fsm_idle_in(void)
{
    PLECS_LOG("inv_fsm enter idle\n");

    if ((p_hal != NULL) &&
        (p_hal->p_inv_rly_off_func != NULL))
    {
        p_hal->p_inv_rly_off_func();
        PLECS_LOG("inv_fsm relay off\n");
    }
}

static void inv_fsm_idle_exe(void)
{
    if (inv_fsm_get_cmd() == inv_fsm_cmd_start)
    {
        if (*p_hal->p_latched == 1)
        {
            PLECS_LOG("inv_fsm start rejected by hard protect latch\n");
            return;
        }

        PLECS_LOG("inv_fsm idle got start, goto relay_on\n");
        fsm_ev = inv_fsm_ev_to_rly_on;
    }
}

static uint32_t inv_fsm_idle_chk(uint32_t event)
{
    if (event == inv_fsm_ev_to_rly_on)
    {
        return inv_fsm_sta_rly_on;
    }

    return 0U;
}

static void inv_fsm_idle_out(void)
{
    PLECS_LOG("inv_fsm leave idle\n");
}

static void inv_fsm_rly_on_in(void)
{
    inv_rly_on_dly = 0U;
    PLECS_LOG("inv_fsm enter relay_on\n");

    if ((p_hal != NULL) &&
        (p_hal->p_inv_rly_on_func != NULL))
    {
        p_hal->p_inv_rly_on_func();
        PLECS_LOG("inv_fsm relay on\n");
    }
}

static void inv_fsm_rly_on_exe(void)
{
    if (inv_rly_on_dly < TIME_CNT_50MS_IN_1MS)
    {
        inv_rly_on_dly++;
    }

    if (inv_fsm_get_cmd() == inv_fsm_cmd_stop)
    {
        PLECS_LOG("inv_fsm relay_on got stop, goto idle\n");
        fsm_ev = inv_fsm_ev_to_idle;
        return;
    }

    if (inv_rly_on_dly >= TIME_CNT_50MS_IN_1MS)
    {
        PLECS_LOG("inv_fsm relay_on delay done, goto run\n");
        fsm_ev = inv_fsm_ev_to_run;
    }
}

static uint32_t inv_fsm_rly_on_chk(uint32_t event)
{
    if (event == inv_fsm_ev_to_run)
    {
        return inv_fsm_sta_run;
    }

    if (event == inv_fsm_ev_to_idle)
    {
        return inv_fsm_sta_idle;
    }

    return 0U;
}

static void inv_fsm_rly_on_out(void)
{
    PLECS_LOG("inv_fsm leave relay_on\n");
}

static void inv_fsm_run_in(void)
{
    PLECS_LOG("inv_fsm enter run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL))
    {
        p_hal->p_enter_run_func();
        PLECS_LOG("inv_fsm control enabled\n");
    }
}

static void inv_fsm_run_exe(void)
{
    if (inv_fsm_get_cmd() == inv_fsm_cmd_stop)
    {
        PLECS_LOG("inv_fsm run got stop, goto idle\n");
        fsm_ev = inv_fsm_ev_to_idle;
    }
}

static uint32_t inv_fsm_run_chk(uint32_t event)
{
    if (event == inv_fsm_ev_to_idle)
    {
        return inv_fsm_sta_idle;
    }

    return 0U;
}

static void inv_fsm_run_out(void)
{
    PLECS_LOG("inv_fsm leave run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        p_hal->p_exit_run_func();
        PLECS_LOG("inv_fsm control disabled\n");
    }
}

REG_FSM(INV_FSM, inv_fsm_sta_init, fsm_ev,
        FSM_ENTRY(inv_fsm_sta_init, inv_fsm_init_in, inv_fsm_init_exe, inv_fsm_init_chk, inv_fsm_init_out),
        FSM_ENTRY(inv_fsm_sta_idle, inv_fsm_idle_in, inv_fsm_idle_exe, inv_fsm_idle_chk, inv_fsm_idle_out),
        FSM_ENTRY(inv_fsm_sta_rly_on, inv_fsm_rly_on_in, inv_fsm_rly_on_exe, inv_fsm_rly_on_chk, inv_fsm_rly_on_out),
        FSM_ENTRY(inv_fsm_sta_run, inv_fsm_run_in, inv_fsm_run_exe, inv_fsm_run_chk, inv_fsm_run_out), )

inv_run_sta_e inv_fsm_get_run_sta(void)
{
    inv_fsm_sta_e sta = (inv_fsm_sta_e)FSM_GET_STATE(INV_FSM);

    if (sta == inv_fsm_sta_init)
    {
        return inv_run_sta_init;
    }
    if (sta == inv_fsm_sta_idle)
    {
        return inv_run_sta_idle;
    }
    return inv_run_sta_run;
}
