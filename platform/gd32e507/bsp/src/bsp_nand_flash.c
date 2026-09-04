// SPDX-License-Identifier: MIT
/**
 * @file    bsp_nand_flash.c
 * @brief   Provide fixed-address EXMC NAND with BCH4, bad-block rejection and bounded operations.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Provide fixed-address EXMC NAND with BCH4, bad-block rejection and bounded operations.
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
#include "bsp_flash_chip.h"
#include "gd32e50x.h"
#include "systick.h"
#include "bch4.h"
#include "flash_integrity.h"
#include <stddef.h>
#include <string.h>
#define NAND_PAGE 2048u /* Main-area bytes. */
#define NAND_BLOCK (64u * NAND_PAGE) /* Physical erase unit. */
#define NAND_CAPACITY (1024u * NAND_BLOCK) /* Main-area capacity only. */
#define NAND_BLOCK_COUNT (NAND_CAPACITY / NAND_BLOCK) /* Physical blocks independent of project partitions. */
#define NAND_DATA (*(volatile uint8_t *)(uintptr_t)0x70000000u) /* EXMC data latch. */
#define NAND_CMD (*(volatile uint8_t *)(uintptr_t)0x70010000u) /* EXMC CLE latch. */
#define NAND_ADDR (*(volatile uint8_t *)(uintptr_t)0x70020000u) /* EXMC ALE latch. */
typedef enum { NAND_IDLE, NAND_MARK0, NAND_MARK1, NAND_READ, NAND_PROGRAM, NAND_ERASE } NAND_PHASE_E;
static NAND_PHASE_E phase = NAND_IDLE; /* Hardware transaction phase. */
static bsp_flash_health_t health = {0}; /* Boot-local health counters. */
static uint8_t page_data[NAND_PAGE + 128u]; /* Task-owned main+OOB transfer/ECC buffer. */
static uint8_t rejected[1024]; /* Known bad/quarantined physical blocks; retained on rescan. */
static uint8_t erased[NAND_BLOCK_COUNT]; /* Blocks explicitly erased since initialization. */
static uint8_t next_page[NAND_BLOCK_COUNT]; /* First legal program page after erase/program. */
static uint32_t current_address = 0u; /* Next main-area address. */
static uint32_t remaining = 0u; /* Remaining read bytes. */
static uint32_t started = 0u; /* Command start in 100 us ticks. */
static uint8_t operation = 0u; /* 1 read, 2 program, 3 erase. */
static uint8_t *p_destination = NULL; /* Caller read buffer. */
static const uint8_t *p_source = NULL; /* Caller program buffer. */

static void address_send(uint32_t row, uint32_t column, uint8_t with_column)
{
    if (with_column == 1u) { NAND_ADDR = (uint8_t)column; NAND_ADDR = (uint8_t)(column >> 8u); }
    NAND_ADDR = (uint8_t)row;
    NAND_ADDR = (uint8_t)(row >> 8u);
}
static uint8_t status_read(void)
{
    NAND_CMD = 0x70u;
    return NAND_DATA;
}
static void read_start(uint32_t row, uint32_t column)
{
    NAND_CMD = 0x00u;
    address_send(row, column, 1u);
    NAND_CMD = 0x30u;
    started = systick_gettime_100us();
}
static void quarantine(int32_t error)
{
    uint32_t block = current_address / NAND_BLOCK; /* Fixed physical block index. */
    if ((block < 1024u) && (rejected[block] == 0u))
    {
        rejected[block] = 1u;
        health.bad_blocks++;
    }
    health.error = error;
}
static BSP_FLASH_RESULT_E init(void)
{
    exmc_nand_parameter_struct config = {0}; /* Board EXMC timing. */
    exmc_nand_pccard_timing_parameter_struct timing = {0}; /* Conservative 180 MHz timings. */
    uint8_t id[5] = {0}; /* Device identification bytes. */
    uint8_t ready = 0u; /* Bounded reset completion. */
    health.error = 0;
    phase = NAND_IDLE;
    (void)memset(erased, 0, sizeof(erased));
    rcu_periph_clock_enable(RCU_EXMC);
    rcu_periph_clock_enable(RCU_GPIOD);
    rcu_periph_clock_enable(RCU_GPIOE);
    gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_14 | GPIO_PIN_15 |
              GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7);
    gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10);
    gpio_init(GPIOD, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    timing.setuptime = 2u; timing.waittime = 7u; timing.holdtime = 4u; timing.databus_hiztime = 4u;
    config.nand_bank = EXMC_BANK1_NAND;
    config.ecc_size = EXMC_ECC_SIZE_2048BYTES;
    config.atr_latency = EXMC_ALE_RE_DELAY_1_HCLK;
    config.ctr_latency = EXMC_CLE_RE_DELAY_1_HCLK;
    config.ecc_logic = DISABLE; /* Software BCH supplies 4-bit/512-byte correction. */
    config.databus_width = EXMC_NAND_DATABUS_WIDTH_8B;
    config.wait_feature = DISABLE; /* Never hold the CPU bus indefinitely on R/B#. */
    config.common_space_timing = &timing;
    config.attribute_space_timing = &timing;
    exmc_nand_init(&config);
    exmc_nand_enable(EXMC_BANK1_NAND);
    NAND_CMD = 0xFFu;
    started = systick_gettime_100us();
    for (uint32_t i = 0u; i < 100000u; i++)
    {
        if (((systick_gettime_100us() - started) >= 2u) && ((status_read() & 0x40u) != 0u))
        { ready = 1u; break; }
    }
    if (ready == 0u) { health.error = -2; return BSP_FLASH_IO_ERROR; }
    NAND_CMD = 0x90u;
    NAND_ADDR = 0u;
    for (uint32_t i = 0u; i < 5u; i++) { id[i] = NAND_DATA; }
    health.id = ((uint32_t)id[0] << 24u) | ((uint32_t)id[1] << 16u) |
                ((uint32_t)id[2] << 8u) | id[3];
    if ((health.id != 0xC8F1801Du) || (id[4] != 0x42u))
    { health.error = -1; return BSP_FLASH_IO_ERROR; }
    return BSP_FLASH_SUCCESS;
}

