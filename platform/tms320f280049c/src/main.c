// SPDX-License-Identifier: MIT
/**
 * @file    main.c
 * @brief   TMS320F280049C SECTION demo entry point.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the F280049C device and interrupt controller
 *          - Initialize the platform services and discover SECTION records
 *          - Execute the cooperative SECTION task dispatcher continuously
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Interrupts start after all SECTION registrations are initialized
 *          - Hardware access is abstracted through C2000Ware and the platform port
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "section.h"

void main(void)
{
    tms320f280049c_device_init();
    Interrupt_initModule();
    Interrupt_initVectorTable();

    tms320f280049c_platform_init();
    section_init();

    EINT;
    ERTM;

    for (;;)
    {
        run_task();
    }
}
