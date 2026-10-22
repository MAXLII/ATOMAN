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
#include "bsp_wifi_channels.h"
#include "bsp_system.h"
#include "comm_discovery.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static bsp_wifi_status_t radio;           /**< Injected network identity. */
static int32_t accept_peer  = -1;         /**< Next peer accepted once. */
static int32_t probe_result = -1;         /**< EAGAIN by default, zero for EOF. */
static int32_t read_result  = 1;          /**< Byte count or EOF from TCP read. */
static int32_t read_error   = ECONNRESET; /**< Injected read error after queued input is exhausted. */
static uint8_t tcp_input[512];            /**< Input bytes available to one bulk socket read. */
static uint32_t tcp_input_size, tcp_input_offset, read_calls, last_read_capacity;
static int32_t write_limit = 512;    /**< Accepted prefix per TCP write. */
static int32_t write_error = EAGAIN; /**< Error when write_limit is negative. */
static uint8_t tcp_output[8192];     /**< Accepted TCP bytes. */
static uint32_t tcp_size;            /**< Recorded TCP length. */
static uint8_t usb_output[8192];     /**< Accepted USB bytes. */
static uint32_t usb_size;            /**< Recorded USB length. */
static int32_t usb_limit = 512;      /**< Accepted prefix per USB write. */
static uint32_t close_count;         /**< Number of descriptors retired. */
static uint8_t udp_input[512];       /**< Next received datagram. */
static uint32_t udp_input_size;      /**< Pending datagram length. */
static uint8_t udp_output[512];      /**< Last accepted datagram. */
static uint32_t udp_output_size;     /**< Last accepted datagram length. */
static int32_t udp_write_error;      /**< Injected UDP send error. */
static int32_t upload_peer = -1, upload_probe = -1;
static uint32_t packet_pulls;
static bool packet_pending, packet_invalid;
static uint32_t packet_bytes = 6u;             /**< Source packet size for small-batch and full-staging cases. */
static uint32_t packet_frames, source_bytes;   /**< Queued 37-byte frames with a continuous byte pattern. */
static uint32_t upload_probes, control_probes; /**< Probe calls counted independently of connection state. */
static uint32_t upload_writes, last_upload_request; /**< Requested socket sizes, including partial and rejected writes. */
static uint32_t clock_us;         /**< Test-owned monotonic clock, including deliberate 32-bit wrap. */
static uint32_t external_depth;   /**< Active simulated task-switch guards; sampling IRQs are not masked. */
static uint32_t external_peak;    /**< Highest nesting reached by formatting plus queue publication. */
static uint32_t external_entries; /**< Guard entry count, proving output paths exercise the contract. */
void bsp_system_runtime_enter(void)
{
    ++external_depth;
    ++external_entries;

    if (external_depth > external_peak)
        external_peak = external_depth;
}
void bsp_system_runtime_exit(void)
{
    assert(external_depth != 0u);
    --external_depth;
}
uint32_t bsp_system_time_us_get(void)
{
    return clock_us;
}
static uint32_t packet_read(uint8_t *p_data, uint32_t capacity)
{
    ++packet_pulls;
    assert(capacity <= BSP_WIFI_UPLOAD_BUFFER_SIZE);

    if (packet_invalid)
        return capacity + 1u;

    if (packet_frames != 0u)
    {
        uint32_t frames = capacity / 37u;

        if (frames > packet_frames)
            frames = packet_frames;

        for (uint32_t i = 0u; i < frames * 37u; ++i)
            p_data[i] = (uint8_t)source_bytes++;
        packet_frames -= frames;
        return frames * 37u;
    }

    if (    !packet_pending
         || capacity < packet_bytes)
        return 0u;

    if (packet_bytes == 6u)
        memcpy(p_data, "abcdef", 6u);
    else
        memset(p_data, 0x5A, packet_bytes);
    packet_pending = false;
    return packet_bytes;
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

    if (socket >= 100)
        ++upload_probes;
    else
        ++control_probes;
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
            && capacity != 0u);
    ++read_calls;
    last_read_capacity = capacity;

    if (tcp_input_offset < tcp_input_size)
    {
        uint32_t count = tcp_input_size - tcp_input_offset;

        if (count > capacity)
            count = capacity;
        memcpy(p_data, &tcp_input[tcp_input_offset], count);
        tcp_input_offset += count;
        return (int32_t)count;
    }
    errno = read_error;

    if (read_result > 0)
    {
        assert((uint32_t)read_result <= capacity);
        memset(p_data, 0xE8, (size_t)read_result);
    }
    return read_result;
}
int32_t bsp_wifi_tcp_write(int32_t socket, const uint8_t *p_data, uint32_t length)
{
    uint32_t count = 0u;
    assert(socket >= 20);

    if (socket >= 100)
    {
        ++upload_writes;
        last_upload_request = length;
    }

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

static void test_control_receive(void)
{
    uint8_t byte        = 0u;
    uint32_t reads      = read_calls;
    uint32_t generation = bsp_comm_tcp_session();
    tcp_input_size      = 300u;
    tcp_input_offset    = 0u;
    read_result         = -1;
    read_error          = EAGAIN;

    for (uint32_t i = 0u; i < tcp_input_size; ++i)
        tcp_input[i] = (uint8_t)i;

    for (uint32_t i = 0u; i < 128u; ++i)
        assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
                && byte == (uint8_t)i);
    assert(    read_calls == reads + 1u
            && last_read_capacity == 256u);
    clock_us += 500u;
    bsp_comm_poll();

    for (uint32_t i = 128u; i < 256u; ++i)
        assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
                && byte == (uint8_t)i);
    assert(read_calls == reads + 1u); /* The next Link round consumes the original read's tail without another SDK call. */

    for (uint32_t i = 256u; i < 300u; ++i)
        assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
                && byte == (uint8_t)i);
    assert(read_calls == reads + 2u);
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 0u
            && read_calls == reads + 3u);
    assert(bsp_comm_tcp_session() == generation); /* EAGAIN retains the same peer. */
    read_result = 0;
    assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u);
    assert(bsp_comm_tcp_session() != generation); /* EOF retires a drained input session. */

    read_result      = -1;
    accept_peer      = 30;
    tcp_input_size   = 2u;
    tcp_input_offset = 0u;
    memcpy(tcp_input, "AB", 2u);
    bsp_comm_poll();
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == (uint8_t)'A');
    assert(bsp_comm_wifi_rx_get_byte(NULL) == 0u);
    radio.ready = 0u;
    bsp_comm_poll();
    assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u);
    radio.ready      = 1u;
    accept_peer      = 31;
    tcp_input_offset = 0u;
    memcpy(tcp_input, "CD", 2u);
    bsp_comm_poll();
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == (uint8_t)'C'); /* Radio loss discarded the old B. */

    for (int32_t failure = -1; failure <= 0; ++failure)
    {
        generation = bsp_comm_tcp_session();
        bsp_comm_wifi_tx("x", 1);
        write_limit = failure;
        write_error = ECONNRESET;
        bsp_comm_poll();
        assert(bsp_comm_tcp_session() != generation);
        assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u); /* Both error and zero-length send retire buffered input immediately. */
        accept_peer      = 33 + failure;
        tcp_input_offset = 0u;
        memcpy(tcp_input, "EF", 2u);
        write_limit = 512;
        write_error = EAGAIN;
        bsp_comm_poll();
        assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
                && byte == (uint8_t)'E');
    }

    bsp_wifi_channel_reset(BSP_WIFI_CHANNEL_CONTROL);
    accept_peer = 34;
    bsp_wifi_channel_poll(); /* Change the peer without the Comm poll; byte access must still notice the generation. */
    tcp_input_offset = 0u;
    memcpy(tcp_input, "GH", 2u);
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == (uint8_t)'G');
    probe_result = 0;
    clock_us += 1000u;
    bsp_comm_poll();
    assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u); /* Probe EOF also discards the buffered H. */
    probe_result     = -1;
    accept_peer      = 35;
    tcp_input_offset = 0u;
    memcpy(tcp_input, "IJ", 2u);
    bsp_comm_poll();
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == (uint8_t)'I');
    assert(    bsp_comm_wifi_rx_get_byte(&byte) == 1u
            && byte == (uint8_t)'J');
    assert(bsp_comm_wifi_rx_get_byte(&byte) == 0u);
    tcp_input_offset = 0u;
    tcp_input_size   = 0u;
    read_result      = 1;
    read_error       = ECONNRESET;
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
    clock_us += 1000u;
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
    test_control_receive();
    assert(!bsp_wifi_channel_configure(2u, 9001u, packet_read));
    assert(!bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 5000u, packet_read));
    assert(bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9001u, packet_read));
    assert(!bsp_wifi_channel_configure(BSP_WIFI_CHANNEL_UPLOAD, 9002u, packet_read));
    upload_peer = 100;
    bsp_comm_poll();
    bsp_wifi_channel_status_t upload;
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(upload.connected);
    uint32_t probes   = upload_probes;
    uint32_t controls = control_probes;

    for (uint32_t poll = 0u; poll < 10u; ++poll)
        bsp_comm_poll();
    assert(upload_probes == probes);    /* Newly accepted upload peers are probed once, then at most every 1 ms. */
    assert(control_probes == controls); /* Both endpoints probe at most once per 1 ms, independent of transport polls. */
    tcp_size       = 0u;
    packet_pending = true;
    write_limit    = 2;
    bsp_comm_poll();
    assert(tcp_size == 0u); /* Tiny writes aggregate before the 1 ms deadline. */
    clock_us += BSP_WIFI_UPLOAD_BATCH_US;
    bsp_comm_poll();
    assert(tcp_size == 2u);
    uint32_t pulls = packet_pulls;
    write_limit    = -1;
    packet_pending = true;
    bsp_comm_poll();
    assert(    tcp_size == 2u
            && packet_pulls == pulls + 1u
            &&!packet_pending); /* New bytes append behind an existing partial send, even during EAGAIN. */
    write_limit = 512;
    bsp_comm_poll();
    assert(    tcp_size == 6u
            && memcmp(tcp_output, "abcdef", 6u) == 0);
    clock_us += BSP_WIFI_UPLOAD_BATCH_US - 1u;
    packet_pending = true;
    bsp_comm_poll();
    assert(tcp_size == 6u); /* The second group keeps its own timestamp after the old prefix drains. */
    ++clock_us;
    bsp_comm_poll();
    assert(    tcp_size == 18u
            && memcmp(tcp_output, "abcdefabcdefabcdef", 18u) == 0); /* Appending does not renew the oldest tail's deadline. */
    packet_pending = true;
    write_limit    = 2;
    bsp_comm_poll();
    clock_us += BSP_WIFI_UPLOAD_BATCH_US;
    bsp_comm_poll();
    assert(tcp_size == 20u);
    generation                 = bsp_comm_tcp_session();
    uint32_t upload_generation = upload.session;
    upload_probe               = 0;
    write_limit                = -1;
    probes                     = upload_probes;
    bsp_comm_poll();
    assert(upload_probes == probes);
    clock_us += BSP_WIFI_UPLOAD_BATCH_US - 1u;
    bsp_comm_poll();
    assert(upload_probes == probes);
    ++clock_us;
    bsp_comm_poll();
    assert(upload_probes == probes + 1u);
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.session != upload_generation
            && upload.error == 3u);
    assert(bsp_comm_tcp_session() == generation);
    upload_probe = -1;
    upload_peer  = 101;
    write_limit  = 512;
    bsp_comm_poll();
    assert(tcp_size == 20u); /* The previous peer's unsent four-byte tail was discarded. */

    packet_bytes   = BSP_WIFI_UPLOAD_BUFFER_SIZE;
    packet_pending = true;
    write_limit    = -1;
    bsp_comm_poll();
    assert(tcp_size == 20u); /* A full staging buffer attempts immediately, then survives EAGAIN. */
    assert(g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].tx_high_water == BSP_WIFI_UPLOAD_BUFFER_SIZE);
    assert(g_bsp_wifi_debug[BSP_WIFI_CHANNEL_UPLOAD].backpressure == 1u);
    uint32_t full_bytes = (BSP_WIFI_UPLOAD_BUFFER_SIZE / 1440u) * 1440u;
    uint32_t full_writes = (full_bytes + 511u) / 512u;
    uint32_t tail_writes = (BSP_WIFI_UPLOAD_BUFFER_SIZE - full_bytes + 511u) / 512u;
    assert(last_upload_request == full_bytes); /* A 3072-byte buffer emits two MSS, while 2500 bytes emits one. */
    write_limit = 512;
    bsp_comm_poll();
    assert(tcp_size == 532u);

    for (uint32_t part = 1u; part < full_writes; ++part)
        bsp_comm_poll();
    assert(tcp_size == 20u + full_bytes); /* Only the sub-MSS remainder waits for the batching deadline. */
    clock_us += BSP_WIFI_UPLOAD_BATCH_US;

    for (uint32_t part = 0u; part < tail_writes; ++part)
        bsp_comm_poll();
    assert(tcp_size == 20u + BSP_WIFI_UPLOAD_BUFFER_SIZE);

    for (uint32_t offset = 20u; offset < tcp_size; ++offset)
        assert(tcp_output[offset] == 0x5Au);

    tcp_size        = 0u;
    source_bytes    = 0u;
    packet_frames   = 39u;
    write_limit     = (int32_t)BSP_WIFI_UPLOAD_BUFFER_SIZE;
    uint32_t writes = upload_writes;
    bsp_comm_poll();
    assert(    tcp_size == 1440u
            && source_bytes == 1443u
            && last_upload_request == 1440u
            && upload_writes == writes + 1u); /* 39 whole frames produce one MSS and a retained three-byte suffix. */
    clock_us += BSP_WIFI_UPLOAD_BATCH_US - 1u;
    packet_frames = 38u;
    bsp_comm_poll();
    assert(tcp_size == 1440u);
    ++clock_us;
    packet_frames = 1u;
    bsp_comm_poll();
    assert(tcp_size == 2880u); /* The retained three bytes stay in front of every newly appended frame. */
    clock_us += BSP_WIFI_UPLOAD_BATCH_US - 2u;
    bsp_comm_poll();
    assert(tcp_size == 2880u);
    ++clock_us;
    bsp_comm_poll();
    assert(tcp_size == 2886u);

    for (uint32_t offset = 0u; offset < tcp_size; ++offset)
        assert(tcp_output[offset] == (uint8_t)offset);

    tcp_size      = 0u;
    source_bytes  = 0u;
    packet_frames = 39u;
    write_limit   = 500;
    bsp_comm_poll();
    assert(tcp_size == 500u);
    clock_us += 200u;
    packet_frames = 38u;
    write_limit   = -1;
    bsp_comm_poll();
    assert(    source_bytes == 2849u
            && packet_frames == 0u
            && tcp_size == 500u);
    write_limit = 1000;
    bsp_comm_poll();
    assert(    tcp_size == 1440u
            && last_upload_request == 940u);
    clock_us += 100u;
    packet_frames = 1u;
    bsp_comm_poll();
    assert(tcp_size == 2440u);
    write_limit = (int32_t)BSP_WIFI_UPLOAD_BUFFER_SIZE;
    bsp_comm_poll();
    assert(    tcp_size == 2880u
            && last_upload_request == 440u);
    clock_us += 900u;
    bsp_comm_poll();
    assert(tcp_size == 2886u);

    for (uint32_t offset = 0u; offset < tcp_size; ++offset)
        assert(tcp_output[offset] == (uint8_t)offset); /* Partial sends, append, and EAGAIN preserve the exact stream. */

    tcp_size       = 0u;
    packet_bytes   = 6u;
    packet_pending = true;
    clock_us       = UINT32_MAX - 500u;
    bsp_comm_poll();
    probes         = upload_probes;
    controls       = control_probes;
    packet_pending = true;
    clock_us += 500u;
    bsp_comm_poll();
    assert(    tcp_size == 0u
            && upload_probes == probes
            && control_probes == controls); /* Both endpoints' probe and aggregation deadlines tolerate clock wrap. */
    clock_us += 500u;
    bsp_comm_poll();
    assert(    tcp_size == 12u
            && upload_probes == probes + 1u
            && control_probes == controls + 1u);
    assert(memcmp(tcp_output, "abcdefabcdef", 12u) == 0);

    tcp_size                 = 0u;
    source_bytes             = 0u;
    uint32_t supplied_frames = 0u;

    for (    uint32_t poll = 0u; poll < 2000u
                              && tcp_size < 7400u;
             ++poll)
    {
        if (supplied_frames < 200u)
        {
            ++packet_frames;
            ++supplied_frames;
        }
        write_limit = poll % 5u == 0u ? -1 : poll % 3u == 0u ? 13 : 1200;
        clock_us += (poll % 4u) * 200u;
        bsp_comm_poll();
    }
    assert(    tcp_size == 7400u
            && source_bytes == 7400u
            && packet_frames == 0u);

    for (uint32_t offset = 0u; offset < tcp_size; ++offset)
        assert(tcp_output[offset] == (uint8_t)offset); /* Sustained short writes and EAGAIN never duplicate or drop bytes. */

    upload_probe = 1;
    clock_us += BSP_WIFI_UPLOAD_BATCH_US;
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
    packet_frames  = 39u;
    write_limit    = 0;
    bsp_comm_poll();
    bsp_wifi_channel_status_get(BSP_WIFI_CHANNEL_UPLOAD, &upload);
    assert(    !upload.connected
            && upload.error == 1u); /* Hard send failure closes immediately, independent of the probe deadline. */
    bsp_comm_usb_printf(NULL);
    bsp_comm_wifi_printf(NULL);
    bsp_comm_usb_tx(NULL, 1);
    bsp_comm_wifi_tx(NULL, 1);
    assert(external_depth == 0u); /* Successful, rejected and empty writes all release their guard. */
    assert(external_peak == 2u);  /* Formatting holds its guard through the nested enqueue transaction. */
    assert(external_entries != 0u);
    printf(
        "PASS real BSP (%u-byte staging): copied queues, 256-byte control RX/session isolation, partial sends/EAGAIN, discovery/UDP, MSS/tail append, 1 ms aggregation/probe with wrap, exact byte stream, stale-tail discard and source bounds\n",
        (unsigned)BSP_WIFI_UPLOAD_BUFFER_SIZE);
    return 0;
}
