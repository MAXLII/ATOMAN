// SPDX-License-Identifier: MIT
/**
 * @file port.c
 * @brief GNU Cortex-M shared-stack dispatch and native MSP return.
 * @details Register save/restore is confined to naked vector functions. The
 *          core runs on MSP with BASEPRI set while it copies task PSP frames.
 *          Higher-priority control IRQs remain eligible throughout that work.
 *          Native MSP software and hardware frames remain resident throughout
 *          dispatch. A NULL selection restores them without an idle task loop.
 *          Object/integer conversions below are limited to architectural
 *          registers, exception frames, and the assembly/C boundary.
 */
#include "port.h"
#include "section.h"

#include <stddef.h>

#if !defined(__GNUC__)
#error "SRTOS_STL M port requires GNU-compatible naked ARM inline assembly."
#endif
#ifndef SECTION_STL_M33_NO_SECURITY
#define SECTION_STL_M33_NO_SECURITY 0 /* ARMv8-M support requires an explicit board choice. */
#endif

#if (SECTION_STL_M33_NO_SECURITY == 1)
#if !defined(__ARM_ARCH_8M_MAIN__)
#error "SECTION_STL_M33_NO_SECURITY requires an ARMv8-M Mainline target."
#endif
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3)
#error "SRTOS_STL M33 port does not support a Secure CMSE image."
#endif
#define SECTION_STL_M_EXC_BASIC 0xFFFFFFBCu /* No Security Extension: S/ES are zero, DCRS is one. */
#define SECTION_STL_M_EXC_FP 0xFFFFFFACu    /* No Security Extension: extended Thread/PSP frame. */
#define SECTION_STL_M_EXC_BASIC_ASM "0xFFFFFFBC" /* Assembly token without the C integer suffix. */
#define SECTION_STL_M_EXC_FP_ASM "0xFFFFFFAC"    /* Assembly token without the C integer suffix. */
#define SECTION_STL_M_NATIVE_BASIC_ASM "0xFFFFFFB8" /* Native Thread/MSP basic frame. */
#define SECTION_STL_M_NATIVE_FP_ASM "0xFFFFFFA8"    /* Native Thread/MSP extended frame. */
#define SECTION_STL_M_LOW_ALIGN_MASK 7u /* PSPLIM uses an 8-byte aligned inclusive low bound. */
#elif (SECTION_STL_M33_NO_SECURITY == 0)
#if !defined(__ARM_ARCH_7M__) && !defined(__ARM_ARCH_7EM__)
#error "Select SECTION_STL_M33_NO_SECURITY=1 for a supported M33; otherwise ARMv7-M is required."
#endif
#define SECTION_STL_M_EXC_BASIC 0xFFFFFFFDu /* Return to Thread mode on PSP without FP state. */
#define SECTION_STL_M_EXC_FP 0xFFFFFFEDu    /* Return to Thread mode on PSP with FP state. */
#define SECTION_STL_M_EXC_BASIC_ASM "0xFFFFFFFD" /* Assembly token without the C integer suffix. */
#define SECTION_STL_M_EXC_FP_ASM "0xFFFFFFED"    /* Assembly token without the C integer suffix. */
#define SECTION_STL_M_NATIVE_BASIC_ASM "0xFFFFFFF9" /* Native Thread/MSP basic frame. */
#define SECTION_STL_M_NATIVE_FP_ASM "0xFFFFFFE9"    /* Native Thread/MSP extended frame. */
#define SECTION_STL_M_LOW_ALIGN_MASK 3u /* ARMv7-M requires natural alignment for stack words. */
#else
#error "SECTION_STL_M33_NO_SECURITY must be 0 or 1."
#endif

#if defined(__ARM_FP) && (__ARM_FP != 0)
#define SECTION_STL_M_FP 1u /* Save upper FP registers when EXC_RETURN requests an extended frame. */
#else
#define SECTION_STL_M_FP 0u /* A pure soft-float image must never activate floating-point state. */
#endif

