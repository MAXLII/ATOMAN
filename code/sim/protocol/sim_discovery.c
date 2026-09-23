// SPDX-License-Identifier: MIT
/**
 * @file sim_discovery.c
 * @brief FRAME probe replies from BSP-registered advertisements.
 * @details Static registration; serialized simulation callbacks; bounded storage.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the project root.
 */
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#undef REG_LINK
#include "sim_discovery.h"
#define SIM_RETRY_MS 100u
static SOCKET discovery             = INVALID_SOCKET;
static uint32_t discovery_retry_ms  = 0u;
static uint32_t ready               = 0u;
static section_item_t *p_registered = NULL;

static SOCKET open_listener(void)
{
    SOCKET result              = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in address = {0};
    u_long nonblocking         = 1u;
    int exclusive              = 1;

    if (result == INVALID_SOCKET)
        return result;
    address.sin_family                  = AF_INET;
    const sim_discovery_reg_t *p_config = p_registered->p_obj;
    address.sin_port                    = htons(p_config->udp_port);
    address.sin_addr.s_addr             = htonl(INADDR_ANY);
    (void)setsockopt(result, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&exclusive, sizeof(exclusive));

    if (    (ioctlsocket(result, (long)FIONBIO, &nonblocking) != 0)
         || (bind(result, (const struct sockaddr *)&address, sizeof(address)) != 0))
    {
        (void)closesocket(result);
        result = INVALID_SOCKET;
    }
    return result;
}

/** @param p_remote Probe sender. @param p_reg Advertised node identity and TCP port. */
static void discovery_reply(const struct sockaddr_in *p_remote, const sim_discovery_reg_t *p_reg)
{
    char response[192] = {0}; /* FRAME discovery record with stable identity. */
    int count          = snprintf(response,
                         sizeof(response),
                         "FRAME_DEVICE_V1;name=%s-%02X;ip=127.0.0.1;tcp_port=%u;"
                                  "mac=02:00:00:00:00:%02X;fw_version=1.5.1;protocol_version=1",
                         p_reg->p_name,
                         (unsigned int)p_reg->node_addr,
                         (unsigned int)p_reg->tcp_port,
                         (unsigned int)p_reg->node_addr);

    if (    (count > 0)
         && ((size_t)count < sizeof(response)))
    {
        (void)sendto(discovery, response, count, 0, (const struct sockaddr *)p_remote, sizeof(*p_remote));
    }
}

/** @param now Wall-clock time for bounded discovery bind retries. */
static void discovery_poll(uint32_t now)
{
    if (    (ready == 0u)
         || (p_registered == NULL))
        return;

    if (discovery == INVALID_SOCKET)
    {
        if ((uint32_t)(now - discovery_retry_ms) < SIM_RETRY_MS)
            return;
        discovery_retry_ms = now;
        discovery          = open_listener();
    }

    if (discovery == INVALID_SOCKET)
        return;

    for (uint32_t i = 0u; i < 8u; ++i)
    {
        char request[64]          = {0};            /* Bounded candidate probe. */
        struct sockaddr_in remote = {0};            /* Reply endpoint. */
        int length                = sizeof(remote); /* Winsock address length. */
        int received = recvfrom(discovery, request, sizeof(request), 0, (struct sockaddr *)&remote, &length);

        if (received == SOCKET_ERROR)
            break;

        if (    (received == 18)
             && (memcmp(request, "FRAME_DISCOVER_V1", 17u) == 0)
             && (request[17] == '\0'))
        {
            received = 17; /* Accept both raw and NUL-terminated discovery probes. */
        }

        if (    (received == 17)
             && (memcmp(request, "FRAME_DISCOVER_V1", 17u) == 0))
        {
            for (const section_item_t *p_item = p_registered; p_item != NULL; p_item = p_item->p_next)
            {
                discovery_reply(&remote, p_item->p_obj);
            }
        }
    }
}

void sim_discovery_stop(void)
{
    if (discovery != INVALID_SOCKET)
        (void)closesocket(discovery);
    discovery = INVALID_SOCKET;

    if (ready == 1u)
        (void)WSACleanup();
    ready = 0u;
}
static void sim_discovery_init(void)
{
    WSADATA data = {0};
    sim_discovery_stop();
    p_registered = section_collect(SECTION_SIM_DISCOVERY);

    if (p_registered == NULL)
        return;

    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        return;
    ready              = 1u;
    discovery_retry_ms = (uint32_t)GetTickCount64() - SIM_RETRY_MS;
    discovery_poll((uint32_t)GetTickCount64());
}
static void sim_discovery_task(void)
{
    discovery_poll((uint32_t)GetTickCount64());
}
REG_INIT(11, sim_discovery_init)
REG_TASK(1, sim_discovery_task)
