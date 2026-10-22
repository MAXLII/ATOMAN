// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi_channels.c
 * @brief Wi-Fi TCP endpoints, packet fetching and bounded partial sends.
 * @details Part of base. Entity: two task-owned TCP endpoints, no protocol state.
 *          Prior: one SECTION caller; packet callbacks copy within the given capacity.
 *          Time: nonblocking socket work; stale output is discarded on session loss.
 *          No RTOS task, queue, sampler or timer is created here.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "bsp_wifi_channels.h"
#include "bsp_wifi.h"
#include "bsp_wifi_socket.h"
#include "bsp_system.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

#define BSP_WIFI_UPLOAD_SEND_BYTES 1440u /**< Fill one configured TCP MSS before bypassing the batching deadline. */
#define BSP_WIFI_PROBE_US          1000u /**< Endpoint receive checks; direct I/O errors are handled immediately. */
_Static_assert(BSP_WIFI_UPLOAD_BUFFER_SIZE >= BSP_WIFI_UPLOAD_SEND_BYTES,
               "Upload staging must hold at least one TCP segment.");
_Static_assert(BSP_WIFI_UPLOAD_BUFFER_SIZE <= BSP_WIFI_SOCKET_TRANSFER_MAX,
               "Upload staging must fit one bounded socket call.");

typedef struct bsp_wifi_channel
{
    uint16_t port;
    int32_t listener, peer;
    uint32_t ipv4, tx_size, tx_flush_size;
    bsp_wifi_channel_status_t status;
    bsp_wifi_packet_read_t p_read;
    uint32_t pending_since_us;    /**< Earliest timestamp of the oldest staged prefix, wrapping in microseconds. */
    uint32_t pending_prefix_size; /**< Bytes belonging to the oldest timestamp group. */
    uint32_t appended_since_us;   /**< Earliest timestamp appended after the oldest prefix. */
    uint32_t last_batch_us;       /**< Previous completed batch timestamp. */
    uint32_t last_probe_us;       /**< Previous endpoint receive check. */
} bsp_wifi_channel_t;
static bsp_wifi_channel_t channels[2] = {{.listener = -1, .peer = -1}, {.listener = -1, .peer = -1}};
static uint8_t upload_tx[BSP_WIFI_UPLOAD_BUFFER_SIZE]; /**< Upload-only staging; control responses retain their existing owner. */
volatile bsp_wifi_debug_t g_bsp_wifi_debug[2];         /**< Task-owned timing and socket counters. */
static uint32_t last_poll_us[2]; /**< Previous endpoint poll timestamps, wrapping in microseconds. */

