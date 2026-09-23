// SPDX-License-Identifier: MIT
/**
 * @file sim_tcp.c
 * @brief Nonblocking TCP transport; no platform, protocol, or discovery identities.
 * @details Static registration; serialized simulation callbacks; bounded storage.
 * @author Max.Li
 * @date 2026-09-17
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the project root.
 */
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#undef REG_LINK /* Windows registry constant is not used by this component. */
#include "sim_tcp.h"

#define SIM_RX_CAPACITY        65536u   /**< Per-link buffered incoming bytes. */
#define SIM_TX_CAPACITY        1048576u /**< Per-link bounded burst capacity. */
#define SIM_IO_BUDGET          32768u   /**< Maximum bytes serviced per link and task pass. */
#define SIM_RETRY_MS           100u     /**< Wall-clock retry spacing. */
#define SIM_CONNECT_TIMEOUT_MS 1000u    /**< Deadline for a nonblocking connect attempt. */

typedef struct sim_tcp
{
    sim_tcp_reg_t *p_reg; /**< Immutable endpoint configuration and mutable status. */
    SOCKET listener;      /**< Listening endpoint when configured as server. */
    SOCKET socket;        /**< Established or connecting endpoint. */
    uint32_t connecting;  /**< Pending nonblocking connection. */
    uint32_t retry_ms;    /**< Last listener/connect attempt time. */
    uint32_t connect_ms;  /**< Pending connect start. */
    uint32_t rx_head;     /**< Next byte for the upper link handler. */
    uint32_t rx_count;    /**< Buffered receive bytes. */
    uint32_t tx_head;     /**< Next byte for Winsock. */
    uint32_t tx_count;    /**< Buffered transmit bytes. */
    uint8_t rx[SIM_RX_CAPACITY]; /**< Receive ring. */
    uint8_t tx[SIM_TX_CAPACITY]; /**< Transmit ring. */
} sim_tcp_t;

#define SIM_TCP_MAX_CHANNELS 8u
static sim_tcp_t links[SIM_TCP_MAX_CHANNELS];
static uint32_t link_count          = 0u;
static section_item_t *p_registered = NULL;
static uint32_t ready               = 0u;

/** @param p_socket Owned socket handle, reset after closure. */
static void close_socket(SOCKET *p_socket)
{
    if (*p_socket != INVALID_SOCKET)
    {
        (void)closesocket(*p_socket);
        *p_socket = INVALID_SOCKET;
    }
}

/** @param index Link whose queued data must not cross a connection boundary. */
static void disconnect_link(uint32_t index)
{
    sim_tcp_t *p_link = &links[index]; /* State owned by the scheduler. */
    close_socket(&p_link->socket);
    p_link->connecting              = 0u;
    p_link->rx_head                 = 0u;
    p_link->rx_count                = 0u;
    p_link->tx_head                 = 0u;
    p_link->tx_count                = 0u;
    p_link->p_reg->status.connected = 0u;
}

/** @param socket Socket to make nonblocking. @return 1 on success. */
static uint32_t configure_socket(SOCKET socket)
{
    u_long nonblocking = 1u;     /* Never block the simulation callback. */
    int no_delay       = 1;      /* Commands should not wait for Nagle aggregation. */
    int buffer_size    = 262144; /* Host receive/transmit buffer request. */
    (void)setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (const char *)&no_delay, sizeof(no_delay));
    (void)setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (const char *)&buffer_size, sizeof(buffer_size));
    (void)setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char *)&buffer_size, sizeof(buffer_size));
    return (ioctlsocket(socket, (long)FIONBIO, &nonblocking) == 0) ? 1u : 0u;
}