#define SECTION_STL_M_BASIC_WORDS 17u       /* R4-R11, EXC_RETURN, and the 8-word hardware frame. */
#define SECTION_STL_M_EXTENDED_WORDS 51u    /* Basic frame plus S16-S31 and the 18-word FP hardware frame. */
#define SECTION_STL_M_XPSR_THUMB 0x01000000u /* Required Thumb execution state. */
#define SECTION_STL_M_XPSR_ALIGN 0x00000200u /* Hardware placed 1 alignment word above its frame. */
#define SECTION_STL_M_XPSR_IPSR 0x000001FFu /* A managed task must resume Thread mode. */
#define SECTION_STL_M_STKALIGN 0x00000200u  /* CCR exception entry maintains 8-byte alignment. */
#define SECTION_STL_M_PENDSVSET 0x10000000u /* ICSR write-one PendSV request. */
#define SECTION_STL_M_FP_LSPEN 0x40000000u  /* FPCCR lazy hardware preservation enable. */
#define SECTION_STL_M_FP_ASPEN 0x80000000u  /* FPCCR automatic FP context preservation enable. */

#if (SECTION_STL_M_BASEPRI_CEILING < 0x20u) || (SECTION_STL_M_BASEPRI_CEILING > 0xFFu)
#error "The encoded BASEPRI ceiling must retain an implemented bit on all supported cores."
#endif

static const uint32_t basepri_ceiling __attribute__((used)) = SECTION_STL_M_BASEPRI_CEILING; /* Naked handlers load this value. */
static volatile uint32_t dispatch_active = 0u; /* Remains set until native C execution resumes. */
static volatile uint32_t start_requested __attribute__((used)) = 0u; /* Only port_run may authorize SVC entry. */
static uint32_t *volatile p_native_frame __attribute__((used)) = NULL; /* Retained MSP software frame. */
static volatile uintptr_t runtime_low __attribute__((used)) = 0u; /* Stack low bound checked before PSP stores. */
static volatile uintptr_t runtime_top __attribute__((used)) = 0u; /* Exclusive bound checked before PSP stores. */

/** @return CONTROL bits of the current execution context. */
static uint32_t control_get(void)
{
    uint32_t control = 0u; /* Privilege, stack selection, and FP-active state. */
    __asm__ volatile("mrs %0, control" : "=r"(control));
    return control;
}

/** @return 1 when PRIMASK, BASEPRI, and FAULTMASK are all clear. */
static uint32_t masks_clear(void)
{
    uint32_t primask = 0u; /* Global configurable-interrupt mask. */
    uint32_t basepri = 0u; /* Priority-threshold mask. */
    uint32_t faultmask = 0u; /* Mask that would prevent safe exception entry. */
    __asm__ volatile("mrs %0, primask\n mrs %1, basepri\n mrs %2, faultmask"
                     : "=r"(primask), "=r"(basepri), "=r"(faultmask));
    return ((primask | basepri | faultmask) == 0u) ? 1u : 0u;
}

uint32_t section_stl_port_lock(void)
{
    uint32_t mask = 0u; /* Caller-owned interrupt state for nested critical sections. */
    const uint32_t ceiling = SECTION_STL_M_BASEPRI_CEILING; /* Never weaken an outer BASEPRI lock. */
    __asm__ volatile("mrs %0, basepri\n msr basepri_max, %1\n dsb\n isb"
                     : "=&r"(mask) : "r"(ceiling) : "memory");
    return mask;
}

void section_stl_port_unlock(uint32_t mask)
{
    __asm__ volatile("dsb\n msr basepri, %0\n isb" : : "r"(mask) : "memory");
}

uint32_t section_stl_port_api_allowed(void)
{
    uint32_t exception = 0u; /* Hardware exception number; 0 denotes Thread mode. */
    volatile const uint8_t *const p_shpr = (volatile const uint8_t *)(uintptr_t)0xE000ED18u; /* System priorities. */
    volatile const uint8_t *const p_ipr = (volatile const uint8_t *)(uintptr_t)0xE000E400u; /* External IRQ priorities. */
    uint32_t priority = 0u; /* Register-encoded priority, with smaller values more urgent. */
    __asm__ volatile("mrs %0, ipsr" : "=r"(exception));
    if (exception == 0u)
    {
        return 1u;
    }
    if (exception >= 16u)
    {
        priority = p_ipr[exception - 16u];
    }
    else if ((exception == 11u) || /* SVC is a scheduler-owned configurable handler. */
             (exception == 14u) || /* PendSV may invoke public helpers within its transaction. */
             (exception == 15u)) /* SysTick is admitted only on the scheduler side of the ceiling. */
    {
        priority = p_shpr[exception - 4u];
    }
    else
    {
        return 0u; /* NMI, faults, DebugMonitor and reserved exceptions cannot call scheduler APIs. */
    }
    return (priority >= SECTION_STL_M_BASEPRI_CEILING) ? 1u : 0u;
}

