// SPDX-License-Identifier: MIT
/**
 * @file    bsp_ethernet.c
 * @brief   Zynq-7020 PS GEM0 and lwIP TCP service implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Initialize the PS GEM0 lwIP network interface with a static IPv4 address
 *          - Accept one FRAME TCP client and preserve the received byte-stream order
 *          - Pump Ethernet input and lwIP TCP timers from the Section task context
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation in project-owned code
 *          - lwIP callbacks and polling run outside ISR context
 *          - Hardware access is abstracted through the Xilinx XEmacPs BSP
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

#include "bsp_timer.h"

#include "lwip/init.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/tcp.h"
#include "netif/xadapter.h"
#include "xparameters.h"
#include "xstatus.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BSP_ETHERNET_TCP_PORT            9000U
#define BSP_ETHERNET_RX_BUFFER_SIZE      16384U
#define BSP_ETHERNET_PRINT_BUFFER_SIZE   512U
#define BSP_ETHERNET_TCP_FAST_TICKS      2500U
#define BSP_ETHERNET_TCP_SLOW_TICKS      5000U

typedef struct
{
    struct netif network_interface;
    struct tcp_pcb *p_listener;
    struct tcp_pcb *p_client;
    uint8_t rx_buffer[BSP_ETHERNET_RX_BUFFER_SIZE];
    uint32_t rx_read_index;
    uint32_t rx_write_index;
    uint32_t rx_count;
    uint32_t last_fast_timer_tick;
    uint32_t last_slow_timer_tick;
    bsp_ethernet_status_t status;
} bsp_ethernet_context_t;

static bsp_ethernet_context_t s_ethernet;

static void client_state_clear(void)
{
    s_ethernet.p_client = NULL;
    s_ethernet.rx_read_index = 0U;
    s_ethernet.rx_write_index = 0U;
    s_ethernet.rx_count = 0U;
    s_ethernet.status.client_connected = 0U;
    s_ethernet.status.receive_buffer_used = 0U;
}

static void client_error(void *p_argument, err_t error)
{
    (void)p_argument;
    (void)error;
    client_state_clear();
}

static err_t client_receive(void *p_argument,
                            struct tcp_pcb *p_control_block,
                            struct pbuf *p_packet,
                            err_t error)
{
    const struct pbuf *p_fragment = NULL;
    uint32_t packet_length = 0U;

    (void)p_argument;
    if ((p_packet == NULL) || (error != ERR_OK))
    {
        if (p_packet != NULL)
        {
            (void)pbuf_free(p_packet);
        }
        if (p_control_block != NULL)
        {
            tcp_arg(p_control_block, NULL);
            tcp_recv(p_control_block, NULL);
            tcp_err(p_control_block, NULL);
            (void)tcp_close(p_control_block);
        }
        client_state_clear();
        return ERR_OK;
    }

    packet_length = (uint32_t)p_packet->tot_len;
    if (packet_length > (BSP_ETHERNET_RX_BUFFER_SIZE - s_ethernet.rx_count))
    {
        s_ethernet.status.receive_overflow = 1U;
        (void)pbuf_free(p_packet);
        tcp_abort(p_control_block);
        client_state_clear();
        return ERR_ABRT;
    }

    for (p_fragment = p_packet; p_fragment != NULL; p_fragment = p_fragment->next)
    {
        const uint8_t *p_source = (const uint8_t *)p_fragment->payload;
        uint16_t index = 0U;

        for (index = 0U; index < p_fragment->len; index++)
        {
            s_ethernet.rx_buffer[s_ethernet.rx_write_index] = p_source[index];
            s_ethernet.rx_write_index =
                (s_ethernet.rx_write_index + 1U) % BSP_ETHERNET_RX_BUFFER_SIZE;
        }
    }
    s_ethernet.rx_count += packet_length;
    s_ethernet.status.received_bytes += packet_length;
    s_ethernet.status.receive_buffer_used = s_ethernet.rx_count;
    tcp_recved(p_control_block, p_packet->tot_len);
    (void)pbuf_free(p_packet);
    return ERR_OK;
}

static err_t client_accept(void *p_argument, struct tcp_pcb *p_client, err_t error)
{
    (void)p_argument;
    if ((p_client == NULL) || (error != ERR_OK))
    {
        return ERR_VAL;
    }
    if (s_ethernet.p_client != NULL)
    {
        tcp_abort(p_client);
        return ERR_ABRT;
    }

    client_state_clear();
    s_ethernet.p_client = p_client;
    s_ethernet.status.client_connected = 1U;
    tcp_arg(p_client, &s_ethernet);
    tcp_recv(p_client, client_receive);
    tcp_err(p_client, client_error);
    tcp_nagle_disable(p_client);
    return ERR_OK;
}

static int32_t server_start(void)
{
    struct tcp_pcb *p_server = tcp_new();
    err_t error = ERR_OK;

    if (p_server == NULL)
    {
        return XST_FAILURE;
    }
    error = tcp_bind(p_server, IP_ADDR_ANY, (u16_t)BSP_ETHERNET_TCP_PORT);
    if (error != ERR_OK)
    {
        tcp_abort(p_server);
        return XST_FAILURE;
    }
    s_ethernet.p_listener = tcp_listen(p_server);
    if (s_ethernet.p_listener == NULL)
    {
        tcp_abort(p_server);
        return XST_FAILURE;
    }
    tcp_accept(s_ethernet.p_listener, client_accept);
    return XST_SUCCESS;
}

int32_t bsp_ethernet_init(void)
{
    ip_addr_t address = {0};
    ip_addr_t netmask = {0};
    ip_addr_t gateway = {0};
    uint8_t mac_address[6] = {0x02U, 0x00U, 0x00U, 0x70U, 0x20U, 0x01U};
    uint32_t current_tick = bsp_timer_gettime_100us();

    (void)memset(&s_ethernet, 0, sizeof(s_ethernet));
    IP4_ADDR(&address, 192U, 168U, 1U, 10U);
    IP4_ADDR(&netmask, 255U, 255U, 255U, 0U);
    IP4_ADDR(&gateway, 192U, 168U, 1U, 1U);

    lwip_init();
    if (xemac_add(&s_ethernet.network_interface,
                  &address,
                  &netmask,
                  &gateway,
                  mac_address,
                  (UINTPTR)XPAR_XEMACPS_0_BASEADDR) == NULL)
    {
        return XST_FAILURE;
    }
    netif_set_default(&s_ethernet.network_interface);
    netif_set_up(&s_ethernet.network_interface);
    if (server_start() != XST_SUCCESS)
    {
        netif_set_down(&s_ethernet.network_interface);
        return XST_FAILURE;
    }

    s_ethernet.last_fast_timer_tick = current_tick;
    s_ethernet.last_slow_timer_tick = current_tick;
    s_ethernet.status.initialized = 1U;
    return XST_SUCCESS;
}

void bsp_ethernet_poll(void)
{
    uint32_t current_tick = 0U;

    if (s_ethernet.status.initialized == 0U)
    {
        return;
    }
    (void)xemacif_input(&s_ethernet.network_interface);
    current_tick = bsp_timer_gettime_100us();
    if ((current_tick - s_ethernet.last_fast_timer_tick) >= BSP_ETHERNET_TCP_FAST_TICKS)
    {
        s_ethernet.last_fast_timer_tick = current_tick;
        tcp_fasttmr();
    }
    if ((current_tick - s_ethernet.last_slow_timer_tick) >= BSP_ETHERNET_TCP_SLOW_TICKS)
    {
        s_ethernet.last_slow_timer_tick = current_tick;
        tcp_slowtmr();
    }
}

uint8_t bsp_ethernet_rx_get_byte(uint8_t *p_data)
{
    if ((p_data == NULL) || (s_ethernet.rx_count == 0U))
    {
        return 0U;
    }
    *p_data = s_ethernet.rx_buffer[s_ethernet.rx_read_index];
    s_ethernet.rx_read_index =
        (s_ethernet.rx_read_index + 1U) % BSP_ETHERNET_RX_BUFFER_SIZE;
    s_ethernet.rx_count--;
    s_ethernet.status.receive_buffer_used = s_ethernet.rx_count;
    return 1U;
}

int32_t bsp_ethernet_tx(const uint8_t *p_data, uint32_t length)
{
    struct tcp_pcb *p_client = s_ethernet.p_client;
    uint32_t offset = 0U;

    if ((p_data == NULL) || (length == 0U) || (p_client == NULL))
    {
        return XST_INVALID_PARAM;
    }
    while (offset < length)
    {
        uint32_t available = (uint32_t)tcp_sndbuf(p_client);
        uint32_t chunk_length = length - offset;
        err_t error = ERR_OK;

        if (available == 0U)
        {
            return XST_FAILURE;
        }
        if (chunk_length > available)
        {
            chunk_length = available;
        }
        if (chunk_length > (uint32_t)UINT16_MAX)
        {
            chunk_length = (uint32_t)UINT16_MAX;
        }
        error = tcp_write(p_client,
                          &p_data[offset],
                          (u16_t)chunk_length,
                          (u8_t)TCP_WRITE_FLAG_COPY);
        if (error != ERR_OK)
        {
            return XST_FAILURE;
        }
        offset += chunk_length;
    }
    if (tcp_output(p_client) != ERR_OK)
    {
        return XST_FAILURE;
    }
    s_ethernet.status.transmitted_bytes += length;
    return XST_SUCCESS;
}

void bsp_ethernet_printf(const char *format, ...)
{
    char buffer[BSP_ETHERNET_PRINT_BUFFER_SIZE] = {0};
    va_list arguments;
    int length = 0;

    if (format == NULL)
    {
        return;
    }
    va_start(arguments, format);
    length = vsnprintf(buffer, sizeof(buffer), format, arguments);
    va_end(arguments);
    if (length <= 0)
    {
        return;
    }
    if ((size_t)length >= sizeof(buffer))
    {
        length = (int)(sizeof(buffer) - 1U);
    }
    (void)bsp_ethernet_tx((const uint8_t *)buffer, (uint32_t)length);
}

void bsp_ethernet_status_get(bsp_ethernet_status_t *p_status)
{
    if (p_status != NULL)
    {
        *p_status = s_ethernet.status;
    }
}
