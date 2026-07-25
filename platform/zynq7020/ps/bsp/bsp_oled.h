// SPDX-License-Identifier: MIT
/**
 * @file    bsp_oled.h
 * @brief   Zynq-7020 PL OLED framebuffer DMA interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Expose the 128x64 non-cacheable DDR framebuffer
 *          - Hide OLED DMA registers, synchronization, and recovery
 *          - Provide task-context display and diagnostic controls
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Normal display updates are polling based
 *          - Only PL OLED error events use the shared GIC
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

#ifndef BSP_OLED_H
#define BSP_OLED_H

#include <stdint.h>

#define BSP_OLED_WIDTH 128U
#define BSP_OLED_HEIGHT 64U
#define BSP_OLED_FRAME_BYTES 1024U

typedef struct
{
    uint32_t version;             /* PL OLED DMA RTL version. */
    uint32_t status;              /* Current hardware status register. */
    uint32_t framebuffer_base;    /* Active DDR framebuffer address. */
    uint32_t frame_count;         /* Successfully transmitted frame count. */
    uint32_t clear_count;         /* Successfully transmitted clear count. */
    uint32_t axi_error_count;     /* AXI read error count. */
    uint32_t command_error_count; /* Configuration, command, and protocol errors. */
    uint32_t dma_stop_reason;     /* Latched DMA stop reason. */
    uint32_t irq_status;          /* Current hardware IRQ status. */
    uint32_t error_irq_count;     /* Number of error ISR entries. */
    uint32_t error_irq_latched;   /* Accumulated error bits observed by the ISR. */
} bsp_oled_status_t;

int32_t bsp_oled_init(void);
void bsp_oled_frame_clear(void);
void bsp_oled_pixel_set(uint32_t x, uint32_t y, uint8_t enabled);
void bsp_oled_present(void);
int32_t bsp_oled_display_clear(void);
int32_t bsp_oled_display_enable(uint8_t enabled);
int32_t bsp_oled_invert_set(uint8_t enabled);
int32_t bsp_oled_contrast_set(uint8_t contrast);
int32_t bsp_oled_auto_refresh_set(uint8_t enabled, uint32_t period_ms);
void bsp_oled_status_get(bsp_oled_status_t *p_status);
void bsp_oled_error_clear(void);
int32_t bsp_oled_reset(void);

#endif /* BSP_OLED_H */
