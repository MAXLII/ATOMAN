// SPDX-License-Identifier: MIT
/**
 * @file    bch4.c
 * @brief   Encode and correct shortened BCH(8191,8139) NAND sectors.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Encode and correct shortened BCH(8191,8139) NAND sectors.
 *          - Keep operations bounded and report invalid requests.
 *          Design notes:
 *          - C11 compatible; no dynamic memory allocation.
 *          - Task-context API; not ISR-safe.
 *          - Hardware access is confined to the platform BSP.
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bch4.h"
#include <stddef.h>
#include <string.h>
#define BCH4_BITS 4148u /* 4096 payload bits followed by 52 parity bits. */
#define BCH4_POLY 0x201Bu /* Primitive x^13+x^4+x^3+x+1. */
#define BCH4_GENERATOR UINT64_C(0x14523043AB86AB) /* Roots alpha^1 through alpha^8. */

static uint16_t multiply(uint16_t a, uint16_t b)
{
    uint32_t left = a; /* Field multiplicand. */
    uint32_t right = b; /* Remaining multiplier bits. */
    uint32_t result = 0u; /* Polynomial product modulo the primitive polynomial. */
    for (uint32_t i = 0u; i < 13u; i++)
    {
        if ((right & 1u) != 0u) { result ^= left; }
        right >>= 1u;
        left <<= 1u;
        if ((left & 0x2000u) != 0u) { left ^= BCH4_POLY; }
    }
    return (uint16_t)result;
}

static uint16_t inverse(uint16_t value)
{
    uint16_t result = 1u; /* Multiplicative identity. */
    uint16_t factor = value; /* Current squared factor. */
    uint32_t exponent = 8190u; /* Nonzero field inverse exponent. */
    for (uint32_t i = 0u; i < 13u; i++)
    {
        if ((exponent & 1u) != 0u) { result = multiply(result, factor); }
        factor = multiply(factor, factor);
        exponent >>= 1u;
    }
    return result;
}

static uint32_t bit_get(const uint8_t *p_data, const uint8_t *p_ecc, uint32_t bit)
{
    if (bit < 52u) { return ((uint32_t)p_ecc[bit / 8u] >> (bit % 8u)) & 1u; }
    bit -= 52u;
    return ((uint32_t)p_data[bit / 8u] >> (bit % 8u)) & 1u;
}

static uint8_t syndromes(const uint8_t *p_data, const uint8_t *p_ecc, uint16_t *p_s)
{
    uint16_t powers[4] = {1u, 1u, 1u, 1u}; /* Odd-syndrome running powers. */
    uint16_t any = 0u; /* OR of all syndromes. */
    (void)memset(p_s, 0, 9u * sizeof(*p_s));
    for (uint32_t bit = 0u; bit < BCH4_BITS; bit++)
    {
        for (uint32_t k = 0u; k < 4u; k++)
        {
            uint32_t power = powers[k]; /* Shift-reduced field power. */
            if (bit_get(p_data, p_ecc, bit) == 1u) { p_s[2u * k + 1u] ^= powers[k]; }
            for (uint32_t j = 0u; j < 2u * k + 1u; j++)
            {
                power <<= 1u;
                if ((power & 0x2000u) != 0u) { power ^= BCH4_POLY; }
            }
            powers[k] = (uint16_t)power;
        }
    }
    for (uint32_t k = 1u; k <= 4u; k++) { p_s[2u * k] = multiply(p_s[k], p_s[k]); }
    for (uint32_t k = 1u; k <= 8u; k++) { any |= p_s[k]; }
    return (uint8_t)(any != 0u);
}

void bch4_encode(const uint8_t *p_data, uint8_t *p_ecc)
{
    uint64_t remainder = 0u; /* Systematic polynomial-division register. */
    if ((p_data == NULL) || (p_ecc == NULL)) { return; }
    for (uint32_t bit = BCH4_BITS; bit > 0u; bit--)
    {
        uint32_t value = 0u; /* Next dividend bit. */
        if (bit > 52u)
        {
            uint32_t index = bit - 53u; /* Little-endian payload bit index. */
            value = ((uint32_t)p_data[index / 8u] >> (index % 8u)) & 1u;
        }
        remainder = (remainder << 1u) | value;
        if ((remainder & (UINT64_C(1) << 52u)) != 0u) { remainder ^= BCH4_GENERATOR; }
    }
    for (uint32_t i = 0u; i < BCH4_ECC_SIZE; i++) { p_ecc[i] = (uint8_t)(remainder >> (8u * i)); }
    p_ecc[6] |= 0xF0u; /* Unused spare bits remain erased. */
}

int32_t bch4_correct(uint8_t *p_data, uint8_t *p_ecc)
{
    uint16_t s[9] = {0}; /* Error syndromes. */
    uint16_t locator[9] = {1u}; /* Current error-locator polynomial. */
    uint16_t previous[9] = {1u}; /* Previous discrepancy polynomial. */
    uint32_t degree = 0u; /* Locator degree. */
    uint32_t shift = 1u; /* Iterations since degree update. */
    uint16_t scale = 1u; /* Previous nonzero discrepancy. */
    uint32_t found = 0u; /* Chien-search roots in the shortened codeword. */
    uint16_t terms[5] = {0}; /* Locator terms advanced at each bit. */
    if ((p_data == NULL) || (p_ecc == NULL)) { return -1; }
    if (syndromes(p_data, p_ecc, s) == 0u) { return 0; }
    for (uint32_t n = 0u; n < 8u; n++)
    {
        uint16_t discrepancy = s[n + 1u]; /* Berlekamp-Massey discrepancy. */
        for (uint32_t j = 1u; j <= degree; j++) { discrepancy ^= multiply(locator[j], s[n + 1u - j]); }
        if (discrepancy == 0u) { shift++; }
        else
        {
            uint16_t saved[9] = {0}; /* Locator before this update. */
            uint16_t ratio = multiply(discrepancy, inverse(scale)); /* Update coefficient. */
            (void)memcpy(saved, locator, sizeof(saved));
            for (uint32_t j = 0u; j + shift < 9u; j++) { locator[j + shift] ^= multiply(ratio, previous[j]); }
            if (2u * degree <= n)
            {
                degree = n + 1u - degree;
                (void)memcpy(previous, saved, sizeof(previous));
                scale = discrepancy;
                shift = 1u;
            }
            else { shift++; }
        }
    }
    if ((degree == 0u) || (degree > 4u)) { return -1; }
    (void)memcpy(terms, locator, sizeof(terms));
    for (uint32_t bit = 0u; bit < BCH4_BITS; bit++)
    {
        uint16_t value = terms[0]; /* Locator evaluated at alpha^-bit. */
        for (uint32_t j = 1u; j <= degree; j++) { value ^= terms[j]; }
        if (value == 0u)
        {
            if (bit < 52u) { p_ecc[bit / 8u] ^= (uint8_t)(1u << (bit % 8u)); }
            else
            {
                uint32_t index = bit - 52u; /* Payload bit to correct. */
                p_data[index / 8u] ^= (uint8_t)(1u << (index % 8u));
            }
            found++;
        }
        for (uint32_t j = 1u; j <= degree; j++)
        {
            for (uint32_t k = 0u; k < j; k++)
            {
                uint32_t term = terms[j]; /* Multiply by alpha^-1. */
                if ((term & 1u) != 0u) { term ^= BCH4_POLY; }
                terms[j] = (uint16_t)(term >> 1u);
            }
        }
    }
    if ((found != degree) || (syndromes(p_data, p_ecc, s) != 0u)) { return -1; }
    return (int32_t)found;
}
