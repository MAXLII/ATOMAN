// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   GD32E507 demo platform entry module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the 100 us platform tick
 *          - Discover and initialize linker-registered demo components
 *          - Run the Cortex-M SRTOS scheduler dispatch loop
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Interrupt sources are enabled by registered BSP initialization
 *          - Hardware access is contained in the GD32E507 BSP
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "gd32e50x.h"
#include "section.h"
#include "systick.h"

/**
 * @brief Initialize and run the GD32E507 demo firmware.
 * @return This function does not return.
 */
int main(void)
{
    SystemCoreClockUpdate(); /* Refresh the CMSIS clock value after SystemInit. */
    systick_config();        /* Start the 100 us scheduler time base. */
    section_init();          /* Initialize registered BSP, demo, and debug modules. */

    for (;;)
    {
        run_task(); /* Dispatch ready SRTOS tasks when the processor is idle. */
    }
}
