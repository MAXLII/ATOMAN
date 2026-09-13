// SPDX-License-Identifier: MIT
/**
 * @file    npc_cfg.h
 * @brief   Application parameters, timing and unpublished control setpoints.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_CFG_H
#define NPC_CFG_H

#include <stdint.h>
#include <stdatomic.h>

/** 固定控制系数，由控制器初始化时复制。 */
typedef struct npc_ctrl_cfg
{
    float ts;                  /* 控制周期，s。 */
    float omega;               /* DSOGI 中心角频率，rad/s；内部不含 PLL。 */
    float sogi_k;              /* DSOGI 阻尼系数。 */
    float kp_v;                /* 4 个电压轴共用的比例增益，A/V。 */
    float ki_v;                /* 电压积分增益，A/(V·s)，计算时乘控制周期。 */
    float kp_i;                /* 4 个电流轴共用的比例增益，V/A。 */
    float ki_i;                /* 电流积分增益，V/(A·s)，计算时乘控制周期。 */
    float kaw_v;               /* 电压环限幅回算速率，1/s。 */
    float kaw_i;               /* 电流环限幅回算速率，1/s。 */
    float current_peak;        /* 正负序电流给定幅值之和的上限，A。 */
    float modulation_headroom; /* 总母线电压的可用比例，范围 (0,1]。 */
    float current_bias_cutoff_hz; /* 非基波电流低频偏置提取截止频率，Hz。 */
    float current_bias_resistance; /* 偏置电流的负反馈电压系数，ohm；0 关闭补偿。 */
    float voltage_damping_gain;    /* 电压非基波高通残差补偿增益，无量纲；0 关闭补偿。 */
    float voltage_damping_cutoff_hz; /* 残差高通的截止频率，Hz。 */
} npc_ctrl_cfg_t;

/* 690 V 线电压有效值对应的相电压峰值，V。 */
#define NPC_CFG_NOMINAL_PHASE_PEAK_V (563.382640840131f)
/* 从零建立额定电压的默认软启动时间，s。 */
#define NPC_CFG_REFERENCE_RAMP_S (2.0f)
/* 幅值给定上升和下降共用的默认斜率，V/s。 */
#define NPC_CFG_DEFAULT_VD_POS_SLEW_VPS (NPC_CFG_NOMINAL_PHASE_PEAK_V / NPC_CFG_REFERENCE_RAMP_S)

/** 应用层运行请求；只有状态机能够发布运行许可。 */
typedef struct npc_cfg
{
    float vd_pos_slew_vps;    /* 软启动斜率，V/s，必须为正有限数。 */
    float freq_hz;            /* DSOGI 中心频率，Hz；相位由 HAL 独立采样。 */
    float v_dc_half_min;      /* 单侧母线最低允许电压，V。 */
    atomic_uchar run_request; /* 开停机请求，0 停机、1 开机；中断也读取此值。 */
} npc_cfg_t;

/** 状态机发布、控制中断按完整快照读取的运行参数。 */
typedef struct npc_ctrl_setpoint
{
    float vd_pos_slew_vps; /* 已发布的软启动斜率，V/s。 */
    float freq_hz;        /* 已发布的 DSOGI 中心频率，Hz。 */
    float v_dc_half_min;  /* 已发布的单侧母线最低允许电压，V。 */
    uint8_t run_allowed; /* 运行许可，0 禁止、1 允许；仅由状态机授予。 */
} npc_ctrl_setpoint_t;

/** 对外运行状态，与故障诊断码分开。 */
typedef enum
{
    NPC_RUN_STA_INIT = 0, /* 初始化，等待依赖就绪。 */
    NPC_RUN_STA_IDLE,     /* 待机，尚未授予运行许可。 */
    NPC_RUN_STA_RUN       /* 运行，状态机正在维持运行许可。 */
} NPC_RUN_STA_E;

/**
 * @brief 返回与 MATLAB 设计对应的固定控制系数。
 * @return 独立的配置值副本，不包含控制动态。
 */
npc_ctrl_cfg_t npc_cfg_default(void);

/**
 * @brief 停机时设置控制周期，应在 section_init 前调用。
 * @param ctrl_ts 控制周期，范围 1e-6～0.01 s。
 * @return 1：参数已接受；0：参数被拒绝，原值保持。
 */
uint8_t npc_cfg_set_ctrl_ts(float ctrl_ts);

/** @return 当前控制周期，s。 */
float npc_cfg_get_ctrl_ts(void);

/** @return 1：频率与周期匹配 DSOGI 数值范围；0：配置未就绪。 */
uint8_t npc_cfg_is_ready(void);

/**
 * @brief 更新应用侧斜率请求，待状态机发布后生效。
 * @param value 幅值上升和下降共用的斜率，范围 0.001～1e6 V/s。
 * @return 1：已接受；0：拒绝并保留原值。
 */
uint8_t npc_cfg_set_vd_pos_slew_vps(float value);

/**
 * @brief 更新应用侧 DSOGI 频率请求。
 * @param value 频率，Hz；至少 1 Hz，且每拍电角度步进须在 0.001～1 rad 内。
 * @return 1：已接受；0：拒绝并保留原值。
 */
uint8_t npc_cfg_set_freq_hz(float value);

/**
 * @brief 更新应用侧单侧母线电压门限请求。
 * @param value 最低允许电压，范围 0.001～1e6 V。
 * @return 1：已接受；0：拒绝并保留原值。
 */
uint8_t npc_cfg_set_v_dc_half_min(float value);

/**
 * @brief 原子更新开停机请求，不直接授予运行许可。
 * @param request 0：请求停机；1：请求开机。
 * @return 1：已接受；0：请求值非法，原值保持。
 */
uint8_t npc_cfg_set_run_request(uint8_t request);

/** @return 0：停机请求；1：开机请求。 */
uint8_t npc_cfg_get_run_request(void);

/** @return 状态机当前对外运行状态。 */
NPC_RUN_STA_E npc_cfg_get_run_state(void);

/**
 * @brief 从应用参数构建待发布快照，运行许可保持为 0。
 * @return 模块持有的静态快照地址；仅供状态机串行调用，下一次调用会更新内容。
 */
const npc_ctrl_setpoint_t *npc_cfg_get_p_building(void);

#endif /* NPC_CFG_H */
