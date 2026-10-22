// SPDX-License-Identifier: MIT
/**
 * @file wifi_interface.h
 * @brief Application Wi-Fi control and diagnostic values, without SDK/BSP types.
 * @details Entity: application-facing values. Prior: initialized BSP. Time: task context.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef WIFI_INTERFACE_H
#define WIFI_INTERFACE_H
#include <stddef.h>
#include <stdint.h>
typedef struct
{
    uint32_t mode;
    uint32_t ready;
    uint32_t ipv4_be;
    int32_t rssi_dbm;
    int32_t error;
    size_t internal_heap_free;
    size_t internal_heap_minimum;
    uint32_t max_poll_gap_us;
    uint32_t max_write_us;
    uint32_t tx_bytes;
    uint32_t backpressure;
    uint32_t tx_high_water;
    uint32_t tx_batches;
    uint32_t max_batch_gap_us;
    uint32_t max_pending_us;
    uint32_t partial_writes;
} wifi_interface_status_t;
/** @return Whether the platform has started its Wi-Fi interface. */
uint32_t wifi_interface_enabled_get(void);
/** @param enable 0 stops Wi-Fi; 1 starts the platform-configured station. */
int32_t wifi_interface_enable_set(uint32_t enable);
/** @param p_status Destination; NULL is ignored. */
void wifi_interface_status_get(wifi_interface_status_t *p_status);
/** Explicit synchronous signal-strength refresh; may delay other application work. */
int32_t wifi_interface_rssi_refresh(void);
#endif
