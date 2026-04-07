/* bb_fsm.h
 * Buck-boost FSM state, event, command, and HAL interface declarations.
 */

#ifndef __BB_FSM_H
#define __BB_FSM_H

#include <stdint.h>
#include "bb_hal.h"
#include "section.h"

typedef enum
{
    bb_fsm_sta_null = 0,
    bb_fsm_sta_init = 1,
    bb_fsm_sta_idle,
    bb_fsm_sta_run,
    bb_fsm_sta_max,
} bb_fsm_sta_e;

typedef enum
{
    bb_fsm_ev_null = 0,
    bb_fsm_ev_to_idle = 1,
    bb_fsm_ev_to_run,
} bb_fsm_ev_e;

typedef enum
{
    bb_fsm_cmd_null = 0,
    bb_fsm_cmd_start,
    bb_fsm_cmd_stop,
} bb_fsm_cmd_e;

typedef enum
{
    bb_run_sta_init = 0,
    bb_run_sta_idle,
    bb_run_sta_run,
} bb_run_sta_e;

/**
 * @brief Post an external command to the buck-boost FSM.
 * @param cmd Command to be consumed on the next FSM execution step.
 * @return None.
 */
void bb_fsm_set_cmd(bb_fsm_cmd_e cmd);

/**
 * @brief Get the coarse run state exposed to the rest of the system.
 * @param None.
 * @return One of init, idle, or run.
 */
bb_run_sta_e bb_fsm_get_run_sta(void);

/**
 * @brief Bind the FSM HAL callbacks.
 * @param p Pointer to the FSM HAL object. Passing NULL detaches the callbacks.
 * @return None.
 */
void bb_fsm_set_p_hal(bb_fsm_hal_t *p);

#endif
