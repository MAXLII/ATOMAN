// SPDX-License-Identifier: MIT
/**
 * @file    bb_ctrl.c
 * @brief   bb_ctrl control module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement buck-boost voltage, current, power-limit, and open-loop control paths
 *          - Prepare and reset PI controllers and slew-limited references before entering run state
 *          - Consume HAL measurements and setpoints to generate PWM compare outputs without allocation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "bb_ctrl.h"
#include "bb_cfg.h"
#include "bb_mode.h"
#include "bb_ol.h"
#include "pi_tustin.h"
#include "section.h"
#include "my_math.h"

#define p_hal (bb_hal_get_ctrl())

static pi_tustin_t out_volt_loop = {0};    /* out_volt_loop: outer output-voltage controller */
static pi_tustin_t in_volt_lmt_loop = {0}; /* in_volt_lmt_loop: input-voltage limiting controller */
static pi_tustin_t in_curr_loop = {0};     /* in_curr_loop: input-current limiting controller */
static pi_tustin_t out_curr_loop = {0};    /* out_curr_loop: output-current limiting controller */
static pi_tustin_t ind_curr_loop = {0};    /* ind_curr_loop: inner inductor-current controller */

static float out_volt_loop_lmt = 0.0f;         /* out_volt_loop_lmt: active upper clamp for the outer voltage loop */
static float in_curr_lmt = 0.0f;               /* in_curr_lmt: runtime input-current limit after power derating */
static float ind_curr_ref = 0.0f;              /* ind_curr_ref: inductor-current reference driven by outer loops */
static float bb_pwm_ts = CTRL_TS;              /* bb_pwm_ts: PWM period used by open-loop DCM duty calculation */
static float bb_open_loop_l = 5.8e-6f;         /* bb_open_loop_l: open-loop equivalent inductance placeholder */
static bb_mode_t bb_mode = {0};                /* bb_mode: CCM modulation solver fed by inductor-voltage command */
static bb_ol_mode_e ol_mode = BB_OL_MODE_BUCK; /* ol_mode: DCM/open-loop mode state with hysteresis */
static bb_ol_t bb_ol = {0};                    /* bb_ol: DCM open-loop duty generator */
static uint8_t bb_ctrl_is_dcm = 0U;            /* bb_ctrl_is_dcm: latched DCM/CCM operating region flag */
static uint8_t bb_ctrl_run_active = 0U;        /* bb_ctrl_run_active: latched run gate for safe PWM shutdown */

#define BB_CTRL_DCM_ENTER_PWR_W (30.0f)
#define BB_CTRL_DCM_EXIT_PWR_W (50.0f)

/**
 * @brief Update the DCM state with input-power hysteresis.
 * @param pwr_in_w Instantaneous input power in watts.
 * @return None. Internal DCM state is updated in-place.
 */
static void bb_ctrl_is_in_dcm(float pwr_in_w)
{
    /* Apply input-power hysteresis to avoid DCM/CCM chatter around the threshold. */
    if (bb_ctrl_is_dcm == 0U)
    {
        if (pwr_in_w < BB_CTRL_DCM_ENTER_PWR_W)
        {
            bb_ctrl_is_dcm = 1U;
        }
    }
    else
    {
        if (pwr_in_w > BB_CTRL_DCM_EXIT_PWR_W)
        {
            bb_ctrl_is_dcm = 0U;
        }
    }
}

/**
 * @brief Update open-loop mode selection from the current voltage gain.
 * @param None. Input voltage and output voltage are read from HAL.
 * @return None. The internal DCM/open-loop mode state is updated in-place.
 */