/** Create one nonblocking TCP listener from its registered bind address. */
static SOCKET open_listener(sim_tcp_t *p_link)
{
    SOCKET result              = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    int exclusive              = 1;

    if (result == INVALID_SOCKET)
    {
        p_link->p_reg->status.last_error = (uint32_t)WSAGetLastError();
        return result;
    }
    address.sin_family = AF_INET;
    address.sin_port   = htons(p_link->p_reg->port);
    (void)inet_pton(AF_INET, p_link->p_reg->p_ip, &address.sin_addr);
    (void)setsockopt(result, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char *)&exclusive, sizeof(exclusive));

    if (    (configure_socket(result) == 0u)
         || (bind(result, (const struct sockaddr *)&address, sizeof(address)) != 0)
         || (listen(result, 1) != 0))
    {
        p_link->p_reg->status.last_error = (uint32_t)WSAGetLastError();
        close_socket(&result);
    }
    return result;
}

/** @param index Newly established link; publish its parser generation. */
static void connected(uint32_t index)
{
    links[index].connecting               = 0u;
    links[index].p_reg->status.connected  = 1u;
    links[index].p_reg->status.last_error = 0u;
    ++links[index].p_reg->status.session;
}

/** @param index Link to service. @param now Wall-clock milliseconds. */
static void connect_poll(uint32_t index, uint32_t now)
{
    sim_tcp_t *p_link = &links[index]; /* Link-local endpoint and reconnect state. */
    const uint32_t client = (p_link->p_reg->role == SIM_TCP_CLIENT) ? 1u : 0u;
    const uint16_t port = p_link->p_reg->port;

    if (p_link->connecting == 1u)
    {
        fd_set writable        = {0};           /* Winsock reports connect completion as write readiness. */
        fd_set errors          = {0};           /* Winsock reports refused connects here. */
        struct timeval timeout = {0};           /* Poll without delaying simulation. */
        int error              = 0;             /* SO_ERROR outcome for the pending connection. */
        int length             = sizeof(error); /* Winsock output size. */
        FD_ZERO(&writable);
        FD_ZERO(&errors);
        FD_SET(p_link->socket, &writable);
        FD_SET(p_link->socket, &errors);

        if (select(0, NULL, &writable, &errors, &timeout) > 0)
        {
            if (    (getsockopt(p_link->socket, SOL_SOCKET, SO_ERROR, (char *)&error, &length) == 0)
                 && (error == 0)
                 && /* Only successful connection completion may publish online. */
                    (FD_ISSET(p_link->socket, &writable) != 0))
            {
                connected(index);
            }
            else
                disconnect_link(index);
        }
        else if ((uint32_t)(now - p_link->connect_ms) >= SIM_CONNECT_TIMEOUT_MS)
        {
            disconnect_link(index);
        }
        return;
    }

    if (p_link->socket != INVALID_SOCKET)
        return;

    if (client == 0u)
    {
        if (p_link->listener == INVALID_SOCKET)
        {
            if ((uint32_t)(now - p_link->retry_ms) < SIM_RETRY_MS)
                return;
            p_link->retry_ms = now;
            p_link->listener = open_listener(p_link);
        }

        if (p_link->listener != INVALID_SOCKET)
        {
            p_link->socket = accept(p_link->listener, NULL, NULL);

            if (p_link->socket != INVALID_SOCKET)
            {
                if (configure_socket(p_link->socket) == 1u)
                    connected(index);
                else
                    disconnect_link(index);
            }
        }
        return;
    }

    if ((uint32_t)(now - p_link->retry_ms) >= SIM_RETRY_MS)
    {
        struct sockaddr_in address = {0}; /* Internal loopback server. */
        p_link->retry_ms           = now;
        p_link->socket             = socket(AF_INET, SOCK_STREAM, 0);

        if (p_link->socket == INVALID_SOCKET)
            return;

        if (configure_socket(p_link->socket) == 0u)
        {
            disconnect_link(index);
            return;
        }
        address.sin_family = AF_INET;
        address.sin_port   = htons(port);
        (void)inet_pton(AF_INET, p_link->p_reg->p_ip, &address.sin_addr);

        if (connect(p_link->socket, (const struct sockaddr *)&address, sizeof(address)) == 0)
        {
            connected(index);
        }
        else if (WSAGetLastError() == WSAEWOULDBLOCK)
        {
            p_link->connecting = 1u;
            p_link->connect_ms = now;
        }
        else
            disconnect_link(index);
    }
}

