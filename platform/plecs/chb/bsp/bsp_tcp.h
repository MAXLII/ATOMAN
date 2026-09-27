// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp.h
 * @brief FRAME TCP endpoint for the CHB PLECS simulation.
 * @details The CHB node uses TCP port 5004 and defaults to address 2.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef CHB_BSP_TCP_H
#define CHB_BSP_TCP_H

#include "sim_tcp.h"
#include "comm_addr.h"
#include <stdint.h>

#define CHB_FRAME_TCP_PORT 5004u
#define CHB_FRAME_NODE_ADDR HOST_ADDR /* Discovery uses the protocol's local address. */

void bsp_tcp_dbg_printf(const char *p_format, ...);
void bsp_tcp_dbg_tx(char *p_data, int len);
uint8_t bsp_tcp_dbg_rx_get_byte(uint8_t *p_data);
/** @param p_format Format string sent to the grid peer. */
void bsp_tcp_peer_printf(const char *p_format, ...);
/** @param p_data Outgoing bytes. @param len Byte count. */
void bsp_tcp_peer_tx(char *p_data, int len);
/** @param p_data Next incoming byte. @return Nonzero when a byte is available. */
uint8_t bsp_tcp_peer_rx_get_byte(uint8_t *p_data);
void sim_comm_stop(void);

#endif /* CHB_BSP_TCP_H */
