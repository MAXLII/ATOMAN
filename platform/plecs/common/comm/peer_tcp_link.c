// SPDX-License-Identifier: MIT
/**
 * @file    peer_tcp_link.c
 * @brief   Windows TCP transport between PLECS communication nodes.
 * @details
 *          This file is part of the base PLECS common platform.
 *
 *          Module responsibilities:
 *          - Connect node 0x02 to the loopback server owned by node 0x03
 *          - Feed received TCP bytes into an address-aware FRAME protocol context
 *          - Queue outbound routed frames without blocking PLECS simulation callbacks
 *          - Re-establish the internal connection after either node restarts
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - One worker owns all socket receive and transmit operations
 *          - Hardware access is not used by this simulation module
 *
 * @author  Max.Li
 * @date    2026-08-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "peer_tcp_link.h"

#include "comm.h"
#include "comm_addr.h"
#include "plecs.h"
#include "plecs_dispatch_lock.h"

#include <stdint.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#ifndef PLECS_PEER_ROLE
#error "PLECS_PEER_ROLE must select PEER_TCP_ROLE_CLIENT or PEER_TCP_ROLE_SERVER"
#endif

#if (PLECS_PEER_ROLE != PEER_TCP_ROLE_CLIENT) && (PLECS_PEER_ROLE != PEER_TCP_ROLE_SERVER)
#error "PLECS_PEER_ROLE has an unsupported value"
#endif

#define PEER_TCP_RX_BUFFER_SIZE COMM_MAX_PAYLOAD_SIZE
#define PEER_TCP_TX_FRAME_SIZE COMM_MAX_FRAME_SIZE
#define PEER_TCP_TX_FRAME_COUNT (64u)
#define PEER_TCP_TX_FLUSH_BUDGET (32768u)
#define PEER_TCP_SOCKET_BUFFER_SIZE (262144)
#define PEER_TCP_SELECT_TIMEOUT_US (10000L)
#define PEER_TCP_RETRY_WAIT_MS (100u)
#define PEER_TCP_STOP_TIMEOUT_MS (3000u)

typedef struct
{
    uint16_t length; /**< Total valid bytes in this queued protocol frame. */
    uint16_t offset; /**< Bytes already accepted by the nonblocking socket. */
    uint8_t data[PEER_TCP_TX_FRAME_SIZE]; /**< Complete encoded FRAME protocol data. */
} peer_tcp_tx_frame_t;

DECLARE_COMM_CTX(peer_comm_ctx, PEER_TCP_RX_BUFFER_SIZE, HOST_ADDR, PEER_TCP_LINK_ID);

static HANDLE worker = NULL; /**< Worker thread that owns the internal TCP socket. */
static volatile LONG stop_requested = 0; /**< Nonzero requests worker termination. */
static SRWLOCK socket_lock = SRWLOCK_INIT; /**< Protects the connected socket handle. */
static CRITICAL_SECTION tx_queue_lock; /**< Protects the fixed outbound frame queue. */
static SOCKET connected_socket = INVALID_SOCKET; /**< Current full-duplex peer connection. */
static uint8_t winsock_ready = 0u; /**< Whether this module owns a WSAStartup reference. */
static uint8_t queue_lock_ready = 0u; /**< Whether tx_queue_lock has been initialized. */
static peer_tcp_tx_frame_t tx_frames[PEER_TCP_TX_FRAME_COUNT]; /**< Fixed outbound frame storage. */
static uint32_t tx_head = 0u; /**< Queue entry currently being transmitted. */
static uint32_t tx_tail = 0u; /**< Queue entry reserved for the next frame. */
static uint32_t tx_count = 0u; /**< Number of queued or partially transmitted frames. */
static volatile LONG tx_drop_count = 0; /**< Frames dropped because the peer or queue was unavailable. */

/**
 * @brief Read the current connected socket under the shared socket lock.
 * @return Connected socket or INVALID_SOCKET when the link is offline.
 */
static SOCKET socket_get(void)
{
    SOCKET peer_socket = INVALID_SOCKET; /* Stable socket snapshot returned to the caller. */

    AcquireSRWLockShared(&socket_lock);
    peer_socket = connected_socket;
    ReleaseSRWLockShared(&socket_lock);
    return peer_socket;
}

