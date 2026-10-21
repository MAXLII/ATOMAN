// SPDX-License-Identifier: MIT
/**
 * @file bsp_wifi.h
 * @brief ESP32-C5 radio ownership and nonblocking reconnect interface.
 * @details Part of the base project. One application task owns every public call.
 *          SDK event tasks own netif updates; status is queried from SDK objects.
 *          Credentials are kept in RAM. No protocol, shared RAM or firmware loader.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef BSP_WIFI_H
#define BSP_WIFI_H
#include <stdint.h>
#include <stdbool.h>

#define BSP_WIFI_CHANNEL_CONTROL 0u
#define BSP_WIFI_CHANNEL_UPLOAD  1u
typedef uint32_t (*bsp_wifi_packet_read_t)(uint8_t *p_data, uint32_t capacity);
typedef struct bsp_wifi_channel_status
{
    bool connected;
    uint32_t session;
    uint32_t error;
} bsp_wifi_channel_status_t;
/** Configure a TCP endpoint once, from task/init context. A non-NULL packet source
 *  makes it upload-only; BSP copies packets and retains partial-send tails. */
bool bsp_wifi_channel_configure(uint8_t channel, uint16_t port, bsp_wifi_packet_read_t p_read);
/** Bounded, nonblocking progress for both TCP endpoints. One SECTION task owns calls. */
void bsp_wifi_channel_poll(void);
void bsp_wifi_channel_reset(uint8_t channel);
void bsp_wifi_channel_status_get(uint8_t channel, bsp_wifi_channel_status_t *p_status);
int32_t bsp_wifi_channel_read(uint8_t channel, uint8_t *p_data, uint32_t capacity);
int32_t bsp_wifi_channel_write(uint8_t channel, const uint8_t *p_data, uint32_t length);

typedef enum
{
    BSP_WIFI_OFF = 0, /**< Radio stopped. */
    BSP_WIFI_STA,     /**< Join an existing access point. */
    BSP_WIFI_AP       /**< Provide a local access point. */
} BSP_WIFI_MODE_E;

typedef struct bsp_wifi_config
{
    BSP_WIFI_MODE_E mode; /**< STA or AP; use stop() for OFF. */
    char ssid[33];        /**< NUL-terminated SSID, 1..32 bytes. */
    char password[64];    /**< Empty for open, or 8..63 bytes for WPA2/WPA3 PSK. */
    uint8_t channel;      /**< STA: 0 scans both bands; AP: explicit channel. */
} bsp_wifi_config_t;

typedef struct bsp_wifi_status
{
    BSP_WIFI_MODE_E mode; /**< Successfully started mode, or OFF. */
    uint32_t ready;       /**< 1 when interface has IPv4 and STA is associated. */
    uint32_t ipv4_be;     /**< IPv4 address in network byte order; 0 when not ready. */
    int32_t rssi_dbm;     /**< Current STA signal strength; 0 when unavailable/AP. */
    int32_t error;        /**< Last setup, query or reconnect ESP-IDF error. */
} bsp_wifi_status_t;

/** @return ESP_OK or ESP-IDF error. Call before other Wi-Fi functions. */
int32_t bsp_wifi_init(void);
/** @param p_mac Destination for six station MAC bytes. @return ESP_OK or SDK error. */
int32_t bsp_wifi_mac_get(uint8_t p_mac[6]);
/** @param p_config Runtime credentials/mode, copied by SDK before return.
 * @return ESP_OK after starting association/AP, or ESP-IDF error.
 * @details Validates before stopping an existing connection. Startup/calibration
 *          can block; call from task context. This driver exclusively owns Wi-Fi.
 */
int32_t bsp_wifi_start(const bsp_wifi_config_t *p_config);
/** @return ESP_OK or ESP-IDF error. Sockets are separately owned by the caller. */
int32_t bsp_wifi_stop(void);
/** @brief Refresh status and retry disconnected STA at most once per 5 s.
 * @return Latest ESP-IDF error, or ESP_OK. Call periodically from the owner task.
 */
int32_t bsp_wifi_poll(void);
/** @param p_status Destination for the most recent poll snapshot. */
void bsp_wifi_status_get(bsp_wifi_status_t *p_status);
#endif /* BSP_WIFI_H */
