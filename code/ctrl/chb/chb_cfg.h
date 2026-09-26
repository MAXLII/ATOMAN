// SPDX-License-Identifier: MIT
/**
 * @file chb_cfg.h
 * @brief CHB rectifier control parameters and application run request.
 * @details Three-cell, single-phase rectifier; parameters are fixed after FSM INIT.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_CFG_H
#define CHB_CFG_H

#include <stdint.h>

#define CHB_CELL_COUNT               3u     /* 三个串联 H 桥，直流母线相互独立。 */
#define CHB_MAIN_RELAY_CLOSE_TIME_S  0.008f /* rly_on 使用的继电器机械闭合时间。 */
#define CHB_MAIN_RELAY_WAIT_MS       20u    /* rly_on 确认闭合后至控制运行的等待时间。 */
#define CHB_MAIN_RELAY_MATCH_RATIO   0.02f  /* 继电器两端压差允许值占电网峰值的比例。 */
#define CHB_BUS_NOTCH_BANDWIDTH_HZ   20.0f  /* 母线二倍工频陷波带宽，Hz。 */
#define CHB_BALANCE_CURRENT_MIN_PK_A 0.5f   /* 轻载功率投影的最小电流，A；低于此值须验证开关纹波影响。 */
#define CHB_PF_Q_SLEW_A_PER_S        600.0f /* 最小无功参考的升降变化率，A/s。 */
#define CHB_PF_VOLTAGE_RESERVE_V     0.0f   /* 规划附加电压余量，V；逐拍分配仍按实时母线限幅。 */
#define CHB_PF_PLAN_TICKS            10u    /* 100 us 控制周期下每 1 ms 重新求可行无功。 */
#define CHB_PF_SCAN_STEPS            32u    /* 在电流额定圆内寻找第一个可行区间。 */
#define CHB_PF_REFINE_STEPS          16u    /* 对第一个不可行/可行边界做有界二分。 */

typedef struct chb_ctrl_cfg
{
    float ts;         /* 控制和 PWM 周期，s。 */
    float grid_hz;    /* 电网基波频率，Hz；相角由外部同步源提供。 */
    float l_grid_h;   /* 输入串联电感，H。 */
    float r_grid_ohm; /* 输入串联电阻，ohm。 */
    float current_sample_delay_s; /* 电流采样相对电网相角的已知延迟，s，由应用配置。 */
    float grid_rms_nominal_v;     /* 已预充额定点电网基波有效值，V。 */
    float bus_ref_v;            /* 每级母线目标电压，V。 */
    float bus_ref_ramp_v_per_s; /* 每级母线给定的最大变化速度，V/s。 */
    float bus_filter_hz;        /* 母线电压一阶低通截止频率，Hz。 */
    float energy_kp;            /* 每桥电容能量环比例增益，1/s。 */
    float energy_ki;            /* 每桥电容能量环积分增益，1/s^2。 */
    float current_kp;           /* dq 电流内环比例增益，V/A。 */
    float current_ki;           /* dq 电流内环积分增益，V/(A s)。 */
    float current_integral_limit_v; /* dq 积分项绝对限幅，V。 */
    float power_limit_kp;           /* 每桥负载功率限制环比例增益，无量纲。 */
    float power_limit_ki;           /* 每桥负载功率限制环积分增益，1/s。 */
    float current_limit_pk_a;       /* 有功电流参考峰值上限，A。 */
    float modulation_limit;         /* 每桥线性调制裕量，0..1。 */
    float bus_capacitance_f[CHB_CELL_COUNT];    /* 每桥直流电容，F。 */
    float output_power_limit_w[CHB_CELL_COUNT]; /* 每桥持续输出功率上限，W。 */
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
