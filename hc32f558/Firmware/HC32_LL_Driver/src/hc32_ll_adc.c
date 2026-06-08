/**
 *******************************************************************************
 * @file  hc32_ll_adc.c
 * @brief This file provides firmware functions to manage the Analog-to-Digital
 *        Converter(ADC).
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

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_adc.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_ADC ADC
 * @brief Analog-to-Digital Converter Driver Library
 * @{
 */

#if (LL_ADC_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup ADC_Local_Macros ADC Local Macros
 * @{
 */

#define ADC_RMU_TIMEOUT                 (100U)

/**
 * @defgroup ADC_AWD_DR_CHSR ADC AWD DR CHSR
 * @{
 */
#define ADC_AWDx_DR(awd, reg_base)      (*(__IO uint16_t *)((uint32_t)(reg_base) + ((uint32_t)(awd) * 8U)))
#define ADC_AWDx_CHSR(awd, reg_base)    (*(__IO uint8_t *)((uint32_t)(reg_base) + ((uint32_t)(awd) * 8U)))
/**
 * @}
 */

/**
 * @defgroup ADC_Func_Macro_For_Offset ADC Function Macro For Correct Offset
 * @{
 */
/* Get absolute value */
#define ADC_GET_ABS(x)          ((x) < 0 ? (-(x)) : (x))
/* Get sign bit */
#define ADC_GET_SIGN(x)         ((x) < 0 ? 1 : 0)
/**
 * @}
 */

/**
 * @defgroup ADC_Channel_Max ADC Channel Max
 * @{
 */
#define ADC1_REMAP_CH_MAX               (ADC_CH15)
#define ADC2_REMAP_CH_MAX               (ADC_CH15)
#define ADC3_REMAP_CH_MAX               (ADC_CH15)
#define ADC4_REMAP_CH_MAX               (ADC_CH11)

#define ADC1_REMAP_PIN_MAX              (ADC1_PIN_PD13)
#define ADC2_REMAP_PIN_MAX              (ADC2_PIN_PE8)
#define ADC3_REMAP_PIN_MAX              (ADC3_PIN_PE8)
#define ADC4_REMAP_PIN_MAX              (ADC4_IN_DAC4)

#define ADC1_PIN_CH_MAX                 (ADC_CH18)
#define ADC2_PIN_CH_MAX                 (ADC_CH21)
#define ADC3_PIN_CH_MAX                 (ADC_CH23)
#define ADC4_PIN_CH_MAX                 (ADC_CH11)

#define ADC1_MX_PIN_CH_ALL              ((1UL << ADC1_PIN_CH_MAX) - 1UL)
#define ADC2_MX_PIN_CH_ALL              ((1UL << ADC2_PIN_CH_MAX) - 1UL)
#define ADC3_MX_PIN_CH_ALL              ((1UL << ADC3_PIN_CH_MAX) - 1UL)
#define ADC4_MX_PIN_CH_ALL              ((1UL << ADC4_PIN_CH_MAX) - 1UL)

#define ADC_SSTR_NUM                    (16U)
#define ADC1_SSTR_NUM                   (ADC_SSTR_NUM)
#define ADC2_SSTR_NUM                   (ADC_SSTR_NUM)
#define ADC3_SSTR_NUM                   (ADC_SSTR_NUM)
#define ADC4_SSTR_NUM                   (11U)
/**
 * @}
 */

/**
 * @defgroup ADC_Reg_Access_Width ADC Register Access Width
 * @{
 */
#define ADC_CHSEL_TYPE                   uint32_t
#define ADC_CHSEL_WRITE                  WRITE_REG32
#define ADC_CHSEL_SETBIT                 SET_REG32_BIT
#define ADC_CHSEL_CLRBIT                 CLR_REG32_BIT
/**
 * @}
 */

/**
 * @defgroup ADC_Check_Parameters_Validity ADC check parameters validity
 * @{
 */
#define IS_ADC_BIT_MASK(x, mask)        (((x) != 0U) && (((x) | (mask)) == (mask)))

/* ADC unit check */
#define IS_ADC_UNIT(x)                                                         \
(   ((x) == CM_ADC1)                        ||                                 \
    ((x) == CM_ADC2)                        ||                                 \
    ((x) == CM_ADC3)                        ||                                 \
    ((x) == CM_ADC4))

#define IS_ADC_DIFF_UNIT(x)             ((x) == CM_ADC4)

/* ADC sequence check */
#define IS_ADC_SEQ(x)                   (((x) == ADC_SEQ_A) || ((x) == ADC_SEQ_B) || ((x) == ADC_SEQ_C))

/* ADC channel check */
#define IS_ADC_CH(adc, ch)                                                     \
(   (((adc) == CM_ADC1) && (((ch) <= ADC_CH18) || (((ch) >= ADC_CH25) && ((ch) <= ADC_CH29))))   || \
    (((adc) == CM_ADC2) && (((ch) <= ADC_CH21) || (((ch) >= ADC_CH25) && ((ch) <= ADC_CH29))))   || \
    (((adc) == CM_ADC3) && (((ch) <= ADC_CH23) || (((ch) >= ADC_CH25) && ((ch) <= ADC_CH29))))   || \
    (((adc) == CM_ADC4) && ((ch) <= ADC_CH11)))

#define IS_ADC_PIN_CH(adc, ch)                                                 \
(   (((adc) == CM_ADC1) && ((ch) <= ADC1_PIN_CH_MAX))   ||                     \
    (((adc) == CM_ADC2) && ((ch) <= ADC2_PIN_CH_MAX))   ||                     \
    (((adc) == CM_ADC3) && ((ch) <= ADC3_PIN_CH_MAX))   ||                     \
    (((adc) == CM_ADC4) && ((ch) <= ADC4_PIN_CH_MAX)))

#define IS_ADC_DIFF_CH(adc, ch)     ((adc) == CM_ADC4) && ((ch) <= ADC_CH4)

/* ADC MX channel check */
#define IS_ADC_MX_CH(adc, ch)                                                  \
(   (((adc) == CM_ADC1) && IS_ADC_BIT_MASK(ch, ADC1_MX_CH_ALL))   ||           \
    (((adc) == CM_ADC2) && IS_ADC_BIT_MASK(ch, ADC2_MX_CH_ALL))   ||           \
    (((adc) == CM_ADC3) && IS_ADC_BIT_MASK(ch, ADC3_MX_CH_ALL))   ||           \
    (((adc) == CM_ADC4) && IS_ADC_BIT_MASK(ch, ADC4_MX_CH_ALL)))

#define IS_ADC_PIN_MX_CH(adc, ch)                                              \
(   (((adc) == CM_ADC1) && IS_ADC_BIT_MASK(ch, ADC1_MX_PIN_CH_ALL))     ||     \
    (((adc) == CM_ADC2) && IS_ADC_BIT_MASK(ch, ADC2_MX_PIN_CH_ALL))     ||     \
    (((adc) == CM_ADC3) && IS_ADC_BIT_MASK(ch, ADC3_MX_PIN_CH_ALL))     ||     \
    (((adc) == CM_ADC4) && IS_ADC_BIT_MASK(ch, ADC4_MX_PIN_CH_ALL)))

#define IS_ADC_MX_DIFF_CH(adc, ch)      (((adc) == CM_ADC4) && IS_ADC_BIT_MASK(ch, ADC4_MX_DIFF_CH_ALL))

#define IS_ADC_SCAN_MD(x)                                                      \
(   ((x) == ADC_MD_SEQA_SINGLESHOT)                 ||                         \
    ((x) == ADC_MD_SEQA_CONT)                       ||                         \
    ((x) == ADC_MD_SEQA_SEQB_SINGLESHOT)            ||                         \
    ((x) == ADC_MD_SEQA_CONT_SEQB_SINGLESHOT)       ||                         \
    ((x) == ADC_MD_SEQA_SEQB_SEQC_SINGLESHOT)       ||                         \
    ((x) == ADC_MD_SEQA_CONT_SEQB_SEQC_SINGLESHOT)  ||                         \
    ((x) == ADC_MD_SEQA_BUFF)                       ||                         \
    ((x) == ADC_MD_SEQA_BUFF_SEQB_SINGLESHOT)       ||                         \
    ((x) == ADC_MD_SEQA_BUFF_SEQB_SEQC_SINGLESHOT))

#define IS_ADC_RESOLUTION(x)                                                   \
(   ((x) == ADC_RESOLUTION_8BIT)            ||                                 \
    ((x) == ADC_RESOLUTION_10BIT)           ||                                 \
    ((x) == ADC_RESOLUTION_12BIT))

#define IS_ADC_HARDTRIG(x)                                                     \
(   ((x) == ADC_HARDTRIG_ADTRG_PIN)         ||                                 \
    ((x) == ADC_HARDTRIG_EVT0)              ||                                 \
    ((x) == ADC_HARDTRIG_EVT1)              ||                                 \
    ((x) == ADC_HARDTRIG_EVT0_EVT1))

#define IS_ADC_DATAALIGN(x)                                                    \
(   ((x) == ADC_DATAALIGN_RIGHT)            ||                                 \
    ((x) == ADC_DATAALIGN_LEFT))

#define IS_ADC_SEQ_RESUME_MD(x)                                                \
(   ((x) == ADC_RESUME_SCAN_CONT)           ||                                 \
    ((x) == ADC_RESUME_SCAN_RESTART))

#define IS_ADC_SAMPLE_TIME(x)           ((x) >= 3U)

#define IS_ADC_SAMPLE_MD(x)                                                    \
(   ((x) == ADC_SAMPLE_MD_NORMAL)           ||                                 \
    ((x) == ADC_SAMPLE_MD_OVER))

#define IS_ADC_OVER_SAMPLE_SHIFT(x)     (((x) >> ADC_CR2_OVSS_POS) <= 8U)

#define IS_ADC_OCD_CFG(x)               (((x) <= 0x1FU) && (((x) & 0xFU) != 0x1U))

#define IS_ADC_INT(x)                   IS_ADC_BIT_MASK(x, ADC_INT_ALL)
#define IS_ADC_FLAG(x)                  IS_ADC_BIT_MASK(x, ADC_FLAG_ALL)

/* Scan-average. */
#define IS_ADC_AVG_CNT(x)               (((x) >> ADC_CR0_AVCNT_POS) <= 7U)
#define IS_ADC_AVG_CH(adc, ch)          IS_ADC_PIN_CH(adc, ch)
#define IS_ADC_AVG_MX_CH(adc, ch)       IS_ADC_PIN_MX_CH(adc, ch)

#define IS_ADC_REMAP_CH(adc, ch)                                               \
(   (((adc) == CM_ADC1) && ((ch) <= ADC1_REMAP_CH_MAX))     ||                 \
    (((adc) == CM_ADC2) && ((ch) <= ADC2_REMAP_CH_MAX))     ||                 \
    (((adc) == CM_ADC3) && ((ch) <= ADC3_REMAP_CH_MAX))     ||                 \
    (((adc) == CM_ADC4) && ((ch) <= ADC4_REMAP_CH_MAX)))

#define IS_ADC_REMAP_PIN(adc, pin)                                             \
(   (((adc) == CM_ADC1) && ((pin) <= ADC1_REMAP_PIN_MAX))   ||                 \
    (((adc) == CM_ADC2) && ((pin) <= ADC2_REMAP_PIN_MAX))   ||                 \
    (((adc) == CM_ADC3) && ((pin) <= ADC3_REMAP_PIN_MAX))   ||                 \
    (((adc) == CM_ADC4) && ((pin) <= ADC4_REMAP_PIN_MAX)))

/* Sync mode. */
#define IS_ADC_SYNC_MD(x)                                                      \
(   ((x) == ADC_SYNC_SINGLE_DELAY_TRIG)     ||                                 \
    ((x) == ADC_SYNC_SINGLE_PARALLEL_TRIG)  ||                                 \
    ((x) == ADC_SYNC_CYCLIC_DELAY_TRIG)     ||                                 \
    ((x) == ADC_SYNC_CYCLIC_PARALLEL_TRIG))

#define IS_ADC_SYNC(x)                                                         \
(   ((x) == ADC_SYNC_ADC1_ADC2)             ||                                 \
    ((x) == ADC_SYNC_ADC1_ADC2_ADC3)        ||                                 \
    ((x) == ADC_SYNC_ADC1_ADC2_ADC3_ADC4))

/* Analog watchdog. */
#define IS_ADC_AWD_CH(adc, ch)      IS_ADC_PIN_CH(adc, ch)

#define IS_ADC_AWD_MD(x)                                                       \
(   ((x) == ADC_AWD_MD_CMP_OUT)             ||                                 \
    ((x) == ADC_AWD_MD_CMP_IN))

#define IS_ADC_AWD(x)                   ((x) <= ADC_AWD1)

/* AWD flag check */
#define IS_ADC_AWD_FLAG(x)              IS_ADC_BIT_MASK(x, ADC_AWD_FLAG_ALL)

#define IS_ADC_AWD_THRESHOLD(res, th)                                          \
(   (((res) == ADC_RESOLUTION_12BIT) && ((th) < 4096U))     ||                 \
    (((res) == ADC_RESOLUTION_10BIT) && ((th) < 1024U))     ||                 \
    (((res) == ADC_RESOLUTION_8BIT) && ((th) < 256U)))

/* Two AWD units */
#define IS_ADC_AWD_COMB_MD(x)                                                  \
(   ((x) == ADC_AWD_COMB_INVD)              ||                                 \
    ((x) == ADC_AWD_COMB_OR)                ||                                 \
    ((x) == ADC_AWD_COMB_AND)               ||                                 \
    ((x) == ADC_AWD_COMB_XOR))

#define IS_ADC_AWD_INT(x)               IS_ADC_BIT_MASK(x, ADC_AWD_INT_ALL)

#define IS_ADC_OFFSET_REG_IDX(x)    ((x) <= 7U)

#define IS_ADC_OFFSET_CH(adc, ch)   IS_ADC_PIN_CH(adc, ch)

#define IS_ADC_OFFSET_VAL(x)        (((x) >= -4095) && ((x) <= 4095))

/**
 * @}
 */

/**
 * @}
 */

/*******************************************************************************
 * Global variable definitions (declared in header file with 'extern')
 ******************************************************************************/

/*******************************************************************************
 * Local function prototypes ('static')
 ******************************************************************************/

/*******************************************************************************
 * Local variable definitions ('static')
 ******************************************************************************/

/*******************************************************************************
 * Function implementation - global ('extern') and local ('static')
 ******************************************************************************/
/**
 * @defgroup ADC_Global_Functions ADC Global Functions
 * @{
 */

/**
 * @brief  Initializes the specified ADC peripheral according to the specified parameters
 *         in the structure pstcAdcInit.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  pstcAdcInit            Pointer to a @ref stc_adc_init_t structure that contains the
 *                                      configuration information for the specified ADC.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAdcInit == NULL.
 */
int32_t ADC_Init(CM_ADC_TypeDef *ADCx, const stc_adc_init_t *pstcAdcInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));

    if (pstcAdcInit != NULL) {
        DDL_ASSERT(IS_ADC_SCAN_MD(pstcAdcInit->u16ScanMode));
        DDL_ASSERT(IS_ADC_RESOLUTION(pstcAdcInit->u16Resolution));
        DDL_ASSERT(IS_ADC_DATAALIGN(pstcAdcInit->u16DataAlign));
        /* Configures scan mode, resolution, data align. */
        WRITE_REG16(ADCx->CR0, pstcAdcInit->u16ScanMode | pstcAdcInit->u16Resolution | pstcAdcInit->u16DataAlign);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Deinitialize the specified ADC peripheral registers to their default reset values.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @retval int32_t:
 *           - LL_OK:                   De-Initialize success.
 */
int32_t ADC_DeInit(CM_ADC_TypeDef *ADCx)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32UnitShift;
    uint32_t u32UnitBase;
    __IO uint8_t u8TimeOut = 0U;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    /* Check FRST register protect */
    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);

    /* Reset ADC */
    u32UnitBase = (uint32_t)ADCx;
    u32UnitShift = (u32UnitBase - CM_ADC1_BASE) / (CM_ADC2_BASE - CM_ADC1_BASE);
    CLR_REG32_BIT(CM_RMU->FRST3, RMU_FRST3_ADC1 << u32UnitShift);
    /* Ensure reset procedure is completed */
    while (0UL == READ_REG32_BIT(CM_RMU->FRST3, RMU_FRST3_ADC1 << u32UnitShift)) {
        u8TimeOut++;
        if (u8TimeOut > ADC_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    return i32Ret;
}

/**
 * @brief  Set each @ref stc_adc_init_t field to default value.
 * @param  [in]  pstcAdcInit            Pointer to a @ref stc_adc_init_t structure
 *                                      whose fields will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAdcInit == NULL.
 */
int32_t ADC_StructInit(stc_adc_init_t *pstcAdcInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcAdcInit != NULL) {
        pstcAdcInit->u16ScanMode   = ADC_MD_SEQA_SINGLESHOT;
        pstcAdcInit->u16Resolution = ADC_RESOLUTION_12BIT;
        pstcAdcInit->u16DataAlign  = ADC_DATAALIGN_RIGHT;
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Enable or disable the specified ADC channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Seq                  The sequence whose channel specified by 'u8Ch' will be enabled or disabled.
 *                                      This parameter can be a value of @ref ADC_Sequence
 *   @arg  ADC_SEQ_A:                   ADC sequence A.
 *   @arg  ADC_SEQ_B:                   ADC sequence B.
 * @param  [in]  u8Ch                   The ADC channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @note   Sequence A and Sequence B CAN NOT include the same channel!
 * @note   Sequence A can always started by software(by calling @ref ADC_Start()),
 *         regardless of whether the hardware trigger source is valid or not.
 * @note   Sequence B must be specified a valid hard trigger by calling functions @ref ADC_TriggerConfig()
 *         and @ref ADC_TriggerCmd().
 */
void ADC_ChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint8_t u8Ch, en_functional_state_t enNewState)
{
    __IO ADC_CHSEL_TYPE *CHSELRx;

    DDL_ASSERT(IS_ADC_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_ADC_SEQ(u8Seq));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (u8Seq == ADC_SEQ_C) {
        CHSELRx = (__IO ADC_CHSEL_TYPE *)((uint32_t)(&ADCx->CHSELRC));
    } else {
        CHSELRx = (__IO ADC_CHSEL_TYPE *)((uint32_t)(&ADCx->CHSELRA) + (u8Seq * 4UL));
    }
    if (enNewState == ENABLE) {
        /* Enable the specified channel. */
        ADC_CHSEL_SETBIT(*CHSELRx, 1UL << u8Ch);
    } else {
        /* Disable the specified channel. */
        ADC_CHSEL_CLRBIT(*CHSELRx, 1UL << u8Ch);
    }
}

/**
 * @brief  Enable or disable the specified ADC Difference channel.
 * @param  [in]  ADCx                   Pointer to differential ADC instance register base.
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8DiffCh               The ADC difference channel.
 *                                      This parameter can be a value of @ref ADC_Diff_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 */
void ADC_DiffChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8DiffCh, en_functional_state_t enNewState)
{
    __IO uint32_t *DIFCHSELx;

    DDL_ASSERT(IS_ADC_DIFF_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_DIFF_CH(ADCx, u8DiffCh));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    DIFCHSELx = (__IO uint32_t *)&ADCx->DIFCHSELR;
    if (ENABLE == enNewState) {
        SET_REG32_BIT(*DIFCHSELx, (1UL << u8DiffCh));
    } else {
        CLR_REG32_BIT(*DIFCHSELx, (1UL << u8DiffCh));
    }
}
/**
 * @brief  Enable or disable the specified ADC channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Seq                  The sequence whose channel specified by 'u32MxCh' will be enabled or disabled.
 *                                      This parameter can be a value of @ref ADC_Sequence
 *   @arg  ADC_SEQ_A:                   ADC sequence A.
 *   @arg  ADC_SEQ_B:                   ADC sequence B.
 * @param  [in]  u32MxCh                The ADC channel.
 *                                      This parameter can be any component of @ref ADC_Mx_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @note   Sequence A and Sequence B CAN NOT include the same channel!
 * @note   Sequence A can always started by software(by calling @ref ADC_Start()),
 *         regardless of whether the hardware trigger source is valid or not.
 * @note   Sequence B must be specified a valid hard trigger by calling functions @ref ADC_TriggerConfig()
 *         and @ref ADC_TriggerCmd().
 */
