// SPDX-License-Identifier: MIT
/**
 * @file chb_fsm.c
 * @brief Gate CHB execution through initialization, bus precharge and fault recovery.
 * @details INIT locks bindings; subsequent states execute in the cooperative 1 ms task.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_fsm.h"
#include "chb_cfg.h"
#include "chb_hal.h"
#include "chb_protect.h"
#include "my_math.h"
#include "rly_on.h"
#include "section.h"

#include <math.h>
#include <stdatomic.h>

#define CHB_FSM_EVENT_NONE       0u           /* 未请求状态转换。 */
#define CHB_FSM_EVENT_IDLE       1u           /* 转入待机。 */
#define CHB_FSM_EVENT_SOFT_START 2u           /* 开始母线预充确认。 */
#define CHB_FSM_EVENT_MAIN_WAIT  3u           /* 预充完成，主继电器等待。 */
#define CHB_FSM_EVENT_RUN        4u           /* 主继电器等待完成。 */
#define CHB_FSM_EVENT_FAULT      5u           /* 母线预充超时。 */
#define CHB_FSM_SOFT_START_MS    500u         /* 上下计数的通过门限，1 ms/计数。 */
#define CHB_FSM_TIMEOUT_MS       5000u        /* 一次软起最长等待时间。 */
#define CHB_FSM_GRID_PEAK_FACTOR 1.272792206f /* 0.9 * sqrt(2)。 */
#define CHB_RELAY_TASK_HZ        10000.0f     /* rly_on 的 100 us 调度频率。 */

static uint32_t     fsm_event               = CHB_FSM_EVENT_NONE; /* Section FSM 事件存储。 */
static uint32_t     soft_start_elapsed      = 0u;   /* 本轮软起经过的 1 ms 次数。 */
static uint32_t     soft_start_qualified    = 0u;   /* 满足门限加一，否则减一。 */
static uint32_t     main_relay_wait_ms      = 0u;   /* 库确认闭合后的 1 ms 等待计数。 */
static rly_on_t     main_relay              = {0};  /* 主继电器的库状态机。 */
static uint8_t      main_relay_on_trig      = 0u;   /* FSM 发出的闭合脉冲。 */
static uint8_t      main_relay_off_trig     = 0u;   /* 库要求的断开触发源。 */
static uint8_t      main_relay_equal        = 0u;   /* 继电器两端电压相等标志。 */
static uint8_t      main_relay_task_enabled = 0u;   /* 等待及运行期间调用继电器库。 */
static float        main_relay_grid_hz      = 0.0f; /* 100 us 任务更新的实测电网频率。 */
static atomic_uchar run_allowed             = ATOMIC_VAR_INIT(0u); /* FSM 单发布者的许可。 */
static atomic_uchar clear_requested         = ATOMIC_VAR_INIT(0u); /* 外部的一次故障清除请求。 */

/** @brief 在 FSM INIT 配置库，并在重复启动时清掉未完成的闭合请求。 */
static void main_relay_prepare(void)
{
    main_relay_on_trig      = 0u;
    main_relay_off_trig     = 0u;
    main_relay_equal        = 0u;
    main_relay_task_enabled = 0u;
    main_relay_grid_hz      = chb_hal_get_sample()->grid_hz;
    rly_on_init(&main_relay,
                &main_relay_on_trig,
                &main_relay_off_trig,
                &main_relay_equal,
                &main_relay_grid_hz,
                CHB_RELAY_TASK_HZ,
                CHB_MAIN_RELAY_CLOSE_TIME_S,
                chb_hal_get_fsm()->p_main_relay_close,
                chb_hal_get_fsm()->p_main_relay_open);
    rly_on_func(&main_relay); /* 库的 INIT 状态转入 IDLE。 */
}

