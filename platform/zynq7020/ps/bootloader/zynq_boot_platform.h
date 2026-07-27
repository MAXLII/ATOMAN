// SPDX-License-Identifier: MIT
/**
 * @file    zynq_boot_platform.h
 * @brief   Zynq-7020 boot reason, image loading, and jump service.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Store the IAP-to-bootloader request in retained on-chip memory
 *          - Validate an A9 application vector and bounded DDR destination
 *          - Load the accepted QSPI IAP image into DDR and transfer control
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Transfer functions execute with interrupts disabled
 *          - Zynq cache and address details remain confined to this platform module
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

#ifndef ZYNQ_BOOT_PLATFORM_H
#define ZYNQ_BOOT_PLATFORM_H

#include "bootloader_core.h"

#define ZYNQ7020_IAP_DDR_ADDRESS 0x00100000u
#define ZYNQ7020_BOOTLOADER_DDR_ADDRESS 0x04000000u
#define ZYNQ7020_DMA_RESERVED_ADDRESS 0x1FF00000u

bootloader_platform_ops_t zynq_boot_platform_ops_make(bootloader_t *p_bootloader);
#endif /* ZYNQ_BOOT_PLATFORM_H */