void ADC_MxChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint32_t u32MxCh, en_functional_state_t enNewState)
{
    __IO ADC_CHSEL_TYPE *CHSELRx;

    DDL_ASSERT(IS_ADC_MX_CH(ADCx, u32MxCh));
    DDL_ASSERT(IS_ADC_SEQ(u8Seq));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (u8Seq == ADC_SEQ_C) {
        CHSELRx = (__IO ADC_CHSEL_TYPE *)((uint32_t)(&ADCx->CHSELRC));
    } else {
        CHSELRx = (__IO ADC_CHSEL_TYPE *)((uint32_t)(&ADCx->CHSELRA) + (u8Seq * 4UL));
    }
    if (enNewState == ENABLE) {
        /* Enable the specified channel. */
        ADC_CHSEL_SETBIT(*CHSELRx, u32MxCh);
    } else {
        /* Disable the specified channel. */
        ADC_CHSEL_CLRBIT(*CHSELRx, u32MxCh);
    }
}

/**
 * @brief  Set sampling time for the specified channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   The channel to be set sampling time.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @param  [in]  u8SampleTime           Sampling time for the channel that specified by 'u8Ch'.
 * @retval None
 */
void ADC_SetSampleTime(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, uint8_t u8SampleTime)
{
    __IO uint8_t *SSTRx;

    DDL_ASSERT(IS_ADC_SAMPLE_TIME(u8SampleTime));

    DDL_ASSERT(IS_ADC_CH(ADCx, u8Ch));
    SSTRx = (__IO uint8_t *)((uint32_t)&ADCx->SSTR0 + u8Ch);
    if (u8Ch < ADC_SSTR_NUM) {
        WRITE_REG8(*SSTRx, u8SampleTime);
    } else {
        WRITE_REG8(ADCx->SSTRL, u8SampleTime);
    }
}

