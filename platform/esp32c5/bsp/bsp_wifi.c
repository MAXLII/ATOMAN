// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi.c
 * @brief ESP-IDF Wi-Fi/netif adapter with task-owned reconnect polling.
 * @details Part of the base project. One caller owns configuration and snapshots.
 *          SDK event callbacks publish atomic association/IP state; the owner polls it.
 *          RSSI queries are explicit because the SDK performs a synchronous Wi-Fi RPC.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_wifi.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <string.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#define BSP_WIFI_RETRY_US INT64_C(5000000) /**< Minimum reconnect spacing. */
static esp_netif_t *p_station;             /**< SDK-owned STA interface after init. */
static esp_netif_t *p_access_point;        /**< SDK-owned AP interface after init. */
static bool initialized;          /**< Driver resources have been acquired. */
static bsp_wifi_status_t status;  /**< Only the owner task writes this snapshot. */
static int64_t last_connect_us;   /**< Last association request in monotonic microseconds. */
static atomic_uint station_state; /**< SDK event task publishes 0=down, 1=associated, 2=IPv4 ready. */
_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "Wi-Fi event state requires lock-free unsigned atomics");

/** @brief Publish SDK events without calling the radio or modifying owner snapshots. */
static void wifi_event(void *p_context,
                       esp_event_base_t base,
                       int32_t id,
                       void *p_data)
{
    (void)p_context;

    if (base == WIFI_EVENT)
    {
        if (id == WIFI_EVENT_STA_CONNECTED)
            atomic_store_explicit(&station_state, 1u, memory_order_release);
        else if (    id == WIFI_EVENT_STA_DISCONNECTED
                  || id == WIFI_EVENT_STA_STOP)
            atomic_store_explicit(&station_state, 0u, memory_order_release);
    }
    else if (base == IP_EVENT)
    {
        if (    id == IP_EVENT_STA_GOT_IP
             && p_data != NULL)
        {
            const ip_event_got_ip_t *p_event = p_data;
            unsigned expected                = 1u;

            if (p_event->esp_netif == p_station)
                (void)atomic_compare_exchange_strong_explicit(&station_state,
                                                              &expected,
                                                              2u,
                                                              memory_order_release,
                                                              memory_order_relaxed);
        }
        else if (id == IP_EVENT_STA_LOST_IP)
        {
            unsigned expected = 2u;
            (void)atomic_compare_exchange_strong_explicit(&station_state,
                                                          &expected,
                                                          1u,
                                                          memory_order_release,
                                                          memory_order_relaxed);
        }
    }
}

int32_t bsp_wifi_mac_get(uint8_t p_mac[6])
{
    if (p_mac == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_wifi_get_mac(WIFI_IF_STA, p_mac);
}

/** @param p_text Fixed-size character buffer. @param capacity Buffer capacity.
 * @return First NUL offset, or capacity when unterminated.
 */
static size_t text_length(const char *p_text, size_t capacity)
{
    size_t length = 0u; /* Bounded input offset. */

    while (length < capacity)
    {
        if (p_text[length] == '\0')
        {
            break;
        }
        ++length;
    }
    return length;
}

int32_t bsp_wifi_init(void)
{
    esp_err_t result          = ESP_OK; /* SDK setup result. */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT(); /* SDK allocation policy. */
    bool driver_ready         = false; /* Roll back only acquired SDK resources. */
    bool wifi_events_ready    = false;

    if (initialized == true)
    {
        return ESP_OK;
    }
    result = esp_netif_init();

    if (result != ESP_OK)
    {
        return result;
    }
    result = esp_event_loop_create_default();

    if (    (result != ESP_OK) /* Reuse the process-wide default event loop. */
         && (result != ESP_ERR_INVALID_STATE)) /* Propagate actual allocation/setup failures. */
    {
        return result;
    }
    p_station      = esp_netif_create_default_wifi_sta();
    p_access_point = esp_netif_create_default_wifi_ap();

    if (    (p_station == NULL)       /* STA must be available before initialization commits. */
         || (p_access_point == NULL)) /* AP is part of the same owned radio. */
    {
        result = ESP_ERR_NO_MEM;
    }
    else
    {
        config.nvs_enable = 0; /* Never persist user credentials or erase NVS on failure. */
        result            = esp_wifi_init(&config);
        driver_ready      = result == ESP_OK;
    }

    if (result == ESP_OK)
    {
        result            = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL);
        wifi_events_ready = result == ESP_OK;
    }

    if (result == ESP_OK)
        result = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL);

    if (result != ESP_OK)
    {
        if (wifi_events_ready)
            (void)esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event);

        if (driver_ready)
            (void)esp_wifi_deinit();

        if (p_station != NULL)
        {
            esp_netif_destroy_default_wifi(p_station);
            p_station = NULL;
        }

        if (p_access_point != NULL)
        {
            esp_netif_destroy_default_wifi(p_access_point);
            p_access_point = NULL;
        }
        return result;
    }
    initialized = true;
    return ESP_OK;
}

