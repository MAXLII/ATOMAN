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
#include <stddef.h>

/** @brief 保护只读输入，连接 ctrl 持有的同拍静态变量。 */
static struct
{
    const float *p_v_grid_raw;
    const float *p_v_grid_rms;
    const float *p_f_grid;
    const float *p_theta_grid;
    const float *p_i_grid_alpha_raw;
    const float *p_i_grid_beta_observed;
    const float *p_v_bus_raw;
    const float *p_i_load_raw;
} input;

/**
 * @brief INIT 时建立应用保护的只读输入连线。
 * @details 数值由 ctrl 采样阶段更新；保护不改写源，也不再次读取 HAL。
 * @param p_v_grid 电网电压，V。
 * @param p_v_grid_rms 电网有效值，V。
 * @param p_f_grid 观测频率，Hz。
 * @param p_theta_grid 观测相角，rad。
 * @param p_i_grid_alpha 物理电流，A。
 * @param p_i_grid_beta 观测正交电流，A。
 * @param p_v_bus 各桥母线电压，V，数组长度 CHB_CELL_COUNT。
 * @param p_i_load 各桥负载电流，A，数组长度 CHB_CELL_COUNT。
 * @return 1：INIT 连线成功；0：运行阶段或输入地址无效。
 */
uint8_t chb_protect_set_input(const float *p_v_grid, const float *p_v_grid_rms,
                              const float *p_f_grid, const float *p_theta_grid,
                              const float *p_i_grid_alpha, const float *p_i_grid_beta,
                              const float *p_v_bus, const float *p_i_load)
{
    if (    (chb_fsm_get_run_state() != CHB_RUN_STATE_INIT)
         || (p_v_grid == NULL)
         || (p_v_grid_rms == NULL)
         || (p_f_grid == NULL)
         || (p_theta_grid == NULL)
         || (p_i_grid_alpha == NULL)
         || (p_i_grid_beta == NULL)
         || (p_v_bus == NULL)
         || (p_i_load == NULL))
    {
        return 0u;
    }

    input.p_v_grid_raw             = p_v_grid;
    input.p_v_grid_rms             = p_v_grid_rms;
    input.p_f_grid                 = p_f_grid;
    input.p_theta_grid             = p_theta_grid;
    input.p_i_grid_alpha_raw       = p_i_grid_alpha;
    input.p_i_grid_beta_observed   = p_i_grid_beta;
    input.p_v_bus_raw              = p_v_bus;
    input.p_i_load_raw             = p_i_load;
    return 1u;
}

static float        i_grid_trip_limit = 0.0f;             /* 应用输入的电流保护门限，A。 */
static float        i_precharge_trip_limit = 0.0f;        /* 被动预充浪涌电流门限，A。 */
static float        v_bus_min_limit = 0.0f;               /* RUN 阶段的母线欠压门限，V。 */
static float        v_bus_max_limit = 0.0f;               /* 应用输入的母线过压门限，V。 */
static uint8_t      configured     = 0u;                  /* 门限齐全后才允许状态机离开 INIT。 */
static atomic_uchar tripped        = ATOMIC_VAR_INIT(0u); /* 应用保护故障闭锁。 */

/** @return 1：所有采样在配置范围内。 */
uint8_t chb_protect_sample_healthy(void)
{
    const float i_grid_limit = (chb_fsm_get_run_state() == CHB_RUN_STATE_BUS_SOFT_START)
                                      ? i_precharge_trip_limit : i_grid_trip_limit;

    if (    (!isfinite(*input.p_v_grid_raw))
         || (!isfinite(*input.p_v_grid_rms))
         || (*input.p_v_grid_rms <= 0.0f)
         || (!isfinite(*input.p_f_grid))
         || (*input.p_f_grid <= 0.0f)
         || (!isfinite(*input.p_theta_grid))
         || (!isfinite(*input.p_i_grid_alpha_raw))
         || (!isfinite(*input.p_i_grid_beta_observed))
         || (fabsf(*input.p_i_grid_alpha_raw) > i_grid_limit))
    {
        return 0u;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (    (!isfinite(input.p_v_bus_raw[cell]))
             || (!isfinite(input.p_i_load_raw[cell]))
             || (input.p_v_bus_raw[cell] < 0.0f)
             || (input.p_v_bus_raw[cell] > v_bus_max_limit))
        {
            return 0u;
        }

        if (    (chb_fsm_get_run_state() == CHB_RUN_STATE_RUN)
             && (input.p_v_bus_raw[cell] < v_bus_min_limit))
        {
            return 0u;
        }
    }
    return 1u;
}

uint8_t                             chb_protect_configure(float i_grid_trip, float i_precharge_trip,
                              float v_bus_min, float v_bus_max)
{
    if (    (chb_fsm_get_run_state() != CHB_RUN_STATE_INIT)
         || (!isfinite(i_grid_trip))
         || (!isfinite(i_precharge_trip))
         || (!isfinite(v_bus_min))
         || (!isfinite(v_bus_max))
         || (i_grid_trip <= 0.0f)
         || (i_precharge_trip <= 0.0f)
         || (v_bus_min <= 0.0f)
         || (v_bus_max <= v_bus_min))
    {
        return 0u;
    }
    i_grid_trip_limit      = i_grid_trip;
    i_precharge_trip_limit = i_precharge_trip;
    v_bus_min_limit        = v_bus_min;
    v_bus_max_limit        = v_bus_max;
    configured             = 1u;
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
