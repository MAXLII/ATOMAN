// SPDX-License-Identifier: MIT
/**
 * @file    f28p55_wire.h
 * @brief   F28P55 logical wire-octet codec.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Represent each protocol octet in one C28x addressable word
 *          - Mask every logical octet to eight significant bits
 *          - Encode and decode multi-octet integers explicitly in little-endian order
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The C28x addressable word is 16 bits; only the low 8 bits are wire data
 *          - Native structures are never used as physical protocol layouts
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef F28P55_WIRE_H
#define F28P55_WIRE_H

#include "platform.h"

#include <string.h>

typedef uint8_t wire_octet_t;

#define WIRE_OCTET_MASK (0x00FFu)
#define WIRE_U16_SIZE (2u)
#define WIRE_U32_SIZE (4u)

static inline wire_octet_t wire_octet_get(uint16_t value)
{
    return (wire_octet_t)(value & WIRE_OCTET_MASK);
}

static inline uint16_t wire_u16_le_read(const wire_octet_t *p_source)
{
    return (uint16_t)((uint16_t)wire_octet_get(p_source[0]) |
                      ((uint16_t)wire_octet_get(p_source[1]) << 8u));
}

static inline uint32_t wire_u32_le_read(const wire_octet_t *p_source)
{
    return (uint32_t)wire_octet_get(p_source[0]) |
           ((uint32_t)wire_octet_get(p_source[1]) << 8u) |
           ((uint32_t)wire_octet_get(p_source[2]) << 16u) |
           ((uint32_t)wire_octet_get(p_source[3]) << 24u);
}

static inline void wire_u16_le_write(wire_octet_t *p_destination, uint16_t value)
{
    p_destination[0] = wire_octet_get(value);
    p_destination[1] = wire_octet_get((uint16_t)(value >> 8u));
}

static inline void wire_u32_le_write(wire_octet_t *p_destination, uint32_t value)
{
    p_destination[0] = wire_octet_get((uint16_t)value);
    p_destination[1] = wire_octet_get((uint16_t)(value >> 8u));
    p_destination[2] = wire_octet_get((uint16_t)(value >> 16u));
    p_destination[3] = wire_octet_get((uint16_t)(value >> 24u));
}

static inline float wire_f32_le_read(const wire_octet_t *p_source)
{
    uint32_t bits = wire_u32_le_read(p_source); /* IEEE-754 bit pattern from four wire octets. */
    float value = 0.0f; /* Decoded floating-point value. */

    (void)memcpy(&value, &bits, sizeof(value));
    return value;
}

static inline void wire_f32_le_write(wire_octet_t *p_destination, float value)
{
    uint32_t bits = 0u; /* IEEE-754 bit pattern written as four wire octets. */

    (void)memcpy(&bits, &value, sizeof(bits));
    wire_u32_le_write(p_destination, bits);
}

#endif /* F28P55_WIRE_H */
