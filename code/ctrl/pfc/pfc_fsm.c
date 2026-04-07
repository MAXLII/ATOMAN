#include "pfc_fsm.h"
#include "pfc_cfg.h"
#include "section.h"
#include "my_math.h"

static uint32_t init_dly = 0;
static uint32_t soft_start_dly = 0;
static uint32_t main_rly_dly = 0;
static pfc_fsm_ev_e fsm_ev = pfc_fsm_ev_null;
#define p_hal (pfc_hal_get_fsm())
static pfc_fsm_cmd_e fsm_cmd = pfc_fsm_cmd_null;

void pfc_fsm_set_cmd(pfc_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

void pfc_fsm_set_p_hal(pfc_fsm_hal_t *p)
{
    (void)p;
}

static pfc_fsm_cmd_e get_fsm_cmd(void)
{
    pfc_fsm_cmd_e temp = fsm_cmd;
    fsm_cmd = pfc_fsm_cmd_null;
    return temp;
}

static void pfc_fsm_init_in(void)
{
    init_dly = 0;
    PLECS_LOG("pfc_fsm enter init\n");
}

static void pfc_fsm_init_exe(void)
{
    if (init_dly < TIME_CNT_200MS_IN_1MS)
    {
        init_dly++;
    }
    else
    {
        if ((p_hal != NULL) &&
            (pfc_cfg_is_ready() == 1))
        {
            PLECS_LOG("pfc_fsm init ready, goto idle\n");
            fsm_ev = pfc_fsm_ev_to_idle;
        }
        else
        {
            PLECS_LOG("pfc_fsm init dep invalid\n");
        }
    }
}

static uint32_t pfc_fsm_init_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_idle)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}

static void pfc_fsm_init_out(void)
{
    PLECS_LOG("pfc_fsm leave init\n");
}

static void pfc_fsm_idle_in(void)
{
    pfc_hal_unlock_binding();
    PLECS_LOG("pfc_fsm enter idle\n");

    if (p_hal == NULL)
    {
        PLECS_LOG("pfc_fsm main relay off skipped: fsm hal is null\n");
    }
    else if (p_hal->p_main_rly_off_func == NULL)
    {
        PLECS_LOG("pfc_fsm main relay off skipped: off hook is null\n");
    }
    else
    {
        p_hal->p_main_rly_off_func();
        PLECS_LOG("pfc_fsm main relay off\n");
    }
}

static void pfc_fsm_idle_exe(void)
{
    if (get_fsm_cmd() == pfc_fsm_cmd_start)
    {
        if (pfc_hal_is_ready() == 0U)
        {
            PLECS_LOG("pfc_fsm start rejected by hal binding invalid\n");
            return;
        }

        if (*p_hal->p_latched == 1)
        {
            PLECS_LOG("pfc_fsm start rejected by hard protect latch\n");
            return;
        }

        PLECS_LOG("pfc_fsm idle got start, goto soft_start\n");
        fsm_ev = pfc_fsm_ev_to_soft_start;
    }
}

static uint32_t pfc_fsm_idle_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_soft_start)
    {
        return pfc_fsm_sta_soft_start;
    }
    return 0;
}

static void pfc_fsm_idle_out(void)
{
    pfc_hal_lock_binding();
    PLECS_LOG("pfc_fsm leave idle\n");
}

static void pfc_fsm_soft_start_in(void)
{
    soft_start_dly = 0;
    PLECS_LOG("pfc_fsm enter soft_start\n");
}

static void pfc_fsm_soft_start_exe(void)
{
    if (p_hal == NULL)
    {
        return;
    }

    if (soft_start_dly < TIME_CNT_5S_IN_1MS)
    {
        soft_start_dly++;
    }

    if (get_fsm_cmd() == pfc_fsm_cmd_stop)
    {
        PLECS_LOG("pfc_fsm soft_start got stop, goto idle\n");
        fsm_ev = pfc_fsm_ev_to_idle;
        return;
    }

    if ((*p_hal->p_vbus_sta == PFC_VBUS_STA_AT_INPUT_PEAK) ||
        (*p_hal->p_vbus_sta == PFC_VBUS_STA_IN_REGULATION))
    {
        PLECS_LOG("pfc_fsm soft_start bus ready, goto main_rly\n");
        fsm_ev = pfc_fsm_ev_to_main_rly;
        return;
    }

    if (soft_start_dly >= TIME_CNT_5S_IN_1MS)
    {
        PLECS_LOG("pfc_fsm soft_start timeout, goto idle\n");
        fsm_ev = pfc_fsm_ev_to_idle;
    }
}

static uint32_t pfc_fsm_soft_start_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_main_rly)
    {
        return pfc_fsm_sta_main_rly;
    }
    if (fsm_ev == pfc_fsm_ev_to_idle)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}

static void pfc_fsm_soft_start_out(void)
{
    PLECS_LOG("pfc_fsm leave soft_start\n");
}

static void pfc_fsm_main_rly_in(void)
{
    main_rly_dly = 0;
    PLECS_LOG("pfc_fsm enter main_rly\n");

    if (p_hal == NULL)
    {
        PLECS_LOG("pfc_fsm main relay on skipped: fsm hal is null\n");
    }
    else if (p_hal->p_main_rly_on_func == NULL)
    {
        PLECS_LOG("pfc_fsm main relay on skipped: on hook is null\n");
    }
    else
    {
        p_hal->p_main_rly_on_func();
        PLECS_LOG("pfc_fsm main relay on\n");
    }
}

