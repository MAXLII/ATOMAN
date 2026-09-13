// SPDX-License-Identifier: MIT
/**
 * @file    npc_ctrl.c
 * @brief   Sample inputs and own DSOGI/PI/soft-start state; drive PWM after application protection.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
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
/* 控制器独占实例类型及固定地址的 DSOGI 输入绑定。 */
typedef struct npc_ctrl_inter
{
    dsogi_t voltage;     /* 电压正负序提取实例，包含内部固定绑定。 */
    dsogi_t current;     /* 电流正负序提取实例，包含内部固定绑定。 */
    float v_ab[2];       /* 电压 DSOGI 的 alpha/beta 输入，V。 */
    float i_ab[2];       /* 电流 DSOGI 的 alpha/beta 输入，A。 */
    float integral_v[4]; /* 电压 PI 积分输出，A。 */
    float integral_i[4]; /* 电流 PI 积分输出，V。 */
    float i_bias_ab[2];  /* 非基波电流残差的低频偏置状态，A。 */
    float bias_filter_coeff; /* 初始化时由截止频率和采样周期计算的一阶系数。 */
    float v_residual_lowpass[2]; /* 电压残差低通状态，用于去除补偿支路的直流增益，V。 */
    float damping_filter_coeff; /* 电压残差滤波的一阶离散系数。 */
    bool initialized;    /* 配置是否已接受；下次初始化前配置保持固定。 */
} npc_ctrl_inter_t;

typedef struct npc_ctrl
{
    npc_ctrl_cfg_t cfg;       /* 实例持有的固定配置。 */
    npc_ctrl_inter_t inter;   /* 私有运行动态；已初始化实例含指针，禁止按值复制。 */
    npc_ctrl_output_t output; /* 最近一次完整计算结果。 */
} npc_ctrl_t;

static npc_ctrl_t controller = {0};      /* 控制实例独占的观测器、积分和输出状态。 */
static npc_ctrl_monitor_t monitor = {0}; /* 供平台串行读取的诊断副本。 */
static npc_ctrl_sample_t sampled_input = {0}; /* 采样阶段写入，保护和控制阶段共用的本拍快照。 */
static float amplitude_target = 0.0f;       /* 与采样同步读取的外部幅值目标，V。 */
static bool control_allowed = false;       /* 本拍采样完成后置位，应用保护可撤销，控制阶段消耗。 */

/* 上一次完整读取的状态机快照；发布进行中时沿用此副本。 */
static npc_ctrl_setpoint_t active_setpoint =
    {
        .vd_pos_slew_vps = NPC_CFG_DEFAULT_VD_POS_SLEW_VPS,
        .freq_hz = 50.0f,
        .v_dc_half_min = 20.0f,
        .run_allowed = 0u};
static float reference_ramp = 0.0f; /* 软启动幅值累加器，V，采用单精度并保持拍后更新时机。 */

/** @param p_ctrl 待复位的控制实例，调用方保证指针有效。 */
static void npc_ctrl_reset_states(npc_ctrl_t *p_ctrl)
{
    dsogi_reset(&p_ctrl->inter.voltage);                                         /* 清除电压观测器历史状态。 */
    dsogi_reset(&p_ctrl->inter.current);                                         /* 清除电流观测器历史状态。 */
    (void)memset(p_ctrl->inter.integral_v, 0, sizeof(p_ctrl->inter.integral_v)); /* 清除电压外环积分。 */
    (void)memset(p_ctrl->inter.integral_i, 0, sizeof(p_ctrl->inter.integral_i)); /* 清除电流内环积分。 */
    (void)memset(p_ctrl->inter.i_bias_ab, 0, sizeof(p_ctrl->inter.i_bias_ab)); /* 停机、重启不沿用偏置补偿。 */
    (void)memset(p_ctrl->inter.v_residual_lowpass, 0,
                 sizeof(p_ctrl->inter.v_residual_lowpass)); /* 重启时清除电压残差滤波历史。 */
    p_ctrl->output = (npc_ctrl_output_t){0};                                     /* 下次使用前必须重新完成有效计算。 */
}

