// SPDX-License-Identifier: MIT
/**
 * @file    zynq7020_section_config.h
 * @brief   Zynq-7020 Cortex-A9 section SRTOS platform configuration.
 * @details
 *          This file is part of the Zynq-7020 platform project.
 *
 *          Module responsibilities:
 *          - Select the dedicated Cortex-A9 section runtime
 *          - Keep section runtime policy inside the platform layer
 *          - Prevent build commands from declaring the SRTOS policy macro
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Included by code/section/platform.h
 *          - Selected through the A9 project include path
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

#ifndef ZYNQ7020_SECTION_CONFIG_H
#define ZYNQ7020_SECTION_CONFIG_H

#if defined(SRTOS)
#error "SRTOS must be selected by the Zynq-7020 platform configuration, not by build flags."
#endif

#define SRTOS 1

#endif /* ZYNQ7020_SECTION_CONFIG_H */
