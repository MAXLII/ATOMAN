// SPDX-License-Identifier: MIT
/**
 * @file    comm_link.c
 * @brief   F28P55 SCIA CommLink binding.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind the XDS110 user UART to one SECTION Link
 *          - Feed received logical octets into the shared COMM parser
 *          - Copy complete binary FRAME packets into the SCIA transmit queue
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - RX dequeue and protocol parsing run in cooperative task context
 *          - Queue-copy TX releases the shared COMM buffer after the callback returns
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "comm.h"
#include "comm_addr.h"
#include "section.h"

#define F28P55_COMM_PAYLOAD_SIZE (128u)

typedef enum
{
    F28P55_SCIA_LINK = 0
} f28p55_link_id_t;

static void f28p55_scia_tx_callback(char *p_data, int length)
{
    if ((p_data == NULL) || (length <= 0))
    {
        return;
    }

    tms320f28p55_uart_tx_write((const uint8_t *)p_data, (uint16_t)length);
}

static section_link_tx_func_t s_f28p55_scia_tx = {
    .my_printf = NULL,
    .tx_by_dma = f28p55_scia_tx_callback,
};

DECLARE_COMM_CTX(s_f28p55_scia_comm,
                 F28P55_COMM_PAYLOAD_SIZE,
                 HOST_ADDR,
                 F28P55_SCIA_LINK);

static const section_link_handler_item_t s_f28p55_scia_handlers[] = {
    {.func = comm_run, .ctx = (void *)&s_f28p55_scia_comm},
};

REG_LINK(F28P55_SCIA_LINK,
         s_f28p55_scia_tx,
         tms320f28p55_uart_rx_get_byte,
         s_f28p55_scia_handlers,
         sizeof(s_f28p55_scia_handlers) / sizeof(s_f28p55_scia_handlers[0]))
