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
#include "bsp_system.h"
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
static uint32_t ap_queries;     /**< Blocking radio RPCs, forbidden in periodic polling. */
static uint32_t driver_deinitializations;
static uint32_t event_unregistrations;
static bool fail_ip_registration;
static esp_event_handler_t wifi_handler;
static esp_event_handler_t ip_handler;
const char fake_wifi_event = 'w';
const char fake_ip_event   = 'i';
static bool reset_called; /**< SDK reset invocation observed by the host. */
static uint32_t interrupt_mask = 0xA5u;
static esp_app_desc_t descriptor;
uint8_t _data_start[64], _bss_end[1];
uint32_t esp_cpu_intr_get_enabled_mask(void)
{
    return interrupt_mask;
}
void esp_cpu_intr_disable(uint32_t mask)
{
    interrupt_mask &= ~mask;
}
void esp_cpu_intr_enable(uint32_t mask)
{
    interrupt_mask |= mask;
}
const esp_app_desc_t *esp_app_get_description(void)
{
    return &descriptor;
}
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
esp_err_t esp_event_handler_register(esp_event_base_t base,
                                     int32_t id,
                                     esp_event_handler_t handler,
                                     void *p_context)
{
    assert(    id == ESP_EVENT_ANY_ID
            && handler != NULL
            && p_context == NULL);

    if (base == WIFI_EVENT)
        wifi_handler = handler;
    else
    {
        assert(base == IP_EVENT);

        if (fail_ip_registration)
            return ESP_FAIL;
        ip_handler = handler;
    }
    return ESP_OK;
}
esp_err_t esp_event_handler_unregister(esp_event_base_t base, int32_t id, esp_event_handler_t handler)
{
    assert(    base == WIFI_EVENT
            && id == ESP_EVENT_ANY_ID
            && handler == wifi_handler);
    wifi_handler = NULL;
    ++event_unregistrations;
    return ESP_OK;
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
esp_err_t esp_wifi_deinit(void)
{
    ++driver_deinitializations;
    return ESP_OK;
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
    ++ap_queries;
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
esp_err_t esp_wifi_set_ps(int mode)
{
    assert(mode == WIFI_PS_NONE);
    return ESP_OK;
}
esp_err_t usb_serial_jtag_wait_tx_done(uint32_t ticks)
{
    assert(ticks == 0u);
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
    assert(ticks == 0u);
    return write_result;
}

/** @brief Supply the station identity through the SDK boundary. */
esp_err_t esp_wifi_get_mac(int interface, uint8_t p_mac[6])
{
    assert(interface == WIFI_IF_STA);
    memset(p_mac, 0x12, 6u);
    return ESP_OK;
}

/** Record the SDK reset request without terminating the host test. */
size_t heap_caps_get_free_size(uint32_t caps)
{
    assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    return 1048576u;
}
size_t heap_caps_get_minimum_free_size(uint32_t caps)
{
    assert(caps == (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    return 524288u;
}
void esp_restart(void)
{
    reset_called = true;
}

/** @return 0 after all real-driver failure paths pass. */
int main(void)
{
    now_us = INT64_C(4294967395);
    assert(    bsp_system_time_us_get() == 99u
            && bsp_system_tick_100us_get() == 42949673u);
    now_us = INT64_C(429496729699);
    assert(bsp_system_tick_100us_get() == 0u);
    ++now_us;
    assert(bsp_system_tick_100us_get() == 1u);
    now_us = 0;
    assert(bsp_system_internal_heap_free() == 1048576u);
    assert(bsp_system_internal_heap_minimum() == 524288u);
    bsp_system_reset();
    assert(reset_called);
    uint32_t saved = bsp_system_critical_enter();
    assert(    saved == 0xA5u
            && interrupt_mask == 0u);
    uint32_t nested = bsp_system_critical_enter();
    bsp_system_critical_exit(nested);
    assert(interrupt_mask == 0u);
    bsp_system_critical_exit(saved);
    assert(interrupt_mask == 0xA5u);
    memset(descriptor.app_elf_sha256, 0x5A, 32u);
    uint8_t hash[32];
    bsp_system_firmware_identity(hash);
    assert(memcmp(hash, descriptor.app_elf_sha256, 32u) == 0);
    assert(bsp_system_sample_address(0u, 4u) == NULL);
    puts("PASS board clock unit conversion and 32-bit rollover; reset SDK binding");
    uint8_t mac[6] = {0};
    assert(bsp_wifi_mac_get(NULL) == ESP_ERR_INVALID_ARG);
    assert(    bsp_wifi_mac_get(mac) == ESP_OK
            && mac[5] == 0x12u);
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
    init_error           = ESP_OK;
    fail_ip_registration = true;
    assert(bsp_wifi_init() == ESP_FAIL);
    assert(    destroyed == 4u
            && driver_deinitializations == 1u
            && event_unregistrations == 1u);
    assert(    wifi_handler == NULL
            && ip_handler == NULL);
    fail_ip_registration = false;
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
    assert(ap_queries == 0u);
    associated = true;
    wifi_handler(NULL, WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, NULL);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 0u); /* Association alone must not publish stale DHCP state. */
    ip_event_got_ip_t ip_event = {.esp_netif = &ap};
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 0u); /* Ignore other netifs. */
    ip_event.esp_netif = &station;
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.ready == 1u
            && snapshot.rssi_dbm == 0
            && snapshot.ipv4_be != 0u);
    assert(ap_queries == 0u);
    assert(    bsp_wifi_refresh_rssi() == ESP_OK
            && ap_queries == 1u);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.rssi_dbm == -42);

    for (uint32_t count = 0u; count < 1000u; ++count)
        assert(bsp_wifi_poll() == ESP_OK);
    assert(ap_queries == 1u); /* Periodic work preserves cached RSSI without another RPC. */
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_LOST_IP, NULL);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.ready == 0u
            && snapshot.ipv4_be == 0u
            && connections == 2u);
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 1u);
    associated = false;
    wifi_handler(NULL, WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, NULL);
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event); /* Stale IP cannot revive a disconnected STA. */
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_LOST_IP, NULL);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(    snapshot.ready == 0u
            && snapshot.ipv4_be == 0u);
    assert(    bsp_wifi_refresh_rssi() == ESP_OK
            && ap_queries == 1u);
    now_us = INT64_C(10000000);
    assert(    bsp_wifi_poll() == ESP_OK
            && connections == 3u);
    associated = true;
    wifi_handler(NULL, WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, NULL);
    ip_handler(NULL, IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 1u);
    wifi_handler(NULL, WIFI_EVENT, WIFI_EVENT_STA_STOP, NULL);
    assert(bsp_wifi_poll() == ESP_OK);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.ready == 0u);
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
    assert(    bsp_wifi_refresh_rssi() == ESP_OK
            && ap_queries == 1u);
    assert(bsp_wifi_stop() == ESP_OK);
    config.mode    = BSP_WIFI_STA;
    config.channel = 157u;
    connect_error  = ESP_FAIL;
    assert(    bsp_wifi_start(&config) == ESP_FAIL
            && band == WIFI_BAND_MODE_5G_ONLY);
    bsp_wifi_status_get(&snapshot);
    assert(snapshot.mode == BSP_WIFI_OFF);
    puts(
        "PASS production Wi-Fi event state, DHCP loss/recovery, RPC-free periodic poll, explicit RSSI, init rollback, band selection, retry deadline, stop failure; USB handover, bounds and partial TX");
    return 0;
}
