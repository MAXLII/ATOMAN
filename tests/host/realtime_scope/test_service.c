// SPDX-License-Identifier: MIT
/**
 * @file test_service.c
 * @brief Exercise production scope COMM, SECTION task and interrupt registrations.
 * @details Part of base. Entity: real service/core with deterministic BSP callbacks.
 *          Prior: only transport, identity and address validation are substituted.
 *          Time: tests explicitly dispatch registered task/interrupt records.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "realtime_scope_service.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

extern section_com_t section_com_0x01_0x3A;
extern reg_task_t reg_task_manage;
extern reg_interrupt_t reg_interrupt_sample_interrupt;
static uint32_t observed = 123u, locks, sent, generation = 1u;
static bool connected = true;
static section_packform_t reply;
static uint8_t reply_data[REALTIME_SCOPE_CONTROL_BYTES];
static void enter(void)
{
    assert(locks == 0u);
    ++locks;
}
static void leave(void)
{
    assert(locks == 1u);
    --locks;
}
static void stop(void)
{
    connected = false;
    ++generation;
}
static bool ready(void)
{
    return connected;
}
static uint32_t session(void)
{
    return generation;
}
static void identity(uint8_t hash[32])
{
    memset(hash, 0x5A, 32u);
}
static const volatile void *resolve(uint32_t address, uint8_t size)
{
    return address == 16u
                && size == 4u
             ? &observed
             : NULL;
}
void comm_send_data(void *p_pack, DEC_MY_PRINTF)
{
    (void)my_printf;
    reply = *(const section_packform_t *)p_pack;
    assert(reply.len <= sizeof(reply_data));
    memcpy(reply_data, reply.p_data, reply.len);
    assert(    reply.is_ack == 1u
            && reply.cmd_set == 1u
            && reply.cmd_word == 0x3Au);
    ++sent;
}
static void enqueue(uint8_t command, const uint8_t *p_data, uint16_t size)
{
    uint8_t payload[REALTIME_SCOPE_CONTROL_BYTES] = {command, 0x34u, 0x12u};
    assert((uint32_t)size + 3u <= sizeof(payload));

    if (size != 0u)
        memcpy(&payload[3], p_data, size);
    section_packform_t packet = {.sop      = 0xE9u,
                                 .version  = 1u,
                                 .src      = 1u,
                                 .dst      = 2u,
                                 .cmd_set  = 1u,
                                 .cmd_word = 0x3Au,
                                 .len      = (uint16_t)(size + 3u),
                                 .p_data   = payload,
                                 .seq      = 5u};
    section_com_0x01_0x3A.func(&packet, NULL);
    memset(payload, 0, sizeof(payload));
}
static uint8_t request(uint8_t command, const uint8_t *p_data, uint16_t size)
{
    uint32_t previous = sent;
    enqueue(command, p_data, size);
    assert(sent == previous);
    reg_task_manage.p_func();
    assert(sent == previous + 1u);
    assert(    reply.sop == 0xE9u
            && reply.seq == 5u
            && reply.src == 2u
            && reply.dst == 1u);
    assert(    reply_data[0] == command
            && reply_data[1] == 0x34u
            && reply_data[2] == 0x12u
            && locks == 0u);
    return reply_data[3];
}
static uint32_t read_le(const uint8_t *p_data)
{
    return (uint32_t)p_data[0] | (uint32_t)p_data[1] << 8u | (uint32_t)p_data[2] << 16u | (uint32_t)p_data[3] << 24u;
}
int main(void)
{
    assert(    reg_task_manage.t_period == 1u
            && reg_interrupt_sample_interrupt.priority == 255u);
    reg_interrupt_sample_interrupt.p_func();
    enqueue(1u, NULL, 0u);
    assert(    sent == 1u
            && reply_data[3] == 7u);
    assert(!realtime_scope_service_bind(NULL));
    const realtime_scope_port_t binding = {.byte_budget      = 1000000u,
                                           .p_lock           = enter,
                                           .p_unlock         = leave,
                                           .p_stop           = stop,
                                           .p_data_connected = ready,
                                           .p_data_session   = session,
                                           .p_identity       = identity,
                                           .p_resolve        = resolve};
    assert(realtime_scope_service_bind(&binding));
    assert(!realtime_scope_service_bind(&binding));
    assert(    request(1u, NULL, 0u) == 0u
            && read_le(&reply_data[6]) == 0u);
    assert(request(6u, NULL, 0u) == 5u);
    assert(realtime_scope_service_source_set(30000u));
    assert(    request(1u, NULL, 0u) == 0u
            && reply.len == 50u
            && reply_data[14] == 0x5Au
            && read_le(&reply_data[46]) == 30000u);
    const uint8_t channel[] = {32u, RS_U32, 16u, 0u, 0u, 0u};
    assert(request(4u, channel, 5u) == 1u);
    assert(request(5u, NULL, 0u) == 3u);
    assert(request(4u, channel, sizeof(channel)) == 0u);
    assert(request(5u, NULL, 0u) == 0u);
    assert(request(6u, NULL, 0u) == 0u);
    assert(!realtime_scope_service_source_set(100u));
    assert(request(3u, NULL, 0u) == 0u);
    assert(request(5u, NULL, 0u) == 3u);
    reg_interrupt_sample_interrupt.p_func();
    assert(    request(7u, NULL, 0u) == 0u
            && reply_data[4] == 1u
            && read_le(&reply_data[5]) == 1u);
    uint8_t packet[18];
    assert(realtime_scope_service_read(packet, 8u) == 0u);
    assert(realtime_scope_service_read(packet, sizeof(packet)) == 9u);
    assert(    packet[0] == 0xEAu
            && packet[1] == 0u
            && packet[2] == 123u
            && packet[6] == 4u);
    uint8_t sum = 0u;

    for (uint32_t i = 0u; i < 8u; ++i)
        sum = (uint8_t)(sum + packet[i]);
    assert(    packet[8] == sum
            && locks == 0u);
    reg_interrupt_sample_interrupt.p_func();
    ++generation;
    assert(realtime_scope_service_read(packet, sizeof(packet)) == 0u);
    reg_task_manage.p_func();
    assert(    request(7u, NULL, 0u) == 0u
            && reply_data[4] == 0u
            && read_le(&reply_data[13]) == 0u);
    assert(request(6u, NULL, 0u) == 0u);
    reg_interrupt_sample_interrupt.p_func();
    assert(request(2u, NULL, 0u) == 0u);
    reg_interrupt_sample_interrupt.p_func();
    assert(    request(7u, NULL, 0u) == 0u
            && read_le(&reply_data[5]) == 1u
            && read_le(&reply_data[13]) == 0u);
    assert(request(6u, NULL, 0u) == 3u);
    connected = true;
    ++generation;
    reg_task_manage.p_func();
    const uint8_t wrong_frequency[] = {0xFFu, 0xFFu, 0u, 0u};
    assert(request(6u, wrong_frequency, sizeof(wrong_frequency)) == 4u);
    assert(request(3u, NULL, 0u) == 0u);

    for (uint8_t slot = 1u; slot <= REALTIME_SCOPE_CHANNELS; ++slot)
    {
        uint8_t wide[] = {slot, RS_U32, 16u, 0u, 0u, 0u};
        assert(request(4u, wide, sizeof(wide)) == 0u);
    }
    assert(request(5u, NULL, 0u) == 0u);
    assert(request(6u, NULL, 0u) == 4u);
    uint8_t invalid[] = {0u, RS_U32, 16u, 0u, 0u, 0u};
    assert(request(4u, invalid, sizeof(invalid)) == 2u);
    invalid[0] = 33u;
    assert(request(4u, invalid, sizeof(invalid)) == 2u);
    invalid[0] = 1u;
    invalid[2] = 17u;
    assert(request(4u, invalid, sizeof(invalid)) == 2u);
    assert(request(99u, NULL, 0u) == 6u);
    uint32_t previous = sent;

    for (uint32_t i = 0u; i < REALTIME_SCOPE_REQUEST_DEPTH; ++i)
        enqueue(1u, NULL, 0u);
    assert(sent == previous);
    enqueue(1u, NULL, 0u);
    assert(    sent == previous + 1u
            && reply_data[3] == 7u);
    reg_task_manage.p_func();
    assert(    sent == previous + 5u
            && reply_data[3] == 0u);
    puts(
        "PASS real REG_COMM/REG_TASK/REG_INTERRUPT records, copied queue/busy ACK, source rate, banks, EA packets, bandwidth and session loss");
    return 0;
}
