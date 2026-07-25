// SPDX-License-Identifier: MIT
/**
 * @file    bsp_oled.c
 * @brief   Zynq-7020 PL OLED framebuffer DMA implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Maintain the page-organized framebuffer in non-cacheable DDR
 *          - Configure and synchronize with the PL OLED DMA peripheral
 *          - Record and clear error-only PL interrupt events
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - DDR and register ordering is enforced before PRESENT
 *          - ISR code never blocks, prints, or transfers framebuffer data
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

#include "bsp_oled.h"

#include "bsp_interrupt.h"
#include "bsp_timer.h"
#include "xil_io.h"
#include "xil_mmu.h"
#include "xstatus.h"

#include <stddef.h>
#include <stdint.h>

#define BSP_OLED_BASE 0x41220000U /* PL OLED DMA control base. */
#define BSP_OLED_FRAMEBUFFER_BASE 0x1FF20000U /* 1024-byte-aligned DDR frame. */
#define BSP_OLED_DMA_WINDOW_BASE 0x1FF00000U /* Shared 1 MiB non-cacheable window. */
#define BSP_OLED_IRQ_ID 62U /* IRQ_F2P[1] GIC SPI identifier. */

#define BSP_OLED_REG_CONTROL 0x00U
#define BSP_OLED_REG_STATUS 0x04U
#define BSP_OLED_REG_FB_BASE 0x08U
#define BSP_OLED_REG_SPI_DIV 0x0CU
#define BSP_OLED_REG_REFRESH_PERIOD 0x10U
#define BSP_OLED_REG_CONTRAST 0x14U
#define BSP_OLED_REG_IRQ_STATUS 0x18U
#define BSP_OLED_REG_IRQ_ENABLE 0x1CU
#define BSP_OLED_REG_FRAME_COUNT 0x20U
#define BSP_OLED_REG_CLEAR_COUNT 0x24U
#define BSP_OLED_REG_AXI_ERRORS 0x28U
#define BSP_OLED_REG_COMMAND_ERRORS 0x2CU
#define BSP_OLED_REG_DMA_STOP 0x30U
#define BSP_OLED_REG_VERSION 0x34U
#define BSP_OLED_REG_GEOMETRY 0x38U
#define BSP_OLED_REG_FRAME_BYTES 0x3CU

#define BSP_OLED_CONTROL_ENABLE 0x00000001U
#define BSP_OLED_CONTROL_SOFT_RESET 0x00000002U
#define BSP_OLED_CONTROL_REINIT 0x00000004U
#define BSP_OLED_CONTROL_PRESENT 0x00000008U
#define BSP_OLED_CONTROL_CLEAR 0x00000010U
#define BSP_OLED_CONTROL_DISPLAY_ON 0x00000020U
#define BSP_OLED_CONTROL_INVERT 0x00000040U
#define BSP_OLED_CONTROL_AUTO_REFRESH 0x00000080U
#define BSP_OLED_CONTROL_LEVEL_MASK 0x000000E1U

#define BSP_OLED_STATUS_INITIALIZED 0x00000002U
#define BSP_OLED_STATUS_DMA_BUSY 0x00000004U
#define BSP_OLED_STATUS_PROTOCOL_BUSY 0x00000008U
#define BSP_OLED_STATUS_COMMAND_PENDING 0x00000010U
#define BSP_OLED_STATUS_HALTED 0x00000100U
#define BSP_OLED_STATUS_ERROR 0x00000400U
#define BSP_OLED_STATUS_BUSY_MASK \
    (BSP_OLED_STATUS_DMA_BUSY | BSP_OLED_STATUS_PROTOCOL_BUSY | \
     BSP_OLED_STATUS_COMMAND_PENDING)

#define BSP_OLED_VERSION 0x00010000U
#define BSP_OLED_GEOMETRY 0x00400080U
#define BSP_OLED_DEFAULT_SPI_DIV 5U
#define BSP_OLED_DEFAULT_CONTRAST 0xCFU
#define BSP_OLED_DEFAULT_REFRESH_CYCLES 50000000U
#define BSP_OLED_IRQ_ERROR_MASK 0x0000000FU
#define BSP_OLED_TIMEOUT_TICKS 20000U /* Two-second command timeout. */
#define BSP_OLED_CLOCKS_PER_MS 50000U

