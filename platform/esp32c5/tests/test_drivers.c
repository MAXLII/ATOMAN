// SPDX-License-Identifier: MIT
/**
 * @file test_drivers.c
 * @brief Fault injection for production Wi-Fi setup/reconnect and USB I/O ownership.
 * @details Part of base. Tests invoke the real BSP APIs; only SDK calls are faked.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "fake_sdk.h"
#include "bsp_wifi.h"
#include "bsp_usart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static esp_netif_t station  = {.id = 1u}; /**< Test STA identity. */
static esp_netif_t ap       = {.id = 2u}; /**< Test AP identity. */
static esp_err_t init_error = ESP_FAIL;   /**< Fail the first SDK init to check cleanup/retry. */
static esp_err_t stop_error;    /**< Injectable stop result. */
static esp_err_t band_error;    /**< Injectable post-start failure. */
static esp_err_t connect_error; /**< Injectable association failure. */
static uint32_t destroyed;      /**< Number of retired netif resources. */
static uint32_t connections;    /**< Association requests, including retries. */
static int64_t now_us;          /**< Deterministic monotonic clock. */
static bool associated;         /**< Fake association state. */
static bool installed   = true; /**< Simulate a preinstalled SDK console driver. */
static int write_result = 3;    /**< Force a partial USB acceptance. */
static wifi_band_mode_t band;   /**< Last selected radio band. */

esp_err_t esp_netif_init(void)
{
    return ESP_OK;
}
esp_err_t esp_event_loop_create_default(void)
{
    return ESP_ERR_INVALID_STATE;
}
esp_netif_t *esp_netif_create_default_wifi_sta(void)
{
    return &station;
}
esp_netif_t *esp_netif_create_default_wifi_ap(void)
{
    return &ap;
}
void esp_netif_destroy_default_wifi(esp_netif_t *p_netif)
{
    assert(p_netif != NULL);
    ++destroyed;
}
esp_err_t esp_wifi_init(const wifi_init_config_t *p_config)
{
    assert(p_config->nvs_enable == 0);
    return init_error;
}
esp_err_t esp_wifi_stop(void)
{
    return stop_error;
}
esp_err_t esp_wifi_set_storage(int storage)
{
    assert(storage == WIFI_STORAGE_RAM);
    return ESP_OK;
}
esp_err_t esp_wifi_set_mode(int mode)
{
    assert(    mode == WIFI_MODE_STA
            || mode == WIFI_MODE_AP);
    return ESP_OK;
}
esp_err_t esp_wifi_set_config(int interface, wifi_config_t *p_config)
{
    assert(p_config != NULL);
    assert(    interface == WIFI_IF_STA
            || interface == WIFI_IF_AP);
    return ESP_OK;
}
esp_err_t esp_wifi_start(void)
{
    return ESP_OK;
}
esp_err_t esp_wifi_set_band_mode(wifi_band_mode_t mode)
{
    band = mode;
    return band_error;
}
esp_err_t esp_wifi_connect(void)
{
    ++connections;
    return connect_error;
}
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *p_record)
{
    p_record->rssi = -42;
    return associated == true ? ESP_OK : ESP_FAIL;
}
esp_err_t esp_netif_get_ip_info(esp_netif_t *p_netif, esp_netif_ip_info_t *p_info)
{
    assert(p_netif != NULL);
    p_info->ip.addr = UINT32_C(0x1101a8c0);
    return ESP_OK;
}
int64_t esp_timer_get_time(void)
{
    return now_us;
}
bool usb_serial_jtag_is_driver_installed(void)
{
    return installed;
}
esp_err_t usb_serial_jtag_wait_tx_done(uint32_t ticks)
{
    assert(ticks == 20u);
    return ESP_ERR_TIMEOUT;
}
void usb_serial_jtag_vfs_use_nonblocking(void)
{
}
esp_err_t usb_serial_jtag_driver_uninstall(void)
{
    installed = false;
    return ESP_OK;
}
esp_err_t usb_serial_jtag_driver_install(usb_serial_jtag_driver_config_t *p_config)
{
    assert(p_config->rx_buffer_size == 2048u);
    assert(p_config->tx_buffer_size == 2048u);
    installed = true;
    return ESP_OK;
}
int usb_serial_jtag_read_bytes(void *p_buffer, uint32_t length, uint32_t ticks)
{
    assert(length != 0u);
    assert(ticks == 0u);
    *(uint8_t *)p_buffer = 0xe8u;
    return 1;
}
int usb_serial_jtag_write_bytes(const void *p_buffer, size_t length, uint32_t ticks)
{
    assert(p_buffer != NULL);
    assert(length <= BSP_USART_TRANSFER_MAX);
    assert(ticks == 20u);
    return write_result;
}

