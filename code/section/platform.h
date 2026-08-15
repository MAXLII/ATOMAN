// SPDX-License-Identifier: MIT
/**
 * @file    platform.h
 * @brief   Section platform adaptation layer.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Select platform-specific tick, reset, linker-section, and RAM-function symbols
 *          - Map MATLAB, PLECS, MCU projects, and fallback builds onto the section runtime contract
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

/* -------------------------------------------------------------------------- */
/* Toolchain selection                                                        */
/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
/* Runtime platform contract                                                  */
/*                                                                            */
/* Porting boundary: each platform block should provide the complete section   */
/* runtime contract used by section.c/section.h. When creating a new MCU       */
/* project, copy the closest MCU block and adjust the symbols listed below.    */
/*                                                                            */
/* Required contract per platform block:                                      */
/* - SECTION_SYS_TICK: monotonic scheduler tick source                         */
/* - SECTION_SYS_TICK_UNIT_US: tick unit in microseconds                       */
/* - SECTION_START / SECTION_STOP: REG_TASK linker section boundaries          */
/* - SYSTEM_RESET: platform reset expression                                  */
/* - FUNC_RAM: optional RAM-function attribute                                */
/* - SECTION_PORT_CONTEXT_SWITCH_REQUEST: Cortex-M context-switch request       */
/* - SECTION_PORT_FPU_LAZY_STACKING_DISABLE: Cortex-M FPU context policy        */
/* - SECTION_PORT_FAULT_HOOK: Cortex-M scheduler fault action                   */
/* -------------------------------------------------------------------------- */

/* Host testbench */
#ifdef IS_TESTBENCH
#define SECTION_SYS_TICK 0u
#define SECTION_SYS_TICK_UNIT_US 1u
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#define SYSTEM_RESET
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define FUNC_RAM
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() \
    do                                        \
    {                                         \
    } while (0)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                           \
    {                                            \
    } while (0)
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                                  \
    {                                   \
        (void)(reason);                 \
    } while (0)

/* Simulation: MATLAB */
#elif defined(IS_MATLAB)
#include "sim_sfunc.h"
extern uint32_t sim_time_100us;
#define SECTION_SYS_TICK sim_time_100us
#define SECTION_SYS_TICK_UNIT_US SIM_TICK_UNIT_US
#if !defined(SECTION_LINKER_SENTINELS)
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#endif
#define SYSTEM_RESET
#define PLECS_LOG(...) SIM_LOG(__VA_ARGS__)
#define FUNC_RAM
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() \
    do                     \
    {                      \
    } while (0)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
    } while (0)

/* Simulation: PLECS */
#elif defined(IS_PLECS)
#include "plecs.h"
extern uint32_t plecs_time_100us;
#define SECTION_SYS_TICK __atomic_load_n(&plecs_time_100us, __ATOMIC_RELAXED)
#define SECTION_SYS_TICK_UNIT_US 100u
#define PLATFORM_PERF_COUNTER_REFRESH() plecs_perf_counter_refresh()
#define __LDREXB(address) __atomic_load_n((address), __ATOMIC_RELAXED)
#define __STREXB(value, address) \
    ((void)(value), (__atomic_test_and_set((address), __ATOMIC_ACQUIRE) ? 1u : 0u))
#define __DMB() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#if !defined(SECTION_LINKER_SENTINELS)
extern size_t __start_section;
extern size_t __stop_section;
#define SECTION_START __start_section
#define SECTION_STOP __stop_section
#endif
#define SYSTEM_RESET
#define FUNC_RAM
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() \
    do                     \
    {                      \
    } while (0)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
    } while (0)

