// SPDX-License-Identifier: MIT
/**
 * @file    trace_service.h
 * @brief   Execution trace service public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define Trace binary command words and payload timing constants
 *          - Expose the scheduled trace service task used by shell and binary reporting
 *          - Keep protocol reporting declarations outside the trace storage module
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

#ifndef TRACE_SERVICE_PRINTF
#define TRACE_SERVICE_PRINTF 0
#endif

#define TRACE_SERVICE_CMD_SET 0x01u
#define TRACE_SERVICE_CMD_CONTROL 0x2Cu
#define TRACE_SERVICE_CMD_RECORD_REPORT 0x2Du

#define TRACE_SERVICE_TIME_UNIT_US 100u

#if TRACE_SERVICE_PRINTF == 1
void dbg_trace_service_print_task(void);
#endif
void dbg_trace_service_binary_task(void);

#endif
