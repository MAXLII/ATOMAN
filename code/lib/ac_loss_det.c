// SPDX-License-Identifier: MIT
/**
 * @file    ac_loss_det.c
 * @brief   ac_loss_det library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Detect AC input loss from sampled voltage history
 *          - Track zero-crossing and voltage-difference conditions
 *          - Expose reset and periodic detection routines for protection logic
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
#include "ac_loss_det.h"
#include "string.h"
#include "my_math.h"

void ac_loss_det_init(ac_loss_det_t *p_str,
                      float *p_v,
                      uint8_t *p_ac_is_ok)
{
    p_str->input.p_v = p_v;
    p_str->input.p_ac_is_ok = p_ac_is_ok;
    memset(p_str->inter.buffer, 0, sizeof(p_str->inter.buffer));
    p_str->inter.buffer_size = ARRAY_SIZE(p_str->inter.buffer);
    p_str->inter.sta = AC_LOSS_DET_STA_IDLE;
    p_str->inter.buffer_index = 0;
    p_str->inter.ovf_diff_cnt = 0;
    p_str->output.is_loss = 1;
}

void ac_loss_det_reset(ac_loss_det_t *p_str)
{
    p_str->inter.sta = AC_LOSS_DET_STA_IDLE;
    p_str->inter.buffer_index = 0;
    p_str->inter.ovf_diff_cnt = 0;
    p_str->output.is_loss = 1;
}

static uint8_t ac_loss_det_record_buffer_func(ac_loss_det_t *p_str)
{
    if (p_str->inter.buffer_index / p_str->inter.buffer_size)
    {
        p_str->inter.buffer_index = 0;
        return 1;
    }

    p_str->inter.buffer[p_str->inter.buffer_index] = *p_str->input.p_v;
    p_str->inter.buffer_index++;

    return 0;
}

static uint8_t ac_loss_det_diff_func(ac_loss_det_t *p_str)
{
    if (p_str->inter.buffer_index / p_str->inter.buffer_size)
    {
        p_str->inter.buffer_index = 0;
        return 1;
    }

    if (fabsf(p_str->inter.buffer[p_str->inter.buffer_index] - (*p_str->input.p_v)) > AC_LOSS_DET_DIFF)
    {
        p_str->inter.ovf_diff_cnt++;
        if (p_str->inter.ovf_diff_cnt > AC_LOSS_DET_DIFF_OVF_TIME_CNT)
        {
            p_str->inter.buffer_index = 0;
            return 1;
        }
    }
    else
    {
        DN_CNT(p_str->inter.ovf_diff_cnt);
    }

    p_str->inter.diff_volt = fabsf(p_str->inter.buffer[p_str->inter.buffer_index] - (*p_str->input.p_v));

    p_str->inter.buffer[p_str->inter.buffer_index] = *p_str->input.p_v;
    p_str->inter.buffer_index++;

    return 0;
}

void ac_loss_det_func(ac_loss_det_t *p_str)
{
    switch (p_str->inter.sta)
    {
    case AC_LOSS_DET_STA_IDLE:
        p_str->output.is_loss = 1;
        p_str->inter.buffer_index = 0;
        if ((*p_str->input.p_v > AC_LOSS_DET_ZERO_VOLT_POS) &&
            (*p_str->input.p_ac_is_ok == 1))
        {
            p_str->inter.sta = AC_LOSS_DET_STA_WAIT_NEG;
        }
        break;

    case AC_LOSS_DET_STA_WAIT_NEG:
        if (*p_str->input.p_v < AC_LOSS_DET_ZERO_VOLT_NEG)
        {
            p_str->inter.sta = AC_LOSS_DET_STA_WAIT_POS;
        }
        break;
    case AC_LOSS_DET_STA_WAIT_POS:
        if (*p_str->input.p_v > AC_LOSS_DET_ZERO_VOLT_POS)
        {
            p_str->inter.sta = AC_LOSS_DET_STA_FIRST_LOOP_POS;
        }
        break;

    case AC_LOSS_DET_STA_FIRST_LOOP_POS:
        if (ac_loss_det_record_buffer_func(p_str))
        {
            p_str->output.is_loss = 1;
            ac_loss_det_reset(p_str);
        }
        else
        {
            if (*p_str->input.p_v < AC_LOSS_DET_ZERO_VOLT_NEG)
            {
                p_str->inter.sta = AC_LOSS_DET_STA_FIRST_LOOP_NEG;
            }
        }
        break;
    case AC_LOSS_DET_STA_FIRST_LOOP_NEG:
        if (ac_loss_det_record_buffer_func(p_str))
        {
            p_str->output.is_loss = 1;
            ac_loss_det_reset(p_str);
        }
        else
        {
            if (*p_str->input.p_v > AC_LOSS_DET_ZERO_VOLT_POS)
            {
                p_str->inter.buffer_index = 0;
                p_str->output.is_loss = 0;
                p_str->inter.sta = AC_LOSS_DET_STA_DET_POS;
            }
        }
        break;
    case AC_LOSS_DET_STA_DET_POS:
        if (ac_loss_det_diff_func(p_str))
        {
            p_str->output.is_loss = 1;
            ac_loss_det_reset(p_str);
        }
        else
        {
            if (*p_str->input.p_v < AC_LOSS_DET_ZERO_VOLT_NEG)
            {
                p_str->inter.sta = AC_LOSS_DET_STA_DET_NEG;
            }
        }
        break;
    case AC_LOSS_DET_STA_DET_NEG:
        if (ac_loss_det_diff_func(p_str))
        {
            p_str->output.is_loss = 1;
            ac_loss_det_reset(p_str);
        }
        else
        {
            if (*p_str->input.p_v > AC_LOSS_DET_ZERO_VOLT_POS)
            {
                p_str->inter.buffer_index = 0;
                p_str->inter.sta = AC_LOSS_DET_STA_DET_POS;
            }
        }
        break;
    }
}
