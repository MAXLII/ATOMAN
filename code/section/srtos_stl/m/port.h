// SPDX-License-Identifier: MIT
/**
 * @file port.h
 * @brief GNU Cortex-M3/M4 and opt-in M33 execution boundary for SRTOS_STL.
 * @details The board explicitly binds the 2 vector entry functions below.
 *          This port owns PSP while active and retains its native caller on MSP.
 *          Define SECTION_STL_M33_NO_SECURITY=1 for Cortex-M33 without the
 *          Security Extension; CPUID and ID_PFR1 are checked at admission.
 *          This is not support for a TrustZone-capable chip with security unused.
 *          It does not support an RTOS caller on PSP, ARMv8-M security states,
 *          SMP, or calls from NMI/HardFault. Compile this translation unit with
 *          -fno-lto and all participating objects with a compatible CPU/FPU ABI.
 *          The core entry must never return. The board must reserve enough MSP
 *          for retained host state and exceptions, and protect hardware PSP
 *          exception entry with stack headroom or an MPU guard.
 */
#ifndef SECTION_STL_M_PORT_H
#define SECTION_STL_M_PORT_H

#include <stdint.h>

#ifndef SECTION_STL_M_BASEPRI_CEILING
#define SECTION_STL_M_BASEPRI_CEILING 0x80u /* Register-encoded scheduler interrupt threshold. */
#endif

/** @return Previous BASEPRI; only scheduler-level interrupts are then masked. */
uint32_t section_stl_port_lock(void);

/** @param mask BASEPRI value returned by the matching lock call. */
void section_stl_port_unlock(uint32_t mask);

/**
 * @return 1 for Thread or an admitted configurable handler, otherwise 0.
 * @details Handlers above the BASEPRI ceiling, NMI and fault handlers must not
 *          access scheduler tables. Priorities/grouping must remain fixed while
 *          the component is in use. High-priority control IRQs may preempt copy
 *          and restore on MSP but must obey the CPU/FPU interrupt ABI.
 */
uint32_t section_stl_port_api_allowed(void);

/**
 * @param p_low Inclusive low address; word aligned, or 8-byte aligned on M33.
 * @param p_top Exclusive high address, aligned to 8 bytes.
 * @param p_entry Resident task trampoline; must never return.
 * @return Initial saved frame, or NULL for an invalid stack or entry.
 */
uint32_t *section_stl_port_prepare(uint32_t *p_low, uint32_t *p_top, void (*p_entry)(void));

/**
 * @param p_sp Readable copied or live saved frame, aligned to a word.
 * @param words Number of readable words, including task locals above the frame.
 * @return 1 for a complete Thread/PSP frame, otherwise 0.
 */
uint32_t section_stl_port_frame_valid(const uint32_t *p_sp, uint32_t words);

/**
 * @brief Run until the core returns NULL, then return to the native caller.
 * @details Requires privileged Thread/MSP, PRIMASK/BASEPRI/FAULTMASK all clear,
 *          CCR.STKALIGN set, and exclusive SVC/PendSV ownership. PendSV must be
 *          the lowest implemented priority. SVC and SysTick (even when stopped)
 *          must have encoded priorities at or below the BASEPRI ceiling in
 *          urgency. The ceiling must be implemented and aligned to a nonempty
 *          preemption-priority group. Admission briefly probes implemented bits
 *          through BASEPRI_MAX and restores BASEPRI without changing NVIC state.
 *          FP builds additionally
 *          require CP10/CP11 enabled and FPCCR.ASPEN set. Pure soft-float builds
 *          require that tasks and interrupts never use floating-point state.
 *          PSP and FPCCR.LSPEN are restored after native return. M33 also
 *          restores native PSPLIM, sets the resident stack limit before each
 *          task return, and leaves native MSPLIM unchanged. M33 requires an
 *          Arm Cortex-M33 CPUID and ID_PFR1.Security equal to zero.
 *          No timeout is
 *          imposed on a task that neither completes nor permits preemption.
 * @return 1 after a valid dispatch returned, or 0 if admission was rejected.
 */
uint32_t section_stl_port_run(void);

/** @brief Pend a switch only while this port owns an active dispatch. */
void section_stl_port_request_switch(void);

/** @return 1 while PendSV is active; only periodic tick requests may be suppressed. */
uint32_t section_stl_port_switch_active(void);

/** @return 1 in Handler mode, otherwise 0. */
uint32_t section_stl_port_in_handler(void);

/** @return 1 until the original native caller has resumed, otherwise 0. */
uint32_t section_stl_port_active(void);

/**
 * @brief Check before entering the core critical section.
 * @return 1 only in the active privileged PSP thread, with no interrupt masks
 *         or pending PendSV; the core must still recheck its state and slice.
 */
uint32_t section_stl_port_inline_allowed(void);

/** @brief Bind directly to SVC; ordinary C wrappers are not valid vector glue. */
void section_stl_port_svc_handler(void);

/** @brief Bind directly to PendSV; ordinary C wrappers are not valid vector glue. */
void section_stl_port_pendsv_handler(void);

#endif /* SECTION_STL_M_PORT_H */
