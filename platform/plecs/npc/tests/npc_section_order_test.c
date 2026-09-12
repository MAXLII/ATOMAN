// SPDX-License-Identifier: MIT
/**
 * @file    npc_section_order_test.c
 * @brief   Verify registration and callback order with the production Section runtime.
 * @details Links the NPC objects, suppresses application callbacks in this test process,
 *          and exercises real initialization, interrupt and task dispatch.
 *          Does not start PLECS, network services or power-control callbacks.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "section.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const reg_section_t section_reg_start; /* Production registration range boundary. */
extern const reg_section_t section_reg_stop; /* Production registration range boundary. */
static char calls[32]; /* Observed callback execution sequence. */
static size_t call_count = 0u; /* Number of recorded fixture callbacks. */

/** @param condition Required invariant. */
static void check(int condition)
{
    if (condition == 0)
    {
        fprintf(stderr, "Section order check failed; callbacks=%s\n", calls);
        exit(EXIT_FAILURE);
    }
}

/** @param tag Callback identity in the execution trace. */
static void record(char tag)
{
    check(call_count < sizeof(calls) - 1u);
    calls[call_count] = tag;
    ++call_count;
    calls[call_count] = '\0';
}

/** @brief Harmless replacement for non-fixture callbacks. */
static void idle(void) {}
static void init_a(void) { record('a'); }
static void init_b(void) { record('b'); }
static void init_c(void) { record('c'); }
static void init_d(void) { record('d'); }
static void init_e(void) { record('e'); }
static void init_f(void) { record('f'); }
REG_INIT(5, init_a)
REG_INIT(2, init_b)
REG_INIT(5, init_c)
REG_INIT(-2, init_d)
REG_INIT(5, init_e)
REG_INIT(2, init_f)

static void irq_a(void) { record('a'); }
static void irq_b(void) { record('b'); }
static void irq_c(void) { record('c'); }
static void irq_d(void) { record('d'); }
static void irq_e(void) { record('e'); }
static void irq_f(void) { record('f'); }
REG_INTERRUPT(5, irq_a)
REG_INTERRUPT(2, irq_b)
REG_INTERRUPT(5, irq_c)
REG_INTERRUPT(0, irq_d)
REG_INTERRUPT(5, irq_e)
REG_INTERRUPT(2, irq_f)

static void task_a(void) { record('a'); }
static void task_b(void) { record('b'); }
static void task_c(void) { record('c'); }
REG_TASK_MS(5, task_a)
REG_TASK_MS(1, task_b)
REG_TASK_MS(3, task_c)

/** @brief Replace application callbacks before the real dispatcher builds its lists. */
static void isolate_callbacks(void)
{
    for (const reg_section_t *p_reg = &section_reg_start + 1; p_reg < &section_reg_stop; ++p_reg)
    {
        section_item_t *p_item = p_reg->p_str; /* Mutable wrapper owned by this test process. */
        if (p_reg->section_type == SECTION_INIT)
        {
            reg_init_t *p_init = p_item->p_obj; /* Application or fixture initializer. */
            if ((p_init->p_func != init_a) && /* Keep fixture callbacks only. */
                (p_init->p_func != init_b) && /* Keep fixture callbacks only. */
                (p_init->p_func != init_c) && /* Keep fixture callbacks only. */
                (p_init->p_func != init_d) && /* Keep fixture callbacks only. */
                (p_init->p_func != init_e) && /* Keep fixture callbacks only. */
                (p_init->p_func != init_f))   /* Keep fixture callbacks only. */
            {
                p_init->p_func = idle;
            }
        }
        else if (p_reg->section_type == SECTION_INTERRUPT)
        {
            reg_interrupt_t *p_irq = p_item->p_obj; /* Application or fixture interrupt callback. */
            if ((p_irq->p_func != irq_a) && /* Keep fixture callbacks only. */
                (p_irq->p_func != irq_b) && /* Keep fixture callbacks only. */
                (p_irq->p_func != irq_c) && /* Keep fixture callbacks only. */
                (p_irq->p_func != irq_d) && /* Keep fixture callbacks only. */
                (p_irq->p_func != irq_e) && /* Keep fixture callbacks only. */
                (p_irq->p_func != irq_f))   /* Keep fixture callbacks only. */
            {
                p_irq->p_func = idle;
            }
        }
        else if (p_reg->section_type == SECTION_TASK)
        {
            reg_task_t *p_task = p_item->p_obj; /* Application or fixture periodic task. */
            if ((p_task->p_func != task_a) && /* Keep fixture callbacks only. */
                (p_task->p_func != task_b) && /* Keep fixture callbacks only. */
                (p_task->p_func != task_c))   /* Keep fixture callbacks only. */
            {
                p_task->p_func = idle;
            }
        }
    }
}

/** @param p_node Runtime wrapper. @return Its index in the linked registration table. */
static size_t registration_index(const section_item_t *p_node)
{
    size_t index = 0u; /* Registration rank independent of runtime list pointers. */
    for (const reg_section_t *p_reg = &section_reg_start + 1; p_reg < &section_reg_stop; ++p_reg)
    {
        if (p_reg->p_str == p_node) return index;
        ++index;
    }
    check(0);
    return 0u;
}

/** @param p_head Runtime list head. @param kind Registration category used for priority comparison. */
static void check_list(const section_item_t *p_head, SECTION_E kind)
{
    int previous_priority = -129; /* Below the signed initialization priority range. */
    size_t previous_index = 0u; /* Registration rank of the preceding node. */
    size_t count = 0u; /* Bound traversal and detect cycles. */
    for (const section_item_t *p_node = p_head; p_node != NULL; p_node = p_node->p_next)
    {
        int priority = 0; /* Task and link lists retain registration order without priorities. */
        size_t index = registration_index(p_node); /* Original registration position. */
        if (kind == SECTION_INIT) priority = ((const reg_init_t *)p_node->p_obj)->priority;
        if (kind == SECTION_INTERRUPT) priority = ((const reg_interrupt_t *)p_node->p_obj)->priority;
        check(priority >= previous_priority);
        if (priority == previous_priority) check(index > previous_index);
        previous_priority = priority;
        previous_index = index;
        ++count;
        check(count < 256u);
    }
    check(count != 0u);
}

/** @brief Clear the fixture execution trace between dispatcher calls. */
static void clear_calls(void)
{
    call_count = 0u;
    calls[0] = '\0';
}

/** @return EXIT_SUCCESS if production list construction and dispatch preserve the contract. */
int main(void)
{
    isolate_callbacks();
    for (unsigned int pass = 0u; pass < 2u; ++pass) /* Reinitialization must preserve order too. */
    {
        clear_calls();
        section_init();
        check(strcmp(calls, "dbface") == 0);
        check_list(p_init_first, SECTION_INIT);
        check_list(p_interrupt_first, SECTION_INTERRUPT);
        check_list(p_task_first, SECTION_TASK);
        check_list(p_link_first, SECTION_LINK);
        clear_calls();
        section_interrupt();
        check(strcmp(calls, "dbface") == 0);
        clear_calls();
        reg_task_task_a.is_ready = 1u;
        reg_task_task_b.is_ready = 1u;
        reg_task_task_c.is_ready = 1u;
        run_task();
        check(strcmp(calls, "abc") == 0);
        clear_calls();
        reg_task_task_a.is_ready = 1u;
        reg_task_task_c.is_ready = 1u;
        run_task();
        check(strcmp(calls, "ac") == 0);
    }
    puts("PASS: stable init/interrupt priorities, task/link order, callback execution and reinitialization");
    return EXIT_SUCCESS;
}