/** @return 1 when the configured priority partition can protect scheduler state. */
static uint32_t priority_policy_valid(void)
{
    volatile const uint32_t *const p_aircr = (volatile const uint32_t *)(uintptr_t)0xE000ED0Cu; /* Priority grouping. */
    volatile const uint8_t *const p_shpr = (volatile const uint8_t *)(uintptr_t)0xE000ED18u; /* System priorities. */
    const uint32_t grouping = (*p_aircr >> 8u) & 7u; /* Architectural binary-point selector. */
    const uint32_t group_mask = 0xFFu & ~((1u << (grouping + 1u)) - 1u); /* Exclude subpriority bits. */
    const uint32_t probe = 0xFFu; /* Probe implemented BASEPRI bits without changing NVIC registers. */
    uint32_t previous = 0u; /* Precisely restored BASEPRI; the caller has checked masks_clear. */
    uint32_t implemented = 0u; /* Readback identifies the least urgent implementable priority. */
    __asm__ volatile("mrs %0, basepri\n msr basepri_max, %2\n dsb\n isb\n mrs %1, basepri\n"
                     "msr basepri, %0\n dsb\n isb"
                     : "=&r"(previous), "=&r"(implemented) : "r"(probe) : "memory");
    return ((previous == 0u) && /* Admission must begin with scheduler interrupts available. */
            (grouping < 7u) && /* At least 1 preemption bit must separate control IRQs. */
            ((implemented & SECTION_STL_M_BASEPRI_CEILING) == SECTION_STL_M_BASEPRI_CEILING) && /* Exact encoding. */
            ((group_mask & SECTION_STL_M_BASEPRI_CEILING) == SECTION_STL_M_BASEPRI_CEILING) && /* Group boundary. */
            ((uint32_t)p_shpr[10] == implemented) && /* PendSV cannot preempt a configurable handler. */
            ((uint32_t)p_shpr[7] >= SECTION_STL_M_BASEPRI_CEILING) && /* SVC shares scheduler protection. */
            ((uint32_t)p_shpr[11] >= SECTION_STL_M_BASEPRI_CEILING)) ? 1u : 0u; /* Avoid a SysTick CTRL read. */
}

uint32_t *section_stl_port_prepare(uint32_t *p_low, uint32_t *p_top, void (*p_entry)(void))
{
    const uintptr_t low = (uintptr_t)p_low; /* Address validation avoids subtracting unrelated pointers. */
    const uintptr_t top = (uintptr_t)p_top; /* Exclusive stack bound provided by the resident core. */
    uint32_t *p_frame = NULL; /* Initial combined software/hardware frame. */

    if ((p_low == NULL) || /* Reject an unbound runtime stack. */
        (p_top == NULL) || /* Reject an absent stack bound. */
        (p_entry == NULL) || /* A task always enters a resident trampoline. */
        (((uintptr_t)p_entry & 1u) == 0u) || /* GNU Cortex-M function pointers encode Thumb state. */
        (((uintptr_t)p_entry & ~(uintptr_t)1u) == 0u) || /* Reject a Thumb tag without an entry address. */
        ((low & SECTION_STL_M_LOW_ALIGN_MASK) != 0u) || /* Honor this architecture's stack-limit alignment. */
        ((top & 7u) != 0u) || /* Exception return requires the configured ABI alignment. */
        (top < low) || /* Bound the unsigned size computation. */
        ((top - low) < (SECTION_STL_M_BASIC_WORDS * sizeof(uint32_t)))) /* Fit the complete initial frame. */
    {
        return NULL;
    }

    runtime_low = low;
    runtime_top = top;
    p_frame = p_top - SECTION_STL_M_BASIC_WORDS;
    for (uint32_t index = 0u; index < SECTION_STL_M_BASIC_WORDS; ++index) /* Zero all initial CPU registers. */
    {
        p_frame[index] = 0u;
    }
    p_frame[8u] = SECTION_STL_M_EXC_BASIC;
    p_frame[14u] = 0xFFFFFFFFu; /* An accidentally returning trampoline must fault, never repeat a task. */
    p_frame[15u] = (uint32_t)((uintptr_t)p_entry & ~(uintptr_t)1u); /* Exception PC stores the aligned address. */
    p_frame[16u] = SECTION_STL_M_XPSR_THUMB;
    return p_frame;
}

