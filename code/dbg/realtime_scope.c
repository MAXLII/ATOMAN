// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope.c
 * @brief Portable linked-channel sampler and EA encoder.
 * @details Part of base. Owns two fixed banks and a bounded frame queue.
 *          Entity: node addresses belong to firmware, queue bytes to this module.
 *          Prior: adapter validates addresses and excludes concurrent calls.
 *          Time: overflow stops acquisition; index wraps modulo 256.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "realtime_scope.h"
#include <stddef.h>
#include <string.h>

uint8_t realtime_scope_type_size(realtime_scope_type_e type)
{
    static const uint8_t sizes[] = {0u, 1u, 1u, 2u, 2u, 4u, 4u, 8u, 8u, 4u, 8u, 1u};
    return ((unsigned)type < sizeof(sizes)) ? sizes[(unsigned)type] : 0u;
}

void realtime_scope_init(realtime_scope_t *p_scope)
{
    memset(p_scope, 0, sizeof(*p_scope));
    p_scope->p_active = &p_scope->banks[0];
    p_scope->p_config = &p_scope->banks[1];
}

void realtime_scope_clear(realtime_scope_t *p_scope)
{
    memset(p_scope->p_config, 0, sizeof(*p_scope->p_config));
}

bool realtime_scope_set(realtime_scope_t *p_scope,
                        uint8_t channel,
                        const volatile void *p_address,
                        realtime_scope_type_e type)
{
    realtime_scope_bank_t *p_bank  = p_scope->p_config;
    realtime_scope_node_t **p_tail = &p_bank->p_head;
    uint8_t size                   = realtime_scope_type_size(type);

    if (    (channel == 0u)
         || (channel > REALTIME_SCOPE_CHANNELS)
         || (    (p_address != NULL)
              && (size == 0u)))
        return false;
    uint32_t bit = UINT32_C(1) << (channel - 1u);
    realtime_scope_node_t *p_node = &p_bank->nodes[channel - 1u];
    p_node->p_address = p_address;
    p_node->type      = type;

    if (p_address == NULL)
        p_bank->enabled &= ~bit;
    else
        p_bank->enabled |= bit;
    p_bank->payload_size = 0u;

    for (uint32_t i = 0u; i < REALTIME_SCOPE_CHANNELS; ++i)
    {
        if ((p_bank->enabled & (UINT32_C(1) << i)) != 0u)
        {
            *p_tail              = &p_bank->nodes[i];
            p_tail               = &p_bank->nodes[i].p_next;
            p_bank->payload_size = (uint16_t)(p_bank->payload_size + realtime_scope_type_size(p_bank->nodes[i].type));
        }
    }
    *p_tail = NULL;
    return true;
}

bool realtime_scope_swap(realtime_scope_t *p_scope)
{
    if (    p_scope->running
         || (p_scope->count != 0u)
         || (p_scope->p_config->enabled == 0u))
        return false;
    realtime_scope_bank_t *p_old = p_scope->p_active;
    p_scope->p_active            = p_scope->p_config;
    p_scope->p_config            = p_old;
    return true;
}

bool realtime_scope_start(realtime_scope_t *p_scope)
{
    if (    p_scope->running
         || (p_scope->count != 0u)
         || (p_scope->p_active->enabled == 0u))
        return false;
    p_scope->index     = 0u;
    p_scope->samples   = 0u;
    p_scope->overflows = 0u;
    p_scope->running   = true;
    return true;
}

void realtime_scope_stop(realtime_scope_t *p_scope)
{
    p_scope->running = false;
}

void realtime_scope_sample(realtime_scope_t *p_scope)
{
    if (!p_scope->running)
        return;

    uint32_t frame_size = (uint32_t)p_scope->p_active->payload_size + 5u;

    if (p_scope->count >= REALTIME_SCOPE_QUEUE_BYTES / frame_size)
    {
        ++p_scope->overflows;
        p_scope->running = false;
        return;
    }
    realtime_scope_frame_t *p_frame = &p_scope->sample_frame;
    uint16_t pos                    = 2u;
    p_frame->data[0]                = 0xEAu;
    p_frame->data[1]                = p_scope->index++;

    for (const realtime_scope_node_t *p_node = p_scope->p_active->p_head; p_node != NULL; p_node = p_node->p_next)
    {
        uint8_t size = realtime_scope_type_size(p_node->type);

        for (uint8_t i = 0u; i < size; ++i)
            p_frame->data[pos++] = p_node->p_address[i];
    }
    p_frame->data[pos++] = (uint8_t)p_scope->p_active->payload_size;
    p_frame->data[pos++] = (uint8_t)(p_scope->p_active->payload_size >> 8u);
    uint8_t sum = 0u;

    for (uint16_t i = 0u; i < pos; ++i)
        sum = (uint8_t)(sum + p_frame->data[i]);
    p_frame->data[pos++] = sum;
    p_frame->size        = pos;
    uint32_t tail = (p_scope->head + p_scope->count * frame_size) % REALTIME_SCOPE_QUEUE_BYTES;
    uint32_t first = REALTIME_SCOPE_QUEUE_BYTES - tail;

    if (first > frame_size)
        first = frame_size;
    memcpy(&p_scope->queue[tail], p_frame->data, first);
    memcpy(p_scope->queue, &p_frame->data[first], frame_size - first);
    ++p_scope->count;
    ++p_scope->samples;
}

bool realtime_scope_pop(realtime_scope_t *p_scope, realtime_scope_frame_t *p_frame)
{
    if (p_scope->count == 0u)
        return false;
    uint32_t frame_size = (uint32_t)p_scope->p_active->payload_size + 5u;
    uint32_t first      = REALTIME_SCOPE_QUEUE_BYTES - p_scope->head;

    if (first > frame_size)
        first = frame_size;
    memcpy(p_frame->data, &p_scope->queue[p_scope->head], first);
    memcpy(&p_frame->data[first], p_scope->queue, frame_size - first);
    p_frame->size = (uint16_t)frame_size;
    p_scope->head = (p_scope->head + frame_size) % REALTIME_SCOPE_QUEUE_BYTES;
    --p_scope->count;
    return true;
}
