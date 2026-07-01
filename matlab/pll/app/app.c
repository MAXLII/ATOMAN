// SPDX-License-Identifier: MIT
/**
 * @file    app.c
 * @brief   MATLAB PLL application module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Read three-phase voltage samples from the MATLAB S-Function input vector
 *          - Convert abc voltages to alpha-beta components using amplitude-invariant Clarke transform
 *          - Execute the reusable PLL library and publish omega and theta to MATLAB outputs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-07-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "app.h"

#include "my_math.h"
#include "pll.h"
#include "section.h"
#include "sim_sfunc.h"
#include "timing.h"

#define APP_PLL_TS (200.0e-6f)
#define APP_PLL_ISR_PERIOD_TICKS (50U)
#define APP_PLL_OMEGA_CENTER_RADPS (M_2PI * 50.0f)
#define APP_PLL_OMEGA_UP_LMT_RADPS (M_2PI * 65.0f)
#define APP_PLL_OMEGA_DN_LMT_RADPS (M_2PI * 35.0f)
#define APP_PLL_VM (34500.0f * M_SQRT2)
#define APP_PLL_ZETA (0.707f)
#define APP_PLL_OMEGA_N_RADPS (M_2PI * 50.0f)
#define APP_PLL_PI_UP_LMT_RADPS (M_2PI * 15.0f)
#define APP_PLL_PI_DN_LMT_RADPS (-M_2PI * 15.0f)
#define APP_PLL_ONE_OVER_SQRT3 (0.57735026918962576451f)

static pll_t app_pll = {0};
static float app_pll_alpha = 0.0f;
static float app_pll_beta = 0.0f;
static uint8_t app_pll_inited = 0U;
static uint16_t app_pll_isr_tick = 0U;

static void app_pll_clarke_amp_inv(float va,
                                   float vb,
                                   float vc,
                                   float *p_alpha,
                                   float *p_beta)
{
    *p_alpha = (2.0f / 3.0f) * (va - (0.5f * vb) - (0.5f * vc));
    *p_beta = APP_PLL_ONE_OVER_SQRT3 * (vb - vc);
}

static void app_pll_init_once(void)
{
    if (app_pll_inited != 0U)
    {
        return;
    }

    if (pll_init(&app_pll,
                 APP_PLL_TS,
                 APP_PLL_OMEGA_CENTER_RADPS,
                 APP_PLL_OMEGA_UP_LMT_RADPS,
                 APP_PLL_OMEGA_DN_LMT_RADPS,
                 APP_PLL_VM,
                 APP_PLL_ZETA,
                 APP_PLL_OMEGA_N_RADPS,
                 APP_PLL_PI_UP_LMT_RADPS,
                 APP_PLL_PI_DN_LMT_RADPS,
                 0.0f,
                 &app_pll_alpha,
                 &app_pll_beta))
    {
        app_pll_inited = 1U;
    }
}

static void app_pll_publish_outputs(void)
{
    sim_set_output(SIM_OUTPUT_OMEGA, app_pll.output.omega);              /* 1 */
    sim_set_output(SIM_OUTPUT_THETA, app_pll.output.theta);              /* 2 */
    sim_set_output(SIM_OUTPUT_ALPHA, app_pll.inter.alpha);               /* 3 */
    sim_set_output(SIM_OUTPUT_BETA, app_pll.inter.beta);                 /* 4 */
    sim_set_output(SIM_OUTPUT_VD, app_pll.inter.vd);                     /* 5 */
    sim_set_output(SIM_OUTPUT_VQ, app_pll.inter.vq);                     /* 6 */
    sim_set_output(SIM_OUTPUT_VF, app_pll.inter.vf);                     /* 7 */
    sim_set_output(SIM_OUTPUT_PI_REF, app_pll.inter.pi_ref);             /* 8 */
    sim_set_output(SIM_OUTPUT_PI_ACT, app_pll.inter.pi_act);             /* 9 */
    sim_set_output(SIM_OUTPUT_TS, app_pll.cfg.ts);                       /* 10 */
    sim_set_output(SIM_OUTPUT_OMEGA_CENTER, app_pll.cfg.omega_center);   /* 11 */
    sim_set_output(SIM_OUTPUT_OMEGA_UP_LMT, app_pll.cfg.omega_up_lmt);   /* 12 */
    sim_set_output(SIM_OUTPUT_OMEGA_DN_LMT, app_pll.cfg.omega_dn_lmt);   /* 13 */
    sim_set_output(SIM_OUTPUT_VM, app_pll.cfg.vm);                       /* 14 */
    sim_set_output(SIM_OUTPUT_ZETA, app_pll.cfg.zeta);                   /* 15 */
    sim_set_output(SIM_OUTPUT_OMEGA_N, app_pll.cfg.omega_n);             /* 16 */
    sim_set_output(SIM_OUTPUT_PI_KP, app_pll.cfg.pi_kp);                 /* 17 */
    sim_set_output(SIM_OUTPUT_PI_KI, app_pll.cfg.pi_ki);                 /* 18 */
    sim_set_output(SIM_OUTPUT_PI_UP_LMT, app_pll.pi.inter.up_lmt);       /* 19 */
    sim_set_output(SIM_OUTPUT_PI_DN_LMT, app_pll.pi.inter.dn_lmt);       /* 20 */
    sim_set_output(SIM_OUTPUT_PI_B0, app_pll.pi.inter.b0);               /* 21 */
    sim_set_output(SIM_OUTPUT_PI_B1, app_pll.pi.inter.b1);               /* 22 */
}

static void app_pll_isr(void)
{
    float va = 0.0f;
    float vb = 0.0f;
    float vc = 0.0f;

    app_pll_init_once();
    if (app_pll_inited == 0U)
    {
        return;
    }

    app_pll_isr_tick++;
    if (app_pll_isr_tick < APP_PLL_ISR_PERIOD_TICKS)
    {
        return;
    }
    app_pll_isr_tick = 0U;

    va = sim_get_input(SIM_INPUT_V_A);
    vb = sim_get_input(SIM_INPUT_V_B);
    vc = sim_get_input(SIM_INPUT_V_C);

    app_pll_clarke_amp_inv(va, vb, vc, &app_pll_alpha, &app_pll_beta);

    if (pll_cal(&app_pll))
    {
        app_pll_publish_outputs();
    }
}

REG_INTERRUPT(0, app_pll_isr)
