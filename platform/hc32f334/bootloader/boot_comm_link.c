// SPDX-License-Identifier: MIT
/**
 * @file    boot_comm_link.c
 * @brief   Dedicated FRAME link for the HC32F334 bootloader.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Allocate a bootloader-only FRAME receive payload context
 *          - Bind USART2 receive and transmit callbacks to the FRAME parser
 *          - Keep the normal IAP communication context unchanged
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The Section link task polls the USART foreground path
 *          - The receive payload accepts the complete 0x0A data command
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

#include "boot_uart.h"
#include "comm.h"
#include "comm_addr.h"
#include "section.h"

#define HC32_BOOTLOADER_LINK_ID         0U
#define HC32_BOOTLOADER_RX_PAYLOAD_SIZE 1033U

static section_link_tx_func_t s_bootloader_tx = {
    .my_printf = hc32_boot_uart_printf,
    .tx_by_dma = hc32_boot_uart_tx,
};

DECLARE_COMM_CTX(s_bootloader_comm,
                 HC32_BOOTLOADER_RX_PAYLOAD_SIZE,
                 HOST_ADDR,
                 HC32_BOOTLOADER_LINK_ID);

static const section_link_handler_item_t s_handlers[] = {
    {.func = comm_run, .ctx = &s_bootloader_comm},
};

REG_LINK(HC32_BOOTLOADER_LINK_ID,
         s_bootloader_tx,
         hc32_boot_uart_rx_get_byte,
         s_handlers,
         sizeof(s_handlers) / sizeof(s_handlers[0]))
