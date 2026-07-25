// SPDX-License-Identifier: MIT
/**
 * @file    zero_player_oled.c
 * @brief   Zynq-7020 zero-player OLED renderer.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Map the 30x31 zero-player grid to the 128x64 OLED
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

#include <stdint.h>

void zero_player_display(int grid[ROWS][COLS])
{
    uint32_t row = 0U; /* Source grid row. */
    uint32_t column = 0U; /* Source grid column. */
    uint32_t pixel_y = 0U; /* Scaled OLED pixel row. */
    uint32_t pixel_x = 0U; /* Scaled OLED pixel column. */
    uint32_t pixel_y_begin = 0U; /* Inclusive top edge of one scaled cell. */
    uint32_t pixel_y_end = 0U; /* Exclusive bottom edge of one scaled cell. */
    uint32_t pixel_x_begin = 0U; /* Inclusive left edge of one scaled cell. */
    uint32_t pixel_x_end = 0U; /* Exclusive right edge of one scaled cell. */

    bsp_oled_frame_clear();
    for (row = 0U; row < (uint32_t)ROWS; ++row)
    {
        pixel_y_begin = (row * BSP_OLED_HEIGHT) / (uint32_t)ROWS;
        pixel_y_end = ((row + 1U) * BSP_OLED_HEIGHT) / (uint32_t)ROWS;
        for (column = 0U; column < (uint32_t)COLS; ++column)
        {
            if (grid[row][column] == 0)
            {
                continue;
            }
            pixel_x_begin = (column * BSP_OLED_WIDTH) / (uint32_t)COLS;
            pixel_x_end = ((column + 1U) * BSP_OLED_WIDTH) / (uint32_t)COLS;
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