/** @param index Connected link to transfer at most one bounded ring segment per direction. */
static void transfer_poll(uint32_t index)
{
    sim_tcp_t *p_link = &links[index]; /* Queues exclusively owned by this callback context. */
    uint32_t tail = (p_link->rx_head + p_link->rx_count) % SIM_RX_CAPACITY;
    uint32_t count = SIM_RX_CAPACITY - p_link->rx_count; /* Preserve backpressure when the RX ring is full. */
    int result     = 0; /* Winsock byte count. */

    if (    (p_link->socket == INVALID_SOCKET)
         || (p_link->connecting == 1u))
        return;

    if (count > SIM_RX_CAPACITY - tail)
        count = SIM_RX_CAPACITY - tail;

    if (count > SIM_IO_BUDGET)
        count = SIM_IO_BUDGET;

    if (count > 0u)
    {
        result = recv(p_link->socket, (char *)&p_link->rx[tail], (int)count, 0);

        if (result > 0)
            p_link->rx_count += (uint32_t)result;
        else if (    (result == 0)
                  || /* FIN terminates the current byte stream. */
                     (WSAGetLastError() != WSAEWOULDBLOCK)) /* Would-block is normal for this polling BSP. */
        {
            disconnect_link(index);
            return;
        }
    }
    count = p_link->tx_count;

    if (count > SIM_TX_CAPACITY - p_link->tx_head)
        count = SIM_TX_CAPACITY - p_link->tx_head;

    if (count > SIM_IO_BUDGET)
        count = SIM_IO_BUDGET;

    if (count > 0u)
    {
        result = send(p_link->socket, (const char *)&p_link->tx[p_link->tx_head], (int)count, 0);

        if (result > 0)
        {
            p_link->tx_head = (p_link->tx_head + (uint32_t)result) % SIM_TX_CAPACITY;
            p_link->tx_count -= (uint32_t)result;
        }
        else if (WSAGetLastError() != WSAEWOULDBLOCK)
            disconnect_link(index);
    }
}

void sim_tcp_stop(void)
{
    for (uint32_t i = 0u; i < link_count; ++i)
    {
        disconnect_link(i);
        close_socket(&links[i].listener);
        *links[i].p_reg->p_handle = NULL;
    }
    link_count = 0u;

    if (ready == 1u)
        (void)WSACleanup();
    ready = 0u;
}

static void sim_tcp_init(void)
{
    WSADATA data       = {0};
    const uint32_t now = (uint32_t)GetTickCount64();
    int error          = 0;
    sim_tcp_stop();
    p_registered = section_collect(SECTION_SIM_TCP);

    if (p_registered == NULL)
        return;
    error = WSAStartup(MAKEWORD(2, 2), &data);
    ready = (error == 0) ? 1u : 0u;

    for (section_item_t *p_item = p_registered; p_item != NULL; p_item = p_item->p_next)
    {
        sim_tcp_reg_t *p_reg   = p_item->p_obj;
        struct in_addr address = {0};
        memset(&p_reg->status, 0, sizeof(p_reg->status));
        *p_reg->p_handle = NULL;

        if (error != 0)
        {
            p_reg->status.last_error = (uint32_t)error;
            continue;
        }

        if (    (p_reg->p_name == NULL)
             || (p_reg->p_name[0] == '\0')
             || (p_reg->p_ip == NULL)
             || (p_reg->port == 0u)
             || (    (p_reg->role != SIM_TCP_SERVER)
                  && (p_reg->role != SIM_TCP_CLIENT))
             || (inet_pton(AF_INET, p_reg->p_ip, &address) != 1))
        {
            p_reg->status.last_error = WSAEINVAL;
            continue;
        }

        if (link_count == SIM_TCP_MAX_CHANNELS)
        {
            p_reg->status.last_error = WSAENOBUFS;
            continue;
        }
        sim_tcp_t *p_link = &links[link_count];
        memset(p_link, 0, sizeof(*p_link));
        p_link->p_reg    = p_reg;
        p_link->listener = INVALID_SOCKET;
        p_link->socket   = INVALID_SOCKET;
        p_link->retry_ms = now - SIM_RETRY_MS;
        *p_reg->p_handle = p_link;
        connect_poll(link_count, now);
        ++link_count;
    }
}

