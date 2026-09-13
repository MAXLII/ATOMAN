// SPDX-License-Identifier: MIT
/**
 * @file    npc_ctrl.h
 * @brief   Control lifecycle and read-only monitoring; periodic computation is registered internally.
 * @details Base digital power framework. C11, static storage, no allocation.
 *          Bindings change only while stopped. PLECS serializes the dispatcher;
 *          ISR setpoints use the FSM publication snapshot.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_CTRL_H
#define NPC_CTRL_H

#include <stdbool.h>
#include <stdint.h>

/** 采样层写入、应用保护层只读的本拍快照；vd_pos_ref 由控制层在计算前填写。 */
typedef struct npc_ctrl_sample
{
    float v_out[3];   /* 输出相电压 A/B/C，V。 */
    float i_l[3];     /* 电感电流 A/B/C，A，桥臂流向输出为正。 */
    float v_dc_p;     /* 正侧母线电压，V。 */
    float v_dc_n;     /* 负侧母线电压幅值，V。 */
    float theta;      /* 本拍外部电角度，rad。 */
    float vd_pos_ref; /* 控制层实际采用的软启动幅值，V。 */
} npc_ctrl_sample_t;

/** 一次控制计算得到的完整输出，数组轴序统一为 d+、q+、d-、q-。 */
typedef struct npc_ctrl_output
{
    float v_alpha;        /* 送入 SVPWM 的 alpha 轴电压指令，V。 */
    float v_beta;         /* 送入 SVPWM 的 beta 轴电压指令，V。 */
    float v_dq[4];        /* 正负序电压反馈，V。 */
    float i_dq[4];        /* 正负序电流反馈，A。 */
    float i_ref[4];       /* 限幅后的正负序电流给定，A。 */
    float u_dq[4];        /* 限幅后的正负序电压指令，V。 */
    bool current_limited; /* 本拍是否触发电流给定联合限幅。 */
    bool voltage_limited; /* 本拍是否触发调制电压跨度限幅。 */
    bool valid;           /* 本拍结果是否有效；复位和失败时为 false。 */
    float i_fundamental[3]; /* 本拍 DSOGI 正负序合成的 A/B/C 基波电流，供中点平衡使用，A。 */
    float i_bias_ab[2];     /* 非基波电流残差中的低频 alpha/beta 偏置估计，A。 */
    float v_damping_ab[2];  /* 调制限幅前的 alpha/beta 电压高通残差补偿，V。 */
} npc_ctrl_output_t;

/** 与平台 Shell 诊断保持兼容的状态值。 */
typedef enum
{
    NPC_CTRL_OFF = 1,        /* 未获得运行许可或收到停机请求。 */
    NPC_CTRL_RUNNING = 2,    /* 控制计算与 PWM 回调均成功。 */
    NPC_CTRL_REFERENCE = 3,  /* 兼容已有诊断编号；控制算法不再检查幅值和相位范围。 */
    NPC_CTRL_INPUT = 5,      /* 兼容已有诊断编号；控制算法不再检查采样有限性。 */
    NPC_CTRL_BUS = 6,        /* 单侧母线低于允许门限。 */
    NPC_CTRL_CONTROL = 7,    /* 控制实例未就绪或观测器失败。 */
    NPC_CTRL_PWM = 8,        /* PWM 回调拒绝本拍输出。 */
    NPC_CTRL_BINDING = 10,   /* 等待 INIT 完成 HAL 绑定检查。 */
    NPC_CTRL_OVERCURRENT = 12 /* 采样过流已闭锁。 */
} NPC_CTRL_STATUS_E;

/** 控制器持有的诊断副本，不挂载输入，也不用于反向修改控制状态。 */
typedef struct npc_ctrl_monitor
{
    npc_ctrl_output_t output; /* 最近一次控制结果。 */
    float integral_v[4];      /* 电压积分输出副本，A。 */
    float integral_i[4];      /* 电流积分输出副本，V。 */
    float vd_pos_ref_act;     /* 本拍实际采用的软启动给定，V。 */
    uint32_t status;          /* 控制诊断码，取值见 NPC_CTRL_STATUS_E；保留协议存储宽度。 */
    uint32_t detail;          /* 输入序号、故障相序号或 PWM 回调错误码。 */
} npc_ctrl_monitor_t;

/** @brief 准备运行所需的配置和动态；调用方须与控制中断串行执行。 */
void npc_ctrl_prepare_run(void);

/** @brief 直接关闭 PWM 并清除控制动态；调用方须已完成 HAL 绑定；不清除保护闭锁。 */
void npc_ctrl_stop(void);

/**
 * @brief 获取控制器拥有的诊断副本，应在控制中断结束后串行读取。
 * @return 静态诊断对象的只读地址；后续控制计算会更新其内容。
 */
const npc_ctrl_monitor_t *npc_ctrl_get_monitor(void);

/**
 * @brief 读取优先级 1 采样阶段生成的快照；应用保护应以优先级 2 注册。
 * @return 控制器持有的只读快照地址；INIT 完成且采样阶段执行后有效。
 */
const npc_ctrl_sample_t *npc_ctrl_get_sample(void);

/**
 * @brief 应用层撤销本拍控制许可、停止 PWM 并发布原因；不会修改应用保护闭锁。
 * @param status 应用层确定的诊断状态码。
 * @param detail 故障相序号或其他诊断明细。
 */
void npc_ctrl_inhibit(uint32_t status, uint32_t detail);

#endif /* NPC_CTRL_H */
