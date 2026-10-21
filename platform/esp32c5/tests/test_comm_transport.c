// SPDX-License-Identifier: MIT
/**
 * @file test_comm_transport.c
 * @brief Fault injection for real BSP queues, sessions and discovery adaptation.
 * @details Part of base. Only the lower USB/Wi-Fi primitive calls are stubbed.
 *          Tests own all time and peer transitions; no hardware claims are made.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "bsp_comm.h"
#include "bsp_usart.h"
#include "bsp_wifi.h"
#include "bsp_system.h"
#include "comm_discovery.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static bsp_wifi_status_t radio;       /**< Injected network identity. */
static int32_t accept_peer  = -1;     /**< Next peer accepted once. */
static int32_t probe_result = -1;     /**< EAGAIN by default, zero for EOF. */
static int32_t read_result  = 1;      /**< Byte count or EOF from TCP read. */
static int32_t write_limit  = 512;    /**< Accepted prefix per TCP write. */
static int32_t write_error  = EAGAIN; /**< Error when write_limit is negative. */
static uint8_t tcp_output[8192];      /**< Accepted TCP bytes. */
static uint32_t tcp_size;             /**< Recorded TCP length. */
static uint8_t usb_output[8192];      /**< Accepted USB bytes. */
static uint32_t usb_size;             /**< Recorded USB length. */
static int32_t usb_limit = 512;       /**< Accepted prefix per USB write. */
static uint32_t close_count;          /**< Number of descriptors retired. */
static uint8_t udp_input[512];        /**< Next received datagram. */
static uint32_t udp_input_size;       /**< Pending datagram length. */
static uint8_t udp_output[512];       /**< Last accepted datagram. */
static uint32_t udp_output_size;      /**< Last accepted datagram length. */
static int32_t udp_write_error;       /**< Injected UDP send error. */
static int32_t upload_peer = -1, upload_probe = -1;
static uint32_t packet_pulls;
static bool packet_pending, packet_invalid;
uint32_t bsp_system_time_us_get(void)
{
    static uint32_t clock_us; /**< Deterministic elapsed time for transport diagnostics. */
    return ++clock_us;
}
static uint32_t packet_read(uint8_t *p_data, uint32_t capacity)
{
    ++packet_pulls;
    assert(capacity == BSP_WIFI_SOCKET_TRANSFER_MAX);

    if (packet_invalid)
        return capacity + 1u;

    if (!packet_pending)
        return 0u;
    memcpy(p_data, "abcdef", 6u);
    packet_pending = false;
    return 6u;
}

