// SPDX-License-Identifier: MIT
/**
 * @file npc_fsm.c
 * @brief Publish run permissions and parameters; drive init/idle/run lifecycle through REG_FSM.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_fsm.h"
#include "npc_hal.h"
#include "section.h"

#include <stddef.h>
#include <stdatomic.h>

/** 1 ms 状态机是唯一发布者；各字段原子访问，供控制中断有界读取。 */
typedef struct npc_fsm_published
{
    atomic_uint sequence;           /* 偶数表示发布完成，奇数表示正在更新。 */
    atomic_uchar run_allowed;       /* 已发布的运行许可，0 禁止、1 允许。 */
    _Atomic(float) vd_pos_slew_vps; /* 已发布的软启动斜率，V/s。 */
    _Atomic(float) freq_hz;         /* 已发布的 DSOGI 中心频率，Hz。 */
    _Atomic(float) v_dc_half_min;   /* 已发布的单侧母线电压门限，V。 */
} npc_fsm_published_t;

/* 已发布参数池，使用序号检测读取期间发生的更新。 */
static npc_fsm_published_t published = {.sequence        = 0u,
                                        .run_allowed     = 0u,
                                        .vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
                                        .freq_hz         = 50.0f,
                                        .v_dc_half_min   = 20.0f};
static uint32_t fsm_ev = NPC_FSM_EV_NULL; /* REG_FSM 事件存储契约要求 uint32_t，不使用枚举存储。 */

/**
 * @brief 发布配置构建副本和本次状态机授予的运行许可。
 * @param run_allowed 0：禁止运行；1：允许运行。
 */
static void npc_fsm_publish_building(uint8_t run_allowed)
{
    const npc_ctrl_setpoint_t *p_building = npc_cfg_get_p_building();         /* 本次构建的应用参数。 */
    unsigned int sequence                 = atomic_load(&published.sequence); /* 与 atomic_uint 对应的发布序号。 */

    atomic_store(&published.sequence, sequence + 1u);  /* 奇数序号标记发布开始。 */
    atomic_store(&published.run_allowed, run_allowed); /* 发布当前状态的运行许可。 */
    atomic_store(&published.vd_pos_slew_vps, p_building->vd_pos_slew_vps); /* 发布幅值斜率。 */
    atomic_store(&published.freq_hz, p_building->freq_hz);             /* 发布观测器频率。 */
    atomic_store(&published.v_dc_half_min, p_building->v_dc_half_min); /* 发布母线门限。 */
    atomic_store(&published.sequence, sequence + 2u); /* 偶数序号标记发布完成。 */
}

uint8_t npc_fsm_read_published(npc_ctrl_setpoint_t *p_setpoint)
{
    npc_ctrl_setpoint_t snapshot = {0}; /* 确认完整性前不更新调用方对象。 */
    unsigned int before          = atomic_load(&published.sequence); /* 与 atomic_uint 对应的起始序号。 */

    if ((before & 1u) != 0u) /* 发布进行中时立即退出，避免中断等待。 */
    {
        return 0u;
    }

    snapshot.run_allowed     = atomic_load(&published.run_allowed);     /* 读取运行许可。 */
    snapshot.vd_pos_slew_vps = atomic_load(&published.vd_pos_slew_vps); /* 读取软启动斜率。 */
    snapshot.freq_hz         = atomic_load(&published.freq_hz);         /* 读取中心频率。 */
    snapshot.v_dc_half_min   = atomic_load(&published.v_dc_half_min);   /* 读取母线门限。 */

    if (before != atomic_load(&published.sequence))
    {
        return 0u;
    }

    *p_setpoint = snapshot; /* 只有序号未变化时才提交完整快照。 */
    return 1u;
}

/** @brief 进入初始化，允许绑定依赖并禁止发波。 */
static void npc_fsm_init_in(void)
{
    npc_hal_unlock_binding();     /* 初始化期间允许挂载采样与回调。 */
    npc_fsm_publish_building(0u); /* 依赖未检查完成前禁止运行。 */
}

/** @brief 等待 HAL 绑定与运行配置同时就绪。 */
static void npc_fsm_init_exe(void)
{
    if (    (npc_hal_is_ready() == 1u)
         && /* 所有采样指针及回调均已挂载。 */
            (npc_cfg_is_ready() == 1u)) /* 频率与控制周期处于有效范围。 */
    {
        npc_hal_lock_binding(); /* 检查通过即锁定，后续状态沿用本次检查结果。 */
        fsm_ev = NPC_FSM_EV_TO_IDLE;
    }
}

/**
 * @param event 当前状态机事件。
 * @return 待机状态编号，或 0 表示保持当前状态。
 */
static uint32_t npc_fsm_init_chk(uint32_t event)
{
    return (event == NPC_FSM_EV_TO_IDLE) ? (uint32_t)NPC_FSM_STA_IDLE : 0u;
}

/** @brief INIT 检查通过后关闭 PWM，随后进入待机。 */
static void npc_fsm_init_out(void)
{
    npc_hal_get_ctrl()->p_pwm_disable(); /* 所有回调已通过 INIT 检查，可直接调用。 */
    PLECS_LOG("npc_fsm dependencies ready\n");
}

