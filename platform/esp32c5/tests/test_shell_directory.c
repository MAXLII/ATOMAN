// SPDX-License-Identifier: MIT
/**
 * @file test_shell_directory.c
 * @brief Regression for command entries without variable/limit storage.
 * @details Part of base. Compile the actual service implementation; capture its
 *          output at comm_send_data so NULL command fields are exercised on host.
 * @author Max.Li
 * @date 2026-10-21
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License; see LICENSE in the repository root.
 */
#include "shell_service.c"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

section_item_t *p_shell_first; /**< Test-owned Shell list head. */
static uint32_t packets;       /**< Number of actual service responses. */
static shell_report_list_t entries[2]; /**< Captured directory payloads. */

uint32_t shell_count_get(void)
{
    return 2u;
}
void shell_item_print(section_shell_t *p_item, DEC_MY_PRINTF)
{
    (void)p_item;
    (void)my_printf;
    assert(false);
}
section_shell_t *shell_find(const char *p_name, uint8_t length)
{
    (void)p_name;
    (void)length;
    assert(false);
    return NULL;
}
void comm_send_data(void *p_context, DEC_MY_PRINTF)
{
    section_packform_t *p_pack = p_context; /* Recover actual service packet at the capture boundary. */
    (void)my_printf;

    if (p_pack->cmd_word == CMD_WORD_SHELL_DATA_NUM)
    {
        assert(p_pack->is_ack == 1u);
    }
    else
    {
        assert(    packets >= 1u
                && packets <= 2u);
        assert(p_pack->cmd_word == CMD_WORD_SHELL_REPORT_LIST);
        assert(p_pack->is_ack == 0u);
        memcpy(&entries[packets - 1u], p_pack->p_data, p_pack->len);
    }
    ++packets;
}

/** @return 0 if the real report task serializes both commands and variables. */
int main(void)
{
    uint32_t value   = 123u;  /* Variable backing storage. */
    uint32_t maximum = 1000u; /* Variable upper limit. */
    uint32_t minimum = 0u;    /* Variable lower limit. */
    section_shell_t command = {.p_name = "cmd", .p_name_size = 3u, .type = SHELL_CMD}; /* NULL data pointers. */
    section_shell_t variable = {.p_name      = "value",
                                .p_name_size = 5u,
                                .p_var       = &value,
                                .type        = SHELL_UINT32,
                                .p_max       = &maximum,
                                .p_min       = &minimum}; /* Normal neighboring entry. */
    section_item_t second = {.p_obj = &variable, .p_next = NULL};      /* List tail. */
    section_item_t first = {.p_obj = &command, .p_next = &second};     /* Command deliberately first. */
    section_packform_t request = {.sop = 0xe8u, .src = 1u, .dst = 2u}; /* Real directory request metadata. */
    p_shell_first = &first;
    shell_data_num_act(&request, NULL);
    shell_data_report_act();
    shell_data_report_act();
    shell_data_report_act();
    assert(packets == 3u);
    assert(entries[0].type == SHELL_CMD);
    assert(    entries[0].data == 0u
            && entries[0].data_min == 0u
            && entries[0].data_max == 0u);
    assert(    entries[1].type == SHELL_UINT32
            && entries[1].data == value);
    assert(entries[1].data_max == maximum);
    assert(shell_report_ctx.active == 0u);
    puts("PASS actual Shell directory command NULL storage and adjacent variable payload");
    return 0;
}
