// SPDX-License-Identifier: MIT
/**
 * @file    npc_cfg.h
 * @brief   Application parameters, timing and unpublished control setpoints.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_CFG_H
#define NPC_CFG_H
#include <stdint.h>
#include <stdatomic.h>
typedef struct npc_ctrl_cfg
{
    float ts;                  /* Fixed control period, s. */
    float omega;               /* Fixed DSOGI center frequency, rad/s; no internal PLL. */
    float sogi_k;              /* DSOGI damping coefficient. */
    float kp_v;                /* Voltage proportional gain, A/V, shared across four axes. */
    float ki_v;                /* Voltage integral gain, A/(V*s). */
    float kp_i;                /* Current proportional gain, V/A, shared across four axes. */
    float ki_i;                /* Current integral gain, V/(A*s). */
    float kaw_v;               /* Voltage-loop back-calculation rate, 1/s. */
    float kaw_i;               /* Current-loop back-calculation rate, 1/s. */
    float current_peak;        /* Sum-of-sequence current-reference magnitude limit, A. */
    float modulation_headroom; /* Usable fraction of total DC bus, (0,1]. */
} npc_ctrl_cfg_t;

#define NPC_CFG_NOMINAL_PHASE_PEAK_V (563.382640840131)
#define NPC_CFG_REFERENCE_RAMP_S (2.0)
#define NPC_CFG_DEFAULT_VD_POS_SLEW_VPS ((float)(NPC_CFG_NOMINAL_PHASE_PEAK_V / NPC_CFG_REFERENCE_RAMP_S))
#define NPC_CFG_TRIP_CURRENT_FACTOR (1.2f)

/* Application parameters are proposals; only FSM may grant run_allowed. */
typedef struct {
    float vd_pos_slew_vps; /* Soft-start/reference slew rate, V/s; positive finite value. */
    float freq_hz; /* DSOGI center frequency; angle is sampled through HAL. */
    float v_dc_half_min; /* Minimum energized half bus, V. */
    atomic_uchar run_request; /* Application start/stop level, also read by the ISR. */
} npc_cfg_t;
/* Coherent control parameters published by the FSM. */
typedef struct {
    float vd_pos_slew_vps; /* Published soft-start/reference slew rate, V/s. */
    float freq_hz; /* Published DSOGI frequency, Hz. */
    float v_dc_half_min; /* Published half-bus inhibit threshold, V. */
    uint8_t run_allowed; /* Granted only by FSM; never written by application. */
} npc_ctrl_setpoint_t;
typedef enum { npc_run_sta_init = 0, npc_run_sta_idle, npc_run_sta_run } npc_run_sta_e;
/* Fixed coefficients retain the MATLAB candidate control law. */
npc_ctrl_cfg_t npc_cfg_default(void);
/* Set timing before section_init; only while stopped. */
uint8_t npc_cfg_set_ctrl_ts(float ctrl_ts);
float npc_cfg_get_ctrl_ts(void);
uint8_t npc_cfg_is_ready(void);
/* Setters validate and update building parameters, not the ISR snapshot. */
/* Accept 0.001..1e6 V/s; applies to rising and falling amplitude commands. */
uint8_t npc_cfg_set_vd_pos_slew_vps(float value);
uint8_t npc_cfg_set_freq_hz(float value);
uint8_t npc_cfg_set_v_dc_half_min(float value);
uint8_t npc_cfg_set_run_request(uint8_t request);
uint8_t npc_cfg_get_run_request(void);
npc_run_sta_e npc_cfg_get_run_state(void);
const npc_ctrl_setpoint_t *npc_cfg_get_p_building(void);
#endif