/** @brief 100 us 调用继电器库；停机后禁止旧等待状态继续闭合。 */
static void main_relay_task(void)
{
    if (main_relay_task_enabled != 0u)
    {
        const chb_hal_sample_t *p_sample = chb_hal_get_sample();
        float voltage_difference_v       = p_sample->grid_v - p_sample->input_cap_v;
        float match_window_v = CHB_MAIN_RELAY_MATCH_RATIO * M_SQRT2 * p_sample->grid_rms_v;

        main_relay_equal = (fabsf(voltage_difference_v) <= match_window_v) ? 1u : 0u;
        main_relay_grid_hz = p_sample->grid_hz;
        rly_on_func(&main_relay);
    }
}
REG_TASK(1u, main_relay_task)

/** @brief 进入 INIT 时开放绑定和参数写入，同时禁止发波。 */
static void init_in(void)
{
    atomic_store(&run_allowed, 0u);
    chb_cfg_unlock();
    chb_hal_unlock_binding();
}

/** @brief 只有平台、控制参数和保护配置均完整时才进入待机。 */
static void init_exe(void)
{
    if (    (chb_hal_is_ready() != 0u)
         && (chb_cfg_is_ready() != 0u)
         && (chb_protect_is_ready() != 0u))
    {
        chb_hal_sample();

        if (chb_protect_sample_healthy() != 0u)
        {
            main_relay_prepare();
            chb_cfg_lock();
            chb_hal_lock_binding();
            fsm_event = CHB_FSM_EVENT_IDLE;
        }
    }
}

/** @param event 本拍 FSM 事件。 @return 下一状态或 0 保持不变。 */
static uint32_t init_chk(uint32_t event)
{
    return (event == CHB_FSM_EVENT_IDLE) ? 2u : 0u;
}

/** @brief INIT 验证完成后先关闭桥臂。 */
static void init_out(void)
{
    chb_hal_get_ctrl()->p_pwm_disable();
    chb_hal_get_fsm()->p_soft_start_relay_open();
    chb_hal_get_fsm()->p_main_relay_open();
}

/** @brief 待机不持有运行许可。 */
static void idle_in(void)
{
    atomic_store(&run_allowed, 0u);
    main_relay_task_enabled = 0u;
    chb_hal_get_fsm()->p_soft_start_relay_open();
    chb_hal_get_fsm()->p_main_relay_open();
}

/** @brief 等待应用运行请求。 */
static void idle_exe(void)
{
    if (chb_cfg_get_run_request() != 0u)
    {
        fsm_event = CHB_FSM_EVENT_SOFT_START;
    }
}

/** @param event 本拍 FSM 事件。 @return 下一状态或 0 保持不变。 */
static uint32_t idle_chk(uint32_t event)
{
    return (event == CHB_FSM_EVENT_SOFT_START) ? 3u : 0u;
}

/** @brief 待机退出无额外器件动作。 */
static void idle_out(void)
{
    /* 运行准备由 RUN 进入动作统一完成。 */
}

/** @brief 开始预充确认；此阶段始终禁止发波。 */
static void soft_start_in(void)
{
    soft_start_elapsed   = 0u;
    soft_start_qualified = 0u;
    atomic_store(&run_allowed, 0u);
    main_relay_task_enabled = 0u;
    chb_hal_get_ctrl()->p_pwm_disable();
    chb_hal_get_fsm()->p_main_relay_open();
    chb_hal_get_fsm()->p_soft_start_relay_close();
}

/** @brief 三路母线之和高于 90% 电网峰值时上数，否则下数。 */
static void soft_start_exe(void)
{
    const chb_hal_sample_t *p_sample = chb_hal_get_sample();
    float total_bus_v                = 0.0f;

    if (chb_cfg_get_run_request() == 0u)
    {
        fsm_event = CHB_FSM_EVENT_IDLE;
        return;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        total_bus_v += p_sample->bus_v[cell];
    }
    ++soft_start_elapsed;

    if (total_bus_v > CHB_FSM_GRID_PEAK_FACTOR * p_sample->grid_rms_v)
    {
        if (soft_start_qualified < CHB_FSM_SOFT_START_MS)
        {
            ++soft_start_qualified;
        }
    }
    else if (soft_start_qualified > 0u)
    {
        --soft_start_qualified;
    }

    if (soft_start_qualified >= CHB_FSM_SOFT_START_MS)
    {
        fsm_event = CHB_FSM_EVENT_MAIN_WAIT;
    }
    else if (soft_start_elapsed >= CHB_FSM_TIMEOUT_MS)
    {
        fsm_event = CHB_FSM_EVENT_FAULT;
    }
}

