// SPDX-License-Identifier: MIT
/**
 * @file    trace_service.h
 * @brief   Execution trace service public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Expose the scheduled trace service print task
 *          - Document the service boundary between trace storage and shell output
 *          - Provide a place for future trace communication reporting APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __TRACE_SERVICE_H__
#define __TRACE_SERVICE_H__

void dbg_trace_service_print_task(void);

#endif
