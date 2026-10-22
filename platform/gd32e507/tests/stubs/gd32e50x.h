// SPDX-License-Identifier: MIT
/** @file gd32e50x.h @brief Host-only interrupt-mask fake for BSP stream tests. */
#ifndef TEST_GD32E50X_H
#define TEST_GD32E50X_H
#include <stdint.h>
extern uint32_t test_primask;
static inline uint32_t __get_PRIMASK(void)
{
    return test_primask;
}
static inline void __disable_irq(void)
{
    test_primask = 1u;
}
static inline void __set_PRIMASK(uint32_t value)
{
    test_primask = value;
}
#endif