static uint32_t soft_start_chk(uint32_t event)
{
    if (event == CHB_FSM_EVENT_FAULT)
    {
        return 6u;
    }

    if (event == CHB_FSM_EVENT_IDLE)
    {
        return 2u;
    }
    return (event == CHB_FSM_EVENT_MAIN_WAIT) ? 4u : 0u;
}

static void soft_start_out(void)
{
    if (fsm_event != CHB_FSM_EVENT_MAIN_WAIT)
    {
        chb_hal_get_fsm()->p_soft_start_relay_open();
    }
}

/** @brief 保持预充支路接通，由 rly_on 发出主继电器闭合命令。 */
static void main_wait_in(void)
{
    if (main_relay.inter.sta != RLY_ON_STA_IDLE)
    {
        main_relay_prepare(); /* 停机或故障后重试时清除库中的旧等待状态。 */
    }
    main_relay_equal        = 0u; /* 100 us 任务根据两端电压更新该标志。 */
    main_relay_on_trig      = 1u;
    main_relay_wait_ms      = 0u;
    main_relay_task_enabled = 1u;
    atomic_store(&run_allowed, 0u);
}

static void main_wait_exe(void)
{
    if (chb_cfg_get_run_request() == 0u)
    {
        fsm_event = CHB_FSM_EVENT_IDLE;
        return;
    }

    if (main_relay.output.is_closed != 0u)
    {
        if (main_relay_wait_ms == 0u)
        {
            chb_hal_get_fsm()->p_soft_start_relay_open(); /* 主支路已接通后再退出预充。 */
        }

        if (main_relay_wait_ms < CHB_MAIN_RELAY_WAIT_MS)
        {
            ++main_relay_wait_ms;
        }

        if (main_relay_wait_ms >= CHB_MAIN_RELAY_WAIT_MS)
        {
            fsm_event = CHB_FSM_EVENT_RUN;
        }
    }
    else
    {
        main_relay_wait_ms = 0u;
    }
}

static uint32_t main_wait_chk(uint32_t event)
{
    if (event == CHB_FSM_EVENT_IDLE)
    {
        return 2u;
    }
    return (event == CHB_FSM_EVENT_RUN) ? 5u : 0u;
}

static void main_wait_out(void)
{
    if (fsm_event != CHB_FSM_EVENT_RUN)
    {
        main_relay_task_enabled = 0u;
        main_relay_on_trig      = 0u;
        main_relay_equal        = 0u;

        if (main_relay.inter.sta == RLY_ON_STA_RUN)
        {
            main_relay_off_trig = 1u;
            rly_on_func(&main_relay); /* 库调用挂载的主继电器断开回调。 */
        }
        else
        {
            chb_hal_get_fsm()->p_main_relay_open(); /* 尚未闭合时确保输出为断开。 */
        }
    }
}

/** @brief 在尚未许可的串行调度边界准备本轮滤波和积分状态。 */
static void run_in(void)
{
    atomic_store(&run_allowed, 0u);
    chb_hal_get_fsm()->p_enter_run_func();
}

/** @brief 请求撤销立即禁止发波；否则授予本轮控制许可。 */
static void run_exe(void)
{
    if (chb_cfg_get_run_request() == 0u)
    {
        atomic_store(&run_allowed, 0u);
        fsm_event = CHB_FSM_EVENT_IDLE;
        return;
    }
    atomic_store(&run_allowed, 1u);
}