/**
 * @brief  Set scan-average count.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16AverageCount        Scan-average count.
 *                                      This parameter can be a value of @ref ADC_Average_Count
 *   @arg  ADC_AVG_CNT2:                2 consecutive average conversions.
 *   @arg  ADC_AVG_CNT4:                4 consecutive average conversions.
 *   @arg  ADC_AVG_CNT8:                8 consecutive average conversions.
 *   @arg  ADC_AVG_CNT16:               16 consecutive average conversions.
 *   @arg  ADC_AVG_CNT32:               32 consecutive average conversions.
 *   @arg  ADC_AVG_CNT64:               64 consecutive average conversions.
 *   @arg  ADC_AVG_CNT128:              128 consecutive average conversions.
 *   @arg  ADC_AVG_CNT256:              256 consecutive average conversions.
 * @retval None
 */
void ADC_ConvDataAverageConfig(CM_ADC_TypeDef *ADCx, uint16_t u16AverageCount)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AVG_CNT(u16AverageCount));
    MODIFY_REG16(ADCx->CR0, ADC_CR0_AVCNT, u16AverageCount);
}

/**
 * @brief  Enable or disable conversion data average calculation channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   The ADC channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_ConvDataAverageChCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_AVG_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG32_BIT(ADCx->AVCHSELR, 1UL << u8Ch);
    } else {
        CLR_REG32_BIT(ADCx->AVCHSELR, 1UL << u8Ch);
    }
}

/**
 * @brief  Enable or disable conversion data average calculation channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u32MxCh                The ADC channel.
 *                                      This parameter can be any component of @ref ADC_Mx_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_ConvDataAverageMxChCmd(CM_ADC_TypeDef *ADCx, uint32_t u32MxCh, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_AVG_MX_CH(ADCx, u32MxCh));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG32_BIT(ADCx->AVCHSELR, u32MxCh);
    } else {
        CLR_REG32_BIT(ADCx->AVCHSELR, u32MxCh);
    }
}

/**
 * @brief  Specifies the sample mode.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16Mode                The sample mode.
 *                                      This parameter can be a value of @ref ADC_Sample_Mode
 * @retval None
 */
