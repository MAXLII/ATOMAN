// SPDX-License-Identifier: MIT
/**
 * @file chb_hal.h
 * @brief CHB sampled-input and three-bridge PWM binding contract.
 * @details Input pointers remain valid through RUN; PWM callbacks consume commands synchronously.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_HAL_H
#define CHB_HAL_H

#include "chb_cfg.h"
#include <stdint.h>

typedef struct chb_hal_sample
{
    float grid_v;      /* 主继电器电网侧瞬时电压，V。 */
    float input_cap_v; /* 主继电器变流器侧电压，当前无输入电容时由上层提供 0 V。 */
    float grid_rms_v;  /* 电网基波有效值，V。 */
    float grid_hz;     /* 电网测得的基波频率，Hz，供继电器同步使用。 */
    float theta_rad;   /* 锁相角，rad；d 轴对齐电网电压。 */
    float i_alpha_a;   /* 物理网侧电流，流向级联桥为正，A。 */
    float i_beta_a;    /* 同拍正交基波电流，A；由外部观测器产生。 */
    float bus_v[CHB_CELL_COUNT];    /* 各桥直流母线电压，V。 */
    float load_i_a[CHB_CELL_COUNT]; /* 各桥负载净电流，流向负载为正，不含电容电流，A。 */
} chb_hal_sample_t;

typedef struct chb_pwm_command
{
    float v_pwm_v[CHB_CELL_COUNT];  /* 本拍各桥端口瞬时电压参考，V。 */
    float bus_v[CHB_CELL_COUNT];    /* 与端口指令同拍的各桥母线电压，V。 */
    float cell_d_v[CHB_CELL_COUNT]; /* 各桥直接分配后的 d 轴电压，V 峰值。 */
    float cell_q_v[CHB_CELL_COUNT]; /* 各桥直接分配后的 q 轴电压，V 峰值。 */
    float total_v_pwm_v;            /* 各桥命令之和，V。 */
    float total_d_v;         /* 总桥基波 d 轴电压，供诊断或正交仿真，V。 */
    float total_q_v;         /* 总桥基波 q 轴电压，供诊断或正交仿真，V。 */
    float i_comp_ref_a;      /* 逆 Park 后的基波电流给定，避免过零时采样纹波使补偿抖动，A。 */
    float i_comp_beta_ref_a; /* 电流参考正交分量，用于补偿采样和发波延迟，A。 */
    float id_ref_a;          /* 本拍有功电流幅值给定，用于应用层诊断记录，A 峰值。 */
    float theta_rad;         /* 本拍电网相角，PWM 接口据此预测各桥实际作用时刻。 */
} chb_pwm_command_t;

typedef struct chb_ctrl_hal
{
    const float *p_grid_v;      /* 电网侧 ADC 瞬时电压，V。 */
    const float *p_input_cap_v; /* 继电器变流器侧电压；无输入电容时指向上层 0 V 变量。 */
    const float *p_grid_rms_v;  /* PLL/电网观测器输出，V RMS。 */
    const float *p_grid_hz;     /* PLL/电网观测器输出的实时频率，Hz。 */
    const float *p_theta_rad;   /* 同一采样时刻的相角，rad。 */
    const float *p_i_alpha_a;   /* ADC 网侧电流，A。 */
    const float *p_i_beta_a;    /* 外部正交电流观测器输出，A。 */
    const float *p_bus_v[CHB_CELL_COUNT];    /* 3 路 ADC 母线电压，V。 */
    const float *p_load_i_a[CHB_CELL_COUNT]; /* 3 路负载电流采样，A。 */
    void         (*p_set_pwm_func)(const chb_pwm_command_t *p_command); /* 同步消费本拍命令，不留存指针。 */
    void         (*p_pwm_disable)(void);            /* 同步关闭全部桥臂。 */
    void         (*p_soft_start_relay_close)(void); /* 闭合母线软起继电器。 */
    void         (*p_soft_start_relay_open)(void);  /* 断开母线软起继电器。 */
    void         (*p_main_relay_close)(void);       /* 闭合主继电器。 */
    void         (*p_main_relay_open)(void);        /* 断开主继电器。 */
} chb_ctrl_hal_t;

typedef struct chb_fsm_hal
{
    void (*p_enter_run_func)(void);         /* 运行许可发出前准备积分与滤波状态。 */
    void (*p_exit_run_func)(void);          /* 退出运行时关闭全部 PWM。 */
    void (*p_soft_start_relay_close)(void); /* 软起阶段接通预充支路。 */
    void (*p_soft_start_relay_open)(void);  /* 主继电器确认闭合或停机、故障时断开预充支路。 */
    void (*p_main_relay_close)(void);       /* 预充完成后接通主支路。 */
    void (*p_main_relay_open)(void);        /* 停机或故障时断开主支路。 */
} chb_fsm_hal_t;

/**
 * @brief 在 INIT 锁定前一次挂载所有采样源和发波动作。
 * @param p_binding 平台拥有的输入地址和回调；地址在整个 RUN 期有效。
 * @return 1：已挂载；0：无效或已锁定。
 */
uint8_t chb_hal_bind(const chb_ctrl_hal_t *p_binding);

/** @return 1：所有采样和发波依赖已挂载。 */
uint8_t chb_hal_is_ready(void);

/** @brief 仅供 FSM 在 INIT 期间解锁绑定。 */
void chb_hal_unlock_binding(void);

/** @brief 仅供 FSM 完成 INIT 检查后锁定绑定。 */
void chb_hal_lock_binding(void);

/** @return 已锁定的绑定地址；有效运行阶段不再逐次检查指针。 */
const chb_ctrl_hal_t *chb_hal_get_ctrl(void);

/** @return INIT 检查通过后使用的生命周期动作。 */
const chb_fsm_hal_t *chb_hal_get_fsm(void);

/** @brief 中断采样阶段一次复制全部输入。 */
void chb_hal_sample(void);

/** @return 保护与控制共用的本拍只读采样。 */
const chb_hal_sample_t *chb_hal_get_sample(void);

/** @brief 下发停波并闭锁应用故障；实际输出延时由平台 BSP 决定。 */
void chb_hal_trip(void);

/** @brief 仅在明确停机及故障消失后清除闭锁。 */
void chb_hal_clear_trip(void);

/** @return 1：保护已闭锁；0：无闭锁。 */
uint8_t chb_hal_is_tripped(void);

#endif /* CHB_HAL_H */