/* MCU: GD32G553 */
#elif defined(IS_GD32G553)
#include "systick.h"
#include "gd32g5x3.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#if defined(TOOLCHAIN_MDK)
extern uint32_t section_image_base __asm("Image$$SECTION$$Base");
extern uint32_t section_image_limit __asm("Image$$SECTION$$Limit");
#define SECTION_START section_image_base
#define SECTION_STOP section_image_limit
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
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                      \
    {                                       \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                            \
        __ISB();                            \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                      \
    {                                       \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
        __disable_irq();         \
        for (;;)                 \
        {                        \
        }                        \
    } while (0)

/* MCU: GD32E507 */
#elif defined(IS_GD32E507)
#include "systick.h"
#include "gd32e50x.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#define PLATFORM_PERF_COUNT_UNIT_US 0.5f
#define PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK 200UL
#define PLATFORM_COMM_LINK_ENABLE_ISO 0u
#define PLATFORM_COMM_LINK_ENABLE_CAN 0u
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 0UL
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET NVIC_SystemReset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                                         \
    {                                                          \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;                    \
        __DSB();                                               \
        __ISB();                                               \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()               \
    do                                                         \
    {                                                          \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk;                    \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()               \
    do                                                         \
    {                                                          \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason)                        \
    do                                                         \
    {                                                          \
        (void)(reason);                                        \
        __disable_irq();                                       \
        for (;;)                                               \
        {                                                      \
        }                                                      \
    } while (0)

/* MCU: HC32F334 */
#elif defined(IS_HC32F334)
#include "systick.h"
#include "hc32f3xx.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#define PLATFORM_PERF_COUNT_UNIT_US (8.0f / 15.0f)
#define PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK 188UL
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 120000000UL
#if defined(TOOLCHAIN_MDK)
extern uint32_t section_load_base __asm("Load$$SECTION$$Base");
extern uint32_t section_load_limit __asm("Load$$SECTION$$Limit");
#define SECTION_START section_load_base
#define SECTION_STOP section_load_limit
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
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                      \
    {                                       \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                            \
        __ISB();                            \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                      \
    {                                       \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
        __disable_irq();         \
        for (;;)                 \
        {                        \
        }                        \
    } while (0)

/* MCU: HC32F558 */
#elif defined(IS_HC32F558)
#include "systick.h"
#include "hc32f5xx.h"
#define SECTION_SYS_TICK systick_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#define PLATFORM_COMM_LINK_ENABLE_ISO 0u
#define PLATFORM_COMM_LINK_ENABLE_CAN 0u
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 120000000UL
#if defined(TOOLCHAIN_MDK)
extern uint32_t section_load_base __asm("Load$$SECTION$$Base");
extern uint32_t section_load_limit __asm("Load$$SECTION$$Limit");
#define SECTION_START section_load_base
#define SECTION_STOP section_load_limit
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
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                      \
    {                                       \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                            \
        __ISB();                            \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                      \
    {                                       \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
        __disable_irq();         \
        for (;;)                 \
        {                        \
        }                        \
    } while (0)

/* SoC: Xilinx Zynq-7020 Cortex-A9 */
#elif defined(IS_ZYNQ7020)
#include "bsp_platform.h"
#include "bsp_timer.h"
#define APP_START_ADDR 0x00100000UL
#define SECTION_SYS_TICK bsp_timer_gettime_100us()
#define SECTION_SYS_TICK_UNIT_US 100u
#define PLATFORM_PERF_COUNT_UNIT_US 0.003f
#define PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK 33333UL
#define PLATFORM_COMM_LINK_ENABLE_ISO 0u
#define PLATFORM_COMM_LINK_ENABLE_CAN 0u
#define PLATFORM_COMM_LINK_ENABLE_PL 1u
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 0UL
#define __LDREXB(address) __atomic_load_n((address), __ATOMIC_RELAXED)
#define __STREXB(value, address) \
    ((void)(value), (__atomic_test_and_set((address), __ATOMIC_ACQUIRE) ? 1u : 0u))
#define __DMB() __atomic_thread_fence(__ATOMIC_SEQ_CST)
extern uint32_t __section_start;
extern uint32_t __section_end;
#define SECTION_START __section_start
#define SECTION_STOP __section_end
#define SYSTEM_RESET bsp_platform_reset()
#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))

/* MCU: APM32F402 */
#elif defined(IS_APM32F402)
#include "apm32f402_403.h"
#include "apm32f402_403_int.h"
#define APP_START_ADDR 0x08000000UL
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
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                      \
    {                                       \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                            \
        __ISB();                            \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                      \
    {                                       \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
        __disable_irq();         \
        for (;;)                 \
        {                        \
        }                        \
    } while (0)

/* Default MCU fallback */
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
#define FUNC_RAM __attribute__((section(".func_ram"), noinline, used))
#define SECTION_PORT_CONTEXT_SWITCH_REQUEST()                  \
    do                                      \
    {                                       \
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; \
        __DSB();                            \
        __ISB();                            \
    } while (0)
#if defined(FPU) && (__FPU_PRESENT == 1U)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE()   \
    do                                      \
    {                                       \
        FPU->FPCCR &= ~FPU_FPCCR_LSPEN_Msk; \
    } while (0)
#else
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() \
    do                                    \
    {                                     \
    } while (0)
#endif
#define SECTION_PORT_FAULT_HOOK(reason) \
    do                           \
    {                            \
        (void)(reason);          \
        __disable_irq();         \
        for (;;)                 \
        {                        \
        }                        \
    } while (0)
#endif

/* -------------------------------------------------------------------------- */
/* Optional platform capabilities                                             */
/* -------------------------------------------------------------------------- */

#ifndef PLATFORM_PERF_COUNT_UNIT_US
#define PLATFORM_PERF_COUNT_UNIT_US 0.5f
#endif

#ifndef PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK
#define PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK 200UL
#endif

#ifndef PLATFORM_PERF_COUNTER_REFRESH
#define PLATFORM_PERF_COUNTER_REFRESH() ((void)0)
#endif

#ifndef PLATFORM_COMM_LINK_ENABLE_ISO
#define PLATFORM_COMM_LINK_ENABLE_ISO 1u
#endif

#ifndef PLATFORM_COMM_LINK_ENABLE_CAN
#define PLATFORM_COMM_LINK_ENABLE_CAN 1u
#endif

#ifndef PLATFORM_COMM_LINK_ENABLE_PL
#define PLATFORM_COMM_LINK_ENABLE_PL 0u
#endif

#ifndef PLATFORM_CTRL_PWM_TIMER_FREQ_HZ
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 0UL
#endif

/* -------------------------------------------------------------------------- */
/* Section registration attributes                                            */
/*                                                                            */
/* These macros place REG_TASK records into the linker-visible section range   */
/* selected by the active runtime platform block above.                        */
/* -------------------------------------------------------------------------- */
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
