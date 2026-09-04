// SPDX-License-Identifier: MIT
/**
 * @file    bsp_nor_flash.c
 * @brief   Drive GD25Q16 through bounded single-lane SQPI transactions.
 * @details
 *          This file is part of the base project.
 *          Module responsibilities:
 *          - Drive GD25Q16 through bounded single-lane SQPI transactions.
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
#include <stddef.h>
#define NOR_CAPACITY (2u * 1024u * 1024u) /* Physical bytes. */
#define NOR_MEM ((uintptr_t)0xB0000000u) /* SQPI command window, never instruction mapped. */
static bsp_flash_health_t health = {0}; /* Device diagnostics. */
static const uint8_t *p_source = NULL; /* Caller buffer retained until the physical operation finishes. */
static uint32_t next_address = 0u; /* Next byte to program. */
static uint32_t remaining = 0u; /* Outstanding bytes. */
static uint32_t started = 0u; /* Current command start, 100 us. */
static uint32_t timeout_ticks = 0u; /* Current command deadline. */
static uint8_t busy = 0u; /* One command is in flight. */

static void address_bits(uint32_t bits)
{
    SQPI_INIT = (SQPI_INIT & ~0x1F000000u) | (bits << 24u);
}
static uint8_t status_read(void)
{
    address_bits(0u);
    sqpi_read_command_config(SQPI_MODE_SSS, 0u, 0x05u);
    return *(volatile uint8_t *)NOR_MEM;
}
static uint8_t special(uint8_t command)
{
    address_bits(0u);
    sqpi_write_command_config(SQPI_MODE_SSS, 0u, command);
    SQPI_WCMD |= SQPI_WCMD_SCMD;
    for (uint32_t i = 0u; i < 10000u; i++)
    {
        if ((SQPI_WCMD & SQPI_WCMD_SCMD) == 0u) { return 1u; }
    }
    health.error = -2;
    return 0u;
}
static uint8_t write_enable(void)
{
    if ((status_read() & 0x1Cu) != 0u) { health.error = -3; return 0u; }
    if (special(0x06u) == 0u) { return 0u; }
    if ((status_read() & 0x03u) != 0x02u) { health.error = -6; return 0u; }
    return 1u;
}
static BSP_FLASH_RESULT_E init(void)
{
    sqpi_parameter_struct config = {0}; /* Conservative single-lane SQPI configuration. */
    health.error = 0;
    busy = 0u;
    remaining = 0u;
    rcu_periph_clock_enable(RCU_GPIOF);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_SQPI);
    gpio_init(GPIOF, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
              GPIO_PIN_0 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_8);
    gpio_init(GPIOF, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2 | GPIO_PIN_10);
    gpio_bit_set(GPIOF, GPIO_PIN_2 | GPIO_PIN_10); /* WP# and HOLD# inactive. */
    sqpi_struct_para_init(&config);
    config.addr_bit = 0u;
    config.clk_div = 20u;
    sqpi_init(&config);
    sqpi_read_command_config(SQPI_MODE_SSS, 0u, 0x9Fu);
    SQPI_RCMD |= SQPI_RCMD_RID;
    for (uint32_t i = 0u; i < 10000u; i++)
    {
        if ((SQPI_RCMD & SQPI_RCMD_RID) == 0u)
        {
            health.id = (SQPI_IDL >> 8u) & 0xFFFFFFu;
            if (health.id == 0xC84015u)
            {
                /* A CPU-only restart may leave the external Flash finishing a command. */
                if ((status_read() & 1u) != 0u) { health.error = -6; return BSP_FLASH_IO_ERROR; }
                return BSP_FLASH_SUCCESS;
            }
            health.error = -1;
            return BSP_FLASH_IO_ERROR;
        }
    }
    health.error = -2;
    return BSP_FLASH_IO_ERROR;
}
static BSP_FLASH_RESULT_E read_data(uint32_t address, uint32_t length, uint8_t *p_data)
{
    if ((p_data == NULL) || (length > 256u) || (address > NOR_CAPACITY) ||
        (length > NOR_CAPACITY - address) || (busy == 1u)) { return BSP_FLASH_INVALID_ARGUMENT; }
    address_bits(24u);
    sqpi_read_command_config(SQPI_MODE_SSS, 0u, 0x03u);
    for (uint32_t i = 0u; i < length; i++) { p_data[i] = *(volatile uint8_t *)(NOR_MEM + address + i); }
    return BSP_FLASH_SUCCESS;
}