int32_t bsp_wifi_stop(void)
{
    esp_err_t result = ESP_OK; /* A failed stop must not claim the radio is off. */

    if (status.mode != BSP_WIFI_OFF)
    {
        result = esp_wifi_stop();

        if (result == ESP_OK)
        {
            status = (bsp_wifi_status_t){0};
            atomic_store_explicit(&station_state, 0u, memory_order_release);
        }
    }
    status.error = result;
    return result;
}

int32_t bsp_wifi_start(const bsp_wifi_config_t *p_config)
{
    size_t ssid_length     = 0u;     /* Validated SSID size without NUL. */
    size_t password_length = 0u;     /* Validated PSK size without NUL. */
    esp_err_t result       = ESP_OK; /* Preserve the original setup failure. */
    wifi_config_t config   = {0};    /* No stale SDK fields from a previous session. */
    wifi_band_mode_t band  = WIFI_BAND_MODE_AUTO; /* STA channel 0 scans both bands. */

    if (p_config == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }
    ssid_length     = text_length(p_config->ssid, sizeof(p_config->ssid));
    password_length = text_length(p_config->password, sizeof(p_config->password));

    if (    (ssid_length == 0u)     /* An SSID is required in both modes. */
         || (ssid_length > 32u)     /* Fixed buffers must contain a terminator. */
         || (password_length > 63u) /* Only ASCII PSK/open modes are exposed. */
         || (    (password_length != 0u)
              && (password_length < 8u)) /* Reject short WPA keys. */
         || (    (p_config->mode != BSP_WIFI_STA)
              && (p_config->mode != BSP_WIFI_AP)) /* Reject unknown modes. */
         || (p_config->channel > 165u)            /* SDK validates the regulatory channel set. */
         || (    (p_config->mode == BSP_WIFI_AP)
              && (p_config->channel == 0u))) /* AP needs an explicit channel. */
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (initialized == false)
    {
        return ESP_ERR_INVALID_STATE;
    }
    result = bsp_wifi_stop();

    if (result != ESP_OK)
    {
        return result;
    }
    result = esp_wifi_set_storage(WIFI_STORAGE_RAM);

    if (result == ESP_OK)
    {
        result = esp_wifi_set_mode(p_config->mode == BSP_WIFI_AP ? WIFI_MODE_AP : WIFI_MODE_STA);
    }

    if (p_config->mode == BSP_WIFI_AP)
    {
        memcpy(config.ap.ssid, p_config->ssid, ssid_length);
        memcpy(config.ap.password, p_config->password, password_length);
        config.ap.ssid_len       = (uint8_t)ssid_length;
        config.ap.channel        = p_config->channel;
        config.ap.max_connection = 2u;
        config.ap.authmode = password_length == 0u ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    }
    else
    {
        memcpy(config.sta.ssid, p_config->ssid, ssid_length);
        memcpy(config.sta.password, p_config->password, password_length);
        config.sta.channel = p_config->channel;
        config.sta.threshold.authmode = password_length == 0u ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA2_PSK;
    }

    if (result == ESP_OK)
    {
        result = esp_wifi_set_config(p_config->mode == BSP_WIFI_AP ? WIFI_IF_AP : WIFI_IF_STA, &config);
    }
    memset(&config, 0, sizeof(config)); /* Do not retain a stack copy of credentials. */

    if (result == ESP_OK)
    {
        result = esp_wifi_start();

        if (result == ESP_OK)
        {
            status.mode = p_config->mode;
        }
    }

    if (p_config->channel != 0u)
    {
        band = p_config->channel > 14u ? WIFI_BAND_MODE_5G_ONLY : WIFI_BAND_MODE_2G_ONLY;
    }

    if (result == ESP_OK)
    {
        result = esp_wifi_set_band_mode(band); /* ESP-IDF requires a started interface. */
    }

    if (    result == ESP_OK
         && p_config->mode == BSP_WIFI_STA)
        result = esp_wifi_set_ps(WIFI_PS_NONE); /* Continuous uploads must not wait for power-save windows. */

    if (    (result == ESP_OK) /* Do not associate after a setup failure. */
         && (p_config->mode == BSP_WIFI_STA)) /* AP accepts peers instead of connecting. */
    {
        last_connect_us = esp_timer_get_time();
        result          = esp_wifi_connect();
    }

    if (result != ESP_OK)
    {
        (void)bsp_wifi_stop(); /* Keep active mode if cleanup itself fails; caller can retry stop. */
    }
    status.error = result;
    return result;
}