static void issue(void)
{
    uint32_t row = current_address / NAND_PAGE; /* Physical main-area page. */
    if (operation == 1u)
    {
        read_start(row, 0u);
        phase = NAND_READ;
    }
    else if (operation == 2u)
    {
        (void)memcpy(page_data, p_source, NAND_PAGE);
        (void)memset(&page_data[NAND_PAGE], 0xFF, 128u);
        flash_integrity_u32_put(&page_data[NAND_PAGE + 8u], 0x314C4146u);
        flash_integrity_u32_put(&page_data[NAND_PAGE + 12u], flash_integrity_crc32(page_data, NAND_PAGE));
        for (uint32_t i = 0u; i < 4u; i++) { bch4_encode(&page_data[i * 512u], &page_data[NAND_PAGE + 16u + i * 7u]); }
        NAND_CMD = 0x80u;
        address_send(row, 0u, 1u);
        for (uint32_t i = 0u; i < sizeof(page_data); i++) { NAND_DATA = page_data[i]; }
        NAND_CMD = 0x10u;
        started = systick_gettime_100us();
        phase = NAND_PROGRAM;
    }
    else
    {
        NAND_CMD = 0x60u;
        address_send(row, 0u, 0u);
        NAND_CMD = 0xD0u;
        started = systick_gettime_100us();
        phase = NAND_ERASE;
    }
}

static uint8_t page_check(void)
{
    uint8_t all_erased = 1u; /* Untouched pages have no ECC record. */
    for (uint32_t i = 0u; i < sizeof(page_data); i++)
    {
        if (page_data[i] != 0xFFu) { all_erased = 0u; break; }
    }
    if (all_erased == 1u) { return 1u; }
    if (flash_integrity_u32_get(&page_data[NAND_PAGE + 8u]) != 0x314C4146u) { return 0u; }
    for (uint32_t i = 0u; i < 4u; i++)
    {
        int32_t corrected = bch4_correct(&page_data[i * 512u], &page_data[NAND_PAGE + 16u + i * 7u]); /* ECC outcome. */
        if (corrected < 0) { return 0u; }
        if ((uint32_t)corrected <= UINT32_MAX - health.corrected_bits) { health.corrected_bits += (uint32_t)corrected; }
    }
    return (flash_integrity_crc32(page_data, NAND_PAGE) ==
            flash_integrity_u32_get(&page_data[NAND_PAGE + 12u])) ? 1u : 0u;
}