/**
 * @param p_ctrl 待初始化实例，地址必须有效。
 * @param p_cfg 固定配置，调用方保证指针及参数有效；可指向实例内部配置。
 * @return true：两个观测器初始化成功；false：观测器初始化失败。
 */
static bool npc_ctrl_init_states(npc_ctrl_t *p_ctrl, const npc_ctrl_cfg_t *p_cfg)
{
    npc_ctrl_cfg_t cfg = *p_cfg;    /* 先复制配置再清空实例，允许以实例内配置重新初始化。 */
    dsogi_cfg_t observer_cfg = {0}; /* 两个固定频率观测器共用的配置。 */
    (void)memset(p_ctrl, 0, sizeof(*p_ctrl));
    p_ctrl->cfg = cfg;
    p_ctrl->inter.bias_filter_coeff = -expm1f(-M_2PI * cfg.current_bias_cutoff_hz * cfg.ts);
    p_ctrl->inter.damping_filter_coeff = -expm1f(-M_2PI * cfg.voltage_damping_cutoff_hz * cfg.ts);
    observer_cfg = (dsogi_cfg_t){
        .ts = cfg.ts,
        .k = cfg.sogi_k,
        .omega_min = cfg.omega,
        .omega_max = cfg.omega};
    if ((dsogi_init(&p_ctrl->inter.voltage, &observer_cfg, &p_ctrl->inter.v_ab[0],
                    &p_ctrl->inter.v_ab[1], &p_ctrl->cfg.omega) == false) || /* 绑定电压观测器输入。 */
        (dsogi_init(&p_ctrl->inter.current, &observer_cfg, &p_ctrl->inter.i_ab[0],
                    &p_ctrl->inter.i_ab[1], &p_ctrl->cfg.omega) == false)) /* 绑定电流观测器输入。 */
    {
        (void)memset(p_ctrl, 0, sizeof(*p_ctrl));
        return false;
    }
    p_ctrl->inter.initialized = true;
    return true;
}

/**
 * @param p_abc 三相采样快照，轴序 A/B/C。
 * @param p_ab 等幅值 Clarke 变换的 alpha/beta 输出地址。
 */
static inline void clarke(const float *p_abc, float *p_ab)
{
    p_ab[0] = (2.0f * p_abc[0] - p_abc[1] - p_abc[2]) / 3.0f;
    p_ab[1] = (p_abc[1] - p_abc[2]) * M_1_SQRT3;
}

/**
 * @param p_seq DSOGI 提取的正负序分量。
 * @param cosine 本拍电角度余弦。
 * @param sine 本拍电角度正弦。
 * @param p_dq 输出地址，轴序 d+、q+、d-、q-。
 */
static inline void park(const dsogi_output_t *p_seq, float cosine, float sine, float *p_dq)
{
    /* 前两轴为正序旋转坐标，后两轴为反向旋转的负序坐标。 */
    p_dq[0] = cosine * p_seq->alpha_pos + sine * p_seq->beta_pos;
    p_dq[1] = -sine * p_seq->alpha_pos + cosine * p_seq->beta_pos;
    p_dq[2] = cosine * p_seq->alpha_neg - sine * p_seq->beta_neg;
    p_dq[3] = sine * p_seq->alpha_neg + cosine * p_seq->beta_neg;
}

/**
 * @param p_ctrl 控制实例，调用方保证指针有效；计算失败时清除动态。
 * @param p_input 本拍电压、电流、相位和给定快照，调用方保证指针及参数有效。
 * @return true：输出有效；false：实例未初始化或观测器计算失败。
 */
