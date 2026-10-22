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
#include "bsp_srtos.h"
#include "wifi_config.h"
#include "section.h"
#include "esp_err.h"
#include "esp_app_desc.h"

void app_main(void)
{
    _Alignas(16) uint32_t runtime_stack[1024] = {0}; /* Shared SRTOS stack inside the SDK host stack bounds. */
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
    bsp_srtos_bind(section_riscv_irq_request, section_task_preempt_lock, section_task_preempt_unlock);
    bsp_srtos_init(); /* Adapt only the SDK interrupt exit and external-call boundary. */
    section_init();   /* Use the current base initialization/demo registrations. */
    section_riscv_run(runtime_stack, 1024u); /* One runtime stack, bounded suspended-context pool. */
}
