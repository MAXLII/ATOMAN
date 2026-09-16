// SPDX-License-Identifier: MIT
/**
 * @file    npc_ctrl.c
 * @brief   Sample inputs and own DSOGI/PI/soft-start state; drive PWM after application protection.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot. The 1 ms ramp task,
 *          control interrupt and lifecycle callbacks require serialized dispatch.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_ctrl.h"
#include "npc_cfg.h"
#include "npc_fsm.h"
#include "npc_hal.h"
#include "dsogi.h"
#include "section.h"
#include "my_math.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

static npc_ctrl_cfg_t ctrl_cfg = {0};     /* 初始化时加载的固定控制系数。 */
static dsogi_t voltage_observer = {0};    /* 电压正负序观测器，绑定下方固定地址的输入。 */
static dsogi_t current_observer = {0};    /* 电流正负序观测器，绑定下方固定地址的输入。 */
static float v_ab[2] = {0};               /* 电压观测器的 alpha/beta 输入，V。 */
static float i_ab[2] = {0};               /* 电流观测器的 alpha/beta 输入，A。 */
static float integral_v[4] = {0};         /* 电压 PI 积分输出，轴序 d+、q+、d-、q-，A。 */
static float integral_i[4] = {0};         /* 电流 PI 积分输出，轴序 d+、q+、d-、q-，V。 */
static float i_bias_ab[2] = {0};          /* 非基波电流残差的低频偏置状态，A。 */
static float bias_filter_coeff = 0.0f;    /* 电流偏置滤波的一阶离散系数。 */
static float v_residual_lowpass[2] = {0}; /* 电压残差低通状态，V。 */
static float damping_filter_coeff = 0.0f; /* 电压残差滤波的一阶离散系数。 */
static bool initialized = false;          /* 两个观测器的初始化结果，用于计算前门控。 */
static float v_alpha = 0.0f;              /* 本拍送入调制的 alpha 轴电压指令，V。 */
static float v_beta = 0.0f;               /* 本拍送入调制的 beta 轴电压指令，V。 */
static float i_fundamental[3] = {0};      /* 同拍正负序合成的 A/B/C 基波电流，供中点平衡使用，A。 */
static float amplitude_target = 0.0f;     /* 与采样同步读取的外部幅值目标，V。 */
static bool control_allowed = false;      /* 本拍采样完成后置位，应用保护可撤销，控制阶段消耗。 */
static bool ramp_allowed = false;         /* 控制成功后允许任务推进斜坡；停机立即撤销。 */

/* 上一次完整读取的状态机快照；发布进行中时沿用此副本。 */
static npc_ctrl_setpoint_t active_setpoint =
    {
        .vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
        .freq_hz = 50.0f,
        .v_dc_half_min = 20.0f,
        .run_allowed = 0u};
static float reference_ramp = 0.0f; /* 1 ms 任务更新的软启动幅值，V；控制中断读取，停机清零。 */

/** @brief 清除观测器、积分、滤波历史及本拍 PWM 指令。 */
static void npc_ctrl_reset_states(void)
{
    dsogi_reset(&voltage_observer);                  /* 清除电压观测器历史状态。 */
    dsogi_reset(&current_observer);                  /* 清除电流观测器历史状态。 */
    (void)memset(integral_v, 0, sizeof(integral_v)); /* 清除电压外环积分。 */
    (void)memset(integral_i, 0, sizeof(integral_i)); /* 清除电流内环积分。 */
    (void)memset(i_bias_ab, 0, sizeof(i_bias_ab));   /* 停机、重启不沿用偏置补偿。 */
    (void)memset(v_residual_lowpass, 0,
                 sizeof(v_residual_lowpass));              /* 重启时清除电压残差滤波历史。 */
    v_alpha = 0.0f;                                        /* 清除停机前的电压指令。 */
    v_beta = 0.0f;                                         /* 清除停机前的电压指令。 */
    (void)memset(i_fundamental, 0, sizeof(i_fundamental)); /* 清除停机前的基波电流。 */
}

