// SPDX-License-Identifier: MIT
/**
 * @file comm_discovery.c
 * @brief FRAME discovery replies and the board UDP echo service.
 * @details Part of the ESP32-C5 BSP, serviced by CommLink polling.
 *          One pending datagram is retried on backpressure and cleared on network loss.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "comm_discovery.h"
#include "bsp_comm.h"
#include "bsp_wifi.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
static uint8_t udp_data[512];    /**< One pending reply, never shared with TCP or scope. */
static uint32_t udp_size;        /**< Bytes in the pending reply. */
static bsp_wifi_peer_t udp_peer; /**< Destination of the pending reply. */
static uint32_t session_ip;      /**< Drop old datagrams when network identity changes. */
static const char *firmware_version = ""; /**< Stable application version supplied at boot. */

void comm_discovery_init(const char *p_version)
{
    firmware_version = p_version != NULL ? p_version : "";
}
void comm_discovery_poll(void)
{
    bsp_wifi_status_t status = {0};
    int32_t received         = -1;
    bsp_wifi_status_get(&status);

    if (    (status.ready == 0u)
         || (session_ip != status.ipv4_be))
    {
        udp_size   = 0u;
        session_ip = status.ipv4_be;
    }

    if (status.ready != 0u)
    {
        if (udp_size == 0u)
        {
            received = bsp_comm_udp_read(udp_data, sizeof(udp_data), &udp_peer);

            if (received > 0)
            {
                udp_size = (uint32_t)received;

                if (    received == (int32_t)(sizeof("FRAME_DISCOVER_V1") - 1u)
                     && memcmp(udp_data, "FRAME_DISCOVER_V1", sizeof("FRAME_DISCOVER_V1") - 1u) == 0)
                {
                    uint8_t mac[6] = {0};

                    if (bsp_wifi_mac_get(mac) == 0)
                    {
                        int size = snprintf(
                            (char *)udp_data,
                            sizeof(udp_data),
                            "FRAME_DEVICE_V1;name=ESP32-C5;ip=%u.%u.%u.%u;tcp_port=5000;mac=%02X:%02X:%02X:%02X:%02X:%02X;fw_version=%s;protocol_version=1",
                            (unsigned)(status.ipv4_be & 255u),
                            (unsigned)((status.ipv4_be >> 8u) & 255u),
                            (unsigned)((status.ipv4_be >> 16u) & 255u),
                            (unsigned)((status.ipv4_be >> 24u) & 255u),
                            (unsigned)mac[0],
                            (unsigned)mac[1],
                            (unsigned)mac[2],
                            (unsigned)mac[3],
                            (unsigned)mac[4],
                            (unsigned)mac[5],
                            firmware_version);
                        udp_size = size > 0
                                        && (size_t)size < sizeof(udp_data)
                                     ? (uint32_t)size
                                     : 0u;
                    }
                    else
                        udp_size = 0u;
                }
            }
        }

        if (udp_size != 0u)
        {
            received = bsp_comm_udp_write(udp_data, udp_size, &udp_peer);

            if (    (received >= 0) /* Whole UDP datagram was accepted. */
                 || (    (errno != EAGAIN)
                      && (errno != EWOULDBLOCK))) /* Retire only permanent errors. */
            {
                udp_size = 0u;
            }
        }
    }
}
