// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief USART1 Link adapter.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Connect the BSP transport to shared Shell, E8 COMM, and E9 COMM v1 parsers
 *          Design notes:
 *          - C11 compatible; no dynamic allocation in platform drivers
 *          - Hardware access belongs to the BSP; framework bindings belong to src
 * @author Max.Li
 * @date 2026-10-14
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li.
 *          All rights reserved.
 *          This file is licensed under the MIT License.
 *          See the LICENSE file in the project root for full license text.
 */
#include "bsp_usart.h"
#include "comm.h"
#include "shell.h"
#define STM32_USART1_LINK 0u /* Physical USART1 is the single debug Link. */
#define STM32_LOCAL_ADDR  2u /* Match the existing demo host address. */
static section_link_tx_func_t uart_tx = {
    /* Asynchronous output callbacks. */
    .my_printf = bsp_usart_dbg_printf,
    .tx_by_dma = bsp_usart_dbg_tx,
};
DECLARE_SHELL_CTX(uart_shell);
DECLARE_COMM_CTX(uart_comm, 256u, STM32_LOCAL_ADDR, STM32_USART1_LINK);
DECLARE_COMM_V1_CTX(uart_comm_v1, STM32_LOCAL_ADDR, STM32_USART1_LINK);
typedef enum
{
    UART_PARSE_IDLE = 0, /* Select a parser at a frame or text-line boundary. */
    UART_PARSE_SHELL,    /* Keep binary-looking bytes inside the current Shell line. */
    UART_PARSE_E8,       /* Keep the whole legacy frame in its own parser. */
    UART_PARSE_E9,       /* Keep the whole COMM v1 frame in its own parser. */
} UART_PARSE_E;
static UART_PARSE_E parser   = UART_PARSE_IDLE; /* Only the Link task accesses parser ownership. */
static uint32_t last_rx_tick = 0u; /* Reset a truncated binary frame after the shared 100 ms timeout. */
static uint32_t wire_index   = 0u; /* Byte offset inside the selected binary frame. */
static uint32_t wire_size    = 0u; /* Complete wire length after the header declares it. */
static uint32_t payload_size = 0u; /* E8 little-endian length accumulator. */
static uint8_t rejected      = 0u; /* Drain rejected frames without interpreting their payload as Shell input. */

/**
 * @brief Preserve parser ownership across each complete binary frame or text line.
 * @param data Next received byte.
 * @param my_printf Link output callbacks.
 * @param p_ctx Unused because this single board Link owns its static contexts.
 * @details Parallel byte dispatch lets a payload or checksum SOP poison the other
 *          parser's next frame. Select ownership here without changing shared parsers.
 */
static void uart_receive(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    uint32_t now = SECTION_SYS_TICK; /* Current monotonic scheduler time. */
    (void)p_ctx;

    if (    (parser != UART_PARSE_IDLE) /* Only an incomplete transaction needs recovery. */
         && ((now - last_rx_tick) > COMM_V1_FRAME_TIMEOUT_TICK)) /* Honor the common binary timeout. */
    {
        parser = UART_PARSE_IDLE;
    }
    last_rx_tick = now;

    if (parser == UART_PARSE_IDLE)
    {
        wire_index   = 0u;
        wire_size    = 0u;
        payload_size = 0u;
        rejected     = 0u;

        if (data == 0xE8u)
        {
            parser = UART_PARSE_E8;
        }
        else if (data == COMM_V1_SOP)
        {
            parser = UART_PARSE_E9;
        }
        else
        {
            parser = UART_PARSE_SHELL;
        }
    }

    if (parser == UART_PARSE_E8)
    {
        if (wire_index == 9u)
        {
            payload_size = data;
        }

        if (wire_index == 10u)
        {
            wire_size = 15u + payload_size + ((uint32_t)data << 8u);
        }

        if (rejected == 0u)
        {
            comm_run(data, my_printf, &uart_comm);

            if (uart_comm.status == SECTION_PACKFORM_STA_SOP)
            {
                rejected = 1u;
            }
        }
    }
    else if (parser == UART_PARSE_E9)
    {
        if (    (wire_index == 4u)      /* Last MSG byte declares pure-command frames. */
             && ((data & 0x40u) != 0u)) /* CMD frames omit LEN and DATA. */
        {
            wire_size = COMM_V1_PURE_CMD_LEN;
        }

        if (    (wire_index == 5u) /* Data frames carry LEN after MSG. */
             && (wire_size == 0u)) /* Pure-command frames already have their size. */
        {
            wire_size = 7u + ((data == 0u) ? COMM_V1_MAX_DATA : (uint32_t)data);
        }

        if (rejected == 0u)
        {
            comm_v1_run(data, my_printf, &uart_comm_v1);

            if (uart_comm_v1.status == COMM_V1_STA_IDLE)
            {
                rejected = 1u;
            }
        }
    }
    else
    {
        shell_run(data, my_printf, &uart_shell);

        if (data == (uint8_t)'\n')
        {
            parser = UART_PARSE_IDLE;
        }
        return;
    }
    wire_index++;

    if (    (wire_size != 0u)          /* The binary header has a complete wire length. */
         && (wire_index >= wire_size)) /* Preserve ownership even after early parser rejection. */
    {
        parser = UART_PARSE_IDLE;
    }
}
static const section_link_handler_item_t uart_handlers[] = {
    /* Shared parser contexts. */
    {.func = uart_receive, .ctx = NULL},
};
REG_LINK(STM32_USART1_LINK, uart_tx, bsp_usart_dbg_rx_get_byte, uart_handlers,
         sizeof(uart_handlers) / sizeof(uart_handlers[0]))