uint32_t section_stl_port_frame_valid(const uint32_t *p_sp, uint32_t words)
{
    uint32_t required = SECTION_STL_M_BASIC_WORDS; /* Minimum complete basic frame. */
    uint32_t xpsr_index = 16u; /* Thumb/thread state in a basic software-plus-hardware frame. */
    uint32_t xpsr = 0u; /* Stacked execution state, read only after the length check. */

    if ((p_sp == NULL) || /* An empty image cannot be resumed. */
        (((uintptr_t)p_sp & 3u) != 0u) || /* Copied images remain word aligned. */
        (words < SECTION_STL_M_BASIC_WORDS)) /* Reading EXC_RETURN and basic xPSR must be bounded. */
    {
        return 0u;
    }
    if (p_sp[8u] != SECTION_STL_M_EXC_BASIC)
    {
#if (SECTION_STL_M_FP == 1u)
        if (p_sp[8u] != SECTION_STL_M_EXC_FP)
        {
            return 0u;
        }
        required = SECTION_STL_M_EXTENDED_WORDS;
        xpsr_index = 32u; /* High FP software words precede the core frame; low FP hardware words follow it. */
#else
        return 0u;
#endif
    }
    if (words < required)
    {
        return 0u;
    }
    xpsr = p_sp[xpsr_index];
    if ((xpsr & SECTION_STL_M_XPSR_ALIGN) != 0u)
    {
        ++required;
    }
    if ((words < required) || /* Include the hardware alignment word when present. */
        ((xpsr & SECTION_STL_M_XPSR_THUMB) == 0u) || /* Managed code executes only Thumb instructions. */
        ((xpsr & SECTION_STL_M_XPSR_IPSR) != 0u) || /* Never restore a business frame into Handler mode. */
        (p_sp[xpsr_index - 1u] == 0u) || /* A null continuation is not executable. */
        ((p_sp[xpsr_index - 1u] & 1u) != 0u)) /* The stacked PC contains an aligned instruction address. */
    {
        return 0u;
    }
    return 1u;
}

uint32_t section_stl_port_in_handler(void)
{
    uint32_t ipsr = 0u; /* Architectural active-exception number. */
    __asm__ volatile("mrs %0, ipsr" : "=r"(ipsr));
    return (ipsr == 0u) ? 0u : 1u;
}

uint32_t section_stl_port_active(void)
{
    return dispatch_active;
}

uint32_t section_stl_port_inline_allowed(void)
{
    volatile const uint32_t *const p_icsr = (volatile const uint32_t *)(uintptr_t)0xE000ED04u; /* Pending exceptions. */
    if ((dispatch_active == 1u) && /* The resident port owns this dispatch. */
        (section_stl_port_in_handler() == 0u) && /* Only a completed callback on PSP can inline another. */
        ((control_get() & 3u) == 2u) && /* Require privileged Thread/PSP execution. */
        (masks_clear() == 1u) && /* The caller must check before its own critical section. */
        ((*p_icsr & SECTION_STL_M_PENDSVSET) == 0u)) /* Honor an already requested context switch. */
    {
        return 1u;
    }
    return 0u;
}

uint32_t section_stl_port_switch_active(void)
{
    volatile const uint32_t *const p_shcsr = (volatile const uint32_t *)(uintptr_t)0xE000ED24u; /* Handler activity. */
    return ((*p_shcsr & 0x00000400u) != 0u) ? 1u : 0u; /* Architectural PENDSVACT bit. */
}

void section_stl_port_request_switch(void)
{
    volatile uint32_t *const p_icsr = (volatile uint32_t *)(uintptr_t)0xE000ED04u; /* Architectural ICSR. */
    if (dispatch_active == 1u)
    {
        *p_icsr = SECTION_STL_M_PENDSVSET;
        __asm__ volatile("dsb\n isb" : : : "memory");
    }
}

