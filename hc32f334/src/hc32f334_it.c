#include "systick.h"
#include "section.h"
#include "hc32f3xx.h"

#if defined(__GNUC__) || defined(__ARMCC_VERSION)
#define HC32_EXCEPTION_NAKED __attribute__((naked))
#else
#define HC32_EXCEPTION_NAKED
#endif

void hardfault_capture(uint32_t *stack, uint32_t exc_return);

void SysTick_Handler(void)
{
    delay_decrement();
    section_task_tick();
    if ((section_task_scheduler_started() != 0u) &&
        (section_task_switch_pending() != 0u))
    {
        SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    }
}

void HC32_EXCEPTION_NAKED SVC_Handler(void)
{
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_start_request     \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
}

void HC32_EXCEPTION_NAKED PendSV_Handler(void)
{
#if defined(__FPU_USED) && (__FPU_USED == 1U)
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_scheduler_started \n"
        "cmp r0, #0                        \n"
        "beq 1f                            \n"
        "ldr lr, [sp, #4]                  \n"
        "tst lr, #0x04                     \n"
        "beq 2f                            \n"
        "mrs r0, psp                       \n"
        "cbz r0, 2f                        \n"
        "tst lr, #0x10                     \n"
        "it eq                             \n"
        "vstmdbeq r0!, {s16-s31}           \n"
        "stmdb r0!, {r4-r11, lr}           \n"
        "b 3f                              \n"
        "2:                                \n"
        "movs r0, #0                       \n"
        "3:                                \n"
        "bl section_task_switch_sp         \n"
        "cbz r0, 1f                        \n"
        "ldmia r0!, {r4-r11, lr}           \n"
        "tst lr, #0x10                     \n"
        "it eq                             \n"
        "vldmiaeq r0!, {s16-s31}           \n"
        "msr psp, r0                       \n"
        "add sp, sp, #8                    \n"
        "bx lr                             \n"
        "1:                                \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
#else
    __ASM volatile(
        "push {r0, lr}                     \n"
        "bl section_task_scheduler_started \n"
        "cmp r0, #0                        \n"
        "beq 1f                            \n"
        "ldr lr, [sp, #4]                  \n"
        "tst lr, #0x04                     \n"
        "beq 2f                            \n"
        "mrs r0, psp                       \n"
        "cbz r0, 2f                        \n"
        "stmdb r0!, {r4-r11, lr}           \n"
        "b 3f                              \n"
        "2:                                \n"
        "movs r0, #0                       \n"
        "3:                                \n"
        "bl section_task_switch_sp         \n"
        "cbz r0, 1f                        \n"
        "ldmia r0!, {r4-r11, lr}           \n"
        "msr psp, r0                       \n"
        "add sp, sp, #8                    \n"
        "bx lr                             \n"
        "1:                                \n"
        "pop {r0, r1}                      \n"
        "bx r1                             \n");
#endif
}

void HC32_EXCEPTION_NAKED HardFault_Handler(void)
{
    __ASM volatile(
        "tst lr, #4                        \n"
        "ite eq                            \n"
        "mrseq r0, msp                     \n"
        "mrsne r0, psp                     \n"
        "mov r1, lr                        \n"
        "b hardfault_capture               \n");
}

void hardfault_capture(uint32_t *stack, uint32_t exc_return)
{
    uint32_t *basic_frame = stack;

    g_section_fault_debug.cfsr = SCB->CFSR;
    g_section_fault_debug.hfsr = SCB->HFSR;
    g_section_fault_debug.bfar = SCB->BFAR;
    g_section_fault_debug.mmfar = SCB->MMFAR;
    g_section_fault_debug.exc_return = exc_return;
    g_section_fault_debug.msp = __get_MSP();
    g_section_fault_debug.psp = __get_PSP();

    if (stack != NULL)
    {
        if ((exc_return & 0x10u) == 0u)
        {
            basic_frame = &stack[18];
        }

        g_section_fault_debug.stacked_lr = basic_frame[5];
        g_section_fault_debug.stacked_pc = basic_frame[6];
        g_section_fault_debug.stacked_xpsr = basic_frame[7];
    }

    while (1)
    {
    }
}

void MemManage_Handler(void)
{
    while (1)
    {
    }
}

void BusFault_Handler(void)
{
    while (1)
    {
    }
}

void UsageFault_Handler(void)
{
    while (1)
    {
    }
}
