// SPDX-License-Identifier: MIT
/**
 * @file chb_protect.c
 * @brief Check CHB bus and current samples before the control/PWM stage.
 * @details Latch invalid feedback or out-of-range electrical quantities until explicit recovery.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_protect.h"
#include "chb_cfg.h"
#include "chb_ctrl.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "section.h"

#include <math.h>
#include <stdatomic.h>

static float        trip_current_a = 0.0f;                /* 应用输入的电流保护门限，A。 */
static float        minimum_bus_v  = 0.0f;                /* RUN 阶段的母线欠压门限，V。 */
static float        maximum_bus_v  = 0.0f;                /* 应用输入的母线过压门限，V。 */
static uint8_t      configured     = 0u;                  /* 门限齐全后才允许状态机离开 INIT。 */
static atomic_uchar tripped        = ATOMIC_VAR_INIT(0u); /* 应用保护故障闭锁。 */

/** @return 1：所有采样在配置范围内。 */
uint8_t chb_protect_sample_healthy(void)
{
    const chb_observer_sample_t *p_sample = chb_observer_get_sample(); /* INIT/保护共用的观测快照。 */

    if (    (!isfinite(p_sample->grid_v))
         || (!isfinite(p_sample->grid_rms_v))
         || (p_sample->grid_rms_v <= 0.0f)
         || (!isfinite(p_sample->grid_hz))
         || (p_sample->grid_hz <= 0.0f)
         || (!isfinite(p_sample->theta_rad))
         || (!isfinite(p_sample->i_alpha_a))
         || (!isfinite(p_sample->i_beta_a))
         || (fabsf(p_sample->i_alpha_a) > trip_current_a))
    {
        return 0u;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (    (!isfinite(p_sample->bus_v[cell]))
             || (!isfinite(p_sample->load_i_a[cell]))
             || (p_sample->bus_v[cell] < 0.0f)
             || (p_sample->bus_v[cell] > maximum_bus_v))
        {
            return 0u;
        }

        if (    (chb_fsm_get_run_state() == CHB_RUN_STATE_RUN)
             && (p_sample->bus_v[cell] < minimum_bus_v))
        {
            return 0u;
        }
    }
    return 1u;
}

uint8_t chb_protect_configure(float current_trip_a, float bus_min_v, float bus_max_v)
{
    if (    (chb_fsm_get_run_state() != CHB_RUN_STATE_INIT)
         || (!isfinite(current_trip_a))
         || (!isfinite(bus_min_v))
         || (!isfinite(bus_max_v))
         || (current_trip_a <= 0.0f)
         || (bus_min_v <= 0.0f)
         || (bus_max_v <= bus_min_v))
    {
        return 0u;
    }
    trip_current_a = current_trip_a;
    minimum_bus_v  = bus_min_v;
    maximum_bus_v  = bus_max_v;
    configured     = 1u;
    return 1u;
}

uint8_t chb_protect_is_ready(void)
{
    return configured;
}

uint8_t chb_protect_clear_latch(void)
{
    CHB_RUN_STATE_E run_state = chb_fsm_get_run_state(); /* 仅在桥臂已停的状态清除应用闭锁。 */

    if (    (    (run_state != CHB_RUN_STATE_IDLE)
              && (run_state != CHB_RUN_STATE_FAULT))
         || (chb_cfg_get_run_request() != 0u)
         || (chb_protect_sample_healthy() == 0u))
    {
        return 0u;
    }
    atomic_store(&tripped, 0u);
    return 1u;
}

uint8_t chb_protect_is_tripped(void)
{
    return atomic_load(&tripped);
}

/** @brief 中断第二阶段检查采样；本拍故障会阻止控制阶段重新发波。 */
static void FUNC_RAM chb_protect_run(void)
{
    if (chb_fsm_get_run_state() == CHB_RUN_STATE_INIT)
    {
        return;
    }

    if (chb_protect_sample_healthy() == 0u)
    {
        chb_hal_get_ctrl()->p_pwm_disable(); /* 先停波，再发布应用闭锁。 */
        atomic_store(&tripped, 1u);
    }

    if (chb_protect_is_tripped() != 0u)
    {
        (void)chb_cfg_set_run_request(0u);
        chb_ctrl_inhibit();
    }
}
REG_INTERRUPT(2, chb_protect_run)

static void chb_protect_init(void)
{
    atomic_store(&tripped, 0u);
}
REG_INIT(0, chb_protect_init)