uint32_t section_stl_port_run(void)
{
    volatile const uint32_t *const p_ccr = (volatile const uint32_t *)(uintptr_t)0xE000ED14u; /* Stack alignment policy. */
    uint32_t saved_psp = 0u; /* Native caller's inactive PSP register. */
    uint32_t mask = 0u; /* Short admission/cleanup critical section. */
    uint32_t accepted = 0u; /* Confirm the bound SVC handler actually accepted this launch. */
#if (SECTION_STL_M33_NO_SECURITY == 1)
    volatile const uint32_t *const p_cpuid = (volatile const uint32_t *)(uintptr_t)0xE000ED00u; /* Core identity. */
    volatile const uint32_t *const p_pfr1 = (volatile const uint32_t *)(uintptr_t)0xE000ED44u; /* Security extension. */
    uint32_t saved_psplim = 0u; /* Preserve the native caller's inactive process-stack limit. */
#endif
#if (SECTION_STL_M_FP == 1u)
    volatile const uint32_t *const p_cpacr = (volatile const uint32_t *)(uintptr_t)0xE000ED88u; /* CP10/CP11 access. */
    volatile uint32_t *const p_fpccr = (volatile uint32_t *)(uintptr_t)0xE000EF34u; /* Automatic FP frame policy. */
    uint32_t saved_lspen = 0u; /* Only the lazy-stacking policy is temporarily changed. */
#endif

    if ((section_stl_port_in_handler() == 1u) || /* SVC cannot replace a handler's native context. */
        ((control_get() & 3u) != 0u) || /* An RTOS PSP caller or unprivileged caller is unsupported. */
        (masks_clear() == 0u) || /* SVC and PendSV must be able to run. */
        ((*p_ccr & SECTION_STL_M_STKALIGN) == 0u)) /* Require the exception ABI's 8-byte alignment. */
    {
        return 0u;
    }
    if (priority_policy_valid() == 0u)
    {
        return 0u;
    }
#if (SECTION_STL_M33_NO_SECURITY == 1)
    if (((*p_cpuid & 0xFF00FFF0u) != 0x4100D210u) || /* Admit only an Arm Cortex-M33, any revision. */
        ((*p_pfr1 & 0xF0u) != 0u)) /* A disabled or unused Security Extension is still unsupported. */
    {
        return 0u;
    }
#endif
#if (SECTION_STL_M_FP == 1u)
    if (((*p_cpacr & 0x00F00000u) != 0x00F00000u) || /* Do not silently enable a disabled coprocessor. */
        ((*p_fpccr & SECTION_STL_M_FP_ASPEN) == 0u)) /* EXC_RETURN must accurately describe FP context. */
    {
        return 0u;
    }
#else
    if ((control_get() & 4u) != 0u)
    {
        return 0u;
    }
#endif
    mask = section_stl_port_lock();
    if ((dispatch_active == 1u) || /* A 2nd native invocation cannot replace the saved host frame. */
        (start_requested == 1u)) /* An admitted SVC launch already owns the transition. */
    {
        section_stl_port_unlock(mask);
        return 0u;
    }
    __asm__ volatile("mrs %0, psp" : "=r"(saved_psp));
#if (SECTION_STL_M33_NO_SECURITY == 1)
    __asm__ volatile("mrs %0, psplim" : "=r"(saved_psplim));
#endif
#if (SECTION_STL_M_FP == 1u)
    saved_lspen = *p_fpccr & SECTION_STL_M_FP_LSPEN;
    *p_fpccr &= ~SECTION_STL_M_FP_LSPEN; /* Materialize all interrupted FP state before stack copying. */
#endif
    start_requested = 1u;
    section_stl_port_unlock(mask);
    __asm__ volatile("svc 0x53" : : : "memory"); /* SVC retains this invocation on MSP until drain/fault. */
    mask = section_stl_port_lock();
    accepted = dispatch_active;
    dispatch_active = 0u; /* The native caller has now resumed; quiescence may subsequently be acknowledged. */
    start_requested = 0u;
    p_native_frame = NULL;
#if (SECTION_STL_M33_NO_SECURITY == 1)
    __asm__ volatile("msr psplim, %0\n msr psp, %1\n msr psplim, %2\n isb"
                     : : "r"(0u), "r"(saved_psp), "r"(saved_psplim) : "memory");
#else
    __asm__ volatile("msr psp, %0" : : "r"(saved_psp) : "memory");
#endif
#if (SECTION_STL_M_FP == 1u)
    *p_fpccr = (*p_fpccr & ~SECTION_STL_M_FP_LSPEN) | saved_lspen;
#endif
    section_stl_port_unlock(mask);
    return accepted;
}