int32_t bsp_wifi_poll(void)
{
    esp_netif_ip_info_t info = {0};            /* SDK IPv4 snapshot. */
    esp_netif_t *p_netif     = p_access_point; /* Select the started mode's netif. */
    int64_t now_us           = esp_timer_get_time(); /* Monotonic retry deadline. */
    status.ready             = 0u;
    status.ipv4_be           = 0u;

    if (status.mode == BSP_WIFI_OFF)
    {
        return status.error;
    }

    if (status.mode == BSP_WIFI_STA)
    {
        unsigned state = atomic_load_explicit(&station_state, memory_order_acquire);
        p_netif        = p_station;

        if (state == 0u)
        {
            status.rssi_dbm = 0;

            if ((now_us - last_connect_us) >= BSP_WIFI_RETRY_US)
            {
                last_connect_us = now_us;
                status.error    = esp_wifi_connect();
            }
            return status.error;
        }

        if (state != 2u)
            return status.error;
    }
    status.error = esp_netif_get_ip_info(p_netif, &info);

    if (    (status.error == ESP_OK) /* Never publish an invalid SDK snapshot. */
         && (info.ip.addr != 0u))    /* Wait for DHCP/static IPv4 before opening listeners. */
    {
        status.ipv4_be = info.ip.addr;
        status.ready   = 1u;
    }
    return status.error;
}

int32_t bsp_wifi_refresh_rssi(void)
{
    wifi_ap_record_t access_point = {0}; /* Explicit diagnostic, not periodic transport work. */

    status.rssi_dbm = 0;

    if (    status.mode != BSP_WIFI_STA
         || atomic_load_explicit(&station_state, memory_order_acquire) == 0u)
        return ESP_OK;

    esp_err_t result = esp_wifi_sta_get_ap_info(&access_point);

    if (result == ESP_OK)
        status.rssi_dbm = access_point.rssi;
    status.error = result;
    return result;
}

void bsp_wifi_status_get(bsp_wifi_status_t *p_status)
{
    if (p_status != NULL)
    {
        *p_status = status;

        if (    p_status->mode == BSP_WIFI_STA
             && atomic_load_explicit(&station_state, memory_order_acquire) != 2u)
        {
            p_status->ready   = 0u;
            p_status->ipv4_be = 0u;
        }
    }
}