static BSP_FLASH_STATE_E state(void)
{
    uint8_t status = 0u; /* Current NAND status byte. */
    uint32_t block = current_address / NAND_BLOCK; /* Active physical block. */
    if (health.error != 0) { return BSP_FLASH_ERROR; }
    if (phase == NAND_IDLE) { return BSP_FLASH_READY; }
    if ((systick_gettime_100us() - started) < 2u) { return BSP_FLASH_BUSY; }
    status = status_read();
    if ((status & 0x40u) == 0u)
    {
        if ((systick_gettime_100us() - started) > 10000u) { health.error = -2; }
        return (health.error == 0) ? BSP_FLASH_BUSY : BSP_FLASH_ERROR;
    }
    if ((phase == NAND_MARK0) || (phase == NAND_MARK1))
    {
        uint8_t marker = 0u; /* Factory/runtime marker in OOB byte 0. */
        NAND_CMD = 0x00u; /* Return from status mode to the completed read cache. */
        marker = NAND_DATA;
        if (marker != 0xFFu) { quarantine(-4); return BSP_FLASH_ERROR; }
        if (phase == NAND_MARK0)
        {
            read_start(block * 64u + 1u, NAND_PAGE);
            phase = NAND_MARK1;
        }
        else { issue(); }
        return BSP_FLASH_BUSY;
    }
    if (phase == NAND_READ)
    {
        uint32_t offset = current_address % NAND_PAGE; /* Offset within current read page. */
        uint32_t chunk = NAND_PAGE - offset; /* Bytes available from that page. */
        NAND_CMD = 0x00u;
        for (uint32_t i = 0u; i < sizeof(page_data); i++) { page_data[i] = NAND_DATA; }
        if (page_check() == 0u) { health.error = -5; return BSP_FLASH_ERROR; }
        if (chunk > remaining) { chunk = remaining; }
        (void)memcpy(p_destination, &page_data[offset], chunk);
        p_destination += chunk;
        current_address += chunk;
        remaining -= chunk;
        if (remaining != 0u)
        {
            if (rejected[current_address / NAND_BLOCK] == 1u)
            { health.error = -4; return BSP_FLASH_ERROR; }
            read_start((current_address / NAND_BLOCK) * 64u, NAND_PAGE);
            phase = NAND_MARK0;
            return BSP_FLASH_BUSY;
        }
    }
    else
    {
        if ((status & 0x81u) != 0x80u) { quarantine(-6); return BSP_FLASH_ERROR; }
        if (phase == NAND_ERASE)
        {
            erased[block] = 1u;
            next_page[block] = 0u;
        }
        else { next_page[block] = (uint8_t)((current_address / NAND_PAGE) % 64u + 1u); }
    }
    phase = NAND_IDLE;
    return BSP_FLASH_READY;
}

static BSP_FLASH_RESULT_E begin(uint32_t address, uint32_t length, uint8_t op)
{
    uint32_t block = address / NAND_BLOCK; /* Stable physical block; no implicit remapping. */
    if ((phase != NAND_IDLE) || (health.error != 0) || (address >= NAND_CAPACITY) ||
        (length == 0u) || (length > NAND_CAPACITY - address)) { return BSP_FLASH_INVALID_ARGUMENT; }
    if (rejected[block] == 1u) { health.error = -4; return BSP_FLASH_IO_ERROR; }
    if (op != 1u)
    {
        if (op == 2u)
        {
            if (((address % NAND_PAGE) != 0u) || (length != NAND_PAGE) ||
                (erased[block] == 0u) ||
                (((address / NAND_PAGE) % 64u) < next_page[block]))
            { health.error = -7; return BSP_FLASH_IO_ERROR; }
        }
        else if (((address % NAND_BLOCK) != 0u) || (length != NAND_BLOCK)) { return BSP_FLASH_INVALID_ARGUMENT; }
    }
    current_address = address;
    remaining = length;
    operation = op;
    read_start(block * 64u, NAND_PAGE);
    phase = NAND_MARK0;
    return BSP_FLASH_SUCCESS;
}
static BSP_FLASH_RESULT_E read_data(uint32_t address, uint32_t length, uint8_t *p_data)
{
    BSP_FLASH_RESULT_E result = BSP_FLASH_INVALID_ARGUMENT; /* Submission outcome before transferring buffer ownership. */
    if ((p_data == NULL) || (length > NAND_PAGE)) { return BSP_FLASH_INVALID_ARGUMENT; }
    result = begin(address, length, 1u);
    if (result == BSP_FLASH_SUCCESS)
    {
        p_destination = p_data; /* Serialized task context binds the buffer before the next state step. */
    }
    return result;
}
static BSP_FLASH_RESULT_E program(uint32_t address, uint32_t length, const uint8_t *p_data)
{
    BSP_FLASH_RESULT_E result = BSP_FLASH_INVALID_ARGUMENT; /* Rejected submissions preserve the active source buffer. */
    if (p_data == NULL) { return BSP_FLASH_INVALID_ARGUMENT; }
    result = begin(address, length, 2u);
    if (result == BSP_FLASH_SUCCESS)
    {
        p_source = p_data; /* begin only starts marker probing; page data is consumed by a later state step. */
    }
    return result;
}
static BSP_FLASH_RESULT_E erase(uint32_t address, uint32_t length) { return begin(address, length, 3u); }
const bsp_flash_chip_t bsp_nand_chip = {
    .p_init = init, .p_state = state, .p_read = read_data,
    .p_program = program, .p_erase = erase, .p_health = &health,
};