void bsp_wifi_status_get(bsp_wifi_status_t *p_status)
{
    *p_status = radio;
}
int32_t bsp_wifi_mac_get(uint8_t p_mac[6])
{
    memset(p_mac, 0x12, 6u);
    return 0;
}
int32_t bsp_wifi_tcp_listen(uint16_t port)
{
    assert(    port == 5000u
            || port == 9001u);
    return port == 5000u ? 10 : 12;
}
int32_t bsp_wifi_udp_bind(uint16_t port)
{
    assert(port == 5000u);
    return 11;
}
int32_t bsp_wifi_tcp_accept(int32_t listener)
{
    int32_t result = accept_peer;
    assert(    listener == 10
            || listener == 12);

    if (listener == 12)
    {
        result      = upload_peer;
        upload_peer = -1;
        errno       = EAGAIN;
        return result;
    }
    accept_peer = -1;
    errno       = EAGAIN;
    return result;
}
int32_t bsp_wifi_tcp_probe(int32_t socket)
{
    assert(socket >= 20);
    errno = EAGAIN;
    return socket >= 100 ? upload_probe : probe_result;
}
int32_t bsp_wifi_socket_close(int32_t socket)
{
    assert(socket >= 10);
    ++close_count;
    return 0;
}
int32_t bsp_wifi_tcp_read(int32_t socket, uint8_t *p_data, uint32_t capacity)
{
    assert(    socket >= 20
            && capacity == 1u);
    *p_data = 0xE8u;
    errno   = ECONNRESET;
    return read_result;
}
int32_t bsp_wifi_tcp_write(int32_t socket, const uint8_t *p_data, uint32_t length)
{
    uint32_t count = 0u;
    assert(socket >= 20);

    if (write_limit < 0)
    {
        errno = write_error;
        return -1;
    }
    count = length < (uint32_t)write_limit ? length : (uint32_t)write_limit;
    assert(tcp_size + count <= sizeof(tcp_output));
    memcpy(&tcp_output[tcp_size], p_data, count);
    tcp_size += count;
    return (int32_t)count;
}
int32_t bsp_usart_write(const uint8_t *p_data, uint32_t length)
{
    uint32_t count = length < (uint32_t)usb_limit ? length : (uint32_t)usb_limit;
    assert(usb_size + count <= sizeof(usb_output));
    memcpy(&usb_output[usb_size], p_data, count);
    usb_size += count;
    return (int32_t)count;
}
int32_t bsp_wifi_udp_read(int32_t socket,
                          uint8_t *p_data,
                          uint32_t capacity,
                          bsp_wifi_peer_t *p_peer)
{
    uint32_t count = udp_input_size;
    assert(socket == 11);

    if (count == 0u)
    {
        errno = EAGAIN;
        return -1;
    }
    assert(count <= capacity);
    memcpy(p_data, udp_input, count);
    udp_input_size  = 0u;
    p_peer->ipv4_be = 1u;
    p_peer->port    = 12345u;
    return (int32_t)count;
}
int32_t bsp_wifi_udp_write(int32_t socket,
                           const uint8_t *p_data,
                           uint32_t length,
                           const bsp_wifi_peer_t *p_peer)
{
    assert(    socket == 11
            && p_peer->port == 12345u
            && length <= sizeof(udp_output));

    if (udp_write_error != 0)
    {
        errno = udp_write_error;
        return -1;
    }
    memset(udp_output, 0, sizeof(udp_output));
    memcpy(udp_output, p_data, length);
    udp_output_size = length;
    return (int32_t)length;
}