/**
 * @brief Publish a new connected socket under the shared socket lock.
 * @param[in] peer_socket Socket to publish or INVALID_SOCKET when disconnected.
 */
static void socket_set(SOCKET peer_socket)
{
    AcquireSRWLockExclusive(&socket_lock);
    connected_socket = peer_socket;
    ReleaseSRWLockExclusive(&socket_lock);
}

/**
 * @brief Reset every outbound queue index after a connection transition.
 */
static void tx_queue_reset(void)
{
    EnterCriticalSection(&tx_queue_lock);
    tx_head = 0u;
    tx_tail = 0u;
    tx_count = 0u;
    LeaveCriticalSection(&tx_queue_lock);
}

/**
 * @brief Report whether at least one outbound frame is waiting.
 * @return 1 when data is queued, otherwise 0.
 */
static uint8_t tx_queue_has_data(void)
{
    uint8_t has_data = 0u; /* Normalized queue state returned to select setup. */

    EnterCriticalSection(&tx_queue_lock);
    has_data = (tx_count > 0u) ? 1u : 0u;
    LeaveCriticalSection(&tx_queue_lock);
    return has_data;
}

/**
 * @brief Copy one encoded protocol frame into the fixed outbound queue.
 * @param[in] p_data Encoded FRAME protocol bytes.
 * @param[in] length Number of bytes in p_data.
 */
static void peer_send(char *p_data, int length)
{
    peer_tcp_tx_frame_t *p_frame = NULL; /* Queue slot receiving the encoded frame. */

    if ((p_data == NULL) || /* The protocol encoder did not supply readable data. */
        (length <= 0) || /* Empty and negative writes are invalid. */
        (length > (int)PEER_TCP_TX_FRAME_SIZE) || /* One frame must fit in a queue slot. */
        (socket_get() == INVALID_SOCKET)) /* No peer can consume the routed frame. */
    {
        (void)InterlockedIncrement(&tx_drop_count);
        return;
    }

    EnterCriticalSection(&tx_queue_lock);
    if (tx_count < PEER_TCP_TX_FRAME_COUNT)
    {
        p_frame = &tx_frames[tx_tail];
        p_frame->length = (uint16_t)length;
        p_frame->offset = 0u;
        (void)memcpy(p_frame->data, p_data, (size_t)length);
        tx_tail = (tx_tail + 1u) % PEER_TCP_TX_FRAME_COUNT;
        ++tx_count;
    }
    else
    {
        (void)InterlockedIncrement(&tx_drop_count);
    }
    LeaveCriticalSection(&tx_queue_lock);
}

static section_link_tx_func_t peer_io = {
    .my_printf = NULL,
    .tx_by_dma = peer_send,
}; /**< Protocol output adapter registered as Section link 2. */

static section_link_t peer_tcp_section_link = {
    .rx_get_byte = NULL,
    .my_printf = &peer_io,
    .handler_arr = NULL,
    .handler_num = 0u,
    .link_id = PEER_TCP_LINK_ID,
}; /**< Transmit-only Section descriptor used by the route lookup. */

REG_SECTION_FUNC(SECTION_LINK, peer_tcp_section_link)

/**
 * @brief Restore the peer parser to its initial frame-search state.
 */
static void parser_reset(void)
{
    peer_comm_ctx.index = 0u;
    peer_comm_ctx.status = SECTION_PACKFORM_STA_SOP;
    peer_comm_ctx.crc = 0u;
    peer_comm_ctx.func = NULL;
    peer_comm_ctx.len = 0u;
    peer_comm_ctx.last_rx_tick = 0u;
    peer_comm_ctx.src_flag = 0u;
    peer_comm_ctx.dst_flag = 0u;
    peer_comm_ctx.cmd_flag = 0u;
    peer_comm_ctx.len_flag = 0u;
    peer_comm_ctx.eop_flag = 0u;
    peer_comm_ctx.is_route = 0u;
}

/**
 * @brief Configure socket buffers, latency, and nonblocking mode.
 * @param[in] peer_socket Newly connected peer socket.
 * @return 1 when every required option succeeds, otherwise 0.
 */
