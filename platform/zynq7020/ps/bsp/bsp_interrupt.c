// SPDX-License-Identifier: MIT
/**
 * @file    bsp_interrupt.c
 * @brief   Zynq-7020 shared GIC service implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Configure the Cortex-A9 generic interrupt controller
 *          - Install the common IRQ exception dispatcher
 *          - Provide shared handler registration for timer and PL peripherals
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Handler registration is performed outside ISR context
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

#include "bsp_interrupt.h"

#include "xparameters.h"
#include "xscugic.h"
#include "xstatus.h"

#include <stddef.h>
#include <stdint.h>

static XScuGic s_interrupt_controller; /* Shared Cortex-A9 GIC instance. */
static uint8_t s_interrupt_initialized = 0U; /* GIC initialization state. */

int32_t bsp_interrupt_init(void)
{
    XScuGic_Config *config = NULL; /* Xilinx GIC hardware description. */
    int32_t status = XST_FAILURE;  /* Driver initialization result. */

    if (s_interrupt_initialized != 0U)
    {
        return XST_SUCCESS;
    }

    config = XScuGic_LookupConfig(XPAR_SCUGIC_SINGLE_DEVICE_ID);
    if (config == NULL)
    {
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(&s_interrupt_controller,
                                   config,
                                   config->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 &s_interrupt_controller);
    s_interrupt_initialized = 1U;

    return XST_SUCCESS;
}

int32_t bsp_interrupt_connect(uint32_t interrupt_id,
                              Xil_ExceptionHandler handler,
                              void *callback_ref)
{
    if ((s_interrupt_initialized == 0U) || (handler == NULL))
    {
        return XST_FAILURE;
    }

    return XScuGic_Connect(&s_interrupt_controller,
                           interrupt_id,
                           handler,
                           callback_ref);
}

void bsp_interrupt_enable(uint32_t interrupt_id)
{
    if (s_interrupt_initialized != 0U)
    {
        XScuGic_Enable(&s_interrupt_controller, interrupt_id);
    }
}

void bsp_interrupt_global_enable(void)
{
    if (s_interrupt_initialized != 0U)
    {
        Xil_ExceptionEnableMask((uint32_t)XIL_EXCEPTION_IRQ);
    }
}
