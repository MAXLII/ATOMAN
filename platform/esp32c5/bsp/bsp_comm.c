// SPDX-License-Identifier: MIT
/**
 * @file bsp_comm.c
 * @brief Task-owned USB/TCP response queues and discovery UDP transport.
 * @details Part of base. Entity: bounded byte transports, with no protocol parser.
 *          Prior: copy whole writes, retain partial tails, discard old TCP output on disconnect.
 *          Time: serialized SECTION transactions; Wi-Fi owns TCP sessions and partial socket I/O.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_comm.h"
#include "bsp_system.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "bsp_wifi_channels.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#define BSP_COMM_TX_DEPTH 16u /**< Whole-message queue slots per transport. */

typedef struct
{
    uint8_t tx[BSP_COMM_TX_DEPTH][512];  /**< Bytes copied before the callback returns. */
    uint32_t tx_size[BSP_COMM_TX_DEPTH]; /**< Valid bytes per slot. */
    uint32_t tx_head;    /**< First occupied slot. */
    uint32_t tx_count;   /**< Occupied slots. */
    uint32_t tx_offset;  /**< Accepted prefix of the first slot. */
    uint32_t tx_dropped; /**< Rejected whole writes for debugger inspection. */
} bsp_comm_queue_t;
static bsp_comm_queue_t links[2]; /**< USB and TCP queues, shared only through serialized task transactions. */
static int32_t datagram = -1;     /**< UDP listener owned by this BSP. */
static uint32_t session_ip;       /**< Address associated with current socket lifetime. */
static uint32_t tcp_session;      /**< Changes whenever the TCP peer is retired or accepted. */
static uint8_t tcp_rx[256];       /**< One socket read supplies multiple registered Link byte requests. */
static uint32_t tcp_rx_size, tcp_rx_offset; /**< Unconsumed input belonging only to tcp_session. */
static bool tcp_connected(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_CONTROL, &status);
    return status.connected;
}

static void clear_tcp_receive(void)
{
    tcp_rx_size   = 0u;
    tcp_rx_offset = 0u;
}

static bool sync_tcp_session(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_CONTROL, &status);

    if (status.session != tcp_session)
    {
        links[1] = (bsp_comm_queue_t){0};
        clear_tcp_receive();
        tcp_session = status.session;
    }

    if (!status.connected)
        clear_tcp_receive();
    return status.connected;
}

/** @param index Link index. @param p_data Bytes copied before return. @param length Signed byte count. */
static void enqueue(uint32_t index, const char *p_data, int length)
{
    bsp_comm_queue_t *p_link = &links[index]; /* Queue protected until publication finishes. */
    uint32_t tail            = 0u;            /* Next free slot, read only after protecting the complete queue transaction. */
    bsp_system_runtime_enter(); /* Prevent task switches while preserving sampling interrupts. */
    tail = (p_link->tx_head + p_link->tx_count) % BSP_COMM_TX_DEPTH;

    if (    (p_data == NULL) /* Reject an absent source. */
         || (length <= 0)    /* Do not convert an invalid signed size. */
         || (length > 512)   /* Match one bounded BSP transfer. */
         || (p_link->tx_count == BSP_COMM_TX_DEPTH) /* Reject whole messages rather than corrupting a frame. */
         || (    (index == 1u)
              &&!tcp_connected())) /* Do not retain replies for a future TCP peer. */
    {
        ++p_link->tx_dropped;
        bsp_system_runtime_exit();
        return;
    }
    memcpy(p_link->tx[tail], p_data, (size_t)length);
    p_link->tx_size[tail] = (uint32_t)length;
    ++p_link->tx_count;
    bsp_system_runtime_exit();
}

/** @param index Link index. @param p_format Format string. @param arguments Formatting arguments. */
static void format_output(uint32_t index, const char *p_format, va_list arguments)
{
    char buffer[512] = {0};     /* Bounded stack output, copied to the Link queue. */
    int length       = 0;       /* Required formatted length. */
    bsp_system_runtime_enter(); /* The SDK C library and nested queue append form one task transaction. */

    if (p_format == NULL)
    {
        bsp_system_runtime_exit();
        return;
    }
    length = vsnprintf(buffer, sizeof(buffer), p_format, arguments);

    if (length >= (int)sizeof(buffer))
    {
        length = (int)sizeof(buffer) - 1;
    }

    if (length > 0)
    {
        enqueue(index, buffer, length);
    }
    bsp_system_runtime_exit();
}

