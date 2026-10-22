// SPDX-License-Identifier: MIT
/**
 * @file bsp_srtos.c
 * @brief Bind IDF interrupt return to a scheduler on the existing host task stack.
 * @details No TCB fields, SDK stacks, IRQ vector tables or Wi-Fi tasks are replaced.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "bsp_srtos.h"
#include "bsp_system.h"
#include "section_riscv_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_idf_version.h"
#include "esp_attr.h"
#include "esp_cpu.h"
#include "esp_timer.h"
#include "riscv/rvruntime-frames.h"
#include "soc/soc_caps.h"
#include <stddef.h>
#include <stdlib.h>

#if (SOC_CPU_CORES_NUM != 1)
#error "The hosted RV32 adapter currently requires one integer-only CPU core."
#endif
#if defined(SOC_CPU_COPROC_NUM)
#if (SOC_CPU_COPROC_NUM != 0)
#error "Extend the shared context before enabling coprocessors."
#endif
#endif
#if (ESP_IDF_VERSION_MAJOR != 6) || (ESP_IDF_VERSION_MINOR != 1)
#error "This interrupt-frame adapter requires the reviewed IDF 6.1 RV32 ABI."
#endif

_Static_assert(offsetof(RvExcFrame, mepc) == 0u, "Unexpected IDF IRQ PC offset");
_Static_assert(offsetof(RvExcFrame, a0) == 40u, "Unexpected IDF IRQ a0 offset");
_Static_assert(offsetof(RvExcFrame, t6) == 124u, "Unexpected IDF IRQ frame length");

static void (*p_irq_callback)(uint32_t *, uint32_t, uint32_t); /* Scheduler-owned outer-frame consumer. */
static void (*p_external_enter)(void); /* Scheduler external-call guard entry. */
static void (*p_external_exit)(void);  /* Scheduler external-call guard exit. */
static uint32_t *p_outer_frame;        /* Interrupted host frame, valid through the outer IRQ. */
static uint32_t outer_status;          /* Machine status before SDK changes ISR state. */
static uint32_t outer_cause;           /* Original CLIC interrupt-return priority and privilege. */
static uint32_t initialized;           /* Callback publication complete. */
static uint32_t irq_depth;             /* Balanced SDK entry/exit nesting, including higher-priority IRQs. */
static uint32_t housekeeping_stamp;    /* Last SDK idle opportunity, in wrapping microseconds. */

void bsp_srtos_bind(void (*p_irq)(uint32_t *, uint32_t, uint32_t), void (*p_enter)(void), void (*p_exit)(void))
{
    p_irq_callback   = p_irq;
    p_external_enter = p_enter;
    p_external_exit  = p_exit;
}

void bsp_srtos_init(void)
{
    initialized = 1u;
}

void bsp_system_runtime_enter(void)
{
    if (p_external_enter != NULL)
        p_external_enter();
}

void bsp_system_runtime_exit(void)
{
    if (p_external_exit != NULL)
        p_external_exit();
}

void IRAM_ATTR bsp_srtos_irq_enter(uint32_t *p_frame, uint32_t status, uint32_t cause)
{
    if (irq_depth == 0u)
    {
        p_outer_frame = p_frame;
        outer_status  = status;
        outer_cause   = cause;
    }
    ++irq_depth;
}

void IRAM_ATTR bsp_srtos_irq_exit(void)
{
    /* IDF already masked interrupts before calling rtos_int_exit. */

    if (    (irq_depth == 1u)
         && (initialized == 1u)
         && (p_irq_callback != NULL)
         && (p_outer_frame != NULL))
        p_irq_callback(p_outer_frame, outer_status, outer_cause);

    if (irq_depth > 0u)
        --irq_depth;
}

void section_riscv_housekeeping(void)
{
    const uint32_t now = (uint32_t)(uint64_t)esp_timer_get_time(); /* SDK wall clock; ISR sampling remains independent. */

    if ((uint32_t)(now - housekeeping_stamp) >= 10000u)
    {
        vTaskDelay(1u); /* Let SDK idle/Wi-Fi run under permanently overloaded application tasks. */
        housekeeping_stamp = (uint32_t)(uint64_t)esp_timer_get_time();
    }
}

void section_riscv_idle(void)
{
    esp_cpu_wait_for_intr(); /* Recheck submillisecond jobs after IRQ; housekeeping separately yields to SDK idle. */
}

void section_riscv_fault(uint32_t reason)
{
    (void)reason;
    abort();
}
