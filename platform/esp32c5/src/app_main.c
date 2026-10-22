// SPDX-License-Identifier: MIT
/**
 * @file app_main.c
 * @brief Start the base demo over USB, optional Wi-Fi and the board periodic interrupt.
 * @details Part of base. SDK starts the runtime; application work is registered
 *          with SECTION and socket I/O remains nonblocking.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "app_main.h"
#include "comm_discovery.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "wifi_config.h"
#include "section.h"
#include "esp_err.h"
#include "esp_app_desc.h"
#include <sys/select.h>

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
    comm_discovery_init(esp_app_get_description()->version);
    section_init(); /* Use the current base initialization/demo registrations. */

    for (;;)
    {
        run_task(); /* SECTION owns services, transports and Link dispatch. */
        struct timeval idle = {.tv_sec = 0, .tv_usec = 1000};
        (void)select(0, NULL, NULL, NULL, &idle); /* SDK wait, with no busy sub-tick delay. */
    }
}
