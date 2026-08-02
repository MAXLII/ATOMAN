// SPDX-License-Identifier: MIT
/**
 * @file    scope.c
 * @brief   Scope Section adapter implementation.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Discover SECTION_SCOPE registrations and build the compatibility list
 *          - Assign stable Scope identifiers used by the service layer
 *          - Forward capture operations to the portable Scope core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Linker-section access is isolated to this file
 *          - Protocol handling belongs to scope_service.c
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
#include "scope.h"

#include <stddef.h>

section_item_t *p_scope_first = NULL;
static section_item_t *p_scope_tail = NULL;
REG_DBG_LIST(scope, p_scope_first)

void scope_init(void)
{
    uint8_t id = 0u;

    p_scope_first = NULL;
    p_scope_tail = NULL;

    for (reg_section_t *p_section = (reg_section_t *)&SECTION_START;
         p_section < (reg_section_t *)&SECTION_STOP;
         ++p_section)
    {
        if (p_section->section_type == SECTION_SCOPE)
        {
            section_item_t *p_item = (section_item_t *)p_section->p_str;
            scope_registration_t *p_registration = NULL;

            if ((p_item == NULL) || (p_item->p_obj == NULL))
            {
                continue;
            }

            p_registration = (scope_registration_t *)p_item->p_obj;
            if (p_registration->p_scope == NULL)
            {
                continue;
            }
            p_registration->scope_id = id++;
            p_item->p_next = NULL;
            if (p_scope_first == NULL)
            {
                p_scope_first = p_item;
            }
            else
            {
                p_scope_tail->p_next = p_item;
            }
            p_scope_tail = p_item;
        }
    }
}

void scope_run(scope_t *p_scope)
{
    scope_core_run(p_scope);
}

void scope_start(scope_t *p_scope)
{
    scope_core_start(p_scope);
}

void scope_stop(scope_t *p_scope)
{
    scope_core_stop(p_scope);
}

void scope_trigger(scope_t *p_scope)
{
    scope_core_trigger(p_scope);
}

void scope_reset(scope_t *p_scope)
{
    scope_core_reset(p_scope);
}

REG_INIT(0, scope_init)
