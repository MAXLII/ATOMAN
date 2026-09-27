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

#if defined(SIM_DISCOVERY_SHARED_PROCESS)
#define SIM_DISCOVERY_SHARED_SLOTS 8u /* Maximum advertisements across participating DLLs. */
#define SIM_DISCOVERY_NAME_SIZE 64u /* Copied names remain valid when a peer DLL stops. */
typedef struct sim_discovery_shared_entry
{
    uintptr_t owner; /* DLL-local static address identifies the publisher within this process. */
    uint32_t node_addr; /* FRAME node identity. */
    uint16_t tcp_port; /* Advertised TCP endpoint. */
    char name[SIM_DISCOVERY_NAME_SIZE]; /* Owned device name, NUL terminated. */
} sim_discovery_shared_entry_t;

static HANDLE shared_mapping = NULL; /* Process/UDP-port-scoped registry mapping handle. */
static HANDLE shared_mutex = NULL; /* Serializes publication, snapshot and removal. */
static sim_discovery_shared_entry_t *p_shared = NULL; /* Mapped registry; no peer pointers are dereferenced. */
static uint8_t shared_published = 0u; /* Nonzero after all local registrations are copied. */

/** @return 1 when the shared registry lock was obtained without blocking. */
static uint8_t shared_lock(void)
{
    if (shared_mutex == NULL)
        return 0u;
    const DWORD result = WaitForSingleObject(shared_mutex, 0u); /* Never stall a simulation callback. */
    return ((result == WAIT_OBJECT_0) || (result == WAIT_ABANDONED)) ? 1u : 0u;
}

/** @brief Close local registry resources; all records must already be removed. */
static void shared_close(void)
{
    if (p_shared != NULL)
        (void)UnmapViewOfFile(p_shared);
    if (shared_mapping != NULL)
        (void)CloseHandle(shared_mapping);
    if (shared_mutex != NULL)
        (void)CloseHandle(shared_mutex);
    p_shared = NULL;
    shared_mapping = NULL;
    shared_mutex = NULL;
    shared_published = 0u;
}

/** @brief Attach a registry shared by DLLs in this host process on the same UDP port. */
static void shared_open(void)
{
    char mapping_name[96] = {0}; /* Kernel object name scoped to the host PID and discovery port. */
    char mutex_name[96] = {0}; /* Lock paired with this registry. */
    const sim_discovery_reg_t *p_config = p_registered->p_obj; /* Local discovery port configuration. */
    (void)snprintf(mapping_name, sizeof(mapping_name), "Local\\PLECS_FRAME_%lu_%u_nodes",
                   (unsigned long)GetCurrentProcessId(), (unsigned int)p_config->udp_port);
    (void)snprintf(mutex_name, sizeof(mutex_name), "Local\\PLECS_FRAME_%lu_%u_lock",
                   (unsigned long)GetCurrentProcessId(), (unsigned int)p_config->udp_port);
    shared_mutex = CreateMutexA(NULL, FALSE, mutex_name);
    if (shared_mutex == NULL)
        return;
    shared_mapping = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0u,
                                        (DWORD)(sizeof(*p_shared) * SIM_DISCOVERY_SHARED_SLOTS), mapping_name);
    if (shared_mapping != NULL)
        p_shared = MapViewOfFile(shared_mapping, FILE_MAP_ALL_ACCESS, 0u, 0u, 0u);
    if (p_shared == NULL)
        shared_close();
}

/** @brief Publish a copied identity; retry on later task calls if the lock is busy. */
static void shared_publish(void)
{
    if ((p_shared == NULL) || (shared_published != 0u) || (shared_lock() == 0u))
        return;
    const uintptr_t owner = (uintptr_t)(const void *)&discovery; /* Identity only; never dereferenced by peers. */
    for (uint32_t index = 0u; index < SIM_DISCOVERY_SHARED_SLOTS; ++index)
    {
        if (p_shared[index].owner == owner)
            (void)memset(&p_shared[index], 0, sizeof(p_shared[index]));
    }
    shared_published = 1u;
    for (const section_item_t *p_item = p_registered; p_item != NULL; p_item = p_item->p_next)
    {
        const sim_discovery_reg_t *p_config = p_item->p_obj; /* Registration copied under the shared lock. */
        uint32_t index = 0u; /* First unoccupied registry entry. */
        while ((index < SIM_DISCOVERY_SHARED_SLOTS) && (p_shared[index].owner != 0u))
            ++index;
        if (index == SIM_DISCOVERY_SHARED_SLOTS)
        {
            shared_published = 0u;
            break;
        }
        p_shared[index].owner = owner;
        p_shared[index].node_addr = p_config->node_addr;
        p_shared[index].tcp_port = p_config->tcp_port;
        (void)snprintf(p_shared[index].name, sizeof(p_shared[index].name), "%s", p_config->p_name);
    }
    (void)ReleaseMutex(shared_mutex);
}
#endif /* SIM_DISCOVERY_SHARED_PROCESS */

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

#if defined(SIM_DISCOVERY_SHARED_PROCESS)
    shared_publish(); /* Every DLL publishes even when another DLL owns UDP 5000. */
#endif

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
#if defined(SIM_DISCOVERY_SHARED_PROCESS)
            if (p_shared != NULL)
            {
                sim_discovery_shared_entry_t entries[SIM_DISCOVERY_SHARED_SLOTS] = {0}; /* Stable response snapshot. */
                if (shared_lock() == 0u)
                    continue;
                (void)memcpy(entries, p_shared, sizeof(entries));
                (void)ReleaseMutex(shared_mutex);
                for (uint32_t index = 0u; index < SIM_DISCOVERY_SHARED_SLOTS; ++index)
                {
                    if (entries[index].owner != 0u)
                    {
                        const sim_discovery_reg_t config = { /* Adapt the owned snapshot to the reply encoder. */
                            .p_name = entries[index].name,
                            .node_addr = entries[index].node_addr,
                            .tcp_port = entries[index].tcp_port,
                            .udp_port = 0u,
                        };
                        discovery_reply(&remote, &config);
                    }
                }
                continue;
            }
#endif
            for (const section_item_t *p_item = p_registered; p_item != NULL; p_item = p_item->p_next)
            {
                discovery_reply(&remote, p_item->p_obj);
            }
        }
    }
}

void sim_discovery_stop(void)
{
#if defined(SIM_DISCOVERY_SHARED_PROCESS)
    if (p_shared != NULL)
    {
        /* Only lifecycle teardown may wait; ensure no stale identity survives DLL shutdown. */
        const DWORD result = WaitForSingleObject(shared_mutex, INFINITE); /* Wait for any in-flight snapshot. */
        if ((result == WAIT_OBJECT_0) || (result == WAIT_ABANDONED))
        {
            const uintptr_t owner = (uintptr_t)(const void *)&discovery; /* Publisher being removed. */
            for (uint32_t index = 0u; index < SIM_DISCOVERY_SHARED_SLOTS; ++index)
            {
                if (p_shared[index].owner == owner)
                    (void)memset(&p_shared[index], 0, sizeof(p_shared[index]));
            }
            (void)ReleaseMutex(shared_mutex);
        }
    }
    shared_close();
#endif
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
#if defined(SIM_DISCOVERY_SHARED_PROCESS)
    shared_open();
#endif
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
