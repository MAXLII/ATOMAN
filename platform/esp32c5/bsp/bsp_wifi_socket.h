// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi_socket.h
 * @brief Nonblocking IPv4 TCP server/UDP primitives over ESP-IDF lwIP.
 * @details Part of base. Caller owns descriptors and must close them on link loss.
 *          No address routing, protocol, message queue or firmware dependency.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef BSP_WIFI_SOCKET_H
#define BSP_WIFI_SOCKET_H
#include <stdint.h>
#define BSP_WIFI_SOCKET_TRANSFER_MAX 2048u /**< Maximum buffer accepted per call. */
typedef struct bsp_wifi_peer
{
    uint32_t ipv4_be; /**< IPv4 address in network byte order. */
    uint16_t port;    /**< UDP peer port in host byte order. */
} bsp_wifi_peer_t;
/** @param port Local nonzero TCP port. @return Nonblocking listener, or -1 with errno. */
int32_t bsp_wifi_tcp_listen(uint16_t port);
/** @param listener Listening descriptor. @return Nonblocking client, or -1 with errno. */
int32_t bsp_wifi_tcp_accept(int32_t listener);
/** @param socket Client descriptor. @param p_data Destination. @param capacity 1..2048.
 * @return Received bytes, 0 for orderly EOF, or -1 with errno (including EAGAIN).
 */
int32_t bsp_wifi_tcp_read(int32_t socket, uint8_t *p_data, uint32_t capacity);
/** @param socket Client descriptor. @return 1 if data waits, 0 on EOF, -1 with errno when empty/error. */
int32_t bsp_wifi_tcp_probe(int32_t socket);
/** @param socket Client descriptor. @param p_data Source. @param length 1..2048.
 * @return Bytes accepted (possibly partial), or -1 with errno. Caller retries unsent tail.
 */
int32_t bsp_wifi_tcp_write(int32_t socket, const uint8_t *p_data, uint32_t length);
/** @param port Local nonzero UDP port. @return Nonblocking UDP descriptor, or -1 with errno. */
int32_t bsp_wifi_udp_bind(uint16_t port);
/** @param socket UDP descriptor. @param p_data Destination. @param capacity 1..2048.
 * @param p_peer Sender, changed only after a complete datagram.
 * @return Datagram bytes (0 is valid), or -1 with errno; truncated packets yield EMSGSIZE.
 */
int32_t bsp_wifi_udp_read(int32_t socket,
                          uint8_t *p_data,
                          uint32_t capacity,
                          bsp_wifi_peer_t *p_peer);
/** @param socket UDP descriptor. @param p_data Source. @param length 1..2048.
 * @param p_peer Destination address/port.
 * @return Datagram bytes accepted, or -1 with errno.
 */
int32_t bsp_wifi_udp_write(int32_t socket,
                           const uint8_t *p_data,
                           uint32_t length,
                           const bsp_wifi_peer_t *p_peer);
/** @param socket Descriptor to close. @return 0 on success, or -1 with errno. */
int32_t bsp_wifi_socket_close(int32_t socket);
#endif /* BSP_WIFI_SOCKET_H */