static bool FUNC_RAM npc_ctrl_cal(npc_ctrl_t *p_ctrl, const npc_ctrl_sample_t *p_input)
{
    float voltage_error[4] = {0};           /* 电压误差，轴序 d+、q+、d-、q-，V。 */
    float current_error[4] = {0};           /* 电流误差，轴序 d+、q+、d-、q-，A。 */
    float current_ref_raw[4] = {0};         /* 未限幅的电流给定，A。 */
    float voltage_ref_raw[4] = {0};         /* 未限幅的电压指令，V。 */
    float cosine = 0.0f;                    /* 本拍电角度余弦。 */
    float sine = 0.0f;                      /* 本拍电角度正弦。 */
    float magnitude = 0.0f;                 /* 正负序电流幅值之和，A。 */
    float scale_i = 1.0f;                   /* 正负序电流给定共用的缩放系数。 */
    float scale_u = 1.0f;                   /* 正负序电压指令共用的缩放系数。 */
    float alpha = 0.0f;                     /* 合成的静止坐标电压，V。 */
    float beta = 0.0f;                      /* 合成的静止坐标电压，V。 */
    float phase_b = 0.0f;                   /* 重构的 B 相电压指令，V。 */
    float phase_c = 0.0f;                   /* 重构的 C 相电压指令，V。 */
    float span = 0.0f;                      /* 所需三相电压跨度，V。 */
    float budget = 0.0f;                    /* 可用母线电压跨度，V。 */
    float current_alpha = 0.0f;             /* 去除开关纹波后的正负序合成电流，A。 */
    float current_beta = 0.0f;              /* 基波电流 beta 分量，A。 */
    float voltage_residual[2] = {0};        /* 去除正负序基波后的电压残差，V。 */
    if (p_ctrl->inter.initialized == false) /* 实例须已完成有效初始化。 */
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    clarke(p_input->v_out, p_ctrl->inter.v_ab);         /* 变换输出电压采样。 */
    clarke(p_input->i_l, p_ctrl->inter.i_ab);           /* 变换电感电流采样。 */
    if ((dsogi_cal(&p_ctrl->inter.voltage) == false) || /* 提取电压正负序分量。 */
        (dsogi_cal(&p_ctrl->inter.current) == false))   /* 提取电流正负序分量。 */
    {
        npc_ctrl_reset_states(p_ctrl);
        return false;
    }
    /* 正序采用 theta，负序采用 -theta；两组变换复用本拍三角函数值。 */
    cosine = cosf(p_input->theta);
    sine = sinf(p_input->theta);
    park(&p_ctrl->inter.voltage.output, cosine, sine, p_ctrl->output.v_dq); /* 变换正负序电压到各自旋转坐标系。 */
    park(&p_ctrl->inter.current.output, cosine, sine, p_ctrl->output.i_dq); /* 变换正负序电流到各自旋转坐标系。 */
    /* 中点平衡使用基波电流，避免周期边界的开关纹波改变冗余电平选择方向。 */
    current_alpha = p_ctrl->inter.current.output.alpha_pos + p_ctrl->inter.current.output.alpha_neg;
    current_beta = p_ctrl->inter.current.output.beta_pos + p_ctrl->inter.current.output.beta_neg;
    p_ctrl->output.i_fundamental[0] = current_alpha;
    p_ctrl->output.i_fundamental[1] = -0.5f * current_alpha + M_SQRT3_2 * current_beta;
    p_ctrl->output.i_fundamental[2] = -0.5f * current_alpha - M_SQRT3_2 * current_beta;
    /* 残差先滤除开关频率分量，再形成低频负反馈，避免直接反馈纹波激发延迟环路。 */
    p_ctrl->inter.i_bias_ab[0] += p_ctrl->inter.bias_filter_coeff *
        (p_ctrl->inter.i_ab[0] - current_alpha - p_ctrl->inter.i_bias_ab[0]);
    p_ctrl->inter.i_bias_ab[1] += p_ctrl->inter.bias_filter_coeff *
        (p_ctrl->inter.i_ab[1] - current_beta - p_ctrl->inter.i_bias_ab[1]);
    p_ctrl->output.i_bias_ab[0] = p_ctrl->inter.i_bias_ab[0];
    p_ctrl->output.i_bias_ab[1] = p_ctrl->inter.i_bias_ab[1];
    for (uint32_t axis = 0u; axis < 4u; ++axis)                             /* 正负序 dq 轴序号。 */
    {
        voltage_error[axis] = -p_ctrl->output.v_dq[axis];
        if (axis == 0u)
        {
            voltage_error[axis] += p_input->vd_pos_ref;
        }
        /* 外环输出作为内环电流给定，积分状态仍为上一拍更新后的值。 */
        current_ref_raw[axis] = p_ctrl->cfg.kp_v * voltage_error[axis] + p_ctrl->inter.integral_v[axis];
    }
    magnitude = hypotf(current_ref_raw[0], current_ref_raw[1]) + hypotf(current_ref_raw[2], current_ref_raw[3]);
    if (magnitude > p_ctrl->cfg.current_peak)
    {
        scale_i = p_ctrl->cfg.current_peak / magnitude;
    }
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* 正负序 dq 轴序号。 */
    {
        p_ctrl->output.i_ref[axis] = current_ref_raw[axis] * scale_i;
        current_error[axis] = p_ctrl->output.i_ref[axis] - p_ctrl->output.i_dq[axis];
        voltage_ref_raw[axis] = p_ctrl->cfg.kp_i * current_error[axis] + p_ctrl->inter.integral_i[axis];
    }
    /* 将正负序电压指令合成到静止坐标系，再按三相电压跨度限制调制量。 */
    alpha = cosine * (voltage_ref_raw[0] + voltage_ref_raw[2]) + sine * (voltage_ref_raw[3] - voltage_ref_raw[1]);
    beta = sine * (voltage_ref_raw[0] - voltage_ref_raw[2]) + cosine * (voltage_ref_raw[1] + voltage_ref_raw[3]);
    /* 低频偏置补偿与 PI 电压共同经过后面的调制范围限幅；这不是 LC 高频阻尼。 */
    alpha -= p_ctrl->cfg.current_bias_resistance * p_ctrl->inter.i_bias_ab[0];
    beta -= p_ctrl->cfg.current_bias_resistance * p_ctrl->inter.i_bias_ab[1];
    /* 提取电压非基波残差；高通去除直流，不改变原 dq 基波调节目标。 */
    voltage_residual[0] = p_ctrl->inter.v_ab[0] - p_ctrl->inter.voltage.output.alpha_pos -
                          p_ctrl->inter.voltage.output.alpha_neg;
    voltage_residual[1] = p_ctrl->inter.v_ab[1] - p_ctrl->inter.voltage.output.beta_pos -
                          p_ctrl->inter.voltage.output.beta_neg;
    for (uint32_t axis = 0u; axis < 2u; ++axis) /* 更新低通状态，再相减得到高通补偿。 */
    {
        p_ctrl->inter.v_residual_lowpass[axis] += p_ctrl->inter.damping_filter_coeff *
            (voltage_residual[axis] - p_ctrl->inter.v_residual_lowpass[axis]);
        p_ctrl->output.v_damping_ab[axis] = p_ctrl->cfg.voltage_damping_gain *
            (voltage_residual[axis] - p_ctrl->inter.v_residual_lowpass[axis]);
    }
    alpha += p_ctrl->output.v_damping_ab[0];
    beta += p_ctrl->output.v_damping_ab[1];
    phase_b = -0.5f * alpha + M_SQRT3_2 * beta;
    phase_c = -0.5f * alpha - M_SQRT3_2 * beta;
    span = fmaxf(alpha, fmaxf(phase_b, phase_c)) - fminf(alpha, fminf(phase_b, phase_c));
    budget = p_ctrl->cfg.modulation_headroom * (p_input->v_dc_p + p_input->v_dc_n);
    if (span > budget)
    {
        scale_u = budget / span;
    }
    p_ctrl->output.v_alpha = alpha * scale_u;
    p_ctrl->output.v_beta = beta * scale_u;
    for (uint32_t axis = 0u; axis < 4u; ++axis) /* 完成本拍输出和限幅后，再更新积分。 */
    {
        p_ctrl->output.u_dq[axis] = voltage_ref_raw[axis] * scale_u;
        /* 使用限幅前后差值回算抗饱和，保持先输出、后积分的离散实现。 */
        p_ctrl->inter.integral_v[axis] += p_ctrl->cfg.ts *
                                          (p_ctrl->cfg.ki_v * voltage_error[axis] +
                                           p_ctrl->cfg.kaw_v * (p_ctrl->output.i_ref[axis] - current_ref_raw[axis]));
        p_ctrl->inter.integral_i[axis] += p_ctrl->cfg.ts *
                                          (p_ctrl->cfg.ki_i * current_error[axis] +
                                           p_ctrl->cfg.kaw_i * (p_ctrl->output.u_dq[axis] - voltage_ref_raw[axis]));
    }
    p_ctrl->output.current_limited = scale_i < 1.0f;
    p_ctrl->output.voltage_limited = scale_u < 1.0f;
    p_ctrl->output.valid = true;
    return true;
}