/** @brief 进入待机，保持绑定锁定和禁止运行。 */
static void npc_fsm_idle_in(void)
{
    npc_fsm_publish_building(0u); /* 尚未通过开机条件检查。 */
}

/** @brief 发布待机配置，满足全部条件后请求进入运行。 */
static void npc_fsm_idle_exe(void)
{
    npc_fsm_publish_building(0u); /* 同步待机期间修改的运行参数。 */

    if (    (npc_cfg_get_run_request() == 1u)
         && /* 应用已请求开机。 */
            (npc_cfg_is_ready() == 1u)
         && /* 配置满足算法数值范围。 */
            (npc_hal_hard_protect_is_latched() == 0u)) /* 尚无保护闭锁。 */
    {
        fsm_ev = NPC_FSM_EV_TO_RUN;
    }
}

/**
 * @param event 当前状态机事件。
 * @return 运行状态编号，或 0 表示保持当前状态。
 */
static uint32_t npc_fsm_idle_chk(uint32_t event)
{
    return (event == NPC_FSM_EV_TO_RUN) ? (uint32_t)NPC_FSM_STA_RUN : 0u;
}

/** @brief 离开待机；绑定已在 INIT 检查通过时锁定。 */
static void npc_fsm_idle_out(void)
{
    /* 无额外退出动作，直接沿用已检查的硬件绑定。 */
}

/** @brief 准备运行动态，首轮运行检查前仍不授予许可。 */
static void npc_fsm_run_in(void)
{
    npc_fsm_publish_building(0u);          /* 准备阶段不允许控制中断输出。 */
    npc_hal_get_fsm()->p_enter_run_func(); /* HAL 已在进入运行前完成就绪检查。 */
}

/** @brief 检查停机条件，继续运行时发布完整参数与许可。 */
static void npc_fsm_run_exe(void)
{
    if (    (npc_cfg_get_run_request() == 0u)
         || /* 应用请求停机。 */
            (npc_hal_hard_protect_is_latched() == 1u)
         || /* 保护已经闭锁。 */
            (npc_cfg_is_ready() == 0u)) /* 运行配置不再有效。 */
    {
        npc_fsm_publish_building(0u); /* 先撤销许可，再请求状态切换。 */
        fsm_ev = NPC_FSM_EV_TO_IDLE;  /* 下一次事件检查进入待机。 */
        return;
    }

    npc_fsm_publish_building(1u); /* 运行条件成立，发布参数与许可。 */
}

/**
 * @param event 当前状态机事件。
 * @return 待机状态编号，或 0 表示保持当前状态。
 */
static uint32_t npc_fsm_run_chk(uint32_t event)
{
    return (event == NPC_FSM_EV_TO_IDLE) ? (uint32_t)NPC_FSM_STA_IDLE : 0u;
}

/** @brief 离开运行时先停止控制，再发布禁止运行的快照。 */
static void npc_fsm_run_out(void)
{
    npc_hal_get_fsm()->p_exit_run_func(); /* 关闭 PWM 并清除控制动态。 */
    npc_fsm_publish_building(0u);         /* 撤销后续中断的运行许可。 */
}

REG_FSM(NPC_FSM, NPC_FSM_STA_INIT, fsm_ev,
        FSM_ENTRY(NPC_FSM_STA_INIT, npc_fsm_init_in, npc_fsm_init_exe, npc_fsm_init_chk, npc_fsm_init_out),
        FSM_ENTRY(NPC_FSM_STA_IDLE, npc_fsm_idle_in, npc_fsm_idle_exe, npc_fsm_idle_chk, npc_fsm_idle_out),
        FSM_ENTRY(NPC_FSM_STA_RUN, npc_fsm_run_in, npc_fsm_run_exe, npc_fsm_run_chk, npc_fsm_run_out))

/** @brief 支持同一 DLL 内重复初始化，恢复状态机入口与未许可快照。 */
static void npc_fsm_init(void)
{
    reg_fsm_NPC_FSM.fsm_sta           = NPC_FSM_STA_INIT; /* 重新进入初始化状态。 */
    reg_fsm_NPC_FSM.fsm_sta_is_change = 1u; /* 首次调度执行状态进入动作。 */
    fsm_ev                            = NPC_FSM_EV_NULL; /* 清除上次运行的切换事件。 */
    npc_fsm_publish_building(0u); /* 重建禁止运行的参数快照。 */
}
REG_INIT(1, npc_fsm_init)

NPC_RUN_STA_E npc_fsm_get_run_sta(void)
{
    uint32_t state = FSM_GET_STATE(NPC_FSM); /* SECTION 状态编号按 uint32_t 存储。 */

    if (state == NPC_FSM_STA_INIT)
    {
        return NPC_RUN_STA_INIT;
    }

    return (state == NPC_FSM_STA_RUN) ? NPC_RUN_STA_RUN : NPC_RUN_STA_IDLE;
}
