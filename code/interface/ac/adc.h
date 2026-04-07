#ifndef __ADC_H
#define __ADC_H

#include <stdint.h>
#include "bsp_adc.h"

static inline float adc_get_v_g(void)
{
    return BSP_ADC_V_G;
}

static inline float adc_get_v_ac_in(void)
{
    return BSP_ADC_V_AC_IN;
}

static inline float adc_get_v_cap(void)
{
    return BSP_ADC_V_CAP;
}

static inline float adc_get_v_ac_cap(void)
{
    return BSP_ADC_V_AC_CAP;
}

static inline float adc_get_i_l(void)
{
    return BSP_ADC_I_L;
}

static inline float adc_get_v_bus(void)
{
    return BSP_ADC_V_BUS;
}

static inline float adc_get_i_out(void)
{
    return BSP_ADC_I_OUT;
}

static inline float adc_get_i_ac_out(void)
{
    return BSP_ADC_I_AC_OUT;
}

static inline float adc_get_v_out(void)
{
    return BSP_ADC_V_OUT;
}

static inline float adc_get_v_ac_out(void)
{
    return BSP_ADC_V_AC_OUT;
}

static inline uint8_t adc_get_sample_is_ok(void)
{
    return 1;
}

static inline void adc_clr_sample_is_ok(void)
{
    /* do nothing */
}

#endif
