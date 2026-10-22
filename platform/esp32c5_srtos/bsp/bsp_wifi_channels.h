// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi_channels.h
 * @brief Task-owned Wi-Fi TCP endpoint interface.
 * @details Part of base. Declares endpoint configuration, polling, bounded packet transfer and diagnostics.
 *          No SDK types or protocol state are exposed.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#ifndef BSP_WIFI_CHANNELS_H
#define BSP_WIFI_CHANNELS_H
#include <stdint.h>
#include <stdbool.h>

#define BSP_WIFI_CHANNEL_CONTROL 0u
#define BSP_WIFI_CHANNEL_UPLOAD  1u
#ifndef BSP_WIFI_UPLOAD_BUFFER_SIZE
#define BSP_WIFI_UPLOAD_BUFFER_SIZE 3072u /**< Sole application upload staging buffer, including partial tails. */
#endif
#define BSP_WIFI_UPLOAD_BATCH_US 1000u /**< Maximum intentional aggregation delay before attempting a write. */
typedef uint32_t (*bsp_wifi_packet_read_t)(uint8_t *p_data, uint32_t capacity);
typedef struct bsp_wifi_channel_status
{
    bool connected;
    uint32_t session;
    uint32_t error;
} bsp_wifi_channel_status_t;
typedef struct
{
    uint32_t max_poll_gap_us;  /**< Longest interval between cooperative endpoint polls. */
    uint32_t max_write_us;     /**< Longest SDK socket write call. */
    uint32_t tx_bytes;         /**< Bytes accepted by the socket in this session. */
    uint32_t backpressure;     /**< Writes postponed by temporary socket backpressure. */
    uint32_t tx_high_water;    /**< Most staged upload bytes waiting for socket acceptance. */
    uint32_t tx_batches;       /**< Completed upload staging batches in this session. */
    uint32_t max_batch_gap_us; /**< Longest interval between completed upload batches. */
    uint32_t max_pending_us;   /**< Longest staged batch lifetime, including socket backpressure. */
    uint32_t partial_writes;   /**< Socket writes accepting only a prefix of the requested bytes. */
} bsp_wifi_debug_t;
extern volatile bsp_wifi_debug_t g_bsp_wifi_debug[2]; /**< Endpoint diagnostics for ELF observation. */
/** Configure a TCP endpoint once, from task/init context. Only the upload channel
 *  accepts a packet source; BSP copies packets and retains partial-send tails. */
bool bsp_wifi_channel_configure(uint8_t channel, uint16_t port, bsp_wifi_packet_read_t p_read);
/** Bounded, nonblocking progress for both TCP endpoints. One SECTION task owns calls. */
void bsp_wifi_channel_poll(void);
void bsp_wifi_channel_reset(uint8_t channel);
void bsp_wifi_channel_status_get(uint8_t channel, bsp_wifi_channel_status_t *p_status);
int32_t bsp_wifi_channel_read(uint8_t channel, uint8_t *p_data, uint32_t capacity);
int32_t bsp_wifi_channel_write(uint8_t channel, const uint8_t *p_data, uint32_t length);
#endif /* BSP_WIFI_CHANNELS_H */
