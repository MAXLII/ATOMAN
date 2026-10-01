// SPDX-License-Identifier: MIT
/**
 * @file chb_cfg.h
 * @brief CHB rectifier control parameters and application run request.
 * @details Cascaded single-phase rectifier; parameters are fixed after FSM INIT.
 *          Entity: series-connected bridges with independent DC buses.
 *          Prior: every bus sample and PWM callback must be valid before RUN.
 *          Time: configuration is fixed after INIT; each control tick publishes one frame.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_CFG_H
#define CHB_CFG_H

#include <stdint.h>

#define CHB_CELL_COUNT               5u     /* 串联 H 桥数量；各桥直流母线相互独立。 */
#define CHB_HARMONIC_COUNT           4u     /* 电网 3、5、7、9 次谐波前馈。 */
#define CHB_HARMONIC_CURRENT_GAIN    400.0f /* 选频电流反馈等效阻尼，V/A。 */
#define CHB_MAIN_RELAY_ON_TIME_S     0.008f /* rly_on 使用的继电器机械闭合时间。 */
#define CHB_MAIN_RELAY_WAIT_MS       20u    /* rly_on 确认闭合后至控制运行的等待时间。 */
#define CHB_MAIN_RELAY_MATCH_RATIO   0.02f  /* 继电器两端压差允许值占电网峰值的比例。 */
#define CHB_BUS_NOTCH_BANDWIDTH_HZ   20.0f  /* 母线二倍工频陷波带宽，Hz。 */
#define CHB_BALANCE_CURRENT_MIN_PK_A 0.5f   /* 轻载功率投影的最小电流，A；低于此值须验证开关纹波影响。 */
#define CHB_PF_Q_SLEW_A_PER_S        600.0f /* 最小无功参考的升降变化率，A/s。 */
#define CHB_PF_VOLTAGE_RESERVE_V     0.0f   /* 规划附加电压余量，V；逐拍分配仍按实时母线限幅。 */
#define CHB_PF_PLAN_TICKS            10u    /* 100 us 控制周期下每 1 ms 重新求可行无功。 */
#define CHB_PF_SCAN_STEPS            32u    /* 在电流额定圆内寻找第一个可行区间。 */
#define CHB_PF_REFINE_STEPS          16u    /* 对第一个不可行/可行边界做有界二分。 */

/* Entity: 默认电感和 PI 整定目标是配置的唯一来源，Kp/Ki 由下方宏生成。
 * Prior: 沿用已验证整定点；电容已进入能量误差，不能在增益中再次计入。
 * Time: 默认增益编译期生成，PLL 增益仅在 app INIT 应用，不增加周期运算。 */
#define CHB_L_GRID_DEFAULT  0.012f        /* 默认输入电感，H；电流环增益与模型解耦共用此值。 */
#define CHB_ENERGY_OMEGA_N  20.0f         /* 能量环自然角频率，rad/s。 */
#define CHB_ENERGY_ZETA     1.0f          /* 能量环阻尼比，对应原 Kp=40、Ki=400。 */
#define CHB_CURRENT_OMEGA_N 763.7626158f  /* 电流环自然角频率，rad/s，保留原整定点。 */
#define CHB_CURRENT_ZETA    3.273268354f  /* 原整定为过阻尼；默认电感下约 Kp=60、Ki=7000。 */

/**
 * @brief PI 初始整定公式，供构建配置时计算，不在周期控制中重复求增益。
 * @details omega_n 为目标自然角频率（rad/s），zeta 为无量纲阻尼比。
 *          自然频率与实际交越频率不同，须结合离散滤波、延时和限幅校核。
 *          参数使用 float 且不带副作用；omega_n 会重复求值。
 *          调用方保证参数有限、omega_n/zeta 为正，生成的控制系数由 cfg 发布入口验证。
 */

/**
 * @brief 母线能量环 Kp = 2*zeta*omega_n，输出单位 1/s。
 * @details 对象近似为功率到电容能量的积分环节，前提是内环能快速跟随功率请求。
 *          C 已进入能量误差 0.5*C*(v_ref^2-v_bus^2)，此处不再乘除电容。
 */
#define CHB_ENERGY_PI_KP_CALC(omega_n, zeta) (2.0f * (zeta) * (omega_n))

/** @brief 母线能量环 Ki = omega_n^2，输出单位 1/s^2；模型前提同能量环 Kp。 */
#define CHB_ENERGY_PI_KI_CALC(omega_n) ((omega_n) * (omega_n))

/**
 * @brief 电流环 Kp = 2*zeta*omega_n*L，l_grid 单位 H，输出单位 V/A。
 * @details 按现有电阻补偿及 dq 解耦后的对象 1/(L*s) 推导，要求补偿准确且 L 为正。
 *          采样、正交观测和 PWM 延时未被此公式消除，必须用于校核目标频率。
 */
#define CHB_CURRENT_PI_KP_CALC(l_grid, omega_n, zeta) (2.0f * (zeta) * (omega_n) * (l_grid))

/** @brief 电流环 Ki = omega_n^2*L，输出单位 V/(A*s)；模型前提同电流环 Kp。 */
#define CHB_CURRENT_PI_KI_CALC(l_grid, omega_n) ((omega_n) * (omega_n) * (l_grid))