void npc_ctrl_stop(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl(); /* 当前已挂载的 PWM 停机路径。 */

    p_hal->p_pwm_disable(); /* 调用方已完成绑定，直接关闭输出。 */

    control_allowed = false;                               /* 撤销尚未执行的本拍控制许可。 */
    npc_ctrl_reset_states(&controller);                     /* 清除观测器、积分和有效输出。 */
    reference_ramp = 0.0f;                                   /* 下一次启动从零幅值开始。 */
    monitor = (npc_ctrl_monitor_t){.status = NPC_CTRL_OFF}; /* 发布停机诊断，不清除保护闭锁。 */
}

/** @brief 初始化控制配置和动态，不访问 PWM 回调。 */
static void npc_ctrl_init(void)
{
    npc_ctrl_cfg_t cfg = npc_cfg_default(); /* 独立配置副本，按本次发布频率更新。 */

    control_allowed = false; /* 初始化后等待新的采样和控制许可。 */
    reference_ramp = 0.0f; /* 下一次启动从零幅值开始。 */
    monitor = (npc_ctrl_monitor_t){.status = NPC_CTRL_OFF}; /* 复位控制诊断。 */
    (void)npc_fsm_read_published(&active_setpoint); /* 发布未完成时保留上次完整快照。 */
    cfg.ts = npc_cfg_get_ctrl_ts();                 /* 使用平台配置的实际控制周期。 */
    cfg.omega = M_2PI * active_setpoint.freq_hz; /* 将已发布频率换算为角频率。 */

    if (npc_ctrl_init_states(&controller, &cfg) == false)
    {
        monitor.status = NPC_CTRL_CONTROL;
    }
}
REG_INIT(2, npc_ctrl_init)