/** @param p_format printf-style Link message. @param ... Formatting arguments. */
void bsp_comm_usb_printf(const char *p_format, ...)
{
    va_list arguments; /* Initialized below before the first use. */
    va_start(arguments, p_format);
    format_output(0u, p_format, arguments);
    va_end(arguments);
}
/** @param p_format printf-style Link message. @param ... Formatting arguments. */
void bsp_comm_wifi_printf(const char *p_format, ...)
{
    va_list arguments; /* Initialized below before the first use. */
    va_start(arguments, p_format);
    format_output(1u, p_format, arguments);
    va_end(arguments);
}
/** @param p_data Binary response bytes. @param length Signed byte count. */
void bsp_comm_usb_tx(char *p_data, int length)
{
    enqueue(0u, p_data, length);
}
/** @param p_data Binary response bytes. @param length Signed byte count. */
void bsp_comm_wifi_tx(char *p_data, int length)
{
    enqueue(1u, p_data, length);
}

/** @brief Discard all bytes owned by the departing TCP peer. */
static void close_tcp(void)
{
    bsp_wifi_channel_reset(BSP_WIFI_CHANNEL_CONTROL);
    links[1] = (bsp_comm_queue_t){0};
    clear_tcp_receive();
}

/** @param index Link whose pending response should advance. */
static void drain_tx(uint32_t index)
{
    bsp_comm_queue_t *p_link = &links[index]; /* Main-task-only TX state. */
    uint32_t remaining       = 0u;            /* Bytes not yet accepted by BSP. */
    int32_t sent             = 0;             /* Partial acceptance or transport error. */

    if (p_link->tx_count == 0u)
    {
        return;
    }
    remaining = p_link->tx_size[p_link->tx_head] - p_link->tx_offset;

    if (index == 0u)
    {
        sent = bsp_usart_write(&p_link->tx[p_link->tx_head][p_link->tx_offset], remaining);
    }
    else
    {
        sent = bsp_wifi_channel_write(BSP_WIFI_CHANNEL_CONTROL,
                                      &p_link->tx[p_link->tx_head][p_link->tx_offset],
                                      remaining);

        if (    (sent == 0)
             || (    (sent < 0) /* A nonblocking wait retains the same response bytes. */
                  && (errno != EAGAIN)
                  && (errno != EWOULDBLOCK))) /* Real failures retire the session. */
        {
            close_tcp();
            return;
        }
    }

    if (sent > 0)
    {
        p_link->tx_offset += (uint32_t)sent;

        if (p_link->tx_offset == p_link->tx_size[p_link->tx_head])
        {
            p_link->tx_offset = 0u;
            p_link->tx_head = (p_link->tx_head + 1u) % BSP_COMM_TX_DEPTH;
            --p_link->tx_count;
        }
    }
}

void bsp_comm_poll(void)
{
    bsp_wifi_status_t status = {0}; /* Last radio poll snapshot. */
    drain_tx(0u);
    (void)bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_CONTROL, BSP_COMM_PORT, NULL);
    bsp_wifi_channel_poll();
    bool connected = sync_tcp_session();
    bsp_wifi_status_get(&status);

    if (    (status.ready == 0u)            /* Retire transports while disconnected. */
         || (session_ip != status.ipv4_be)) /* A new address means a new socket lifetime. */
    {
        if (datagram >= 0)
        {
            (void)bsp_wifi_socket_close(datagram);
            datagram = -1;
        }
        session_ip = status.ipv4_be;
    }

    if (status.ready == 0u)
    {
        return;
    }

    if (datagram < 0)
    {
        datagram = bsp_wifi_udp_bind(BSP_COMM_PORT);
    }

    if (connected)
        drain_tx(1u);
}

uint32_t bsp_comm_tcp_session(void)
{
    bsp_wifi_channel_status_t status;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_CONTROL, &status);
    return status.session;
}

uint8_t bsp_comm_wifi_rx_get_byte(uint8_t *p_data)
{
    int32_t received = -1;

    if (    (p_data == NULL)
         || !sync_tcp_session()
         || (links[1].tx_count == BSP_COMM_TX_DEPTH))
    {
        return 0u;
    }

    if (tcp_rx_offset == tcp_rx_size)
    {
        clear_tcp_receive();
        received = bsp_wifi_channel_read(BSP_WIFI_CHANNEL_CONTROL, tcp_rx, sizeof(tcp_rx));

        if (received <= 0)
        {
            if (    (received == 0)
                 || (    (errno != EAGAIN)
                      && (errno != EWOULDBLOCK)))
                close_tcp();
            return 0u;
        }
        tcp_rx_size = (uint32_t)received;
    }
    *p_data = tcp_rx[tcp_rx_offset++];
    return 1u;
}

int32_t bsp_comm_udp_read(uint8_t *p_data, uint32_t capacity, bsp_wifi_peer_t *p_peer)
{
    if (datagram < 0)
    {
        errno = ENOTCONN;
        return -1;
    }
    return bsp_wifi_udp_read(datagram, p_data, capacity, p_peer);
}

int32_t bsp_comm_udp_write(const uint8_t *p_data, uint32_t length, const bsp_wifi_peer_t *p_peer)
{
    if (datagram < 0)
    {
        errno = ENOTCONN;
        return -1;
    }
    return bsp_wifi_udp_write(datagram, p_data, length, p_peer);
}
