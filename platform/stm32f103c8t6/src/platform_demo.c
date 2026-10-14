// SPDX-License-Identifier: MIT
/**
 * @file platform_demo.c
 * @brief Platform bring-up observability.
 * @details
 *          This file is part of the digital power framework project.
 *          Module responsibilities:
 *          - Expose runtime/clock/queue diagnostics through the shared Shell
 *          - Register heartbeat and interrupt counters and the existing FRAME loopback command
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
#include "bsp_gpio.h"
#include "bsp_usart.h"
#include "stm32f10x.h"
#include "systick.h"
#include "section.h"
#include "shell.h"
#include "comm.h"
static volatile uint32_t heartbeat_count = 0u; /* Registered 500 ms heartbeat executions. */
static volatile uint32_t interrupt_count = 0u; /* TIM2-driven registered ISR executions. */
static void heartbeat(void)
{
    bsp_gpio_led_toggle();
    heartbeat_count++;
}
static void interrupt_probe(void)
{
    interrupt_count++;
}
static void board(shell_core_io_t *p_io)
{
    if (    (p_io == NULL)             /* No transport callback object. */
         || (p_io->my_printf == NULL)) /* This Link cannot print. */
    {
        return;
    }
    p_io->my_printf(
        "board=stm32f103c8t6 runtime=%s preemptive=%u clock=%u tick=%u led=%u irq=%u txdrop=%u rxdrop=%u rxerr=%u perf=%u\r\n",
        section_runtime_name_get(),
        (unsigned)section_runtime_preemptive_get(),
        (unsigned)SystemCoreClock,
        (unsigned)systick_gettime_100us(),
        (unsigned)heartbeat_count,
        (unsigned)interrupt_count,
        (unsigned)g_bsp_usart_dbg_tx_drop_count,
        (unsigned)g_bsp_usart_dbg_rx_drop_count,
        (unsigned)g_bsp_usart_dbg_rx_error_count,
        (unsigned)perf_base_cnt_get());
}
static void loopback(void *p_frame, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = p_frame; /* Borrowed validated parser frame. */
    section_packform_t reply   = {0};     /* Response retains request command and payload. */

    if (    (p_pack == NULL)        /* Parser supplied no frame. */
         || (p_pack->is_ack == 1u)) /* Replies do not trigger reply loops. */
    {
        return;
    }
    reply        = *p_pack;
    reply.src    = p_pack->dst;
    reply.d_src  = p_pack->d_dst;
    reply.dst    = p_pack->src;
    reply.d_dst  = p_pack->d_src;
    reply.is_ack = 1u;
    comm_send_data(&reply, my_printf);
}
REG_TASK_MS(500u, heartbeat)
REG_INTERRUPT(0, interrupt_probe)
REG_SHELL_CMD(board, board)
REG_COMM(0x01, 0x17, loopback)