void npc_ctrl_prepare_run(void)
{
    npc_ctrl_stop(); /* 运行准备阶段先通过已绑定的回调关闭输出。 */
    npc_ctrl_init(); /* 重新加载配置并复位控制动态。 */
}

const npc_ctrl_monitor_t *npc_ctrl_get_monitor(void)
{
    return &monitor;
}

const npc_ctrl_sample_t *npc_ctrl_get_sample(void)
{
    return &sampled_input;
}

void npc_ctrl_inhibit(uint32_t status, uint32_t detail)
{
    npc_ctrl_stop();          /* 应用层禁止本拍及当前 PWM 输出。 */
    monitor.status = status; /* 发布应用层给出的停机原因。 */
    monitor.detail = detail; /* 发布故障相序号等附加信息。 */
}

/** @brief 中断第一层：读取本拍模拟采样、幅值和相位，不执行保护或发波。 */
static void FUNC_RAM npc_ctrl_sample(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl(); /* INIT 已检查并锁定的采样绑定。 */

    control_allowed = false; /* 每拍重新准备快照和许可，避免沿用上一拍数据。 */
    if (npc_fsm_get_run_sta() == NPC_RUN_STA_INIT)
    {
        monitor.status = NPC_CTRL_BINDING; /* INIT 未完成时不访问采样指针。 */
        return;
    }

    (void)npc_fsm_read_published(&active_setpoint); /* 发布冲突时沿用上次完整参数快照。 */
    for (uint32_t phase = 0u; phase < 3u; ++phase) /* 按 A、B、C 相采集反馈。 */
    {
        sampled_input.v_out[phase] = *p_hal->p_v_out[phase];
        sampled_input.i_l[phase] = *p_hal->p_i_l[phase];
    }
    sampled_input.v_dc_p = *p_hal->p_v_dc_p; /* 正侧母线电压。 */
    sampled_input.v_dc_n = *p_hal->p_v_dc_n; /* 负侧母线电压幅值。 */
    sampled_input.theta = *p_hal->p_theta;   /* 本拍电角度。 */
    amplitude_target = *p_hal->p_vd_pos_ref; /* 外部幅值目标，软启动在控制阶段处理。 */
    control_allowed = true;                 /* 采样完成，应用层保护可在控制阶段前撤销许可。 */
}
REG_INTERRUPT(1, npc_ctrl_sample)

