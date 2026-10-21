// SPDX-License-Identifier: MIT
/**
 * @file bsp_timer.c
 * @brief Fixed board interrupt driven by a hardware auto-reload timer.
 * @details Part of base.
 *          Entity: one board timer and an application-provided ISR callback.
 *          Prior: callback consumers are initialized before enabling the timer.
 *          Time: 32 us hardware reload; never replay missed samples in a task.
 * @author Max.Li
 * @date 2026-10-22
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License; see LICENSE.
 */
#include "bsp_timer.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_cpu.h"
#include <stddef.h>

#define BSP_TIMER_RESOLUTION_HZ 1000000u /**< Requested counter ticks per second. */
#define BSP_TIMER_PERIOD_TICKS  32u      /**< Hardware reload interval: 32 us. */

static gptimer_handle_t p_timer;     /**< SDK timer owned for the firmware lifetime. */
static void (*p_interrupt)(void);    /**< Published before the hardware interrupt starts. */
static uint32_t interrupt_frequency; /**< Actual configured Hz, task-owned after startup. */

/**
 * @param timer SDK timer invoking this callback; unused.
 * @param p_event Captured alarm data; unused because reload is handled by hardware.
 * @param p_context SDK callback context; unused.
 * @return false; this callback does not wake or create SDK tasks.
 */
volatile bsp_timer_debug_t g_bsp_timer_debug; /**< Timer-only timing observations; never transmitted by the BSP. */

static bool alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *p_event, void *p_context)
{
    uint32_t now     = esp_cpu_get_cycle_count();           /**< ISR entry timestamp at the configured 240 MHz CPU clock. */
    uint32_t elapsed = now - g_bsp_timer_debug.cycle_stamp; /**< Unsigned subtraction handles one counter wrap. */

    if (g_bsp_timer_debug.interrupts != 0u)
    {
        if (elapsed > g_bsp_timer_debug.max_gap_cycles)
            g_bsp_timer_debug.max_gap_cycles = elapsed;

        if (elapsed > 11520u) /* 1.5 periods at 240 MHz and 31250 Hz. */
            ++g_bsp_timer_debug.overdue;
    }
    g_bsp_timer_debug.cycle_stamp = now;
    ++g_bsp_timer_debug.interrupts;
    (void)timer;
    (void)p_event;
    (void)p_context;
    p_interrupt();
    uint32_t cost = esp_cpu_get_cycle_count() - now; /**< Dispatch execution cost, excluding inter-interrupt idle time. */
    g_bsp_timer_debug.busy_cycles += cost;

    if (cost > g_bsp_timer_debug.max_body_cycles)
        g_bsp_timer_debug.max_body_cycles = cost;
    return false;
}

void bsp_timer_init(void (*p_callback)(void))
{
    ESP_ERROR_CHECK(p_callback == NULL ? ESP_ERR_INVALID_ARG : ESP_OK);
    ESP_ERROR_CHECK(p_timer != NULL ? ESP_ERR_INVALID_STATE : ESP_OK);
    const gptimer_config_t config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = BSP_TIMER_RESOLUTION_HZ,
        .intr_priority = 1,
    }; /**< Fixed board configuration, independent of communication settings. */
    const gptimer_event_callbacks_t callbacks = {.on_alarm = alarm_callback}; /**< ISR entry only. */
    const gptimer_alarm_config_t alarm = {
        .alarm_count                = BSP_TIMER_PERIOD_TICKS,
        .reload_count               = 0u,
        .flags.auto_reload_on_alarm = 1u,
    }; /**< Hardware owns the periodic counter reload. */
    uint32_t resolution = 0u; /**< Actual counter clock reported by the SDK. */

    ESP_ERROR_CHECK(gptimer_new_timer(&config, &p_timer));
    ESP_ERROR_CHECK(gptimer_get_resolution(p_timer, &resolution));
    ESP_ERROR_CHECK(resolution % BSP_TIMER_PERIOD_TICKS == 0u ? ESP_OK : ESP_ERR_INVALID_STATE);
    p_interrupt = p_callback;
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(p_timer, &callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(p_timer, &alarm));
    ESP_ERROR_CHECK(gptimer_enable(p_timer));
    ESP_ERROR_CHECK(gptimer_start(p_timer));
    interrupt_frequency = resolution / BSP_TIMER_PERIOD_TICKS;
}

uint32_t bsp_timer_frequency_get(void)
{
    return interrupt_frequency;
}
