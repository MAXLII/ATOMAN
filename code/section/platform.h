// SPDX-License-Identifier: MIT
/**
 * @file    platform.h
 * @brief   Section platform adaptation layer.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Select platform-specific tick, reset, linker-section, and RAM-function symbols
 *          - Map PLECS, GD32, HC32, and fallback builds onto the section runtime contract
 *          - Provide compile-time abstraction macros without exposing BSP calls to application code
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
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef IS_PLECS
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us
#define SECTION_SYS_TICK_UNIT_US 100u
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#define SYSTEM_RESET
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#define FUNC_RAM

#elif defined(IS_GD32)
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))

#elif defined(IS_HC32)
#include "systick.h"
#include "hc32f3xx.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#if defined(TOOLCHAIN_KEIL)
extern uint32_t Load$$SECTION$$Base;
extern uint32_t Load$$SECTION$$Limit;
#define SECTION_START Load$$SECTION$$Base
#define SECTION_STOP Load$$SECTION$$Limit
#else
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#endif
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))

#else
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#define FUNC_RAM __attribute__((section(".func_ram")))
#endif