/* The assembly symbols below refer only to this translation unit's resident
 * storage. Basic asm is required in naked functions: no C statements, operands,
 * compiler-generated stack adjustment, or ordinary C vector wrappers are safe. */

/** @brief Fail before corrupting resident memory; the board fault handler must not return. */
static void __attribute__((naked, used, noreturn)) port_stack_fault(void)
{
    __asm__ volatile(
        "cpsid i                             \n"
        "udf #0                              \n" /* Escalate through the board's fatal exception path. */
        "b port_stack_fault                  \n"); /* Never resume a corrupt PSP if a fault handler returns. */
}

/** @brief Restore the retained native MSP frame; called only with interrupts masked. */
static void __attribute__((naked, used, noreturn)) native_restore(void)
{
    __asm__ volatile(
        "ldr r0, =0xE000ED04                 \n"
        "ldr r1, =0x08000000                 \n" /* Clear stale PendSV before native return. */
        "str r1, [r0]                        \n"
        "ldr r0, =p_native_frame             \n"
        "ldr r0, [r0]                        \n"
        "mov sp, r0                          \n"
        "ldmia sp!, {r3-r11, lr}             \n" /* r3 holds the native BASEPRI; frame is 10 words. */
#if (SECTION_STL_M_FP == 1u)
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vldmiaeq sp!, {s16-s31}              \n"
#endif
        "dsb                                 \n"
        "msr basepri, r3                     \n"
        "isb                                 \n"
        "bx lr                               \n");
}

void __attribute__((naked)) section_stl_port_svc_handler(void)
{
    __asm__ volatile(
        "mrs r2, basepri                     \n"
        "ldr r0, =basepri_ceiling             \n"
        "ldr r0, [r0]                        \n"
        "msr basepri_max, r0                 \n"
        "dsb                                 \n"
        "isb                                 \n"
        "ldr r0, =" SECTION_STL_M_NATIVE_BASIC_ASM " \n"
        "cmp lr, r0                          \n"
        "beq 3f                              \n" /* Accept only an originating native Thread/MSP frame. */
#if (SECTION_STL_M_FP == 1u)
        "ldr r0, =" SECTION_STL_M_NATIVE_FP_ASM " \n"
        "cmp lr, r0                          \n"
        "beq 3f                              \n"
#endif
        "b 2f                                \n"
        "3:                                  \n"
        "mov r1, sp                          \n"
        "ldr r1, [r1, #24]                   \n" /* PC immediately following the 16-bit SVC instruction. */
        "ldrb r1, [r1, #-2]                  \n"
        "cmp r1, #0x53                       \n"
        "bne 2f                              \n" /* Do not consume an unrelated SVC during admission. */
        "ldr r0, =start_requested            \n"
        "ldr r0, [r0]                        \n"
        "cmp r0, #1                          \n"
        "bne 2f                              \n"
        "ldr r0, =dispatch_active            \n"
        "ldr r1, [r0]                        \n"
        "cmp r1, #0                          \n"
        "bne 2f                              \n"
#if (SECTION_STL_M_FP == 1u)
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vstmdbeq sp!, {s16-s31}              \n"
#endif
        "mov r3, r2                          \n"
        "stmdb sp!, {r3-r11, lr}             \n" /* Save 10 words so subsequent C calls keep MSP aligned. */
        "ldr r1, =p_native_frame             \n"
        "mov r2, sp                          \n"
        "str r2, [r1]                        \n"
        "movs r1, #1                         \n"
        "str r1, [r0]                        \n"
        "movs r0, #0                         \n"
        "bl section_stl_switch               \n"
        "cmp r0, #0                          \n"
        "beq native_restore                  \n"
        "ldmia r0!, {r4-r11, lr}             \n"
#if (SECTION_STL_M_FP == 1u)
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vldmiaeq r0!, {s16-s31}              \n"
#endif
#if (SECTION_STL_M33_NO_SECURITY == 1)
        "ldr r1, =runtime_low                \n"
        "ldr r1, [r1]                        \n"
        "msr psplim, r1                      \n" /* prepare has now established the shared-stack bound. */
#endif
        "msr psp, r0                         \n"
        "dsb                                 \n"
        "ldr r1, =p_native_frame             \n"
        "ldr r1, [r1]                        \n"
        "ldr r1, [r1]                        \n" /* Native software frame starts with the saved BASEPRI. */
        "msr basepri, r1                     \n"
        "isb                                 \n"
        "bx lr                               \n"
        "2:                                  \n"
        "dsb                                 \n"
        "msr basepri, r2                     \n"
        "isb                                 \n"
        "bx lr                               \n");
}

