// SPDX-License-Identifier: MIT
/**
 * @file wifi_interface.c
 * @brief Translate application Wi-Fi requests and values to the platform BSP.
 * @details Entity: typed mappings only. Prior: initialized BSP. Time: guarded task calls.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "wifi_interface.h"
#include "bsp_wifi.h"
#include "bsp_wifi_channels.h"
#include "bsp_system.h"
#include "wifi_config.h"

uint32_t wifi_interface_enabled_get(void)
{
    bsp_wifi_status_t status = {0};
    bsp_wifi_status_get(&status);
    return status.mode != BSP_WIFI_OFF ? 1u : 0u;
}

int32_t wifi_interface_enable_set(uint32_t enable)
{
    const bsp_wifi_config_t config = {.mode     = BSP_WIFI_STA,
                                      .ssid     = ESP32C5_WIFI_SSID,
                                      .password = ESP32C5_WIFI_PASSWORD,
                                      .channel  = ESP32C5_WIFI_CHANNEL};
    int32_t result;
    bsp_system_runtime_enter();
    result = enable == 1u ? bsp_wifi_start(&config) : bsp_wifi_stop();
    bsp_system_runtime_exit();
    return result;
}

void wifi_interface_status_get(wifi_interface_status_t *p_status)
{
    if (p_status == NULL)
        return;
    bsp_wifi_status_t status = {0};
    bsp_system_runtime_enter();
    bsp_wifi_status_get(&status);
    p_status->mode                  = (uint32_t)status.mode;
    p_status->ready                 = status.ready;
    p_status->ipv4_be               = status.ipv4_be;
    p_status->rssi_dbm              = status.rssi_dbm;
    p_status->error                 = status.error;
    p_status->internal_heap_free    = bsp_system_internal_heap_free();
    p_status->internal_heap_minimum = bsp_system_internal_heap_minimum();
    p_status->max_poll_gap_us       = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_poll_gap_us;
    p_status->max_write_us          = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_write_us;
    p_status->tx_bytes              = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].tx_bytes;
    p_status->backpressure          = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].backpressure;
    p_status->tx_high_water         = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].tx_high_water;
    p_status->tx_batches            = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].tx_batches;
    p_status->max_batch_gap_us      = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_batch_gap_us;
    p_status->max_pending_us        = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_pending_us;
    p_status->partial_writes        = g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].partial_writes;
    bsp_system_runtime_exit();
}

int32_t wifi_interface_rssi_refresh(void)
{
    int32_t result;
    bsp_system_runtime_enter();
    result = bsp_wifi_refresh_rssi();
    bsp_system_runtime_exit();
    return result;
}
