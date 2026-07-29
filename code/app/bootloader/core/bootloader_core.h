// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_core.h
 * @brief   Platform-independent boot and firmware installation state machine.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Decide whether to stay available for upgrade or launch a valid IAP image
 *          - Receive firmware into a direct or staging logical flash region
 *          - Install a verified staging image without exposing platform flash types
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Not ISR-safe; Section or another scheduler serializes calls
 *          - Hardware and storage access are abstracted through mounted operation tables
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

#ifndef BOOTLOADER_CORE_H
#define BOOTLOADER_CORE_H

#include <stdint.h>

#define BOOTLOADER_PACKET_DATA_SIZE 1024u /**< FRAME firmware data bytes per packet. */
#define BOOTLOADER_FOOTER_SIZE 34u        /**< Existing FRAME firmware footer bytes. */

typedef enum
{
    BOOTLOADER_RESULT_SUCCESS_E = 0,            /**< Operation completed successfully. */
    BOOTLOADER_RESULT_IN_PROGRESS_E = 1,        /**< Operation is active. */
    BOOTLOADER_RESULT_BUSY_E = 2,               /**< A previous operation is still active. */
    BOOTLOADER_RESULT_INVALID_ARGUMENT_E = -1,  /**< A caller argument is invalid. */
    BOOTLOADER_RESULT_OUT_OF_RANGE_E = -2,      /**< A request exceeds a logical region. */
    BOOTLOADER_RESULT_CONFIG_ERROR_E = -3,      /**< A required mount or mapping is invalid. */
    BOOTLOADER_RESULT_PERMISSION_DENIED_E = -4, /**< Storage permissions reject the request. */
    BOOTLOADER_RESULT_STORAGE_ERROR_E = -5,     /**< The mounted storage implementation failed. */
    BOOTLOADER_RESULT_PROTOCOL_ERROR_E = -6,    /**< Upgrade ordering or payload validation failed. */
    BOOTLOADER_RESULT_IMAGE_INVALID_E = -7,     /**< A candidate application image is invalid. */
    BOOTLOADER_RESULT_RECOVERY_REQUIRED_E = -8  /**< Device must remain in the bootloader. */
} bootloader_result_t;

