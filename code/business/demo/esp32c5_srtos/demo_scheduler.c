// SPDX-License-Identifier: MIT
/**
 * @file demo_scheduler.c
 * @brief Application diagnostics for the SRTOS scheduler.
 * @details Entity: read-only Shell registrations. Prior: SRTOS debug counters exist.
 *          Time: normal application parameter queries.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "section.h"
#include "shell.h"
REG_SHELL_VAR(SRTOS_SWITCHES, g_section_scheduler_debug.task_switch_count, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_SAVES, g_section_scheduler_debug.context_save_count, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_RESTORES, g_section_scheduler_debug.context_restore_count, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_CONTEXT_PEAK, g_section_scheduler_debug.context_pool_high_water_words, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_STACK_PEAK, g_section_scheduler_debug.runtime_stack_peak_used_words, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_INVARIANTS, g_section_scheduler_debug.invariant_fail_count, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_FAULT, g_section_fault_debug.task_fault_reason, SHELL_UINT32, 0u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_CRITICAL_MAX, g_section_scheduler_debug.critical_max_cycles, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(SRTOS_CRITICAL_COUNT, g_section_scheduler_debug.critical_count, SHELL_UINT32, 0u, 0u, NULL,
              SHELL_STA_NULL)
