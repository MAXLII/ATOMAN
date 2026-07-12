#ifndef __BPS_ADC_H
#define __BPS_ADC_H

#include "hc32_ll.h"
#include "hc32_ll_adc.h"

#define SAMPLE_TIME 16                      // 采样周期
#define ADC_TRIG_EVT EVT_SRC_HRPWM_3_SCMP_A // ADC触发事件

#define BSP_ADC_BIL (*((&bsp_adc_param[BIL].adc_periph->DR0) + bsp_adc_param[BIL].adc_ch))
#define BSP_ADC_F3VS (*((&bsp_adc_param[F3VS].adc_periph->DR0) + bsp_adc_param[F3VS].adc_ch))
#define BSP_ADC_R48VS (*((&bsp_adc_param[R48VS].adc_periph->DR0) + bsp_adc_param[R48VS].adc_ch))
#define BSP_ADC_VO (*((&bsp_adc_param[VO].adc_periph->DR0) + bsp_adc_param[VO].adc_ch))
#define BSP_ADC_VAUX (*((&bsp_adc_param[VAUX].adc_periph->DR0) + bsp_adc_param[VAUX].adc_ch))
#define BSP_ADC_TEMP3 (*((&bsp_adc_param[TEMP3].adc_periph->DR0) + bsp_adc_param[TEMP3].adc_ch))
#define BSP_ADC_ACCVS (*((&bsp_adc_param[ACCVS].adc_periph->DR0) + bsp_adc_param[ACCVS].adc_ch))
#define BSP_ADC_RMTVS (*((&bsp_adc_param[RMTVS].adc_periph->DR0) + bsp_adc_param[RMTVS].adc_ch))
#define BSP_ADC_TEMP1 (*((&bsp_adc_param[TEMP1].adc_periph->DR0) + bsp_adc_param[TEMP1].adc_ch))
#define BSP_ADC_TEMP2 (*((&bsp_adc_param[TEMP2].adc_periph->DR0) + bsp_adc_param[TEMP2].adc_ch))

typedef enum
{
    BIL,   // 电感电流
    F3VS,  // 单电芯电压
    R48VS, // 48V电压
    VO,    // 母线电压
    VAUX,  // 8V电压
    TEMP3, // 温度采样
    ACCVS, // 输入ACC驱动电压
    RMTVS, // 输出RMT补偿电压
    TEMP1, // 温度采样
    TEMP2, // 温度采样
    ADC_TABLE_MAX,
} ADC_TABLE_E;

typedef struct
{
    uint32_t adc_name;
    uint32_t gpio_periph;
    uint32_t pin;
    CM_ADC_TypeDef *adc_periph;
    uint8_t adc_ch;
    uint32_t sample_time;
} bsp_adc_param_t;

#endif