void ADC_SetSampleMode(CM_ADC_TypeDef *ADCx, uint16_t u16Mode)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_SAMPLE_MD(u16Mode));

    if (ADC_SAMPLE_MD_NORMAL == u16Mode) {
        CLR_REG16_BIT(ADCx->CR2, ADC_SAMPLE_MD_OVER);
    } else {
        SET_REG16_BIT(ADCx->CR2, ADC_SAMPLE_MD_OVER);
    }
}

/**
 * @brief  Specifies the over sample shift value.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16ShiftValue          The shift value.
 *                                      This parameter can be a value of @ref ADC_Over_Sample_Shift
 * @retval None
 */
void ADC_SetOverSampleShift(CM_ADC_TypeDef *ADCx, uint16_t u16ShiftValue)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_OVER_SAMPLE_SHIFT(u16ShiftValue));

    MODIFY_REG16(ADCx->CR2, ADC_CR2_OVSS, u16ShiftValue);
}

/**
 * @brief  Set open circuit detect function.
 *         Set sampling pre-charge or pre-discharge cycles for each channel.
 *         If the alalog source to ADC is open circuit, the ADC conversion value will be 0xFFF or 0x000.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16OpenCircuitDetect   The open circuit detect configuration value.
 *                                      This parameter can be a value of @ref ADC_Open_Circuit_Detect
 * @retval None
 */
