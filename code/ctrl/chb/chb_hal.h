// SPDX-License-Identifier: MIT
/**
 * @file chb_hal.h
 * @brief CHB physical samples, three-bridge PWM and relay binding contract.
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

typedef enum
{
    CHB_PWM_DEADTIME_NEGATIVE = -1,
    CHB_PWM_DEADTIME_OFF      = 0,
    CHB_PWM_DEADTIME_POSITIVE = 1,
} chb_pwm_deadtime_flag_t;

typedef void (*chb_set_pwm_func_t)(float v_pwm_v, float bus_v,
                                   chb_pwm_deadtime_flag_t deadtime_flag);

typedef struct chb_ctrl_hal
{
    const float       *p_grid_v;                   /* 电网侧 ADC 瞬时电压，V。 */
    const float       *p_i_alpha_a;                /* ADC 网侧电流，A。 */
    const float       *p_bus_v[CHB_CELL_COUNT];    /* 各级 ADC 母线电压，V。 */
    const float       *p_load_i_a[CHB_CELL_COUNT]; /* 各级负载电流采样，A。 */
    chb_set_pwm_func_t p_set_pwm_func[CHB_CELL_COUNT];  /* 逐级写入最终调制电压和同拍母线电压。 */
    void               (*p_pwm_disable)(void);          /* 同步关闭全部桥臂。 */
    void               (*p_soft_start_relay_on)(void);  /* 闭合母线软起继电器。 */
    void               (*p_soft_start_relay_off)(void); /* 断开母线软起继电器。 */
    void               (*p_main_relay_on)(void);        /* 闭合主继电器。 */
    void               (*p_main_relay_off)(void);       /* 断开主继电器。 */
} chb_ctrl_hal_t;

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

#endif /* CHB_HAL_H */