static void bb_ctrl_cal_ol_mode(void)
{
    float gain = 0.0f;  /* gain: instantaneous buck-boost voltage gain */
    float v_in = 0.0f;  /* v_in: sampled input voltage */
    float v_out = 0.0f; /* v_out: sampled output voltage */

    if ((p_hal == NULL) ||
        (p_hal->p_v_in == NULL) ||
        (p_hal->p_v_out == NULL))
    {
        return;
    }

    v_in = *p_hal->p_v_in;
    v_out = *p_hal->p_v_out;
    DN_LMT(v_in, 0.001f);
    gain = v_out / v_in;

    switch (ol_mode)
    {
    case BB_OL_MODE_BOOST:
        if (gain < BB_CTRL_BOOST_TO_BUCK_BOOST_THR)
        {
            ol_mode = BB_OL_MODE_BUCK_BOOST;
        }
        break;

    case BB_OL_MODE_BUCK:
        if (gain > BB_CTRL_BUCK_TO_BUCK_BOOST_THR)
        {
            ol_mode = BB_OL_MODE_BUCK_BOOST;
        }
        break;

    case BB_OL_MODE_BUCK_BOOST:
        if (gain > BB_CTRL_BUCK_BOOST_TO_BOOST_THR)
        {
            ol_mode = BB_OL_MODE_BOOST;
        }
        else if (gain < BB_CTRL_BUCK_BOOST_TO_BUCK_THR)
        {
            ol_mode = BB_OL_MODE_BUCK;
        }
        break;

    default:
        ol_mode = BB_OL_MODE_BUCK;
        break;
    }
}

/**
 * @brief Reinitialize all controller states, loop objects, and helper modules.
 * @param None. Active setpoint and HAL pointers are read internally.
 * @return None. If dependencies are incomplete, the function exits without changes.
 */
static void bb_ctrl_reinit_states(void)
{
    bb_ctrl_setpoint_t *p_active_setpoint = bb_cfg_get_p_active(); /* p_active_setpoint: active controller setpoint image */

    if ((p_hal == NULL) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_in == NULL) ||
        (p_hal->p_i_in == NULL) ||
        (p_hal->p_v_out == NULL) ||
        (p_hal->p_i_out == NULL) ||
        (p_hal->p_i_l == NULL))
    {
        return;
    }

    bb_mode_init(&bb_mode,
                 &ind_curr_loop.output.val,
                 p_hal->p_v_in,
                 p_hal->p_v_out,
                 (*p_hal->p_v_in > *p_hal->p_v_out)
                     ? BB_MODE_BUCK
                     : BB_MODE_BOOST);

    ol_mode = (*p_hal->p_v_in > *p_hal->p_v_out)
                  ? BB_OL_MODE_BUCK
                  : BB_OL_MODE_BOOST;

    bb_ol_init(&bb_ol,
               &ol_mode,
               &ind_curr_ref,
               &bb_pwm_ts,
               p_hal->p_v_in,
               p_hal->p_v_out,
               &bb_open_loop_l);

    bb_ctrl_is_dcm = 0U;
    bb_ctrl_run_active = 0U;
    out_volt_loop_lmt = 0.0f;
    in_curr_lmt = p_active_setpoint->in_curr_lmt;

    pi_tustin_init(&out_volt_loop,
                   BB_CTRL_OUT_VOLT_LOOP_KP,
                   BB_CTRL_OUT_VOLT_LOOP_KI,
                   CTRL_TS,
                   BB_CTRL_OUT_VOLT_LOOP_UP_LMT,
                   BB_CTRL_OUT_VOLT_LOOP_DN_LMT,
                   &p_active_setpoint->out_volt_ref,
                   p_hal->p_v_out);

    pi_tustin_init(&in_volt_lmt_loop,
                   BB_CTRL_IN_VOLT_LMT_LOOP_KP,
                   BB_CTRL_IN_VOLT_LMT_LOOP_KI,
                   CTRL_TS,
                   BB_CTRL_IN_VOLT_LMT_LOOP_UP_LMT,
                   BB_CTRL_IN_VOLT_LMT_LOOP_DN_LMT,
                   p_hal->p_v_in,
                   &p_active_setpoint->in_volt_lmt);

    pi_tustin_init(&in_curr_loop,
                   BB_CTRL_IN_CURR_LOOP_KP,
                   BB_CTRL_IN_CURR_LOOP_KI,
                   CTRL_TS,
                   BB_CTRL_IN_CURR_LOOP_UP_LMT,
                   BB_CTRL_IN_CURR_LOOP_DN_LMT,
                   &in_curr_lmt,
                   p_hal->p_i_in);

    pi_tustin_init(&out_curr_loop,
                   BB_CTRL_OUT_CURR_LOOP_KP,
                   BB_CTRL_OUT_CURR_LOOP_KI,
                   CTRL_TS,
                   BB_CTRL_OUT_CURR_LOOP_UP_LMT,
                   BB_CTRL_OUT_CURR_LOOP_DN_LMT,
                   &p_active_setpoint->out_curr_lmt,
                   p_hal->p_i_out);

    pi_tustin_init(&ind_curr_loop,
                   BB_CTRL_IND_CURR_LOOP_KP,
                   BB_CTRL_IND_CURR_LOOP_KI,
                   CTRL_TS,
                   BB_CTRL_IND_CURR_LOOP_UP_LMT,
                   BB_CTRL_IND_CURR_LOOP_DN_LMT,
                   &ind_curr_ref,
                   p_hal->p_i_l);
}

