// SPDX-License-Identifier: MIT
/**
 * @file    bsp_interrupt.h
 * @brief   Zynq-7020 shared GIC service interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the Cortex-A9 generic interrupt controller once
 *          - Share one GIC instance between the private timer and PL devices
 *          - Connect and enable BSP interrupt handlers through a small API
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Registration is completed before global IRQ enable
 *          - Hardware access is abstracted through the Xilinx standalone BSP
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_INTERRUPT_H
#define BSP_INTERRUPT_H

#include "xil_exception.h"

#include <stdint.h>

int32_t bsp_interrupt_init(void);
int32_t bsp_interrupt_connect(uint32_t interrupt_id,
                              Xil_ExceptionHandler handler,
                              void *callback_ref);
void bsp_interrupt_enable(uint32_t interrupt_id);
void bsp_interrupt_global_enable(void);

#endif /* BSP_INTERRUPT_H */
