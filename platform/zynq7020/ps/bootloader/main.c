// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   Zynq-7020 bootloader runtime entry point.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the Section time base and runtime port
 *          - Start the periodic Section interrupt source
 *          - Run registered services from the foreground loop
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Bootloader services initialize through Section registration
 *          - Platform composition remains outside the entry point
 *
 * @author  Max.Li
 * @date    2026-07-29
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_timer.h"
#include "section.h"
#include "xstatus.h"

int main(void)
{
    bsp_timer_init();    /* Establish the monotonic time base used by Section. */
    section_port_init(); /* Initialize the selected Section runtime port. */
    section_init();      /* Initialize services registered in linker sections. */

    if (bsp_timer_interrupt_start(10000u) != XST_SUCCESS)
    {
        for (;;)
        {
        }
    }

    for (;;)
    {
        run_task(); /* Advance all registered foreground services. */
    }
}