static volatile uint32_t oled_error_irq_count = 0U; /* Error ISR entry count. */
static volatile uint32_t oled_error_irq_latched = 0U; /* Accumulated IRQ bits. */
static uint8_t oled_initialized = 0U; /* BSP and hardware initialization state. */

static uint32_t register_read(uint32_t offset)
{
    return Xil_In32(BSP_OLED_BASE + offset);
}

static void register_write(uint32_t offset, uint32_t value)
{
    Xil_Out32(BSP_OLED_BASE + offset, value);
}

static uint32_t control_levels_get(void)
{
    return register_read(BSP_OLED_REG_CONTROL) & BSP_OLED_CONTROL_LEVEL_MASK;
}

static int32_t wait_status(uint32_t mask, uint32_t expected)
{
    uint32_t start_tick = bsp_timer_gettime_100us(); /* Polling timeout origin. */

    while ((register_read(BSP_OLED_REG_STATUS) & mask) != expected)
    {
        if ((bsp_timer_gettime_100us() - start_tick) >= BSP_OLED_TIMEOUT_TICKS)
        {
            return XST_FAILURE;
        }
    }
    return XST_SUCCESS;
}

static int32_t wait_counter_change(uint32_t offset, uint32_t previous_value)
{
    uint32_t start_tick = bsp_timer_gettime_100us(); /* Polling timeout origin. */

    while (register_read(offset) == previous_value)
    {
        uint32_t status = register_read(BSP_OLED_REG_STATUS); /* Current error state. */

        if (((status & (BSP_OLED_STATUS_HALTED | BSP_OLED_STATUS_ERROR)) != 0U) ||
            ((bsp_timer_gettime_100us() - start_tick) >= BSP_OLED_TIMEOUT_TICKS))
        {
            return XST_FAILURE;
        }
    }
    return XST_SUCCESS;
}

static void oled_error_isr(void *p_callback)
{
    uint32_t irq_status = register_read(BSP_OLED_REG_IRQ_STATUS); /* Active error sources. */

    (void)p_callback;
    oled_error_irq_count++;
    oled_error_irq_latched |= irq_status;
    register_write(BSP_OLED_REG_IRQ_STATUS, irq_status);
}

int32_t bsp_oled_init(void)
{
    uint32_t control = 0U; /* Initial level control word. */
    int32_t status = XST_FAILURE; /* Shared GIC connection result. */

    if ((register_read(BSP_OLED_REG_VERSION) != BSP_OLED_VERSION) ||
        (register_read(BSP_OLED_REG_GEOMETRY) != BSP_OLED_GEOMETRY) ||
        (register_read(BSP_OLED_REG_FRAME_BYTES) != BSP_OLED_FRAME_BYTES))
    {
        return XST_FAILURE;
    }

    Xil_SetTlbAttributes((INTPTR)BSP_OLED_DMA_WINDOW_BASE, NORM_NONCACHE);
    bsp_oled_frame_clear();
    register_write(BSP_OLED_REG_FB_BASE, BSP_OLED_FRAMEBUFFER_BASE);
    register_write(BSP_OLED_REG_SPI_DIV, BSP_OLED_DEFAULT_SPI_DIV);
    register_write(BSP_OLED_REG_REFRESH_PERIOD, BSP_OLED_DEFAULT_REFRESH_CYCLES);
    register_write(BSP_OLED_REG_CONTRAST, BSP_OLED_DEFAULT_CONTRAST);
    register_write(BSP_OLED_REG_IRQ_STATUS, BSP_OLED_IRQ_ERROR_MASK);
    register_write(BSP_OLED_REG_IRQ_ENABLE, BSP_OLED_IRQ_ERROR_MASK);

    status = bsp_interrupt_connect(BSP_OLED_IRQ_ID, oled_error_isr, NULL);
    if (status != XST_SUCCESS)
    {
        return status;
    }
    bsp_interrupt_enable(BSP_OLED_IRQ_ID);

    control = BSP_OLED_CONTROL_ENABLE | BSP_OLED_CONTROL_DISPLAY_ON;
    register_write(BSP_OLED_REG_CONTROL, control);
    status = wait_status(BSP_OLED_STATUS_INITIALIZED,
                         BSP_OLED_STATUS_INITIALIZED);
    if (status != XST_SUCCESS)
    {
        return status;
    }

    oled_initialized = 1U;
    bsp_oled_present();
    return ((register_read(BSP_OLED_REG_STATUS) &
             (BSP_OLED_STATUS_HALTED | BSP_OLED_STATUS_ERROR)) == 0U) ?
               XST_SUCCESS :
               XST_FAILURE;
}

