// SPDX-License-Identifier: MIT
/**
 * @file bsp_comm.h
 * @brief Buffered board communication transports.
 * @details Part of base. No SECTION, COMM, Shell or application dependencies.
 *          Call all APIs from the same main task after USB/Wi-Fi initialization.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef BSP_COMM_H
#define BSP_COMM_H
#include "bsp_wifi_socket.h"
#include <stdint.h>
#define BSP_COMM_PORT 5000u /**< Board control TCP and discovery/echo UDP port. */
/** @brief Drain at most one queued frame per transport and service socket lifetimes. */
void bsp_comm_poll(void);
/** @return TCP generation; reset higher-layer state before consuming a different generation. */
uint32_t bsp_comm_tcp_session(void);
/** @param p_data Destination byte. @return 1 when received, otherwise 0. */
uint8_t bsp_comm_wifi_rx_get_byte(uint8_t *p_data);
/** @brief Copy a complete USB message; reject invalid or full-queue writes. */
void bsp_comm_usb_tx(char *p_data, int length);
/** @brief Copy a complete TCP message; disconnected/full-queue writes are discarded. */
void bsp_comm_wifi_tx(char *p_data, int length);
/** @brief Format a bounded message and copy it into the USB queue. */
void bsp_comm_usb_printf(const char *p_format, ...);
/** @brief Format a bounded message and copy it into the TCP queue. */
void bsp_comm_wifi_printf(const char *p_format, ...);
/** @brief Receive one datagram; return byte count or -1/errno, including EAGAIN. */
int32_t bsp_comm_udp_read(uint8_t *p_data, uint32_t capacity, bsp_wifi_peer_t *p_peer);
/** @brief Send one datagram; retain caller bytes on EAGAIN. */
int32_t bsp_comm_udp_write(const uint8_t *p_data, uint32_t length, const bsp_wifi_peer_t *p_peer);
#endif /* BSP_COMM_H */
