// SPDX-License-Identifier: MIT
/**
 * @file    frame_tcp_server.c
 * @brief   Windows TCP transport for FRAME-to-PLECS communication.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Listen on TCP port 5000 for a FRAME Ethernet connection
 *          - Feed received bytes into the shared 0xE8 protocol parser
 *          - Queue protocol output so PLECS callbacks never block on socket transmission
 *          - Send queued frames and expose dispatch serialization to PLECS projects
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One worker owns all nonblocking socket receive and transmit operations
 *          - Hardware access is not used by this simulation project
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "frame_tcp_server.h"

#include "comm.h"
#include "comm_addr.h"
#include "plecs.h"
#include "shell_service.h"

#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#define FRAME_TCP_RX_BUFFER_SIZE (512u)
#define FRAME_TCP_TX_FRAME_SIZE (512u)
#define FRAME_TCP_TX_PRIORITY_FRAME_COUNT (32u)
#define FRAME_TCP_TX_STREAM_FRAME_COUNT (128u)
#define FRAME_TCP_TX_FLUSH_BUDGET (32768u)
#define FRAME_TCP_SOCKET_BUFFER_SIZE (262144)
#define FRAME_TCP_SELECT_TIMEOUT_US (10000L)
#define FRAME_TCP_WAIT_MS (100u)
#define FRAME_TCP_STOP_TIMEOUT_MS (3000u)
#define FRAME_TCP_LINK_ID (1u)
#define FRAME_TCP_CMD_SET_OFFSET (6u)
#define FRAME_TCP_CMD_WORD_OFFSET (7u)

typedef struct
{
    uint16_t length;                    /**< Total valid bytes stored in this queued protocol frame. */
    uint16_t offset;                    /**< Bytes already accepted by the nonblocking socket. */
    uint8_t data[FRAME_TCP_TX_FRAME_SIZE]; /**< Complete protocol frame copied from the shared encoder. */
} frame_tcp_tx_frame_t;

typedef struct
{
    frame_tcp_tx_frame_t *p_frames; /**< Storage owned by this queue. */
    uint32_t capacity;              /**< Number of frame slots in the storage array. */
    uint32_t head;                  /**< Entry currently being transmitted by the worker. */
    uint32_t tail;                  /**< Entry reserved for the next complete protocol frame. */
    uint32_t count;                 /**< Complete or partially transmitted frames currently queued. */
} frame_tcp_tx_queue_t;

DECLARE_COMM_CTX(s_frame_comm_ctx, FRAME_TCP_RX_BUFFER_SIZE, HOST_ADDR, FRAME_TCP_LINK_ID);

static HANDLE s_worker = NULL;
static volatile LONG s_stop_requested = 0;
static SRWLOCK s_socket_lock = SRWLOCK_INIT;
static CRITICAL_SECTION s_debug_lock;
static CRITICAL_SECTION s_tx_queue_lock;
static SOCKET s_client_socket = INVALID_SOCKET;
static uint8_t s_winsock_ready = 0u;
static uint8_t s_critical_sections_ready = 0u;
static frame_tcp_tx_frame_t tx_priority_frames[FRAME_TCP_TX_PRIORITY_FRAME_COUNT]; /**< Control and query frames. */
static frame_tcp_tx_frame_t tx_stream_frames[FRAME_TCP_TX_STREAM_FRAME_COUNT]; /**< Droppable real-time wave frames. */
static frame_tcp_tx_queue_t tx_priority_queue = {
    .p_frames = tx_priority_frames,
    .capacity = FRAME_TCP_TX_PRIORITY_FRAME_COUNT,
    .head = 0u,
    .tail = 0u,
    .count = 0u,
};
static frame_tcp_tx_queue_t tx_stream_queue = {
    .p_frames = tx_stream_frames,
    .capacity = FRAME_TCP_TX_STREAM_FRAME_COUNT,
    .head = 0u,
    .tail = 0u,
    .count = 0u,
};
static volatile LONG tx_drop_count = 0; /**< Frames dropped instead of blocking the simulation thread. */
static volatile LONG tx_would_block_count = 0; /**< Nonblocking sends deferred because Winsock was full. */

static uint32_t frame_tcp_time_get_ms(void)
{
    return (uint32_t)GetTickCount64();
}

static SOCKET frame_tcp_client_get(void)
{
    SOCKET client_socket = INVALID_SOCKET;

    AcquireSRWLockShared(&s_socket_lock);
    client_socket = s_client_socket;
    ReleaseSRWLockShared(&s_socket_lock);
    return client_socket;
}

