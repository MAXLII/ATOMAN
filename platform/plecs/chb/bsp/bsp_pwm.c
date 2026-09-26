// SPDX-License-Identifier: MIT
/**
 * @file bsp_pwm.c
 * @brief Write CHB PWM duties and common enable to PLECS output ports.
 * @details The PLECS gate blocks generate the complementary leg and dead time.
 *          This module does not change the main relay output port.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "bsp_pwm.h"
#include "plecs.h"

_Static_assert(PLECS_OUTPUT_PWM_ENABLE == BSP_PWM_CELL_COUNT,
               "Three duty outputs must precede the PWM enable");

static float delayed_duty[BSP_PWM_CELL_COUNT] = {0.5f, 0.5f, 0.5f}; /* Previous control-cycle duties. */
static uint8_t delayed_enable; /* Previous control-cycle common enable. */
static uint32_t output_tick; /* PLECS 100 us tick of the last published frame. */
static uint8_t output_tick_valid; /* Whether this simulation has published a PWM frame. */

void bsp_pwm_set(const float p_duty[BSP_PWM_CELL_COUNT], uint8_t enable)
{
    const uint32_t tick = plecs_time_100us; /* Current 100 us control-cycle identifier. */

    if ((output_tick_valid != 0u) && (tick < output_tick))
    {
        for (uint32_t cell = 0u; cell < BSP_PWM_CELL_COUNT; ++cell)
        {
            delayed_duty[cell] = 0.5f;
        }
        delayed_enable = 0u;
        output_tick_valid = 0u;
    }

    if ((output_tick_valid == 0u) || (tick != output_tick))
    {
        for (uint32_t cell = 0u; cell < BSP_PWM_CELL_COUNT; ++cell)
        {
            plecs_set_output((PLECS_OUTPUT_E)cell, delayed_duty[cell]);
        }
        plecs_set_output(PLECS_OUTPUT_PWM_ENABLE, (float)delayed_enable);
        output_tick = tick;
        output_tick_valid = 1u;
    }

    for (uint32_t cell = 0u; cell < BSP_PWM_CELL_COUNT; ++cell)
    {
        if (enable == 0u)
        {
            delayed_duty[cell] = 0.5f;
        }
        else
        {
            delayed_duty[cell] = p_duty[cell];
        }
    }
    delayed_enable = (enable != 0u) ? 1u : 0u;
}