void ADC_SetOpenCircuitDetect(CM_ADC_TypeDef *ADCx, uint16_t u16OpenCircuitDetect)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_OCD_CFG(u16OpenCircuitDetect));

    MODIFY_REG16(ADCx->CR2, ADC_CR2_ADDIS, u16OpenCircuitDetect);
}

/**
 * @brief  Specifies the hard trigger for the specified ADC sequence.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADCx or CM_ADC
 * @param  [in]  u8Seq                  The sequence to be configured.
 *                                      This parameter can be a value of @ref ADC_Sequence
 *   @arg  ADC_SEQ_A:                   Sequence A.
 *   @arg  ADC_SEQ_B:                   Sequence B.
 * @param  [in]  u8TriggerSel           Hard trigger selection.
 *                                      This parameter can be a value of @ref ADC_Hard_Trigger_Sel
 * @retval None
 * @note   ADC must be stopped while calling this function.
 * @note   The trigger source CANNOT be an event that generated by the sequence itself.
 */
void ADC_TriggerConfig(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, uint8_t u8TriggerSel)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_SEQ(u8Seq));
    DDL_ASSERT(IS_ADC_HARDTRIG(u8TriggerSel));

    u8Seq *= ADC_TRGSR_TRGSELB_POS;
    MODIFY_REG32(ADCx->TRGSR, (uint32_t)ADC_TRGSR_TRGSELA << u8Seq, (uint32_t)u8TriggerSel << u8Seq);
}

/**
 * @brief  Enable or disable the hard trigger of the specified ADC sequence.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADCx or CM_ADC
 * @param  [in]  u8Seq                  The sequence to be configured.
 *                                      This parameter can be a value of @ref ADC_Sequence
 *   @arg  ADC_SEQ_A:                   Sequence A.
 *   @arg  ADC_SEQ_B:                   Sequence B.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   ADC must be stopped while calling this function.
 */
void ADC_TriggerCmd(CM_ADC_TypeDef *ADCx, uint8_t u8Seq, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_SEQ(u8Seq));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    if (enNewState == ENABLE) {
        SET_REG32_BIT(ADCx->TRGSR, (uint32_t)ADC_TRGSR_TRGENA << (u8Seq * ADC_TRGSR_TRGSELB_POS));
    } else {
        CLR_REG32_BIT(ADCx->TRGSR, (uint32_t)ADC_TRGSR_TRGENA << (u8Seq * ADC_TRGSR_TRGSELB_POS));
    }
}

/**
 * @brief  Enable or disable ADC interrupts.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8IntType              ADC interrupt.
 *                                      This parameter can be values of @ref ADC_Int_Type
 *   @arg  ADC_INT_EOCA:                Interrupt of the end of conversion of sequence A.
 *   @arg  ADC_INT_EOCB:                Interrupt of the end of conversion of sequence B.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_IntCmd(CM_ADC_TypeDef *ADCx, uint8_t u8IntType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_INT(u8IntType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG8_BIT(ADCx->ICR, u8IntType);
    } else {
        CLR_REG8_BIT(ADCx->ICR, u8IntType);
    }
}

/**
 * @brief  Start sequence A conversion.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @retval int32_t
 *           - LL_OK:                   Start success.
 *           - LL_ERR_BUSY:             ADC is busy.
 */
int32_t ADC_Start(CM_ADC_TypeDef *ADCx)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));

    if (1U == READ_REG8(ADCx->STR)) {
        i32Ret = LL_ERR_BUSY;
    } else {
        WRITE_REG8(ADCx->STR, ADC_STR_STRT);
    }

    return i32Ret;
}

/**
 * @brief  Stop ADC conversion, both sequence A and sequence B.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @retval None
 */
void ADC_Stop(CM_ADC_TypeDef *ADCx)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    WRITE_REG8(ADCx->STR, 0U);
}

/**
 * @brief  Get the ADC value of the specified channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   The ADC channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @retval An uint16_t type value of ADC value.
 */
uint16_t ADC_GetValue(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch)
{
    DDL_ASSERT(IS_ADC_CH(ADCx, u8Ch));

    return RW_MEM16((uint32_t)&ADCx->DR0 + u8Ch * 2UL);
}

/**
 * @brief  Get the ADC value(single-precision float type) of the specified channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   The ADC channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @retval An single-precision float type value of ADC value.
 */
float32_t ADC_GetFloatValue(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch)
{
    DDL_ASSERT(IS_ADC_PIN_CH(ADCx, u8Ch));

    return *(float *)((uint32_t)&ADCx->FDR0 + ((uint32_t)u8Ch * 4UL));
}

/**
 * @brief  Get the ADC resolution.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @retval An uint16_t type value of ADC resolution. @ref ADC_Resolution
 */
uint16_t ADC_GetResolution(const CM_ADC_TypeDef *ADCx)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));

    return READ_REG16_BIT(ADCx->CR0, ADC_CR0_ACCSEL);
}

/**
 * @brief  Get the status of the specified ADC flag.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Flag                 ADC status flag.
 *                                      This parameter can be a value of @ref ADC_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t ADC_GetStatus(const CM_ADC_TypeDef *ADCx, uint8_t u8Flag)
{
    en_flag_status_t enStatus = RESET;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_FLAG(u8Flag));

    if (READ_REG8_BIT(ADCx->ISR, u8Flag) != 0U) {
        enStatus = SET;
    }

    return enStatus;
}

/**
 * @brief  Clear the status of the specified ADC flag.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Flag                 ADC status flag.
 *                                      This parameter can be values of @ref ADC_Status_Flag
 * @retval None
 */
void ADC_ClearStatus(CM_ADC_TypeDef *ADCx, uint8_t u8Flag)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_FLAG(u8Flag));

    WRITE_REG8(ADCx->ISCLRR, u8Flag);
}

/**
 * @brief  Remap the correspondence between ADC channel and analog input pins.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   This parameter can be a value of @ref ADC_Channel
 * @param  [in]  u8AdcPin               This parameter can be a value of @ref ADC_Remap_Pin
 * @retval None
 */
