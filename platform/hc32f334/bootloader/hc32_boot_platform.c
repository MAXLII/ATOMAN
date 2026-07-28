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

#include "bootloader_update_request.h"
#include "fal_cfg.h"
#include "hc32_ll.h"

#include <stddef.h>
#include <string.h>

#define HC32_SRAM_MAIN_START 0x1FFFC000UL
#define HC32_SRAM_MAIN_END 0x20004000UL
#define HC32_SRAMB_START 0x200F0000UL
#define HC32_SRAMB_END 0x200F1000UL

#if defined(__GNUC__) || defined(__clang__)
#define HC32_BOOT_RETAINED __attribute__((used, section(".noinit.boot_request")))
#else
#define HC32_BOOT_RETAINED
#endif

static volatile bootloader_update_request_t boot_request
    HC32_BOOT_RETAINED; /* IAP request retained at the same address in both images. */

static uint8_t boot_request_read(bootloader_upgrade_info_t *p_info)
{
    bootloader_protocol_info_request_t request_info = {0}; /* Stable copy used for integrity validation. */
    uint16_t checksum = 0u;                                /* Retained checksum sampled before validation. */

    if (p_info == NULL)
    {
        return 0u;
    }
    if (boot_request.magic != BOOTLOADER_UPDATE_REQUEST_MAGIC)
    {
        return 0u;
    }
    request_info = boot_request.info;
    checksum = boot_request.checksum;
    if ((boot_request.magic != BOOTLOADER_UPDATE_REQUEST_MAGIC) ||
        (checksum != bootloader_update_request_checksum_calculate(&request_info)))
    {
        return 0u;
    }
    p_info->module_id = request_info.module_id;
    p_info->version = request_info.version.raw;
    p_info->file_size = request_info.file_size;
    p_info->update_type = request_info.update_type;
    return 1u;
}

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
    bootloader_upgrade_info_t info = {0}; /* Retained upgrade request validated only for its boot reason. */

    (void)p_context;
    return (boot_request_read(&info) == 1u)
               ? BOOTLOADER_BOOT_REASON_IAP_REQUEST_E
               : BOOTLOADER_BOOT_REASON_POWER_ON_E;
}

static bootloader_result_t upgrade_info_get(void *p_context, bootloader_upgrade_info_t *p_info)
{
    (void)p_context;
    return (boot_request_read(p_info) == 1u)
               ? BOOTLOADER_RESULT_SUCCESS_E
               : BOOTLOADER_RESULT_IMAGE_INVALID_E;
}

static bootloader_result_t boot_reason_clear(void *p_context)
{
    (void)p_context;
    boot_request.magic = 0u;
    return BOOTLOADER_RESULT_SUCCESS_E;
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
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
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
    return BOOTLOADER_RESULT_SUCCESS_E;
}

static bootloader_result_t jump_to_iap(void *p_context)
{
    const uint32_t *vectors = (const uint32_t *)(uintptr_t)HC32F334_IAP_BASE;
    const uint32_t stack_pointer = vectors[0];
    const uint32_t reset_handler = vectors[1];
    uint32_t register_index = 0UL;
    void (*entry)(void);

    (void)p_context;
    if ((0U == stack_pointer_is_valid(stack_pointer)) ||
        (0UL == (reset_handler & 1UL)))
    {
        return BOOTLOADER_RESULT_IMAGE_INVALID_E;
    }

    __disable_irq();
    SysTick->CTRL = 0UL;
    SysTick->LOAD = 0UL;
    SysTick->VAL = 0UL;
    USART_FuncCmd(CM_USART2, USART_RX | USART_TX, DISABLE);
    SPI_Cmd(CM_SPI, DISABLE);
    DMA_MxChCmd(CM_DMA, DMA_MX_CH0, DISABLE);
    DMA_MxChCmd(CM_DMA, DMA_MX_CH4, DISABLE);
    DMA_MxChCmd(CM_DMA, DMA_MX_CH5, DISABLE);
    (void)DMA_ChCmd(CM_DMA, DMA_CH0, DISABLE);
    (void)DMA_ChCmd(CM_DMA, DMA_CH4, DISABLE);
    (void)DMA_ChCmd(CM_DMA, DMA_CH5, DISABLE);
    DMA_Cmd(CM_DMA, DISABLE);
    for (register_index = 0UL;
         register_index < (uint32_t)(sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]));
         register_index++)
    {
        NVIC->ICER[register_index] = UINT32_MAX;
        NVIC->ICPR[register_index] = UINT32_MAX;
    }
    SCB->VTOR = HC32F334_IAP_BASE;
    __DSB();
    __ISB();
    __set_MSP(stack_pointer);
    __enable_irq();
    entry = (void (*)(void))(uintptr_t)reset_handler;
    entry();
    return BOOTLOADER_RESULT_STORAGE_ERROR_E;
}

bootloader_platform_ops_t hc32_boot_platform_ops_make(void)
{
    const bootloader_platform_ops_t ops = {
        .p_context = NULL,
        .p_boot_reason_get = boot_reason_get,
        .p_upgrade_info_get = upgrade_info_get,
        .p_boot_reason_clear = boot_reason_clear,
        .p_image_header_is_valid = image_header_is_valid,
        .p_jump_to_iap = jump_to_iap,
        .p_watchdog_kick = NULL,
    };
    return ops;
}
