// SPDX-License-Identifier: MIT
/**
 * @file pwm.c
 * @brief CHB voltage normalization and 0..1 duty mapping.
 * @details The RUN-stage protection guarantees a valid positive DC bus before
 *          this callback. The downstream PLECS model applies carrier phase
 *          shifts, complementary gates and dead time.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "pwm.h"
#include "bsp_pwm.h"
#include <math.h>

_Static_assert(CHB_CELL_COUNT == BSP_PWM_CELL_COUNT, "CHB and BSP cell counts must match");

static float duty[CHB_CELL_COUNT] = {0.5f, 0.5f, 0.5f};
static uint32_t updated_mask;
static uint8_t frame_valid = 1u;

/** @brief PLECS 收齐三个通道后自动整体加载；硬件平台由定时器更新事件完成同样动作。 */
static void pwm_publish_complete_frame(void)
{
    const uint32_t complete_mask = ((uint32_t)1u << CHB_CELL_COUNT) - 1u;

    if (updated_mask == complete_mask)
    {
        bsp_pwm_set(duty, frame_valid);
        updated_mask = 0u;
        frame_valid = 1u;
    }
}

void chb_pwm_set_cell(uint32_t cell, float v_pwm_v, float bus_v,
                      chb_pwm_deadtime_flag_t deadtime_flag)
{
    if (cell >= CHB_CELL_COUNT)
    {
        return;
    }
    updated_mask |= (uint32_t)1u << cell;
    if (    (!isfinite(v_pwm_v))
         || (!isfinite(bus_v))
         || (bus_v <= 0.0f)
         || ((deadtime_flag != CHB_PWM_DEADTIME_NEGATIVE)
             && (deadtime_flag != CHB_PWM_DEADTIME_OFF)
             && (deadtime_flag != CHB_PWM_DEADTIME_POSITIVE)))
    {
        duty[cell] = 0.5f;
        frame_valid = 0u;
        pwm_publish_complete_frame();
        return;
    }
    const float compensated = bsp_pwm_deadtime_compensate(v_pwm_v / bus_v, (int8_t)deadtime_flag);
    const float modulation = fminf(1.0f, fmaxf(-1.0f, compensated));
    duty[cell] = 0.5f * (modulation + 1.0f);
    pwm_publish_complete_frame();
}

void chb_pwm_disable(void)
{
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        duty[cell] = 0.5f;
    }
    updated_mask = 0u;
    frame_valid = 1u;

    bsp_pwm_set(duty, 0u);
}