/** @brief 中断第三层：执行软启动、正负序双环计算并直接发波；第二层保护由 app 注册。 */
static void FUNC_RAM npc_ctrl_run(void)
{
    const npc_ctrl_hal_t *p_hal = npc_hal_get_ctrl(); /* INIT 已检查并锁定的 PWM 回调。 */
    uint32_t pwm_status = 0u;                        /* PWM 回调状态，0 为成功。 */
    float omega = 0.0f;                             /* 本拍发布频率对应的角频率，rad/s。 */
    float ramp_step = 0.0f;                         /* 已发布斜率乘控制周期，V。 */

    if (control_allowed == false)
    {
        return; /* 没有本拍采样或被应用保护禁止时，保留诊断并跳过控制。 */
    }
    control_allowed = false; /* 消耗本拍许可，重复调用不能重复更新积分或发波。 */

    if ((active_setpoint.run_allowed == 0u) || /* 状态机未授予运行许可。 */
        (npc_cfg_get_run_request() == 0u))     /* 停机请求本拍生效。 */
    {
        npc_ctrl_stop();
        return;
    }

    omega = M_2PI * active_setpoint.freq_hz; /* 将已发布频率换算为角频率。 */
    ramp_step = active_setpoint.vd_pos_slew_vps * npc_cfg_get_ctrl_ts(); /* 单精度软启动步进。 */
    if ((controller.inter.initialized == false) || /* 尚未完成有效初始化。 */
        (controller.cfg.omega != omega))          /* 中心频率改变时重置观测器及软启动。 */
    {
        npc_ctrl_prepare_run();
    }

    sampled_input.vd_pos_ref = reference_ramp; /* 本拍使用更新前的软启动幅值。 */
    if (npc_ctrl_cal(&controller, &sampled_input) == false)
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_CONTROL;
        return;
    }

    pwm_status = p_hal->p_set_pwm_func(controller.output.v_alpha, controller.output.v_beta,
                                      sampled_input.v_dc_p, sampled_input.v_dc_n,
                                      controller.output.i_fundamental); /* 同拍电压指令和基波电流直接交给调制。 */
    if (pwm_status != 0u)
    {
        npc_ctrl_stop();
        monitor.status = NPC_CTRL_PWM;
        monitor.detail = pwm_status;
        return;
    }

    monitor.output = controller.output; /* 发布本拍完整控制输出。 */
    (void)memcpy(monitor.integral_v, controller.inter.integral_v,
                 sizeof(monitor.integral_v)); /* 复制外环积分监视值。 */
    (void)memcpy(monitor.integral_i, controller.inter.integral_i,
                 sizeof(monitor.integral_i)); /* 复制内环积分监视值。 */
    monitor.vd_pos_ref_act = sampled_input.vd_pos_ref; /* 本拍实际采用的幅值。 */
    monitor.status = NPC_CTRL_RUNNING;
    monitor.detail = 0u;

    /* 保留拍后更新顺序和单精度累加；新幅值供下一拍使用。 */
    if (reference_ramp < amplitude_target)
    {
        reference_ramp = fminf(reference_ramp + ramp_step, amplitude_target);
    }
    else
    {
        reference_ramp = fmaxf(reference_ramp - ramp_step, amplitude_target);
    }
}
REG_INTERRUPT(3, npc_ctrl_run)
