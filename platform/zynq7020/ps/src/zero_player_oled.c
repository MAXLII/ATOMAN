// SPDX-License-Identifier: MIT
/**
 * @file    zero_player_oled.c
 * @brief   Zynq-7020 zero-player OLED renderer.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Fetch the current 32x64 zero-player grid through its data interface
 *          - Scale the complete game grid across the full OLED area
 *          - Present one coherent framebuffer per game generation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Rendering runs in task context
 *          - OLED transport remains inside the platform BSP
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

#include "bsp_oled.h"
#include "section.h"

#include <stdint.h>

static void zero_player_oled_refresh(void)
{
    const zero_player_grid_t *p_grid = zero_player_grid_get(); /* Grid dimensions and read-only cell storage. */
    uint32_t row = 0U; /* Source grid row. */
    uint32_t column = 0U; /* Source grid column. */
    uint32_t pixel_y = 0U; /* Scaled OLED pixel row. */
    uint32_t pixel_x = 0U; /* Scaled OLED pixel column. */
    uint32_t pixel_y_begin = 0U; /* Inclusive top edge of one scaled cell. */
    uint32_t pixel_y_end = 0U; /* Exclusive bottom edge of one scaled cell. */
    uint32_t pixel_x_begin = 0U; /* Inclusive left edge of one scaled cell. */
    uint32_t pixel_x_end = 0U; /* Exclusive right edge of one scaled cell. */

    if (p_grid == NULL)
    {
        return;
    }
    if (p_grid->p_cells == NULL)
    {
        return;
    }
    if (p_grid->rows == 0U)
    {
        return;
    }
    if (p_grid->columns == 0U)
    {
        return;
    }

    bsp_oled_frame_clear();
    for (row = 0U; row < p_grid->rows; ++row)
    {
        pixel_y_begin = (row * BSP_OLED_HEIGHT) / p_grid->rows;
        pixel_y_end = ((row + 1U) * BSP_OLED_HEIGHT) / p_grid->rows;
        for (column = 0U; column < p_grid->columns; ++column)
        {
            if (p_grid->p_cells[row][column] == 0)
            {
                continue;
            }
            pixel_x_begin = (column * BSP_OLED_WIDTH) / p_grid->columns;
            pixel_x_end = ((column + 1U) * BSP_OLED_WIDTH) / p_grid->columns;
            for (pixel_y = pixel_y_begin; pixel_y < pixel_y_end; ++pixel_y)
            {
                for (pixel_x = pixel_x_begin; pixel_x < pixel_x_end; ++pixel_x)
                {
                    bsp_oled_pixel_set(pixel_x, pixel_y, 1U);
                }
            }
        }
    }
    bsp_oled_present();
}

REG_TASK_MS(1000U, zero_player_oled_refresh)
