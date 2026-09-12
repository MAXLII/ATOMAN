// SPDX-License-Identifier: MIT
/**
 * @file    npc_fsm.c
 * @brief   Publish run permissions and parameters; drive init/idle/run lifecycle through REG_FSM.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_fsm.h"
#include "npc_hal.h"
#include "section.h"
#include <stddef.h>
#include <stdatomic.h>
/* Single writer: the 1 ms FSM task. All fields are atomic for ISR readers. */
typedef struct {
    atomic_uint sequence;
    atomic_uchar run_allowed;
    _Atomic(float) vd_pos_slew_vps;
    _Atomic(float) freq_hz;
    _Atomic(float) v_dc_half_min;
} npc_fsm_published_t;
static npc_fsm_published_t published = {0U, 0U, NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, 50.0f, 20.0f};
static uint32_t fsm_ev = npc_fsm_ev_null;

static void npc_fsm_publish_building(uint8_t run_allowed)
{
    const npc_ctrl_setpoint_t *p_building = npc_cfg_get_p_building();
    unsigned int sequence = atomic_load(&published.sequence);
    atomic_store(&published.sequence, sequence + 1U);
    atomic_store(&published.run_allowed, run_allowed);
    atomic_store(&published.vd_pos_slew_vps, p_building->vd_pos_slew_vps);
    atomic_store(&published.freq_hz, p_building->freq_hz);
    atomic_store(&published.v_dc_half_min, p_building->v_dc_half_min);
    atomic_store(&published.sequence, sequence + 2U);
}
uint8_t npc_fsm_read_published(npc_ctrl_setpoint_t *p_setpoint)
{
    npc_ctrl_setpoint_t snapshot = {0};
    unsigned int before = atomic_load(&published.sequence);
    if ((p_setpoint == NULL) || ((before & 1U) != 0U)) return 0U;
    snapshot.run_allowed = atomic_load(&published.run_allowed);
    snapshot.vd_pos_slew_vps = atomic_load(&published.vd_pos_slew_vps);
    snapshot.freq_hz = atomic_load(&published.freq_hz);
    snapshot.v_dc_half_min = atomic_load(&published.v_dc_half_min);
    if (before != atomic_load(&published.sequence)) return 0U;
    *p_setpoint = snapshot;
    return 1U;
}
static void npc_fsm_init_in(void)
{
    npc_hal_unlock_binding();
    npc_fsm_publish_building(0U);
}
static void npc_fsm_init_exe(void)
{
    if ((npc_hal_is_ready() != 0U) && (npc_cfg_is_ready() != 0U))
        fsm_ev = npc_fsm_ev_to_idle;
}
static uint32_t npc_fsm_init_chk(uint32_t event)
{ return (event == npc_fsm_ev_to_idle) ? (uint32_t)npc_fsm_sta_idle : 0U; }
static void npc_fsm_init_out(void) { PLECS_LOG("npc_fsm dependencies ready\n"); }
static void npc_fsm_idle_in(void)
{
    npc_hal_unlock_binding();
    npc_fsm_publish_building(0U);
}
static void npc_fsm_idle_exe(void)
{
    npc_fsm_publish_building(0U);
    if ((npc_cfg_get_run_request() != 0U) && (npc_cfg_is_ready() != 0U) &&
        (npc_hal_is_ready() != 0U) && (npc_hal_hard_protect_is_latched() == 0U))
        fsm_ev = npc_fsm_ev_to_run;
}
static uint32_t npc_fsm_idle_chk(uint32_t event)
{ return (event == npc_fsm_ev_to_run) ? (uint32_t)npc_fsm_sta_run : 0U; }
static void npc_fsm_idle_out(void) { npc_hal_lock_binding(); }
static void npc_fsm_run_in(void)
{
    npc_fsm_publish_building(0U);
    npc_hal_get_fsm()->p_enter_run_func();
}
static void npc_fsm_run_exe(void)
{
    if ((npc_cfg_get_run_request() == 0U) || (npc_hal_hard_protect_is_latched() != 0U) ||
        (npc_cfg_is_ready() == 0U))
    {
        npc_fsm_publish_building(0U);
        fsm_ev = npc_fsm_ev_to_idle;
        return;
    }
    npc_fsm_publish_building(1U);
}
static uint32_t npc_fsm_run_chk(uint32_t event)
{ return (event == npc_fsm_ev_to_idle) ? (uint32_t)npc_fsm_sta_idle : 0U; }
static void npc_fsm_run_out(void)
{
    npc_hal_get_fsm()->p_exit_run_func();
    npc_fsm_publish_building(0U);
}
REG_FSM(NPC_FSM, npc_fsm_sta_init, fsm_ev,
    FSM_ENTRY(npc_fsm_sta_init, npc_fsm_init_in, npc_fsm_init_exe, npc_fsm_init_chk, npc_fsm_init_out),
    FSM_ENTRY(npc_fsm_sta_idle, npc_fsm_idle_in, npc_fsm_idle_exe, npc_fsm_idle_chk, npc_fsm_idle_out),
    FSM_ENTRY(npc_fsm_sta_run, npc_fsm_run_in, npc_fsm_run_exe, npc_fsm_run_chk, npc_fsm_run_out))
/* section_init may run repeatedly in the same loaded simulation DLL. */
static void npc_fsm_init(void)
{
    reg_fsm_NPC_FSM.fsm_sta = npc_fsm_sta_init;
    reg_fsm_NPC_FSM.fsm_sta_is_change = 1U;
    fsm_ev = npc_fsm_ev_null;
    npc_fsm_publish_building(0U);
}
REG_INIT(1, npc_fsm_init)
npc_run_sta_e npc_fsm_get_run_sta(void)
{
    uint32_t state = FSM_GET_STATE(NPC_FSM);
    if (state == npc_fsm_sta_init) return npc_run_sta_init;
    return (state == npc_fsm_sta_run) ? npc_run_sta_run : npc_run_sta_idle;
}
