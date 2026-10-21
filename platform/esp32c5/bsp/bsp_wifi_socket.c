// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi_socket.c
 * @brief Bounded nonblocking socket operations with explicit partial/EOF/error results.
 * @details Part of base. All descriptors remain caller-owned; failed setup closes
 *          its temporary descriptor. UDP truncation is rejected, never parsed as a packet.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_wifi_socket.h"
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include "lwip/sockets.h"

/** @param socket Newly acquired descriptor. @return Descriptor, or -1 after cleanup. */
static int32_t make_nonblocking(int32_t socket)
{
    int flags = fcntl(socket, F_GETFL, 0); /* Preserve existing descriptor flags. */

    if (    (flags < 0) /* Stop on invalid descriptor. */
         || (fcntl(socket, F_SETFL, flags | O_NONBLOCK) < 0)) /* Every operation must remain nonblocking. */
    {
        int error = errno; /* Preserve the failing operation across close(). */
        (void)close(socket);
        errno = error;
        return -1;
    }
    return socket;
}

/** @param type SOCK_STREAM or SOCK_DGRAM. @param port Local nonzero port.
 * @return Bound nonblocking descriptor, or -1 with errno.
 */
static int32_t open_bound(int type, uint16_t port)
{
    int32_t descriptor         = -1;  /* Owned only after socket() succeeds. */
    int reuse                  = 1;   /* Permit quick listener restart after disconnect. */
    struct sockaddr_in address = {0}; /* Bind on the active local IPv4 interfaces. */

    if (port == 0u)
    {
        errno = EINVAL;
        return -1;
    }
    descriptor = socket(AF_INET, type, IPPROTO_IP);

    if (descriptor < 0)
    {
        return -1;
    }
    descriptor = make_nonblocking(descriptor);

    if (descriptor < 0)
    {
        return -1;
    }
    address.sin_family      = AF_INET;
    address.sin_port        = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (    (setsockopt(descriptor, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) /* Restart policy. */
         || (bind(descriptor, (const struct sockaddr *)&address, sizeof(address)) < 0))   /* Acquire local port. */
    {
        int error = errno; /* Preserve bind/setup error across cleanup. */
        (void)close(descriptor);
        errno = error;
        return -1;
    }
    return descriptor;
}

int32_t bsp_wifi_tcp_listen(uint16_t port)
{
    int32_t descriptor = open_bound(SOCK_STREAM, port); /* Temporary listener ownership. */

    if (descriptor >= 0)
    {
        if (listen(descriptor, 2) < 0)
        {
            int error = errno; /* Keep the listen error visible. */
            (void)close(descriptor);
            errno = error;
            return -1;
        }
    }
    return descriptor;
}

int32_t bsp_wifi_tcp_accept(int32_t listener)
{
    int32_t client = accept(listener, NULL, NULL); /* Newly accepted client ownership. */

    if (client >= 0)
    {
        client = make_nonblocking(client);
    }
    return client;
}

/** @param p_data Buffer. @param length Requested byte count. @return 1 for valid inputs, else 0. */
static uint8_t valid_buffer(const void *p_data, uint32_t length)
{
    if (    (p_data == NULL) /* Prevent invalid SDK memory access. */
         || (length == 0u)   /* Require explicit useful buffer space. */
         || (length > BSP_WIFI_SOCKET_TRANSFER_MAX)) /* Bound each call. */
    {
        errno = EINVAL;
        return 0u;
    }
    return 1u;
}

int32_t bsp_wifi_tcp_read(int32_t socket, uint8_t *p_data, uint32_t capacity)
{
    if (valid_buffer(p_data, capacity) == 0u)
    {
        return -1;
    }
    return (int32_t)recv(socket, p_data, capacity, 0);
}

int32_t bsp_wifi_tcp_write(int32_t socket, const uint8_t *p_data, uint32_t length)
{
    if (valid_buffer(p_data, length) == 0u)
    {
        return -1;
    }
    return (int32_t)send(socket, p_data, length, 0);
}

int32_t bsp_wifi_tcp_probe(int32_t socket)
{
    uint8_t byte = 0u; /* Probe without consuming the Link parser's next byte. */
    return (int32_t)recv(socket, &byte, 1u, MSG_PEEK);
}

int32_t bsp_wifi_udp_bind(uint16_t port)
{
    return open_bound(SOCK_DGRAM, port);
}

int32_t bsp_wifi_udp_read(int32_t socket,
                          uint8_t *p_data,
                          uint32_t capacity,
                          bsp_wifi_peer_t *p_peer)
{
    struct sockaddr_in address = {0}; /* Peer address from this datagram only. */
    struct iovec vector = {.iov_base = p_data, .iov_len = capacity}; /* One bounded payload buffer. */
    struct msghdr message = {
        .msg_name    = &address,
        .msg_namelen = sizeof(address),
        .msg_iov     = &vector,
        .msg_iovlen  = 1,
    }; /* recvmsg reports truncation independently of payload bytes. */
    int32_t received = -1; /* Datagram length or socket error. */

    if (p_peer == NULL)
    {
        errno = EINVAL;
        return -1;
    }

    if (valid_buffer(p_data, capacity) == 0u)
    {
        return -1;
    }
    received = (int32_t)recvmsg(socket, &message, 0);

    if (received < 0)
    {
        return received;
    }

    if ((message.msg_flags & MSG_TRUNC) != 0)
    {
        errno = EMSGSIZE;
        return -1;
    }
    p_peer->ipv4_be = address.sin_addr.s_addr;
    p_peer->port    = ntohs(address.sin_port);
    return received;
}

int32_t bsp_wifi_udp_write(int32_t socket,
                           const uint8_t *p_data,
                           uint32_t length,
                           const bsp_wifi_peer_t *p_peer)
{
    struct sockaddr_in address = {0}; /* Destination metadata is outside the raw payload. */

    if (    (p_peer == NULL) /* Require a complete UDP destination. */
         || (valid_buffer(p_data, length) == 0u)) /* Check caller's bytes before SDK access. */
    {
        errno = EINVAL;
        return -1;
    }

    if (p_peer->port == 0u)
    {
        errno = EINVAL;
        return -1;
    }
    address.sin_family      = AF_INET;
    address.sin_port        = htons(p_peer->port);
    address.sin_addr.s_addr = p_peer->ipv4_be;
    return (int32_t)sendto(socket, p_data, length, 0, (const struct sockaddr *)&address, sizeof(address));
}

int32_t bsp_wifi_socket_close(int32_t socket)
{
    return close(socket);
}
