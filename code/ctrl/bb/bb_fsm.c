/* bb_fsm.c
 * Minimal buck-boost run-state machine: init, idle, and run transitions.
 */

#include "bb_fsm.h"

static bb_fsm_ev_e fsm_ev = bb_fsm_ev_null; /* fsm_ev: pending transition event consumed by REG_FSM */
static bb_fsm_cmd_e fsm_cmd = bb_fsm_cmd_null; /* fsm_cmd: latched external command */
static bb_fsm_hal_t *p_hal = NULL; /* p_hal: bound FSM HAL hooks */

/**
 * @brief Latch an external command for the FSM.
 * @param cmd Command to store until consumed by the state machine.
 * @return None.
 */
void bb_fsm_set_cmd(bb_fsm_cmd_e cmd)
{
    fsm_cmd = cmd;
}

/**
 * @brief Bind the HAL callbacks consumed by the FSM.
 * @param p Pointer to the FSM HAL object. Passing NULL detaches the FSM.
 * @return None.
 */
void bb_fsm_set_p_hal(bb_fsm_hal_t *p)
{
    p_hal = p;
}

/**
 * @brief Fetch and clear the pending one-shot command.
 * @param None.
 * @return The last latched command, or bb_fsm_cmd_null if none is pending.
 */
static bb_fsm_cmd_e bb_fsm_get_cmd(void)
{
    bb_fsm_cmd_e temp = fsm_cmd; /* temp: one-shot command snapshot */
    fsm_cmd = bb_fsm_cmd_null;
    return temp;
}

/**
 * @brief Init-state entry action.
 * @param None.
 * @return None.
 */
static void bb_fsm_init_in(void)
{
    PLECS_LOG("bb_fsm enter init\n");
}

/**
 * @brief Init-state execution step.
 * @param None.
 * @return None. Transitions to idle only after HAL callbacks are valid.
 */
static void bb_fsm_init_exe(void)
{
    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        PLECS_LOG("bb_fsm init ready, goto idle\n");
        fsm_ev = bb_fsm_ev_to_idle;
    }
}

/**
 * @brief Init-state transition checker.
 * @param event Pending FSM event.
 * @return Next state id when a transition is accepted, otherwise 0.
 */
static uint32_t bb_fsm_init_chk(uint32_t event)
{
    if (event == bb_fsm_ev_to_idle)
    {
        return bb_fsm_sta_idle;
    }
    return 0U;
}

/**
 * @brief Init-state exit action.
 * @param None.
 * @return None.
 */
static void bb_fsm_init_out(void)
{
    PLECS_LOG("bb_fsm leave init\n");
}

/**
 * @brief Idle-state entry action.
 * @param None.
 * @return None.
 */
static void bb_fsm_idle_in(void)
{
    PLECS_LOG("bb_fsm enter idle\n");
}

/**
 * @brief Idle-state execution step.
 * @param None.
 * @return None. A start command schedules transition to run.
 */
static void bb_fsm_idle_exe(void)
{
    if (bb_fsm_get_cmd() == bb_fsm_cmd_start)
    {
        if (*p_hal->p_latched == 1)
        {
            PLECS_LOG("bb_fsm start rejected by hard protect latch\n");
            return;
        }

        PLECS_LOG("bb_fsm idle got start, goto run\n");
        fsm_ev = bb_fsm_ev_to_run;
    }
}

/**
 * @brief Idle-state transition checker.
 * @param event Pending FSM event.
 * @return Next state id when a transition is accepted, otherwise 0.
 */
static uint32_t bb_fsm_idle_chk(uint32_t event)
{
    if (event == bb_fsm_ev_to_run)
    {
        return bb_fsm_sta_run;
    }
    return 0U;
}

/**
 * @brief Idle-state exit action.
 * @param None.
 * @return None.
 */
static void bb_fsm_idle_out(void)
{
    PLECS_LOG("bb_fsm leave idle\n");
}

/**
 * @brief Run-state entry action.
 * @param None.
 * @return None. Calls the HAL enter-run hook when available.
 */
static void bb_fsm_run_in(void)
{
    PLECS_LOG("bb_fsm enter run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_enter_run_func != NULL))
    {
        p_hal->p_enter_run_func();
        PLECS_LOG("bb_fsm control prepared\n");
    }
}

/**
 * @brief Run-state execution step.
 * @param None.
 * @return None. A stop command schedules transition back to idle.
 */
static void bb_fsm_run_exe(void)
{
    if (bb_fsm_get_cmd() == bb_fsm_cmd_stop)
    {
        PLECS_LOG("bb_fsm run got stop, goto idle\n");
        fsm_ev = bb_fsm_ev_to_idle;
    }
}

/**
 * @brief Run-state transition checker.
 * @param event Pending FSM event.
 * @return Next state id when a transition is accepted, otherwise 0.
 */
static uint32_t bb_fsm_run_chk(uint32_t event)
{
    if (event == bb_fsm_ev_to_idle)
    {
        return bb_fsm_sta_idle;
    }
    return 0U;
}

/**
 * @brief Run-state exit action.
 * @param None.
 * @return None. Calls the HAL exit-run hook when available.
 */
static void bb_fsm_run_out(void)
{
    PLECS_LOG("bb_fsm leave run\n");

    if ((p_hal != NULL) &&
        (p_hal->p_exit_run_func != NULL))
    {
        p_hal->p_exit_run_func();
        PLECS_LOG("bb_fsm control stopped\n");
    }
}

REG_FSM(BB_FSM, bb_fsm_sta_init, fsm_ev,
        FSM_ENTRY(bb_fsm_sta_init, bb_fsm_init_in, bb_fsm_init_exe, bb_fsm_init_chk, bb_fsm_init_out),
        FSM_ENTRY(bb_fsm_sta_idle, bb_fsm_idle_in, bb_fsm_idle_exe, bb_fsm_idle_chk, bb_fsm_idle_out),
        FSM_ENTRY(bb_fsm_sta_run, bb_fsm_run_in, bb_fsm_run_exe, bb_fsm_run_chk, bb_fsm_run_out), )

/**
 * @brief Return the coarse public run state from the internal FSM state.
 * @param None.
 * @return One of bb_run_sta_init, bb_run_sta_idle, or bb_run_sta_run.
 */
bb_run_sta_e bb_fsm_get_run_sta(void)
{
    bb_fsm_sta_e sta = (bb_fsm_sta_e)FSM_GET_STATE(BB_FSM);

    if (sta == bb_fsm_sta_init)
    {
        return bb_run_sta_init;
    }
    if (sta == bb_fsm_sta_idle)
    {
        return bb_run_sta_idle;
    }
    return bb_run_sta_run;
}
