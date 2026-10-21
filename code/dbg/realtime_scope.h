// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope.h
 * @brief Bounded interrupt sampling and two independently linked channel banks.
 * @details Part of base. Entity: 32 slots per bank, never heap allocated.
 *          Prior: the adapter validates readable addresses and serializes calls.
 *          Time: one ISR call is one sample; overflow latches a stopped state.
 *          Hardware timers, network ownership and critical sections belong to the adapter.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#ifndef REALTIME_SCOPE_H
#define REALTIME_SCOPE_H
#include <stdbool.h>
#include <stdint.h>

#define REALTIME_SCOPE_CHANNELS    32u
#define REALTIME_SCOPE_FRAME_MAX   261u
#define REALTIME_SCOPE_QUEUE_BYTES 65536u

/* Wire types: signed/unsigned 8,16,32,64; IEEE754 float32/float64; bool. */
typedef enum
{
    RS_I8 = 1,
    RS_U8,
    RS_I16,
    RS_U16,
    RS_I32,
    RS_U32,
    RS_I64,
    RS_U64,
    RS_F32,
    RS_F64,
    RS_BOOL
} realtime_scope_type_e;
typedef struct realtime_scope_node
{
    const volatile uint8_t *p_address;
    realtime_scope_type_e type;
    struct realtime_scope_node *p_next;
} realtime_scope_node_t;
typedef struct
{
    realtime_scope_node_t nodes[REALTIME_SCOPE_CHANNELS];
    realtime_scope_node_t *p_head;
    uint32_t enabled;
    uint16_t payload_size;
} realtime_scope_bank_t;
typedef struct
{
    uint8_t data[REALTIME_SCOPE_FRAME_MAX];
    uint16_t size;
} realtime_scope_frame_t;
typedef struct
{
    realtime_scope_bank_t banks[2];
    realtime_scope_bank_t *p_active;
    realtime_scope_bank_t *p_config;
    uint8_t queue[REALTIME_SCOPE_QUEUE_BYTES];
    realtime_scope_frame_t sample_frame;
    uint32_t head, count, samples, overflows;
    uint8_t index;
    bool running;
} realtime_scope_t;

uint8_t realtime_scope_type_size(realtime_scope_type_e type);
void realtime_scope_init(realtime_scope_t *p_scope);
void realtime_scope_clear(realtime_scope_t *p_scope);
bool realtime_scope_set(realtime_scope_t *p_scope,
                        uint8_t channel,
                        const volatile void *p_address,
                        realtime_scope_type_e type);
bool realtime_scope_swap(realtime_scope_t *p_scope);
bool realtime_scope_start(realtime_scope_t *p_scope);
void realtime_scope_stop(realtime_scope_t *p_scope);
/** ISR path: bounded 32 nodes/256 bytes, no allocation, blocking or network calls. */
void realtime_scope_sample(realtime_scope_t *p_scope);
/** Copies the oldest whole frame. Caller owns serialization with the ISR. */
bool realtime_scope_pop(realtime_scope_t *p_scope, realtime_scope_frame_t *p_frame);
#endif
