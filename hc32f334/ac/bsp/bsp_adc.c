#include "bsp_adc.h"
#include "hc32_ll_aos.h"
#include "section.h"
#include "shell.h"

const bsp_adc_param_t bsp_adc_param[ADC_TABLE_MAX] = {
    [BIL] = {.adc_name = BIL, .adc_periph = CM_ADC1, .adc_ch = ADC_CH0, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_00, .sample_time = SAMPLE_TIME},
    [F3VS] = {.adc_name = F3VS, .adc_periph = CM_ADC2, .adc_ch = ADC_CH1, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_01, .sample_time = SAMPLE_TIME},
    [R48VS] = {.adc_name = R48VS, .adc_periph = CM_ADC1, .adc_ch = ADC_CH2, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_02, .sample_time = SAMPLE_TIME},
    [VO] = {.adc_name = VO, .adc_periph = CM_ADC2, .adc_ch = ADC_CH5, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_05, .sample_time = SAMPLE_TIME},
    [VAUX] = {.adc_name = VAUX, .adc_periph = CM_ADC3, .adc_ch = ADC_CH3, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_03, .sample_time = SAMPLE_TIME},
    [TEMP3] = {.adc_name = TEMP3, .adc_periph = CM_ADC3, .adc_ch = ADC_CH6, .gpio_periph = GPIO_PORT_A, .pin = GPIO_PIN_06, .sample_time = SAMPLE_TIME},
    [ACCVS] = {.adc_name = ACCVS, .adc_periph = CM_ADC3, .adc_ch = ADC_CH8, .gpio_periph = GPIO_PORT_B, .pin = GPIO_PIN_00, .sample_time = SAMPLE_TIME},
    [RMTVS] = {.adc_name = RMTVS, .adc_periph = CM_ADC3, .adc_ch = ADC_CH9, .gpio_periph = GPIO_PORT_B, .pin = GPIO_PIN_01, .sample_time = SAMPLE_TIME},
    [TEMP1] = {.adc_name = TEMP1, .adc_periph = CM_ADC3, .adc_ch = ADC_CH10, .gpio_periph = GPIO_PORT_B, .pin = GPIO_PIN_02, .sample_time = SAMPLE_TIME},
    [TEMP2] = {.adc_name = TEMP2, .adc_periph = CM_ADC3, .adc_ch = ADC_CH11, .gpio_periph = GPIO_PORT_B, .pin = GPIO_PIN_10, .sample_time = SAMPLE_TIME},
};



void bsp_adc_init(void)
{
    stc_gpio_init_t stcGpioInit;

    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC1, ENABLE);
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC2, ENABLE);
    FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC3, ENABLE);

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);

    GPIO_StructInit(&stcGpioInit);

    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;

    for (uint8_t i = 0; i < ADC_TABLE_MAX; i++)
    {
        if ((i == TEMP1) || (i == TEMP2))
        {
            continue;
        }

        GPIO_Init(bsp_adc_param[i].gpio_periph, bsp_adc_param[i].pin, &stcGpioInit);
        ADC_ChCmd(bsp_adc_param[i].adc_periph,
                  ADC_SEQ_A,
                  bsp_adc_param[i].adc_ch,
                  ENABLE);
        ADC_SetSampleTime(bsp_adc_param[i].adc_periph,
                          bsp_adc_param[i].adc_ch,
                          bsp_adc_param[i].sample_time);
    }

    ADC_TriggerConfig(CM_ADC1, ADC_SEQ_A, ADC_HARDTRIG_EVT0);
    AOS_SetTriggerEventSrc(AOS_ADC1_0, ADC_TRIG_EVT);

    CM_ADC1->SYNCCR |= (1 << 0) | /* ADC同步启动使能 */
                       (2 << 4)   /*
                            SYNCMD[2]
                            0：单次触发
                            SYNCMD[1]
                            1：并行触发模式
                            SYNCMD[0]
                            0：ADC1和ADC2同步工作，ADC3独立工作
                            */
        ;

    ADC_TriggerCmd(CM_ADC1, ADC_SEQ_A, ENABLE);
}

REG_INIT(0, bsp_adc_init)

static void bsp_adc_trig_cycle_task(void)
{
    CM_ADC3->STR |= 1 << 0; /* 启动ADC3转换 */
}

REG_TASK_MS(1, bsp_adc_trig_cycle_task)

uint16_t BIL_VALUE = 0;
uint16_t F3VS_VALUE = 0;
uint16_t R48VS_VALUE = 0;
uint16_t VO_VALUE = 0;
uint16_t VAUX_VALUE = 0;
uint16_t TEMP3_VALUE = 0;
uint16_t ACCVS_VALUE = 0;
uint16_t RMTVS_VALUE = 0;
uint16_t TEMP1_VALUE = 0;
uint16_t TEMP2_VALUE = 0;

REG_SHELL_VAR(BIL_VALUE, BIL_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(F3VS_VALUE, F3VS_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(R48VS_VALUE, R48VS_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VO_VALUE, VO_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VAUX_VALUE, VAUX_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TEMP3_VALUE, TEMP3_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(ACCVS_VALUE, ACCVS_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RMTVS_VALUE, RMTVS_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TEMP1_VALUE, TEMP1_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(TEMP2_VALUE, TEMP2_VALUE, SHELL_UINT16, UINT16_MAX, 0U, NULL, SHELL_STA_NULL)

static void bsp_adc_scope(void)
{
    BIL_VALUE = BSP_ADC_BIL;
    F3VS_VALUE = BSP_ADC_F3VS;
    R48VS_VALUE = BSP_ADC_R48VS;
    VO_VALUE = BSP_ADC_VO;
    VAUX_VALUE = BSP_ADC_VAUX;
    TEMP3_VALUE = BSP_ADC_TEMP3;
    ACCVS_VALUE = BSP_ADC_ACCVS;
    RMTVS_VALUE = BSP_ADC_RMTVS;
    TEMP1_VALUE = 0U;
    TEMP2_VALUE = 0U;
}

REG_TASK_MS(1, bsp_adc_scope)