void ADC_ChRemap(CM_ADC_TypeDef *ADCx, uint8_t u8Ch, uint8_t u8AdcPin)
{
    uint8_t u8FieldOfs;
    __IO uint16_t *CHMUXRx;

    DDL_ASSERT(IS_ADC_REMAP_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_ADC_REMAP_PIN(ADCx, u8AdcPin));

    CHMUXRx    = (__IO uint16_t *)(((uint32_t)&ADCx->CHMUXR0) + (u8Ch / 4UL) * 2UL);
    u8FieldOfs = (u8Ch % 4U) * 4U;
    MODIFY_REG16(*CHMUXRx, ((uint32_t)ADC_CHMUXR0_CH00MUX << u8FieldOfs), ((uint32_t)u8AdcPin << u8FieldOfs));
}

/**
 * @brief  Get the ADC pin corresponding to the specified ADC channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8Ch                   ADC channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @retval An uint8_t type value of ADC pin. @ref ADC_Remap_Pin
 */
uint8_t ADC_GetChPin(const CM_ADC_TypeDef *ADCx, uint8_t u8Ch)
{
    uint8_t u8RetPin;
    uint8_t u8FieldOfs;
    __IO uint16_t *CHMUXRx;

    DDL_ASSERT(IS_ADC_REMAP_CH(ADCx, u8Ch));

    CHMUXRx    = (__IO uint16_t *)(((uint32_t)&ADCx->CHMUXR0) + (u8Ch / 4UL) * 2UL);
    u8FieldOfs = (u8Ch % 4U) * 4U;
    u8RetPin   = ((uint8_t)(*CHMUXRx >> u8FieldOfs)) & 0xFU;

    return u8RetPin;
}

/**
 * @brief  Reset channel-pin mapping.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @retval None
 */
void ADC_ResetChMapping(CM_ADC_TypeDef *ADCx)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));

    WRITE_REG16(ADCx->CHMUXR0, 0x3210U);
    WRITE_REG16(ADCx->CHMUXR1, 0x7654U);
    WRITE_REG16(ADCx->CHMUXR2, 0xBA98U);
    WRITE_REG16(ADCx->CHMUXR3, 0xFEDCU);
}

/**
 * @brief  Configures synchronous mode.
 * @param  [in]  u16SyncUnit            Specify the ADC units which work synchronously.
 *                                      This parameter can be a value of @ref ADC_Sync_Unit
 * @param  [in]  u16SyncMode            Synchronous mode.
 *                                      This parameter can be a value of @ref ADC_Sync_Mode
 *   @arg  ADC_SYNC_SINGLE_DELAY_TRIG:  Single shot delayed trigger mode.
 *                                      When the trigger condition occurs, ADC1 starts first, then ADC2, last ADC3(if has).
 *                                      All ADCs scan once.
 *   @arg  ADC_SYNC_SINGLE_PARALLEL_TRIG: Single shot parallel trigger mode.
 *                                        When the trigger condition occurs, all ADCs start at the same time.
 *                                        All ADCs scan once.
 *   @arg  ADC_SYNC_CYCLIC_DELAY_TRIG:  Cyclic delayed trigger mode.
 *                                      When the trigger condition occurs, ADC1 starts first, then ADC2, last ADC3(if has).
 *                                      All ADCs scan cyclically(keep scanning till you stop them).
 *   @arg  ADC_SYNC_CYCLIC_PARALLEL_TRIG: Single shot parallel trigger mode.
 *                                        When the trigger condition occurs, all ADCs start at the same time.
 *                                        All ADCs scan cyclically(keep scanning till you stop them).
 * @param  [in]  u8TriggerDelay         Trigger delay time(ADCLK cycle), range is [1, 255].
 * @retval None
 */
void ADC_SyncModeConfig(uint16_t u16SyncUnit, uint16_t u16SyncMode, uint8_t u8TriggerDelay)
{
    DDL_ASSERT(IS_ADC_SYNC(u16SyncUnit));
    DDL_ASSERT(IS_ADC_SYNC_MD(u16SyncMode));

    u16SyncMode |= ((uint16_t)((uint32_t)u8TriggerDelay << ADC_SYNCCR_SYNCDLY_POS)) | u16SyncUnit;
    MODIFY_REG16(CM_ADC1->SYNCCR, ADC_SYNCCR_SYNCMD | ADC_SYNCCR_SYNCDLY, u16SyncMode);
}

/**
 * @brief  Enable or disable synchronous mode.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_SyncModeCmd(en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));
    WRITE_REG32(bCM_ADC1->SYNCCR_b.SYNCEN, enNewState);
}

/**
 * @brief  Configures analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be configured.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @param  [in]  u8Ch                   The channel that to be used as an analog watchdog channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @param  [in]  pstcAwd                Pointer to a @ref stc_adc_awd_config_t structure value that
 *                                      contains the configuration information of the AWD.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAwd == NULL.
 * @note  Call this function after ADC_Init().
 */