/**
 * @brief Module initialization hook registered in the init section.
 * @param None.
 * @return None.
 */
static void bb_ctrl_init(void)
{
    bb_ctrl_reinit_states();
}

/**
 * @brief Main fast control interrupt for the buck-boost controller.
 * @param None. HAL samples and active setpoint are read internally.
 * @return None. PWM outputs are updated when run is allowed and dependencies are valid.
 */
static void bb_ctrl_isr(void)
{
    bb_ctrl_setpoint_t *p_active_setpoint = bb_cfg_get_p_active(); /* p_active_setpoint: active setpoint used in this ISR pass */

    if ((p_hal == NULL) ||
        (bb_cfg_is_ready() == 0U) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_in == NULL) ||
        (p_hal->p_i_in == NULL) ||
        (p_hal->p_v_out == NULL) ||
        (p_hal->p_i_out == NULL) ||
        (p_hal->p_i_l == NULL) ||
        (p_hal->p_set_pwm_func == NULL))
    {
        return;
    }

    bb_cfg_sync_building_to_active();

    const float v_in = *p_hal->p_v_in;   /* v_in: ISR input-voltage snapshot */
    const float i_in = *p_hal->p_i_in;   /* i_in: ISR input-current snapshot */
    const float v_out = *p_hal->p_v_out; /* v_out: ISR output-voltage snapshot */
    const float i_out = *p_hal->p_i_out; /* i_out: ISR output-current snapshot */
    const float i_l = *p_hal->p_i_l;     /* i_l: ISR inductor-current snapshot */
    const float pwr_in_w = v_in * i_in;  /* pwr_in_w: instantaneous input power estimate */

    (void)v_out;
    (void)i_out;
    (void)i_l;

    if (p_active_setpoint->run_allowed == 0U)
    {
        if (bb_ctrl_run_active != 0U)
        {
            if (p_hal->p_pwm_disable != NULL)
            {
                p_hal->p_pwm_disable();
            }
            bb_ctrl_run_active = 0U;
        }
        return;
    }

    bb_ctrl_run_active = 1U;

    bb_ctrl_is_in_dcm(pwr_in_w);
    bb_ctrl_cal_ol_mode();

    pi_tustin_cal(&in_volt_lmt_loop);
    pi_tustin_cal(&in_curr_loop);
    pi_tustin_cal(&out_curr_loop);

    /* The outer voltage loop is clamped by the tightest upstream limiter. */
    MIN(out_volt_loop_lmt,
        in_volt_lmt_loop.output.val,
        in_curr_loop.output.val);
    MIN(out_volt_loop_lmt,
        out_volt_loop_lmt,
        out_curr_loop.output.val);

    out_volt_loop.inter.up_lmt = out_volt_loop_lmt;

    pi_tustin_cal(&out_volt_loop);
    ind_curr_ref = out_volt_loop.output.val;

    if (bb_ctrl_is_dcm == 0U)
    {
        /* CCM path: close the inductor-current loop, then synthesize PWM duty. */
        pi_tustin_cal(&ind_curr_loop);
        bb_mode_func(&bb_mode);

        p_hal->p_set_pwm_func(bb_mode.output.buck_duty,
                              1U,
                              1U,
                              bb_mode.output.boost_duty,
                              1U,
                              1U);
    }
    else
    {
        /* DCM path: reset the current loop and use the open-loop duty generator. */
        if (out_volt_loop.output.val < 0.0f)
        {
            pi_tustin_reset(&out_volt_loop);
        }

        pi_tustin_reset(&ind_curr_loop);
        switch (ol_mode)
        {
        case BB_OL_MODE_BOOST:
            bb_mode.inter.mode = BB_MODE_BOOST;
            break;
        case BB_OL_MODE_BUCK_BOOST:
            bb_mode.inter.mode = BB_MODE_BUCK_BOOST;
            break;
        case BB_OL_MODE_BUCK:
        default:
            bb_mode.inter.mode = BB_MODE_BUCK;
            break;
        }

        bb_ol_func(&bb_ol);

        p_hal->p_set_pwm_func(bb_ol.output.buck_duty,
                              bb_ol.output.buck_up_en,
                              bb_ol.output.buck_dn_en,
                              bb_ol.output.boost_duty,
                              bb_ol.output.boost_up_en,
                              bb_ol.output.boost_dn_en);
    }
}

