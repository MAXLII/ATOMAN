// SPDX-License-Identifier: MIT
/**
 * @file chb_hal.c
 * @brief Own CHB hardware bindings, coherent sample and immediate PWM inhibit.
 * @details Binding locks after FSM INIT; protection and control consume one sampled frame.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_hal.h"
#include "chb_ctrl.h"
#include "section.h"

#include <stddef.h>
#include <stdatomic.h>

static chb_ctrl_hal_t   binding        = {0}; /* 平台挂载的长寿命输入及 PWM 回调。 */
static chb_hal_sample_t sample         = {0}; /* 保护和控制共用的同拍副本。 */
static atomic_uchar     binding_locked = ATOMIC_VAR_INIT(0u); /* INIT 后禁止换源。 */
static atomic_uchar     tripped        = ATOMIC_VAR_INIT(0u); /* 应用保护闭锁。 */

/** @brief 在串行调度边界准备控制状态。 */
static void enter_run(void)
{
    chb_ctrl_prepare_run();
}

/** @brief 退出运行时关闭全部 PWM。 */
static void exit_run(void)
{
    chb_ctrl_stop();
}

static void soft_start_relay_close(void)
{
    binding.p_soft_start_relay_close();
}

static void soft_start_relay_open(void)
{
    binding.p_soft_start_relay_open();
}

static void main_relay_close(void)
{
    binding.p_main_relay_close();
}

static void main_relay_open(void)
{
    binding.p_main_relay_open();
}

static const chb_fsm_hal_t fsm_binding = { /* 生命周期动作无平台器件时序。 */
                                          .p_enter_run_func         = enter_run,
                                          .p_exit_run_func          = exit_run,
                                          .p_soft_start_relay_close = soft_start_relay_close,
                                          .p_soft_start_relay_open  = soft_start_relay_open,
                                          .p_main_relay_close       = main_relay_close,
                                          .p_main_relay_open        = main_relay_open};

uint8_t chb_hal_bind(const chb_ctrl_hal_t *p_binding)
{
    if (    (p_binding == NULL)
         || (atomic_load(&binding_locked) != 0u))
    {
        return 0u;
    }
    binding = *p_binding; /* 绑定对象复制一次；采样源地址仍由平台拥有。 */
    return 1u;
}

uint8_t chb_hal_is_ready(void)
{
    if (    (binding.p_grid_v == NULL)
         || (binding.p_input_cap_v == NULL)
         || (binding.p_grid_rms_v == NULL)
         || (binding.p_grid_hz == NULL)
         || (binding.p_theta_rad == NULL)
         || (binding.p_i_alpha_a == NULL)
         || (binding.p_i_beta_a == NULL)
         || (binding.p_set_pwm_func == NULL)
         || (binding.p_pwm_disable == NULL)
         || (binding.p_soft_start_relay_close == NULL)
         || (binding.p_soft_start_relay_open == NULL)
         || (binding.p_main_relay_close == NULL)
         || (binding.p_main_relay_open == NULL))
    {
        return 0u;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (    (binding.p_bus_v[cell] == NULL)
             || (binding.p_load_i_a[cell] == NULL))
        {
            return 0u;
        }
    }
    return 1u;
}

void chb_hal_unlock_binding(void)
{
    atomic_store(&binding_locked, 0u);
}

void chb_hal_lock_binding(void)
{
    atomic_store(&binding_locked, 1u);
}

const chb_ctrl_hal_t *chb_hal_get_ctrl(void)
{
    return &binding;
}

const chb_fsm_hal_t *chb_hal_get_fsm(void)
{
    return &fsm_binding;
}

void FUNC_RAM chb_hal_sample(void)
{
    sample.grid_v      = *binding.p_grid_v;
    sample.input_cap_v = *binding.p_input_cap_v;
    sample.grid_rms_v  = *binding.p_grid_rms_v;
    sample.grid_hz     = *binding.p_grid_hz;
    sample.theta_rad   = *binding.p_theta_rad;
    sample.i_alpha_a   = *binding.p_i_alpha_a;
    sample.i_beta_a    = *binding.p_i_beta_a;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        sample.bus_v[cell]    = *binding.p_bus_v[cell];
        sample.load_i_a[cell] = *binding.p_load_i_a[cell];
    }
}

const chb_hal_sample_t *chb_hal_get_sample(void)
{
    return &sample;
}

void chb_hal_trip(void)
{
    binding.p_pwm_disable(); /* INIT 已验证回调；先停波再更新闭锁。 */
    atomic_store(&tripped, 1u);
}

void chb_hal_clear_trip(void)
{
    atomic_store(&tripped, 0u);
}

uint8_t chb_hal_is_tripped(void)
{
    return atomic_load(&tripped);
}

/** @brief 初始化只清故障，不撤销平台在 section_init 前准备的绑定。 */
static void chb_hal_init(void)
{
    atomic_store(&tripped, 0u);
}
REG_INIT(0, chb_hal_init)
