// SPDX-License-Identifier: MIT
/**
 * @file fake_sdk.h
 * @brief Minimal test-only SDK surface used by the real Wi-Fi and USB drivers.
 * @details Part of base. No production source depends on this header.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef FAKE_SDK_H
#define FAKE_SDK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef int32_t esp_err_t;
#define ESP_OK                     0    /* Successful fake operation. */
#define ESP_FAIL                   (-1) /* Injected SDK failure. */
#define ESP_ERR_INVALID_ARG        258  /* Argument error. */
#define ESP_ERR_INVALID_STATE      259  /* State error. */
#define ESP_ERR_NO_MEM             257  /* Allocation error. */
#define ESP_ERR_TIMEOUT            263  /* Expected disconnected USB timeout. */
#define WIFI_STORAGE_RAM           0    /* No persisted credentials. */
#define WIFI_MODE_STA              1    /* Station mode. */
#define WIFI_MODE_AP               2    /* Access-point mode. */
#define WIFI_IF_STA                0    /* Station interface. */
#define WIFI_IF_AP                 1    /* Access-point interface. */
#define WIFI_AUTH_OPEN             0    /* Open network. */
#define WIFI_AUTH_WPA2_PSK         3    /* WPA2 threshold. */
#define WIFI_INIT_CONFIG_DEFAULT() {0}  /* Deterministic test initialization. */
#define WIFI_PS_NONE               0
typedef struct
{
    uint8_t app_elf_sha256[32];
} esp_app_desc_t;
const esp_app_desc_t *esp_app_get_description(void);
uint32_t esp_cpu_intr_get_enabled_mask(void);
void esp_cpu_intr_disable(uint32_t mask);
void esp_cpu_intr_enable(uint32_t mask);
typedef enum
{
    WIFI_BAND_MODE_2G_ONLY = 1,
    WIFI_BAND_MODE_5G_ONLY,
    WIFI_BAND_MODE_AUTO
} wifi_band_mode_t;
typedef struct esp_netif
{
    uint32_t id; /**< Test interface identity. */
} esp_netif_t;
typedef struct
{
    uint32_t addr; /**< Network-order IPv4. */
} fake_ip_t;
typedef struct
{
    fake_ip_t ip; /**< Fake IPv4 snapshot. */
} esp_netif_ip_info_t;
typedef struct
{
    int8_t rssi; /**< Test dBm sample. */
} wifi_ap_record_t;
typedef struct
{
    int nvs_enable; /**< Persisted storage flag. */
} wifi_init_config_t;
typedef struct
{
    struct
    {
        uint8_t ssid[32];       /**< AP SSID. */
        uint8_t password[64];   /**< AP password. */
        uint8_t ssid_len;       /**< AP name length. */
        uint8_t channel;        /**< AP channel. */
        uint8_t max_connection; /**< Peer capacity. */
        int authmode;           /**< AP authentication. */
    } ap; /**< AP fields. */
    struct
    {
        uint8_t ssid[32];     /**< STA SSID. */
        uint8_t password[64]; /**< STA password. */
        uint8_t channel;      /**< STA channel. */
        struct
        {
            int authmode; /**< STA minimum authentication. */
        } threshold;      /**< Authentication policy. */
    } sta; /**< STA fields. */
} wifi_config_t;
typedef struct
{
    size_t tx_buffer_size; /**< Fake TX allocation size. */
    size_t rx_buffer_size; /**< Fake RX allocation size. */
} usb_serial_jtag_driver_config_t;
esp_err_t esp_netif_init(void);
esp_err_t esp_event_loop_create_default(void);
esp_netif_t *esp_netif_create_default_wifi_sta(void);
esp_netif_t *esp_netif_create_default_wifi_ap(void);
void esp_netif_destroy_default_wifi(esp_netif_t *p_netif);
esp_err_t esp_wifi_init(const wifi_init_config_t *p_config);
esp_err_t esp_wifi_get_mac(int interface, uint8_t p_mac[6]);
esp_err_t esp_wifi_stop(void);
esp_err_t esp_wifi_set_storage(int storage);
esp_err_t esp_wifi_set_mode(int mode);
esp_err_t esp_wifi_set_config(int interface, wifi_config_t *p_config);
esp_err_t esp_wifi_start(void);
esp_err_t esp_wifi_set_band_mode(wifi_band_mode_t mode);
esp_err_t esp_wifi_set_ps(int mode);
esp_err_t esp_wifi_connect(void);
esp_err_t esp_wifi_sta_get_ap_info(wifi_ap_record_t *p_record);
esp_err_t esp_netif_get_ip_info(esp_netif_t *p_netif, esp_netif_ip_info_t *p_info);
int64_t esp_timer_get_time(void);
void esp_restart(void);
bool usb_serial_jtag_is_driver_installed(void);
esp_err_t usb_serial_jtag_wait_tx_done(uint32_t ticks);
void usb_serial_jtag_vfs_use_nonblocking(void);
esp_err_t usb_serial_jtag_driver_uninstall(void);
esp_err_t usb_serial_jtag_driver_install(usb_serial_jtag_driver_config_t *p_config);
int usb_serial_jtag_read_bytes(void *p_buffer, uint32_t length, uint32_t ticks);
int usb_serial_jtag_write_bytes(const void *p_buffer, size_t length, uint32_t ticks);
#endif /* FAKE_SDK_H */
