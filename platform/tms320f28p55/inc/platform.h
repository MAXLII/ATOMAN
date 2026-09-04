// SPDX-License-Identifier: MIT
/**
 * @file    platform.h
 * @brief   TMS320F28P55 SECTION platform contract.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Bind the SECTION scheduler to the F28P55 100 us system tick
 *          - Expose linker boundaries for automatic SECTION registration
 *          - Import the official C2000Ware C28x byte-compatible typedefs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The scheduler tick is written in the CPU Timer0 ISR
 *          - Hardware access is implemented by the local platform port
 *
 * @author  Max.Li
 * @date    2026-09-04
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef TMS320F28P55_SECTION_PLATFORM_H
#define TMS320F28P55_SECTION_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

/*
 * C28x <stdint.h> has no native 8-bit integer type. C2000Ware driverlib
 * officially defines uint8_t/int8_t as 16-bit C28x storage units.
 */
#include "driverlib.h"
#include "device.h"

#include "tms320f28p55_platform.h"

extern const uint16_t __section_start;
extern const uint16_t __section_end;

#define SECTION_SYS_TICK tms320f28p55_section_tick_get()
#define SECTION_SYS_TICK_UNIT_US 100u
#define SECTION_START __section_start
#define SECTION_STOP __section_end

#define SYSTEM_RESET SysCtl_resetDevice()
#define FUNC_RAM

#ifndef PLECS_LOG
#define PLECS_LOG(...)
#endif

#define SECTION_PORT_CONTEXT_SWITCH_REQUEST() ((void)0)
#define SECTION_PORT_FPU_LAZY_STACKING_DISABLE() ((void)0)
#define SECTION_PORT_FAULT_HOOK(reason) ((void)(reason))

/* COMM runs only in the cooperative foreground on this platform. */
#define __LDREXB(p_address) (*(p_address))
#define __STREXB(value, p_address) ((*(p_address) = (value)), 0u)
#define __DMB() ((void)0)

#define PLATFORM_PERF_COUNT_UNIT_US (1.0f / 150.0f)
#define PLATFORM_PERF_CNT_PER_SECTION_SYS_TICK 15000u
#define PLATFORM_PERF_COUNTER_REFRESH() ((void)0)
#define PLATFORM_COMM_LINK_ENABLE_ISO 0u
#define PLATFORM_COMM_LINK_ENABLE_CAN 0u
#define PLATFORM_COMM_LINK_ENABLE_PL 0u
#define PLATFORM_CTRL_PWM_TIMER_FREQ_HZ 0u

#define SECTION_REG_ATTR_PREFIX
#define SECTION_REG_ATTR_SUFFIX __attribute__((section("AUTO_REG_SECTION")))

#endif /* TMS320F28P55_SECTION_PLATFORM_H */
