// SPDX-License-Identifier: MIT
/**
 * @file    zero_player.c
 * @brief   Zero-player cellular automaton implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Evolve a wrapping 30x31 Conway cellular-automaton grid
 *          - Re-seed stable or oscillating states with deterministic pseudo-random cells
 *          - Stream the grid to Shell and a platform-provided display hook
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Grid evolution and display run from task context
 *          - The weak display hook keeps the game hardware-independent
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "zero_player.h"

#include "section.h"
#include "shell.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int last_grid[ROWS][COLS] = {0}; /* Previous generation for oscillator detection. */
static int grid[ROWS][COLS] = {0}; /* Current generation rendered to the user. */
static int next_grid[ROWS][COLS] = {0}; /* Next generation under construction. */
static uint8_t print_state = 0U; /* Shell print state: 0 idle, 1 streaming, 2 complete. */
static uint32_t print_row = 0U; /* Current Shell output row. */
static uint32_t print_column = 0U; /* Current Shell output column. */
static section_link_tx_func_t *p_print_link = NULL; /* Shell link owning the active print. */

void zero_player_init(const int init[ROWS][COLS])
{
    if (init == NULL)
    {
        return;
    }

    (void)memcpy(grid, init, sizeof(grid));
    (void)memset(last_grid, 0, sizeof(last_grid));
    (void)memset(next_grid, 0, sizeof(next_grid));
}

__attribute__((weak)) void zero_player_display(int display_grid[ROWS][COLS])
{
    (void)display_grid;
}

static void zero_player_print(DEC_MY_PRINTF)
{
    uint32_t column = 0U; /* Header separator column. */

    if ((print_state != 0U) ||
        (my_printf == NULL) ||
        (my_printf->my_printf == NULL))
    {
        return;
    }

    p_print_link = my_printf;
    print_state = 1U;
    print_row = 0U;
    print_column = 0U;
    for (column = 0U; column < (uint32_t)COLS; ++column)
    {
        my_printf->my_printf("--");
    }
    my_printf->my_printf("\r\n");
}

static void zero_player_print_step(void)
{
    if ((print_state != 1U) ||
        (p_print_link == NULL) ||
        (p_print_link->my_printf == NULL))
    {
        return;
    }

    p_print_link->my_printf("%c ",
                            (grid[print_row][print_column] != 0) ? '*' : ' ');
    print_column = (print_column + 1U) % (uint32_t)COLS;
    if (print_column == 0U)
    {
        p_print_link->my_printf("\r\n");
        print_row++;
    }

    if (print_row >= (uint32_t)ROWS)
    {
        print_state = 2U;
    }
}

REG_TASK_MS(1U, zero_player_print_step)

REG_SHELL_CMD(zero_player_print, zero_player_print)

static int count_neighbors(int row, int column)
{
    int count = 0; /* Number of live cells in the wrapped 3x3 neighborhood. */
    int row_delta = 0; /* Neighbor row displacement. */
    int column_delta = 0; /* Neighbor column displacement. */

    for (row_delta = -1; row_delta <= 1; ++row_delta)
    {
        for (column_delta = -1; column_delta <= 1; ++column_delta)
        {
            int neighbor_row = 0; /* Wrapped neighbor row. */
            int neighbor_column = 0; /* Wrapped neighbor column. */

            if ((row_delta == 0) &&
                (column_delta == 0))
            {
                continue;
            }
            neighbor_row = (row + row_delta + ROWS) % ROWS;
            neighbor_column = (column + column_delta + COLS) % COLS;
            count += grid[neighbor_row][neighbor_column];
        }
    }
    return count;
}

void zero_player_step(void)
{
    uint8_t equal_to_current = 1U; /* Next generation equals the current generation. */
    uint8_t equal_to_previous = 1U; /* Next generation forms a period-2 oscillator. */
    int row = 0; /* Current generation row. */
    int column = 0; /* Current generation column. */

    if (print_state == 1U)
    {
        return;
    }
    print_state = 0U;
    zero_player_display(grid);

    for (row = 0; row < ROWS; ++row)
    {
        for (column = 0; column < COLS; ++column)
        {
            int neighbors = count_neighbors(row, column); /* Live neighbor count. */

            if (grid[row][column] != 0)
            {
                next_grid[row][column] =
                    ((neighbors == 2) ||
                     (neighbors == 3)) ? 1 : 0;
            }
            else
            {
                next_grid[row][column] = (neighbors == 3) ? 1 : 0;
            }

            if (next_grid[row][column] != last_grid[row][column])
            {
                equal_to_previous = 0U;
            }

            if (grid[row][column] != next_grid[row][column])
            {
                equal_to_current = 0U;
            }
        }
    }
    (void)memcpy(last_grid, grid, sizeof(grid));
    (void)memcpy(grid, next_grid, sizeof(grid));
    if ((equal_to_previous == 1U) ||
        (equal_to_current == 1U))
    {
        zero_player_add(p_print_link);
    }
}

REG_TASK_MS(1000U, zero_player_step)

void zero_player_add(DEC_MY_PRINTF)
{
    uint32_t system_time = SECTION_SYS_TICK; /* Current 100 us platform time. */
    uint32_t random_seed = system_time ^ 0xA5A5A5A5U; /* LCG state. */
    uint32_t row = 0U; /* Grid row being populated. */
    uint32_t column = 0U; /* Grid column being populated. */

    if ((my_printf != NULL) &&
        (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("system_time = %u\r\n", system_time);
    }

    for (row = 0U; row < (uint32_t)ROWS; ++row)
    {
        for (column = 0U; column < (uint32_t)COLS; ++column)
        {
            if (grid[row][column] == 0)
            {
                random_seed = (random_seed * 1664525U) +
                              1013904223U +
                              (row * 73U) +
                              (column * 37U);
                grid[row][column] =
                    (((random_seed >> 16U) & 0xFFU) < 51U) ? 1 : 0;
            }
        }
    }
    if ((my_printf != NULL) &&
        (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("zero_player_add: random fill done\r\n");
    }
}

REG_SHELL_CMD(zero_player_add, zero_player_add)

static void zero_player_start(void)
{
    zero_player_add(NULL);
}

REG_INIT(2, zero_player_start)

static void timing_add_seed(void)
{
    zero_player_add(NULL);
}

REG_TASK_MS(10U * 60U * 1000U, timing_add_seed)
