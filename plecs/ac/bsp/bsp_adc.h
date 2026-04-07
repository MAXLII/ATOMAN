#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "plecs.h"

#define BSP_ADC_V_AC_IN plecs_get_input(PLECS_INPUT_V_G)
#define BSP_ADC_V_AC_OUT plecs_get_input(PLECS_INPUT_V_OUT)
#define BSP_ADC_V_AC_CAP plecs_get_input(PLECS_INPUT_V_CAP)
#define BSP_ADC_I_L plecs_get_input(PLECS_INPUT_I_L)
#define BSP_ADC_V_BUS plecs_get_input(PLECS_INPUT_V_BUS)
#define BSP_ADC_I_AC_OUT plecs_get_input(PLECS_INPUT_I_OUT)

#define BSP_ADC_V_G BSP_ADC_V_AC_IN
#define BSP_ADC_V_OUT BSP_ADC_V_AC_OUT
#define BSP_ADC_V_CAP BSP_ADC_V_AC_CAP
#define BSP_ADC_I_OUT BSP_ADC_I_AC_OUT

#endif