static uint8_t socket_configure(SOCKET peer_socket)
{
    const int socket_buffer_size = PEER_TCP_SOCKET_BUFFER_SIZE; /* Requested Windows socket buffer size. */
    const int tcp_no_delay = 1; /* Disable Nagle delay for command and ACK frames. */
    u_long nonblocking = 1u; /* Winsock flag enabling nonblocking receive and transmit. */

    if (setsockopt(peer_socket,
                   IPPROTO_TCP,
                   TCP_NODELAY,
                   (const char *)&tcp_no_delay,
                   (int)sizeof(tcp_no_delay)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (setsockopt(peer_socket,
                   SOL_SOCKET,
                   SO_SNDBUF,
                   (const char *)&socket_buffer_size,
                   (int)sizeof(socket_buffer_size)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (setsockopt(peer_socket,
                   SOL_SOCKET,
                   SO_RCVBUF,
                   (const char *)&socket_buffer_size,
                   (int)sizeof(socket_buffer_size)) == SOCKET_ERROR)
    {
        return 0u;
    }
    if (ioctlsocket(peer_socket, (long)FIONBIO, &nonblocking) == SOCKET_ERROR)
    {
        return 0u;
    }
    return 1u;
}

/**
 * @brief Flush queued frames to the nonblocking peer socket.
 * @param[in] peer_socket Active peer connection.
 * @return 1 while the connection remains usable, otherwise 0.
 */
static uint8_t tx_flush(SOCKET peer_socket)
{
    uint32_t bytes_sent = 0u; /* Bytes accepted during this bounded worker pass. */

    while (bytes_sent < PEER_TCP_TX_FLUSH_BUDGET)
    {
        peer_tcp_tx_frame_t *p_frame = NULL; /* Queue head currently being sent. */
        int sent = 0; /* Result returned by Winsock send. */
        int send_error = 0; /* Stable Winsock error captured before releasing the queue lock. */

        EnterCriticalSection(&tx_queue_lock);
        if (tx_count == 0u)
        {
            LeaveCriticalSection(&tx_queue_lock);
            return 1u;
        }

        p_frame = &tx_frames[tx_head];
        sent = send(peer_socket,
                    (const char *)&p_frame->data[p_frame->offset],
                    (int)(p_frame->length - p_frame->offset),
                    0);
        if (sent > 0)
        {
            p_frame->offset = (uint16_t)(p_frame->offset + (uint16_t)sent);
            bytes_sent += (uint32_t)sent;
            if (p_frame->offset == p_frame->length)
            {
                tx_head = (tx_head + 1u) % PEER_TCP_TX_FRAME_COUNT;
                --tx_count;
            }
            LeaveCriticalSection(&tx_queue_lock);
            continue;
        }

        send_error = WSAGetLastError();
        LeaveCriticalSection(&tx_queue_lock);
        if ((sent == SOCKET_ERROR) && /* Winsock reported an error result. */
            (send_error == WSAEWOULDBLOCK)) /* The socket remains valid but cannot accept more data yet. */
        {
            return 1u;
        }
        return 0u;
    }
    return 1u;
}

#if (PLECS_PEER_ROLE == PEER_TCP_ROLE_SERVER)
/**
 * @brief Open the node 0x03 loopback listening socket.
 * @return Listening socket or INVALID_SOCKET when setup fails.
 */
static SOCKET listener_open(void)
{
    struct sockaddr_in address = {0}; /* Loopback endpoint reserved for the internal node link. */
    SOCKET listen_socket = INVALID_SOCKET; /* Socket accepting the node 0x02 connection. */
    u_long nonblocking = 1u; /* Nonblocking accept allows bounded worker shutdown. */

    listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((u_short)PEER_TCP_LINK_PORT);
    if (bind(listen_socket, (const struct sockaddr *)&address, (int)sizeof(address)) == SOCKET_ERROR)
    {
        PLECS_LOG("Peer TCP: bind 127.0.0.1:%u failed: %d\n", PEER_TCP_LINK_PORT, WSAGetLastError());
        (void)closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    if (listen(listen_socket, 1) == SOCKET_ERROR)
    {
        (void)closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    if (ioctlsocket(listen_socket, (long)FIONBIO, &nonblocking) == SOCKET_ERROR)
    {
        (void)closesocket(listen_socket);
        return INVALID_SOCKET;
    }
    PLECS_LOG("Peer TCP: node 0x03 listening on 127.0.0.1:%u\n", PEER_TCP_LINK_PORT);
    return listen_socket;
}

/**
 * @brief Accept the node 0x02 connection from the nonblocking listener.
 * @param[in] listen_socket Active node 0x03 listening socket.
 * @return Connected socket or INVALID_SOCKET while no client is pending.
 */
static SOCKET connection_open(SOCKET listen_socket)
{
    return accept(listen_socket, NULL, NULL);
}
#else
/**
 * @brief Connect node 0x02 to the node 0x03 loopback listener.
 * @param[in] listen_socket Unused placeholder matching the server helper signature.
 * @return Connected socket or INVALID_SOCKET when the current attempt fails.
 */
static SOCKET connection_open(SOCKET listen_socket)
{
    struct sockaddr_in address = {0}; /* Fixed node 0x03 loopback endpoint. */
    SOCKET peer_socket = INVALID_SOCKET; /* Socket used for this connection attempt. */

    (void)listen_socket;
    peer_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (peer_socket == INVALID_SOCKET)
    {
        return INVALID_SOCKET;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons((u_short)PEER_TCP_LINK_PORT);
    if (connect(peer_socket, (const struct sockaddr *)&address, (int)sizeof(address)) == SOCKET_ERROR)
    {
        (void)closesocket(peer_socket);
        return INVALID_SOCKET;
    }
    return peer_socket;
}
#endif /* PLECS_PEER_ROLE */

/**
 * @brief Run receive and transmit processing until the active connection closes.
 * @param[in] peer_socket Active full-duplex internal TCP connection.
 */
static void connection_run(SOCKET peer_socket)
{
    while (InterlockedCompareExchange(&stop_requested, 0, 0) == 0)
    {
        fd_set read_set = {0}; /* Socket readiness set for incoming protocol bytes. */
        fd_set write_set = {0}; /* Socket readiness set for queued routed frames. */
        fd_set error_set = {0}; /* Socket readiness set for asynchronous connection errors. */
        struct timeval timeout = {
            .tv_sec = 0,
            .tv_usec = PEER_TCP_SELECT_TIMEOUT_US,
        }; /* Bounded wait keeps stop latency predictable. */
        const uint8_t has_tx_data = tx_queue_has_data(); /* Whether select must monitor writability. */
        int select_result = 0; /* Readiness count or Winsock error. */

        FD_ZERO(&read_set);
        FD_ZERO(&write_set);
        FD_ZERO(&error_set);
        FD_SET(peer_socket, &read_set);
        FD_SET(peer_socket, &error_set);
        if (has_tx_data == 1u)
        {
            FD_SET(peer_socket, &write_set);
        }

        select_result = select(0,
                               &read_set,
                               (has_tx_data == 1u) ? &write_set : NULL,
                               &error_set,
                               &timeout);
        if ((select_result == SOCKET_ERROR) || /* Winsock could not poll the active connection. */
            FD_ISSET(peer_socket, &error_set)) /* The connection reported an asynchronous error. */
        {
            break;
        }

        if (FD_ISSET(peer_socket, &read_set))
        {
            uint8_t rx_buffer[PEER_TCP_RX_BUFFER_SIZE] = {0}; /* TCP bytes delivered in this receive call. */
            const int received = recv(peer_socket, (char *)rx_buffer, (int)sizeof(rx_buffer), 0); /* Byte count. */
            const uint32_t receive_time_ms = (uint32_t)GetTickCount64(); /* Parser timeout timestamp. */
            int index = 0; /* Byte currently dispatched to the protocol parser. */

            if (received <= 0)
            {
                if ((received == SOCKET_ERROR) && /* Winsock returned an error result. */
                    (WSAGetLastError() == WSAEWOULDBLOCK)) /* No bytes are ready but the socket remains valid. */
                {
                    continue;
                }
                break;
            }

            plecs_dispatch_lock_enter();
            for (index = 0; index < received; ++index)
            {
                comm_run_with_time(rx_buffer[index], &peer_io, &peer_comm_ctx, receive_time_ms);
            }
            plecs_dispatch_lock_exit();
        }

        if ((has_tx_data == 1u) && /* At least one complete routed frame is queued. */
            FD_ISSET(peer_socket, &write_set) && /* Winsock reports transmit capacity. */
            (tx_flush(peer_socket) == 0u)) /* A fatal send error closed the usable connection. */
        {
            break;
        }
    }
}

/**
 * @brief Own the peer listener, reconnect loop, and connected socket lifecycle.
 * @param[in] p_context Unused Windows thread context.
 * @return Windows thread exit code 0.
 */
static DWORD WINAPI peer_worker(void *p_context)
{
    SOCKET listen_socket = INVALID_SOCKET; /* Server listener; unused by the client build. */

    (void)p_context;
#if (PLECS_PEER_ROLE == PEER_TCP_ROLE_SERVER)
    listen_socket = listener_open();
    if (listen_socket == INVALID_SOCKET)
    {
        return 0u;
    }
#endif

    while (InterlockedCompareExchange(&stop_requested, 0, 0) == 0)
    {
        SOCKET peer_socket = connection_open(listen_socket); /* Connection established during this loop pass. */

        if (peer_socket == INVALID_SOCKET)
        {
            Sleep(PEER_TCP_RETRY_WAIT_MS);
            continue;
        }
        if (socket_configure(peer_socket) == 0u)
        {
            (void)closesocket(peer_socket);
            Sleep(PEER_TCP_RETRY_WAIT_MS);
            continue;
        }

        tx_queue_reset();
        parser_reset();
        socket_set(peer_socket);
        PLECS_LOG("Peer TCP: connected on link %u\n", PEER_TCP_LINK_ID);
        connection_run(peer_socket);
        socket_set(INVALID_SOCKET);
        tx_queue_reset();
        (void)shutdown(peer_socket, SD_BOTH);
        (void)closesocket(peer_socket);
        PLECS_LOG("Peer TCP: disconnected, tx_drop=%ld\n", InterlockedCompareExchange(&tx_drop_count, 0, 0));
    }

    if (listen_socket != INVALID_SOCKET)
    {
        (void)closesocket(listen_socket);
    }
    return 0u;
}

void peer_tcp_link_start(void)
{
    WSADATA winsock_data = {0}; /* Winsock version state acquired by this transport. */

    if (worker != NULL)
    {
        return;
    }
    if (queue_lock_ready == 0u)
    {
        InitializeCriticalSection(&tx_queue_lock);
        queue_lock_ready = 1u;
    }
    if (WSAStartup(MAKEWORD(2, 2), &winsock_data) != 0)
    {
        PLECS_LOG("Peer TCP: WSAStartup failed\n");
        return;
    }

    winsock_ready = 1u;
    tx_queue_reset();
    parser_reset();
    socket_set(INVALID_SOCKET);
    (void)InterlockedExchange(&tx_drop_count, 0);
    (void)InterlockedExchange(&stop_requested, 0);
    worker = CreateThread(NULL, 0u, peer_worker, NULL, 0u, NULL);
    if (worker == NULL)
    {
        PLECS_LOG("Peer TCP: worker creation failed: %lu\n", GetLastError());
        (void)WSACleanup();
        winsock_ready = 0u;
    }
}

void peer_tcp_link_stop(void)
{
    const SOCKET peer_socket = socket_get(); /* Connection interrupted to wake the worker promptly. */

    if (worker != NULL)
    {
        (void)InterlockedExchange(&stop_requested, 1);
        if (peer_socket != INVALID_SOCKET)
        {
            (void)shutdown(peer_socket, SD_BOTH);
        }
        (void)WaitForSingleObject(worker, PEER_TCP_STOP_TIMEOUT_MS);
        (void)CloseHandle(worker);
        worker = NULL;
    }
    socket_set(INVALID_SOCKET);
    if (winsock_ready == 1u)
    {
        (void)WSACleanup();
        winsock_ready = 0u;
    }
    if (queue_lock_ready == 1u)
    {
        DeleteCriticalSection(&tx_queue_lock);
        queue_lock_ready = 0u;
    }
}

uint8_t peer_tcp_link_is_connected(void)
{
    return (socket_get() != INVALID_SOCKET) ? 1u : 0u;
}