static void frame_tcp_client_set(SOCKET client_socket)
{
    AcquireSRWLockExclusive(&s_socket_lock);
    s_client_socket = client_socket;
    ReleaseSRWLockExclusive(&s_socket_lock);
}

static void frame_tcp_tx_queue_reset(void)
{
    EnterCriticalSection(&s_tx_queue_lock);
    tx_priority_queue.head = 0u;
    tx_priority_queue.tail = 0u;
    tx_priority_queue.count = 0u;
    tx_stream_queue.head = 0u;
    tx_stream_queue.tail = 0u;
    tx_stream_queue.count = 0u;
    LeaveCriticalSection(&s_tx_queue_lock);
}

static uint8_t frame_tcp_tx_queue_has_data(void)
{
    uint8_t has_data = 0u;

    EnterCriticalSection(&s_tx_queue_lock);
    has_data = ((tx_priority_queue.count > 0u) ||
                (tx_stream_queue.count > 0u))
                   ? 1u
                   : 0u;
    LeaveCriticalSection(&s_tx_queue_lock);
    return has_data;
}

static frame_tcp_tx_queue_t *frame_tcp_tx_queue_select(const char *p_data, int length)
{
    if ((length > (int)FRAME_TCP_CMD_WORD_OFFSET) &&
        ((uint8_t)p_data[FRAME_TCP_CMD_SET_OFFSET] == CMD_SET_SHELL_WAVE_PARAM) &&
        ((uint8_t)p_data[FRAME_TCP_CMD_WORD_OFFSET] == CMD_WORD_SHELL_WAVE_PARAM))
    {
        return &tx_stream_queue;
    }
    return &tx_priority_queue;
}

static void frame_tcp_send(char *p_data, int length)
{
    frame_tcp_tx_frame_t *p_frame = NULL;
    frame_tcp_tx_queue_t *p_queue = NULL;
    const SOCKET client_socket = frame_tcp_client_get();

    if ((p_data == NULL) ||
        (length <= 0) ||
        (length > (int)FRAME_TCP_TX_FRAME_SIZE) ||
        (client_socket == INVALID_SOCKET))
    {
        return;
    }

    EnterCriticalSection(&s_tx_queue_lock);
    p_queue = frame_tcp_tx_queue_select(p_data, length);
    if (p_queue->count < p_queue->capacity)
    {
        p_frame = &p_queue->p_frames[p_queue->tail];
        p_frame->length = (uint16_t)length;
        p_frame->offset = 0u;
        (void)memcpy(p_frame->data, p_data, (size_t)length);
        p_queue->tail = (p_queue->tail + 1u) % p_queue->capacity;
        ++p_queue->count;
    }
    else
    {
        (void)InterlockedIncrement(&tx_drop_count);
    }
    LeaveCriticalSection(&s_tx_queue_lock);
}

static uint8_t frame_tcp_tx_flush(SOCKET client_socket)
{
    uint32_t bytes_sent = 0u;

    while (bytes_sent < FRAME_TCP_TX_FLUSH_BUDGET)
    {
        frame_tcp_tx_frame_t *p_frame = NULL;
        frame_tcp_tx_queue_t *p_queue = NULL;
        int sent = 0;
        int send_error = 0;

        EnterCriticalSection(&s_tx_queue_lock);
        if (tx_priority_queue.count > 0u)
        {
            p_queue = &tx_priority_queue;
        }
        else if (tx_stream_queue.count > 0u)
        {
            p_queue = &tx_stream_queue;
        }
        else
        {
            LeaveCriticalSection(&s_tx_queue_lock);
            return 1u;
        }

        p_frame = &p_queue->p_frames[p_queue->head];
        sent = send(client_socket,
                    (const char *)&p_frame->data[p_frame->offset],
                    (int)(p_frame->length - p_frame->offset),
                    0);
        if (sent > 0)
        {
            p_frame->offset = (uint16_t)(p_frame->offset + (uint16_t)sent);
            bytes_sent += (uint32_t)sent;
            if (p_frame->offset == p_frame->length)
            {
                p_queue->head = (p_queue->head + 1u) % p_queue->capacity;
                --p_queue->count;
            }
            LeaveCriticalSection(&s_tx_queue_lock);
            continue;
        }

        send_error = WSAGetLastError();
        LeaveCriticalSection(&s_tx_queue_lock);
        if ((sent == SOCKET_ERROR) && (send_error == WSAEWOULDBLOCK))
        {
            (void)InterlockedIncrement(&tx_would_block_count);
            return 1u;
        }
        return 0u;
    }

    return 1u;
}

