#ifndef __INV_FSM_H
#define __INV_FSM_H

#include <stdint.h>
#include "inv_hal.h"
#include "section.h"

typedef enum
{
    inv_fsm_sta_null = 0,
    inv_fsm_sta_init = 1,
    inv_fsm_sta_idle,
    inv_fsm_sta_rly_on,
    inv_fsm_sta_run,
    inv_fsm_sta_max,
} inv_fsm_sta_e;

typedef enum
{
    inv_fsm_ev_null = 0,
    inv_fsm_ev_to_idle = 1,
    inv_fsm_ev_to_rly_on,
    inv_fsm_ev_to_run,
} inv_fsm_ev_e;

typedef enum
{
    inv_fsm_cmd_null = 0,
    inv_fsm_cmd_start,
    inv_fsm_cmd_stop,
} inv_fsm_cmd_e;

typedef enum
{
    inv_run_sta_init = 0,
    inv_run_sta_idle,
    inv_run_sta_run,
} inv_run_sta_e;

void inv_fsm_set_cmd(inv_fsm_cmd_e cmd);
inv_run_sta_e inv_fsm_get_run_sta(void);
void inv_fsm_set_p_hal(inv_fsm_hal_t *p);

#endif
