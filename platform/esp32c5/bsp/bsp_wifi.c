// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi.c
 * @brief ESP-IDF Wi-Fi/netif adapter with task-owned reconnect polling.
 * @details Part of the base project. One caller owns configuration and snapshots.
 *          Default SDK event handlers maintain DHCP/netif. No application callbacks.
 *          Setup may allocate through SDK; poll never waits for association/DHCP.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_wifi.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "esp_wifi.h"

#define BSP_WIFI_RETRY_US INT64_C(5000000) /**< Minimum reconnect spacing. */
static esp_netif_t *p_station;             /**< SDK-owned STA interface after init. */
static esp_netif_t *p_access_point;        /**< SDK-owned AP interface after init. */
static bool initialized;         /**< Driver resources have been acquired. */
static bsp_wifi_status_t status; /**< Only the owner task writes this snapshot. */
static int64_t last_connect_us;  /**< Last association request in monotonic microseconds. */

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
    }

    if (result != ESP_OK)
    {
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
    esp_netif_ip_info_t info      = {0};            /* SDK IPv4 snapshot. */
    wifi_ap_record_t access_point = {0};            /* Current STA association. */
    esp_netif_t *p_netif          = p_access_point; /* Select the started mode's netif. */
    int64_t now_us                = esp_timer_get_time(); /* Monotonic retry deadline. */
    status.ready                  = 0u;
    status.ipv4_be                = 0u;
    status.rssi_dbm               = 0;

    if (status.mode == BSP_WIFI_OFF)
    {
        return status.error;
    }

    if (status.mode == BSP_WIFI_STA)
    {
        p_netif = p_station;

        if (esp_wifi_sta_get_ap_info(&access_point) != ESP_OK)
        {
            if ((now_us - last_connect_us) >= BSP_WIFI_RETRY_US)
            {
                last_connect_us = now_us;
                status.error    = esp_wifi_connect();
            }
            return status.error;
        }
        status.rssi_dbm = access_point.rssi;
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

void bsp_wifi_status_get(bsp_wifi_status_t *p_status)
{
    if (p_status != NULL)
    {
        *p_status = status;
    }
}
