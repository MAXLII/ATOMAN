// SPDX-License-Identifier: MIT
/**
 * @file    shell.c
 * @brief   Shell Section adapter implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover SECTION_SHELL registrations and build the compatibility list
 *          - Adapt section_item_t traversal to the pure shell_core list view
 *          - Preserve the existing Shell runtime API for platform link handlers
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Linker-section access is isolated to this file
 *          - Protocol handling belongs to shell_service.c
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "shell.h"

#include <stddef.h>

section_item_t *p_shell_first = NULL;
static section_item_t *p_shell_tail = NULL;
REG_DBG_LIST(shell, p_shell_first)

static void *shell_list_first(void *p_context, void **pp_cursor)
{
    section_item_t *p_item = *(section_item_t **)p_context;

    *pp_cursor = p_item;
    return (p_item == NULL) ? NULL : p_item->p_obj;
}

static void *shell_list_next(void *p_context, void **pp_cursor)
{
    section_item_t *p_item = (section_item_t *)*pp_cursor;

    (void)p_context;
    p_item = (p_item == NULL) ? NULL : p_item->p_next;
    *pp_cursor = p_item;
    return (p_item == NULL) ? NULL : p_item->p_obj;
}

static const shell_core_list_t shell_list = {
    .p_context = &p_shell_first,
    .p_first = shell_list_first,
    .p_next = shell_list_next,
};

void shell_init(void)
{
    p_shell_first = NULL;
    p_shell_tail = NULL;

    for (reg_section_t *p_section = (reg_section_t *)&SECTION_START;
         p_section < (reg_section_t *)&SECTION_STOP;
         ++p_section)
    {
        if (p_section->section_type == SECTION_SHELL)
        {
            section_item_t *p_item = (section_item_t *)p_section->p_str;

            if ((p_item == NULL) || (p_item->p_obj == NULL))
            {
                continue;
            }

            p_item->p_next = NULL;
            if (p_shell_first == NULL)
            {
                p_shell_first = p_item;
            }
            else
            {
                p_shell_tail->p_next = p_item;
            }
            p_shell_tail = p_item;
        }
    }
}

void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx)
{
    shell_core_run(&shell_list, data, (shell_core_io_t *)my_printf, p_ctx);
}

void shell_item_print(section_shell_t *p_item, DEC_MY_PRINTF)
{
    shell_core_item_print(p_item, (shell_core_io_t *)my_printf);
}

uint32_t shell_count_get(void)
{
    return shell_core_count_get(&shell_list);
}

section_shell_t *shell_find(const char *p_name, uint8_t len)
{
    return shell_core_find(&shell_list, p_name, len);
}

static void shell_time(DEC_MY_PRINTF)
{
    if ((my_printf == NULL) || (my_printf->my_printf == NULL))
    {
        return;
    }

    my_printf->my_printf("time = %us.%03ums\r\n",
                         (unsigned)(SECTION_SYS_TICK / 10000u),
                         (unsigned)((SECTION_SYS_TICK % 10000u) / 10u));
}

static void shell_reset(DEC_MY_PRINTF)
{
    (void)my_printf;
    SYSTEM_RESET;
}

REG_INIT(0, shell_init)
REG_SHELL_CMD(time, shell_time)
REG_SHELL_CMD(reset, shell_reset)
