// SPDX-License-Identifier: MIT
/**
 * @file    hc32_boot_platform.c
 * @brief   HC32F334 boot decision and Cortex-M handoff implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Maintain a complemented boot request in retained SRAM
 *          - Validate the IAP initial MSP and Thumb reset-handler address
 *          - Quiesce the processor and jump through the IAP vector table
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Jump executes with interrupts disabled
 *          - Retained data uses value and inverse to reject random SRAM contents
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

#include "hc32_boot_platform.h"

#include "fal_cfg.h"
#include "hc32_boot_handoff.h"
#include "hc32_ll.h"

#include <stddef.h>
#include <string.h>

#define HC32_SRAM_MAIN_START    0x1FFFC000UL
#define HC32_SRAM_MAIN_END      0x20004000UL
#define HC32_SRAMB_START        0x200F0000UL
#define HC32_SRAMB_END          0x200F1000UL

static uint8_t stack_pointer_is_valid(uint32_t stack_pointer)
{
    const uint8_t main_sram = (uint8_t)((stack_pointer >= HC32_SRAM_MAIN_START) &&
                                        (stack_pointer <= HC32_SRAM_MAIN_END));
    const uint8_t ramb = (uint8_t)((stack_pointer >= HC32_SRAMB_START) &&
                                   (stack_pointer <= HC32_SRAMB_END));
    return (uint8_t)(main_sram | ramb);
}

static bootloader_boot_reason_t boot_reason_get(void *p_context)
{
    (void)p_context;
    return hc32_boot_request_get();
}

static bootloader_result_t boot_reason_clear(void *p_context)
{
    (void)p_context;
    return hc32_boot_request_clear();
}

static bootloader_result_t image_header_is_valid(void *p_context,
                                                  const uint8_t *p_header,
                                                  uint32_t header_length,
                                                  uint32_t image_size,
                                                  uint8_t *p_valid)
{
    uint32_t stack_pointer;
    uint32_t reset_handler;
    uint32_t reset_address;

    (void)p_context;
    if ((NULL == p_header) || (NULL == p_valid) || (header_length < 8UL))
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT;
    }
    (void)memcpy(&stack_pointer, &p_header[0], sizeof(stack_pointer));
    (void)memcpy(&reset_handler, &p_header[4], sizeof(reset_handler));
    reset_address = reset_handler & ~1UL;
    *p_valid = (uint8_t)((0U != stack_pointer_is_valid(stack_pointer)) &&
                         (0UL != (reset_handler & 1UL)) &&
                         (reset_address >= HC32F334_IAP_BASE) &&
                         (reset_address < HC32F334_FLASH_END) &&
                         (image_size >= 8UL) &&
                         (image_size <= HC32F334_IAP_SIZE));
    return BOOTLOADER_RESULT_SUCCESS;
}

static bootloader_result_t jump_to_iap(void *p_context)
{
    const uint32_t *vectors = (const uint32_t *)(uintptr_t)HC32F334_IAP_BASE;
    const uint32_t stack_pointer = vectors[0];
    const uint32_t reset_handler = vectors[1];
    void (*entry)(void);

    (void)p_context;
    if ((0U == stack_pointer_is_valid(stack_pointer)) ||
        (0UL == (reset_handler & 1UL)))
    {
        return BOOTLOADER_RESULT_IMAGE_INVALID;
    }

    __disable_irq();
    SysTick->CTRL = 0UL;
    SysTick->LOAD = 0UL;
    SysTick->VAL = 0UL;
    USART_FuncCmd(CM_USART2, USART_RX | USART_TX, DISABLE);
    SPI_Cmd(CM_SPI, DISABLE);
    DMA_Cmd(CM_DMA, DISABLE);
    SCB->VTOR = HC32F334_IAP_BASE;
    __DSB();
    __ISB();
    __set_MSP(stack_pointer);
    entry = (void (*)(void))(uintptr_t)reset_handler;
    entry();
    return BOOTLOADER_RESULT_STORAGE_ERROR;
}

bootloader_platform_ops_t hc32_boot_platform_ops_make(void)
{
    const bootloader_platform_ops_t ops = {
        NULL,
        boot_reason_get,
        boot_reason_clear,
        image_header_is_valid,
        jump_to_iap,
        NULL,
    };
    return ops;
}
