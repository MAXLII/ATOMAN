// SPDX-License-Identifier: MIT
/**
 * @file    zero_player.h
 * @brief   Zero-player cellular automaton interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define the fixed cellular-automaton grid dimensions
 *          - Expose grid initialization and random population APIs
 *          - Expose the current grid through a read-only pointer interface
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Grid consumers read the data without taking ownership
 *          - Hardware access remains outside the application module
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

#ifndef ZERO_PLAYER_H
#define ZERO_PLAYER_H

#include <stdint.h>

#define ROWS 32
#define COLS 64

typedef struct
{
    uint32_t rows; /* Number of valid grid rows. */
    uint32_t columns; /* Number of valid cells in each row. */
    const int (*p_cells)[COLS]; /* Read-only cells addressed as p_cells[row][column]. */
} zero_player_grid_t;

void zero_player_init(const int init[ROWS][COLS]);
void zero_player_step(void);
void zero_player_add(void);
const zero_player_grid_t *zero_player_grid_get(void);

#endif /* ZERO_PLAYER_H */
