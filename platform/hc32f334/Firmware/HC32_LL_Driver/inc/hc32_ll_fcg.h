/**
 *******************************************************************************
 * @file  hc32_ll_fcg.h
 * @brief This file contains all the functions prototypes of the FCG driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
 @endverbatim
 *******************************************************************************
 * Copyright (C) 2022-2025, Xiaohua Semiconductor Co., Ltd. All rights reserved.
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

#include "hc32f3xx.h"
#include "hc32f3xx_conf.h"
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
#define FCG0_PERIPH_SRAM0               (PWC_FCG0_SRAM0)
#define FCG0_PERIPH_SRAMB               (PWC_FCG0_SRAMB)
#define FCG0_PERIPH_PLA                 (PWC_FCG0_PLA)
#define FCG0_PERIPH_DMA                 (PWC_FCG0_DMA)
#define FCG0_PERIPH_FCM                 (PWC_FCG0_FCM)
#define FCG0_PERIPH_AOS                 (PWC_FCG0_AOS)
#define FCG0_PERIPH_CTC                 (PWC_FCG0_CTC)
#define FCG0_PERIPH_CRC                 (PWC_FCG0_CRC)
/**
 * @}
 */

/**
 * @defgroup FCG_FCG1_Peripheral FCG FCG1 peripheral
 * @{
 */
#define FCG1_PERIPH_MCAN1               (PWC_FCG1_MCAN1)
#define FCG1_PERIPH_MCAN2               (PWC_FCG1_MCAN2)
#define FCG1_PERIPH_I2C                 (PWC_FCG1_I2C)
#define FCG1_PERIPH_SPI                 (PWC_FCG1_SPI)
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
#define FCG2_PERIPH_HRPWM_1             (PWC_FCG2_HRPWM_1)
#define FCG2_PERIPH_HRPWM_2             (PWC_FCG2_HRPWM_2)
#define FCG2_PERIPH_HRPWM_3             (PWC_FCG2_HRPWM_3)
#define FCG2_PERIPH_HRPWM_4             (PWC_FCG2_HRPWM_4)
#define FCG2_PERIPH_HRPWM_5             (PWC_FCG2_HRPWM_5)
#define FCG2_PERIPH_HRPWM_6             (PWC_FCG2_HRPWM_6)
#define FCG2_PERIPH_TMR4                (PWC_FCG2_TMR4)
#define FCG2_PERIPH_TMR0_1              (PWC_FCG2_TMR0_1)
#define FCG2_PERIPH_TMR0_2              (PWC_FCG2_TMR0_2)
#define FCG2_PERIPH_EMB                 (PWC_FCG2_EMB)
#define FCG2_PERIPH_TMRA_1              (PWC_FCG2_TMRA_1)
#define FCG2_PERIPH_TMRA_2              (PWC_FCG2_TMRA_2)
#define FCG2_PERIPH_TMRA_3              (PWC_FCG2_TMRA_3)
#define FCG2_PERIPH_TMRA_4              (PWC_FCG2_TMRA_4)
#define FCG2_PERIPH_TMRA_5              (PWC_FCG2_TMRA_5)
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
#define FCG3_PERIPH_DAC1                (PWC_FCG3_DAC1)
#define FCG3_PERIPH_DAC2                (PWC_FCG3_DAC2)
#define FCG3_PERIPH_CMP1_2              (PWC_FCG3_CMP12)
#define FCG3_PERIPH_CMP3                (PWC_FCG3_CMP3)
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
#define FCG_FCG0_PERIPH_MASK            (0x00874C11UL)
#define FCG_FCG1_PERIPH_MASK            (0x00010013UL)
#define FCG_FCG2_PERIPH_MASK            (0x01F0B7FFUL)
#define FCG_FCG3_PERIPH_MASK            (0x00F00337UL)
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
