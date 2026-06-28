// SPDX-License-Identifier: MIT
/**
 * @file    timing.h
 * @brief   Control and PWM timing configuration.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define project-level control-loop and PWM frequency defaults
 *          - Derive sampling periods used by control modules and BSP adapters
 *          - Provide platform-specific PWM compare-domain limits for simulation and MCU builds
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path provides compile-time constants only
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-06-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __TIMING_H
#define __TIMING_H

#include <stdint.h>

/* Topology defaults used by demo builds. Real projects may override these
 * macros from their project or compiler configuration before including this file.
 */
#if defined(IS_BUCK)
#ifndef PWM_FREQ
#define PWM_FREQ 400.0e3f
#endif
#ifndef BUCK_PWM_FREQ
#define BUCK_PWM_FREQ PWM_FREQ
#endif
#ifndef CTRL_FREQ
#define CTRL_FREQ 100.0e3f
#endif
#elif defined(IS_BOOST)
#ifndef PWM_FREQ
#define PWM_FREQ 300.0e3f
#endif
#ifndef CTRL_FREQ
#define CTRL_FREQ 100.0e3f
#endif
#else
#ifndef BUCK_PWM_FREQ
#define BUCK_PWM_FREQ 60.0e3f
#endif
#ifndef CTRL_FREQ
#define CTRL_FREQ 30.0e3f
#endif
#endif

#ifndef PFC_PWM_FREQ
#define PFC_PWM_FREQ 30.0e3f
#endif

#if defined(PWM_FREQ) && !defined(PWM_TS)
#define PWM_TS (1.0f / PWM_FREQ)
#endif

#ifndef BUCK_PWM_TS
#define BUCK_PWM_TS (1.0f / BUCK_PWM_FREQ)
#endif

#ifndef CTRL_TS
#define CTRL_TS (1.0f / CTRL_FREQ)
#endif

#if defined(PWM_FREQ) && !defined(CTRL_PWM_CMP_MAX)
#if defined(IS_HC32F334) || defined(IS_HC32F558)
#ifndef CTRL_PWM_TIMER_FREQ_HZ
#define CTRL_PWM_TIMER_FREQ_HZ (120000000UL)
#endif
#define CTRL_PWM_CMP_MAX ((int32_t)((((float)CTRL_PWM_TIMER_FREQ_HZ * 64.0f) / PWM_FREQ / 2.0f) + 0.5f))
#else
#define CTRL_PWM_CMP_MAX ((int32_t)65535)
#endif
#endif

#endif