typedef enum
{
    BOOTLOADER_FLASH_ZONE_IAP_E = 0, /**< Executable application image. */
    BOOTLOADER_FLASH_ZONE_STAGING_E, /**< Fully downloaded image awaiting installation. */
    BOOTLOADER_FLASH_ZONE_META_A_E,  /**< First atomic upgrade metadata copy. */
    BOOTLOADER_FLASH_ZONE_META_B_E,  /**< Second atomic upgrade metadata copy. */
    BOOTLOADER_FLASH_ZONE_LAYOUT_E,  /**< Read-only generated flash layout descriptor. */
    BOOTLOADER_FLASH_ZONE_COUNT_E    /**< Number of bootloader-visible logical regions. */
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

/** Flash virtual functions implemented by the platform adapter. */
typedef struct
{
    bootloader_result_t (*p_zone_info_get)(bootloader_flash_zone_t zone,
                                           bootloader_flash_zone_info_t *p_info); /**< Query region geometry. */
    bootloader_result_t (*p_read)(bootloader_flash_zone_t zone,
                                  uint32_t offset,
                                  uint32_t length,
                                  uint8_t *p_data); /**< Submit a logical read. */
    bootloader_result_t (*p_write)(bootloader_flash_zone_t zone,
                                   uint32_t offset,
                                   uint32_t length,
                                   const uint8_t *p_data); /**< Submit a logical program. */
    bootloader_result_t (*p_erase)(bootloader_flash_zone_t zone,
                                   uint32_t offset,
                                   uint32_t length); /**< Submit a logical erase. */
    uint8_t (*p_is_busy)(void);                      /**< Report whether Flash work is active. */
    bootloader_result_t (*p_result_get)(void);       /**< Return the latest Flash result. */
} bootloader_flash_ops_t;

typedef enum
{
    BOOTLOADER_UPGRADE_MODE_DIRECT_E = 0, /**< Download directly into the IAP region. */
    BOOTLOADER_UPGRADE_MODE_STAGED_E      /**< Verify staging before replacing the IAP region. */
} bootloader_upgrade_mode_t;

typedef enum
{
    BOOTLOADER_BOOT_REASON_POWER_ON_E = 0, /**< Normal reset with no explicit upgrade request. */
    BOOTLOADER_BOOT_REASON_IAP_REQUEST_E,  /**< IAP requested that the bootloader remain active. */
    BOOTLOADER_BOOT_REASON_RECOVERY_E      /**< Persistent installation state requires recovery. */
} bootloader_boot_reason_t;

typedef enum
{
    BOOTLOADER_STATE_UNINITIALIZED_E = 0,     /**< Required operation tables are not mounted. */
    BOOTLOADER_STATE_META_A_READ_E,           /**< Reading the first redundant metadata copy. */
    BOOTLOADER_STATE_META_A_WAIT_E,           /**< Waiting for the first metadata read. */
    BOOTLOADER_STATE_META_B_READ_E,           /**< Reading the second redundant metadata copy. */
    BOOTLOADER_STATE_META_B_WAIT_E,           /**< Waiting for the second metadata read. */
    BOOTLOADER_STATE_STARTUP_READ_E,          /**< Reading the IAP prefix for structural validation. */
    BOOTLOADER_STATE_STARTUP_WAIT_E,          /**< Waiting for the startup prefix read. */
    BOOTLOADER_STATE_WAIT_UPGRADE_E,          /**< Safe resident state accepting a new upgrade. */
    BOOTLOADER_STATE_DOWNLOAD_ERASE_E,        /**< Submitting erase for the selected download zone. */
    BOOTLOADER_STATE_DOWNLOAD_ERASE_WAIT_E,   /**< Waiting for download-zone erase completion. */
    BOOTLOADER_STATE_DOWNLOAD_READY_E,        /**< Ready to accept the next firmware packet. */
    BOOTLOADER_STATE_PACKET_WRITE_WAIT_E,     /**< Waiting for a submitted packet program. */
    BOOTLOADER_STATE_COPY_ERASE_E,            /**< Submitting erase for the IAP install region. */
    BOOTLOADER_STATE_COPY_ERASE_WAIT_E,       /**< Waiting for IAP erase completion. */
    BOOTLOADER_STATE_COPY_READ_E,             /**< Submitting the next staging read. */
    BOOTLOADER_STATE_COPY_READ_WAIT_E,        /**< Waiting for staging read completion. */
    BOOTLOADER_STATE_COPY_WRITE_E,            /**< Submitting the next IAP program. */
    BOOTLOADER_STATE_COPY_WRITE_WAIT_E,       /**< Waiting for IAP program completion. */
    BOOTLOADER_STATE_COPY_VERIFY_READ_E,      /**< Reading back the copied IAP chunk. */
    BOOTLOADER_STATE_COPY_VERIFY_READ_WAIT_E, /**< Waiting for copied-chunk readback. */
    BOOTLOADER_STATE_VERIFY_READ_E,           /**< Reading the complete target for CRC verification. */
    BOOTLOADER_STATE_VERIFY_READ_WAIT_E,      /**< Waiting for a complete-target verification chunk. */
    BOOTLOADER_STATE_FINAL_READ_E,            /**< Reading the installed IAP prefix. */
    BOOTLOADER_STATE_FINAL_READ_WAIT_E,       /**< Waiting for final prefix validation data. */
    BOOTLOADER_STATE_METADATA_ERASE_E,        /**< Erasing the alternate metadata copy. */
    BOOTLOADER_STATE_METADATA_ERASE_WAIT_E,   /**< Waiting for metadata erase completion. */
    BOOTLOADER_STATE_METADATA_WRITE_E,        /**< Programming a committed metadata record. */
    BOOTLOADER_STATE_METADATA_WRITE_WAIT_E,   /**< Waiting for metadata program completion. */
    BOOTLOADER_STATE_JUMP_PENDING_E,          /**< A verified IAP image is ready to launch. */
    BOOTLOADER_STATE_CONFIG_ERROR_E           /**< Required configuration is invalid. */
} bootloader_state_t;

typedef struct
{
    uint8_t module_id;   /**< Target module encoded in FRAME firmware packets. */
    uint32_t file_size;  /**< Firmware bytes including the existing footer. */
    uint32_t version;    /**< Packed firmware version received with 0x08. */
    uint8_t update_type; /**< Existing normal or force update policy value. */
} bootloader_upgrade_info_t;

typedef struct
{
    void *p_context;                                                /**< Platform boot and jump context. */
    bootloader_boot_reason_t (*p_boot_reason_get)(void *p_context); /**< Read the current boot reason. */
    bootloader_result_t (*p_upgrade_info_get)(
        void *p_context,
        bootloader_upgrade_info_t *p_info);                         /**< Read an optional retained IAP request. */
    bootloader_result_t (*p_boot_reason_clear)(void *p_context);    /**< Clear an accepted upgrade request. */
    bootloader_result_t (*p_image_header_is_valid)(void *p_context,
                                                   const uint8_t *p_header,
                                                   uint32_t header_length,
                                                   uint32_t image_size,
                                                   uint8_t *p_valid); /**< Validate platform image entry/stack form. */
    bootloader_result_t (*p_jump_to_iap)(void *p_context);            /**< Transfer control to the prepared IAP. */
    void (*p_watchdog_kick)(void *p_context);                         /**< Optionally maintain the watchdog while resident. */
} bootloader_platform_ops_t;

typedef struct
{
    uint8_t expected_module_id;             /**< Only firmware for this module is accepted. */
    bootloader_upgrade_mode_t default_mode; /**< Mode selected when IAP requests an upgrade. */
    uint32_t image_header_length;           /**< Prefix bytes passed to platform validation. */
    uint8_t *p_packet_buffer;               /**< Persistent receive and header scratch buffer. */
    uint32_t packet_buffer_size;            /**< Capacity of p_packet_buffer in bytes. */
    uint8_t *p_copy_buffer;                 /**< Persistent staging-to-IAP copy buffer. */
    uint32_t copy_buffer_size;              /**< Capacity of p_copy_buffer in bytes. */
    uint16_t (*p_crc16_init)(void);         /**< Initialize the existing FRAME CRC16. */
    uint16_t (*p_crc16_update)(const uint8_t *p_data,
                               uint32_t length,
                               uint16_t crc); /**< Continue the existing FRAME CRC16. */
} bootloader_config_t;

typedef struct
{
    bootloader_config_t config;                    /**< Immutable caller configuration copy. */
    bootloader_flash_ops_t flash_ops;              /**< Mounted logical flash service. */
    bootloader_platform_ops_t platform_ops;        /**< Mounted boot and jump service. */
    bootloader_upgrade_info_t upgrade_info;        /**< Active firmware session information. */
    bootloader_upgrade_info_t startup_upgrade_info; /**< Retained request preserved while metadata is evaluated. */
    bootloader_state_t state;                      /**< Current state-machine state. */
    bootloader_result_t result;                    /**< Latest externally relevant result. */
    bootloader_upgrade_mode_t mode;                /**< Active direct or staging mode. */
    bootloader_flash_zone_t download_zone;         /**< Zone receiving FRAME data. */
    uint32_t received_length;                      /**< Confirmed firmware bytes written. */
    uint32_t pending_packet_offset;                /**< Flash offset of the write awaiting completion. */
    uint32_t pending_packet_length;                /**< Packet bytes awaiting write completion. */
    uint32_t last_packet_offset;                   /**< Offset of the most recently confirmed packet. */
    uint32_t last_packet_length;                   /**< Length of the most recently confirmed packet. */
    uint32_t copy_offset;                          /**< Confirmed staging bytes installed into IAP. */
    uint32_t copy_chunk_length;                    /**< Staging chunk currently being copied. */
    uint32_t verify_offset;                        /**< IAP bytes included in target readback CRC. */
    uint16_t copy_chunk_crc;                       /**< Expected CRC16 for the current copied chunk. */
    uint16_t verify_crc;                           /**< CRC16 accumulated from IAP target readback. */
    uint16_t running_crc;                          /**< CRC16 over confirmed received firmware bytes. */
    uint16_t expected_crc;                         /**< CRC16 received with the end command. */
    uint8_t footer_buffer[BOOTLOADER_FOOTER_SIZE]; /**< Rolling final firmware footer bytes. */
    uint8_t footer_length;                         /**< Valid bytes currently held in footer_buffer. */
    uint8_t session_active;                        /**< Normalized active upgrade-session flag. */
    uint8_t jump_called;                           /**< Prevent repeated jump callback invocation. */
    bootloader_boot_reason_t boot_reason;          /**< Startup reason retained through metadata reads. */
    bootloader_state_t metadata_next_state;        /**< State entered after an atomic metadata commit. */
    bootloader_flash_zone_t metadata_source_zone;  /**< Newest valid metadata copy. */
    bootloader_flash_zone_t metadata_target_zone;  /**< Alternate copy being committed. */
    uint32_t metadata_sequence;                    /**< Newest committed metadata sequence. */
    uint8_t metadata_state;                        /**< Persistent recovery state encoding. */
    uint8_t metadata_retry_count;                  /**< Automatic staged recovery attempt count. */
    uint8_t metadata_valid;                        /**< A redundant committed record was selected. */
    uint8_t startup_upgrade_info_valid;            /**< Retained IAP request can start a session after metadata reads. */
} bootloader_t;

bootloader_result_t bootloader_flash_ops_init(bootloader_t *p_bootloader,
                                              const bootloader_flash_ops_t *p_flash_ops);
bootloader_result_t bootloader_init(bootloader_t *p_bootloader,
                                    const bootloader_config_t *p_config,
                                    const bootloader_platform_ops_t *p_platform_ops);
void bootloader_process(bootloader_t *p_bootloader);
bootloader_result_t bootloader_upgrade_begin(bootloader_t *p_bootloader,
                                             const bootloader_upgrade_info_t *p_info,
                                             bootloader_upgrade_mode_t mode);
bootloader_result_t bootloader_packet_submit(bootloader_t *p_bootloader,
                                             uint32_t offset,
                                             const uint8_t *p_data,
                                             uint32_t length);
bootloader_result_t bootloader_upgrade_end(bootloader_t *p_bootloader, uint16_t expected_crc);
uint8_t bootloader_is_download_ready(const bootloader_t *p_bootloader);
bootloader_state_t bootloader_state_get(const bootloader_t *p_bootloader);
bootloader_result_t bootloader_result_get(const bootloader_t *p_bootloader);

#endif /* BOOTLOADER_CORE_H */
