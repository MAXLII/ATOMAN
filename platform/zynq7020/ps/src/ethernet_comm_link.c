// SPDX-License-Identifier: MIT
/**
 * @file    ethernet_comm_link.c
 * @brief   Zynq-7020 PS GEM0-to-Section communication link adapter.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register the PS GEM0 TCP stream as an independent Section link
 *          - Route received bytes to Shell and the existing 0xE8 communication parser
 *          - Poll the lwIP network interface from the cooperative task context
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - No Ethernet or lwIP processing occurs in the Section timer ISR
 *          - Hardware access is abstracted through bsp_ethernet
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

#include "bsp_ethernet.h"
#include "comm.h"
#include "comm_addr.h"
#include "comm_link.h"
#include "section.h"
#include "shell.h"

#include <stdint.h>

#define ZYNQ_ETHERNET_COMM_PAYLOAD_SIZE 2048U

static void ethernet_tx_callback(char *p_data, int length)
{
    if ((p_data != NULL) && (length > 0))
    {
        (void)bsp_ethernet_tx((const uint8_t *)p_data, (uint32_t)length);
    }
}

static section_link_tx_func_t s_ethernet_tx = {
    .my_printf = bsp_ethernet_printf,
    .tx_by_dma = ethernet_tx_callback,
};

DECLARE_SHELL_CTX(s_ethernet_shell_context);
DECLARE_COMM_CTX(s_ethernet_comm_context,
                 ZYNQ_ETHERNET_COMM_PAYLOAD_SIZE,
                 HOST_ADDR,
                 ETHERNET_LINK);

static const section_link_handler_item_t s_ethernet_handlers[] = {
    {.func = shell_run, .ctx = (void *)&s_ethernet_shell_context},
    {.func = comm_run, .ctx = (void *)&s_ethernet_comm_context},
};

static void ethernet_poll_task(void)
{
    bsp_ethernet_poll();
}

REG_LINK(ETHERNET_LINK,
         s_ethernet_tx,
         bsp_ethernet_rx_get_byte,
         s_ethernet_handlers,
         sizeof(s_ethernet_handlers) / sizeof(s_ethernet_handlers[0]))
REG_TASK_MS(1, ethernet_poll_task)
