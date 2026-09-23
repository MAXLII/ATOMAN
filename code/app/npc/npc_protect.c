// SPDX-License-Identifier: MIT
/**
 * @file npc_protect.c
 * @brief Apply NPC application protection between sampling and control interrupts.
 * @details Read the priority-1 sample, latch overcurrent, inhibit undervoltage,
 *          and revoke priority-3 control permission on a fault. Single precision.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_protect.h"
#include "npc_ctrl.h"
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "npc_hal.h"
#include "section.h"
#include <math.h>

static float current_trip = 0.0f;     /* 应用采样过流门限，A，初始化时从控制预算计算。 */
static npc_ctrl_setpoint_t setpoint = /* 发布进行中时沿用上次完整的应用运行参数。 */
    {.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS, .freq_hz = 50.0f, .v_dc_half_min = 20.0f, .run_allowed = 0u};

/** @brief 配置应用保护门限，不调用采样或 PWM 指针。 */
static void npc_protect_init(void)
{
    npc_ctrl_cfg_t cfg = npc_cfg_default(); /* 与控制器使用同一电流预算。 */

    current_trip         = NPC_PROTECT_CURRENT_FACTOR * cfg.current_peak;
    setpoint.run_allowed = 0u; /* 重复初始化时不得沿用上次运行许可。 */
}
REG_INIT(2, npc_protect_init)

/** @brief 中断第二层：检查本拍过流和欠压，故障时禁止后续控制及 PWM。 */
static void FUNC_RAM npc_protect_run(void)
{
    const npc_hal_sample_t *p_sample = npc_hal_get_sample(); /* 采样阶段已形成的本拍快照。 */
    uint32_t fault                   = 0u; /* 本拍首个过流故障码，0 表示无新故障。 */
    uint32_t phase_fault             = 0u; /* 首个过流故障的相序号，0=A、1=B、2=C。 */

    if (npc_fsm_get_run_sta() == NPC_RUN_STA_INIT)
    {
        return; /* INIT 未完成时不处理尚未采集的快照。 */
    }

    (void)npc_fsm_read_published(&setpoint); /* 使用状态机完整发布的运行参数。 */

    for (uint32_t phase = 0u; phase < 3u; ++phase) /* 按 A、B、C 相保留首次故障。 */
    {
        if (    (fabsf(p_sample->i_l[phase]) > current_trip)
             && /* 当前相超过应用过流门限。 */
                (fault == 0u)) /* 仅记录本拍首个过流相。 */
        {
            fault       = NPC_PROTECT_OVERCURRENT;
            phase_fault = phase;
        }
    }

    if (    (fault != 0u)
         && /* 本拍出现过流。 */
            (npc_cfg_get_run_request() == 1u)) /* 仅在应用请求运行时触发闭锁。 */
    {
        npc_hal_hard_protect_trip(fault, phase_fault);
    }

    if (    (fault == 0u)
         && /* 本拍没有检测到过流。 */
            (npc_cfg_get_run_request() == 0u)) /* 明确停机后才确认恢复。 */
    {
        npc_hal_hard_protect_clear();
    }

    if (npc_hal_hard_protect_is_latched() == 1u)
    {
        npc_ctrl_stop();
        return;
    }

    if (    (setpoint.run_allowed == 0u)
         || /* 状态机尚未允许运行。 */
            (npc_cfg_get_run_request() == 0u)) /* 停机由控制层正常处理，不报告母线故障。 */
    {
        return;
    }

    if (    (p_sample->v_dc_p < setpoint.v_dc_half_min)
         || /* 正侧母线低于运行门限。 */
            (p_sample->v_dc_n < setpoint.v_dc_half_min)) /* 负侧母线低于运行门限。 */
    {
        npc_ctrl_stop();
    }
}
REG_INTERRUPT(2, npc_protect_run)
