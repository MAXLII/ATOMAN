// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope_service.c
 * @brief Registered, hardware-independent real-time scope COMM service.
 * @details Part of base. Registers 01/3A, validates channel and rate requests,
 *          controls the portable sampler and formats E8/E9 direct ACKs.
 *          No SDK, RTOS, timer, socket or MCU address rules live in this module.
 *          Entity: one sampler and a fixed request queue owned by SECTION.
 *          Prior: control-loop initialization supplies the real interrupt frequency.
 *          Time: REG_TASK manages configuration; the application registers ISR sampling.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope_service.h"
#include <stddef.h>
#include <string.h>

#if REALTIME_SCOPE_ENABLE
static realtime_scope_t sampler;
static realtime_scope_t *p_sampler;
static realtime_scope_port_t port;
static realtime_scope_control_t requests[REALTIME_SCOPE_REQUEST_DEPTH];
static uint32_t request_head, request_count, sample_frequency, active_session;

static uint32_t read_le(const uint8_t *p_data, uint32_t size)
{
    uint32_t value = 0u;

    for (uint32_t i = 0u; i < size; ++i)
        value |= (uint32_t)p_data[i] << (i * 8u);
    return value;
}
static void write_le(uint8_t *p_data, uint32_t value, uint32_t size)
{
    for (uint32_t i = 0u; i < size; ++i)
        p_data[i] = (uint8_t)(value >> (i * 8u));
}
static void finish(realtime_scope_control_t *p_control,
                   uint8_t status,
                   const uint8_t *p_data,
                   uint16_t size)
{
    section_packform_t request = p_control->packet;
    p_control->packet.src      = request.dst;
    p_control->packet.d_src    = request.d_dst;
    p_control->packet.dst      = request.src;
    p_control->packet.d_dst    = request.d_src;
    p_control->packet.is_ack   = 1u;
    p_control->packet.len      = (uint16_t)(size + 4u);
    p_control->packet.p_data   = NULL;
    p_control->data[3]         = status;

    if (size != 0u)
        memcpy(&p_control->data[4], p_data, size);
}
static void send_reply(realtime_scope_control_t *p_reply)
{
    p_reply->packet.p_data = p_reply->data;
    comm_send_data(&p_reply->packet, p_reply->p_output);
}
static void control_request(void *p_pack, DEC_MY_PRINTF)
{
    /* COMM owns the generic callback type; this is its typed boundary. */
    const section_packform_t *p_request = p_pack;

    if (    p_request == NULL
         || p_request->p_data == NULL
         || p_request->is_ack != 0u
         || p_request->len < 3u
         || p_request->len > REALTIME_SCOPE_CONTROL_BYTES
         || (    p_request->dst == 0u
              && p_request->d_dst == 0u))
        return;
    realtime_scope_control_t request = {.packet = *p_request, .p_output = my_printf};
    memcpy(request.data, p_request->p_data, p_request->len);
    request.packet.p_data = NULL;

    bool queued = false;

    if (p_sampler != NULL)
    {
        port.p_lock();

        if (request_count < REALTIME_SCOPE_REQUEST_DEPTH)
        {
            requests[(request_head + request_count) % REALTIME_SCOPE_REQUEST_DEPTH] = request;
            ++request_count;
            queued = true;
        }
        port.p_unlock();
    }

    if (!queued)
    {
        finish(&request, 7u, NULL, 0u);
        send_reply(&request);
        return;
    }
}
static void process(realtime_scope_control_t *p_control);
static void stop_acquisition(void)
{
    port.p_lock();
    realtime_scope_stop(p_sampler);
    p_sampler->head  = 0u;
    p_sampler->count = 0u;
    port.p_unlock();
}
static void manage(void)
{
    if (p_sampler == NULL)
        return;
    uint32_t session = port.p_data_session();

    if (    !port.p_data_connected()
         || session != active_session)
        stop_acquisition();
    active_session = session;

    for (uint32_t i = 0u; i < REALTIME_SCOPE_REQUEST_DEPTH; ++i)
    {
        realtime_scope_control_t request;
        port.p_lock();

        if (request_count == 0u)
        {
            port.p_unlock();
            break;
        }
        request = requests[request_head];
        request_head = (request_head + 1u) % REALTIME_SCOPE_REQUEST_DEPTH;
        --request_count;
        port.p_unlock();
        process(&request);
        send_reply(&request);
    }
}
void realtime_scope_service_sample(void)
{
    if (p_sampler != NULL)
        realtime_scope_sample(p_sampler);
}
REG_COMM(REALTIME_SCOPE_CMD_SET, REALTIME_SCOPE_CMD_WORD, control_request)
REG_TASK(1, manage)