REG_INTERRUPT(3, bb_ctrl_isr)

/**
 * @brief Placeholder background task for future non-time-critical control work.
 * @param None.
 * @return None.
 */
static void bb_ctrl_task(void)
{
}

REG_TASK(1, bb_ctrl_task)

/**
 * @brief 1 ms task that converts power limit into an input-current limit.
 * @param None. Active setpoint and input voltage are read internally.
 * @return None. The runtime input-current limit reference is updated in-place.
 */
static void bb_ctrl_in_curr_lmt_task(void)
{
    bb_ctrl_setpoint_t *p_active_setpoint = bb_cfg_get_p_active(); /* p_active_setpoint: active setpoint sampled by 1 ms task */
    float vin_for_lmt = 0.0f;                                      /* vin_for_lmt: guarded input voltage used for power-to-current conversion */
    float pwr_to_curr_lmt = 0.0f;                                  /* pwr_to_curr_lmt: current limit derived from input power limit */

    if ((p_hal == NULL) ||
        (p_active_setpoint == NULL) ||
        (p_hal->p_v_in == NULL))
    {
        return;
    }

    vin_for_lmt = *p_hal->p_v_in;
    DN_LMT(vin_for_lmt, 0.001f);

    pwr_to_curr_lmt = p_active_setpoint->pwr_lmt / vin_for_lmt;
    MIN(in_curr_lmt, pwr_to_curr_lmt, p_active_setpoint->in_curr_lmt);
}

REG_TASK_MS(1, bb_ctrl_in_curr_lmt_task)

/**
 * @brief Bind the controller HAL used by ISR and helper tasks.
 * @param p Pointer to the controller HAL. Passing NULL detaches all runtime IO.
 * @return None.
 */
void bb_ctrl_set_p_hal(bb_ctrl_hal_t *p)
{
    (void)p;
}

/**
 * @brief Prepare the controller for a new run entry.
 * @param None.
 * @return None. Internal loop states and helper modules are reinitialized.
 */
void bb_ctrl_prepare_run(void)
{
    bb_ctrl_reinit_states();
}
