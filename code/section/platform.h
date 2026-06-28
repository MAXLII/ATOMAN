// SPDX-License-Identifier: MIT
/**
 * @file    platform.h
 * @brief   Section platform adaptation layer.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Select platform-specific tick, reset, linker-section, and RAM-function symbols
 *          - Map MATLAB, PLECS, GD32, HC32, APM32, and fallback builds onto the section runtime contract
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

/* Toolchain selection */
#if !defined(TOOLCHAIN_MDK) && !defined(TOOLCHAIN_GCC) && !defined(TOOLCHAIN_MSVC)
#if defined(__CC_ARM) || defined(__ARMCC_VERSION)
#define TOOLCHAIN_MDK 1
#elif defined(_MSC_VER)
#define TOOLCHAIN_MSVC 1
#elif defined(__GNUC__)
#define TOOLCHAIN_GCC 1
#else
#error "Define one section toolchain macro: TOOLCHAIN_MDK, TOOLCHAIN_GCC, or TOOLCHAIN_MSVC."
#endif
#endif

#if (defined(TOOLCHAIN_MDK) + defined(TOOLCHAIN_GCC) + defined(TOOLCHAIN_MSVC)) != 1
#error "Define exactly one section toolchain macro: TOOLCHAIN_MDK, TOOLCHAIN_GCC, or TOOLCHAIN_MSVC."
#endif

#if defined(TOOLCHAIN_MSVC) || (defined(TOOLCHAIN_GCC) && defined(_WIN32) && (defined(IS_MATLAB) || defined(IS_PLECS)))
#define SECTION_LINKER_SENTINELS 1
#endif

/* Runtime platform contract */
#ifdef IS_MATLAB
#include "sim_sfunc.h"
extern uint32_t sim_time_100us;
#define SECTION_SYS_TICK sim_time_100us
#define SECTION_SYS_TICK_UNIT_US SIM_TICK_UNIT_US
#ifndef SRTOS
#define SRTOS 0
#endif
#if !defined(SECTION_LINKER_SENTINELS)
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#endif
#define SYSTEM_RESET
#define PLECS_LOG(...) SIM_LOG(__VA_ARGS__)
#define FUNC_RAM

#elif defined(IS_PLECS)
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK plecs_time_100us
#define SECTION_SYS_TICK_UNIT_US 100u
#ifndef SRTOS
#define SRTOS 0
#endif
#if !defined(SECTION_LINKER_SENTINELS)
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#endif
#define SYSTEM_RESET
#define FUNC_RAM

#elif defined(IS_GD32)
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#ifndef SRTOS
#define SRTOS 0
#endif
#if defined(TOOLCHAIN_MDK)
extern uint32_t Image$$SECTION$$Base;
extern uint32_t Image$$SECTION$$Limit;
#define SECTION_START Image$$SECTION$$Base
#define SECTION_STOP Image$$SECTION$$Limit
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
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))

#elif defined(IS_HC32)
#include "systick.h"
#if defined(HC32F558)
#include "hc32f5xx.h"
#else
#include "hc32f3xx.h"
#endif
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#ifndef SRTOS
#if defined(HC32F334)
#define SRTOS 1
#else
#define SRTOS 0
#endif
#endif
#if defined(TOOLCHAIN_MDK)
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
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))

#elif defined(IS_APM32)
#include "apm32f402_403.h"
#include "apm32f402_403_int.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#ifndef SRTOS
#define SRTOS 0
#endif
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))

#else
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#ifndef SRTOS
#define SRTOS 0
#endif
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))
#endif

#if (SRTOS != 0) && (SRTOS != 1)
#error "SRTOS must be 0 or 1."
#endif

#if defined(IS_GD32) || defined(IS_HC32) || defined(IS_APM32) || (!defined(IS_MATLAB) && !defined(IS_PLECS))
static inline uint32_t srtos_critical_enter(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static inline void srtos_critical_exit(uint32_t primask)
{
    __set_PRIMASK(primask);
}

#define SRTOS_CRITICAL_ENTER() srtos_critical_enter()
#define SRTOS_CRITICAL_EXIT(primask) srtos_critical_exit(primask)
#else
#define SRTOS_CRITICAL_ENTER() (0u)
#define SRTOS_CRITICAL_EXIT(primask) \
    do                              \
    {                               \
        (void)(primask);            \
    } while (0)
#endif

#if (SRTOS == 1)
#if defined(IS_GD32) || defined(IS_HC32) || defined(IS_APM32) || (!defined(IS_MATLAB) && !defined(IS_PLECS))
#define SRTOS_PENDSV_SET()                 \
    do                                     \
    {                                      \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                           \
        __ISB();                           \
    } while (0)

#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SRTOS_FPU_DISABLE_LAZY_STACKING() \
    do                                    \
    {                                     \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SRTOS_FPU_DISABLE_LAZY_STACKING() \
    do                                    \
    {                                     \
    } while (0)
#endif
#else
#error "SRTOS=1 requires a hardware platform to provide Cortex-M exception interfaces."
#endif
#else
#define SRTOS_PENDSV_SET() \
    do                     \
    {                      \
    } while (0)
#define SRTOS_FPU_DISABLE_LAZY_STACKING() \
    do                                    \
    {                                     \
    } while (0)
#endif

/* Section registration attributes */
#if defined(TOOLCHAIN_MSVC)
#pragma section("section$a", read)
#pragma section("section$m", read)
#pragma section("section$z", read)
#define SECTION_SENTINEL_REG_SECTION 1
#define SECTION_REG_ATTR_PREFIX __declspec(allocate("section$m"))
#define SECTION_REG_START_ATTR_PREFIX __declspec(allocate("section$a"))
#define SECTION_REG_STOP_ATTR_PREFIX __declspec(allocate("section$z"))
#define AUTO_REG_SECTION
#elif defined(TOOLCHAIN_GCC) && defined(_WIN32) && (defined(IS_MATLAB) || defined(IS_PLECS))
#define SECTION_SENTINEL_REG_SECTION 1
#define SECTION_REG_START_ATTR_PREFIX __attribute__((used, section("section$a")))
#define SECTION_REG_STOP_ATTR_PREFIX __attribute__((used, section("section$z")))
#define AUTO_REG_SECTION __attribute__((used, section("section$m")))
#elif defined(TOOLCHAIN_GCC) && (defined(IS_MATLAB) || defined(IS_PLECS))
#define AUTO_REG_SECTION __attribute__((__section__("section")))
#elif defined(TOOLCHAIN_GCC) || defined(TOOLCHAIN_MDK)
#define AUTO_REG_SECTION __attribute__((used, __section__("section")))
#else
#error "Unsupported section toolchain macro."
#endif

#ifndef SECTION_REG_ATTR_PREFIX
#define SECTION_REG_ATTR_PREFIX
#endif

#define SECTION_REG_ATTR_SUFFIX AUTO_REG_SECTION
