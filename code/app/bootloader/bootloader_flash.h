// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_flash.h
 * @brief   Bootloader-owned logical flash service contract.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define only the logical storage regions required by the bootloader
 *          - Decouple bootloader state machines from FAL and platform flash types
 *          - Provide an asynchronous mount point for storage implementations
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; service calls are serialized by the bootloader task
 *          - Hardware access is delegated through bootloader_flash_ops_t
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

#ifndef BOOTLOADER_FLASH_H
#define BOOTLOADER_FLASH_H

#include <stdint.h>

typedef enum
{
    BOOTLOADER_RESULT_SUCCESS = 0,       /**< Operation completed successfully. */
    BOOTLOADER_RESULT_IN_PROGRESS = 1,   /**< Operation is active. */
    BOOTLOADER_RESULT_BUSY = 2,          /**< A previous operation is still active. */
    BOOTLOADER_RESULT_INVALID_ARGUMENT = -1, /**< A caller argument is invalid. */
    BOOTLOADER_RESULT_OUT_OF_RANGE = -2, /**< A request exceeds a logical region. */
    BOOTLOADER_RESULT_CONFIG_ERROR = -3, /**< A required mount or mapping is invalid. */
    BOOTLOADER_RESULT_PERMISSION_DENIED = -4, /**< Storage permissions reject the request. */
    BOOTLOADER_RESULT_STORAGE_ERROR = -5, /**< The mounted storage implementation failed. */
    BOOTLOADER_RESULT_PROTOCOL_ERROR = -6, /**< Upgrade ordering or payload validation failed. */
    BOOTLOADER_RESULT_IMAGE_INVALID = -7, /**< A candidate application image is invalid. */
    BOOTLOADER_RESULT_RECOVERY_REQUIRED = -8 /**< Device must remain in the bootloader. */
} bootloader_result_t;

typedef enum
{
    BOOTLOADER_FLASH_ZONE_IAP = 0, /**< Executable application image. */
    BOOTLOADER_FLASH_ZONE_STAGING, /**< Fully downloaded image awaiting installation. */
    BOOTLOADER_FLASH_ZONE_META_A,  /**< First atomic upgrade metadata copy. */
    BOOTLOADER_FLASH_ZONE_META_B,  /**< Second atomic upgrade metadata copy. */
    BOOTLOADER_FLASH_ZONE_LAYOUT,  /**< Read-only generated flash layout descriptor. */
    BOOTLOADER_FLASH_ZONE_COUNT    /**< Number of bootloader-visible logical regions. */
} bootloader_flash_zone_t;

typedef struct
{
    uint32_t size;              /**< Logical region capacity in bytes. */
    uint32_t program_page_size; /**< Underlying device page size in bytes. */
    uint32_t erase_block_size;  /**< Underlying device erase size in bytes. */
    uint8_t readable;           /**< Normalized read permission. */
    uint8_t writable;           /**< Normalized program permission. */
    uint8_t erasable;           /**< Normalized erase permission. */
} bootloader_flash_zone_info_t;

typedef struct
{
    void *p_context; /**< Mounted storage implementation context. */
    bootloader_result_t (*p_init)(void *p_context); /**< Initialize and validate storage. */
    void (*p_process)(void *p_context); /**< Advance one bounded storage step. */
    bootloader_result_t (*p_zone_info_get)(void *p_context,
                                           bootloader_flash_zone_t zone,
                                           bootloader_flash_zone_info_t *p_info); /**< Query region geometry. */
    bootloader_result_t (*p_read)(void *p_context,
                                  bootloader_flash_zone_t zone,
                                  uint32_t offset,
                                  uint32_t length,
                                  uint8_t *p_data); /**< Submit a logical read. */
    bootloader_result_t (*p_write)(void *p_context,
                                   bootloader_flash_zone_t zone,
                                   uint32_t offset,
                                   uint32_t length,
                                   const uint8_t *p_data); /**< Submit a logical program. */
    bootloader_result_t (*p_erase)(void *p_context,
                                   bootloader_flash_zone_t zone,
                                   uint32_t offset,
                                   uint32_t length); /**< Submit a logical erase. */
    uint8_t (*p_is_busy)(void *p_context); /**< Report whether storage work is active. */
    bootloader_result_t (*p_result_get)(void *p_context); /**< Return the latest storage result. */
} bootloader_flash_ops_t;

#endif /* BOOTLOADER_FLASH_H */
