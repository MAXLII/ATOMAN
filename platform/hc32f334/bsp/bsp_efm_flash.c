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
    int32_t result;

    if ((NULL == p_data) || (0UL == length) ||
        (0UL != (address % BSP_EFM_FLASH_PROGRAM_SIZE)) ||
        (0UL != (length % BSP_EFM_FLASH_PROGRAM_SIZE)))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_efm_flash_range_is_valid(address, length))
    {
        return BSP_EFM_FLASH_RESULT_OUT_OF_RANGE;
    }

    result = EFM_ProgramReadBack(address, p_data, length);
    if (LL_OK != result)
    {
        s_efm_error = 1U;
        return BSP_EFM_FLASH_RESULT_IO_ERROR;
    }
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}

bsp_efm_flash_result_t bsp_efm_flash_erase(uint32_t address, uint32_t length)
{
    int32_t result;

    if ((BSP_EFM_FLASH_ERASE_SIZE != length) ||
        (0UL != (address % BSP_EFM_FLASH_ERASE_SIZE)))
    {
        return BSP_EFM_FLASH_RESULT_INVALID_ARGUMENT;
    }
    if (0U == bsp_efm_flash_range_is_valid(address, length))
    {
        return BSP_EFM_FLASH_RESULT_OUT_OF_RANGE;
    }

    result = EFM_SectorErase(address);
    if (LL_OK != result)
    {
        s_efm_error = 1U;
        return BSP_EFM_FLASH_RESULT_IO_ERROR;
    }
    return BSP_EFM_FLASH_RESULT_SUCCESS;
}
