// SPDX-License-Identifier: MIT
/**
 * @file chb_hal.c
 * @brief Own CHB physical-input, PWM and relay bindings.
 * @details Binding locks after FSM INIT; chb_ctrl.c reads the bound inputs once per control tick.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_hal.h"
#include <stddef.h>
#include <stdatomic.h>

static chb_ctrl_hal_t binding        = {0};                 /* 平台挂载的长寿命输入及 PWM 回调。 */
static atomic_uchar   binding_locked = ATOMIC_VAR_INIT(0u); /* INIT 后禁止换源。 */

uint8_t chb_hal_bind(const chb_ctrl_hal_t *p_binding)
{
    if (    (p_binding == NULL)
         || (atomic_load(&binding_locked) != 0u))
    {
        return 0u;
    }
    binding = *p_binding; /* 绑定对象复制一次；采样源地址仍由平台拥有。 */
    return 1u;
}

uint8_t chb_hal_is_ready(void)
{
    if (    (binding.p_grid_v == NULL)
         || (binding.p_i_alpha_a == NULL)
         || (binding.p_pwm_disable == NULL)
         || (binding.p_soft_start_relay_on == NULL)
         || (binding.p_soft_start_relay_off == NULL)
         || (binding.p_main_relay_on == NULL)
         || (binding.p_main_relay_off == NULL))
    {
        return 0u;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (    (binding.p_bus_v[cell] == NULL)
             || (binding.p_load_i_a[cell] == NULL)
             || (binding.p_set_pwm_func[cell] == NULL))
        {
            return 0u;
        }
    }
    return 1u;
}

void chb_hal_unlock_binding(void)
{
    atomic_store(&binding_locked, 0u);
}

void chb_hal_lock_binding(void)
{
    atomic_store(&binding_locked, 1u);
}

const chb_ctrl_hal_t *chb_hal_get_ctrl(void)
{
    return &binding;
}