void bsp_oled_frame_clear(void)
{
    volatile uint8_t *p_framebuffer =
        (volatile uint8_t *)(UINTPTR)BSP_OLED_FRAMEBUFFER_BASE; /* DDR frame. */
    uint32_t index = 0U; /* Byte being cleared. */

    for (index = 0U; index < BSP_OLED_FRAME_BYTES; ++index)
    {
        p_framebuffer[index] = 0U;
    }
}

void bsp_oled_pixel_set(uint32_t x, uint32_t y, uint8_t enabled)
{
    volatile uint8_t *p_framebuffer =
        (volatile uint8_t *)(UINTPTR)BSP_OLED_FRAMEBUFFER_BASE; /* DDR frame. */
    uint32_t byte_index = 0U; /* Page-organized framebuffer byte index. */
    uint8_t bit_mask = 0U; /* Pixel bit within the selected display page. */

    if ((x >= BSP_OLED_WIDTH) ||
        (y >= BSP_OLED_HEIGHT))
    {
        return;
    }

    byte_index = ((y >> 3U) * BSP_OLED_WIDTH) + x;
    bit_mask = (uint8_t)(1U << (y & 0x07U));
    if (enabled == 1U)
    {
        p_framebuffer[byte_index] |= bit_mask;
    }
    else
    {
        p_framebuffer[byte_index] &= (uint8_t)(~bit_mask);
    }
}

void bsp_oled_present(void)
{
    uint32_t previous_count = 0U; /* Frame count before this request. */

    if (oled_initialized == 0U)
    {
        return;
    }
    if (wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U) != XST_SUCCESS)
    {
        return;
    }

    previous_count = register_read(BSP_OLED_REG_FRAME_COUNT);
    __asm__ volatile("dmb sy" ::: "memory");
    register_write(BSP_OLED_REG_CONTROL,
                   control_levels_get() | BSP_OLED_CONTROL_PRESENT);
    (void)wait_counter_change(BSP_OLED_REG_FRAME_COUNT, previous_count);
}

int32_t bsp_oled_display_clear(void)
{
    uint32_t previous_count = 0U; /* Clear count before this request. */

    if ((oled_initialized == 0U) ||
        (wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U) != XST_SUCCESS))
    {
        return XST_FAILURE;
    }
    previous_count = register_read(BSP_OLED_REG_CLEAR_COUNT);
    register_write(BSP_OLED_REG_CONTROL,
                   control_levels_get() | BSP_OLED_CONTROL_CLEAR);
    return wait_counter_change(BSP_OLED_REG_CLEAR_COUNT, previous_count);
}

int32_t bsp_oled_display_enable(uint8_t enabled)
{
    uint32_t control = control_levels_get(); /* Updated display level controls. */

    if (enabled == 1U)
    {
        control |= BSP_OLED_CONTROL_DISPLAY_ON;
    }
    else
    {
        control &= ~BSP_OLED_CONTROL_DISPLAY_ON;
    }
    register_write(BSP_OLED_REG_CONTROL, control);
    return wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U);
}

int32_t bsp_oled_invert_set(uint8_t enabled)
{
    uint32_t control = control_levels_get(); /* Updated inversion level controls. */

    if (enabled == 1U)
    {
        control |= BSP_OLED_CONTROL_INVERT;
    }
    else
    {
        control &= ~BSP_OLED_CONTROL_INVERT;
    }
    register_write(BSP_OLED_REG_CONTROL, control);
    return wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U);
}

