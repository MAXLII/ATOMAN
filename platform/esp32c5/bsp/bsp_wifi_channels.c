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
#include "bsp_wifi.h"
#include "bsp_wifi_socket.h"
#include <errno.h>
#include <stddef.h>

typedef struct bsp_wifi_channel
{
    uint16_t port;
    int32_t listener, peer;
    uint32_t ipv4, tx_size, tx_offset;
    bsp_wifi_channel_status_t status;
    bsp_wifi_packet_read_t p_read;
    uint8_t tx[BSP_WIFI_SOCKET_TRANSFER_MAX];
} bsp_wifi_channel_t;
static bsp_wifi_channel_t channels[2] = {{.listener = -1, .peer = -1}, {.listener = -1, .peer = -1}};

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
    p_channel->peer             = -1;
    p_channel->status.connected = false;
    p_channel->tx_size          = 0u;
    p_channel->tx_offset        = 0u;
}
bool bsp_wifi_channel_configure(uint8_t channel, uint16_t port, bsp_wifi_packet_read_t p_read)
{
    if (    channel >= 2u
         || port == 0u
         || channels[1u - channel].port == port)
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
    int32_t sent = bsp_wifi_tcp_write(channels[channel].peer, p_data, length);

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
            ++p_channel->status.session;
            p_channel->status.connected = true;
            p_channel->status.error     = 0u;
        }
    }

    if (!p_channel->status.connected)
        return;
    int32_t available = bsp_wifi_tcp_probe(p_channel->peer);

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

    if (p_channel->p_read == NULL)
        return;

    for (uint32_t batch = 0u; batch < 16u; ++batch)
    {
        if (p_channel->tx_offset == p_channel->tx_size)
        {
            p_channel->tx_offset = 0u;
            p_channel->tx_size   = p_channel->p_read(p_channel->tx, sizeof(p_channel->tx));

            if (p_channel->tx_size > sizeof(p_channel->tx))
            {
                p_channel->status.error = 4u;
                bsp_wifi_channel_reset(channel);
                return;
            }
        }

        if (p_channel->tx_size == 0u)
            break;
        int32_t sent = bsp_wifi_channel_write(channel,
                                              &p_channel->tx[p_channel->tx_offset],
                                              p_channel->tx_size - p_channel->tx_offset);

        if (sent <= 0)
            break;
        p_channel->tx_offset += (uint32_t)sent;

        if (p_channel->tx_offset != p_channel->tx_size)
            break;
    }
}
void bsp_wifi_channel_poll(void)
{
    bsp_wifi_status_t radio = {0};
    bsp_wifi_status_get(&radio);

    for (uint8_t channel = 0u; channel < 2u; ++channel)
        poll_channel(channel, &radio);
}
