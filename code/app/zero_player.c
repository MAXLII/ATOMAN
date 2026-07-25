// SPDX-License-Identifier: MIT
/**
 * @file    zero_player.c
 * @brief   Zero-player cellular automaton implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Evolve a wrapping 32x64 Conway cellular-automaton grid
 *          - Re-seed stable or oscillating states with deterministic pseudo-random cells
 *          - Publish the current grid through a read-only pointer interface
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Grid evolution runs from task context
 *          - Display and transport concerns remain outside this module
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

#include <stdint.h>
#include <string.h>

static int last_grid[ROWS][COLS] = {0}; /* Previous generation for oscillator detection. */
static int grid[ROWS][COLS] = {0}; /* Current generation exposed to display consumers. */
static int next_grid[ROWS][COLS] = {0}; /* Next generation under construction. */
static const zero_player_grid_t grid_view = { /* Read-only grid descriptor shared with display consumers. */
    .rows = (uint32_t)ROWS,
    .columns = (uint32_t)COLS,
    .p_cells = (const int (*)[COLS])grid,
};

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

const zero_player_grid_t *zero_player_grid_get(void)
{
    return &grid_view;
}

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
        zero_player_add();
    }
}

REG_TASK_MS(1000U, zero_player_step)

void zero_player_add(void)
{
    uint32_t system_time = SECTION_SYS_TICK; /* Current 100 us platform time. */
    uint32_t random_seed = system_time ^ 0xA5A5A5A5U; /* LCG state. */
    uint32_t row = 0U; /* Grid row being populated. */
    uint32_t column = 0U; /* Grid column being populated. */

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
}

static void zero_player_start(void)
{
    zero_player_add();
}

REG_INIT(2, zero_player_start)

static void timing_add_seed(void)
{
    zero_player_add();
}

REG_TASK_MS(10U * 60U * 1000U, timing_add_seed)
