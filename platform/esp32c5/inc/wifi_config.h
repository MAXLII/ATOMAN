// SPDX-License-Identifier: MIT
/**
 * @file wifi_config.h
 * @brief Credential-free defaults for the ESP32-C5 base demo.
 * @details Part of base. A private build-directory header can override this file.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H
#define ESP32C5_WIFI_SSID     "" /**< Empty SSID leaves the radio stopped. */
#define ESP32C5_WIFI_PASSWORD "" /**< Credentials are supplied only by the local build. */
#define ESP32C5_WIFI_CHANNEL  0u /**< Scan both bands in STA mode. */
#endif /* WIFI_CONFIG_H */
