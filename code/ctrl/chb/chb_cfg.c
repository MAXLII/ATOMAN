// SPDX-License-Identifier: MIT
/**
 * @file chb_cfg.c
 * @brief Validate and retain CHB controller configuration and run request.
 * @details Parameters are immutable during IDLE/RUN; the FSM alone grants run permission.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_cfg.h"
#include "section.h"

#include <math.h>
#include <stddef.h>
#include <stdatomic.h>

#define CHB_DEFAULT_CFG                                             \
    {.ts                       = 0.0001f,                           \
     .grid_hz                  = 50.0f,                             \
     .l_grid_h                 = 0.012f,                            \
     .r_grid_ohm               = 0.5f,                              \
     .current_sample_delay_s   = 0.0f,                              \
     .grid_rms_nominal_v       = 6000.0f,                           \
     .bus_ref_v                = 3200.0f,                           \
     .bus_ref_ramp_v_per_s     = 5000.0f,                           \
     .bus_filter_hz            = 20.0f,                             \
     .energy_kp                = 40.0f,                             \
     .energy_ki                = 400.0f,                            \
     .current_kp               = 60.0f,                             \
     .current_ki               = 7000.0f,                           \
     .current_integral_limit_v = 2000.0f,                           \
     .power_limit_kp           = 1.0f,                              \
     .power_limit_ki           = 20.0f,                             \
     .current_limit_pk_a       = 25.0f,                             \
     .modulation_limit         = 0.98f,                             \
     .bus_capacitance_f        = {600.0e-6f, 600.0e-6f, 600.0e-6f}, \
     .output_power_limit_w     = {20000.0f, 20000.0f, 20000.0f}}

_Static_assert(CHB_PF_PLAN_TICKS > 0u, "PF planning interval must be positive");
_Static_assert(CHB_PF_SCAN_STEPS > 0u, "PF search requires a nonzero scan count");
_Static_assert(CHB_PF_REFINE_STEPS > 0u, "PF search requires refinement steps");

static const chb_ctrl_cfg_t default_cfg   = CHB_DEFAULT_CFG;     /* PLECS 空载及差模功率均衡调试基准。 */
static chb_ctrl_cfg_t       control_cfg   = CHB_DEFAULT_CFG;     /* INIT 前可改，随后锁定。 */
static atomic_uchar         run_request   = ATOMIC_VAR_INIT(0u); /* 应用请求，独立于 FSM 许可。 */
static atomic_uchar         config_locked = ATOMIC_VAR_INIT(0u); /* INIT 完成后禁止改系数。 */

chb_ctrl_cfg_t chb_cfg_default(void)
{
    return default_cfg;
}

/** @param p_cfg 待检查的完整参数。 @return 1：所有控制域有效。 */
static uint8_t cfg_valid(const chb_ctrl_cfg_t *p_cfg)
{
    if (p_cfg == NULL)
    {
        return 0u;
    }
    const float value[] = { /* 独立成员拷贝避免依赖结构体填充。 */
                           p_cfg->ts,
                           p_cfg->grid_hz,
                           p_cfg->l_grid_h,
                           p_cfg->r_grid_ohm,
                           p_cfg->current_sample_delay_s,
                           p_cfg->grid_rms_nominal_v,
                           p_cfg->bus_ref_v,
                           p_cfg->bus_ref_ramp_v_per_s,
                           p_cfg->bus_filter_hz,
                           p_cfg->energy_kp,
                           p_cfg->energy_ki,
                           p_cfg->current_kp,
                           p_cfg->current_ki,
                           p_cfg->current_integral_limit_v,
                           p_cfg->power_limit_kp,
                           p_cfg->power_limit_ki,
                           p_cfg->current_limit_pk_a,
                           p_cfg->modulation_limit,
                           CHB_PF_Q_SLEW_A_PER_S,
                           CHB_PF_VOLTAGE_RESERVE_V};

    for (uint32_t index = 0u; index < (uint32_t)(sizeof(value) / sizeof(value[0])); ++index)
    {
        if (!isfinite(value[index]))
        {
            return 0u;
        }
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (    !isfinite(p_cfg->bus_capacitance_f[cell])
             || !isfinite(p_cfg->output_power_limit_w[cell])
             || (p_cfg->bus_capacitance_f[cell] <= 0.0f)
             || (p_cfg->output_power_limit_w[cell] <= 0.0f))
        {
            return 0u;
        }
    }
    return (    (p_cfg->ts >= 1.0e-6f)
             && (p_cfg->ts <= 0.01f)
             && (p_cfg->grid_hz >= 1.0f)
             && (p_cfg->grid_hz <= 400.0f)
             && (p_cfg->l_grid_h > 0.0f)
             && (p_cfg->r_grid_ohm >= 0.0f)
             && (p_cfg->current_sample_delay_s >= 0.0f)
             && (p_cfg->current_sample_delay_s <= p_cfg->ts)
             && (p_cfg->grid_rms_nominal_v > 0.0f)
             && (p_cfg->bus_ref_v > 0.0f)
             && (p_cfg->bus_ref_ramp_v_per_s > 0.0f)
             && (p_cfg->bus_filter_hz > 0.0f)
             && (p_cfg->energy_kp >= 0.0f)
             && (p_cfg->energy_ki >= 0.0f)
             && (p_cfg->current_kp >= 0.0f)
             && (p_cfg->current_ki >= 0.0f)
             && (p_cfg->current_integral_limit_v > 0.0f)
             && (p_cfg->power_limit_kp >= 0.0f)
             && (p_cfg->power_limit_ki >= 0.0f)
             && (CHB_PF_Q_SLEW_A_PER_S > 0.0f)
             && (CHB_PF_VOLTAGE_RESERVE_V >= 0.0f)
             && (CHB_PF_VOLTAGE_RESERVE_V < p_cfg->modulation_limit * p_cfg->bus_ref_v)
             && (p_cfg->current_limit_pk_a > 0.0f)
             && (CHB_BALANCE_CURRENT_MIN_PK_A > 0.0f)
             && (CHB_BALANCE_CURRENT_MIN_PK_A <= p_cfg->current_limit_pk_a)
             && (p_cfg->modulation_limit > 0.0f)
             && (p_cfg->modulation_limit <= 1.0f))
             ? 1u
             : 0u;
}

uint8_t chb_cfg_set_ctrl_cfg(const chb_ctrl_cfg_t *p_cfg)
{
    if (    (atomic_load(&config_locked) != 0u)
         || (cfg_valid(p_cfg) == 0u))
    {
        return 0u;
    }
    control_cfg = *p_cfg; /* INIT 前单发布者一次复制完整参数。 */
    return 1u;
}

const chb_ctrl_cfg_t *chb_cfg_get_ctrl_cfg(void)
{
    return &control_cfg;
}

uint8_t chb_cfg_is_ready(void)
{
    return cfg_valid(&control_cfg);
}

void chb_cfg_unlock(void)
{
    atomic_store(&config_locked, 0u);
}

void chb_cfg_lock(void)
{
    atomic_store(&config_locked, 1u);
}

uint8_t chb_cfg_set_run_request(uint8_t request)
{
    if (request > 1u)
    {
        return 0u;
    }
    atomic_store(&run_request, request);
    return 1u;
}

uint8_t chb_cfg_get_run_request(void)
{
    return atomic_load(&run_request);
}

/** @brief 初始化只复位运行请求，保留平台在启动前发布的配置。 */
static void chb_cfg_init(void)
{
    atomic_store(&run_request, 0u);
}
REG_INIT(0, chb_cfg_init)
