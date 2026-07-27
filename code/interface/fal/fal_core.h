// SPDX-License-Identifier: MIT
/**
 * @file    fal_core.h
 * @brief   Platform-independent Flash Abstraction Layer interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Describe flash devices and logical partitions without platform headers
 *          - Expose a caller-owned asynchronous read, program, and erase state machine
 *          - Publish a mountable API table for upper-layer storage adapters
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; one execution context owns each fal_t instance
 *          - Hardware access is supplied exclusively through fal_flash_ops_t
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

#ifndef FAL_CORE_H
#define FAL_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FAL_ZONE_PERMISSION_READ  (1u << 0u) /**< Permit reads from a logical zone. */
#define FAL_ZONE_PERMISSION_WRITE (1u << 1u) /**< Permit programming a logical zone. */
#define FAL_ZONE_PERMISSION_ERASE (1u << 2u) /**< Permit erasing a logical zone. */
#define FAL_ZONE_PERMISSION_ALL   (FAL_ZONE_PERMISSION_READ | FAL_ZONE_PERMISSION_WRITE | \
                                   FAL_ZONE_PERMISSION_ERASE) /**< Permit every operation. */

typedef uint16_t fal_zone_id_t;   /**< Platform-defined logical partition identifier. */
typedef uint16_t fal_device_id_t; /**< Platform-defined physical flash identifier. */

typedef enum
{
    FAL_RESULT_SUCCESS = 0,       /**< Operation completed successfully. */
    FAL_RESULT_IN_PROGRESS = 1,   /**< Operation is accepted and still active. */
    FAL_RESULT_BUSY = 2,          /**< Another operation already owns the instance. */
    FAL_RESULT_INVALID_ARGUMENT = -1, /**< A caller argument is invalid. */
    FAL_RESULT_OUT_OF_RANGE = -2, /**< A request exceeds its logical partition. */
    FAL_RESULT_CONFIG_ERROR = -3, /**< The mounted device or zone table is invalid. */
    FAL_RESULT_PERMISSION_DENIED = -4, /**< Zone permissions reject the operation. */
    FAL_RESULT_DRIVER_ERROR = -5, /**< A platform flash operation failed. */
    FAL_RESULT_STOPPED = -6       /**< The instance has entered its stopped state. */
} fal_result_t;

typedef enum
{
    FAL_DEVICE_STATE_READY = 0, /**< The device can accept another operation. */
    FAL_DEVICE_STATE_BUSY,      /**< A previously issued operation is active. */
    FAL_DEVICE_STATE_ERROR      /**< The device reports a persistent operation error. */
} fal_device_state_t;

typedef enum
{
    FAL_STATE_UNINITIALIZED = 0, /**< No valid configuration has been mounted. */
    FAL_STATE_IDLE,              /**< No operation is active. */
    FAL_STATE_READ,              /**< The next read chunk is ready to issue. */
    FAL_STATE_WRITE,             /**< The next program chunk is ready to issue. */
    FAL_STATE_ERASE,             /**< The next erase block is ready to issue. */
    FAL_STATE_WAIT_DEVICE,       /**< A device operation is in flight. */
    FAL_STATE_STOPPED,           /**< New work is rejected until reinitialization. */
    FAL_STATE_ERROR              /**< Initialization or runtime configuration failed. */
} fal_state_t;

typedef enum
{
    FAL_OPERATION_NONE = 0, /**< No request is active. */
    FAL_OPERATION_READ,     /**< Read data from a zone. */
    FAL_OPERATION_WRITE,    /**< Program data into a zone. */
    FAL_OPERATION_ERASE     /**< Erase blocks covering a zone range. */
} fal_operation_type_t;

typedef struct
{
    void *p_context; /**< Platform driver instance passed to every callback. */
    fal_result_t (*p_init)(void *p_context); /**< Initialize the physical flash device. */
    fal_device_state_t (*p_get_state)(void *p_context); /**< Query asynchronous device state. */
    fal_result_t (*p_read)(void *p_context,
                           uint32_t address,
                           uint32_t length,
                           uint8_t *p_data); /**< Start a physical read operation. */
    fal_result_t (*p_program)(void *p_context,
                              uint32_t address,
                              uint32_t length,
                              const uint8_t *p_data); /**< Start a physical program operation. */
    fal_result_t (*p_erase)(void *p_context,
                            uint32_t address,
                            uint32_t length); /**< Start a physical erase operation. */
    fal_result_t (*p_sync)(void *p_context); /**< Optionally flush driver-side buffered work. */
} fal_flash_ops_t;

typedef struct
{
    fal_device_id_t device_id;    /**< Identifier referenced by logical zones. */
    uint32_t capacity;            /**< Addressable device capacity in bytes. */
    uint32_t program_page_size;   /**< Maximum program page size in bytes. */
    uint32_t erase_block_size;    /**< Minimum erase block size in bytes. */
    uint32_t max_read_size;       /**< Maximum read chunk; 0 permits the full request. */
    fal_flash_ops_t ops;          /**< Mounted platform flash operations. */
} fal_device_cfg_t;