/**
 * @param p_cfg 固定配置，调用方保证指针及参数有效。
 * @return true：两个观测器初始化成功；false：观测器初始化失败。
 */
static bool npc_ctrl_init_states(const npc_ctrl_cfg_t *p_cfg)
{
    npc_ctrl_cfg_t cfg = *p_cfg;         /* 本次控制系数副本。 */
    dsogi_cfg_t observer_cfg = {0};      /* 两个固定频率观测器共用的配置。 */
    npc_ctrl_reset_states();             /* 初始化不沿用上次运行历史。 */
    initialized = false;                 /* 两个观测器初始化成功后才允许计算。 */
    (void)memset(v_ab, 0, sizeof(v_ab)); /* 观测器绑定前清除上次采样输入。 */
    (void)memset(i_ab, 0, sizeof(i_ab)); /* 观测器绑定前清除上次采样输入。 */
    ctrl_cfg = cfg;
    bias_filter_coeff = -expm1f(-M_2PI * cfg.current_bias_cutoff_hz * cfg.ts);
    damping_filter_coeff = -expm1f(-M_2PI * cfg.voltage_damping_cutoff_hz * cfg.ts);
    observer_cfg = (dsogi_cfg_t){
        .ts = cfg.ts,
        .k = cfg.sogi_k,
        .omega_min = cfg.omega,
        .omega_max = cfg.omega};
    if ((dsogi_init(&voltage_observer, &observer_cfg, &v_ab[0],
                    &v_ab[1], &ctrl_cfg.omega) == false) || /* 绑定电压观测器输入。 */
        (dsogi_init(&current_observer, &observer_cfg, &i_ab[0],
                    &i_ab[1], &ctrl_cfg.omega) == false)) /* 绑定电流观测器输入。 */
    {
        npc_ctrl_reset_states(); /* 初始化失败时保持禁止计算。 */
        return false;
    }
    initialized = true;
    return true;
}

void npc_ctrl_stop(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl(); /* 当前已挂载的 PWM 停机路径。 */

    p_hal->p_pwm_disable(); /* 调用方已完成绑定，直接关闭输出。 */

    control_allowed = false; /* 撤销尚未执行的本拍控制许可。 */
    ramp_allowed = false;    /* 停机后任务不再推进斜坡。 */
    npc_ctrl_reset_states(); /* 清除观测器、积分和有效输出。 */
    reference_ramp = 0.0f;   /* 下一次启动从零幅值开始。 */
}

/** @brief 初始化控制配置和动态，不访问 PWM 回调。 */
static void npc_ctrl_init(void)
{
    npc_ctrl_cfg_t cfg = npc_cfg_default(); /* 独立配置副本，按本次发布频率更新。 */

    control_allowed = false;                        /* 初始化后等待新的采样和控制许可。 */
    ramp_allowed = false;                           /* 等待控制成功后再启动斜坡任务。 */
    reference_ramp = 0.0f;                          /* 下一次启动从零幅值开始。 */
    (void)npc_fsm_read_published(&active_setpoint); /* 发布未完成时保留上次完整快照。 */
    cfg.ts = npc_cfg_get_ctrl_ts();                 /* 使用平台配置的实际控制周期。 */
    cfg.omega = M_2PI * active_setpoint.freq_hz;    /* 将已发布频率换算为角频率。 */

    (void)npc_ctrl_init_states(&cfg); /* 就绪标志由初始化结果维护，计算前检查。 */
}
REG_INIT(2, npc_ctrl_init)

void npc_ctrl_prepare_run(void)
{
    npc_ctrl_stop(); /* 运行准备阶段先通过已绑定的回调关闭输出。 */
    npc_ctrl_init(); /* 重新加载配置并复位控制动态。 */
}

