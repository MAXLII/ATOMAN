/**
 *******************************************************************************
 * @file  hc32_ll_fcg.h
 * @brief This file contains all the functions prototypes of the FCG driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2026, Xiaohua Semiconductor Co., Ltd. All rights reserved.
 *
 * This software component is licensed by XHSC under BSD 3-Clause license
 * (the "License"); You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                    opensource.org/licenses/BSD-3-Clause
 *
 *******************************************************************************
 */
#ifndef __HC32_LL_FCG_H__
#define __HC32_LL_FCG_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"
/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_FCG
 * @{
 */

#if (LL_FCG_ENABLE == DDL_ON)
/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup FCG_Global_Macros FCG Global Macros
 * @{
 */
/**
 * @defgroup FCG_FCG0_Peripheral FCG FCG0 peripheral
 * @{
 */
#define FCG0_PERIPH_SRAMH               (PWC_FCG0_SRAMH)
#define FCG0_PERIPH_SRAM1               (PWC_FCG0_SRAM1)
#define FCG0_PERIPH_PLA                 (PWC_FCG0_PLA)
#define FCG0_PERIPH_DMA1                (PWC_FCG0_DMA1)
#define FCG0_PERIPH_DMA2                (PWC_FCG0_DMA2)
#define FCG0_PERIPH_FCM                 (PWC_FCG0_FCM)
#define FCG0_PERIPH_AOS                 (PWC_FCG0_AOS)
#define FCG0_PERIPH_CTC                 (PWC_FCG0_CTC)
#define FCG0_PERIPH_CORDIC              (PWC_FCG0_CORDIC)
#define FCG0_PERIPH_SKE                 (PWC_FCG0_SKE)
#define FCG0_PERIPH_HASH                (PWC_FCG0_HASH)
#define FCG0_PERIPH_TRNG                (PWC_FCG0_TRNG)
#define FCG0_PERIPH_CRC                 (PWC_FCG0_CRC)
#define FCG0_PERIPH_PID                 (PWC_FCG0_PID)
#define FCG0_PERIPH_PID_CMP             (PWC_FCG0_PID_CMP)
#define FCG0_PERIPH_XCMP                (PWC_FCG0_XCMP)
#define FCG0_PERIPH_SDFM                (PWC_FCG0_SDFM)
#define FCG0_PERIPH_DSOGI_PLL           (PWC_FCG0_DSOGI_PLL)
/**
 * @}
 */

/**
 * @defgroup FCG_FCG1_Peripheral FCG FCG1 peripheral
 * @{
 */
#define FCG1_PERIPH_I2C1                (PWC_FCG1_I2C1)
#define FCG1_PERIPH_I2C2                (PWC_FCG1_I2C2)
#define FCG1_PERIPH_I2C3                (PWC_FCG1_I2C3)
#define FCG1_PERIPH_SPI1                (PWC_FCG1_SPI1)
#define FCG1_PERIPH_SPI2                (PWC_FCG1_SPI2)
#define FCG1_PERIPH_SPI3                (PWC_FCG1_SPI3)
#define FCG1_PERIPH_FMAC                (PWC_FCG1_FMAC)
#define FCG1_PERIPH_MCAN1               (PWC_FCG1_MCAN1)
#define FCG1_PERIPH_MCAN2               (PWC_FCG1_MCAN2)
#define FCG1_PERIPH_MCAN3               (PWC_FCG1_MCAN3)
/**
 * @}
 */

/**
 * @defgroup FCG_FCG2_Peripheral FCG FCG2 peripheral
 * @{
 */
#define FCG2_PERIPH_TMR6_1              (PWC_FCG2_TMR6_1)
#define FCG2_PERIPH_TMR6_2              (PWC_FCG2_TMR6_2)
#define FCG2_PERIPH_TMR6_3              (PWC_FCG2_TMR6_3)
#define FCG2_PERIPH_TMR6_4              (PWC_FCG2_TMR6_4)
#define FCG2_PERIPH_TMR6_5              (PWC_FCG2_TMR6_5)
#define FCG2_PERIPH_TMR6_6              (PWC_FCG2_TMR6_6)
#define FCG2_PERIPH_TMR0_1              (PWC_FCG2_TMR0_1)
#define FCG2_PERIPH_TMR0_2              (PWC_FCG2_TMR0_2)
#define FCG2_PERIPH_EMB                 (PWC_FCG2_EMB)
#define FCG2_PERIPH_HRPWM_1             (PWC_FCG2_HRPWM_1)
#define FCG2_PERIPH_HRPWM_2             (PWC_FCG2_HRPWM_2)
#define FCG2_PERIPH_HRPWM_3             (PWC_FCG2_HRPWM_3)
#define FCG2_PERIPH_HRPWM_4             (PWC_FCG2_HRPWM_4)
#define FCG2_PERIPH_HRPWM_5             (PWC_FCG2_HRPWM_5)
#define FCG2_PERIPH_HRPWM_6             (PWC_FCG2_HRPWM_6)
#define FCG2_PERIPH_HRPWM_7             (PWC_FCG2_HRPWM_7)
#define FCG2_PERIPH_HRPWM_8             (PWC_FCG2_HRPWM_8)
#define FCG2_PERIPH_TRLPWM              (PWC_FCG2_TRLPWM)
#define FCG2_PERIPH_XBAR                (PWC_FCG2_XBAR)
/**
 * @}
 */

/**
 * @defgroup FCG_FCG3_Peripheral FCG FCG3 peripheral
 * @{
 */
#define FCG3_PERIPH_ADC1                (PWC_FCG3_ADC1)
#define FCG3_PERIPH_ADC2                (PWC_FCG3_ADC2)
#define FCG3_PERIPH_ADC3                (PWC_FCG3_ADC3)
#define FCG3_PERIPH_ADC4                (PWC_FCG3_ADC4)
#define FCG3_PERIPH_CMBIAS              (PWC_FCG3_CMBIAS)
#define FCG3_PERIPH_DAC1                (PWC_FCG3_DAC1)
#define FCG3_PERIPH_DAC2                (PWC_FCG3_DAC2)
#define FCG3_PERIPH_DAC3                (PWC_FCG3_DAC3)
#define FCG3_PERIPH_DAC4                (PWC_FCG3_DAC4)
#define FCG3_PERIPH_CMP1_2              (PWC_FCG3_CMP12)
#define FCG3_PERIPH_CMP3_4              (PWC_FCG3_CMP34)
#define FCG3_PERIPH_CMP5_6              (PWC_FCG3_CMP56)
#define FCG3_PERIPH_CMP7_8              (PWC_FCG3_CMP78)
#define FCG3_PERIPH_OTS                 (PWC_FCG3_OTS)
#define FCG3_PERIPH_VREF                (PWC_FCG3_VREF)
#define FCG3_PERIPH_USART1              (PWC_FCG3_USART1)
#define FCG3_PERIPH_USART2              (PWC_FCG3_USART2)
#define FCG3_PERIPH_USART3              (PWC_FCG3_USART3)
#define FCG3_PERIPH_USART4              (PWC_FCG3_USART4)
/**
 * @}
 */

/**
 * @defgroup FCG_FCGx_Peripheral_Mask FCG FCGx Peripheral Mask
 * @{
 */
#define FCG_FCG0_PERIPH_MASK            (0xE6FFC811UL)
#define FCG_FCG1_PERIPH_MASK            (0x71070070UL)
#define FCG_FCG2_PERIPH_MASK            (0xC0FFB03FUL)
#define FCG_FCG3_PERIPH_MASK            (0x00F07FFFUL)
/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup FCG_Global_Functions
 * @{
 */

void FCG_Fcg0PeriphClockCmd(uint32_t u32Fcg0Periph, en_functional_state_t enNewState);

void FCG_Fcg1PeriphClockCmd(uint32_t u32Fcg1Periph, en_functional_state_t enNewState);
void FCG_Fcg2PeriphClockCmd(uint32_t u32Fcg2Periph, en_functional_state_t enNewState);
void FCG_Fcg3PeriphClockCmd(uint32_t u32Fcg3Periph, en_functional_state_t enNewState);

/**
 * @}
 */

#endif /* LL_FCG_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_FCG_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
