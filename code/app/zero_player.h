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
 *          - Provide a platform display hook for each generation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Display implementations run from task context
 *          - Hardware access is supplied by the platform display hook
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

#include "section.h"

#define ROWS 32
#define COLS 64

void zero_player_init(const int init[ROWS][COLS]);
void zero_player_step(void);
void zero_player_add(DEC_MY_PRINTF);
void zero_player_display(int grid[ROWS][COLS]);

#endif /* ZERO_PLAYER_H */
