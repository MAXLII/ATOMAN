// SPDX-License-Identifier: MIT
/**
 * @file chb_fsm.h
 * @brief CHB INIT/IDLE/BUS_SOFT_START/RUN/FAULT state and run permission.
 * @details Numeric control configuration locks in INIT; only the FSM grants run permission.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_FSM_H
#define CHB_FSM_H

#include <stdint.h>

typedef enum chb_run_state
{
    CHB_RUN_STATE_INIT = 0,        /* 等待绑定、参数和应用保护配置。 */
    CHB_RUN_STATE_IDLE,            /* 已准备，但未授予运行许可。 */
    CHB_RUN_STATE_BUS_SOFT_START,  /* 等待三级直流母线预充完成。 */
    CHB_RUN_STATE_MAIN_RELAY_WAIT, /* 等待继电器两端电压匹配、闭合确认及控制延时。 */
    CHB_RUN_STATE_RUN,             /* 已进入运行状态。 */
    CHB_RUN_STATE_FAULT            /* 故障闭锁，等待明确清除。 */
} CHB_RUN_STATE_E;

/** @return FSM 当前状态，供采样及应用保护门控。 */
CHB_RUN_STATE_E chb_fsm_get_run_state(void);

/** @return 1：FSM 已授予运行许可；0：禁止发波。 */
uint8_t chb_fsm_run_allowed(void);

/**
 * @brief 在软起超时 FAULT 状态且已请求停机后，申请清除 FSM 故障。
 * @return 1：清除请求已接受；0：状态不符或仍请求运行。
 * @details 下次 FSM 任务返回 IDLE；应用电气保护闭锁由应用单独清除。
 */
uint8_t chb_fsm_clear_fault(void);

#endif /* CHB_FSM_H */
