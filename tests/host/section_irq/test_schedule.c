// SPDX-License-Identifier: MIT
/**
 * @file test_schedule.c
 * @brief Execute production queue, deadline and global-window policies on host.
 * @details Only CPU exception delivery and stack copying are outside this test.
 *          Physical-board acceptance exercises those paths separately.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "section.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "task_state.inc"

uint32_t section_testbench_time_100us;
volatile section_fault_debug_t g_section_fault_debug;
#if TEST_A9
volatile section_scheduler_debug_t g_section_scheduler_debug;
#endif
static reg_task_t *p_srtos_task_ready_first, *p_srtos_task_ready_tail;
static reg_task_t *p_task_current;
static uint32_t task_last_switch_tick;
static uint8_t task_scheduler_ready = 1u;
section_item_t *p_task_first;
#define section_race_probe_begin(tag)             ((void)(tag))
#define section_race_probe_end()                  ((void)0)
#define section_race_probe_invariant(first, tail) ((void)(first), (void)(tail))
#define section_race_probe_delay()                ((void)0)
static uint32_t section_critical_enter(void)
{
    return 0u;
}
static void section_critical_exit(uint32_t state)
{
    (void)state;
}
static void srtos_task_ready_enqueue_unlocked(reg_task_t **first, reg_task_t **tail, reg_task_t *task);
static reg_task_t *srtos_task_ready_pop_unlocked(reg_task_t **first, reg_task_t **tail);
static void srtos_task_schedule_next(reg_task_t *task, uint32_t elapsed);
#include "schedule_production.inc"

static void callback(void)
{
}
static void enqueue_old(reg_task_t *task)
{
    task->state = (uint8_t)TASK_STACK_STATE_READY_OLD;
    srtos_task_ready_enqueue_unlocked(&p_srtos_task_ready_first, &p_srtos_task_ready_tail, task);
}
int main(void)
{
    reg_task_t a = {0}, b = {0}, c = {0};
    a.t_period = b.t_period = c.t_period = 10u;
    a.p_func = b.p_func = c.p_func = callback;
    /* Repeated new arrivals cannot bypass either older suspended context. */
    enqueue_old(&a);
    enqueue_old(&b);
    assert(srtos_task_activate_if_due(&c, 10u) == 1u);
#if !TEST_A9
    assert(section_task_start_ready_new_inline() == 0u);
#endif
    assert(task_stack_pick_next() == &a);
    enqueue_old(&a);
    assert(task_stack_pick_next() == &b);
    assert(srtos_task_activate_if_due(&c, 10u) == 0u);
    assert(task_stack_pick_next() == &c);
    c.state = (uint8_t)TASK_STACK_STATE_SLEEPING;
    assert(srtos_task_activate_if_due(&c, 10u) == 1u);
#if !TEST_A9
    assert(section_task_start_ready_new_inline() == 0u);
#endif
    assert(task_stack_pick_next() == &a);
    assert(task_stack_pick_next() == &c);
    assert(task_stack_pick_next() == NULL);
    /* A voluntary continuation joins the same FIFO behind waiting new work. */
    a.state = (uint8_t)TASK_STACK_STATE_SLEEPING;
    assert(srtos_task_activate_if_due(&a, 10u) == 1u);
    p_task_current = &b;
    b.state        = (uint8_t)TASK_STACK_STATE_RUNNING;
    section_task_continue_current();
    assert(task_stack_pick_next() == &a);
    assert(task_stack_pick_next() == &b);
    assert(task_stack_pick_next() == NULL);
    /* Busy and suspended invocations retain the deadline of their release. */
    b.time_last = 100u;
    b.state     = (uint8_t)TASK_STACK_STATE_RUNNING;
    assert(    srtos_task_activate_if_due(&b, 15u) == 0u
            && b.time_last == 100u);
    b.state = (uint8_t)TASK_STACK_STATE_READY_OLD;
    assert(    srtos_task_activate_if_due(&b, 25u) == 0u
            && b.time_last == 100u);
    p_task_current = &b;
    section_task_complete_current();
    assert(    srtos_task_activate_if_due(&b, 15u) == 1u
            && b.time_last == 110u);
    assert(task_stack_pick_next() == &b);
    /* Short lateness retains phase; long lateness synchronises to now. */

    for (uint32_t elapsed = 10u; elapsed <= 25u; ++elapsed)
    {
        a.time_last = b.time_last = 100u;
        srtos_task_schedule_next(&a, elapsed);
        baremetal_task_schedule_next(&b, elapsed);
        assert(a.time_last == (elapsed < 20u ? 110u : 100u + elapsed));
        assert(a.time_last == b.time_last);
    }
    a.time_last = UINT32_MAX - 4u;
    srtos_task_schedule_next(&a, 15u);
    assert(a.time_last == 5u);
    /* A 600 us callback leaves 400 us in the same 1 ms global window. */
    task_last_switch_tick        = 100u;
    section_testbench_time_100us = 106u;
    task_slice_advance();
    assert(    task_last_switch_tick == 100u
            && section_task_slice_elapsed() == 0u);
#if !TEST_A9
    a.is_ready = 0u;
    a.state    = (uint8_t)TASK_STACK_STATE_SLEEPING;
    assert(srtos_task_activate_if_due(&a, 10u) == 1u);
    assert(    section_task_start_ready_new_inline() == 1u
            && p_task_current == &a);
    assert(task_last_switch_tick == 100u);
#endif
    section_testbench_time_100us = 110u;
    assert(section_task_slice_elapsed() == 1u);
    task_slice_advance();
    assert(task_last_switch_tick == 110u);
    section_testbench_time_100us = 137u;
    task_slice_advance();
    assert(    task_last_switch_tick == 130u
            && section_task_slice_elapsed() == 0u);
    task_last_switch_tick        = UINT32_MAX - 4u;
    section_testbench_time_100us = 5u;
    assert(section_task_slice_elapsed() == 1u);
    task_slice_advance();
    assert(task_last_switch_tick == 5u);
    /* Exercise the real list scan and completion path, not a test scheduler. */
    memset(&a, 0, sizeof(a));
    a.t_period = 10u;
    a.p_func   = callback;
    section_item_t item = {.p_obj = &a, .p_next = NULL};
    p_task_first                 = &item;
    section_testbench_time_100us = 25u;
    section_task_tick();
    assert(    a.time_last == 25u
            && task_stack_pick_next() == &a);
    (void)puts(
        "PASS: global window/remainder, unified FIFO, no inline bypass, busy deadlines, bare-metal parity and tick wrap");
    return 0;
}