static void sim_tcp_task(void)
{
    const uint32_t now = (uint32_t)GetTickCount64();

    if (ready == 0u)
        return;

    for (uint32_t i = 0u; i < link_count; ++i)
    {
        connect_poll(i, now);
        transfer_poll(i);
    }
}
REG_INIT(10, sim_tcp_init)
REG_TASK(1, sim_tcp_task)

const sim_tcp_status_t *sim_tcp_get_status(const char *p_name)
{
    static const sim_tcp_status_t empty = {0};

    if (p_name == NULL)
        return &empty;

    for (const section_item_t *p_item = p_registered; p_item != NULL; p_item = p_item->p_next)
    {
        const sim_tcp_reg_t *p_reg = p_item->p_obj;

        if (    (p_reg->p_name != NULL)
             && (strcmp(p_name, p_reg->p_name) == 0))
            return &p_reg->status;
    }
    return &empty;
}

/** @param p_tcp Registered channel. @param p_data Source. @param len Whole-write byte count. */
void sim_tcp_tx(struct sim_tcp *p_tcp, const char *p_data, int len)
{
    sim_tcp_t *p_link = p_tcp; /* Destination ring. */
    uint32_t tail     = 0u;    /* Append position. */
    uint32_t first    = 0u;    /* Bytes before wrap. */

    if (    (p_link == NULL)
         || (p_data == NULL)
         || (len <= 0))
        return;

    if (    (ready == 0u)
         || (p_link->socket == INVALID_SOCKET)
         || (p_link->connecting == 1u)
         || ((uint32_t)len > SIM_TX_CAPACITY - p_link->tx_count))
    {
        if (p_link->p_reg->status.tx_dropped != UINT32_MAX)
            ++p_link->p_reg->status.tx_dropped;
        return;
    }
    tail = (p_link->tx_head + p_link->tx_count) % SIM_TX_CAPACITY;
    first = SIM_TX_CAPACITY - tail;

    if (first > (uint32_t)len)
        first = (uint32_t)len;
    memcpy(&p_link->tx[tail], p_data, first);
    memcpy(p_link->tx, &p_data[first], (uint32_t)len - first);
    p_link->tx_count += (uint32_t)len;
}

/** @param p_tcp Registered channel. @param p_data Destination. @return 1 for a consumed byte. */
uint8_t sim_tcp_rx_get_byte(struct sim_tcp *p_tcp, uint8_t *p_data)
{
    sim_tcp_t *p_link = p_tcp; /* Source ring. */

    if (    (p_link == NULL)
         || (p_data == NULL)
         || (p_link->rx_count == 0u))
        return 0u;
    *p_data = p_link->rx[p_link->rx_head];
    p_link->rx_head = (p_link->rx_head + 1u) % SIM_RX_CAPACITY;
    --p_link->rx_count;
    return 1u;
}

/** @param p_tcp Registered channel. @param p_format Format. @param args Arguments consumed synchronously. */
void sim_tcp_vprintf(struct sim_tcp *p_tcp, const char *p_format, va_list args)
{
    char text[1024] = {0}; /* Bounded console message. */
    int length      = 0;   /* Required bytes excluding terminator. */

    if (p_format == NULL)
        return;
    length = vsnprintf(text, sizeof(text), p_format, args);

    if (    (length > 0)
         && ((size_t)length < sizeof(text)))
        sim_tcp_tx(p_tcp, text, length);
}
