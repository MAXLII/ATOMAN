// SPDX-License-Identifier: MIT
/**
 * @file app_main.c
 * @brief Start the base demo over USB and optional Wi-Fi, with no other board peripherals.
 * @details Part of base. IDF initializes clocks/FreeRTOS; this single owner task
 *          runs SECTION cooperatively and yields at least 1 tick per iteration.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "esp32c5_platform.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "wifi_config.h"
#include "section.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    bsp_wifi_config_t config = {
        .mode     = BSP_WIFI_STA,
        .ssid     = ESP32C5_WIFI_SSID,
        .password = ESP32C5_WIFI_PASSWORD,
        .channel  = ESP32C5_WIFI_CHANNEL,
    }; /* Credentials are local to this boot and the SDK RAM configuration. */
    ESP_ERROR_CHECK(bsp_usart_init()); /* Take exclusive ownership of raw USB. */
    ESP_ERROR_CHECK(bsp_wifi_init());  /* Initialize only the networking resources. */

    if (config.ssid[0] != '\0')
    {
        ESP_ERROR_CHECK(bsp_wifi_start(&config)); /* Start asynchronous STA association. */
    }
    section_init(); /* Use the current base initialization/demo registrations. */

    for (;;)
    {
        esp32c5_link_poll(); /* Move queued bytes and maintain TCP/UDP sessions. */
        run_task();          /* Dispatch demo tasks and registered Link parsers serially. */
        vTaskDelay(1u);      /* Allow SDK Wi-Fi, idle and watchdog tasks to run. */
    }
}
