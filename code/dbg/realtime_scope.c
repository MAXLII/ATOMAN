// SPDX-License-Identifier: MIT
/**
 * @file realtime_scope.c
 * @brief Portable linked-channel sampler and EA encoder.
 * @details Part of base. Owns two fixed banks and a bounded whole-frame queue.
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

bool realtime_scope_init(realtime_scope_t *p_scope, uint8_t *p_queue, uint32_t capacity)
{
    if (    p_scope == NULL
         || p_queue == NULL
         || capacity < REALTIME_SCOPE_FRAME_MAX
         || capacity > UINT32_MAX / 2u)
        return false;
    memset(p_scope, 0, sizeof(*p_scope));
    p_scope->p_queue        = p_queue;
    p_scope->queue_capacity = capacity;
    p_scope->p_active       = &p_scope->banks[0];
    p_scope->p_config       = &p_scope->banks[1];
    return true;
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
    p_scope->index            = 0u;
    p_scope->samples          = 0u;
    p_scope->overflows        = 0u;
    p_scope->queue_high_water = 0u;
    p_scope->dequeued         = 0u;
    p_scope->frame_size       = (uint32_t)p_scope->p_active->payload_size + 5u;
    p_scope->queue_frames     = p_scope->queue_capacity / p_scope->frame_size;
    p_scope->queue_limit      = p_scope->queue_frames * p_scope->frame_size;
    p_scope->head             = 0u;
    p_scope->tail             = 0u;
    p_scope->running          = true;
    return true;
}

void realtime_scope_stop(realtime_scope_t *p_scope)
{
    p_scope->running = false;
}

static inline uint32_t sample_byte(const volatile uint8_t *p_source, uint8_t *p_target)
{
    uint8_t value = *p_source;
    *p_target     = value;
    return value;
}

static inline uint32_t sample_four(const volatile uint8_t *p_source, uint8_t *p_target)
{
    uint32_t sum = sample_byte(p_source, p_target);
    sum += sample_byte(p_source + 1u, p_target + 1u);
    sum += sample_byte(p_source + 2u, p_target + 2u);
    sum += sample_byte(p_source + 3u, p_target + 3u);
    return sum;
}

void realtime_scope_sample(realtime_scope_t *p_scope)
{
    if (!p_scope->running)
        return;

    if (p_scope->count >= p_scope->queue_frames)
    {
        ++p_scope->overflows;
        p_scope->running = false;
        return;
    }
    uint8_t *p_frame      = &p_scope->p_queue[p_scope->tail];
    uint8_t *p_output     = p_frame + 2u;
    uint16_t payload_size = p_scope->p_active->payload_size;
    p_frame[0]            = 0xEAu;
    p_frame[1]            = p_scope->index++;
    uint32_t sum = UINT32_C(0xEA) + p_frame[1] + (uint8_t)payload_size + (uint8_t)(payload_size >> 8u);

    for (const realtime_scope_node_t *p_node = p_scope->p_active->p_head; p_node != NULL; p_node = p_node->p_next)
    {
        uint8_t size = realtime_scope_type_size(p_node->type);

        /* Preserve byte access and order even for packed or unaligned objects. */

        switch (size)
        {
        case 1u:
            sum += sample_byte(p_node->p_address, p_output);
            break;
        case 2u:
            sum += sample_byte(p_node->p_address, p_output);
            sum += sample_byte(p_node->p_address + 1u, p_output + 1u);
            break;
        case 4u:
            sum += sample_four(p_node->p_address, p_output);
            break;
        case 8u:
            sum += sample_four(p_node->p_address, p_output);
            sum += sample_four(p_node->p_address + 4u, p_output + 4u);
            break;
        default:
            break;
        }
        p_output += size;
    }
    p_output[0] = (uint8_t)payload_size;
    p_output[1] = (uint8_t)(payload_size >> 8u);
    p_output[2] = (uint8_t)sum;
    p_scope->tail += p_scope->frame_size;

    if (p_scope->tail == p_scope->queue_limit)
        p_scope->tail = 0u;
    /* Publish only the complete frame; caller retains the existing serialization. */
    ++p_scope->count;

    if (p_scope->count > p_scope->queue_high_water)
        p_scope->queue_high_water = p_scope->count;
    ++p_scope->samples;
}

bool realtime_scope_pop(realtime_scope_t *p_scope, realtime_scope_frame_t *p_frame)
{
    if (p_scope->count == 0u)
        return false;
    memcpy(p_frame->data, &p_scope->p_queue[p_scope->head], p_scope->frame_size);
    p_frame->size = (uint16_t)p_scope->frame_size;
    p_scope->head += p_scope->frame_size;

    if (p_scope->head == p_scope->queue_limit)
        p_scope->head = 0u;
    --p_scope->count;
    ++p_scope->dequeued;
    return true;
}