/**
 * @brief PLL Kp = 2*zeta*omega_n/Vm，v_grid_peak 单位 V，输出单位 rad/(V*s)。
 * @details 对应使用电压 Vq 作为误差的 PLL，与 pll 库的初始化公式一致。
 *          Vm 为正的设计电压峰值，额定 RMS 转峰值时乘 sqrt(2)。
 *          若 Vq 已按电压幅值归一化，则不再除以 Vm。
 */
#define CHB_PLL_PI_KP_CALC(v_grid_peak, omega_n, zeta) (2.0f * (zeta) * (omega_n) / (v_grid_peak))

/** @brief PLL Ki = omega_n^2/Vm，输出单位 rad/(V*s^2)；幅值约定同 PLL Kp。 */
#define CHB_PLL_PI_KI_CALC(v_grid_peak, omega_n) ((omega_n) * (omega_n) / (v_grid_peak))

/**
 * @brief 求闭环总等效延时，三个参数及输出单位均为 s。
 * @details t_sense 包含传感器、采样和可等效为纯延时的滤波延时；
 *          t_control 为采样结果到 PWM 寄存器生效的延时，包含计算及更新等待；
 *          t_pwm 为寄存器生效到平均输出响应的调制延时，各项不得重复计入。
 *          PLL 不经过 PWM 调制环节；MSOGI 等动态滤波不能全频段当成固定延时。
 */
#define CHB_LOOP_DELAY_CALC(t_sense, t_control, t_pwm) ((t_sense) + (t_control) + (t_pwm))

/**
 * @brief 按 omega_cross*t_loop_delay <= phase_delay_budget 求交越角频率上限，rad/s。
 * @details phase_delay_budget 为允许总延时消耗的相位（rad），须在对象及 PI 相位之外分配。
 *          调用方保证预算有限且为正、t_loop_delay 有限且大于 0；这只是延时约束，
 *          不能直接作为 omega_n，也不替代完整相位裕量及离散闭环校核。
 */
#define CHB_DELAY_CROSSOVER_MAX_CALC(phase_delay_budget, t_loop_delay) ((phase_delay_budget) / (t_loop_delay))

typedef struct chb_ctrl_cfg
{
    float t_ctrl_period; /* 控制和 PWM 周期，s。 */
    float f_grid;        /* 电网基波频率，Hz；相角由外部同步源提供。 */
    float l_grid;        /* 输入串联电感，H。 */
    float r_grid;        /* 输入串联电阻，ohm。 */
    float t_current_sample_delay; /* 电流采样相对电网相角的已知延迟，s，由应用配置。 */
    float v_grid_rms_nominal;     /* 已预充额定点电网基波有效值，V。 */
    float v_bus_ref;      /* 每级母线目标电压，V。 */
    float v_bus_ref_slew; /* 每级母线给定的最大变化速度，V/s。 */
    float f_bus_filter;   /* 母线电压一阶低通截止频率，Hz。 */
    float k_energy_p;     /* 每桥电容能量环比例增益，1/s。 */
    float k_energy_i;     /* 每桥电容能量环积分增益，1/s^2。 */
    float k_current_p;    /* dq 电流内环比例增益，V/A。 */
    float k_current_i;    /* dq 电流内环积分增益，V/(A s)。 */
    float v_current_integral_limit; /* dq 积分项绝对限幅，V。 */
    float k_power_limit_p;          /* 每桥负载功率限制环比例增益，无量纲。 */
    float k_power_limit_i;          /* 每桥负载功率限制环积分增益，1/s。 */
    float i_grid_peak_limit;        /* 有功电流参考峰值上限，A。 */
    float modulation_limit;         /* 每桥线性调制裕量，0..1。 */
    float c_bus;          /* 所有桥共用的单级直流电容，F。 */
    float pwr_load_limit; /* 所有桥共用的单级持续输出功率上限，W。 */
} chb_ctrl_cfg_t;

/** @return 当前 PLECS 调试基准的独立参数副本。 */
chb_ctrl_cfg_t chb_cfg_default(void);

/**
 * @brief INIT 锁定前发布完整控制参数。
 * @param p_cfg 待发布的参数，调用方在应用边界提供。
 * @return 1：有效且已发布；0：无效或已锁定。
 */
uint8_t chb_cfg_set_ctrl_cfg(const chb_ctrl_cfg_t *p_cfg);

/** @return INIT 已锁定的完整配置地址，运行中只读。 */
const chb_ctrl_cfg_t *chb_cfg_get_ctrl_cfg(void);

/** @return 1：控制参数有效；0：不允许进入待机。 */
uint8_t chb_cfg_is_ready(void);

/** @brief 仅供同模块 FSM 在 INIT 期间解除配置锁。 */
void chb_cfg_unlock(void);

/** @brief 仅供同模块 FSM 在 INIT 检查完成后锁定配置。 */
void chb_cfg_lock(void);

/**
 * @brief 设置应用开停机请求，不直接授予运行许可。
 * @param request 0：停机；1：请求运行。
 * @return 1：已接受；0：请求值非法。
 */
uint8_t chb_cfg_set_run_request(uint8_t request);

/** @return 0：停机请求；1：运行请求。 */
uint8_t chb_cfg_get_run_request(void);

#endif /* CHB_CFG_H */