int32_t ADC_AWD_Config(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint8_t u8Ch, const stc_adc_awd_config_t *pstcAwd)
{
    uint8_t u8Pos;
    uint32_t u32AwdDr0;
    uint32_t u32AwdDr1;
    uint32_t u32AwdChsr;
    uint32_t u32Addr;
    uint16_t u16LowThreshold;
    uint16_t u16HighThreshold;
    uint16_t u16Res;
    const uint8_t au8Lshift[] = {4U, 6U, 8U, 0U};
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    DDL_ASSERT(IS_ADC_AWD_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_ADC_AWD(u8AwdUnit));

    if (pstcAwd != NULL) {
        DDL_ASSERT(IS_ADC_AWD_MD(pstcAwd->u16WatchdogMode));
        u16LowThreshold  = pstcAwd->u16LowThreshold;
        u16HighThreshold = pstcAwd->u16HighThreshold;
        u16Res = READ_REG16_BIT(ADCx->CR0, ADC_CR0_ACCSEL);
        DDL_ASSERT(IS_ADC_AWD_THRESHOLD(u16Res, u16LowThreshold));
        DDL_ASSERT(IS_ADC_AWD_THRESHOLD(u16Res, u16HighThreshold));
        if (READ_REG16_BIT(ADCx->CR0, ADC_CR0_DFMT) == ADC_DATAALIGN_LEFT) {
            u16Res >>= ADC_CR0_ACCSEL_POS;
            u16LowThreshold  <<= au8Lshift[u16Res];
            u16HighThreshold <<= au8Lshift[u16Res];
        }

        u8Pos      = (u8AwdUnit * 4U) + ADC_AWDCR_AWD0MD_POS;
        u32Addr    = (uint32_t)&ADCx->AWDCR;
        u32AwdDr0  = (uint32_t)&ADCx->AWD0DR0;
        u32AwdDr1  = (uint32_t)&ADCx->AWD0DR1;
        u32AwdChsr = (uint32_t)&ADCx->AWD0CHSR;

        WRITE_REG32(PERIPH_BIT_BAND(u32Addr, u8Pos), pstcAwd->u16WatchdogMode);
        WRITE_REG16(ADC_AWDx_DR(u8AwdUnit, u32AwdDr0), u16LowThreshold);
        WRITE_REG16(ADC_AWDx_DR(u8AwdUnit, u32AwdDr1), u16HighThreshold);
        WRITE_REG8(ADC_AWDx_CHSR(u8AwdUnit, u32AwdChsr), u8Ch);
        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Specifies combination mode of analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16CombMode            Combination mode of analog watchdog.
 *                                      This parameter can be a value of @ref ADC_AWD_Comb_Mode
 *   @arg  ADC_AWD_COMB_INVD            Combination mode is invalid.
 *   @arg  ADC_AWD_COMB_OR:             The status of AWD0 is set or the status of AWD1 is set, the status of combination mode is set.
 *   @arg  ADC_AWD_COMB_AND:            The status of AWD0 is set and the status of AWD1 is set, the status of combination mode is set.
 *   @arg  ADC_AWD_COMB_XOR:            Only one of the status of AWD0 and AWD1 is set, the status of combination mode is set.
 * @retval None
 */
void ADC_AWD_SetCombMode(CM_ADC_TypeDef *ADCx, uint16_t u16CombMode)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD_COMB_MD(u16CombMode));
    MODIFY_REG16(ADCx->AWDCR, ADC_AWDCR_AWDCM, u16CombMode);
}

/**
 * @brief  Specifies the compare mode of analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be configured.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @param  [in]  u16WatchdogMode        Analog watchdog compare mode.
 *                                      This parameter can be a value of @ref ADC_AWD_Mode
 *   @arg  ADC_AWD_MD_CMP_OUT:          ADCValue > HighThreshold or ADCValue < LowThreshold
 *   @arg  ADC_AWD_MD_CMP_IN:           LowThreshold < ADCValue < HighThreshold
 * @retval None
 */
void ADC_AWD_SetMode(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint16_t u16WatchdogMode)
{
    uint8_t u8Pos;
    uint32_t u32Addr;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD(u8AwdUnit));
    DDL_ASSERT(IS_ADC_AWD_MD(u16WatchdogMode));

    u8Pos   = (u8AwdUnit * 4U) + ADC_AWDCR_AWD0MD_POS;
    u32Addr = (uint32_t)&ADCx->AWDCR;
    WRITE_REG32(PERIPH_BIT_BAND(u32Addr, u8Pos), u16WatchdogMode);
}

/**
 * @brief  Get the compare mode of analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be configured.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @retval Analog watchdog compare mode. A value of @ref ADC_AWD_Mode
 *         - ADC_AWD_MD_CMP_OUT:        ADCValue > HighThreshold or ADCValue < LowThreshold
 *         - ADC_AWD_MD_CMP_IN:         LowThreshold < ADCValue < HighThreshold
 */
uint16_t ADC_AWD_GetMode(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit)
{
    uint16_t u16RetMode;

    uint8_t u8Pos;
    uint32_t u32Addr;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD(u8AwdUnit));

    u8Pos   = (u8AwdUnit * 4U) + ADC_AWDCR_AWD0MD_POS;
    u32Addr = (uint32_t)&ADCx->AWDCR;
    u16RetMode = (uint16_t)PERIPH_BIT_BAND(u32Addr, u8Pos);

    return u16RetMode;
}

/**
 * @brief  Specifies the low threshold and high threshold of analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be configured.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @param  [in]  u16LowThreshold        Low threshold of analog watchdog.
 * @param  [in]  u16HighThreshold       High threshold of analog watchdog.
 * @retval None
 * @note  Call this function after ADC_Init().
 */
void ADC_AWD_SetThreshold(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint16_t u16LowThreshold, uint16_t u16HighThreshold)
{
    uint32_t u32AwdDr0;
    uint32_t u32AwdDr1;
    uint16_t u16Res;
    const uint8_t au8Lshift[] = {4U, 6U, 8U, 0U};

    u16Res = READ_REG16_BIT(ADCx->CR0, ADC_CR0_ACCSEL);
    DDL_ASSERT(IS_ADC_AWD_THRESHOLD(u16Res, u16LowThreshold));
    DDL_ASSERT(IS_ADC_AWD_THRESHOLD(u16Res, u16HighThreshold));
    if (READ_REG16_BIT(ADCx->CR0, ADC_CR0_DFMT) == ADC_DATAALIGN_LEFT) {
        u16Res >>= ADC_CR0_ACCSEL_POS;
        u16LowThreshold  <<= au8Lshift[u16Res];
        u16HighThreshold <<= au8Lshift[u16Res];
    }

    u32AwdDr0 = (uint32_t)&ADCx->AWD0DR0;
    u32AwdDr1 = (uint32_t)&ADCx->AWD0DR1;
    WRITE_REG16(ADC_AWDx_DR(u8AwdUnit, u32AwdDr0), u16LowThreshold);
    WRITE_REG16(ADC_AWDx_DR(u8AwdUnit, u32AwdDr1), u16HighThreshold);
}

/**
 * @brief  Select the specified ADC channel as an analog watchdog channel.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be configured.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @param  [in]  u8Ch                   The channel that to be used as an analog watchdog channel.
 *                                      This parameter can be a value of @ref ADC_Channel
 * @retval None
 */
void ADC_AWD_SelectCh(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, uint8_t u8Ch)
{
    uint32_t u32AwdChsr;
    DDL_ASSERT(IS_ADC_AWD_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_ADC_AWD(u8AwdUnit));

    u32AwdChsr = (uint32_t)&ADCx->AWD0CHSR;
    WRITE_REG8(ADC_AWDx_CHSR(u8AwdUnit, u32AwdChsr), u8Ch);
}

/**
 * @brief  Enable or disable the specified analog watchdog.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u8AwdUnit              AWD unit that is going to be enabled or disabled.
 *                                      This parameter can be a value of @ref ADC_AWD_Unit
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_AWD_Cmd(CM_ADC_TypeDef *ADCx, uint8_t u8AwdUnit, en_functional_state_t enNewState)
{
    uint32_t u32Addr;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD(u8AwdUnit));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    u32Addr = (uint32_t)&ADCx->AWDCR;
    /* Enable bit position: u8AwdUnit * 4 */
    WRITE_REG32(PERIPH_BIT_BAND(u32Addr, (u8AwdUnit * 4UL)), enNewState);
}