static uint8_t byte_program(void)
{
    if (write_enable() == 0u) { return 0u; }
    address_bits(24u);
    sqpi_write_command_config(SQPI_MODE_SSS, 0u, 0x02u);
    *(volatile uint8_t *)(NOR_MEM + next_address) = *p_source;
    started = systick_gettime_100us();
    timeout_ticks = 1000u;
    return 1u;
}

static BSP_FLASH_STATE_E state(void)
{
    if (health.error != 0) { return BSP_FLASH_ERROR; }
    if (busy == 0u) { return BSP_FLASH_READY; }
    if ((status_read() & 1u) != 0u)
    {
        if ((systick_gettime_100us() - started) >= timeout_ticks) { health.error = -2; }
        return (health.error == 0) ? BSP_FLASH_BUSY : BSP_FLASH_ERROR;
    }
    if (remaining != 0u)
    {
        uint8_t value = 0u; /* Verify the byte whose command just completed. */
        address_bits(24u);
        sqpi_read_command_config(SQPI_MODE_SSS, 0u, 0x03u);
        value = *(volatile uint8_t *)(NOR_MEM + next_address);
        if (value != *p_source) { health.error = -6; return BSP_FLASH_ERROR; }
        remaining--;
        next_address++;
        p_source++;
        if (remaining != 0u)
        {
            if (byte_program() == 0u) { return BSP_FLASH_ERROR; }
            return BSP_FLASH_BUSY;
        }
    }
    busy = 0u;
    return BSP_FLASH_READY;
}

static BSP_FLASH_RESULT_E program(uint32_t address, uint32_t length, const uint8_t *p_data)
{
    if ((p_data == NULL) || (length == 0u) || (length > 256u) ||
        (address >= NOR_CAPACITY) ||
        (length > NOR_CAPACITY - address) || ((address % 256u) + length > 256u) ||
        (busy == 1u) || (health.error != 0)) { return BSP_FLASH_INVALID_ARGUMENT; }
    p_source = p_data;
    next_address = address;
    remaining = length;
    if (byte_program() == 0u) { return BSP_FLASH_IO_ERROR; }
    busy = 1u;
    return BSP_FLASH_SUCCESS;
}

static BSP_FLASH_RESULT_E erase(uint32_t address, uint32_t length)
{
    if ((address >= NOR_CAPACITY) ||
        ((address % 4096u) != 0u) || (length != 4096u) ||
        (busy == 1u) || (health.error != 0)) { return BSP_FLASH_INVALID_ARGUMENT; }
    if (write_enable() == 0u) { return BSP_FLASH_IO_ERROR; }
    /* The SQPI bus write contributes the last address octet, not page data. */
    address_bits(16u);
    sqpi_write_command_config(SQPI_MODE_SSS, 0u, 0x20u);
    *(volatile uint8_t *)(NOR_MEM + (address >> 8u)) = (uint8_t)address;
    started = systick_gettime_100us();
    timeout_ticks = 30000u;
    remaining = 0u;
    busy = 1u;
    return BSP_FLASH_SUCCESS;
}

const bsp_flash_chip_t bsp_nor_chip = {
    .p_init = init,
    .p_state = state,
    .p_read = read_data,
    .p_program = program,
    .p_erase = erase,
    .p_health = &health,
};