void __attribute__((naked)) section_stl_port_pendsv_handler(void)
{
    __asm__ volatile(
        "mrs r0, basepri                     \n"
        "ldr r1, =basepri_ceiling             \n"
        "ldr r1, [r1]                        \n"
        "msr basepri_max, r1                 \n"
        "dsb                                 \n"
        "isb                                 \n"
        "push {r0, lr}                       \n" /* Preserve mask and return target with 8-byte alignment. */
        "ldr r0, =dispatch_active            \n"
        "ldr r0, [r0]                        \n"
        "cmp r0, #1                          \n"
        "bne 2f                              \n"
        "tst lr, #4                          \n"
        "beq 2f                              \n" /* Late requests on the native MSP caller are ignored. */
        "mrs r0, psp                         \n"
        "tst r0, #7                          \n"
        "bne port_stack_fault                \n" /* Hardware frame alignment follows the required STKALIGN policy. */
        "movs r2, #36                        \n" /* Basic software frame size in bytes. */
        "movs r3, #32                        \n" /* Basic hardware frame size in bytes. */
        "ldr r1, =" SECTION_STL_M_EXC_BASIC_ASM " \n"
        "cmp lr, r1                          \n"
        "beq 3f                              \n"
#if (SECTION_STL_M_FP == 1u)
        "ldr r1, =" SECTION_STL_M_EXC_FP_ASM " \n"
        "cmp lr, r1                          \n"
        "bne port_stack_fault                \n"
        "movs r2, #100                       \n" /* R4-R11, EXC_RETURN, and S16-S31. */
        "movs r3, #104                       \n" /* Core hardware frame plus S0-S15/FPSCR/reserved. */
#else
        "b port_stack_fault                  \n" /* An FP/Handler frame is outside this build's contract. */
#endif
        "3:                                  \n"
        "subs r1, r0, r2                     \n"
        "bcc port_stack_fault                \n" /* Reject unsigned underflow before comparing bounds. */
        "ldr r2, =runtime_low                \n"
        "ldr r2, [r2]                        \n"
        "cmp r1, r2                          \n"
        "blo port_stack_fault                \n" /* No software save may write below the resident stack. */
        "adds r1, r0, r3                     \n"
        "bcs port_stack_fault                \n"
        "ldr r2, =runtime_top                \n"
        "ldr r2, [r2]                        \n"
        "cmp r1, r2                          \n"
        "bhi port_stack_fault                \n" /* Require the complete hardware frame to lie inside the stack. */
#if (SECTION_STL_M_FP == 1u)
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vstmdbeq r0!, {s16-s31}              \n"
#endif
        "stmdb r0!, {r4-r11, lr}             \n"
        "bl section_stl_switch               \n"
        "cmp r0, #0                          \n"
        "beq native_restore                  \n" /* Restore MSP from its resident pointer, dropping this local frame. */
        "ldmia r0!, {r4-r11, lr}             \n"
#if (SECTION_STL_M_FP == 1u)
        "tst lr, #0x10                       \n"
        "it eq                               \n"
        "vldmiaeq r0!, {s16-s31}              \n"
#endif
#if (SECTION_STL_M33_NO_SECURITY == 1)
        "ldr r1, =runtime_low                \n"
        "ldr r1, [r1]                        \n"
        "msr psplim, r1                      \n" /* Reestablish the resident limit before every task return. */
#endif
        "msr psp, r0                         \n"
        "pop {r1, r2}                        \n"
        "dsb                                 \n"
        "msr basepri, r1                     \n"
        "isb                                 \n"
        "bx lr                               \n"
        "2:                                  \n"
        "pop {r0, lr}                        \n"
        "dsb                                 \n"
        "msr basepri, r0                     \n"
        "isb                                 \n"
        "bx lr                               \n");
}
