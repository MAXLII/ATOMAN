// SPDX-License-Identifier: MIT
/**
 * @file    npc_fsm.h
 * @brief   Lifecycle states and bounded access to the FSM-owned control snapshot.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_FSM_H
#define NPC_FSM_H
#include "npc_cfg.h"
/* Lifecycle state is separate from protection/diagnostic status. */
typedef enum { npc_fsm_sta_init = 1, npc_fsm_sta_idle, npc_fsm_sta_run } npc_fsm_sta_e;
typedef enum { npc_fsm_ev_null = 0, npc_fsm_ev_to_idle, npc_fsm_ev_to_run } npc_fsm_ev_e;
npc_run_sta_e npc_fsm_get_run_sta(void);
/* One bounded read; returns 0 during publication, leaving the destination unchanged. */
uint8_t npc_fsm_read_published(npc_ctrl_setpoint_t *p_setpoint);
#endif
