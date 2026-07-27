// SPDX-License-Identifier: MIT
/**
 * @file    bootloader_section_service.h
 * @brief   Section scheduler and communication binding for Bootloader Core.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Mount one bootloader and protocol instance into Section callbacks
 *          - Register FRAME 0x08, 0x09, 0x0A, and 0x0B direct command handlers
 *          - Advance the bounded bootloader state machine from a periodic task
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Communication callbacks only parse, copy, submit, and acknowledge events
 *          - Platform storage and jump access remain outside this service
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

#ifndef BOOTLOADER_SECTION_SERVICE_H
#define BOOTLOADER_SECTION_SERVICE_H

#include "bootloader_protocol.h"

bootloader_result_t bootloader_section_service_mount(bootloader_t *p_bootloader,
                                                       bootloader_protocol_t *p_protocol);

#endif /* BOOTLOADER_SECTION_SERVICE_H */
