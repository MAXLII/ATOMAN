// SPDX-License-Identifier: MIT
/**
 * @file    npc_cfg.c
 * @brief   Validate application requests and maintain the FSM building parameters.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "section.h"
#include <stdatomic.h>
static npc_cfg_t config = {NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f, 0U};
static npc_ctrl_setpoint_t building = {NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f, 0U};
static float sample_period = 0.0002f;

npc_ctrl_cfg_t npc_cfg_default(void)
{
    return (npc_ctrl_cfg_t){
        .ts = 0.0002f,
        .omega = 314.1592653589793f,
        .sogi_k = 1.4142135623730951f,
        /* Joint tuning with DSOGI, sample delay and dynamic modulation loss. */
        .kp_v = 1.0f,
        .ki_v = 1300.0f,
        .kp_i = 0.15f,
        .ki_i = 0.05f,
        .kaw_v = 10.0f,
        .kaw_i = 10.0f,
        .current_peak = 5200.0f, /* Y-delta analysis capacity; not a hardware current rating. */
        .modulation_headroom = 0.95f};
}

static void npc_cfg_init(void)
{
    config.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS;
    config.freq_hz = 50.0f;
    config.v_dc_half_min = 20.0f;
    building = (npc_ctrl_setpoint_t){NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f, 0U};
    atomic_store(&config.run_request, 0U);
}
REG_INIT(0, npc_cfg_init)

uint8_t npc_cfg_set_ctrl_ts(float ctrl_ts)
{
    if (!((ctrl_ts >= 1.0e-6f) && (ctrl_ts <= 0.01f))) return 0U;
    sample_period = ctrl_ts;
    return 1U;
}
float npc_cfg_get_ctrl_ts(void) { return sample_period; }
uint8_t npc_cfg_is_ready(void)
{
    float step = 6.283185307179586f * config.freq_hz * sample_period;
    return ((step >= 0.001f) && (step <= 1.0f)) ? 1U : 0U;
}
uint8_t npc_cfg_set_vd_pos_slew_vps(float value)
{
    if (!((value >= 0.001f) && (value <= 1.0e6f))) return 0U;
    config.vd_pos_slew_vps = value;
    return 1U;
}
uint8_t npc_cfg_set_freq_hz(float value)
{
    double step = 6.28318530717958647692 * (double)value * (double)sample_period;
    if (!((value >= 1.0f) && (step >= 0.001) && (step <= 1.0))) return 0U;
    config.freq_hz = value;
    return 1U;
}
uint8_t npc_cfg_set_v_dc_half_min(float value)
{
    if (!((value >= 0.001f) && (value <= 1.0e6f))) return 0U;
    config.v_dc_half_min = value;
    return 1U;
}
uint8_t npc_cfg_set_run_request(uint8_t request)
{
    if (request > 1U) return 0U;
    atomic_store(&config.run_request, request);
    return 1U;
}
uint8_t npc_cfg_get_run_request(void) { return atomic_load(&config.run_request); }
npc_run_sta_e npc_cfg_get_run_state(void) { return npc_fsm_get_run_sta(); }
const npc_ctrl_setpoint_t *npc_cfg_get_p_building(void)
{
    /* FSM task builds a proposal from application values; permission stays zero. */
    building.vd_pos_slew_vps = config.vd_pos_slew_vps;
    building.freq_hz = config.freq_hz;
    building.v_dc_half_min = config.v_dc_half_min;
    building.run_allowed = 0U;
    return &building;
}
