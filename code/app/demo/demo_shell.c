// SPDX-License-Identifier: MIT
/**
 * @file    demo_shell.c
 * @brief   shell section demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register shell variables with SECTION_SHELL
 *          - Register one shell command with SECTION_SHELL
 *          - Keep shell state local to this demo file
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-18
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "shell.h"

#include <stdint.h>

static uint32_t s_demo_shell_counter = 0u;
static float s_demo_shell_gain = 1.0f;

static void demo_shell_counter_changed(DEC_MY_PRINTF)
{
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("shell counter=%lu\r\n", (unsigned long)s_demo_shell_counter);
    }
}

static void demo_shell_ping_cmd(DEC_MY_PRINTF)
{
    s_demo_shell_counter++;

    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("shell ping: counter=%lu gain=%f\r\n",
                             (unsigned long)s_demo_shell_counter,
                             (double)s_demo_shell_gain);
    }
}

REG_SHELL_VAR(DEMO_SHELL_COUNTER, s_demo_shell_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, demo_shell_counter_changed, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_SHELL_GAIN, s_demo_shell_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
REG_SHELL_CMD(DEMO_SHELL_PING, demo_shell_ping_cmd)
