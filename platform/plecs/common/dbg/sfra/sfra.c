// SPDX-License-Identifier: MIT
/**
 * @file    sfra.c
 * @brief   SFRA Section adapter implementation.
 * @details
 *          This file is part of the base PLECS platform project.
 *
 *          Module responsibilities:
 *          - Discover SECTION_SFRA registrations between Windows linker sentinels
 *          - Assign stable SFRA identifiers used by the service layer
 *          - Forward existing APIs to the portable SFRA core
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Windows linker-section access is isolated to this PLECS adapter
 *          - Protocol handling belongs to sfra_service.c
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "sfra.h"

#include <stddef.h>

section_item_t *p_sfra_first = NULL;
static section_item_t *p_sfra_tail = NULL;
REG_DBG_LIST(sfra, p_sfra_first)

void sfra_init_list(void)
{
    uint8_t id = 0u;
    const reg_section_t *p_section_first = NULL;
    const reg_section_t *p_section_last = NULL;

    p_sfra_first = NULL;
    p_sfra_tail = NULL;

#if defined(SECTION_SENTINEL_REG_SECTION)
    extern const reg_section_t section_reg_start;
    extern const reg_section_t section_reg_stop;
    p_section_first = &section_reg_start + 1;
    p_section_last = &section_reg_stop;
#else
    p_section_first = (const reg_section_t *)&SECTION_START;
    p_section_last = (const reg_section_t *)&SECTION_STOP;
#endif

    for (const reg_section_t *p_section = p_section_first;
         p_section < p_section_last;
         ++p_section)
    {
        if (p_section->section_type == SECTION_SFRA)
        {
            section_item_t *p_item = (section_item_t *)p_section->p_str;
            sfra_registration_t *p_registration = NULL;

            if ((p_item == NULL) || (p_item->p_obj == NULL))
            {
                continue;
            }

            p_registration = (sfra_registration_t *)p_item->p_obj;
            if (p_registration->p_sfra == NULL)
            {
                continue;
            }
            p_registration->sfra_id = id++;
            p_item->p_next = NULL;
            if (p_sfra_first == NULL)
            {
                p_sfra_first = p_item;
            }
            else
            {
                p_sfra_tail->p_next = p_item;
            }
            p_sfra_tail = p_item;
        }
    }
}

sfra_status_t sfra_init(sfra_t *p_sfra,
                        float *p_inject,
                        float *p_collect,
                        float isr_freq_hz,
                        float inject_amplitude,
                        float freq_start_hz,
                        float freq_step_mul)
{
    return sfra_core_init(p_sfra,
                          p_inject,
                          p_collect,
                          isr_freq_hz,
                          inject_amplitude,
                          freq_start_hz,
                          freq_step_mul);
}

sfra_status_t sfra_start(sfra_t *p_sfra)
{
    return sfra_core_start(p_sfra);
}

sfra_status_t sfra_stop(sfra_t *p_sfra)
{
    return sfra_core_stop(p_sfra);
}

sfra_status_t sfra_reset(sfra_t *p_sfra)
{
    return sfra_core_reset(p_sfra);
}

sfra_status_t sfra_set_sweep_range(sfra_t *p_sfra,
                                   float freq_start_hz,
                                   float freq_end_hz)
{
    return sfra_core_set_sweep_range(p_sfra, freq_start_hz, freq_end_hz);
}

sfra_status_t sfra_set_inject_delay(sfra_t *p_sfra, uint16_t inject_delay_tick)
{
    return sfra_core_set_inject_delay(p_sfra, inject_delay_tick);
}

void sfra_isr_pre_sample(sfra_t *p_sfra)
{
    sfra_core_isr_pre_sample(p_sfra);
}

void sfra_isr_post_sample(sfra_t *p_sfra)
{
    sfra_core_isr_post_sample(p_sfra);
}

sfra_status_t sfra_task(sfra_t *p_sfra)
{
    return sfra_core_task(p_sfra);
}

REG_INIT(0, sfra_init_list)
