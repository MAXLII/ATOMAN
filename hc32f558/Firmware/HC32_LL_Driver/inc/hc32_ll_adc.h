/**
 *******************************************************************************
 * @file  hc32_ll_adc.h
 * @brief This file contains all the functions prototypes of the ADC driver
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
#ifndef __HC32_LL_ADC_H__
#define __HC32_LL_ADC_H__

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
 * @addtogroup LL_ADC
 * @{
 */

#if (LL_ADC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup ADC_Global_Types ADC Global Types
 * @{
 */
/**
 * @brief Structure definition of analog watchdog(AWD) configuration.
 */
typedef struct {
    uint16_t u16WatchdogMode;           /*!< Specifies the ADC analog watchdog mode.
                                             This parameter can be a value of @ref ADC_AWD_Mode */
    uint16_t u16LowThreshold;           /*!< Specifies the ADC analog watchdog Low threshold value. */
    uint16_t u16HighThreshold;          /*!< Specifies the ADC analog watchdog High threshold value. */
    uint16_t u16FilterCheckCount;       /*!< Specifies the ADC analog watchdog filter check count.
                                             Specifies the number of consecutive comparisons
                                             that the result must met the comparison window
                                             to trigger the analog watchdog interrupt or event.
                                             This parameter can be a value of @ref ADC_AWD_Filter_Check_Cnt
                                             NOTE: This parameter is only valid for AWD0. */
} stc_adc_awd_config_t;

/**
 * @brief Structure definition of ADC initialization.
 */
typedef struct {
    uint16_t u16ScanMode;               /*!< Specifies the ADC scan convert mode.
                                             This parameter can be a value of @ref ADC_Scan_Mode */
    uint16_t u16Resolution;             /*!< Specifies the ADC resolution.
                                             This parameter can be a value of @ref ADC_Resolution */
    uint16_t u16DataAlign;              /*!< Specifies ADC data alignment.
                                             This parameter can be a value of @ref ADC_Data_Align */
} stc_adc_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup ADC_Global_Macros ADC Global Macros
 * @{
 */

/**
 * @defgroup ADC_Sequence ADC Sequence
 * @{
 */
#define ADC_SEQ_A                       (0U)                /*!< ADC sequence A. */
#define ADC_SEQ_B                       (1U)                /*!< ADC sequence B. */
#define ADC_SEQ_C                       (2U)                /*!< ADC sequence C. */
/**
 * @}
 */

/**
 * @defgroup ADC_Channel ADC Channel
 * @{
 */
/* Comment 'ADCn-x' means the ADC unit n(n=1,2,4) does NOT support the corresponding channel.
   Comment 'Default input source' indicates that the source of the corresponding channel can be remapped. */
#define ADC_CH0                         (0U)        /*!< Default input source: ADC1-PA0   ADC2-PA0,  ADC3-PE12, ADC4-differential voltage between positive pin PE11 and negative pin PE12 */
#define ADC_CH1                         (1U)        /*!< Default input source: ADC1-PA1   ADC2-PA1,  ADC3-PE9,  ADC4-differential voltage between positive pin PB14 and negative pin PB15 */
#define ADC_CH2                         (2U)        /*!< Default input source: ADC1-PA2,  ADC2-PE12, ADC3-PE13, ADC4-differential voltage between positive pin PD8 and negative pin PD9 */
#define ADC_CH3                         (3U)        /*!< Default input source: ADC1-PA3,  ADC2-PA7,  ADC3-PE7,  ADC4-differential voltage between positive pin PD10 and negative pin PD11 */
#define ADC_CH4                         (4U)        /*!< Default input source: ADC1-PC0,  ADC2-PC4,  ADC3-PB13, ADC4-differential voltage between positive pin PD12 and negative pin PD13 */
#define ADC_CH5                         (5U)        /*!< Default input source: ADC1-PC1,  ADC2-PC0,  ADC3-PE14, ADC4-PB12 */
#define ADC_CH6                         (6U)        /*!< Default input source: ADC1-PC2,  ADC2-PC1,  ADC3-PE15, ADC4-PB15 */
#define ADC_CH7                         (7U)        /*!< Default input source: ADC1-PC3,  ADC2-PC2,  ADC3-PB14, ADC4-internal reference voltage or VDD */
#define ADC_CH8                         (8U)        /*!< Default input source: ADC1-PF0,  ADC2-PC3,  ADC3-PB15, ADC4-divided voltage of AVCC */
#define ADC_CH9                         (9U)        /*!< Default input source: ADC1-PB1,  ADC2-PF1,  ADC3-PA8,  ADC4-voltage of VREFINT */
#define ADC_CH10                        (10U)       /*!< Default input source: ADC1-PB11, ADC2-PC5,  ADC3-PA9,  ADC4-output voltage of OTS */
#define ADC_CH11                        (11U)       /*!< Default input source: ADC1-PB0,  ADC2-PB2,  ADC3-PB0,  ADC4-DAC4 */
#define ADC_CH12                        (12U)       /*!< Default input source: ADC1-PE8,  ADC2-PA5,  ADC3-PD8,  ADC4-x */
#define ADC_CH13                        (13U)       /*!< Default input source: ADC1-PD9,  ADC2-PB11, ADC3-PB11, ADC4-x */
#define ADC_CH14                        (14U)       /*!< Default input source: ADC1-PD11, ADC2-PB1,  ADC3-PB12, ADC4-x */
#define ADC_CH15                        (15U)       /*!< Default input source: ADC1-PD13, ADC2-PE8,  ADC3-PE8,  ADC4-x */
#define ADC_CH16                        (16U)       /*!< Default input source: ADC1-PD14, ADC2-PA4,  ADC3-PD9,  ADC4-x */
#define ADC_CH17                        (17U)       /*!< Input source:         ADC1-PE10, ADC2-PD11, ADC3-PD10, ADC4-x */
#define ADC_CH18                        (18U)       /*!< Input source:         ADC1-PE12, ADC2-PD13, ADC3-PD11, ADC4-x */
#define ADC_CH19                        (19U)       /*!< Input source:         ADC1-x,    ADC2-PD14, ADC3-PD12, ADC4-x */
#define ADC_CH20                        (20U)       /*!< Input source:         ADC1-x,    ADC2-PE10, ADC3-PD13, ADC4-x */
#define ADC_CH21                        (21U)       /*!< Input source:         ADC1-x,    ADC2-PE12, ADC3-PD14, ADC4-x */
#define ADC_CH22                        (22U)       /*!< Input source:         ADC1-x,    ADC2-x,    ADC3-PE10, ADC4-x */
#define ADC_CH23                        (23U)       /*!< Input source:         ADC1-x,    ADC2-x,    ADC3-PE11, ADC4-x */

#define ADC_CH25                        (25U)       /*!< Input source: ADC1/2/3-internal reference voltage or VDD. ADC4-x */
#define ADC_CH26                        (26U)       /*!< Input source: ADC1/2/3-divided voltage of AVCC.           ADC4-x */
#define ADC_CH27                        (27U)       /*!< Input source: ADC1/2/3-voltage of VREFINT.                ADC4-x */
#define ADC_CH28                        (28U)       /*!< Input source: ADC1/2/3-output voltage of OTS.             ADC4-x */
#define ADC_CH29                        (29U)       /*!< Input source: ADC1-DAC1, ADC2-DAC2, ADC3-DAC3.            ADC4-x */
/**
 * @}
 */

/**
 * @defgroup ADC_Diff_Channel ADC Difference Channel
 * @{
 */
#define ADC_DIFF_CH_0                   (ADC_CH0)
#define ADC_DIFF_CH_1                   (ADC_CH1)
#define ADC_DIFF_CH_2                   (ADC_CH2)
#define ADC_DIFF_CH_3                   (ADC_CH3)
#define ADC_DIFF_CH_4                   (ADC_CH4)
#define ADC_DIFF_CH_MAX                 (ADC4_DIFF_CH_4)
/**
 * @}
 */

/**
 * @defgroup ADC_Mx_Channel ADC Multiplex Channel Selection
 * @{
 */
#define ADC_MX_CH0                      (1U << 0U)          /*!< ADC channel 0 position  */
#define ADC_MX_CH1                      (1U << 1U)          /*!< ADC channel 1 position  */
#define ADC_MX_CH2                      (1U << 2U)          /*!< ADC channel 2 position  */
#define ADC_MX_CH3                      (1U << 3U)          /*!< ADC channel 3 position  */
#define ADC_MX_CH4                      (1U << 4U)          /*!< ADC channel 4 position  */
#define ADC_MX_CH5                      (1U << 5U)          /*!< ADC channel 5 position  */
#define ADC_MX_CH6                      (1U << 6U)          /*!< ADC channel 6 position  */
#define ADC_MX_CH7                      (1U << 7U)          /*!< ADC channel 7 position  */
#define ADC_MX_CH8                      (1U << 8U)          /*!< ADC channel 8 position  */
#define ADC_MX_CH9                      (1U << 9U)          /*!< ADC channel 9 position  */
#define ADC_MX_CH10                     (1U << 10U)         /*!< ADC channel 10 position */
#define ADC_MX_CH11                     (1U << 11U)         /*!< ADC channel 11 position */
#define ADC_MX_CH12                     (1U << 12U)         /*!< ADC channel 12 position */
#define ADC_MX_CH13                     (1U << 13U)         /*!< ADC channel 13 position */
#define ADC_MX_CH14                     (1U << 14U)         /*!< ADC channel 14 position */
#define ADC_MX_CH15                     (1U << 15U)         /*!< ADC channel 15 position */
#define ADC_MX_CH16                     (1U << 16U)         /*!< ADC channel 16 position */
#define ADC_MX_CH17                     (1U << 17U)         /*!< ADC channel 17 position */
#define ADC_MX_CH18                     (1U << 18U)         /*!< ADC channel 18 position */
#define ADC_MX_CH19                     (1U << 19U)         /*!< ADC channel 19 position */
#define ADC_MX_CH20                     (1U << 20U)         /*!< ADC channel 20 position */
#define ADC_MX_CH21                     (1U << 21U)         /*!< ADC channel 21 position */
#define ADC_MX_CH22                     (1U << 22U)         /*!< ADC channel 22 position */
#define ADC_MX_CH23                     (1U << 23U)         /*!< ADC channel 23 position */
#define ADC_MX_CH25                     (1U << 25U)         /*!< ADC channel 25 position */
#define ADC_MX_CH26                     (1U << 26U)         /*!< ADC channel 26 position */
#define ADC_MX_CH27                     (1U << 27U)         /*!< ADC channel 27 position */
#define ADC_MX_CH28                     (1U << 28U)         /*!< ADC channel 28 position */
#define ADC_MX_CH29                     (1U << 29U)         /*!< ADC channel 29 position */

#define ADC1_MX_CH_ALL                  (0x3E07FFFFUL)      /*!< ADC1 Channel mask position */
#define ADC2_MX_CH_ALL                  (0x3E3FFFFFUL)      /*!< ADC2 Channel mask position */
#define ADC3_MX_CH_ALL                  (0x3EFFFFFFUL)      /*!< ADC3 Channel mask position */
#define ADC4_MX_CH_ALL                  (0x00000FFFUL)      /*!< ADC4 Channel mask position */
#define ADC4_MX_DIFF_CH_ALL             (0x0000001FUL)      /*!< ADC4 Differential Channel mask position */
/**
 * @}
 */

/**
 * @defgroup ADC_Scan_Mode ADC Scan Convert Mode
 * @{
 */
#define ADC_MD_SEQA_SINGLESHOT                  (0x0U)                      /*!< Sequence A single shot. Sequence B and C are disabled. */
#define ADC_MD_SEQA_CONT                        (0x1U << ADC_CR0_MS_POS)    /*!< Sequence A continuous. Sequence B and C are disabled. */
#define ADC_MD_SEQA_SEQB_SINGLESHOT             (0x2U << ADC_CR0_MS_POS)    /*!< Sequence A and B both single shot. Sequence C is disabled. */
#define ADC_MD_SEQA_CONT_SEQB_SINGLESHOT        (0x3U << ADC_CR0_MS_POS)    /*!< Sequence A continuous and sequence B single shot. Sequence C is disabled. */
#define ADC_MD_SEQA_SEQB_SEQC_SINGLESHOT        (0xAU << ADC_CR0_MS_POS)    /*!< Sequence A/B/C all single shot. */
#define ADC_MD_SEQA_CONT_SEQB_SEQC_SINGLESHOT   (0xBU << ADC_CR0_MS_POS)    /*!< Sequence A continuous. Sequence B and C single shot. */
#define ADC_MD_SEQA_BUFF                        (0x4U << ADC_CR0_MS_POS)    /*!< Sequence A data buffer. Sequence B and C are disabled. */
#define ADC_MD_SEQA_BUFF_SEQB_SINGLESHOT        (0x6U << ADC_CR0_MS_POS)    /*!< Sequence A data buffer. Sequence B single shot. Sequence C is disabled. */
#define ADC_MD_SEQA_BUFF_SEQB_SEQC_SINGLESHOT   (0xEU << ADC_CR0_MS_POS)    /*!< Sequence A data buffer. Sequence B and C single shot. */
/**
 * @}
 */

/**
 * @defgroup ADC_Resolution ADC Resolution
 * @{
 */
#define ADC_RESOLUTION_12BIT            (0x0U)              /*!< Resolution is 12 bit. */
#define ADC_RESOLUTION_10BIT            (ADC_CR0_ACCSEL_0)  /*!< Resolution is 10 bit. */
#define ADC_RESOLUTION_8BIT             (ADC_CR0_ACCSEL_1)  /*!< Resolution is 8 bit. */
/**
 * @}
 */

/**
 * @defgroup ADC_Data_Align ADC Data Align
 * @{
 */
#define ADC_DATAALIGN_RIGHT             (0x0U)              /*!< Right alignment of converted data. */
#define ADC_DATAALIGN_LEFT              (ADC_CR0_DFMT)      /*!< Left alignment of converted data. */
/**
 * @}
 */

/**
 * @defgroup ADC_Average_Count ADC Average Count
 * @{
 */
#define ADC_AVG_CNT2                    (0x0U)                      /*!< 2 consecutive average conversions. */
#define ADC_AVG_CNT4                    (0x1U << ADC_CR0_AVCNT_POS) /*!< 4 consecutive average conversions. */
#define ADC_AVG_CNT8                    (0x2U << ADC_CR0_AVCNT_POS) /*!< 8 consecutive average conversions. */
#define ADC_AVG_CNT16                   (0x3U << ADC_CR0_AVCNT_POS) /*!< 16 consecutive average conversions. */
#define ADC_AVG_CNT32                   (0x4U << ADC_CR0_AVCNT_POS) /*!< 32 consecutive average conversions. */
#define ADC_AVG_CNT64                   (0x5U << ADC_CR0_AVCNT_POS) /*!< 64 consecutive average conversions. */
#define ADC_AVG_CNT128                  (0x6U << ADC_CR0_AVCNT_POS) /*!< 128 consecutive average conversions. */
#define ADC_AVG_CNT256                  (0x7U << ADC_CR0_AVCNT_POS) /*!< 256 consecutive average conversions. */
/**
 * @}
 */

/**
 * @defgroup ADC_Seq_Resume_Mode ADC Sequence Resume Mode
 * @brief After interrupted by high priority sequence, low priority sequence continues to scan from the interrupt channel or the first channel.
 * @{
 */
#define ADC_RESUME_SCAN_CONT            (0U)                /*!< Scanning will continue from the interrupted channel. */
#define ADC_RESUME_SCAN_RESTART         (ADC_CR1_RSCHSEL)   /*!< Scanning will start from the first channel. */
/**
 * @}
 */

/**
 * @defgroup ADC_Sample_Mode ADC Sample Mode
 * @{
 */
#define ADC_SAMPLE_MD_NORMAL            (0U)                /*!< ADC normal sampling mode. */
#define ADC_SAMPLE_MD_OVER              (ADC_CR2_OVSMOD)    /*!< ADC over sampling mode. */
/**
 * @}
 */

/**
 * @defgroup ADC_Over_Sample_Shift ADC Over Sample Shift
 * @{
 */
#define ADC_OVER_SAMPLE_SHIFT_0BIT      (0U)                        /*!< Right shift 0 bit when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_1BIT      (1U << ADC_CR2_OVSS_POS)    /*!< Right shift 1 bit when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_2BIT      (2U << ADC_CR2_OVSS_POS)    /*!< Right shift 2 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_3BIT      (3U << ADC_CR2_OVSS_POS)    /*!< Right shift 3 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_4BIT      (4U << ADC_CR2_OVSS_POS)    /*!< Right shift 4 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_5BIT      (5U << ADC_CR2_OVSS_POS)    /*!< Right shift 5 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_6BIT      (6U << ADC_CR2_OVSS_POS)    /*!< Right shift 6 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_7BIT      (7U << ADC_CR2_OVSS_POS)    /*!< Right shift 7 bits when use over sampling mode. */
#define ADC_OVER_SAMPLE_SHIFT_8BIT      (8U << ADC_CR2_OVSS_POS)    /*!< Right shift 8 bits when use over sampling mode. */
/**
 * @}
 */

/**
 * @defgroup ADC_Open_Circuit_Detect ADC Open Circuit Detection
 * @{
 */
#define ADC_OCD_DISABLE                     (0U)                            /*!< Disable open circuit detection, no pre-discharge and no pre-charge. */
#define ADC_OCD_PRE_DISCHARGE_2CYCLES       (2U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 2 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_3CYCLES       (3U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 3 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_4CYCLES       (4U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 4 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_5CYCLES       (5U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 5 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_6CYCLES       (6U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 6 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_7CYCLES       (7U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 7 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_8CYCLES       (8U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 8 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_9CYCLES       (9U << ADC_SCD_CR_SCDPRE_POS)   /*!< Each channle will be pre-discharged for 9 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_10CYCLES      (10U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 10 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_11CYCLES      (11U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 11 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_12CYCLES      (12U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 12 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_13CYCLES      (13U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 13 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_14CYCLES      (14U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 14 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_DISCHARGE_15CYCLES      (15U << ADC_SCD_CR_SCDPRE_POS)  /*!< Each channle will be pre-discharged for 15 ADC clock cycles before sampling. */

#define ADC_OCD_PRE_CHARGE_2CYCLES          (ADC_CR2_ADDIS || (2U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 2 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_3CYCLES          (ADC_CR2_ADDIS || (3U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 3 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_4CYCLES          (ADC_CR2_ADDIS || (4U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 4 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_5CYCLES          (ADC_CR2_ADDIS || (5U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 5 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_6CYCLES          (ADC_CR2_ADDIS || (6U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 6 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_7CYCLES          (ADC_CR2_ADDIS || (7U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 7 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_8CYCLES          (ADC_CR2_ADDIS || (8U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 8 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_9CYCLES          (ADC_CR2_ADDIS || (9U << ADC_SCD_CR_SCDPRE_POS))   /*!< Each channle will be pre-charged for 9 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_10CYCLES         (ADC_CR2_ADDIS || (10U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 10 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_11CYCLES         (ADC_CR2_ADDIS || (11U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 11 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_12CYCLES         (ADC_CR2_ADDIS || (12U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 12 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_13CYCLES         (ADC_CR2_ADDIS || (13U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 13 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_14CYCLES         (ADC_CR2_ADDIS || (14U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 14 ADC clock cycles before sampling. */
#define ADC_OCD_PRE_CHARGE_15CYCLES         (ADC_CR2_ADDIS || (15U << ADC_SCD_CR_SCDPRE_POS))  /*!< Each channle will be pre-charged for 15 ADC clock cycles before sampling. */
/**
 * @}
 */

/**
 * @defgroup ADC_Hard_Trigger_Sel ADC Hard Trigger Selection
 * @{
 */
#define ADC_HARDTRIG_ADTRG_PIN          (0x00U)                     /*!< Selects the falling edge of pin ADTRG as the trigger of ADC sequence. */
#define ADC_HARDTRIG_EVT0               (0x01U)                     /*!< Selects an internal event as the trigger of ADC sequence.
                                                                         This event is specified by register ADCx_TRGSEL0(x=(null), 1, 2, 3). */
#define ADC_HARDTRIG_EVT1               (0x02U)                     /*!< Selects an internal event as the trigger of ADC sequence.
                                                                         This event is specified by register ADCx_TRGSEL1(x=(null), 1, 2, 3). */
#define ADC_HARDTRIG_EVT0_EVT1          (0x03U)                     /*!< Selects two internal events as the trigger of ADC sequence.
                                                                         The two events are specified by register ADCx_TRGSEL0 and register ADCx_TRGSEL1. */
/* More trigger source for 3 sequences scan mode. */
#define ADC_HARDTRIG_EVT2               (0x04U)                     /*!< Selects an internal event as the trigger of ADC sequence.
                                                                         This event is specified by register ADCx_TRGSEL2(x=(null), 1, 2, 3). */
#define ADC_HARDTRIG_EVT0_EVT2          (0x05U)                     /*!< Selects two internal events as the trigger of ADC sequence.
                                                                         The two events are specified by register ADCx_TRGSEL0 and register ADCx_TRGSEL2. */
#define ADC_HARDTRIG_EVT1_EVT2          (0x06U)                     /*!< Selects two internal events as the trigger of ADC sequence.
                                                                         The two events are specified by register ADCx_TRGSEL1 and register ADCx_TRGSEL2. */
#define ADC_HARDTRIG_EVT0_EVT1_EVT2     (0x07U)                     /*!< Selects three internal events as the trigger of ADC sequence.
                                                                         The three events are specified by register ADCx_TRGSEL0, ADCx_TRGSEL1 and ADCx_TRGSEL2. */
/**
 * @}
 */

/**
 * @defgroup ADC_Int_Type ADC Interrupt Type
 * @{
 */
#define ADC_INT_EOCA                    (ADC_ICR_EOCAIEN)           /*!< Interrupt of the end of conversion of sequence A. */
#define ADC_INT_EOCB                    (ADC_ICR_EOCBIEN)           /*!< Interrupt of the end of conversion of sequence B. */
#define ADC_INT_EOCC                    (ADC_ICR_EOCCIEN)           /*!< Interrupt of the end of conversion of sequence C. */
#define ADC_INT_ALL                     (ADC_INT_EOCA | ADC_INT_EOCB | ADC_INT_EOCC)
/**
 * @}
 */

/**
 * @defgroup ADC_Status_Flag ADC Status Flag
 * @{
 */
#define ADC_FLAG_EOCA                   (ADC_ISR_EOCAF)             /*!< Status flag of the end of conversion of sequence A. */
#define ADC_FLAG_EOCB                   (ADC_ISR_EOCBF)             /*!< Status flag of the end of conversion of sequence B. */
#define ADC_FLAG_EOCC                   (ADC_ISR_EOCCF)             /*!< Status flag of the end of conversion of sequence C. */
#define ADC_FLAG_NESTED_A               (ADC_ISR_SASTPDF)           /*!< Status flag of sequence A was interrupted by sequence B or C. */
#define ADC_FLAG_NESTED_B               (ADC_ISR_SBSTPDF)           /*!< Status flag of sequence B was interrupted by sequence C. */
#define ADC_FLAG_ALL                    (ADC_FLAG_EOCA | ADC_FLAG_EOCB | ADC_FLAG_EOCC | ADC_FLAG_NESTED_A | ADC_FLAG_NESTED_B)
/**
 * @}
 */

/**
 * @defgroup ADC_Sync_Unit ADC Synchronous Unit
 * @{
 */
#define ADC_SYNC_ADC1_ADC2              (0U)                                /*!< ADC1 and ADC2 work synchronously. */
#define ADC_SYNC_ADC1_ADC2_ADC3         (0x1U << ADC_SYNCCR_SYNCMD_POS)     /*!< ADC1, ADC2 and ADC3 work synchronously. */
#define ADC_SYNC_ADC1_ADC2_ADC3_ADC4    (0x9U << ADC_SYNCCR_SYNCMD_POS)     /*!< ADC1, ADC2, ADC3 and ADC4 work synchronously. */
/**
 * @}
 */

/**
 * @defgroup ADC_Sync_Mode ADC Synchronous Mode
 * @{
 */
#define ADC_SYNC_SINGLE_DELAY_TRIG      (0U)                            /*!< Single shot delayed trigger mode.
                                                                             When the trigger condition occurs, ADC1 starts first, then ADC2, ....
                                                                             All ADCs scan once. */
#define ADC_SYNC_SINGLE_PARALLEL_TRIG   (0x2U << ADC_SYNCCR_SYNCMD_POS) /*!< Single shot parallel trigger mode.
                                                                             When the trigger condition occurs, all ADCs start at the same time.
                                                                             All ADCs scan once. */
#define ADC_SYNC_CYCLIC_DELAY_TRIG      (0x4U << ADC_SYNCCR_SYNCMD_POS) /*!< Cyclic delayed trigger mode.
                                                                             When the trigger condition occurs, ADC1 starts first, then ADC2, ....
                                                                             All ADCs scan cyclically(keep scanning till you stop them). */
#define ADC_SYNC_CYCLIC_PARALLEL_TRIG   (0x6U << ADC_SYNCCR_SYNCMD_POS) /*!< Single shot parallel trigger mode.
                                                                             When the trigger condition occurs, all ADCs start at the same time.
                                                                             All ADCs scan cyclically(keep scanning till you stop them). */
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Unit ADC Analog Watchdog Unit
 * @{
 */
#define ADC_AWD0                        (0U)    /*!< ADC analog watchdog 0. */
#define ADC_AWD1                        (1U)    /*!< ADC analog watchdog 1. */
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Int_Type ADC AWD Interrupt Type
 * @{
 */
#define ADC_AWD_INT_AWD0                (ADC_AWDCR_AWD0IEN)     /*!< Interrupt of AWD0. */
#define ADC_AWD_INT_AWD1                (ADC_AWDCR_AWD1IEN)     /*!< Interrupt of AWD1. */
#define ADC_AWD_INT_ALL                 (ADC_AWD_INT_AWD0 | ADC_AWD_INT_AWD1)
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Mode ADC Analog Watchdog Mode
 * @{
 */
#define ADC_AWD_MD_CMP_OUT              (0x0U)  /*!< ADCValue > HighThreshold or ADCValue < LowThreshold */
#define ADC_AWD_MD_CMP_IN               (0x1U)  /*!< LowThreshold < ADCValue < HighThreshold */
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Comb_Mode ADC AWD(Analog Watchdog) Combination Mode
 * @note If combination mode is valid(ADC_AWD_COMB_OR/ADC_AWD_COMB_AND/ADC_AWD_COMB_XOR) and
 *       the Channels selected by the AWD0 and AWD1 are different, make sure that the channel
 *       of AWD1 is converted after the channel conversion of AWD0 ends.
 * @{
 */
#define ADC_AWD_COMB_INVD               (0U)                /*!< Combination mode is invalid. */
#define ADC_AWD_COMB_OR                 (ADC_AWDCR_AWDCM_0) /*!< The status of AWD0 is set or the status of AWD1 is set, the status of combination mode is set. */
#define ADC_AWD_COMB_AND                (ADC_AWDCR_AWDCM_1) /*!< The status of AWD0 is set and the status of AWD1 is set, the status of combination mode is set. */
#define ADC_AWD_COMB_XOR                (ADC_AWDCR_AWDCM)   /*!< Only one of the status of AWD0 and AWD1 is set, the status of combination mode is set. */
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Filter_Check_Cnt ADC AWD Filter Check Count
 * @{
 */
#define ADC_AWD_FILTER_DISABLE          (0U)                            /*!< Disable filter. */
#define ADC_AWD_FILTER_CHECK_CNT_2      (0x1U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 2 times. */
#define ADC_AWD_FILTER_CHECK_CNT_3      (0x2U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 3 times. */
#define ADC_AWD_FILTER_CHECK_CNT_4      (0x3U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 4 times. */
#define ADC_AWD_FILTER_CHECK_CNT_5      (0x4U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 5 times. */
#define ADC_AWD_FILTER_CHECK_CNT_6      (0x5U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 6 times. */
#define ADC_AWD_FILTER_CHECK_CNT_7      (0x6U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 7 times. */
#define ADC_AWD_FILTER_CHECK_CNT_8      (0x7U << ADC_AWDCR_AWDFLT_POS)  /*!< AWD consecutive compare 8 times. */
/**
 * @}
 */

/**
 * @defgroup ADC_AWD_Status_Flag ADC AWD Status Flag
 * @{
 */
#define ADC_AWD_FLAG_AWD0               (ADC_AWDSR_AWD0F)   /*!< Flag of AWD0. */
#define ADC_AWD_FLAG_AWD1               (ADC_AWDSR_AWD1F)   /*!< Flag of AWD1. */
#define ADC_AWD_FLAG_COMB               (ADC_AWDSR_AWDCMF)  /*!< Flag of combination of mode. */
#define ADC_AWD_FLAG_ALL                (ADC_AWD_FLAG_AWD0 | ADC_AWD_FLAG_AWD1 | ADC_AWD_FLAG_COMB)
/**
 * @}
 */

/**
 * @defgroup ADC_Remap_Pin ADC Remap Pin
 * @{
 */
#define ADC1_PIN_PA0                    (0U)    /*!< ADC12_IN0(PA0): default channel is ADC_CH0 of ADC1 */
#define ADC1_PIN_PA1                    (1U)    /*!< ADC12_IN1(PA1): default channel is ADC_CH1 of ADC1 */
#define ADC1_PIN_PA2                    (2U)    /*!< ADC1_IN2(PA2): default channel is ADC_CH2 of ADC1 */
#define ADC1_PIN_PA3                    (3U)    /*!< ADC1_IN3(PA3): default channel is ADC_CH3 of ADC1 */
#define ADC1_PIN_PC0                    (4U)    /*!< ADC1_IN4(PC0): default channel is ADC_CH4 of ADC1 */
#define ADC1_PIN_PC1                    (5U)    /*!< ADC1_IN5(PC1): default channel is ADC_CH5 of ADC1 */
#define ADC1_PIN_PC2                    (6U)    /*!< ADC1_IN6(PC2): default channel is ADC_CH6 of ADC1 */
#define ADC1_PIN_PC3                    (7U)    /*!< ADC1_IN7(PC3): default channel is ADC_CH7 of ADC1 */
#define ADC1_PIN_PF0                    (8U)    /*!< ADC1_IN8(PF0): default channel is ADC_CH8 of ADC1 */
#define ADC1_PIN_PB1                    (9U)    /*!< ADC1_IN9(PB1): default channel is ADC_CH9 of ADC1 */
#define ADC1_PIN_PB11                   (10U)   /*!< ADC1_IN10(PB11): default channel is ADC_CH10 of ADC1 */
#define ADC1_PIN_PB0                    (11U)   /*!< ADC1_IN11(PB0): default channel is ADC_CH11 of ADC1 */
#define ADC1_PIN_PE8                    (12U)   /*!< ADC1_IN12(PE8): default channel is ADC_CH12 of ADC1 */
#define ADC1_PIN_PD9                    (13U)   /*!< ADC1_IN13(PD9): default channel is ADC_CH13 of ADC1 */
#define ADC1_PIN_PD11                   (14U)   /*!< ADC1_IN14(PD11): default channel is ADC_CH14 of ADC1 */
#define ADC1_PIN_PD13                   (15U)   /*!< ADC1_IN15(PD13): default channel is ADC_CH15 of ADC1 */

#define ADC2_PIN_PA0                    (0U)    /*!< ADC12_IN0(PA0): default channel is ADC_CH0 of ADC2 */
#define ADC2_PIN_PA1                    (1U)    /*!< ADC12_IN1(PA1): default channel is ADC_CH1 of ADC2 */
#define ADC2_PIN_PE12                   (2U)    /*!< ADC2_IN2(PE12): default channel is ADC_CH2 of ADC2 */
#define ADC2_PIN_PA7                    (3U)    /*!< ADC2_IN3(PA7): default channel is ADC_CH3 of ADC2 */
#define ADC2_PIN_PC4                    (4U)    /*!< ADC2_IN4(PC4): default channel is ADC_CH4 of ADC2 */
#define ADC2_PIN_PC0                    (5U)    /*!< ADC2_IN5(PC0): default channel is ADC_CH5 of ADC2 */
#define ADC2_PIN_PC1                    (6U)    /*!< ADC2_IN6(PC1): default channel is ADC_CH6 of ADC2 */
#define ADC2_PIN_PC2                    (7U)    /*!< ADC2_IN7(PC2): default channel is ADC_CH7 of ADC2 */
#define ADC2_PIN_PC3                    (8U)    /*!< ADC2_IN8(PC3): default channel is ADC_CH8 of ADC2 */
#define ADC2_PIN_PF1                    (9U)    /*!< ADC2_IN9(PF1): default channel is ADC_CH9 of ADC2 */
#define ADC2_PIN_PC5                    (10U)   /*!< ADC2_IN10(PC5): default channel is ADC_CH10 of ADC2 */
#define ADC2_PIN_PB2                    (11U)   /*!< ADC2_IN11(PB2): default channel is ADC_CH11 of ADC2 */
#define ADC2_PIN_PA5                    (12U)   /*!< ADC2_IN12(PA5): default channel is ADC_CH12 of ADC2 */
#define ADC2_PIN_PB11                   (13U)   /*!< ADC23_IN13(PB11): default channel is ADC_CH13 of ADC2 */
#define ADC2_PIN_PB1                    (14U)   /*!< ADC2_IN14(PB1): default channel is ADC_CH14 of ADC2 */
#define ADC2_PIN_PE8                    (15U)   /*!< ADC23_IN15(PE8): default channel is ADC_CH15 of ADC2 */

#define ADC3_PIN_PE12                   (0U)    /*!< ADC3_IN0(PE12): default channel is ADC_CH0 of ADC3 */
#define ADC3_PIN_PE9                    (1U)    /*!< ADC3_IN1(PE9): default channel is ADC_CH1 of ADC3 */
#define ADC3_PIN_PE13                   (2U)    /*!< ADC3_IN2(PE13): default channel is ADC_CH2 of ADC3 */
#define ADC3_PIN_PE7                    (3U)    /*!< ADC3_IN3(PE7): default channel is ADC_CH3 of ADC3 */
#define ADC3_PIN_PB13                   (4U)    /*!< ADC3_IN4(PB13): default channel is ADC_CH4 of ADC3 */
#define ADC3_PIN_PE14                   (5U)    /*!< ADC3_IN5(PE14): default channel is ADC_CH5 of ADC3 */
#define ADC3_PIN_PE15                   (6U)    /*!< ADC3_IN6(PE15): default channel is ADC_CH6 of ADC3 */
#define ADC3_PIN_PB14                   (7U)    /*!< ADC3_IN7(PB14): default channel is ADC_CH7 of ADC3 */
#define ADC3_PIN_PB15                   (8U)    /*!< ADC3_IN8(PB15): default channel is ADC_CH8 of ADC3 */
#define ADC3_PIN_PA8                    (9U)    /*!< ADC3_IN9(PA8): default channel is ADC_CH9 of ADC3 */
#define ADC3_PIN_PA9                    (10U)   /*!< ADC3_IN10(PA9): default channel is ADC_CH10 of ADC3 */
#define ADC3_PIN_PB0                    (11U)   /*!< ADC3_IN11(PB0): default channel is ADC_CH11 of ADC3 */
#define ADC3_PIN_PD8                    (12U)   /*!< ADC3_IN12(PD8): default channel is ADC_CH12 of ADC3 */
#define ADC3_PIN_PB11                   (13U)   /*!< ADC23_IN13(PB11): default channel is ADC_CH13 of ADC3 */
#define ADC3_PIN_PB12                   (14U)   /*!< ADC3_IN14(PB12): default channel is ADC_CH14 of ADC3 */
#define ADC3_PIN_PE8                    (15U)   /*!< ADC23_IN15(PE8): default channel is ADC_CH15 of ADC3 */

/* If differential channel enabled */
#define ADC4_PIN_DIFF_PE11_PE12         (0U)    /*!< Diff-vol between ADC4_INP0(PE11) and ADC4_INN0(PE12): default channel is ADC_CH0 of ADC4 */
#define ADC4_PIN_DIFF_PB14_PB15         (1U)    /*!< Diff-vol between ADC4_INP1(PB14) and ADC4_INN1(PB15): default channel is ADC_CH1 of ADC4 */
#define ADC4_PIN_DIFF_PD8_PD9           (2U)    /*!< Diff-vol between ADC4_INP2(PD8) and ADC4_INN2(PD9): default channel is ADC_CH2 of ADC4 */
#define ADC4_PIN_DIFF_PD10_PD11         (3U)    /*!< Diff-vol between ADC4_INP3(PD10) and ADC4_INN3(PD11): default channel is ADC_CH3 of ADC4 */
#define ADC4_PIN_DIFF_PD12_PD13         (4U)    /*!< Diff-vol between ADC4_INP4(PD12) and ADC4_INN4(PD13): default channel is ADC_CH4 of ADC4 */
/* If differential channel disabled */
#define ADC4_PIN_PE11                   (0U)    /*!< ADC4_INP0(PE11): default channel is ADC_CH0 of ADC4 */
#define ADC4_PIN_PB14                   (1U)    /*!< ADC4_INP1(PB14): default channel is ADC_CH1 of ADC4 */
#define ADC4_PIN_PD8                    (2U)    /*!< ADC4_INP2(PD8): default channel is ADC_CH2 of ADC4 */
#define ADC4_PIN_PD10                   (3U)    /*!< ADC4_INP3(PD10): default channel is ADC_CH3 of ADC4 */
#define ADC4_PIN_PD12                   (4U)    /*!< ADC4_INP4(PD12): default channel is ADC_CH4 of ADC4 */
#define ADC4_PIN_PB12                   (5U)    /*!< ADC4_INP5(PB12): default channel is ADC_CH5 of ADC4 */
#define ADC4_PIN_PB15                   (6U)    /*!< ADC4_INP6(PB15): default channel is ADC_CH6 of ADC4 */
#define ADC4_IN_INREF_VDD               (7U)    /*!< Internal reference voltage or VDD: default channel is ADC_CH7 of ADC4 */
#define ADC4_IN_AVCC_DIV                (8U)    /*!< Divided voltage of AVCC: default channel is ADC_CH8 of ADC4 */
#define ADC4_IN_VREFINT                 (9U)    /*!< VREFINT: default channel is ADC_CH9 of ADC4 */
#define ADC4_IN_OTS                     (10U)   /*!< OTS output: default channel is ADC_CH10 of ADC4 */
#define ADC4_IN_DAC4                    (11U)   /*!< DAC4 ouput: default channel is ADC_CH11 of ADC4 */
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
 * @addtogroup ADC_Global_Functions
 * @{
 */
/*******************************************************************************
  Basic features
 ******************************************************************************/
int32_t ADC_Init(CM_ADC_TypeDef *ADCx, const stc_adc_init_t *pstcAdcInit);
int32_t ADC_DeInit(CM_ADC_TypeDef *ADCx);
int32_t ADC_StructInit(stc_adc_init_t *pstcAdcInit);
void ADC_ChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint8_t u8Ch, en_functional_state_t enNewState);
void ADC_DiffChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8DiffCh, en_functional_state_t enNewState);
void ADC_MxChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint32_t u32MxCh, en_functional_state_t enNewState);
void ADC_SetSampleTime(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, uint8_t u8SampleTime);

/* Conversion data average calculation function. */
void ADC_ConvDataAverageConfig(CM_ADC_TypeDef *ADCx, uint16_t u16AverageCount);
void ADC_ConvDataAverageChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, en_functional_state_t enNewState);
void ADC_ConvDataAverageMxChCmd(CM_ADC_TypeDef *ADCx, uint32_t u32MxCh, en_functional_state_t enNewState);
void ADC_SetSampleMode(CM_ADC_TypeDef *ADCx, uint16_t u16Mode);
void ADC_SetOverSampleShift(CM_ADC_TypeDef *ADCx, uint16_t u16ShiftValue);
void ADC_SetOpenCircuitDetect(CM_ADC_TypeDef *ADCx, uint16_t u16OpenCircuitDetect);

void ADC_TriggerConfig(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint8_t u8TriggerSel);
void ADC_TriggerCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, en_functional_state_t enNewState);
void ADC_IntCmd(CM_ADC_TypeDef *ADCx, uint8_t u8IntType, en_functional_state_t enNewState);
int32_t ADC_Start(CM_ADC_TypeDef *ADCx);
void ADC_Stop(CM_ADC_TypeDef *ADCx);
uint16_t ADC_GetValue(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch);
float32_t ADC_GetFloatValue(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch);
uint16_t ADC_GetResolution(const CM_ADC_TypeDef *ADCx);
en_flag_status_t ADC_GetStatus(const CM_ADC_TypeDef *ADCx, uint8_t u8Flag);
void ADC_ClearStatus(CM_ADC_TypeDef *ADCx, uint8_t u8Flag);
/*******************************************************************************
  Advanced features
 ******************************************************************************/
/* Channel remap. */
void ADC_ChRemap(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, uint8_t u8AdcPin);
uint8_t ADC_GetChPin(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch);
void ADC_ResetChMapping(CM_ADC_TypeDef *ADCx);

/* Sync mode. */
void ADC_SyncModeConfig(uint16_t u16SyncUnit, uint16_t u16SyncMode, uint8_t u8TriggerDelay);
void ADC_SyncModeCmd(en_functional_state_t enNewState);

/* Analog watchdog */
int32_t ADC_AWD_Config(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint8_t u8Ch, const stc_adc_awd_config_t *pstcAwd);
/* Combination mode. */
void ADC_AWD_SetCombMode(CM_ADC_TypeDef *ADCx, uint16_t u16CombMode);
void ADC_AWD_SetMode(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint16_t u16WatchdogMode);
uint16_t ADC_AWD_GetMode(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit);
void ADC_AWD_SetThreshold(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint16_t u16LowThreshold, uint16_t u16HighThreshold);
void ADC_AWD_SelectCh(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint8_t u8Ch);
void ADC_AWD_Cmd(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, en_functional_state_t enNewState);
void ADC_AWD_IntCmd(CM_ADC_TypeDef *ADCx, uint16_t u16IntType, en_functional_state_t enNewState);
en_flag_status_t ADC_AWD_GetStatus(const CM_ADC_TypeDef *ADCx, uint32_t u32Flag);
void ADC_AWD_ClearStatus(CM_ADC_TypeDef *ADCx, uint32_t u32Flag);

/* Offset correction */
void ADC_CorrectOffsetConfig(CM_ADC_TypeDef *ADCx, uint8_t u8RegIndex, uint8_t u8Ch, int16_t i16Offset, en_functional_state_t enSatState);
void ADC_CorrectOffsetCmd(CM_ADC_TypeDef *ADCx, uint8_t u8RegIndex, en_functional_state_t enNewState);

void ADC_DataRegAutoClearCmd(CM_ADC_TypeDef *ADCx, en_functional_state_t enNewState);
void ADC_SetSeqResumeMode(CM_ADC_TypeDef *ADCx, uint16_t u16SeqResumeMode);
/**
 * @}
 */

#endif /* LL_ADC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_ADC_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
