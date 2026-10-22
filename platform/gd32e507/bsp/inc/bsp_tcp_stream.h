// SPDX-License-Identifier: MIT
/**
 * @file bsp_tcp_stream.h
 * @brief Task-owned, upload-only TCP byte stream over the existing Ethernet stack.
 * @details Part of base. Application supplies a bounded packet source; BSP owns
 *          connection lifetime and ACK-retained storage. No scope or protocol types.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef BSP_TCP_STREAM_H
#define BSP_TCP_STREAM_H
#include <stdbool.h>
#include <stdint.h>
typedef struct bsp_tcp_stream_debug
{
    uint32_t batches;
    uint32_t max_batch_gap_ms;
    uint32_t ring_high_water;
    uint32_t no_send_space;
    uint32_t write_busy;
    uint32_t output_busy;
} bsp_tcp_stream_debug_t;
extern volatile bsp_tcp_stream_debug_t g_bsp_tcp_stream_debug;
bool bsp_tcp_stream_configure(uint16_t port, uint32_t (*p_read)(uint8_t *p_data, uint32_t capacity));
/** Call from the LwIP owner task; batches every 1 ms, retries output each pass. */
void bsp_tcp_stream_poll(bool ready);
/** Thread-safe retirement request; raw TCP teardown runs on the stack owner task. */
void bsp_tcp_stream_reset(void);
bool bsp_tcp_stream_connected(void);
uint32_t bsp_tcp_stream_session(void);
uint32_t bsp_tcp_stream_error(void);
#endif