static bool temporary_error(void)
{
    return errno == EAGAIN
        || errno == EWOULDBLOCK;
}
void bsp_wifi_channel_reset(uint8_t channel)
{
    if (channel >= 2u)
        return;
    bsp_wifi_channel_t *p_channel = &channels[channel];

    if (p_channel->peer >= 0)
    {
        (void)bsp_wifi_socket_close(p_channel->peer);
        ++p_channel->status.session;
    }
    p_channel->peer                = -1;
    p_channel->status.connected    = false;
    p_channel->tx_size             = 0u;
    p_channel->tx_flush_size       = 0u;
    p_channel->pending_since_us    = 0u;
    p_channel->pending_prefix_size = 0u;
    p_channel->appended_since_us   = 0u;
    p_channel->last_batch_us       = 0u;
    p_channel->last_probe_us       = 0u;
}
bool bsp_wifi_channel_configure(uint8_t channel, uint16_t port, bsp_wifi_packet_read_t p_read)
{
    if (    channel >= 2u
         || port == 0u
         || channels[1u - channel].port == port
         || (    p_read != NULL
              && channel != BSP_WIFI_CHANNEL_UPLOAD))
        return false;
    bsp_wifi_channel_t *p_channel = &channels[channel];

    if (p_channel->port != 0u)
        return p_channel->port == port
            && p_channel->p_read == p_read;
    p_channel->port   = port;
    p_channel->p_read = p_read;
    return true;
}
void bsp_wifi_channel_status_get(uint8_t channel, bsp_wifi_channel_status_t *p_status)
{
    if (p_status != NULL)
        *p_status = channel < 2u ? channels[channel].status : (bsp_wifi_channel_status_t){0};
}
int32_t bsp_wifi_channel_read(uint8_t channel, uint8_t *p_data, uint32_t capacity)
{
    if (    channel >= 2u
         || !channels[channel].status.connected
         || channels[channel].p_read != NULL)
    {
        errno = ENOTCONN;
        return -1;
    }
    int32_t received = bsp_wifi_tcp_read(channels[channel].peer, p_data, capacity);

    if (    received == 0
         || (    received < 0
              &&!temporary_error()))
    {
        channels[channel].status.error = received == 0 ? 3u : (uint32_t)errno;
        bsp_wifi_channel_reset(channel);
    }
    return received;
}
int32_t bsp_wifi_channel_write(uint8_t channel, const uint8_t *p_data, uint32_t length)
{
    if (    channel >= 2u
         || !channels[channel].status.connected)
    {
        errno = ENOTCONN;
        return -1;
    }
    uint32_t start    = bsp_system_time_us_get(); /**< SDK entry timestamp. */
    int32_t sent      = bsp_wifi_tcp_write(channels[channel].peer, p_data, length);
    uint32_t duration = bsp_system_time_us_get() - start; /**< Socket call elapsed microseconds. */

    if (duration > g_bsp_wifi_debug[channel].max_write_us)
        g_bsp_wifi_debug[channel].max_write_us = duration;

    if (sent > 0)
    {
        g_bsp_wifi_debug[channel].tx_bytes += (uint32_t)sent;

        if ((uint32_t)sent < length)
            ++g_bsp_wifi_debug[channel].partial_writes;
    }
    else if (    sent < 0
              && temporary_error())
        ++g_bsp_wifi_debug[channel].backpressure;

    if (    sent == 0
         || (    sent < 0
              &&!temporary_error()))
    {
        channels[channel].status.error = sent == 0 ? 1u : (uint32_t)errno;
        bsp_wifi_channel_reset(channel);
    }
    return sent;
}
static void poll_channel(uint8_t channel, const bsp_wifi_status_t *p_radio)
{
    bsp_wifi_channel_t *p_channel = &channels[channel];
    uint32_t now                  = bsp_system_time_us_get();    /**< Main-loop endpoint entry time. */
    uint32_t elapsed              = now - last_poll_us[channel]; /**< Gap across task execution and SDK scheduling. */

    if (    p_channel->status.connected
         && elapsed > g_bsp_wifi_debug[channel].max_poll_gap_us)
        g_bsp_wifi_debug[channel].max_poll_gap_us = elapsed;
    last_poll_us[channel] = now;

    if (p_channel->port == 0u)
        return;

    if (    p_radio->ready == 0u
         || p_radio->ipv4_be != p_channel->ipv4)
    {
        bsp_wifi_channel_reset(channel);

        if (p_channel->listener >= 0)
            (void)bsp_wifi_socket_close(p_channel->listener);
        p_channel->listener = -1;
        p_channel->ipv4     = p_radio->ipv4_be;
    }

    if (p_radio->ready == 0u)
        return;

    if (p_channel->listener < 0)
        p_channel->listener = bsp_wifi_tcp_listen(p_channel->port);

    if (    p_channel->peer < 0
         && p_channel->listener >= 0)
    {
        p_channel->peer = bsp_wifi_tcp_accept(p_channel->listener);

        if (p_channel->peer >= 0)
        {
            g_bsp_wifi_debug[channel] = (bsp_wifi_debug_t){0};
            ++p_channel->status.session;
            p_channel->status.connected = true;
            p_channel->status.error     = 0u;
            p_channel->last_probe_us    = now - BSP_WIFI_PROBE_US;
        }
    }

    if (!p_channel->status.connected)
        return;

    if (now - p_channel->last_probe_us >= BSP_WIFI_PROBE_US)
    {
        int32_t available        = bsp_wifi_tcp_probe(p_channel->peer);
        p_channel->last_probe_us = now;

        if (    available == 0
             || (    available < 0
                  &&!temporary_error())
             || (    available > 0
                  && p_channel->p_read != NULL))
        {
            p_channel->status.error = available > 0 ? 2u : available == 0 ? 3u : (uint32_t)errno;
            bsp_wifi_channel_reset(channel);
            return;
        }
    }

    if (p_channel->p_read == NULL)
        return;

    for (uint32_t batch = 0u; batch < 16u; ++batch)
    {
        uint32_t pending_us = 0u; /**< Age of the oldest still-owned staged byte. */
        now                 = bsp_system_time_us_get();

        if (p_channel->tx_size < sizeof(upload_tx))
        {
            uint32_t capacity = (uint32_t)sizeof(upload_tx) - p_channel->tx_size;            /**< Accepted prefixes have already been removed. */
            uint32_t added    = p_channel->p_read(&upload_tx[p_channel->tx_size], capacity); /**< Whole packets copied by the source. */

            if (added > capacity)
            {
                p_channel->status.error = 4u;
                bsp_wifi_channel_reset(channel);
                return;
            }

            if (added != 0u)
            {
                if (p_channel->tx_size == 0u)
                {
                    p_channel->pending_since_us    = now;
                    p_channel->pending_prefix_size = added;
                }
                else if (p_channel->pending_prefix_size == p_channel->tx_size)
                    p_channel->appended_since_us = now;
            }
            p_channel->tx_size += added;

            if (p_channel->tx_size > g_bsp_wifi_debug[channel].tx_high_water)
                g_bsp_wifi_debug[channel].tx_high_water = p_channel->tx_size;
        }

        if (p_channel->tx_size == 0u)
            break;
        pending_us = now - p_channel->pending_since_us;

        if (pending_us > g_bsp_wifi_debug[channel].max_pending_us)
            g_bsp_wifi_debug[channel].max_pending_us = pending_us;

        if (p_channel->tx_flush_size == 0u)
        {
            if (p_channel->tx_size >= BSP_WIFI_UPLOAD_SEND_BYTES)
                p_channel->tx_flush_size = (p_channel->tx_size / BSP_WIFI_UPLOAD_SEND_BYTES)
                                         * BSP_WIFI_UPLOAD_SEND_BYTES;
            else if (pending_us >= BSP_WIFI_UPLOAD_BATCH_US)
                p_channel->tx_flush_size = p_channel->tx_size;
            else
                break;
        }
        int32_t sent = bsp_wifi_channel_write(channel, upload_tx, p_channel->tx_flush_size);

        if (sent <= 0)
            break;
        now        = bsp_system_time_us_get();
        pending_us = now - p_channel->pending_since_us;

        if (pending_us > g_bsp_wifi_debug[channel].max_pending_us)
            g_bsp_wifi_debug[channel].max_pending_us = pending_us;
        p_channel->tx_size -= (uint32_t)sent;
        p_channel->tx_flush_size -= (uint32_t)sent;

        if ((uint32_t)sent < p_channel->pending_prefix_size)
            p_channel->pending_prefix_size -= (uint32_t)sent;
        else
        {
            /* The new group keeps its original arrival time; appending never renews an older byte's deadline. */
            p_channel->pending_since_us    = p_channel->appended_since_us;
            p_channel->pending_prefix_size = p_channel->tx_size;
            p_channel->appended_since_us   = 0u;
        }

        if (p_channel->tx_size != 0u)
            memmove(upload_tx, &upload_tx[(uint32_t)sent], p_channel->tx_size);

        if (p_channel->tx_flush_size != 0u)
            break; /* The next poll may append behind this partial flush, but must resume it first. */

        if (    (g_bsp_wifi_debug[channel].tx_batches != 0u) /* No interval exists before the first completed batch. */
             && ((now - p_channel->last_batch_us) > g_bsp_wifi_debug[channel].max_batch_gap_us)) /* Include SDK scheduling and retransmission stalls. */
            g_bsp_wifi_debug[channel].max_batch_gap_us = now - p_channel->last_batch_us;
        ++g_bsp_wifi_debug[channel].tx_batches;
        p_channel->last_batch_us = now;
    }
}
void bsp_wifi_channel_poll(void)
{
    bsp_wifi_status_t radio = {0};
    bsp_wifi_status_get(&radio);

    for (uint8_t channel = 0u; channel < 2u; ++channel)
        poll_channel(channel, &radio);
}
