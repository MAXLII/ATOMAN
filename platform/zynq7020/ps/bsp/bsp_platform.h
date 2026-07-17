// SPDX-License-Identifier: MIT
/**
 * @file    bsp_platform.h
 * @brief   Zynq-7020 platform control interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the Cortex-A9 platform reset operation
 *          - Keep Zynq PS control-register access outside shared framework code
 *          - Provide the platform contract required by the section framework
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Reset execution does not return
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef BSP_PLATFORM_H
#define BSP_PLATFORM_H

void bsp_platform_reset(void);

#endif /* BSP_PLATFORM_H */