/** @brief 中断第一层：读取本拍模拟采样、幅值和相位，不执行保护或发波。 */
static void FUNC_RAM npc_ctrl_sample(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl(); /* INIT 已检查并锁定的采样绑定。 */

    control_allowed = false; /* 每拍重新准备快照和许可，避免沿用上一拍数据。 */
    if (npc_fsm_get_run_sta() == NPC_RUN_STA_INIT)
    {
        return;
    }

    (void)npc_fsm_read_published(&active_setpoint); /* 发布冲突时沿用上次完整参数快照。 */
    npc_hal_sample();                               /* 形成保护与控制共用的本拍快照。 */
    amplitude_target = *p_hal->p_vd_pos_ref;        /* 外部幅值目标，软启动由 1 ms 任务处理。 */
    control_allowed = true;                         /* 采样完成，应用层保护可在控制阶段前撤销许可。 */
}
REG_INTERRUPT(1, npc_ctrl_sample)

/** @brief 中断第三层：读取软启动幅值，执行正负序双环计算并发波；第二层保护由 app 注册。 */
static void FUNC_RAM npc_ctrl_run(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl();        /* INIT 已检查并锁定的 PWM 回调。 */
    const npc_hal_sample_t *p_sample = npc_hal_get_sample(); /* 保护已检查的同拍采样。 */
    float omega = 0.0f;                                      /* 本拍发布频率对应的角频率，rad/s。 */
    float v_dq[4] = {0};             /* 本拍正负序电压反馈，V。 */
    float i_dq[4] = {0};             /* 本拍正负序电流反馈，A。 */
    float i_ref[4] = {0};            /* 本拍限幅后的电流给定，A。 */
    float u_dq[4] = {0};             /* 本拍限幅后的电压指令，V。 */
    float v_damping_ab[2] = {0};     /* 本拍高通残差补偿电压，V。 */
    float voltage_error[4] = {0};    /* 电压误差，轴序 d+、q+、d-、q-，V。 */
    float current_error[4] = {0};    /* 电流误差，轴序 d+、q+、d-、q-，A。 */
    float current_ref_raw[4] = {0};  /* 未限幅的电流给定，A。 */
    float voltage_ref_raw[4] = {0};  /* 未限幅的电压指令，V。 */
    float cosine = 0.0f;             /* 本拍电角度余弦。 */
    float sine = 0.0f;               /* 本拍电角度正弦。 */
    float magnitude = 0.0f;          /* 正负序电流幅值之和，A。 */
    float scale_i = 1.0f;            /* 正负序电流给定共用的缩放系数。 */
    float scale_u = 1.0f;            /* 正负序电压指令共用的缩放系数。 */
    float alpha = 0.0f;              /* 合成的静止坐标电压，V。 */
    float beta = 0.0f;               /* 合成的静止坐标电压，V。 */
    float phase_voltage[3] = {0};   /* 逆 Clarke 重构的 A/B/C 相电压指令，V。 */
    float voltage_pos_ab[2] = {0};  /* 正序逆 Park 输出，V。 */
    float voltage_neg_ab[2] = {0};  /* 负序逆 Park 输出，V。 */
    float span = 0.0f;               /* 所需三相电压跨度，V。 */
    float budget = 0.0f;             /* 可用母线电压跨度，V。 */
    float current_alpha = 0.0f;      /* 去除开关纹波后的正负序合成电流，A。 */
    float current_beta = 0.0f;       /* 基波电流 beta 分量，A。 */
    float voltage_residual[2] = {0}; /* 去除正负序基波后的电压残差，V。 */

    if (control_allowed == false)
    {
        return; /* 没有本拍采样或被应用保护禁止时，跳过控制。 */
    }
    control_allowed = false; /* 消耗本拍许可，重复调用不能重复更新积分或发波。 */

    if ((active_setpoint.run_allowed == 0u) || /* 状态机未授予运行许可。 */
        (npc_cfg_get_run_request() == 0u))     /* 停机请求本拍生效。 */
    {
        npc_ctrl_stop();
        return;
    }

    omega = M_2PI * active_setpoint.freq_hz; /* 将已发布频率换算为角频率。 */
    if ((initialized == false) ||            /* 尚未完成有效初始化。 */
        (ctrl_cfg.omega != omega))           /* 中心频率改变时重置观测器及软启动。 */
    {
        npc_ctrl_prepare_run();
    }

    if (initialized == false)        /* 实例须已完成有效初始化。 */
    {
        npc_ctrl_stop();
        return;
    }
    clarke(p_sample->v_out[0], p_sample->v_out[1], p_sample->v_out[2], &v_ab[0], &v_ab[1]); /* 电压采样变换。 */
    clarke(p_sample->i_l[0], p_sample->i_l[1], p_sample->i_l[2], &i_ab[0], &i_ab[1]); /* 电流采样变换。 */
    if ((dsogi_cal(&voltage_observer) == false) || /* 提取电压正负序分量。 */
        (dsogi_cal(&current_observer) == false))   /* 提取电流正负序分量。 */
    {
        npc_ctrl_stop();
        return;
    }
    /* 正序采用 theta，负序采用 -theta；两组变换复用本拍三角函数值。 */
    cosine = cosf(p_sample->theta);
    sine = sinf(p_sample->theta);
    park(voltage_observer.output.alpha_pos, voltage_observer.output.beta_pos,
                 sine, cosine, &v_dq[0], &v_dq[1]); /* 正序电压采用 theta。 */
    park(voltage_observer.output.alpha_neg, voltage_observer.output.beta_neg,
                 -sine, cosine, &v_dq[2], &v_dq[3]); /* 负序电压采用 -theta。 */
    park(current_observer.output.alpha_pos, current_observer.output.beta_pos,
                 sine, cosine, &i_dq[0], &i_dq[1]); /* 正序电流采用 theta。 */
    park(current_observer.output.alpha_neg, current_observer.output.beta_neg,
                 -sine, cosine, &i_dq[2], &i_dq[3]); /* 负序电流采用 -theta。 */
    /* 中点平衡使用基波电流，避免周期边界的开关纹波改变冗余电平选择方向。 */
    current_alpha = current_observer.output.alpha_pos + current_observer.output.alpha_neg;
    current_beta = current_observer.output.beta_pos + current_observer.output.beta_neg;
    inv_clarke(current_alpha, current_beta,
                       &i_fundamental[0], &i_fundamental[1], &i_fundamental[2]); /* 重构三相基波电流。 */
    /* 残差先滤除开关频率分量，再形成低频负反馈，避免直接反馈纹波激发延迟环路。 */
    i_bias_ab[0] += bias_filter_coeff *
                    (i_ab[0] - current_alpha - i_bias_ab[0]);
    i_bias_ab[1] += bias_filter_coeff *
                    (i_ab[1] - current_beta - i_bias_ab[1]);
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* 正负序 dq 轴序号。 */
    {
        voltage_error[axis] = -v_dq[axis];
        if (axis == 0u)
        {
            voltage_error[axis] += reference_ramp;
        }
        /* 外环输出作为内环电流给定，积分状态仍为上一拍更新后的值。 */
        current_ref_raw[axis] = ctrl_cfg.kp_v * voltage_error[axis] + integral_v[axis];
    }
    magnitude = hypotf(current_ref_raw[0], current_ref_raw[1]) + hypotf(current_ref_raw[2], current_ref_raw[3]);
    if (magnitude > ctrl_cfg.current_peak)
    {
        scale_i = ctrl_cfg.current_peak / magnitude;
    }
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* 正负序 dq 轴序号。 */
    {
        i_ref[axis] = current_ref_raw[axis] * scale_i;
        current_error[axis] = i_ref[axis] - i_dq[axis];
        voltage_ref_raw[axis] = ctrl_cfg.kp_i * current_error[axis] + integral_i[axis];
    }
    /* 将正负序电压指令合成到静止坐标系，再按三相电压跨度限制调制量。 */
    inv_park(voltage_ref_raw[0], voltage_ref_raw[1], sine, cosine,
                     &voltage_pos_ab[0], &voltage_pos_ab[1]); /* 正序电压返回静止坐标系。 */
    inv_park(voltage_ref_raw[2], voltage_ref_raw[3], -sine, cosine,
                     &voltage_neg_ab[0], &voltage_neg_ab[1]); /* 负序电压返回静止坐标系。 */
    alpha = voltage_pos_ab[0] + voltage_neg_ab[0]; /* 合成 alpha 轴电压指令。 */
    beta = voltage_pos_ab[1] + voltage_neg_ab[1]; /* 合成 beta 轴电压指令。 */
    /* 低频偏置补偿与 PI 电压共同经过后面的调制范围限幅；这不是 LC 高频阻尼。 */
    alpha -= ctrl_cfg.current_bias_resistance * i_bias_ab[0];
    beta -= ctrl_cfg.current_bias_resistance * i_bias_ab[1];
    /* 提取电压非基波残差；高通去除直流，不改变原 dq 基波调节目标。 */
    voltage_residual[0] = v_ab[0] - voltage_observer.output.alpha_pos -
                          voltage_observer.output.alpha_neg;
    voltage_residual[1] = v_ab[1] - voltage_observer.output.beta_pos -
                          voltage_observer.output.beta_neg;
    for (uint32_t axis = 0u; axis < 2u; ++axis) /* 更新低通状态，再相减得到高通补偿。 */
    {
        v_residual_lowpass[axis] += damping_filter_coeff *
                                    (voltage_residual[axis] - v_residual_lowpass[axis]);
        v_damping_ab[axis] = ctrl_cfg.voltage_damping_gain *
                             (voltage_residual[axis] - v_residual_lowpass[axis]);
    }
    alpha += v_damping_ab[0];
    beta += v_damping_ab[1];
    inv_clarke(alpha, beta, &phase_voltage[0], &phase_voltage[1], &phase_voltage[2]); /* 重构相电压。 */
    span = fmaxf(phase_voltage[0], fmaxf(phase_voltage[1], phase_voltage[2])) -
           fminf(phase_voltage[0], fminf(phase_voltage[1], phase_voltage[2])); /* 三相最大电压跨度。 */
    budget = ctrl_cfg.modulation_headroom * (p_sample->v_dc_p + p_sample->v_dc_n);
    if (span > budget)
    {
        scale_u = budget / span;
    }
    v_alpha = alpha * scale_u;
    v_beta = beta * scale_u;
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* 完成本拍输出和限幅后，再更新积分。 */
    {
        u_dq[axis] = voltage_ref_raw[axis] * scale_u;
        /* 使用限幅前后差值回算抗饱和，保持先输出、后积分的离散实现。 */
        integral_v[axis] += ctrl_cfg.ts *
                            (ctrl_cfg.ki_v * voltage_error[axis] +
                             ctrl_cfg.kaw_v * (i_ref[axis] - current_ref_raw[axis]));
        integral_i[axis] += ctrl_cfg.ts *
                            (ctrl_cfg.ki_i * current_error[axis] +
                             ctrl_cfg.kaw_i * (u_dq[axis] - voltage_ref_raw[axis]));
    }

    p_hal->p_set_pwm_func(v_alpha, v_beta,
                                       p_sample->v_dc_p, p_sample->v_dc_n,
                                       i_fundamental); /* 同拍电压指令和基波电流直接交给调制。 */

    ramp_allowed = true; /* 本拍控制计算完成并已调用发波，允许后续 1 ms 任务推进幅值。 */
}
REG_INTERRUPT(3, npc_ctrl_run)

/** @brief 1 ms 幅值斜坡任务；与控制中断串行调度，更新后的幅值供后续控制拍使用。 */
static void npc_ctrl_task(void)
{
    float ramp_step = active_setpoint.vd_pos_slew_vps * 0.001f; /* V/s 换算成每 1 ms 的幅值步长，V。 */

    if ((ramp_allowed == false) ||             /* 尚未成功运行或已经停波。 */
        (active_setpoint.run_allowed == 0u) || /* 状态机已撤销运行许可。 */
        (npc_cfg_get_run_request() == 0u))     /* 应用请求停机时不再推进幅值。 */
    {
        return;
    }

    RAMP(reference_ramp, amplitude_target, ramp_step); /* 相同步长升降，到达目标后保持。 */
}
REG_TASK_MS(1, npc_ctrl_task)
