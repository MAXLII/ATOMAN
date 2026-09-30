// SPDX-License-Identifier: MIT
/**
 * @file    fake_flash.c
 * @brief   Model tail Flash storage, asynchronous completion and injected BSP failures.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Model tail Flash storage, asynchronous completion and injected BSP failures.
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
#include "fake_flash.h"
#include <assert.h>
#include <string.h>
#define ARENA 1048576u
static uint8_t memory[2][ARENA]; /* Tail-only physical backing. */
static uint8_t present[2], busy[2]; /* Device and operation states. */
static uint32_t read_fault[2], program_fault[2]; /* Exact failing start offsets. */
static uint32_t writes, prefix_writes; /* Mutating callback counters. */
static int32_t errors[2]; /* Sticky until remount. */
static uint32_t capacity(uint8_t d) { return d == 0u ? 2097152u : 134217728u; }
static uint32_t block_size(uint8_t d) { return d == 0u ? 4096u : 131072u; }
static uint32_t base(uint8_t d) { return capacity(d) - 8u * block_size(d); }
void fake_reset(void)
{
    (void)memset(memory, 0xFF, sizeof(memory));
    for (uint8_t d = 0u; d < 2u; d++)
    { present[d] = 1u; busy[d] = 0u; errors[d] = 0; read_fault[d] = UINT32_MAX; program_fault[d] = UINT32_MAX; }
    writes = 0u; prefix_writes = 0u;
}
void fake_present(uint8_t d, uint8_t value) { present[d] = value; }
void fake_read_fault(uint8_t d, uint32_t address) { read_fault[d] = address; }
void fake_program_fault(uint8_t d, uint32_t address) { program_fault[d] = address; }
void fake_flip(uint8_t d, uint32_t address)
{ assert(address >= base(d)); memory[d][address - base(d)] ^= 1u; }
uint32_t fake_writes(void) { return writes; }
uint32_t fake_prefix_writes(void) { return prefix_writes; }
BSP_FLASH_RESULT_E bsp_flash_init(uint8_t d)
{ busy[d] = 0u; errors[d] = present[d] == 1u ? 0 : -1; return errors[d] == 0 ? BSP_FLASH_SUCCESS : BSP_FLASH_IO_ERROR; }
BSP_FLASH_STATE_E bsp_flash_state(uint8_t d)
{
    if (errors[d] != 0) { return BSP_FLASH_ERROR; }
    if (busy[d] != 0u) { busy[d]--; return BSP_FLASH_BUSY; }
    return BSP_FLASH_READY;
}
void bsp_flash_geometry_get(uint8_t d, uint32_t *cap, uint32_t *page, uint32_t *block, uint32_t *unit)
{ *cap = capacity(d); *page = d == 0u ? 256u : 2048u; *block = block_size(d); *unit = d == 0u ? 1u : 2048u; }
void bsp_flash_health_get(uint8_t d, bsp_flash_health_t *health)
{ (void)memset(health, 0, sizeof(*health)); health->id = d == 0u ? 0xC84015u : 0xC8F1801Du; health->error = errors[d]; }
BSP_FLASH_RESULT_E bsp_flash_read(uint8_t d, uint32_t address, uint32_t length, uint8_t *data)
{
    if (address == read_fault[d]) { errors[d] = -5; return BSP_FLASH_IO_ERROR; }
    assert(address >= base(d) && length <= capacity(d) - address);
    (void)memcpy(data, &memory[d][address - base(d)], length); busy[d] = 1u; return BSP_FLASH_SUCCESS;
}
BSP_FLASH_RESULT_E bsp_flash_program(uint8_t d, uint32_t address, uint32_t length, const uint8_t *data)
{
    if (address < base(d)) { prefix_writes++; return BSP_FLASH_INVALID_ARGUMENT; }
    if (address == program_fault[d]) { errors[d] = -6; return BSP_FLASH_IO_ERROR; }
    assert(length <= capacity(d) - address);
    if (d == 1u) { assert((address % 2048u) == 0u && length == 2048u); }
    for (uint32_t i = 0u; i < length; i++)
    {
        uint8_t *old = &memory[d][address - base(d) + i]; /* NOR-like one-to-zero storage. */
        assert((*old & data[i]) == data[i]); *old &= data[i];
    }
    writes++; busy[d] = 1u; return BSP_FLASH_SUCCESS;
}
BSP_FLASH_RESULT_E bsp_flash_erase(uint8_t d, uint32_t address, uint32_t length)
{
    if (address < base(d)) { prefix_writes++; return BSP_FLASH_INVALID_ARGUMENT; }
    assert(address % block_size(d) == 0u && length == block_size(d));
    assert(length <= capacity(d) - address);
    (void)memset(&memory[d][address - base(d)], 0xFF, length);
    writes++; busy[d] = 1u; return BSP_FLASH_SUCCESS;
}