/** @return 0 after all real-driver failure paths pass. */
int main(void)
{
    bsp_wifi_config_t config = {.mode = BSP_WIFI_STA, .ssid = "test", .password = "12345678", .channel = 0u};
    bsp_wifi_config_t invalid  = {0}; /* Independent malformed input. */
    bsp_wifi_status_t snapshot = {0}; /* Owner-visible state after each transition. */
    uint8_t bytes[8]           = {0}; /* Serial partial-transfer test buffer. */
    assert(bsp_usart_read(bytes, 1u) == -1);
    assert(bsp_usart_write(bytes, 1u) == -1);
    assert(bsp_usart_init() == ESP_OK);
    assert(bsp_usart_init() == ESP_OK);
    assert(bsp_usart_write(NULL, 1u) == -1);
    assert(bsp_usart_write(bytes, 513u) == -1);
    assert(bsp_usart_write(bytes, 8u) == 3);
    write_result = 0;
    assert(bsp_usart_write(bytes, 8u) == 0);
    assert(    bsp_usart_dbg_rx_get_byte(bytes) == 1u
            && bytes[0] == 0xe8u);
    assert(bsp_usart_read(bytes, 0u) == -1);
    assert(bsp_wifi_start(&config) == ESP_ERR_INVALID_STATE);
    assert(bsp_wifi_init() == ESP_FAIL);
    assert(destroyed == 2u);
    init_error = ESP_OK;
    assert(bsp_wifi_init() == ESP_OK);
    assert(bsp_wifi_init() == ESP_OK);
    assert(bsp_wifi_start(NULL) == ESP_ERR_INVALID_ARG);
    assert(bsp_wifi_start(&config) == ESP_OK);
    assert(    connections == 1u
            && band == WIFI_BAND_MODE_AUTO);
    invalid = config;
    memset(invalid.ssid, 'x', sizeof(invalid.ssid));
    assert(bsp_wifi_start(&invalid) == ESP_ERR_INVALID_ARG);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.mode == BSP_WIFI_STA);
    invalid             = config;
    invalid.password[1] = '\0';
    assert(bsp_wifi_start(&invalid) == ESP_ERR_INVALID_ARG);
    assert(bsp_wifi_poll() == ESP_OK);
    now_us = INT64_C(4999999);
    assert(    bsp_wifi_poll() == ESP_OK
            && connections == 1u);
    now_us = INT64_C(5000000);
    assert(    bsp_wifi_poll() == ESP_OK
            && connections == 2u);
    associated = true;
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.ready == 1u
            && snapshot.rssi_dbm == -42
            && snapshot.ipv4_be != 0u);
    associated = false;
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.ready == 0u
            && snapshot.ipv4_be == 0u);
    stop_error = ESP_FAIL;
    assert(bsp_wifi_stop() == ESP_FAIL);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.mode == BSP_WIFI_STA);
    stop_error = ESP_OK;
    assert(bsp_wifi_stop() == ESP_OK);
    band_error = ESP_FAIL;
    assert(bsp_wifi_start(&config) == ESP_FAIL);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.mode == BSP_WIFI_OFF
            && snapshot.error == ESP_FAIL);
    band_error     = ESP_OK;
    config.mode    = BSP_WIFI_AP;
    config.channel = 6u;
    assert(    bsp_wifi_start(&config) == ESP_OK
            && band == WIFI_BAND_MODE_2G_ONLY);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 1u);
    assert(bsp_wifi_stop() == ESP_OK);
    config.mode    = BSP_WIFI_STA;
    config.channel = 157u;
    connect_error  = ESP_FAIL;
    assert(    bsp_wifi_start(&config) == ESP_FAIL
            && band == WIFI_BAND_MODE_5G_ONLY);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.mode == BSP_WIFI_OFF);
    puts(
        "PASS production Wi-Fi validation, init rollback, band selection, retry deadline, status, stop failure; USB handover, bounds and partial TX");
    return 0;
}