/**
 * @brief  Enable or disable the specified analog watchdog interrupts.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16IntType             Interrupt of AWD.
 *                                      This parameter can be a value of @ref ADC_AWD_Int_Type
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_AWD_IntCmd(CM_ADC_TypeDef *ADCx, uint16_t u16IntType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD_INT(u16IntType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG16_BIT(ADCx->AWDCR, u16IntType);
    } else {
        CLR_REG16_BIT(ADCx->AWDCR, u16IntType);
    }
}

/**
 * @brief  Get the status of the specified analog watchdog flag.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u32Flag                AWD status flag.
 *                                      This parameter can be values of @ref ADC_AWD_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t ADC_AWD_GetStatus(const CM_ADC_TypeDef *ADCx, uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;

    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD_FLAG(u32Flag));
    if (READ_REG8_BIT(ADCx->AWDSR, u32Flag) != 0U) {
        enStatus = SET;
    }

    return enStatus;
}

/**
 * @brief  Clear the status of the specified analog watchdog flag.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u32Flag                AWD status flag.
 *                                      This parameter can be values of @ref ADC_AWD_Status_Flag
 * @retval None
 */
void ADC_AWD_ClearStatus(CM_ADC_TypeDef *ADCx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_AWD_FLAG(u32Flag));
    WRITE_REG8(ADCx->AWDSCLRR, u32Flag);
}

/* Offset correction */
/**
 * @brief  ADC offset correction configuration.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC:                      ADC instance register base.
 * @param  [in]  u8RegIndex             The index of offset register to be configured.
 *                                      This parameter can be a value between 0 and 7.
 * @param  [in]  u8Ch                   The channel that to be configured offset correction.
 *                                      This parameter can be a value of @ref ADC_Channel between:
 *                                      ADC1: ADC_CH0 ~ ADC_CH18
 *                                      ADC2: ADC_CH0 ~ ADC_CH21
 *                                      ADC3: ADC_CH0 ~ ADC_CH23
 *                                      ADC4: ADC_CH0 ~ ADC_CH11
 * @param  [in]  i16Offset              Offset value.
 *                                      This parameter can be a signed value between -4095 and +4095.
 * @param  [in]  enSatState             An @ref en_functional_state_t enumeration value.
 *   @arg  ENABLE:                      Enable offset correction saturation.
 *   @arg  DISABLE:                     Disable offset correction saturation.
 * @retval None
 */
void ADC_CorrectOffsetConfig(CM_ADC_TypeDef *ADCx, uint8_t u8RegIndex, uint8_t u8Ch, int16_t i16Offset, en_functional_state_t enSatState)
{
    uint8_t u8OffsetSign  = (uint8_t)ADC_GET_SIGN(i16Offset);
    uint16_t u16OffsetAbs = (uint16_t)ADC_GET_ABS(i16Offset);
    uint32_t u32OffsetCfg;
    __IO uint32_t *OFRx;

    DDL_ASSERT(IS_ADC_OFFSET_REG_IDX(u8RegIndex));
    DDL_ASSERT(IS_ADC_OFFSET_CH(ADCx, u8Ch));
    DDL_ASSERT(IS_ADC_OFFSET_VAL(i16Offset));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enSatState));

    OFRx = (__IO uint32_t *)&ADCx->OFR0;
    u32OffsetCfg = ((uint32_t)enSatState << ADC_OFR_SATEN_POS)       | \
                   ((uint32_t)u8OffsetSign << ADC_OFR_OFFSETPOS_POS) | \
                   ((uint32_t)u8Ch << ADC_OFR_OFFSETCH_POS)          | \
                   (uint32_t)u16OffsetAbs;
    MODIFY_REG32(OFRx[u8RegIndex], ADC_OFR_SATEN | ADC_OFR_OFFSETPOS | ADC_OFR_OFFSETCH | ADC_OFR_OFFSET, u32OffsetCfg);
}

/**
 * @brief  Enable or disable ADC offset correction.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC:                      ADC instance register base.
 * @param  [in]  u8RegIndex             The index of offset register to be enabled or disabled.
 *                                      This parameter can be a value between 0 and 7.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_CorrectOffsetCmd(CM_ADC_TypeDef *ADCx, uint8_t u8RegIndex, en_functional_state_t enNewState)
{
    __IO uint32_t *OFRx;

    DDL_ASSERT(IS_ADC_OFFSET_REG_IDX(u8RegIndex));
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    OFRx = (__IO uint32_t *)&ADCx->OFR0;
    if (ENABLE == enNewState) {
        SET_REG32_BIT(OFRx[u8RegIndex], ADC_OFR_OFFSETEN);
    } else {
        CLR_REG32_BIT(OFRx[u8RegIndex], ADC_OFR_OFFSETEN);
    }
}

/**
 * @brief  Enable or disable automatically clear data register.
 *         The automatic clearing function is mainly used to detect whether the data register is updated.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void ADC_DataRegAutoClearCmd(CM_ADC_TypeDef *ADCx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG16_BIT(ADCx->CR0, ADC_CR0_CLREN);
    } else {
        CLR_REG16_BIT(ADCx->CR0, ADC_CR0_CLREN);
    }
}

/**
 * @brief  The low priority sequence restart channel selection.
 * @param  [in]  ADCx                   Pointer to ADC instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_ADC or CM_ADCx:           ADC instance register base.
 * @param  [in]  u16SeqResumeMode       Sequence resume mode.
 *                                      This parameter can be a value of @ref ADC_Seq_Resume_Mode
 *   @arg  ADC_RESUME_SCAN_CONT:        Scanning will continue from the interrupted channel.
 *   @arg  ADC_RESUME_SCAN_RESTART:     Scanning will start from the first channel.
 * @retval None
 */
void ADC_SetSeqResumeMode(CM_ADC_TypeDef *ADCx, uint16_t u16SeqResumeMode)
{
    DDL_ASSERT(IS_ADC_UNIT(ADCx));
    DDL_ASSERT(IS_ADC_SEQ_RESUME_MD(u16SeqResumeMode));
    MODIFY_REG16(ADCx->CR1, ADC_CR1_RSCHSEL, u16SeqResumeMode);
}

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

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
