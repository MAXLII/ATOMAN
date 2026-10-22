// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp_stream.c
 * @brief Nonblocking TCP upload with a stable ACK-retained byte ring.
 * @details Part of base. Entity: one raw-API endpoint and fixed transmit storage.
 *          Prior: LwIP callbacks and poll have one owner; reset may come from another task.
 *          Time: bounded work, no waits; session retirement discards only that peer's bytes.
 *          Packet contents and sampling state belong to the caller, never to this BSP.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "bsp_tcp_stream.h"
#include "gd32e50x.h"
#include "lwip/tcp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <stddef.h>
#include <string.h>

#define TCP_STREAM_RING_BYTES 2500u
#define TCP_STREAM_BATCH_MS   1u
static struct tcp_pcb *p_listener, *p_peer;
static uint32_t (*p_source)(uint8_t *p_data, uint32_t capacity);
static uint8_t ring[TCP_STREAM_RING_BYTES];
static uint8_t packet[1024];
static uint32_t head, submit, tail, pending;
static uint32_t last_batch_ms;
static uint16_t listen_port;
static volatile bool connected, reset_requested;
static volatile uint32_t session, last_error;
volatile bsp_tcp_stream_debug_t g_bsp_tcp_stream_debug;
static uint32_t lock(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}
static void unlock(uint32_t state)
{
    __set_PRIMASK(state);
}
static void retire(uint32_t error)
{
    uint32_t state = lock();

    if (connected)
        ++session;
    connected       = false;
    reset_requested = true;
    last_error      = error;
    unlock(state);
}
void bsp_tcp_stream_reset(void)
{
    retire(0u);
}
bool bsp_tcp_stream_connected(void)
{
    return connected;
}
uint32_t bsp_tcp_stream_session(void)
{
    return session;
}
uint32_t bsp_tcp_stream_error(void)
{
    return last_error;
}
bool bsp_tcp_stream_configure(uint16_t port, uint32_t (*p_read)(uint8_t *p_data, uint32_t capacity))
{
    if (    port == 0u
         || p_read == NULL
         || listen_port != 0u)
        return false;
    listen_port = port;
    p_source    = p_read;
    return true;
}
static void clear_peer(void)
{
    if (p_peer != NULL)
    {
        struct tcp_pcb *p_old = p_peer;
        p_peer                = NULL;
        tcp_arg(p_old, NULL);
        tcp_recv(p_old, NULL);
        tcp_sent(p_old, NULL);
        tcp_err(p_old, NULL);
        tcp_abort(p_old);
    }
    head            = 0u;
    submit          = 0u;
    tail            = 0u;
    pending         = 0u;
    uint32_t state  = lock();
    reset_requested = false;
    unlock(state);
}
static void failed(void *p_argument, err_t error)
{
    (void)p_argument;
    p_peer = NULL; /* LwIP already freed the PCB. */
    retire((uint32_t)(-(int32_t)error));
}
static err_t received(void *p_argument,
                      struct tcp_pcb *p_connection,
                      struct pbuf *p_packet,
                      err_t error)
{
    (void)p_argument;

    if (p_packet != NULL)
        pbuf_free(p_packet);
    retire(error != ERR_OK ? (uint32_t)(-(int32_t)error) : p_packet == NULL ? 3u : 2u);
    p_peer = NULL;
    tcp_err(p_connection, NULL);
    tcp_abort(p_connection);
    return ERR_ABRT;
}
static err_t acknowledged(void *p_argument, struct tcp_pcb *p_connection, u16_t length)
{
    (void)p_argument;

    if (    p_connection != p_peer
         || (uint32_t)length > pending)
        return ERR_OK;
    tail = (tail + length) % sizeof(ring);
    pending -= length;
    return ERR_OK;
}
static err_t accepted(void *p_argument, struct tcp_pcb *p_connection, err_t error)
{
    (void)p_argument;

    if (error != ERR_OK)
        return error;

    if (    p_peer != NULL
         || reset_requested)
    {
        tcp_abort(p_connection);
        return ERR_ABRT;
    }
    head          = 0u;
    submit        = 0u;
    tail          = 0u;
    pending       = 0u;
    p_peer        = p_connection;
    last_batch_ms = sys_now();
    memset((void *)&g_bsp_tcp_stream_debug, 0, sizeof(g_bsp_tcp_stream_debug));
    tcp_arg(p_connection, p_connection);
    tcp_recv(p_connection, received);
    tcp_sent(p_connection, acknowledged);
    tcp_err(p_connection, failed);
    tcp_nagle_disable(p_connection);
    uint32_t state = lock();
    ++session;
    connected  = true;
    last_error = 0u;
    unlock(state);
    return ERR_OK;
}
void bsp_tcp_stream_poll(bool ready)
{
    if (    reset_requested
         || !ready)
        clear_peer();

    if (!ready)
    {
        if (connected)
            retire(3u);

        if (p_listener != NULL)
        {
            (void)tcp_close(p_listener);
            p_listener = NULL;
        }
        return;
    }

    if (listen_port == 0u)
        return;

    if (p_listener == NULL)
    {
        struct tcp_pcb *p_new = tcp_new_ip_type(IPADDR_TYPE_V4);

        if (p_new == NULL)
            return;

        if (tcp_bind(p_new, IP_ANY_TYPE, listen_port) != ERR_OK)
        {
            tcp_abort(p_new);
            return;
        }
        p_listener = tcp_listen(p_new);

        if (p_listener == NULL)
        {
            tcp_abort(p_new);
            return;
        }
        tcp_accept(p_listener, accepted);
    }

    if (    !connected
         || p_peer == NULL
         || reset_requested)
        return;
    uint32_t current_session = session;
    uint32_t now_ms          = sys_now();
    uint32_t gap             = now_ms - last_batch_ms;
    bool batch_due           = gap >= TCP_STREAM_BATCH_MS;

    if (batch_due)
    {
        if (gap > g_bsp_tcp_stream_debug.max_batch_gap_ms)
            g_bsp_tcp_stream_debug.max_batch_gap_ms = gap;
        ++g_bsp_tcp_stream_debug.batches;
        last_batch_ms = now_ms;
    }
    /* New batches are paced; connection handling and existing TCP output
     * retries still run on every owner-task pass. */

    for (    uint32_t batch = 0u; batch_due
                               && batch < 4u;
             ++batch)
    {
        uint32_t free_bytes = (tail + sizeof(ring) - head - 1u) % sizeof(ring);
        uint32_t capacity = free_bytes < sizeof(packet) ? free_bytes : sizeof(packet);

        if (capacity != 0u)
        {
            uint32_t size = p_source(packet, capacity);

            if (    !connected
                 || reset_requested
                 || session != current_session)
                return;

            if (size > capacity)
            {
                retire(4u);
                return;
            }
            uint32_t first = sizeof(ring) - head;

            if (first > size)
                first = size;
            memcpy(&ring[head], packet, first);
            memcpy(ring, &packet[first], size - first);
            head = (head + size) % sizeof(ring);
            uint32_t used = (head + sizeof(ring) - tail) % sizeof(ring);

            if (used > g_bsp_tcp_stream_debug.ring_high_water)
                g_bsp_tcp_stream_debug.ring_high_water = used;
        }

        if (submit == head)
            break;
        uint32_t length = head > submit ? head - submit : sizeof(ring) - submit;

        if (length > tcp_sndbuf(p_peer))
            length = tcp_sndbuf(p_peer);

        if (length == 0u)
        {
            ++g_bsp_tcp_stream_debug.no_send_space;
            break;
        }
        err_t result = tcp_write(p_peer, &ring[submit], (u16_t)length, 0u);

        if (result == ERR_MEM)
        {
            ++g_bsp_tcp_stream_debug.write_busy;
            break;
        }

        if (result != ERR_OK)
        {
            retire((uint32_t)(-(int32_t)result));
            break;
        }
        submit = (submit + length) % sizeof(ring);
        pending += length;
    }

    if (    p_peer != NULL
         &&!reset_requested)
    {
        if (tcp_output(p_peer) == ERR_MEM)
            ++g_bsp_tcp_stream_debug.output_busy;
    }
}