bool realtime_scope_service_bind(const realtime_scope_port_t *p_port, uint8_t *p_queue, uint32_t capacity)
{
    if (    p_sampler != NULL
         || p_port == NULL
         || p_port->byte_budget < 6u
         || p_port->p_lock == NULL
         || p_port->p_unlock == NULL
         || p_port->p_stop == NULL
         || p_port->p_data_connected == NULL
         || p_port->p_data_session == NULL
         || p_port->p_identity == NULL
         || p_port->p_resolve == NULL)
        return false;

    if (!realtime_scope_init(&sampler, p_queue, capacity))
        return false;
    port           = *p_port;
    p_sampler      = &sampler;
    active_session = port.p_data_session();
    return true;
}
bool realtime_scope_service_source_set(uint32_t frequency)
{
    if (    p_sampler == NULL
         || p_sampler->running)
        return false;
    sample_frequency = frequency;
    return true;
}
uint32_t realtime_scope_service_source_frequency_get(void)
{
    return sample_frequency;
}
uint32_t realtime_scope_service_read(uint8_t *p_data, uint32_t capacity)
{
    if (    p_sampler == NULL
         || p_data == NULL
         || !port.p_data_connected()
         || port.p_data_session() != active_session)
        return 0u;
    uint32_t size = 0u;
    realtime_scope_frame_t frame;

    while (capacity - size >= 6u)
    {
        port.p_lock();
        uint32_t frame_size = (uint32_t)p_sampler->p_active->payload_size + 5u;

        if (capacity - size < frame_size)
        {
            port.p_unlock();
            break;
        }
        bool available = realtime_scope_pop(p_sampler, &frame);
        port.p_unlock();

        if (!available)
            break;
        memcpy(&p_data[size], frame.data, frame.size);
        size += frame.size;
    }
    return size;
}
static void process(realtime_scope_control_t *p_control)
{
    if (p_control == NULL)
        return;
    uint8_t result[64] = {0};
    uint16_t size      = 0u;
    uint8_t status     = 0u;

    if (    p_sampler == NULL
         || p_control->packet.len < 3u
         || p_control->packet.len > REALTIME_SCOPE_CONTROL_BYTES)
    {
        finish(p_control, 1u, NULL, 0u);
        return;
    }
    const uint8_t *p_data = &p_control->data[3];
    uint32_t length       = (uint32_t)p_control->packet.len - 3u;

    switch (p_control->data[0])
    {
    case 1u:
        result[0] = 1u;
        result[1] = REALTIME_SCOPE_CHANNELS;
        write_le(&result[2], sample_frequency, 4u);
        write_le(&result[6], port.byte_budget, 4u);
        port.p_identity(&result[10]);
        write_le(&result[42], sample_frequency, 4u);
        size = 46u;
        break;
    case 2u:
        port.p_stop();
        stop_acquisition();
        active_session = port.p_data_session();
        break;
    case 3u:
        realtime_scope_clear(p_sampler);
        write_le(result, sample_frequency, 4u);
        size = 4u;
        break;
    case 4u:

        if (length < 6u)
        {
            status = 1u;
            break;
        }
        {
            uint32_t address               = read_le(&p_data[2], 4u);
            realtime_scope_type_e type     = (realtime_scope_type_e)p_data[1];
            uint8_t width                  = realtime_scope_type_size(type);
            const volatile void *p_address = NULL;

            if (    address != 0u
                 && width != 0u)
                p_address = port.p_resolve(address, width);

            if (    (    address != 0u
                      && p_address == NULL)
                 || !realtime_scope_set(p_sampler, p_data[0], p_address, type))
                status = 2u;
        }

        if (status == 0u)
        {
            write_le(result, sample_frequency, 4u);
            size = 4u;
        }
        break;
    case 5u:
        port.p_lock();

        if (!realtime_scope_swap(p_sampler))
            status = 3u;
        else
        {
            write_le(result, p_sampler->p_active->payload_size, 2u);
            write_le(&result[2], sample_frequency, 4u);
            size = 6u;
        }
        port.p_unlock();
        break;
    case 6u:

        if (    length != 0u
             && length != 4u)
        {
            status = 1u;
            break;
        }
        {
            uint32_t frequency = sample_frequency;

            if (frequency == 0u)
            {
                status = 5u;
                break;
            }

            if (    (    length == 4u
                      && read_le(p_data, 4u) != frequency)
                 || frequency > port.byte_budget / ((uint32_t)p_sampler->p_active->payload_size + 5u))
            {
                status = 4u;
                break;
            }
            port.p_lock();
            bool started = !p_sampler->running
                        && port.p_data_connected()
                        && realtime_scope_start(p_sampler);
            port.p_unlock();

            if (!started)
            {
                status = 3u;
                break;
            }

            write_le(result, frequency, 4u);
            size = 4u;
        }
        break;
    case 7u:
        port.p_lock();
        result[0] = p_sampler->running ? 1u : 0u;
        write_le(&result[1], p_sampler->samples, 4u);
        write_le(&result[5], p_sampler->overflows, 4u);
        write_le(&result[9], p_sampler->count, 4u);
        write_le(&result[13], 0u, 4u); /* Legacy timer-fault slot; this service owns no timer. */
        result[17] = port.p_data_connected() ? 1u : 0u;
        write_le(&result[18], port.p_link_fault != NULL ? port.p_link_fault() : 0u, 4u);
        write_le(&result[22], p_sampler->queue_high_water, 4u);
        write_le(&result[26], p_sampler->dequeued, 4u);
        port.p_unlock();
        size = 30u;
        break;
    default:
        status = 6u;
        break;
    }
    finish(p_control, status, result, size);
}
#endif
