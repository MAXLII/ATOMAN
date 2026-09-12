// SPDX-License-Identifier: MIT
/**
 * @file    npc_ctrl.h
 * @brief   Control lifecycle and read-only monitoring; periodic computation is registered internally.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_CTRL_H
#define NPC_CTRL_H
#include <stdbool.h>
#include <stdint.h>
typedef struct npc_ctrl_output
{
    float v_alpha;        /* Modulation voltage command, V; connects to SVPWM input. */
    float v_beta;         /* Modulation voltage command, V. */
    float v_dq[4];        /* Voltage feedback [d+,q+,d-,q-], V. */
    float i_dq[4];        /* Current feedback [d+,q+,d-,q-], A. */
    float i_ref[4];       /* Limited current references [d+,q+,d-,q-], A. */
    float u_dq[4];        /* Limited sequence voltage commands [d+,q+,d-,q-], V. */
    bool current_limited; /* Current-reference limit engaged this sample. */
    bool voltage_limited; /* Modulation span limit engaged this sample. */
    bool valid;           /* False after reset or rejected calculation; caller must inhibit PWM. */
} npc_ctrl_output_t;


/* Stable diagnostic values shared with the platform's existing Shell status codes. */
typedef enum {
    NPC_CTRL_OFF = 1,
    NPC_CTRL_RUNNING = 2,
    NPC_CTRL_REFERENCE = 3,
    NPC_CTRL_INPUT = 5,
    NPC_CTRL_BUS = 6,
    NPC_CTRL_CONTROL = 7,
    NPC_CTRL_PWM = 8,
    NPC_CTRL_BINDING = 10,
    NPC_CTRL_OVERCURRENT = 12
} npc_ctrl_status_e;
/* Read-only diagnostic snapshot; contains no observer bindings or writable control state. */
typedef struct {
    npc_ctrl_output_t output;
    float integral_v[4]; /* A, copied for trace inspection. */
    float integral_i[4]; /* V, copied for trace inspection. */
    float vd_pos_ref_act; /* Soft-start value used in this sample, V. */
    uint32_t status; /* 1 off, 2 running, 5 input, 6 bus, 7 control, 8 PWM, 10 binding, 12 overcurrent. */
    uint32_t detail; /* Failed input, phase index or PWM callback status. */
} npc_ctrl_monitor_t;
/* Called by HAL run entry/exit; caller must serialize with section_interrupt. */
void npc_ctrl_prepare_run(void);
void npc_ctrl_stop(void);
/* Read after section_interrupt under the same dispatcher lock. */
const npc_ctrl_monitor_t *npc_ctrl_get_monitor(void);
#endif
