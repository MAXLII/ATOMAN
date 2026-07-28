// SPDX-License-Identifier: MIT
/**
 * @file    bsp_efm_flash.c
 * @brief   HC32F334 embedded flash physical driver.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Unlock and configure the HC32 embedded flash controller
 *          - Read physical flash bytes and program aligned native words
 *          - Erase aligned 4 KiB sectors using the vendor RAM-resident routine
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; calls are serialized by FAL
 *          - Vendor EFM mutation routines execute from RAM through __EFM_FUNC
 *
 * @author  Max.Li
 * @date    2026-07-27
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_efm_flash.h"

#include "hc32_ll.h"

#include <stddef.h>
#include <string.h>

static uint8_t s_efm_error;

static uint8_t bsp_efm_flash_range_is_valid(uint32_t address, uint32_t length)
{
    return (uint8_t)((address <= BSP_EFM_FLASH_CAPACITY_BYTES) &&
                     (length <= (BSP_EFM_FLASH_CAPACITY_BYTES - address)));
}

bsp_efm_flash_result_t bsp_efm_flash_init(void)
{
    s_efm_error = 0U;
    EFM_REG_Unlock();
    EFM_FWMC_Cmd(ENABLE);
    EFM_ClearStatus(EFM_FLAG_ALL);
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}

bsp_efm_flash_result_t bsp_efm_flash_write_range_enable(uint32_t address, uint32_t length)
{
    uint32_t start_sector;
    uint32_t sector_count;

    if ((0UL == length) ||
        (0UL != (address % BSP_EFM_FLASH_ERASE_SIZE)) ||
        (0UL != (length % BSP_EFM_FLASH_ERASE_SIZE)) ||
        (0U == bsp_efm_flash_range_is_valid(address, length)))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }

    start_sector = address / BSP_EFM_FLASH_ERASE_SIZE;
    sector_count = length / BSP_EFM_FLASH_ERASE_SIZE;
    EFM_SequenceSectorOperateCmd(start_sector, (uint16_t)sector_count, ENABLE);
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}

bsp_efm_flash_state_t bsp_efm_flash_state_get(void)
{
    return (0U == s_efm_error) ? BSP_EFM_FLASH_STATE_READY : BSP_EFM_FLASH_STATE_ERROR;
}

bsp_efm_flash_result_t bsp_efm_flash_read(uint32_t address, uint32_t length, uint8_t *p_data)
{
    if ((0UL != length) && (NULL == p_data))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_efm_flash_range_is_valid(address, length))
    {
        return BSP_EFM_FLASH_RESULT_OUT_OF_RANGE;
    }
    if (0UL != length)
    {
        (void)memcpy(p_data, (const void *)(uintptr_t)address, length);
    }
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}

bsp_efm_flash_result_t bsp_efm_flash_program(uint32_t address,
                                             uint32_t length,
                                             const uint8_t *p_data)
{
    uint8_t program_data[BSP_EFM_FLASH_PROGRAM_SIZE] = {0U};
    uint32_t program_address = 0UL;
    uint32_t source_offset = 0UL;
    uint32_t byte_offset = 0UL;
    uint32_t copy_length = 0UL;
    uint32_t primask = 0UL;
    int32_t result = LL_OK;

    if ((NULL == p_data) || (0UL == length))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_efm_flash_range_is_valid(address, length))
    {
        return BSP_EFM_FLASH_RESULT_OUT_OF_RANGE;
    }

    while (source_offset < length)
    {
        program_address = (address + source_offset) & ~(BSP_EFM_FLASH_PROGRAM_SIZE - 1UL);
        byte_offset = (address + source_offset) - program_address;
        copy_length = BSP_EFM_FLASH_PROGRAM_SIZE - byte_offset;
        if (copy_length > (length - source_offset))
        {
            copy_length = length - source_offset;
        }

        (void)memcpy(program_data,
                     (const void *)(uintptr_t)program_address,
                     BSP_EFM_FLASH_PROGRAM_SIZE);
        (void)memcpy(&program_data[byte_offset], &p_data[source_offset], copy_length);

        primask = __get_PRIMASK();
        __disable_irq();
        result = EFM_ProgramReadBack(program_address,
                                     program_data,
                                     BSP_EFM_FLASH_PROGRAM_SIZE);
        __set_PRIMASK(primask);
        if (LL_OK != result)
        {
            s_efm_error = 1U;
            return BSP_EFM_FLASH_RESULT_IO_ERROR;
        }
        source_offset += copy_length;
    }
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}

bsp_efm_flash_result_t bsp_efm_flash_erase(uint32_t address, uint32_t length)
{
    int32_t result;
    uint32_t primask;

    if ((BSP_EFM_FLASH_ERASE_SIZE != length) ||
        (0UL != (address % BSP_EFM_FLASH_ERASE_SIZE)))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_efm_flash_range_is_valid(address, length))
    {
        return BSP_EFM_FLASH_RESULT_OUT_OF_RANGE;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    result = EFM_SectorErase(address);
    __set_PRIMASK(primask);
    if (LL_OK != result)
    {
        s_efm_error = 1U;
        return BSP_EFM_FLASH_RESULT_IO_ERROR;
    }
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}
