// SPDX-License-Identifier: MIT
/**
 * @file    test_section_list.c
 * @brief   Host tests for section registration list primitives.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Verify front, back, and ordered-position insertion behavior
 *          - Verify removal and move-to-front head and tail maintenance
 *          - Verify wrapper-to-business-object access and indexed lookup
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Runs on the MinGW host toolchain
 *          - Does not access hardware
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "section_list.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void require_true(int condition, const char *p_message)
{
    if (condition == 0)
    {
        (void)fprintf(stderr, "FAIL: %s\n", p_message);
        exit(EXIT_FAILURE);
    }
}

int main(void)
{
    uint32_t values[3] = {10u, 20u, 30u};
    section_item_t first = {.p_obj = &values[0], .p_next = NULL};
    section_item_t second = {.p_obj = &values[1], .p_next = NULL};
    section_item_t third = {.p_obj = &values[2], .p_next = NULL};
    section_list_t list;

    section_list_init(&list);
    section_list_push_back(&list, &first);
    section_list_push_back(&list, &third);
    section_list_insert_after(&list, &first, &second);
    require_true(list.count == 3u, "insert count");
    require_true(section_list_at(&list, 1u) == &second, "indexed lookup");
    require_true(*SECTION_LIST_OBJ(&second, uint32_t) == 20u, "business object unwrap");
    require_true(list.p_tail == &third, "tail after insert");

    section_list_move_to_front(&list, &third);
    require_true((list.p_first == &third) && (list.p_tail == &second), "move to front");
    section_list_remove(&list, &first);
    require_true((list.count == 2u) && (first.p_next == NULL), "remove middle");
    section_list_push_back(&list, &third);
    require_true(list.count == 2u, "duplicate insertion ignored");

    (void)puts("section_list tests passed");
    return EXIT_SUCCESS;
}
