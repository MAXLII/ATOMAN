// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_comm_link.c
 * @brief   Dedicated FRAME communication link for the Zynq bootloader image.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Allocate a bootloader-only 1033-byte FRAME receive payload context
 *          - Bind PS UART byte receive and transmit callbacks to comm_run
 *          - Keep the normal IAP communication context and common TX pool unchanged
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Polling PS UART receive is advanced by the Section link task
 *          - Hardware access remains in bsp_usart
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_usart.h"
#include "comm.h"
#include "comm_addr.h"
#include "section.h"

#include <stddef.h>

#define ZYNQ_BOOTLOADER_LINK_ID 0u
#define ZYNQ_BOOTLOADER_RX_PAYLOAD_SIZE 1033u

static void bootloader_tx(char *p_data, int length)
{
    bsp_usart_dbg_tx(p_data, length);
}

static section_link_tx_func_t s_bootloader_tx = {
    .my_printf = bsp_usart_dbg_printf,
    .tx_by_dma = bootloader_tx,
};

DECLARE_COMM_CTX(s_bootloader_comm,
                 ZYNQ_BOOTLOADER_RX_PAYLOAD_SIZE,
                 HOST_ADDR,
                 ZYNQ_BOOTLOADER_LINK_ID);

static const section_link_handler_item_t s_handlers[] = {
    {.func = comm_run, .ctx = &s_bootloader_comm},
};

REG_LINK(ZYNQ_BOOTLOADER_LINK_ID,
         s_bootloader_tx,
         bsp_usart_dbg_rx_get_byte,
         s_handlers,
         sizeof(s_handlers) / sizeof(s_handlers[0]))