static section_link_tx_func_t s_frame_io = {
    .my_printf = NULL,
    .tx_by_dma = frame_tcp_send,
};

static void frame_tcp_parser_reset(void)
{
    s_frame_comm_ctx.index = 0u;
    s_frame_comm_ctx.status = SECTION_PACKFORM_STA_SOP;
    s_frame_comm_ctx.crc = 0u;
    s_frame_comm_ctx.func = NULL;
    s_frame_comm_ctx.len = 0u;
    s_frame_comm_ctx.last_rx_tick = 0u;
    s_frame_comm_ctx.src_flag = 0u;
    s_frame_comm_ctx.dst_flag = 0u;
    s_frame_comm_ctx.cmd_flag = 0u;
    s_frame_comm_ctx.len_flag = 0u;
    s_frame_comm_ctx.eop_flag = 0u;
    s_frame_comm_ctx.is_route = 0u;
}

static uint8_t frame_tcp_client_configure(SOCKET client_socket)
{
    const BOOL tcp_no_delay = TRUE;
    const int socket_buffer_size = FRAME_TCP_SOCKET_BUFFER_SIZE;
    u_long nonblocking = 1u;

    if (setsockopt(client_socket,
                   IPPROTO_TCP,
                   TCP_NODELAY,
                   (const char *)&tcp_no_delay,
                   (int)sizeof(tcp_no_delay)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (setsockopt(client_socket,
                   SOL_SOCKET,
                   SO_SNDBUF,
                   (const char *)&socket_buffer_size,
                   (int)sizeof(socket_buffer_size)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (setsockopt(client_socket,
                   SOL_SOCKET,
                   SO_RCVBUF,
                   (const char *)&socket_buffer_size,
                   (int)sizeof(socket_buffer_size)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (ioctlsocket(client_socket, (long)FIONBIO, &nonblocking) == SOCKET_ERROR)
    {
        return 0u;
    }
    return 1u;
}

static DWORD WINAPI frame_tcp_worker(void *context)
{
    struct sockaddr_in address = {0};
    SOCKET listen_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    u_long nonblocking = 1u;

    (void)context;
    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        PLECS_LOG("FRAME TCP: socket creation failed: %d\n", WSAGetLastError());
        return 0u;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((u_short)FRAME_TCP_SERVER_PORT);
    if (bind(listen_socket, (const struct sockaddr *)&address, (int)sizeof(address)) == SOCKET_ERROR)
    {
        PLECS_LOG("FRAME TCP: bind port %u failed: %d\n", FRAME_TCP_SERVER_PORT, WSAGetLastError());
        (void)closesocket(listen_socket);
        return 0u;
    }
    if (listen(listen_socket, 1) == SOCKET_ERROR)
    {
        PLECS_LOG("FRAME TCP: listen failed: %d\n", WSAGetLastError());
        (void)closesocket(listen_socket);
        return 0u;
    }
    (void)ioctlsocket(listen_socket, (long)FIONBIO, &nonblocking);
    PLECS_LOG("FRAME TCP: listening on 0.0.0.0:%u\n", FRAME_TCP_SERVER_PORT);

    while (InterlockedCompareExchange(&s_stop_requested, 0, 0) == 0)
    {
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET)
        {
            Sleep(FRAME_TCP_WAIT_MS);
            continue;
        }

        if (frame_tcp_client_configure(client_socket) == 0u)
        {
            PLECS_LOG("FRAME TCP: client configuration failed: %d\n", WSAGetLastError());
            (void)closesocket(client_socket);
            continue;
        }
        frame_tcp_tx_queue_reset();
        frame_tcp_client_set(client_socket);
        frame_tcp_parser_reset();
        PLECS_LOG("FRAME TCP: client connected\n");
        while (InterlockedCompareExchange(&s_stop_requested, 0, 0) == 0)
        {
            fd_set read_set;
            fd_set write_set;
            fd_set error_set;
            struct timeval timeout = {
                .tv_sec = 0,
                .tv_usec = FRAME_TCP_SELECT_TIMEOUT_US,
            };
            const uint8_t has_tx_data = frame_tcp_tx_queue_has_data();
            int select_result = 0;

            FD_ZERO(&read_set);
            FD_ZERO(&write_set);
            FD_ZERO(&error_set);
            FD_SET(client_socket, &read_set);
            FD_SET(client_socket, &error_set);
            if (has_tx_data == 1u)
            {
                FD_SET(client_socket, &write_set);
            }

            select_result = select(0,
                                   &read_set,
                                   (has_tx_data == 1u) ? &write_set : NULL,
                                   &error_set,
                                   &timeout);
            if ((select_result == SOCKET_ERROR) || FD_ISSET(client_socket, &error_set))
            {
                break;
            }

            if (FD_ISSET(client_socket, &read_set))
            {
                uint8_t rx_buffer[FRAME_TCP_RX_BUFFER_SIZE] = {0};
                const int received = recv(client_socket, (char *)rx_buffer, (int)sizeof(rx_buffer), 0);
                const uint32_t receive_time_ms = frame_tcp_time_get_ms();
                int i = 0;

                if (received <= 0)
                {
                    if ((received == SOCKET_ERROR) && (WSAGetLastError() == WSAEWOULDBLOCK))
                    {
                        continue;
                    }
                    break;
                }
                EnterCriticalSection(&s_debug_lock);
                for (i = 0; i < received; ++i)
                {
                    comm_run_with_time(rx_buffer[i], &s_frame_io, &s_frame_comm_ctx, receive_time_ms);
                }
                LeaveCriticalSection(&s_debug_lock);
            }

            if ((has_tx_data == 1u) &&
                FD_ISSET(client_socket, &write_set) &&
                (frame_tcp_tx_flush(client_socket) == 0u))
            {
                break;
            }
        }
        frame_tcp_client_set(INVALID_SOCKET);
        frame_tcp_tx_queue_reset();
        (void)shutdown(client_socket, SD_BOTH);
        (void)closesocket(client_socket);
        PLECS_LOG("FRAME TCP: client disconnected, tx_drop=%ld, tx_would_block=%ld\n",
                  InterlockedCompareExchange(&tx_drop_count, 0, 0),
                  InterlockedCompareExchange(&tx_would_block_count, 0, 0));
    }

    (void)closesocket(listen_socket);
    return 0u;
}

void frame_tcp_server_start(void)
{
    WSADATA winsock_data;

    if (s_worker != NULL)
    {
        return;
    }
    if (s_critical_sections_ready == 0u)
    {
        InitializeCriticalSection(&s_debug_lock);
        InitializeCriticalSection(&s_tx_queue_lock);
        s_critical_sections_ready = 1u;
    }
    if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0)
    {
        PLECS_LOG("FRAME TCP: WSAStartup failed\n");
        return;
    }
    s_winsock_ready = 1u;
    frame_tcp_tx_queue_reset();
    (void)InterlockedExchange(&tx_drop_count, 0);
    (void)InterlockedExchange(&tx_would_block_count, 0);
    (void)InterlockedExchange(&s_stop_requested, 0);
    s_worker = CreateThread(NULL, 0u, frame_tcp_worker, NULL, 0u, NULL);
    if (s_worker == NULL)
    {
        PLECS_LOG("FRAME TCP: worker creation failed: %lu\n", GetLastError());
        (void)WSACleanup();
        s_winsock_ready = 0u;
    }
}

void frame_tcp_server_stop(void)
{
    const SOCKET client_socket = frame_tcp_client_get();

    if (s_worker != NULL)
    {
        (void)InterlockedExchange(&s_stop_requested, 1);
        if (client_socket != INVALID_SOCKET)
        {
            (void)shutdown(client_socket, SD_BOTH);
        }
        (void)WaitForSingleObject(s_worker, FRAME_TCP_STOP_TIMEOUT_MS);
        (void)CloseHandle(s_worker);
        s_worker = NULL;
    }
    if (s_winsock_ready != 0u)
    {
        (void)WSACleanup();
        s_winsock_ready = 0u;
    }
    if (s_critical_sections_ready != 0u)
    {
        DeleteCriticalSection(&s_tx_queue_lock);
        DeleteCriticalSection(&s_debug_lock);
        s_critical_sections_ready = 0u;
    }
}

void frame_tcp_server_dispatch_enter(void)
{
    EnterCriticalSection(&s_debug_lock);
}

void frame_tcp_server_dispatch_exit(void)
{
    LeaveCriticalSection(&s_debug_lock);
}
