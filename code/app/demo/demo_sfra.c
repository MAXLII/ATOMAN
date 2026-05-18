// SPDX-License-Identifier: MIT
/**
 * @file    demo_sfra.c
 * @brief   SFRA demo.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Register one SFRA object
 *          - Initialize one PI controller as the measured object
 *          - Run SFRA sampling and background processing tasks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-18
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "pi_tustin.h"
#include "section.h"
#include "sfra.h"

static float s_demo_sfra_ref = 0.0f;
static float s_demo_sfra_fbk = 0.0f;
static pi_tustin_t s_demo_sfra_pi;

static void demo_sfra_prepare_freq(void *p_ctx);

REG_SFRA(demo_sfra,
         0U,
         0.0001f,
         1.0f,
         10.0f,
         5000.0f,
         demo_sfra_prepare_freq,
         &s_demo_sfra_pi)

static void demo_sfra_prepare_freq(void *p_ctx)
{
    pi_tustin_t *p_pi = (pi_tustin_t *)p_ctx;

    if (p_pi != NULL)
    {
        pi_tustin_reset(p_pi);
    }

    demo_sfra_collect = 0.0f;
}

static void demo_sfra_init(void)
{
    (void)pi_tustin_init(&s_demo_sfra_pi,
                         1.0f,
                         400.0f,
                         0.0001f,
                         100.0f,
                         -100.0f,
                         &s_demo_sfra_ref,
                         &s_demo_sfra_fbk);
}

static void demo_sfra_isr_10k_task(void)
{
    sfra_isr_pre_sample(&demo_sfra);

    s_demo_sfra_ref = demo_sfra_inject;
    s_demo_sfra_fbk = 0.0f;
    (void)pi_tustin_cal(&s_demo_sfra_pi);
    demo_sfra_collect = s_demo_sfra_pi.output.val;

    sfra_isr_post_sample(&demo_sfra);
}

static void demo_sfra_task_1ms(void)
{
    (void)sfra_task(&demo_sfra);
}

REG_INIT(5, demo_sfra_init)
REG_TASK(1, demo_sfra_isr_10k_task)
REG_TASK_MS(1, demo_sfra_task_1ms)