int32_t bsp_oled_contrast_set(uint8_t contrast)
{
    register_write(BSP_OLED_REG_CONTRAST, contrast);
    return wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U);
}

int32_t bsp_oled_auto_refresh_set(uint8_t enabled, uint32_t period_ms)
{
    uint32_t control = control_levels_get(); /* Updated automatic-refresh controls. */
    uint32_t period_cycles = 0U; /* Requested refresh period in PL clock cycles. */

    if ((period_ms == 0U) ||
        (period_ms > (UINT32_MAX / BSP_OLED_CLOCKS_PER_MS)))
    {
        return XST_FAILURE;
    }
    period_cycles = period_ms * BSP_OLED_CLOCKS_PER_MS;
    register_write(BSP_OLED_REG_REFRESH_PERIOD, period_cycles);
    if (enabled == 1U)
    {
        control |= BSP_OLED_CONTROL_AUTO_REFRESH;
    }
    else
    {
        control &= ~BSP_OLED_CONTROL_AUTO_REFRESH;
    }
    register_write(BSP_OLED_REG_CONTROL, control);
    return (enabled == 0U) ?
               wait_status(BSP_OLED_STATUS_BUSY_MASK, 0U) :
               XST_SUCCESS;
}

void bsp_oled_status_get(bsp_oled_status_t *p_status)
{
    if (p_status == NULL)
    {
        return;
    }

    p_status->version = register_read(BSP_OLED_REG_VERSION);
    p_status->status = register_read(BSP_OLED_REG_STATUS);
    p_status->framebuffer_base = register_read(BSP_OLED_REG_FB_BASE);
    p_status->frame_count = register_read(BSP_OLED_REG_FRAME_COUNT);
    p_status->clear_count = register_read(BSP_OLED_REG_CLEAR_COUNT);
    p_status->axi_error_count = register_read(BSP_OLED_REG_AXI_ERRORS);
    p_status->command_error_count = register_read(BSP_OLED_REG_COMMAND_ERRORS);
    p_status->dma_stop_reason = register_read(BSP_OLED_REG_DMA_STOP);
    p_status->irq_status = register_read(BSP_OLED_REG_IRQ_STATUS);
    p_status->error_irq_count = oled_error_irq_count;
    p_status->error_irq_latched = oled_error_irq_latched;
}

void bsp_oled_error_clear(void)
{
    register_write(BSP_OLED_REG_IRQ_STATUS, BSP_OLED_IRQ_ERROR_MASK);
    oled_error_irq_count = 0U;
    oled_error_irq_latched = 0U;
}

int32_t bsp_oled_reset(void)
{
    uint32_t control = BSP_OLED_CONTROL_ENABLE |
                       BSP_OLED_CONTROL_DISPLAY_ON; /* Restored default levels. */

    oled_initialized = 0U;
    register_write(BSP_OLED_REG_CONTROL,
                   control_levels_get() | BSP_OLED_CONTROL_SOFT_RESET);
    register_write(BSP_OLED_REG_FB_BASE, BSP_OLED_FRAMEBUFFER_BASE);
    register_write(BSP_OLED_REG_SPI_DIV, BSP_OLED_DEFAULT_SPI_DIV);
    register_write(BSP_OLED_REG_REFRESH_PERIOD, BSP_OLED_DEFAULT_REFRESH_CYCLES);
    register_write(BSP_OLED_REG_CONTRAST, BSP_OLED_DEFAULT_CONTRAST);
    bsp_oled_error_clear();
    register_write(BSP_OLED_REG_CONTROL, control);
    if (wait_status(BSP_OLED_STATUS_INITIALIZED,
                    BSP_OLED_STATUS_INITIALIZED) != XST_SUCCESS)
    {
        return XST_FAILURE;
    }
    oled_initialized = 1U;
    bsp_oled_present();
    return XST_SUCCESS;
}