static void pfc_fsm_main_rly_exe(void)
{
    if (p_hal == NULL)
    {
        return;
    }

    if (main_rly_dly < TIME_CNT_3S_IN_1MS)
    {
        main_rly_dly++;
    }

    if (get_fsm_cmd() == pfc_fsm_cmd_stop)
    {
        PLECS_LOG("pfc_fsm main_rly got stop, goto idle\n");
        fsm_ev = pfc_fsm_ev_to_idle;
        return;
    }

    if ((p_hal->p_main_rly_is_closed != NULL) &&
        (*p_hal->p_main_rly_is_closed == 1U))
    {
        PLECS_LOG("pfc_fsm main_rly confirmed closed, goto run\n");
        fsm_ev = pfc_fsm_ev_to_run;
        return;
    }

    if (main_rly_dly >= TIME_CNT_3S_IN_1MS)
    {
        PLECS_LOG("pfc_fsm main_rly timeout, goto idle\n");
        fsm_ev = pfc_fsm_ev_to_idle;
    }
}

static uint32_t pfc_fsm_main_rly_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_run)
    {
        return pfc_fsm_sta_run;
    }
    if (fsm_ev == pfc_fsm_ev_to_idle)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}

static void pfc_fsm_main_rly_out(void)
{
    PLECS_LOG("pfc_fsm leave main_rly\n");
}

static void pfc_fsm_run_in(void)
{
    PLECS_LOG("pfc_fsm enter run\n");

    if (p_hal == NULL)
    {
        PLECS_LOG("pfc_fsm run entry skipped: fsm hal is null\n");
        return;
    }

    if (p_hal->p_enter_run_func != NULL)
    {
        p_hal->p_enter_run_func();
        PLECS_LOG("pfc_fsm control enabled\n");
    }
    else
    {
        PLECS_LOG("pfc_fsm run entry skipped: enter_run hook is null\n");
    }

    if (p_hal->p_main_rly_on_func != NULL)
    {
        p_hal->p_main_rly_on_func();
        PLECS_LOG("pfc_fsm main relay on\n");
    }
    else
    {
        PLECS_LOG("pfc_fsm run entry skipped: main relay on hook is null\n");
    }
}

static void pfc_fsm_run_exe(void)
{
    if (p_hal == NULL)
    {
        return;
    }

    if (get_fsm_cmd() == pfc_fsm_cmd_stop)
    {
        PLECS_LOG("pfc_fsm run got stop, goto idle\n");
        fsm_ev = pfc_fsm_ev_to_idle;
        return;
    }
}

static uint32_t pfc_fsm_run_chk(uint32_t fsm_ev)
{
    if (fsm_ev == pfc_fsm_ev_to_idle)
    {
        return pfc_fsm_sta_idle;
    }
    return 0;
}

static void pfc_fsm_run_out(void)
{
    PLECS_LOG("pfc_fsm leave run\n");

    if (p_hal == NULL)
    {
        PLECS_LOG("pfc_fsm run exit skipped: fsm hal is null\n");
        return;
    }

    if (p_hal->p_exit_run_func != NULL)
    {
        p_hal->p_exit_run_func();
        PLECS_LOG("pfc_fsm control disabled\n");
    }
    else
    {
        PLECS_LOG("pfc_fsm run exit skipped: exit_run hook is null\n");
    }

    if (p_hal->p_main_rly_off_func != NULL)
    {
        p_hal->p_main_rly_off_func();
        PLECS_LOG("pfc_fsm main relay off\n");
    }
    else
    {
        PLECS_LOG("pfc_fsm run exit skipped: main relay off hook is null\n");
    }
}

REG_FSM(PFC_FSM, pfc_fsm_sta_init, fsm_ev,
        FSM_ENTRY(pfc_fsm_sta_init, pfc_fsm_init_in, pfc_fsm_init_exe, pfc_fsm_init_chk, pfc_fsm_init_out),
        FSM_ENTRY(pfc_fsm_sta_idle, pfc_fsm_idle_in, pfc_fsm_idle_exe, pfc_fsm_idle_chk, pfc_fsm_idle_out),
        FSM_ENTRY(pfc_fsm_sta_soft_start, pfc_fsm_soft_start_in, pfc_fsm_soft_start_exe, pfc_fsm_soft_start_chk, pfc_fsm_soft_start_out),
        FSM_ENTRY(pfc_fsm_sta_main_rly, pfc_fsm_main_rly_in, pfc_fsm_main_rly_exe, pfc_fsm_main_rly_chk, pfc_fsm_main_rly_out),
        FSM_ENTRY(pfc_fsm_sta_run, pfc_fsm_run_in, pfc_fsm_run_exe, pfc_fsm_run_chk, pfc_fsm_run_out), )

pfc_run_sta_e pfc_fsm_get_run_sta(void)
{
    pfc_fsm_sta_e sta = (pfc_fsm_sta_e)FSM_GET_STATE(PFC_FSM);

    if (sta == pfc_fsm_sta_init)
    {
        return pfc_run_sta_init;
    }
    if (sta == pfc_fsm_sta_idle)
    {
        return pfc_run_sta_idle;
    }
    return pfc_run_sta_run;
}
