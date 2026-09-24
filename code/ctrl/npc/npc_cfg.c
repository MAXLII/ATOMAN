// SPDX-License-Identifier: MIT
/**
 * @file npc_cfg.c
 * @brief Validate application requests and maintain the FSM building parameters.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "section.h"
#include "my_math.h"

#include <stdatomic.h>

/* 应用参数池；开停机请求允许中断原子读取。 */
static npc_cfg_t config = {.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
                           .freq_hz         = 50.0f,
                           .v_dc_half_min   = 20.0f,
                           .run_request     = 0u};

/* 状态机发布前的构建副本，不直接授予运行许可。 */
static npc_ctrl_setpoint_t building = {.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
                                       .freq_hz         = 50.0f,
                                       .v_dc_half_min   = 20.0f,
                                       .run_allowed     = 0u};

static float sample_period = 0.0002f; /* 当前控制周期，s，停机配置后使用。 */

npc_ctrl_cfg_t npc_cfg_default(void)
{
    return (npc_ctrl_cfg_t){.ts     = 0.0002f,
                            .omega  = M_2PI * 50.0f,
                            .sogi_k = M_SQRT2,
                            /* 保留 DSOGI、采样延迟和动态调制误差模型的联合整定结果。 */
                            .kp_v                    = 1.0f,
                            .ki_v                    = 1300.0f,
                            .kp_i                    = 0.15f,
                            .ki_i                    = 0.05f,
                            .kaw_v                   = 10.0f,
                            .kaw_i                   = 10.0f,
                            .current_peak            = 5200.0f, /* Yd 模型的分析容量，不表示实物额定电流。 */
                            .modulation_headroom     = 0.95f,
                            .current_bias_cutoff_hz  = 3.0f,
                            .current_bias_resistance = 0.1f,
                            /* 5 kHz 控制下，容性负载的约 2.3 kHz 模态会被延迟的电压残差正反馈激发。
                             * 默认关闭此补偿；保留基波双环及低频电流偏置反馈，不放宽过流门限。 */
                            .voltage_damping_gain      = 0.0f,
                            .voltage_damping_cutoff_hz = 100.0f};
}

/** @brief 复位运行参数池和构建副本，保留平台已经设置的控制周期。 */
static void npc_cfg_init(void)
{
    config.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS; /* 恢复默认软启动斜率。 */
    config.freq_hz         = 50.0f;  /* 恢复默认电频率。 */
    config.v_dc_half_min   = 20.0f;  /* 恢复单侧母线门限。 */
    building = (npc_ctrl_setpoint_t) /* 清除上次运行的构建参数。 */
        {.vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
         .freq_hz         = 50.0f,
         .v_dc_half_min   = 20.0f,
         .run_allowed     = 0u};
    atomic_store(&config.run_request, 0u); /* 初始化后必须重新请求开机。 */
}
REG_INIT(0, npc_cfg_init)

uint8_t npc_cfg_set_ctrl_ts(float ctrl_ts)
{
    if (!(    (ctrl_ts >= 1.0e-6f) /* 排除过小周期及 NaN。 */
           && (ctrl_ts <= 0.01f))) /* 排除过大周期及无穷大。 */
    {
        return 0u;
    }

    sample_period = ctrl_ts;
    return 1u;
}

float npc_cfg_get_ctrl_ts(void)
{
    return sample_period;
}

uint8_t npc_cfg_is_ready(void)
{
    float step = M_2PI * config.freq_hz * sample_period; /* 每拍电角度步进，rad。 */

    return (    (step >= 0.001f) /* 满足 DSOGI 数值步进下限。 */
             && (step <= 1.0f))  /* 满足 DSOGI 数值步进上限。 */
             ? 1u
             : 0u;
}

uint8_t npc_cfg_set_vd_pos_slew_vps(float value)
{
    if (!(    (value >= 0.001f)   /* 斜率必须为正且不能为 NaN。 */
           && (value <= 1.0e6f))) /* 约束每拍幅值更新的计算范围。 */
    {
        return 0u;
    }

    config.vd_pos_slew_vps = value;
    return 1u;
}

uint8_t npc_cfg_set_freq_hz(float value)
{
    float step = M_2PI * value * sample_period; /* 每拍电角度，rad。 */

    if (!(    (value >= 1.0f)  /* 频率请求至少为 1 Hz。 */
           && (step >= 0.001f) /* 满足 DSOGI 数值步进下限。 */
           && (step <= 1.0f))) /* 同时排除过快步进及无穷大。 */
    {
        return 0u;
    }

    config.freq_hz = value;
    return 1u;
}

uint8_t npc_cfg_set_v_dc_half_min(float value)
{
    if (!(    (value >= 0.001f)   /* 单侧母线门限必须为正有限数。 */
           && (value <= 1.0e6f))) /* 保持控制器支持的电压计算范围。 */
    {
        return 0u;
    }

    config.v_dc_half_min = value;
    return 1u;
}

uint8_t npc_cfg_set_run_request(uint8_t request)
{
    if (request > 1u)
    {
        return 0u;
    }

    atomic_store(&config.run_request, request);
    return 1u;
}

uint8_t npc_cfg_get_run_request(void)
{
    return atomic_load(&config.run_request);
}

NPC_RUN_STA_E npc_cfg_get_run_state(void)
{
    return npc_fsm_get_run_sta();
}

const npc_ctrl_setpoint_t *npc_cfg_get_p_building(void)
{
    building.vd_pos_slew_vps = config.vd_pos_slew_vps; /* 复制应用侧斜率请求。 */
    building.freq_hz         = config.freq_hz;         /* 复制 DSOGI 频率请求。 */
    building.v_dc_half_min   = config.v_dc_half_min;   /* 复制母线门限请求。 */
    building.run_allowed     = 0u; /* 运行许可只由状态机发布。 */
    return &building;
}