typedef struct
{
    fal_zone_id_t zone_id;       /**< Identifier visible to FAL clients. */
    fal_device_id_t device_id;   /**< Physical device containing the zone. */
    uint32_t device_offset;      /**< Zone start address within the physical device. */
    uint32_t size;               /**< Zone capacity in bytes. */
    uint8_t permissions;         /**< FAL_ZONE_PERMISSION_* access mask. */
} fal_zone_cfg_t;

typedef struct
{
    const fal_device_cfg_t *p_devices; /**< Platform device configuration array. */
    uint16_t device_count;             /**< Number of device entries. */
    const fal_zone_cfg_t *p_zones;     /**< Platform logical partition array. */
    uint16_t zone_count;               /**< Number of zone entries. */
} fal_cfg_t;

typedef struct
{
    fal_zone_id_t zone_id;       /**< Queried logical partition identifier. */
    fal_device_id_t device_id;   /**< Physical device containing the partition. */
    uint32_t device_offset;      /**< Partition start within the device. */
    uint32_t size;               /**< Partition capacity in bytes. */
    uint32_t program_page_size;  /**< Device program page size in bytes. */
    uint32_t erase_block_size;   /**< Device erase block size in bytes. */
    uint8_t permissions;         /**< Effective FAL_ZONE_PERMISSION_* mask. */
} fal_zone_info_t;

typedef struct
{
    const fal_cfg_t *p_cfg;              /**< Mounted platform configuration. */
    const fal_zone_cfg_t *p_zone;        /**< Zone owned by the active request. */
    const fal_device_cfg_t *p_device;    /**< Device owned by the active request. */
    uint8_t *p_read_data;                /**< Caller read destination retained until completion. */
    const uint8_t *p_write_data;         /**< Caller write source retained until completion. */
    uint32_t physical_address;           /**< Address of the next chunk within the device. */
    uint32_t remaining;                  /**< Request bytes not yet completed. */
    uint32_t chunk_length;               /**< In-flight chunk length in bytes. */
    fal_state_t state;                   /**< Public state-machine state. */
    fal_state_t operation_state;         /**< READ, WRITE, or ERASE state to resume. */
    fal_operation_type_t operation;      /**< Active request type. */
    fal_result_t result;                 /**< Last submitted operation result. */
    uint8_t stop_requested;              /**< Deferred stop request flag. */
    uint8_t sync_issued;                 /**< Final optional sync has been issued. */
} fal_t;

typedef struct
{
    fal_result_t (*p_init)(fal_t *p_fal, const fal_cfg_t *p_cfg); /**< Mount and validate cfg. */
    void (*p_process)(fal_t *p_fal); /**< Advance one bounded state-machine step. */
    fal_result_t (*p_zone_info_get)(const fal_t *p_fal,
                                    fal_zone_id_t zone_id,
                                    fal_zone_info_t *p_info); /**< Query a logical zone. */
    fal_result_t (*p_read)(fal_t *p_fal,
                           fal_zone_id_t zone_id,
                           uint32_t offset,
                           uint32_t length,
                           uint8_t *p_data); /**< Submit an asynchronous read. */
    fal_result_t (*p_write)(fal_t *p_fal,
                            fal_zone_id_t zone_id,
                            uint32_t offset,
                            uint32_t length,
                            const uint8_t *p_data); /**< Submit an asynchronous program. */
    fal_result_t (*p_erase)(fal_t *p_fal,
                            fal_zone_id_t zone_id,
                            uint32_t offset,
                            uint32_t length); /**< Submit an asynchronous erase. */
    uint8_t (*p_is_busy)(const fal_t *p_fal); /**< Report whether work is active. */
    fal_result_t (*p_result_get)(const fal_t *p_fal); /**< Return the last operation result. */
} fal_api_t;

fal_result_t fal_init(fal_t *p_fal, const fal_cfg_t *p_cfg);
void fal_process(fal_t *p_fal);
fal_result_t fal_zone_info_get(const fal_t *p_fal, fal_zone_id_t zone_id, fal_zone_info_t *p_info);
fal_result_t fal_read(fal_t *p_fal,
                      fal_zone_id_t zone_id,
                      uint32_t offset,
                      uint32_t length,
                      uint8_t *p_data);
fal_result_t fal_write(fal_t *p_fal,
                       fal_zone_id_t zone_id,
                       uint32_t offset,
                       uint32_t length,
                       const uint8_t *p_data);
fal_result_t fal_erase(fal_t *p_fal, fal_zone_id_t zone_id, uint32_t offset, uint32_t length);
uint8_t fal_is_busy(const fal_t *p_fal);
fal_state_t fal_state_get(const fal_t *p_fal);
fal_result_t fal_result_get(const fal_t *p_fal);
fal_result_t fal_stop_request(fal_t *p_fal);
uint8_t fal_is_stopped(const fal_t *p_fal);

extern const fal_api_t g_fal_api; /**< Default mountable FAL API implementation. */

#ifdef __cplusplus
}
#endif

#endif /* FAL_CORE_H */
