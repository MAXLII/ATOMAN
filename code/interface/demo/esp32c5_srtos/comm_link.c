// SPDX-License-Identifier: MIT
/**
 * @file comm_link.c
 * @brief Bridge BSP byte transports and session events to demo protocol callbacks.
 * @details Entity: Link bindings only. Prior: initialized BSP and protocol callbacks.
 *          Time: SECTION task polling; SDK transactions retain balanced runtime guards.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "comm_link.h"
#include "demo_comm.h"
#include "comm_discovery.h"
#include "bsp_comm.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "bsp_system.h"
#include <stddef.h>
static uint32_t wifi_session;
static section_link_tx_func_t usb_output = {.my_printf = bsp_comm_usb_printf, .tx_by_dma = bsp_comm_usb_tx};
static section_link_tx_func_t wifi_output = {.my_printf = bsp_comm_wifi_printf, .tx_by_dma = bsp_comm_wifi_tx};
static uint8_t wifi_rx(uint8_t *p_data)
{
    uint32_t session = bsp_comm_tcp_session();

    if (session != wifi_session)
    {
        demo_comm_wifi_session_reset();
        wifi_session = session;
    }
    return bsp_comm_wifi_rx_get_byte(p_data);
}
/** @brief Refresh radio state independently of incoming protocol requests. */
static void poll_radio(void)
{
    bsp_system_runtime_enter();
    (void)bsp_wifi_poll();
    bsp_system_runtime_exit();
}
static void poll_transport(void)
{
    bsp_system_runtime_enter();
    bsp_comm_poll();
    bsp_system_runtime_exit();
}
static void poll_discovery(void)
{
    bsp_system_runtime_enter();
    comm_discovery_poll();
    bsp_system_runtime_exit();
}

static const section_link_handler_item_t usb_handlers[] = {{.func = demo_comm_usb_receive, .ctx = NULL}};   /**< USB owner. */
static const section_link_handler_item_t wifi_handlers[] = {{.func = demo_comm_wifi_receive, .ctx = NULL}}; /**< TCP owner. */
REG_LINK(USART_DBG_LINK, usb_output, bsp_usart_dbg_rx_get_byte, usb_handlers, 1u)
REG_LINK(WIFI_DBG_LINK, wifi_output, wifi_rx, wifi_handlers, 1u)
REG_TASK_MS(100u, poll_radio)
REG_TASK(1, poll_transport)
REG_TASK_MS(10u, poll_discovery)