int main(void)
{
    uint8_t byte        = 0u;
    char message[]      = "abcdef";
    uint32_t generation = 0u;
    bsp_comm_wifi_tx(message, 6); /* Disconnected messages cannot leak to a future peer. */
    bsp_comm_usb_tx(message, 6);
    message[0] = 'X';
    usb_limit  = 2;
    bsp_comm_poll();
    bsp_comm_poll();
    bsp_comm_poll();
    assert(    usb_size == 6u
            && memcmp(usb_output, "abcdef", 6u) == 0);
    radio.ready   = 1u;
    radio.ipv4_be = 0x1101A8C0u;
    accept_peer   = 20;
    bsp_comm_poll();
    generation = bsp_comm_tcp_session();
    assert(    generation != 0u
            && tcp_size == 0u);
    bsp_comm_wifi_tx("abcdef", 6);
    write_limit = 2;
    bsp_comm_poll();
    write_limit = -1;
    bsp_comm_poll();
    assert(tcp_size == 2u);
    write_limit = 512;
    bsp_comm_poll();
    assert(    tcp_size == 6u
            && memcmp(tcp_output, "abcdef", 6u) == 0);
    tcp_size    = 0u;
    write_limit = -1;

    for (uint32_t i = 0u; i < 17u; ++i)
    {
        bsp_comm_wifi_tx("Q", 1);
    }
    assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u); /* Full TX queue backpressures requests. */
    write_limit = 512;

    for (uint32_t i = 0u; i < 16u; ++i)
    {
        bsp_comm_poll();
    }
    assert(    tcp_size == 16u
            && bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == 0xE8u);
    bsp_comm_wifi_tx("old", 3);
    bsp_comm_usb_tx("USB", 3);
    usb_limit    = 0;
    probe_result = 0;
    bsp_comm_poll();
    assert(bsp_comm_tcp_session() != generation);
    generation   = bsp_comm_tcp_session();
    probe_result = -1;
    accept_peer  = 21;
    usb_limit    = 512;
    bsp_comm_poll();
    assert(    bsp_comm_tcp_session() != generation
            && tcp_size == 16u
            && usb_size == 9u);
    assert(memcmp(&usb_output[6], "USB", 3u) == 0);
    generation  = bsp_comm_tcp_session();
    read_result = -1;
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 0u
            && bsp_comm_tcp_session() != generation);
    read_result = 1;
    accept_peer = 22;
    bsp_comm_poll();
    generation  = bsp_comm_tcp_session();
    write_limit = -1;
    write_error = ECONNRESET;
    bsp_comm_wifi_tx("old", 3);
    bsp_comm_poll();
    assert(bsp_comm_tcp_session() != generation);
    write_limit = 512;
    write_error = EAGAIN;
    accept_peer = 23;
    bsp_comm_poll();
    bsp_comm_wifi_printf("%s:%u", "ok", 12u);
    bsp_comm_poll();
    assert(    tcp_size == 21u
            && memcmp(&tcp_output[16], "ok:12", 5u) == 0);
    bsp_comm_usb_tx(NULL, 1);
    bsp_comm_usb_tx("", 0);
    bsp_comm_usb_tx("", 513);
    bsp_comm_poll();
    assert(usb_size == 9u);
    comm_discovery_init("test-fw");
    memcpy(udp_input, "FRAME_DISCOVER_V1", 17u);
    udp_input_size  = 17u;
    udp_write_error = EAGAIN;
    comm_discovery_poll();
    assert(udp_output_size == 0u);
    udp_write_error = 0;
    comm_discovery_poll();
    assert(strstr((const char *)udp_output, "FRAME_DEVICE_V1;name=ESP32-C5;ip=192.168.1.17;") != NULL);
    assert(strstr((const char *)udp_output, "mac=12:12:12:12:12:12;fw_version=test-fw;") != NULL);
    udp_input[0]   = 0u;
    udp_input[1]   = 0xE9u;
    udp_input_size = 2u;
    comm_discovery_poll();
    assert(    udp_output_size == 2u
            && udp_output[1] == 0xE9u);
    udp_input[0]    = 7u;
    udp_input_size  = 1u;
    udp_write_error = EAGAIN;
    comm_discovery_poll();
    generation  = bsp_comm_tcp_session();
    radio.ready = 0u;
    bsp_comm_poll();
    comm_discovery_poll();
    assert(    bsp_comm_tcp_session() != generation
            && close_count >= 5u);
    radio.ready     = 1u;
    radio.ipv4_be   = 0x1201A8C0u;
    accept_peer     = 24;
    udp_write_error = 0;
    bsp_comm_poll();
    comm_discovery_poll();
    assert(udp_output_size == 2u); /* Old pending UDP reply was discarded on loss. */
    assert(!bsp_wifi_channel_configure(2u, 9001u, packet_read));
    assert(!bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 5000u, packet_read));
    assert(bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9001u, packet_read));
    assert(!bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9002u, packet_read));
    upload_peer = 100;
    bsp_comm_poll();
    bsp_wifi_channel_status_t upload;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(upload.connected);
    tcp_size       = 0u;
    packet_pending = true;
    write_limit    = 2;
    bsp_comm_poll();
    assert(tcp_size == 2u);
    uint32_t pulls = packet_pulls;
    write_limit    = -1;
    bsp_comm_poll();
    assert(    tcp_size == 2u
            && packet_pulls == pulls);
    write_limit = 512;
    bsp_comm_poll();
    assert(    tcp_size == 6u
            && memcmp(tcp_output, "abcdef", 6u) == 0);
    packet_pending = true;
    write_limit    = 2;
    bsp_comm_poll();
    assert(tcp_size == 8u);
    generation                 = bsp_comm_tcp_session();
    uint32_t upload_generation = upload.session;
    upload_probe               = 0;
    bsp_comm_poll();
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.session != upload_generation
            && upload.error == 3u);
    assert(bsp_comm_tcp_session() == generation);
    upload_probe = -1;
    upload_peer  = 101;
    write_limit  = 512;
    bsp_comm_poll();
    assert(tcp_size == 8u);
    upload_probe = 1;
    bsp_comm_poll();
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.error == 2u);
    upload_probe   = -1;
    upload_peer    = 102;
    packet_invalid = true;
    bsp_comm_poll();
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.error == 4u);
    packet_invalid = false;
    upload_peer    = 103;
    packet_pending = true;
    write_limit    = 0;
    bsp_comm_poll();
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.error == 1u);
    puts(
        "PASS real BSP: copied queues, partial sends/EAGAIN, session isolation, discovery/UDP, upload packet fetch, stale-tail discard and invalid input/source bounds");
    return 0;
}
