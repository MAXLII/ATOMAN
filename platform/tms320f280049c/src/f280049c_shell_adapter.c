// SPDX-License-Identifier: MIT
/**
 * @file    f280049c_shell_adapter.c
 * @brief   F280049C SECTION Shell registry adapter.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Discover SECTION Shell registrations without the text-shell parser
 *          - Maintain the runtime Shell list used by FRAME parameter services
 *          - Provide bounded parameter count and name lookup operations
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Only binary FRAME parameter access is enabled on this UART
 *          - Protocol serialization is owned by frame_parameter_service.c
 *
 * @author  Max.Li
 * @date    2026-09-05
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "shell.h"

section_item_t *p_shell_first = NULL;
static section_item_t *s_shell_tail = NULL; /* Tail used while constructing the Shell list. */

REG_DBG_LIST(shell, p_shell_first)

void shell_init(void)
{
    reg_section_t *p_section; /* Current automatic registration record. */

    p_shell_first = NULL;
    s_shell_tail = NULL;

    for (p_section = (reg_section_t *)&SECTION_START;
         p_section < (reg_section_t *)&SECTION_STOP;
         p_section++)
    {
        if (p_section->section_type == (uint32_t)SECTION_SHELL)
        {
            section_item_t *p_item = (section_item_t *)p_section->p_str; /* Shell list wrapper. */

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
                s_shell_tail->p_next = p_item;
            }
            s_shell_tail = p_item;
        }
    }
}

uint32_t shell_count_get(void)
{
    const section_item_t *p_item = p_shell_first; /* Current registered Shell item. */
    uint32_t count = 0u;                          /* Number of valid Shell registrations. */

    while ((p_item != NULL) && (count < UINT32_MAX))
    {
        count++;
        p_item = p_item->p_next;
    }
    return count;
}

section_shell_t *shell_find(const char *p_name, uint8_t length)
{
    section_item_t *p_item = p_shell_first; /* Current registered Shell item. */

    if ((p_name == NULL) || (length == 0u) || (length > SHELL_STR_SIZE_MAX))
    {
        return NULL;
    }

    while (p_item != NULL)
    {
        section_shell_t *p_shell = (section_shell_t *)p_item->p_obj; /* Current parameter descriptor. */
        uint16_t index = 0u; /* Name character currently being compared. */
        uint8_t matches = 1u; /* Nonzero while all compared characters match. */

        if ((p_shell != NULL) && (p_shell->p_name_size == (uint32_t)length))
        {
            for (index = 0u; index < (uint16_t)length; index++)
            {
                if (((uint16_t)p_shell->p_name[index] & 0x00FFu) !=
                    ((uint16_t)p_name[index] & 0x00FFu))
                {
                    matches = 0u;
                    break;
                }
            }
            if (matches != 0u)
            {
                return p_shell;
            }
        }
        p_item = p_item->p_next;
    }
    return NULL;
}

REG_INIT(0, shell_init)