/** @param event 本拍 FSM 事件。 @return 下一状态或 0 保持不变。 */
static uint32_t run_chk(uint32_t event)
{
    return (event == CHB_FSM_EVENT_IDLE) ? 2u : 0u;
}

/** @brief 退出运行先停波，再维持禁止许可。 */
static void run_out(void)
{
    chb_hal_get_fsm()->p_exit_run_func();
    main_relay_task_enabled = 0u;
    main_relay_equal        = 0u;
    main_relay_off_trig     = 1u;
    rly_on_func(&main_relay); /* RUN 状态由库调用挂载的断开回调。 */
    atomic_store(&run_allowed, 0u);
}

/** @brief 母线软起超时闭锁，直到应用明确请求清除。 */
static void fault_in(void)
{
    atomic_store(&run_allowed, 0u);
    chb_hal_get_ctrl()->p_pwm_disable();
    main_relay_task_enabled = 0u;
    main_relay_on_trig      = 0u;
    main_relay_equal        = 0u;
    chb_hal_get_fsm()->p_soft_start_relay_open();
    chb_hal_get_fsm()->p_main_relay_open();
}

static void fault_exe(void)
{
    if (atomic_exchange(&clear_requested, 0u) != 0u)
    {
        fsm_event = CHB_FSM_EVENT_IDLE;
    }
}

static uint32_t fault_chk(uint32_t event)
{
    return (event == CHB_FSM_EVENT_IDLE) ? 2u : 0u;
}

static void fault_out(void)
{
    /* 清除闭锁后由 IDLE 等待新运行请求。 */
}

REG_FSM(CHB_FSM, 1u, fsm_event, FSM_ENTRY(1u, init_in, init_exe, init_chk, init_out),
        FSM_ENTRY(2u, idle_in, idle_exe, idle_chk, idle_out),
        FSM_ENTRY(3u, soft_start_in, soft_start_exe, soft_start_chk, soft_start_out),
        FSM_ENTRY(4u, main_wait_in, main_wait_exe, main_wait_chk, main_wait_out),
        FSM_ENTRY(5u, run_in, run_exe, run_chk, run_out),
        FSM_ENTRY(6u, fault_in, fault_exe, fault_chk, fault_out))

/** @brief 重复初始化时重建 FSM 初始状态。 */
static void chb_fsm_init(void)
{
    reg_fsm_CHB_FSM.fsm_sta           = 1u;
    reg_fsm_CHB_FSM.fsm_sta_is_change = 1u;
    fsm_event                         = CHB_FSM_EVENT_NONE;
    soft_start_elapsed                = 0u;
    soft_start_qualified              = 0u;
    main_relay_wait_ms                = 0u;
    main_relay_on_trig                = 0u;
    main_relay_off_trig               = 0u;
    main_relay_equal                  = 0u;
    main_relay_task_enabled           = 0u;
    atomic_store(&run_allowed, 0u);
    atomic_store(&clear_requested, 0u);
}
REG_INIT(1, chb_fsm_init)

CHB_RUN_STATE_E chb_fsm_get_run_state(void)
{
    uint32_t state = FSM_GET_STATE(CHB_FSM); /* Section 当前状态编号。 */

    switch (state)
    {
    case 1u:
        return CHB_RUN_STATE_INIT;
    case 3u:
        return CHB_RUN_STATE_BUS_SOFT_START;
    case 4u:
        return CHB_RUN_STATE_MAIN_RELAY_WAIT;
    case 5u:
        return CHB_RUN_STATE_RUN;
    case 6u:
        return CHB_RUN_STATE_FAULT;
    default:
        return CHB_RUN_STATE_IDLE;
    }
}

uint8_t chb_fsm_run_allowed(void)
{
    return atomic_load(&run_allowed);
}

uint8_t chb_fsm_clear_fault(void)
{
    if (    (chb_fsm_get_run_state() != CHB_RUN_STATE_FAULT)
         || (chb_cfg_get_run_request() != 0u))
    {
        return 0u;
    }
    atomic_store(&clear_requested, 1u);
    return 1u;
}
