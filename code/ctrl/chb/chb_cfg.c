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

#define CHB_DEFAULT_CFG                     \
    {.t_ctrl_period            = 0.0001f,   \
     .f_grid                   = 50.0f,     \
     .l_grid                   = CHB_L_GRID_DEFAULT, \
     .r_grid                   = 0.5f,      \
     .t_current_sample_delay   = 0.0f,      \
     .v_grid_rms_nominal       = 10000.0f,  \
     .v_bus_ref                = 3200.0f,   \
     .v_bus_ref_slew           = 5000.0f,   \
     .f_bus_filter             = 20.0f,     \
     .k_energy_p               = CHB_ENERGY_PI_KP_CALC(CHB_ENERGY_OMEGA_N, CHB_ENERGY_ZETA), \
     .k_energy_i               = CHB_ENERGY_PI_KI_CALC(CHB_ENERGY_OMEGA_N), \
     .k_current_p              = CHB_CURRENT_PI_KP_CALC(CHB_L_GRID_DEFAULT, CHB_CURRENT_OMEGA_N, CHB_CURRENT_ZETA), \
     .k_current_i              = CHB_CURRENT_PI_KI_CALC(CHB_L_GRID_DEFAULT, CHB_CURRENT_OMEGA_N), \
     .v_current_integral_limit = 2000.0f,   \
     .k_power_limit_p          = 1.0f,      \
     .k_power_limit_i          = 20.0f,     \
     .i_grid_peak_limit        = 25.0f,     \
     .modulation_limit         = 0.98f,     \
     .c_bus                    = 600.0e-6f, \
     .pwr_load_limit           = 20000.0f}

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
                           p_cfg->t_ctrl_period,
                           p_cfg->f_grid,
                           p_cfg->l_grid,
                           p_cfg->r_grid,
                           p_cfg->t_current_sample_delay,
                           p_cfg->v_grid_rms_nominal,
                           p_cfg->v_bus_ref,
                           p_cfg->v_bus_ref_slew,
                           p_cfg->f_bus_filter,
                           p_cfg->k_energy_p,
                           p_cfg->k_energy_i,
                           p_cfg->k_current_p,
                           p_cfg->k_current_i,
                           p_cfg->v_current_integral_limit,
                           p_cfg->k_power_limit_p,
                           p_cfg->k_power_limit_i,
                           p_cfg->i_grid_peak_limit,
                           p_cfg->modulation_limit,
                           p_cfg->c_bus,
                           p_cfg->pwr_load_limit,
                           CHB_PF_Q_SLEW_A_PER_S,
                           CHB_PF_VOLTAGE_RESERVE_V};

    for (uint32_t index = 0u; index < (uint32_t)(sizeof(value) / sizeof(value[0])); ++index)
    {
        if (!isfinite(value[index]))
        {
            return 0u;
        }
    }

    return (    (p_cfg->t_ctrl_period >= 1.0e-6f)
             && (p_cfg->t_ctrl_period <= 0.01f)
             && (p_cfg->f_grid >= 1.0f)
             && (p_cfg->f_grid <= 400.0f)
             && (p_cfg->l_grid > 0.0f)
             && (p_cfg->r_grid >= 0.0f)
             && (p_cfg->t_current_sample_delay >= 0.0f)
             && (p_cfg->t_current_sample_delay <= p_cfg->t_ctrl_period)
             && (p_cfg->v_grid_rms_nominal > 0.0f)
             && (p_cfg->v_bus_ref > 0.0f)
             && (p_cfg->v_bus_ref_slew > 0.0f)
             && (p_cfg->f_bus_filter > 0.0f)
             && (p_cfg->k_energy_p >= 0.0f)
             && (p_cfg->k_energy_i >= 0.0f)
             && (p_cfg->k_current_p >= 0.0f)
             && (p_cfg->k_current_i >= 0.0f)
             && (p_cfg->v_current_integral_limit > 0.0f)
             && (p_cfg->k_power_limit_p >= 0.0f)
             && (p_cfg->k_power_limit_i >= 0.0f)
             && (CHB_PF_Q_SLEW_A_PER_S > 0.0f)
             && (CHB_PF_VOLTAGE_RESERVE_V >= 0.0f)
             && (CHB_PF_VOLTAGE_RESERVE_V < p_cfg->modulation_limit * p_cfg->v_bus_ref)
             && (p_cfg->i_grid_peak_limit > 0.0f)
             && (p_cfg->c_bus > 0.0f)
             && (p_cfg->pwr_load_limit > 0.0f)
             && (CHB_BALANCE_CURRENT_MIN_PK_A > 0.0f)
             && (CHB_BALANCE_CURRENT_MIN_PK_A <= p_cfg->i_grid_peak_limit)
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
