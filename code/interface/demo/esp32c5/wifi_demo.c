// SPDX-License-Identifier: MIT
/**
 * @file wifi_demo.c
 * @brief Demo-only Wi-Fi control/status and the FRAME binary loopback command.
 * @details Part of base. Demo Interface adapts Shell/COMM commands to BSP. Commands and
 *          transport callbacks execute on the same SECTION/main task.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_wifi.h"
#include "esp_heap_caps.h"
#include "wifi_config.h"
#include "comm.h"
#include "shell.h"
#include <stddef.h>

static uint32_t wifi_enable = sizeof(ESP32C5_WIFI_SSID) > 1u ? 1u : 0u; /**< Demo radio request. */
static int32_t wifi_control_error; /**< Most recent demo start/stop result. */

/** @param my_printf Requesting Link output, unused for binary write acknowledgments. */
static void wifi_changed(DEC_MY_PRINTF)
{
    bsp_wifi_config_t config = {
        .mode     = BSP_WIFI_STA,
        .ssid     = ESP32C5_WIFI_SSID,
        .password = ESP32C5_WIFI_PASSWORD,
        .channel  = ESP32C5_WIFI_CHANNEL,
    }; /* Private build-time credentials, copied only into SDK RAM. */
    (void)my_printf;

    if (wifi_enable == 1u)
    {
        wifi_control_error = bsp_wifi_start(&config);
    }
    else
    {
        wifi_control_error = bsp_wifi_stop();
    }
}

/** @param my_printf Requesting Link output. */
static void wifi(DEC_MY_PRINTF)
{
    bsp_wifi_status_t status = {0}; /* Latest task-owned driver snapshot. */
    uint32_t address         = 0u;  /* Host-independent bytes from the network-order IPv4 value. */
    const uint8_t *p_bytes   = (const uint8_t *)&address; /* Object representation is valid byte access. */
    bsp_wifi_status_get(&status);
    address = status.ipv4_be;

    if (    (my_printf != NULL)             /* Require a bound Link. */
         && (my_printf->my_printf != NULL)) /* Text output is optional on generic Links. */
    {
        my_printf->my_printf("wifi mode=%u ready=%lu ip=%u.%u.%u.%u rssi=%ld error=%ld control=%ld\r\n",
                             (unsigned)status.mode,
                             (unsigned long)status.ready,
                             (unsigned)p_bytes[0],
                             (unsigned)p_bytes[1],
                             (unsigned)p_bytes[2],
                             (unsigned)p_bytes[3],
                             (long)status.rssi_dbm,
                             (long)status.error,
                             (long)wifi_control_error);
        my_printf->my_printf("wifi internal_heap=%lu poll_us=%lu write_us=%lu tx_bytes=%lu blocked=%lu\r\n",
                             (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                             (unsigned long)g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_poll_gap_us,
                             (unsigned long)g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].max_write_us,
                             (unsigned long)g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].tx_bytes,
                             (unsigned long)g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].backpressure);
    }
}

/** @param p_context Validated packet from the generic COMM registration. @param my_printf Requesting Link output. */
static void loopback(void *p_context, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = p_context; /* Recover the generic framework payload at its boundary. */
    section_packform_t reply   = {0};       /* Direct same-command acknowledgment. */

    if (p_pack == NULL)
    {
        return;
    }
    reply        = *p_pack;
    reply.src    = p_pack->dst;
    reply.d_src  = p_pack->d_dst;
    reply.dst    = p_pack->src;
    reply.d_dst  = p_pack->d_src;
    reply.is_ack = 1u;
    comm_send_data(&reply, my_printf);
}

REG_SHELL_VAR(WIFI_ENABLE, wifi_enable, SHELL_UINT32, 1u, 0u, wifi_changed, SHELL_STA_NULL)
REG_SHELL_CMD(wifi, wifi)
REG_COMM(0x01, 0x17, loopback)
