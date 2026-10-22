// SPDX-License-Identifier: MIT
/**
 * @file section_riscv_port.h
 * @brief RV32 integer context boundary for a hosted shared-stack scheduler.
 * @details The platform supplies idle/fault handling; no vendor SDK is included.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef SECTION_RISCV_PORT_H
#define SECTION_RISCV_PORT_H
#include <stdint.h>

/** @param p_frame 36-word context: status/cause/padding, then 32 RV32 integer words. */
void section_riscv_enter(uint32_t *p_frame);
/** Save the caller's full integer state and queue it at the runnable FIFO tail. */
void section_riscv_yield(void);
/** @param p_frame Complete 36-word context from the voluntary-yield assembly. */
void section_riscv_yield_commit(uint32_t *p_frame);
/** Give the SDK bounded housekeeping opportunities under continuous application load. */
void section_riscv_housekeeping(void);
/** Return to the saved host dispatcher without continuing the virtual task. */
void section_riscv_dispatch_return(void) __attribute__((noreturn));
/** Give the platform runtime an idle opportunity when no SECTION task is ready. */
void section_riscv_idle(void);
/** @param reason Shared-stack or queue invariant violation; never returns. */
void section_riscv_fault(uint32_t reason) __attribute__((noreturn));

/** @return Wrapping CPU cycle count for bounded critical-section observations. */
static inline uint32_t section_riscv_cycle_count(void)
{
    uint32_t cycles; /* Current low 32 bits of the architectural cycle counter. */
    __asm__ volatile("rdcycle %0" : "=r"(cycles));
    return cycles;
}

/** @return Former machine status, for a balanced restore. */
static inline uint32_t section_riscv_irq_save(void)
{
    uint32_t state; /* Complete former machine status. */
    __asm__ volatile("csrrci %0, mstatus, 8" : "=r"(state)::"memory");
    return state;
}

/** @param state Machine status returned by section_riscv_irq_save. */
static inline void section_riscv_irq_restore(uint32_t state)
{
    if ((state & 8u) != 0u)
        __asm__ volatile("csrsi mstatus, 8" ::: "memory");
}
#endif
