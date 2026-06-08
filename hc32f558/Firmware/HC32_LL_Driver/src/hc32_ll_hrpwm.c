/**
 *******************************************************************************
 * @file  hc32_ll_hrpwm.c
 * @brief This file provides firmware functions to manage the High Resolution
 *        Pulse-Width Modulation(HRPWM).
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
#include "hc32_ll_hrpwm.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_HRPWM HRPWM
 * @brief HRPWM Driver Library
 * @{
 */

#if (LL_HRPWM_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup HRPWM_Local_Macros HRPWM Local Macros
 * @{
 */
#define HRPWM_DMA_UNIT_REG1(reg_base, unit)         (*(__IO uint32_t *)((uint32_t)(&(reg_base)) + ((unit) * 0x08UL)))
#define HRPWM_DMA_UNIT_REG2(reg_base, unit)         (*(__IO uint32_t *)((uint32_t)(&(reg_base)) + ((unit) * 0x08UL) - 0x04UL))

/**
 * @defgroup HRPWM_Check_Param_Validity HRPWM Check Parameters Validity
 * @{
 */

/*!< Parameter valid check for HRPWM EMB flag & state */
#define IS_HRPWM_EMB_FLAG_STATE(x)                                             \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_EMB_FLAG_ALL) == HRPWM_EMB_FLAG_ALL))

/*!< Parameter valid check for HRPWM EMB clear flag */
#define IS_HRPWM_EMB_CLR_FLAG(x)                                               \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_EMB_FLAG_CLR_ALL) == HRPWM_EMB_FLAG_CLR_ALL))

/*!< Parameter valid check for HRPWM EMB noise filter clock division */
#define IS_HRPWM_EMB_NF_CLK_DIV(x)                                             \
(   ((x) == HRPWM_EMB_NF_CLK_DIV1)              ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV2)              ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV4)              ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV8)              ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV16)             ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV32)             ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV64)             ||                             \
    ((x) == HRPWM_EMB_NF_CLK_DIV128))

/*!< Parameter valid check for HRPWM EMB noise filter state */
#define IS_HRPWM_EMB_NF_STATE(x)                                               \
(   ((x) == HRPWM_EMB_NF_ON)                    ||                             \
    ((x) == HRPWM_EMB_NF_OFF))

/*!< Parameter valid check for HRPWM EMB noise filter mode */
#define IS_HRPWM_EMB_NF_MD(x)                                                  \
(   ((x) == HRPWM_EMB_NF_CNT_3)                 ||                             \
    ((x) == HRPWM_EMB_NF_CNT_2))

/*!< Parameter valid check for HRPWM EMB interrupt */
#define IS_HRPWM_EMB_INT(x)                                                    \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_EMB_INT_ALL) == HRPWM_EMB_INT_ALL))

/*!< Parameter valid check for HRPWM EMB enable event */
#define IS_HRPWM_EMB_EVT(x)                                                    \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_EMB_EVT_ALL) == HRPWM_EMB_EVT_ALL))

/*!< Parameter valid check for HRPWM EMB input selection */
#define IS_HRPWM_EMB_INP_SEL(x)                                                \
(   ((x) == HRPWM_EMB_INPUT_SEL_POTT)           ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT1)       ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT2)       ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT3))

/*!< Parameter valid check for HRPWM EMB level */
#define IS_HRPWM_EMB_LVL(x)                                                    \
(   ((x) == HRPWM_EMB_LVL_LOW)                  ||                             \
    ((x) == HRPWM_EMB_LVL_HIGH))

/*!< Parameter valid check for HRPWM EMB PWM release condition */
#define IS_HRPWM_EMB_PWM_RELEASE_COND(x)                                       \
(   ((x) == HRPWM_EMB_RELEASE_PWM_COND_FLAG_ZERO)       ||                     \
    ((x) == HRPWM_EMB_RELEASE_PWM_COND_STAT_ZERO))

/*!< Parameter valid check for HRPWM EMB blank time */
#define IS_HRPWM_EMB_BLANK_TIME(x)                                             \
(   ((x) == HRPWM_EMB_BLANK_TIME_REF)           ||                             \
    ((x) == HRPWM_EMB_BLANK_TIME_OFFSET))

/*!< Parameter valid check for HRPWM EMB accumulator reset mode */
#define IS_HRPWM_EMB_ACCUM_RST_MD(x)                                           \
(   ((x) == HRPWM_EMB_ACCUM_RST_PERIOD_POINT)   ||                             \
    ((x) == HRPWM_EMB_ACCUM_RST_PERIOD_POINT_NO_EVT))

/*!< Parameter valid check for HRPWM EMB accumulator event selection */
#define IS_HRPWM_EMB_ACCUM_EVT_SEL(x)                                          \
(   ((x) == HRPWM_EMB_ACCUM_EVT_SEL_NONE)       ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN1)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN2)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN3)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN4)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN5)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN6)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN7)    ||                             \
    ((x) == HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN8))

/*!< Parameter valid check for HRPWM EMB accumulator threshold */
#define IS_HRPWM_EMB_ACCUM_THRESHOLD(x)         ((x) <= HRPWM_EMB_CTL5_EMBACNT)

/*!< Parameter valid check for HRPWM EMB accumulator edge */
#define IS_HRPWM_EMB_ACCUM_EDGE(x)                                             \
(   ((x) == HRPWM_EMB_ACCUM_EDGE_FALLING)       ||                             \
    ((x) == HRPWM_EMB_ACCUM_EDGE_RISING))

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
 * @defgroup HRPWM_Global_Functions HRPWM Global Functions
 * @{
 */

/**
 * @brief  Calibrate process for single mode
 * @param  None
 * @retval int32_t:
 *         - LL_OK:                 Success
 *         - LL_ERR_TIMEOUT:        Time out
 *         - LL_ERR:                Failed
 * @note  Please confirm that the pclk0 is ready for the function
 */
int32_t HRPWM_CALIB_ProcessSingle(void)
{
    __IO uint32_t u32Timeout = HRPWM_CALIB_TIMEOUT;
    int32_t i32Ret = LL_OK;

    HRPWM_CALIB_SingleDisable();
    HRPWM_CALIB_PeriodDisable();
    HRPWM_CALIB_ClearFlag(HRPWM_CALIB_FLAG_END | HRPWM_CALIB_FLAG_ERR);
    HRPWM_CALIB_SingleEnable();

    while (RESET == HRPWM_CALIB_GetStatus(HRPWM_CALIB_FLAG_END)) {
        if (0UL == u32Timeout--) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }
    if (LL_OK == i32Ret) {
        if (SET == HRPWM_CALIB_GetStatus(HRPWM_CALIB_FLAG_ERR)) {
            i32Ret = LL_ERR;
        }
    }
    return i32Ret;
}

/**
 * @brief  Calibrate initialize for period mode
 * @param  [in] u32Period           Calibrate period, @ref HRPWM_Calib_Period_Define
 * @retval None
 * @note  Please confirm that the pclk0 is ready for the function
 */
void HRPWM_CALIB_PeriodInit(uint32_t u32Period)
{
    HRPWM_CALIB_SingleDisable();
    HRPWM_CALIB_PeriodDisable();
    HRPWM_CALIB_ClearFlag(HRPWM_CALIB_FLAG_END | HRPWM_CALIB_FLAG_ERR);

    HRPWM_CALIB_SetPeriod(u32Period);
    HRPWM_CALIB_PeriodEnable();
}

/**
 * @brief  Set the fields of structure stc_hrpwm_init_t to default values
 * @param  [out] pstcHrpwmInit      Pointer to a @ref stc_hrpwm_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_StructInit(stc_hrpwm_init_t *pstcHrpwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcHrpwmInit) {
        pstcHrpwmInit->u32CountMode = HRPWM_MD_SAWTOOTH;
        pstcHrpwmInit->u32CountReload = HRPWM_CNT_RELOAD_ON;
        pstcHrpwmInit->u32PeriodValue = HRPWM_REG_VALUE_MAX;
        pstcHrpwmInit->u32CountDiv    = HRPWM_CNT_CLK_DIV1;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Initialize the HRPWM count function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcHrpwmInit       Pointer of configuration structure @ref stc_hrpwm_init_t
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 * @note  Make sure the HRPWM had been calibrated before calling this function
 */
int32_t HRPWM_Init(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_init_t *pstcHrpwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcHrpwmInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_CNT_MD(pstcHrpwmInit->u32CountMode));
        DDL_ASSERT(IS_HRPWM_CNT_RELOAD_MD(pstcHrpwmInit->u32CountReload));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(pstcHrpwmInit->u32PeriodValue));

        WRITE_REG32(HRPWMx->HRPERAR, pstcHrpwmInit->u32PeriodValue);
        MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_MODE | HRPWM_GCONR_RETRIG | HRPWM_GCONR_CKDIV, \
                     pstcHrpwmInit->u32CountMode | pstcHrpwmInit->u32CountReload | pstcHrpwmInit->u32CountDiv);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  De-initialize the HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
void HRPWM_DeInit(CM_HRPWM_TypeDef *HRPWMx)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    WRITE_REG32(HRPWMx->GCONR, 0UL);

    WRITE_REG32(HRPWMx->CNTER, 0UL);
    WRITE_REG32(HRPWMx->UPDAR, 0UL);
    WRITE_REG32(HRPWMx->HRPERAR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRPERBR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMAR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMBR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMCR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMDR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMER, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMFR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMGR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRGCMHR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->SCMAR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->SCMBR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->SCMCR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->SCMDR, HRPWM_REG_VALUE_MAX);

    WRITE_REG32(HRPWMx->HRDTUAR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRDTDAR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRDTUBR, HRPWM_REG_VALUE_MAX);
    WRITE_REG32(HRPWMx->HRDTDBR, HRPWM_REG_VALUE_MAX);

    WRITE_REG32(HRPWMx->ICONR, 0UL);
    WRITE_REG32(HRPWMx->BCONR1, 0UL);
    WRITE_REG32(HRPWMx->DCONR, 0UL);
    WRITE_REG32(HRPWMx->PCNAR1, 0UL);
    WRITE_REG32(HRPWMx->PCNBR1, 0UL);
    WRITE_REG32(HRPWMx->VPERR, 0UL);
    WRITE_REG32(HRPWMx->STFLR1, 0x80000000UL);
    WRITE_REG32(HRPWMx->STFLR2, 0UL);
    WRITE_REG32(HRPWMx->HSTAR1, 0UL);
    WRITE_REG32(HRPWMx->HCLRR1, 0UL);
    WRITE_REG32(HRPWMx->HCPAR1, 0UL);
    WRITE_REG32(HRPWMx->HCPBR1, 0UL);
    WRITE_REG32(HRPWMx->HCPAR2, 0UL);
    WRITE_REG32(HRPWMx->HCPBR2, 0UL);
    WRITE_REG32(HRPWMx->BCONR2, 0UL);

    WRITE_REG32(HRPWMx->PCNAR2, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->PCNBR2, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->HSTAR2, 0UL);
    WRITE_REG32(HRPWMx->HCLRR2, 0UL);

    WRITE_REG32(HRPWMx->EEFOFFSETAR, 0x00400000UL);
    WRITE_REG32(HRPWMx->EEFOFFSETBR, 0x00400000UL);
    WRITE_REG32(HRPWMx->EEFWINAR, 0x00400000UL);
    WRITE_REG32(HRPWMx->EEFWINBR, 0x00400000UL);
    WRITE_REG32(HRPWMx->EEFLTCR1, 0UL);
    WRITE_REG32(HRPWMx->EEFLTCR2, 0UL);
    WRITE_REG32(HRPWMx->PCNAR3, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->PCNBR3, 0x000AAAAAUL);

    WRITE_REG32(HRPWMx->IDLECR, 0UL);
    WRITE_REG32(HRPWMx->GCONR1, 0UL);

    WRITE_REG32(HRPWMx->BICONR, 0UL);
    WRITE_REG32(HRPWMx->BPCNAR1, 0UL);
    WRITE_REG32(HRPWMx->BPCNBR1, 0UL);
    WRITE_REG32(HRPWMx->BPCNAR2, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->BPCNBR2, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->BPCNAR3, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->BPCNBR3, 0x000AAAAAUL);
    WRITE_REG32(HRPWMx->BGCONR1, 0UL);
    WRITE_REG32(HRPWMx->CR, 0UL);
    WRITE_REG32(HRPWMx->PHSCTL, 0UL);
}

/**
 * @brief  De-initialize the HRPWM COMMON register
 * @param  None
 * @retval None
 */
void HRPWM_CommonDeInit(void)
{
    WRITE_REG32(CM_HRPWM_COMMON->CALCR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SCAPR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SSTAIDLR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SSTARUNR1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SSTADIDLR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GCTLR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GBCONR, 0x0000003FUL);
    WRITE_REG32(CM_HRPWM_COMMON->GBSFLR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BMCR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BMSTRG1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BMSTRG2, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BMPERAR, 0x0000FFFFUL);
    WRITE_REG32(CM_HRPWM_COMMON->BMPERBR, 0x0000FFFFUL);
    WRITE_REG32(CM_HRPWM_COMMON->BMCMAR, 0x0000FFFFUL);
    WRITE_REG32(CM_HRPWM_COMMON->BMCMBR, 0x0000FFFFUL);
    WRITE_REG32(CM_HRPWM_COMMON->EECR1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->EECR2, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->EECR3, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SYNOCR, 0x00001000UL);
    WRITE_REG32(CM_HRPWM_COMMON->FCNTR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SSTAR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SSTPR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SCLRR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->SUPDR, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR2, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR3, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR4, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR5, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR6, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSSELR7, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSUR1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSUR2, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSPSCR1, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->GLAOSPSCR2, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR11, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR12, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR21, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR22, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR31, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR32, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR41, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR42, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR51, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR52, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR61, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR62, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR71, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR72, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR81, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDUPR82, 0UL);
    WRITE_REG32(CM_HRPWM_COMMON->BDMADR, 0UL);
}

/**
 * @brief  De-initialize the HRPWM unit
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @retval None
 */
void HRPWM_EMB_DeInit(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx)
{
    WRITE_REG32(HRPWM_EMBx->INTEN, 0UL);
    WRITE_REG32(HRPWM_EMBx->RLSSEL, 0UL);
    WRITE_REG32(HRPWM_EMBx->CTL5, 0UL);
    WRITE_REG32(HRPWM_EMBx->CTL6, 0UL);
}

/**
 * @brief  Set the fields of structure stc_hrpwm_pwm_init_t to default values
 * @param  [out] pstcPwmInit        Pointer to a @ref stc_hrpwm_pwm_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_StructInit(stc_hrpwm_pwm_init_t *pstcPwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcPwmInit) {
        pstcPwmInit->u32CompareValue = HRPWM_REG_VALUE_MAX;
        pstcPwmInit->u32StartPolarity = HRPWM_PWM_START_LOW;
        pstcPwmInit->u32StopPolarity = HRPWM_PWM_STOP_LOW;
        pstcPwmInit->u32PeakPolarity = HRPWM_PWM_PEAK_LOW;
        pstcPwmInit->u32ValleyPolarity = HRPWM_PWM_VALLEY_LOW;
        pstcPwmInit->u32UpMatchAPolarity = HRPWM_PWM_UP_MATCH_A_LOW;
        pstcPwmInit->u32DownMatchAPolarity = HRPWM_PWM_DOWN_MATCH_A_LOW;
        pstcPwmInit->u32UpMatchBPolarity = HRPWM_PWM_UP_MATCH_B_LOW;
        pstcPwmInit->u32DownMatchBPolarity = HRPWM_PWM_DOWN_MATCH_B_LOW;
        pstcPwmInit->u32UpMatchEPolarity = HRPWM_PWM_UP_MATCH_E_LOW;
        pstcPwmInit->u32DownMatchEPolarity = HRPWM_PWM_DOWN_MATCH_E_LOW;
        pstcPwmInit->u32UpMatchFPolarity = HRPWM_PWM_UP_MATCH_F_LOW;
        pstcPwmInit->u32DownMatchFPolarity = HRPWM_PWM_DOWN_MATCH_F_LOW;
        pstcPwmInit->u32UpMatchSpecialAPolarity = HRPWM_PWM_UP_MATCH_SPECIAL_A_LOW;
        pstcPwmInit->u32DownMatchSpecialAPolarity = HRPWM_PWM_DOWN_MATCH_SPECIAL_A_LOW;
        pstcPwmInit->u32UpMatchSpecialBPolarity = HRPWM_PWM_UP_MATCH_SPECIAL_B_LOW;
        pstcPwmInit->u32DownMatchSpecialBPolarity = HRPWM_PWM_DOWN_MATCH_SPECIAL_B_LOW;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM Channel A function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmInit         Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_ChAInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(pstcPwmInit->u32CompareValue));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(pstcPwmInit->u32StartPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(pstcPwmInit->u32StopPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(pstcPwmInit->u32PeakPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(pstcPwmInit->u32ValleyPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(pstcPwmInit->u32UpMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(pstcPwmInit->u32DownMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(pstcPwmInit->u32UpMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(pstcPwmInit->u32DownMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(pstcPwmInit->u32UpMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(pstcPwmInit->u32DownMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(pstcPwmInit->u32UpMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(pstcPwmInit->u32DownMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(pstcPwmInit->u32UpMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(pstcPwmInit->u32DownMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(pstcPwmInit->u32UpMatchSpecialBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(pstcPwmInit->u32DownMatchSpecialBPolarity));

        WRITE_REG32(HRPWMx->HRGCMAR, pstcPwmInit->u32CompareValue);
        MODIFY_REG32(HRPWMx->PCNAR1, PCNAR1_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32StartPolarity
                     | pstcPwmInit->u32StopPolarity
                     | pstcPwmInit->u32PeakPolarity
                     | pstcPwmInit->u32ValleyPolarity
                     | pstcPwmInit->u32UpMatchAPolarity
                     | pstcPwmInit->u32DownMatchAPolarity
                     | pstcPwmInit->u32UpMatchBPolarity
                     | pstcPwmInit->u32DownMatchBPolarity);
        MODIFY_REG32(HRPWMx->PCNAR2, PCNAR2_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32UpMatchEPolarity
                     | pstcPwmInit->u32UpMatchFPolarity
                     | pstcPwmInit->u32UpMatchSpecialAPolarity
                     | pstcPwmInit->u32UpMatchSpecialBPolarity);
        MODIFY_REG32(HRPWMx->PCNAR3, PCNAR3_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32DownMatchEPolarity
                     | pstcPwmInit->u32DownMatchFPolarity
                     | pstcPwmInit->u32DownMatchSpecialAPolarity
                     | pstcPwmInit->u32DownMatchSpecialBPolarity);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM Channel A function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmInit         Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_ChAInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(pstcPwmInit->u32CompareValue));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(pstcPwmInit->u32StartPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(pstcPwmInit->u32StopPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(pstcPwmInit->u32PeakPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(pstcPwmInit->u32ValleyPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(pstcPwmInit->u32UpMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(pstcPwmInit->u32DownMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(pstcPwmInit->u32UpMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(pstcPwmInit->u32DownMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(pstcPwmInit->u32UpMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(pstcPwmInit->u32DownMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(pstcPwmInit->u32UpMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(pstcPwmInit->u32DownMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(pstcPwmInit->u32UpMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(pstcPwmInit->u32DownMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(pstcPwmInit->u32UpMatchSpecialBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(pstcPwmInit->u32DownMatchSpecialBPolarity));

        WRITE_REG32(HRPWMx->HRGCMCR, pstcPwmInit->u32CompareValue);
        MODIFY_REG32(HRPWMx->BPCNAR1, PCNAR1_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32StartPolarity
                     | pstcPwmInit->u32StopPolarity
                     | pstcPwmInit->u32PeakPolarity
                     | pstcPwmInit->u32ValleyPolarity
                     | pstcPwmInit->u32UpMatchAPolarity
                     | pstcPwmInit->u32DownMatchAPolarity
                     | pstcPwmInit->u32UpMatchBPolarity
                     | pstcPwmInit->u32DownMatchBPolarity);
        MODIFY_REG32(HRPWMx->BPCNAR2, PCNAR2_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32UpMatchEPolarity
                     | pstcPwmInit->u32UpMatchFPolarity
                     | pstcPwmInit->u32UpMatchSpecialAPolarity
                     | pstcPwmInit->u32UpMatchSpecialBPolarity);
        MODIFY_REG32(HRPWMx->BPCNAR3, PCNAR3_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32DownMatchEPolarity
                     | pstcPwmInit->u32DownMatchFPolarity
                     | pstcPwmInit->u32DownMatchSpecialAPolarity
                     | pstcPwmInit->u32DownMatchSpecialBPolarity);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM Channel B function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmInit         Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_ChBInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(pstcPwmInit->u32CompareValue));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(pstcPwmInit->u32StartPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(pstcPwmInit->u32StopPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(pstcPwmInit->u32PeakPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(pstcPwmInit->u32ValleyPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(pstcPwmInit->u32UpMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(pstcPwmInit->u32DownMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(pstcPwmInit->u32UpMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(pstcPwmInit->u32DownMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(pstcPwmInit->u32UpMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(pstcPwmInit->u32DownMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(pstcPwmInit->u32UpMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(pstcPwmInit->u32DownMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(pstcPwmInit->u32UpMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(pstcPwmInit->u32DownMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(pstcPwmInit->u32UpMatchSpecialBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(pstcPwmInit->u32DownMatchSpecialBPolarity));

        WRITE_REG32(HRPWMx->HRGCMBR, pstcPwmInit->u32CompareValue);
        MODIFY_REG32(HRPWMx->PCNBR1, PCNBR1_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32StartPolarity
                     | pstcPwmInit->u32StopPolarity
                     | pstcPwmInit->u32PeakPolarity
                     | pstcPwmInit->u32ValleyPolarity
                     | pstcPwmInit->u32UpMatchAPolarity
                     | pstcPwmInit->u32DownMatchAPolarity
                     | pstcPwmInit->u32UpMatchBPolarity
                     | pstcPwmInit->u32DownMatchBPolarity);
        MODIFY_REG32(HRPWMx->PCNBR2, PCNBR2_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32UpMatchEPolarity
                     | pstcPwmInit->u32UpMatchFPolarity
                     | pstcPwmInit->u32UpMatchSpecialAPolarity
                     | pstcPwmInit->u32UpMatchSpecialBPolarity);
        MODIFY_REG32(HRPWMx->PCNBR3, PCNBR3_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32DownMatchEPolarity
                     | pstcPwmInit->u32DownMatchFPolarity
                     | pstcPwmInit->u32DownMatchSpecialAPolarity
                     | pstcPwmInit->u32DownMatchSpecialBPolarity);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM Channel B function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmInit         Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_ChBInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(pstcPwmInit->u32CompareValue));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(pstcPwmInit->u32StartPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(pstcPwmInit->u32StopPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(pstcPwmInit->u32PeakPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(pstcPwmInit->u32ValleyPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(pstcPwmInit->u32UpMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(pstcPwmInit->u32DownMatchAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(pstcPwmInit->u32UpMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(pstcPwmInit->u32DownMatchBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(pstcPwmInit->u32UpMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(pstcPwmInit->u32DownMatchEPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(pstcPwmInit->u32UpMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(pstcPwmInit->u32DownMatchFPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(pstcPwmInit->u32UpMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(pstcPwmInit->u32DownMatchSpecialAPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(pstcPwmInit->u32UpMatchSpecialBPolarity));
        DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(pstcPwmInit->u32DownMatchSpecialBPolarity));

        WRITE_REG32(HRPWMx->HRGCMDR, pstcPwmInit->u32CompareValue);
        MODIFY_REG32(HRPWMx->BPCNBR1, PCNBR1_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32StartPolarity
                     | pstcPwmInit->u32StopPolarity
                     | pstcPwmInit->u32PeakPolarity
                     | pstcPwmInit->u32ValleyPolarity
                     | pstcPwmInit->u32UpMatchAPolarity
                     | pstcPwmInit->u32DownMatchAPolarity
                     | pstcPwmInit->u32UpMatchBPolarity
                     | pstcPwmInit->u32DownMatchBPolarity);
        MODIFY_REG32(HRPWMx->BPCNBR2, PCNBR2_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32UpMatchEPolarity
                     | pstcPwmInit->u32UpMatchFPolarity
                     | pstcPwmInit->u32UpMatchSpecialAPolarity
                     | pstcPwmInit->u32UpMatchSpecialBPolarity);
        MODIFY_REG32(HRPWMx->BPCNBR3, PCNBR3_REG_POLARITY_CFG_MASK,
                     pstcPwmInit->u32DownMatchEPolarity
                     | pstcPwmInit->u32DownMatchFPolarity
                     | pstcPwmInit->u32DownMatchSpecialAPolarity
                     | pstcPwmInit->u32DownMatchSpecialBPolarity);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_pwm_output_init_t to default values
 * @param  [out] pstcPwmOutputInit  Pointer to a @ref stc_hrpwm_pwm_output_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_OutputStructInit(stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcPwmOutputInit) {
        pstcPwmOutputInit->u32ChSwapMode = HRPWM_PWM_CH_SWAP_MD_NOT_IMMED;
        pstcPwmOutputInit->u32ChSwap = HRPWM_PWM_CH_SWAP_OFF;
        pstcPwmOutputInit->u32ChAInvert = HRPWM_PWM_CHA_INVT_OFF;
        pstcPwmOutputInit->u32ChBInvert = HRPWM_PWM_CHB_INVT_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM output function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmOutputInit   Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_OutputInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmOutputInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_PWM_SWAP_MD(pstcPwmOutputInit->u32ChSwapMode));
        DDL_ASSERT(IS_HRPWM_PWM_SWAP(pstcPwmOutputInit->u32ChSwap));
        DDL_ASSERT(IS_HRPWM_PWM_CHA_INVT(pstcPwmOutputInit->u32ChAInvert));
        DDL_ASSERT(IS_HRPWM_PWM_CHB_INVT(pstcPwmOutputInit->u32ChBInvert));

        MODIFY_REG32(HRPWMx->GCONR1,
                     HRPWM_GCONR1_SWAPMD | HRPWM_GCONR1_SWAPEN | HRPWM_GCONR1_INVCAEN | HRPWM_GCONR1_INVCBEN,
                     pstcPwmOutputInit->u32ChSwapMode
                     | pstcPwmOutputInit->u32ChSwap
                     | pstcPwmOutputInit->u32ChAInvert
                     | pstcPwmOutputInit->u32ChBInvert);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize PWM output function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcPwmOutputInit   Pointer of initialize structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PWM_OutputInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcPwmOutputInit) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_PWM_SWAP_MD(pstcPwmOutputInit->u32ChSwapMode));
        DDL_ASSERT(IS_HRPWM_PWM_SWAP(pstcPwmOutputInit->u32ChSwap));
        DDL_ASSERT(IS_HRPWM_PWM_CHA_INVT(pstcPwmOutputInit->u32ChAInvert));
        DDL_ASSERT(IS_HRPWM_PWM_CHB_INVT(pstcPwmOutputInit->u32ChBInvert));

        MODIFY_REG32(HRPWMx->BGCONR1,
                     HRPWM_GCONR1_SWAPMD | HRPWM_GCONR1_SWAPEN | HRPWM_GCONR1_INVCAEN | HRPWM_GCONR1_INVCBEN,
                     pstcPwmOutputInit->u32ChSwapMode
                     | pstcPwmOutputInit->u32ChSwap
                     | pstcPwmOutputInit->u32ChAInvert
                     | pstcPwmOutputInit->u32ChBInvert);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM trigger pin input filter clock configuration
 * @param  [in] u32Pin              Pin to be configured
 *   @arg  HRPWM_INPUT_TRIGA
 *   @arg  HRPWM_INPUT_TRIGB
 *   @arg  HRPWM_INPUT_TRIGC
 *   @arg  HRPWM_INPUT_TRIGD
 * @param  [in] u32Div              Filter clock @ref HRPWM_Input_Filter_Clock_Define
 * @retval None
 */
void HRPWM_TriggerPinSetFilterClock(uint32_t u32Pin, uint32_t u32Div)
{
    DDL_ASSERT(IS_HRPWM_TRIG_PIN(u32Pin));
    DDL_ASSERT(IS_HRPWM_FILTER_CLK(u32Div));

    switch (u32Pin) {
        case HRPWM_INPUT_TRIGA:
            MODIFY_REG32(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFICKTA, u32Div << HRPWM_COMMON_FCNTR_NOFICKTA_POS);
            break;
        case HRPWM_INPUT_TRIGB:
            MODIFY_REG32(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFICKTB, u32Div << HRPWM_COMMON_FCNTR_NOFICKTB_POS);
            break;
        case HRPWM_INPUT_TRIGC:
            MODIFY_REG32(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFICKTC, u32Div << HRPWM_COMMON_FCNTR_NOFICKTC_POS);
            break;
        case HRPWM_INPUT_TRIGD:
            MODIFY_REG32(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFICKTD, u32Div << HRPWM_COMMON_FCNTR_NOFICKTD_POS);
            break;
        default:
            break;
    }
}

/**
 * @brief  HRPWM trigger pin filter function enable
 * @param  [in] u32Pin              Pin to be configured
 *   @arg  HRPWM_INPUT_TRIGA
 *   @arg  HRPWM_INPUT_TRIGB
 *   @arg  HRPWM_INPUT_TRIGC
 *   @arg  HRPWM_INPUT_TRIGD
 * @retval None
 */
void HRPWM_TriggerPinFilterEnable(uint32_t u32Pin)
{
    DDL_ASSERT(IS_HRPWM_TRIG_PIN(u32Pin));
    switch (u32Pin) {
        case HRPWM_INPUT_TRIGA:
            SET_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTA);
            break;
        case HRPWM_INPUT_TRIGB:
            SET_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTB);
            break;
        case HRPWM_INPUT_TRIGC:
            SET_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTC);
            break;
        case HRPWM_INPUT_TRIGD:
            SET_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTD);
            break;
        default:
            break;
    }
}

/**
 * @brief  HRPWM trigger pin filter function disable
 * @param  [in] u32Pin              Pin to be configured
 *   @arg  HRPWM_INPUT_TRIGA
 *   @arg  HRPWM_INPUT_TRIGB
 *   @arg  HRPWM_INPUT_TRIGC
 *   @arg  HRPWM_INPUT_TRIGD
 * @retval None
 */
void HRPWM_TriggerPinFilterDisable(uint32_t u32Pin)
{
    DDL_ASSERT(IS_HRPWM_TRIG_PIN(u32Pin));
    switch (u32Pin) {
        case HRPWM_INPUT_TRIGA:
            CLR_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTA);
            break;
        case HRPWM_INPUT_TRIGB:
            CLR_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTB);
            break;
        case HRPWM_INPUT_TRIGC:
            CLR_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTC);
            break;
        case HRPWM_INPUT_TRIGD:
            CLR_REG32_BIT(CM_HRPWM_COMMON->FCNTR, HRPWM_COMMON_FCNTR_NOFIENTD);
            break;
        default:
            break;
    }
}

/**
 * @brief  Set the fields of structure stc_hrpwm_deadtime_config_t to default values
 * @param  [out] pstcDeadTimeConfig Pointer to a @ref stc_hrpwm_deadtime_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_DeadTimeStructInit(stc_hrpwm_deadtime_config_t *pstcDeadTimeConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcDeadTimeConfig) {
        pstcDeadTimeConfig->u32EqualUpDown = HRPWM_DEADTIME_EQUAL_OFF;
        pstcDeadTimeConfig->u32BufUp = HRPWM_DEADTIME_CNT_UP_BUF_OFF;
        pstcDeadTimeConfig->u32BufDown = HRPWM_DEADTIME_CNT_DOWN_BUF_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Dead time function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in]  pstcDeadTimeConfig Pointer to a @ref stc_hrpwm_deadtime_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 * @note  Please notice that HRPWM_DeadTimeBufConfig() function should be executed before call this function
 */
int32_t HRPWM_DeadTimeConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_deadtime_config_t *pstcDeadTimeConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcDeadTimeConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DEADTIME_EQUAL_FUNC(pstcDeadTimeConfig->u32EqualUpDown));
        DDL_ASSERT(IS_HRPWM_DEADTIME_BUF_FUNC_DTUAR(pstcDeadTimeConfig->u32BufUp));
        DDL_ASSERT(IS_HRPWM_DEADTIME_BUF_FUNC_DTDAR(pstcDeadTimeConfig->u32BufDown));

        MODIFY_REG32(HRPWMx->DCONR, HRPWM_DCONR_SEPA | HRPWM_DCONR_DTBENU | HRPWM_DCONR_DTBEND,
                     pstcDeadTimeConfig->u32EqualUpDown | pstcDeadTimeConfig->u32BufUp | pstcDeadTimeConfig->u32BufDown);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_buf_config_t to default values
 * @param  [out] pstcBufConfig      Pointer to a @ref stc_hrpwm_buf_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_BufStructInit(stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBufConfig) {
        pstcBufConfig->u32BufTransCond = HRPWM_BUF_TRANS_INVD;
        pstcBufConfig->enBufTransU1Single = DISABLE;
        pstcBufConfig->enBufTransAfterU1Single = DISABLE;
        pstcBufConfig->enGlobalBufTrans = DISABLE;

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM general compare register A and E buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_GeneralAEBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUAE | HRPWM_BCONR1_BTRDAE,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUAE_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PAE | HRPWM_BCONR2_BTRU0AE,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PAE_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0AE_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_GCMAER,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_GCMAER_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM general compare register B and F buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_GeneralBFBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUBF | HRPWM_BCONR1_BTRDBF,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUBF_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PBF | HRPWM_BCONR2_BTRU0BF,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PBF_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0BF_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_GCMBFR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_GCMBFR_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM period buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PeriodBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUP | HRPWM_BCONR1_BTRDP,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUP_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PP | HRPWM_BCONR2_BTRU0P,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PP_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0P_POS);
        }
        /* Configure global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_PERAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_GCMAER_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM special compare register A buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_SpecialABufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUSPA | HRPWM_BCONR1_BTRDSPA,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUSPA_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PSPA | HRPWM_BCONR2_BTRU0SPA,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PSPA_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0SPA_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_SCMAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_SCMAR_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM special compare register B buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_SpecialBBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUSPB | HRPWM_BCONR1_BTRDSPB,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUSPB_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PSPB | HRPWM_BCONR2_BTRU0SPB,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PSPB_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0SPB_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_SCMBR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_SCMBR_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM port control buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM8
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PortBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }

        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUPCN | HRPWM_BCONR1_BTRDPCN,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUPCN_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~8 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PPCN | HRPWM_BCONR2_BTRU0PCN,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PPCN_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0PCN_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_PCN,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_PCN_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM valid period buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM8
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_ValidPeriodBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }

        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRUVPE | HRPWM_BCONR1_BTRDVPE,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR1_BTRUVPE_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~8 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR1, HRPWM_BCONR1_BTRU0VPE | HRPWM_BCONR1_BTRU0PVPE,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR1_BTRU0VPE_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR1_BTRU0PVPE_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_VPER,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_VPER_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM external event filter window register buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EVT_WindowBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRUEEFWIN | HRPWM_BCONR2_BTRDEEFWIN,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR2_BTRUEEFWIN_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PEEFWIN | HRPWM_BCONR2_BTRU0EEFWIN,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PEEFWIN_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0EEFWIN_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_EEFWINAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_EEFWINAR_POS);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM external event filter offset register buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EVT_OffsetBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRUEEFOFF | HRPWM_BCONR2_BTRDEEFOFF,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR2_BTRUEEFOFF_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PEEFOFF | HRPWM_BCONR2_BTRU0EEFOFF,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PEEFOFF_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0EEFOFF_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_EEFOFFSETAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_EEFOFFSETAR_POS);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM control register buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_ControlRegBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRUCTL | HRPWM_BCONR2_BTRDCTL,
                     pstcBufConfig->u32BufTransCond << HRPWM_BCONR2_BTRUCTL_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PCTL | HRPWM_BCONR2_BTRU0CTL,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PCTL_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0CTL_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_CTLR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_CTLR_POS);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM dead time register buffer function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_DeadTimeBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));
        if (CM_HRPWM1 != HRPWMx) {
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransU1Single));
            DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enBufTransAfterU1Single));
        }
        /* Config normal buffer transfer condition */
        MODIFY_REG32(HRPWMx->DCONR, HRPWM_DCONR_DTBTRU | HRPWM_DCONR_DTBTRD,
                     pstcBufConfig->u32BufTransCond << HRPWM_DCONR_DTBTRU_POS);
        if (CM_HRPWM1 != HRPWMx) {
            /* Config HRPWM2~6 buffer single transfer function */
            MODIFY_REG32(HRPWMx->BCONR2, HRPWM_BCONR2_BTRU0PDT | HRPWM_BCONR2_BTRU0DT,
                         (uint32_t)pstcBufConfig->enBufTransU1Single << HRPWM_BCONR2_BTRU0PDT_POS |
                         (uint32_t)pstcBufConfig->enBufTransAfterU1Single << HRPWM_BCONR2_BTRU0DT_POS);
        }
        /* Config global buffer transfer function */
        MODIFY_REG32(HRPWMx->GBCFG, HRPWM_GBCFG_DTAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_DTAR_POS);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_idle_delay_init_t to default values
 * @param  [out] pstcIdleDelayInit  Pointer to a @ref stc_hrpwm_idle_delay_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_DELAY_StructInit(stc_hrpwm_idle_delay_init_t *pstcIdleDelayInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcIdleDelayInit) {
        pstcIdleDelayInit->u32TriggerSrc = HRPWM_IDLE_DELAY_TRIG_EVT6;
        pstcIdleDelayInit->u32PeriodPoint = HRPWM_CPLT_PERIOD_SAWTOOTH_PEAK_TRIANGLE_VALLEY;
        pstcIdleDelayInit->u32IdleOutputChAStatus = HRPWM_IDLE_OUTPUT_CHA_OFF;
        pstcIdleDelayInit->u32IdleOutputChBStatus = HRPWM_IDLE_OUTPUT_CHB_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM initialize the idle delay output function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcIdleDelayInit   Pointer to a @ref stc_hrpwm_idle_delay_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_DELAY_Init(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_idle_delay_init_t *pstcIdleDelayInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    /* Check structure pointer */
    if (NULL != pstcIdleDelayInit) {
        DDL_ASSERT(IS_HRPWM_IDLE_DELAY_TRIG_SRC(pstcIdleDelayInit->u32TriggerSrc));
        DDL_ASSERT(IS_HRPWM_CPLT_PERIOD_POINT(pstcIdleDelayInit->u32PeriodPoint));
        DDL_ASSERT(IS_HRPWM_IDLE_OUTPUT_CHA_STAT(pstcIdleDelayInit->u32IdleOutputChAStatus));
        DDL_ASSERT(IS_HRPWM_IDLE_OUTPUT_CHB_STAT(pstcIdleDelayInit->u32IdleOutputChBStatus));

        MODIFY_REG32(HRPWMx->IDLECR,
                     HRPWM_IDLECR_DLYEVSEL | HRPWM_IDLECR_DLYCHA | HRPWM_IDLECR_DLYCHB,
                     pstcIdleDelayInit->u32TriggerSrc | pstcIdleDelayInit->u32IdleOutputChAStatus |
                     pstcIdleDelayInit->u32IdleOutputChBStatus);
        MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_PRDSEL, pstcIdleDelayInit->u32PeriodPoint);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_idle_bm_init_t to default values
 * @param  [out] pstcBMInit       Pointer to a @ref stc_hrpwm_idle_bm_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_BM_StructInit(stc_hrpwm_idle_bm_init_t *pstcBMInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBMInit) {
        pstcBMInit->u32Mode = HRPWM_BM_ACTION_MD1;
        pstcBMInit->u32CountSrc = HRPWM_BM_CNT_SRC_PEROID_POINT_U1;
        pstcBMInit->u32CountPclkDiv = HRPWM_BM_CNT_SRC_PCLK_DIV1;
        pstcBMInit->u32PeriodValue = 0x00UL;
        pstcBMInit->u32CompareValue = 0x00UL;
        pstcBMInit->u32CountReload = HRPWM_BM_CNT_RELOAD_OFF;
        pstcBMInit->u64TriggerSrc = HRPWM_BM_TRIG_NONE;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Idle BM function configuration
 * @param  [in] pstcBMInit          Point a structure @ref stc_hrpwm_idle_bm_init_t
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_BM_Init(stc_hrpwm_idle_bm_init_t *pstcBMInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBMInit) {
        DDL_ASSERT(IS_HRPWM_BM_ACTION_MD(pstcBMInit->u32Mode));
        DDL_ASSERT(IS_HRPWM_BM_CNT_SRC(pstcBMInit->u32CountSrc));
        DDL_ASSERT(IS_HRPWM_BM_CNT_PCLK0_DIV(pstcBMInit->u32CountPclkDiv));
        DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(pstcBMInit->u32PeriodValue));
        DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(pstcBMInit->u32CompareValue));
        DDL_ASSERT(IS_HRPWM_BM_CNT_RELOAD(pstcBMInit->u32CountReload));
        DDL_ASSERT(IS_HRPWM_BM_TRIG_SRC(pstcBMInit->u64TriggerSrc));

        MODIFY_REG32(CM_HRPWM_COMMON->BMCR,
                     HRPWM_COMMON_BMCR_BMMD | HRPWM_COMMON_BMCR_BMCLKS | HRPWM_COMMON_BMCR_BMPSC | HRPWM_COMMON_BMCR_BMCTN | HRPWM_BM_FLAG_ALL,
                     pstcBMInit->u32Mode | pstcBMInit->u32CountSrc | pstcBMInit->u32CountPclkDiv |
                     pstcBMInit->u32CountReload | HRPWM_BM_FLAG_ALL);
        WRITE_REG32(CM_HRPWM_COMMON->BMPERAR, pstcBMInit->u32PeriodValue);
        WRITE_REG32(CM_HRPWM_COMMON->BMCMAR, pstcBMInit->u32CompareValue);
        WRITE_REG32(CM_HRPWM_COMMON->BMSTRG1, pstcBMInit->u64TriggerSrc & 0xFFFFFFFFUL);
        WRITE_REG32(CM_HRPWM_COMMON->BMSTRG2, (pstcBMInit->u64TriggerSrc >> 32U) & 0xFFFFFFFFUL);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_idle_bm_output_init_t to default values
 * @param  [out] pstcBMOutputInit   Pointer to a @ref stc_hrpwm_idle_bm_output_init_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_BM_OutputStructInit(stc_hrpwm_idle_bm_output_init_t *pstcBMOutputInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBMOutputInit) {
        pstcBMOutputInit->u32IdleOutputChAStatus = HRPWM_BM_OUTPUT_CHA_OFF;
        pstcBMOutputInit->u32IdleOutputChBStatus = HRPWM_BM_OUTPUT_CHB_OFF;
        pstcBMOutputInit->u32IdleChAEnterDelay = HRPWM_BM_DELAY_ENTER_CHA_OFF;
        pstcBMOutputInit->u32IdleChBEnterDelay = HRPWM_BM_DELAY_ENTER_CHB_OFF;
        pstcBMOutputInit->u32Follow = HRPWM_BM_FOLLOW_FUNC_OFF;
        pstcBMOutputInit->u32UnitCountReset = HRPWM_BM_UNIT_CNT_CONTINUE;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Idle BM output function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcBMOutputInit    Point a structure @ref stc_hrpwm_idle_bm_output_init_t
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_IDLE_BM_OutputInit(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_idle_bm_output_init_t *pstcBMOutputInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    uint32_t u32UnitNum;
    /* Check structure pointer */
    if (NULL != pstcBMOutputInit) {
        DDL_ASSERT(IS_HRPWM_BM_OUTPUT_CHA_STAT(pstcBMOutputInit->u32IdleOutputChAStatus));
        DDL_ASSERT(IS_HRPWM_BM_OUTPUT_CHB_STAT(pstcBMOutputInit->u32IdleOutputChBStatus));
        DDL_ASSERT(IS_HRPWM_BM_DELAY_ENTER_CHA_STAT(pstcBMOutputInit->u32IdleChAEnterDelay));
        DDL_ASSERT(IS_HRPWM_BM_DELAY_ENTER_CHB_STAT(pstcBMOutputInit->u32IdleChBEnterDelay));
        DDL_ASSERT(IS_HRPWM_BM_FOLLOW_FUNC(pstcBMOutputInit->u32Follow));
        DDL_ASSERT(IS_HRPWM_BM_UNIT_CNT_RST_FUNC(pstcBMOutputInit->u32UnitCountReset));

        MODIFY_REG32(HRPWMx->IDLECR,
                     HRPWM_IDLECR_IDLEBMA | HRPWM_IDLECR_IDLEBMB |
                     HRPWM_IDLECR_DIDLA | HRPWM_IDLECR_DIDLB | HRPWM_IDLECR_FOLLOW,
                     pstcBMOutputInit->u32IdleOutputChAStatus | pstcBMOutputInit->u32IdleOutputChBStatus |
                     pstcBMOutputInit->u32IdleChAEnterDelay | pstcBMOutputInit->u32IdleChBEnterDelay |
                     pstcBMOutputInit->u32Follow);
        u32UnitNum = ((uint32_t)HRPWMx - CM_HRPWM1_BASE) / (CM_HRPWM2_BASE - CM_HRPWM1_BASE);

        MODIFY_REG32(CM_HRPWM_COMMON->BMCR, (HRPWM_COMMON_BMCR_BMTMR1 << u32UnitNum) | HRPWM_BM_FLAG_ALL,
                     (pstcBMOutputInit->u32UnitCountReset << u32UnitNum) | HRPWM_BM_FLAG_ALL);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_valid_period_config_t to default values
 * @param  [out] pstcValidperiodConfig  Pointer to a @ref stc_hrpwm_valid_period_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_ValidPeriodStructInit(stc_hrpwm_valid_period_config_t *pstcValidperiodConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcValidperiodConfig) {
        pstcValidperiodConfig->u32CountCond = HRPWM_VALID_PERIOD_INVD;
        pstcValidperiodConfig->u32Interval = 0x00UL;
        pstcValidperiodConfig->u32SpecialA = HRPWM_VALID_PERIOD_SPECIAL_A_OFF;
        pstcValidperiodConfig->u32SpecialB = HRPWM_VALID_PERIOD_SPECIAL_B_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM valid period function configuration for special compare function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcValidperiodConfig   Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_ValidPeriodConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_valid_period_config_t *pstcValidperiodConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcValidperiodConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_PERIOD_CNT_COND(pstcValidperiodConfig->u32CountCond));
        DDL_ASSERT(IS_HRPWM_PERIOD_INTERVAL(pstcValidperiodConfig->u32Interval));
        DDL_ASSERT(IS_HRPWM_PERIOD_SPECIAL_A_FUNC(pstcValidperiodConfig->u32SpecialA));
        DDL_ASSERT(IS_HRPWM_PERIOD_SPECIAL_B_FUNC(pstcValidperiodConfig->u32SpecialB));

        MODIFY_REG32(HRPWMx->VPERR, HRPWM_VPERR_PCNTS | HRPWM_VPERR_PCNTE | HRPWM_VPERR_SPPERIA | HRPWM_VPERR_SPPERIB,
                     pstcValidperiodConfig->u32CountCond |
                     pstcValidperiodConfig->u32Interval << HRPWM_VPERR_PCNTS_POS |
                     pstcValidperiodConfig->u32SpecialA |
                     pstcValidperiodConfig->u32SpecialB);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_evt_config_t to default values
 * @param  [out] pstcEventConfig        Pointer to a @ref stc_hrpwm_evt_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EVT_StructInit(stc_hrpwm_evt_config_t *pstcEventConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcEventConfig) {
        pstcEventConfig->u32EventSrc = HRPWM_EVT_SRC1;
        pstcEventConfig->u32ValidLevel = HRPWM_EVT_VALID_LVL_HIGH;
        pstcEventConfig->u32ValidAction = HRPWM_EVT_VALID_LVL;
        pstcEventConfig->u32FastAsyncMode = HRPWM_EVT_FAST_ASYNC_OFF;
        pstcEventConfig->u32FilterClock = HRPWM_EVT_FILTER_NONE;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  External event configuration
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] pstcEventConfig     Point External event source config pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EVT_Config(uint32_t u32EventNum, const stc_hrpwm_evt_config_t *pstcEventConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcEventConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
        DDL_ASSERT(IS_HRPWM_EVT_SRC(pstcEventConfig->u32EventSrc));
        DDL_ASSERT(IS_HRPWM_EVT_VALID_ACTION(pstcEventConfig->u32ValidAction));
        DDL_ASSERT(IS_HRPWM_EVT_VALID_LVL(pstcEventConfig->u32ValidLevel));
        if (u32EventNum <= HRPWM_EVT5) {
            DDL_ASSERT(IS_HRPWM_EVT_FAST_ASYNC_MD(pstcEventConfig->u32FastAsyncMode));
        } else {
            DDL_ASSERT(HRPWM_EVT_FAST_ASYNC_OFF == pstcEventConfig->u32FastAsyncMode);
            DDL_ASSERT(IS_HRPWM_EVT_FILTER_CLK(pstcEventConfig->u32FilterClock));
        }

        if (u32EventNum <= HRPWM_EVT5) {
            /* Event1 ~ Event5 */
            MODIFY_REG32(CM_HRPWM_COMMON->EECR1,
                         HRPWM_EXTEVT1_CONFIG_MASK << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum),
                         (pstcEventConfig->u32EventSrc | pstcEventConfig->u32ValidLevel |
                          pstcEventConfig->u32ValidAction | pstcEventConfig->u32FastAsyncMode) << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum));
        } else {
            /* Event6 ~ Event10 */
            MODIFY_REG32(CM_HRPWM_COMMON->EECR2,
                         HRPWM_EXTEVT6_CONFIG_MASK << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)),
                         (pstcEventConfig->u32EventSrc | pstcEventConfig->u32ValidLevel |
                          pstcEventConfig->u32ValidAction) << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)));
            MODIFY_REG32(CM_HRPWM_COMMON->EECR3, HRPWM_COMMON_EECR3_EE6F << (HRPWM_COMMON_EECR3_EE7F_POS * (u32EventNum - HRPWM_EVT6)),
                         pstcEventConfig->u32FilterClock << (HRPWM_COMMON_EECR3_EE7F_POS * (u32EventNum - HRPWM_EVT6)));
        }
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_evt_filter_config_t to default values
 * @param  [out] pstcFilterConfig       Pointer to a @ref stc_hrpwm_evt_filter_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EVT_FilterStructInit(stc_hrpwm_evt_filter_config_t *pstcFilterConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcFilterConfig) {
        pstcFilterConfig->u32Mode = HRPWM_EVT_FILTER_OFF;
        pstcFilterConfig->u32Latch = HRPWM_EVT_FILTER_LATCH_OFF;
        pstcFilterConfig->u32WindowTimeout = HRPWM_EVT_FILTER_TIMEOUT_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  External event filter configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] pstcFilterConfig    Point External filter source config pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EVT_FilterConfig(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, const stc_hrpwm_evt_filter_config_t *pstcFilterConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcFilterConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
        DDL_ASSERT(IS_HRPWM_EVT_FILTER_MD(pstcFilterConfig->u32Mode));
        DDL_ASSERT(IS_HRPWM_EVT_LATCH_FUNC(pstcFilterConfig->u32Latch));
        DDL_ASSERT(IS_HRPWM_EVT_TIMEOUT_FUNC(pstcFilterConfig->u32WindowTimeout));

        if (u32EventNum <= HRPWM_EVT5) {
            /* Event1 ~ Event5 */
            MODIFY_REG32(HRPWMx->EEFLTCR1,
                         HRPWM_EXTEVT1_FILTER_CONFIG_MASK << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum),
                         (pstcFilterConfig->u32Mode | pstcFilterConfig->u32Latch | pstcFilterConfig->u32WindowTimeout)
                         << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum));
        } else {
            /* Event6 ~ Event10 */
            MODIFY_REG32(HRPWMx->EEFLTCR2,
                         HRPWM_EXTEVT1_FILTER_CONFIG_MASK << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)),
                         (pstcFilterConfig->u32Mode | pstcFilterConfig->u32Latch | pstcFilterConfig->u32WindowTimeout)
                         << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)));
        }
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_evt_filter_signal_config_t to default values
 * @param  [out] pstcSignalConfig   Pointer to a @ref stc_hrpwm_evt_filter_signal_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EVT_FilterSignalStructInit(stc_hrpwm_evt_filter_signal_config_t *pstcSignalConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcSignalConfig) {
        pstcSignalConfig->u32InitPolarity = HRPWM_EVT_FILTER_INIT_POLARITY_LOW;
        pstcSignalConfig->u32Offset = 0x00UL;
        pstcSignalConfig->u32OffsetDir = HRPWM_EVT_FILTER_OFS_DIR_DOWN;
        pstcSignalConfig->u32Window = 0x00UL;
        pstcSignalConfig->u32WindowDir = HRPWM_EVT_FILTER_WIN_DIR_DOWN;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM external event filter signal configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcSignalConfig    Point External filter source config pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EVT_FilterSignalConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_evt_filter_signal_config_t *pstcSignalConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcSignalConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EVT_FILTER_INIT_POLARITY(pstcSignalConfig->u32InitPolarity));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(pstcSignalConfig->u32Offset));
        DDL_ASSERT(IS_HRPWM_EVT_FILTER_OFS_DIR(pstcSignalConfig->u32OffsetDir));
        DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(pstcSignalConfig->u32Window));
        DDL_ASSERT(IS_HRPWM_EVT_FILTER_WIN_DIR(pstcSignalConfig->u32WindowDir));

        MODIFY_REG32(HRPWMx->EEFLTCR1, HRPWM_EEFLTCR1_EEINTPOL, pstcSignalConfig->u32InitPolarity);
        WRITE_REG32(HRPWMx->EEFOFFSETAR, pstcSignalConfig->u32Offset | pstcSignalConfig->u32OffsetDir);
        WRITE_REG32(HRPWMx->EEFWINAR, pstcSignalConfig->u32Window | pstcSignalConfig->u32WindowDir);
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_sync_output_config_t to default values
 * @param  [out] pstcSyncConfig     Pointer to a @ref stc_hrpwm_sync_output_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_SYNC_StructInit(stc_hrpwm_sync_output_config_t *pstcSyncConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcSyncConfig) {
        pstcSyncConfig->u32Src = HRPWM_SYNC_SRC_U1_CNT_VALLEY;
        pstcSyncConfig->u32MatchBDir = HRPWM_SYNC_MATCH_B_DIR_DOWN;
        pstcSyncConfig->u32Pulse = HRPWM_SYNC_PULSE_OFF;
        pstcSyncConfig->u32PulseWidth = 0x01UL;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM synchronous output configure
 * @param  [in] pstcSyncConfig      Point a structure @ref stc_hrpwm_sync_output_config_t
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_SYNC_Config(stc_hrpwm_sync_output_config_t *pstcSyncConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcSyncConfig) {
        DDL_ASSERT(IS_HRPWM_SYNC_SRC(pstcSyncConfig->u32Src));
        DDL_ASSERT(IS_HRPWM_SYNC_MATCH_B_DIR(pstcSyncConfig->u32MatchBDir));
        DDL_ASSERT(IS_HRPWM_SYNC_PULSE(pstcSyncConfig->u32Pulse));
        DDL_ASSERT(IS_HRPWM_SYNC_PULSE_WIDTH(pstcSyncConfig->u32PulseWidth));
        WRITE_REG32(CM_HRPWM_COMMON->SYNOCR, pstcSyncConfig->u32Src | pstcSyncConfig->u32MatchBDir |
                    pstcSyncConfig->u32Pulse | pstcSyncConfig->u32PulseWidth << HRPWM_COMMON_SYNOCR_SYNCMP_POS);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_dac_trigger_config_t to default values
 * @param  [out] pstcDacTriggerConfig   Pointer to a @ref stc_hrpwm_dac_trigger_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_DAC_TriggerStructInit(stc_hrpwm_dac_trigger_config_t *pstcDacTriggerConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    /* Check structure pointer */
    if (NULL != pstcDacTriggerConfig) {
        pstcDacTriggerConfig->u32Src = HRPWM_DAC_TRIG_SRC_CNT_VALLEY;
        pstcDacTriggerConfig->u32DacCh1Dest = HRPWM_DAC_CH1_TRIG_DEST_NONE;
        pstcDacTriggerConfig->u32DacCh2Dest = HRPWM_DAC_CH2_TRIG_DEST_NONE;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM DAC synchronous trigger configure
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcDacTriggerConfig    Point a structure @ref stc_hrpwm_dac_trigger_config_t
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_DAC_TriggerConfig(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_dac_trigger_config_t *pstcDacTriggerConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcDacTriggerConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_DAC_TRIG_SRC(pstcDacTriggerConfig->u32Src));
        DDL_ASSERT(IS_HRPWM_DAC_CH1_TRIG_DEST(pstcDacTriggerConfig->u32DacCh1Dest));
        DDL_ASSERT(IS_HRPWM_DAC_CH2_TRIG_DEST(pstcDacTriggerConfig->u32DacCh2Dest));

        MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DACSRC | HRPWM_CR_DACSYNC1 | HRPWM_CR_DACSYNC2,
                     pstcDacTriggerConfig->u32Src |
                     pstcDacTriggerConfig->u32DacCh1Dest |
                     pstcDacTriggerConfig->u32DacCh2Dest);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_ph_config_t to default values
 * @param  [out] pstcPHConfig       Pointer to a @ref stc_hrpwm_ph_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PH_StructInit(stc_hrpwm_ph_config_t *pstcPHConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcPHConfig) {
        pstcPHConfig->u32PhaseIndex = HRPWM_PH_MATCH_IDX1;
        pstcPHConfig->u32ForceChA = HRPWM_PH_MATCH_FORCE_CHA_OFF;
        pstcPHConfig->u32ForceChB = HRPWM_PH_MATCH_FORCE_CHB_OFF;
        pstcPHConfig->u32PeriodLink = HRPWM_PH_PERIOD_LINK_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM Phase configuration for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] pstcPHConfig     Point a structure @ref stc_hrpwm_ph_config_t
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PH_Config(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_ph_config_t *pstcPHConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcPHConfig) {
        DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(pstcPHConfig->u32PhaseIndex));
        DDL_ASSERT(IS_HRPWM_PH_MATCH_FORCE_CHA_FUNC(pstcPHConfig->u32ForceChA));
        DDL_ASSERT(IS_HRPWM_PH_MATCH_FORCE_CHB_FUNC(pstcPHConfig->u32ForceChB));
        DDL_ASSERT(IS_HRPWM_PH_PERIOD_LINK(pstcPHConfig->u32PeriodLink));

        WRITE_REG32(HRPWMx->PHSCTL,
                    (pstcPHConfig->u32PhaseIndex << HRPWM_PHSCTL_PHCMPSEL_POS) |
                    pstcPHConfig->u32ForceChA | pstcPHConfig->u32ForceChB);
        MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_PRDLK, pstcPHConfig->u32PeriodLink);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM general compare register A and E buffer function configuration
 * @param  [in] pstcBufConfig       Pointer of configuration structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_PH_BufConfig(const stc_hrpwm_buf_config_t *pstcBufConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcBufConfig) {
        /* Check parameters */
        DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(pstcBufConfig->u32BufTransCond));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcBufConfig->enGlobalBufTrans));

        /* Config normal buffer transfer condition */
        MODIFY_REG32(CM_HRPWM1->PHSCTL, HRPWM_PHSCTL_BTRDPHS | HRPWM_PHSCTL_BTRUPHS,
                     pstcBufConfig->u32BufTransCond << HRPWM_PHSCTL_BTRUPHS_POS);
        /* Config global buffer transfer function */
        MODIFY_REG32(CM_HRPWM1->GBCFG, HRPWM_GBCFG_PHSCMPAR,
                     (uint32_t)pstcBufConfig->enGlobalBufTrans << HRPWM_GBCFG_PHSCMPAR_POS);

        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_emb_config_t to default values
 * @param  [out] pstcEmbConfig      Pointer to a @ref stc_hrpwm_emb_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_StructInit(stc_hrpwm_emb_config_t *pstcEmbConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcEmbConfig) {
        pstcEmbConfig->u32ValidCh = HRPWM_EMB_EVT_CH0;
        pstcEmbConfig->u32ReleaseMode = HRPWM_EMB_RELEASE_IMMED;
        pstcEmbConfig->u32PinStatus = HRPWM_EMB_PIN_NORMAL;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM channel A EMB function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcEmbConfig       Point EMB function Config Pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_ChAConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcEmbConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EMB_CH(pstcEmbConfig->u32ValidCh));
        DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(pstcEmbConfig->u32ReleaseMode));
        DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(pstcEmbConfig->u32PinStatus));

        MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_EMBSA | HRPWM_PCNAR1_EMBRA | HRPWM_PCNAR1_EMBCA,
                     pstcEmbConfig->u32ValidCh | pstcEmbConfig->u32ReleaseMode | pstcEmbConfig->u32PinStatus);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM channel A EMB function configuration (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcEmbConfig       Point EMB function Config Pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_ChAConfig_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcEmbConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EMB_CH(pstcEmbConfig->u32ValidCh));
        DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(pstcEmbConfig->u32ReleaseMode));
        DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(pstcEmbConfig->u32PinStatus));

        MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_EMBSA | HRPWM_PCNAR1_EMBRA | HRPWM_PCNAR1_EMBCA,
                     pstcEmbConfig->u32ValidCh | pstcEmbConfig->u32ReleaseMode | pstcEmbConfig->u32PinStatus);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM channel B EMB function configuration
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcEmbConfig       Point EMB function Config Pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_ChBConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcEmbConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EMB_CH(pstcEmbConfig->u32ValidCh));
        DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(pstcEmbConfig->u32ReleaseMode));
        DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(pstcEmbConfig->u32PinStatus));

        MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_EMBSB | HRPWM_PCNBR1_EMBRB | HRPWM_PCNBR1_EMBCB,
                     pstcEmbConfig->u32ValidCh | pstcEmbConfig->u32ReleaseMode | pstcEmbConfig->u32PinStatus);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM channel B EMB function configuration (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] pstcEmbConfig       Point EMB function Config Pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_ChBConfig_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    if (NULL != pstcEmbConfig) {
        DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
        DDL_ASSERT(IS_HRPWM_EMB_CH(pstcEmbConfig->u32ValidCh));
        DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(pstcEmbConfig->u32ReleaseMode));
        DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(pstcEmbConfig->u32PinStatus));

        MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_EMBSB | HRPWM_PCNBR1_EMBRB | HRPWM_PCNBR1_EMBCB,
                     pstcEmbConfig->u32ValidCh | pstcEmbConfig->u32ReleaseMode | pstcEmbConfig->u32PinStatus);
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM channel B EMB function configuration (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM8
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @param  [in] enNewState          New state of the update registers
 *  @arg ENABLE
 *  @arg DISABLE
 * @retval None
 */
void HRPWM_DMA_UpdateRegCmd_U2_8(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64UpdateReg, en_functional_state_t enNewState)
{
    uint32_t u32UnitNum;

    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    u32UnitNum = ((uint32_t)HRPWMx - CM_HRPWM2_BASE) / (CM_HRPWM3_BASE - CM_HRPWM2_BASE);

    if (ENABLE == enNewState) {
        SET_REG32_BIT(HRPWM_DMA_UNIT_REG1(CM_HRPWM_COMMON->BDUPR21, u32UnitNum), (uint32_t)u64UpdateReg);
        SET_REG32_BIT(HRPWM_DMA_UNIT_REG2(CM_HRPWM_COMMON->BDUPR22, u32UnitNum), (uint32_t)(u64UpdateReg << 32U));

    } else {
        CLR_REG32_BIT(HRPWM_DMA_UNIT_REG1(CM_HRPWM_COMMON->BDUPR21, u32UnitNum), (uint32_t)u64UpdateReg);
        CLR_REG32_BIT(HRPWM_DMA_UNIT_REG2(CM_HRPWM_COMMON->BDUPR22, u32UnitNum), (uint32_t)(u64UpdateReg << 32U));
    }
}

/**
 * @brief  Clear HRPWM EMB flag status.
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Flag                 HRPWM EMB flag  @ref HRPWM_EMB_Flag_Clear
 * @retval None
 */
void HRPWM_EMB_ClearStatus(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Flag)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_CLR_FLAG(u32Flag));

    WRITE_REG32(HRPWM_EMBx->STATCLR, u32Flag);
}

/**
 * @brief  Get HRPWM EMB flag status.
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Flag                 HRPWM EMB flag  @ref HRPWM_EMB_Flag_Clear
 * @retval None
 */
en_flag_status_t HRPWM_EMB_GetStatus(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Flag)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_FLAG_STATE(u32Flag));

    return (READ_REG32_BIT(HRPWM_EMBx->STAT, u32Flag) == 0UL) ? RESET : SET;
}

/**
 * @brief  Enable HRPWM EMB event.
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Event                HRPWM EMB event  @ref HRPWM_EMB_Event
 * @retval None
 */
void HRPWM_EMB_EventEnable(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_EVT(u32Event));

    WRITE_REG32(HRPWM_EMBx->CTL1, u32Event);
}

/**
 * @brief  Enable or disable HRPWM EMB event interrupt.
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Event                HRPWM EMB event  @ref HRPWM_EMB_INT
 * @param  [in] enNewState              New state of the specified HRPWM EMB event interrupt @ref en_functional_state_t
 * @retval None
 */
void HRPWM_EMB_IntCmd(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event, en_functional_state_t enNewState)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_INT(u32Event));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(HRPWM_EMBx->INTEN, u32Event);
    } else {
        CLR_REG32_BIT(HRPWM_EMBx->INTEN, u32Event);
    }
}

/**
 * @brief  Set EMB accumulator threshold
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Threshold            Specifies the EMB accumulator threshold value, range from 0 to 15
 * @retval None
 */
void HRPWM_EMB_SetAccumThreshold(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Threshold)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_ACCUM_THRESHOLD(u32Threshold));

    MODIFY_REG32(HRPWM_EMBx->CTL5, HRPWM_EMB_CTL5_EMBACNT, u32Threshold);
}

/**
 * @brief  Set EMB accumulator threshold
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Event                Specifies the EMB accumulator event @ref HRPWM_EMB_Accumulator_Event_Selection
 * @retval None
 */
void HRPWM_EMB_SetAccumEvent(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_ACCUM_EVT_SEL(u32Event));

    MODIFY_REG32(HRPWM_EMBx->CTL5, HRPWM_EMB_CTL5_EMBASEL, u32Event);
}

/**
 * @brief  Set EMB accumulator reset mode
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32Mode                Specifies the EMB accumulator reset mode @ref HRPWM_EMB_Accumulator_Reset_Mode
 * @retval None
 */
void HRPWM_EMB_SetAccumResetMode(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Mode)
{
    /* Check parameters */
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_ACCUM_RST_MD(u32Mode));

    MODIFY_REG32(HRPWM_EMBx->CTL5, HRPWM_EMB_CTL5_EMBARSTM, u32Mode);
}

/**
 * @brief  Set the fields of structure pstcLevelFilterConfig to default values
 * @param  [out] pstcLevelFilterConfig       Pointer to a @ref stc_hrpwm_emb_level_filter_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_LevelAndFilterStructInit(stc_hrpwm_emb_level_filter_t *pstcLevelFilterConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcLevelFilterConfig) {
        pstcLevelFilterConfig->CTL2_f.u32Pwm1Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm2Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm3Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm4Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm5Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm6Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm7Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32Pwm8Level   = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn1Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn2Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn3Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn4Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn5Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn6Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn7Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32EmbIn8Level = HRPWM_EMB_LVL_HIGH;
        pstcLevelFilterConfig->CTL2_f.u32FilterMode  = HRPWM_EMB_NF_CNT_3;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM EMB noise filter configuration
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPW_EMBx
 * @param  [in] pstcLevelFilterConfig   Point EMB level & filter mode Config Pointer
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EMB_LevelAndFilterConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_level_filter_t *pstcLevelFilterConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcLevelFilterConfig) {
        DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm1Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm2Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm3Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm4Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm5Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm6Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm7Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32Pwm8Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn1Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn2Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn3Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn4Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn5Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn6Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn7Level));
        DDL_ASSERT(IS_HRPWM_EMB_LVL(pstcLevelFilterConfig->CTL2_f.u32EmbIn8Level));
        DDL_ASSERT(IS_HRPWM_EMB_NF_MD(pstcLevelFilterConfig->CTL2_f.u32FilterMode));

        WRITE_REG32(HRPWM_EMBx->CTL2, pstcLevelFilterConfig->CTL2);

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_emb_nf_config_t to default values
 * @param  [out] pstcNfConfig       Pointer to a @ref stc_hrpwm_emb_nf_config_t structure
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_NoiseFilterStructInit(stc_hrpwm_emb_nf_config_t *pstcNfConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcNfConfig) {
        pstcNfConfig->CTL3_f.u32EmbIn1FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn1FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn2FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn2FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn3FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn3FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn4FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn4FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn5FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn5FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn6FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn6FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn7FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn7FilterState = HRPWM_EMB_NF_OFF;
        pstcNfConfig->CTL3_f.u32EmbIn8FilterClock = HRPWM_EMB_NF_CLK_DIV1;
        pstcNfConfig->CTL3_f.u32EmbIn8FilterState = HRPWM_EMB_NF_OFF;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM EMB noise filter configuration
 * @param  [in] HRPWM_EMBx          HRPWM EMB unit
 *  @arg CM_HRPW_EMBx
 * @param  [in] pstcNfConfig        Point EMB noise filter Config Pointer
 * @retval int32_t:
 *         - LL_OK:                 Successfully done
 *         - LL_ERR_INVD_PARAM:     Parameter error
 */
int32_t HRPWM_EMB_NoiseFilterConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_nf_config_t *pstcNfConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcNfConfig) {
        DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn1FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn1FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn2FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn2FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn3FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn3FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn4FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn4FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn5FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn5FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn6FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn6FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn7FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn7FilterState));
        DDL_ASSERT(IS_HRPWM_EMB_NF_CLK_DIV(pstcNfConfig->CTL3_f.u32EmbIn8FilterClock));
        DDL_ASSERT(IS_HRPWM_EMB_NF_STATE(pstcNfConfig->CTL3_f.u32EmbIn8FilterState));

        WRITE_REG32(HRPWM_EMBx->CTL3, pstcNfConfig->CTL3);

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_emb_blank_Accum_t to default values
 * @param  [out] pstcBlankAccumConfig   Pointer to a @ref stc_hrpwm_emb_blank_Accum_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EMB_BlankAndAccumStructInit(stc_hrpwm_emb_blank_Accum_t *pstcBlankAccumConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBlankAccumConfig) {
        pstcBlankAccumConfig->CTL4_f.u32EmbBlankState = (uint32_t)DISABLE;
        pstcBlankAccumConfig->CTL4_f.u32EmbBlankTime  = HRPWM_EMB_BLANK_TIME_REF;
        pstcBlankAccumConfig->CTL4_f.u32EmbAccumEdge  = HRPWM_EMB_ACCUM_EDGE_FALLING;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM EMB blank and accumulate configuration
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPW_EMBx
 * @param  [in] pstcBlankAccumConfig    Point EMB blank and accumulate Config Pointer
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EMB_BlankAndAccumConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_blank_Accum_t *pstcBlankAccumConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcBlankAccumConfig) {
        DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
        DDL_ASSERT(IS_FUNCTIONAL_STATE((en_functional_state_t)pstcBlankAccumConfig->CTL4_f.u32EmbBlankState));
        DDL_ASSERT(IS_HRPWM_EMB_BLANK_TIME(pstcBlankAccumConfig->CTL4_f.u32EmbBlankTime));
        DDL_ASSERT(IS_HRPWM_EMB_ACCUM_EDGE(pstcBlankAccumConfig->CTL4_f.u32EmbAccumEdge));

        WRITE_REG32(HRPWM_EMBx->CTL4, pstcBlankAccumConfig->CTL4);

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_hrpwm_emb_Accum_config_t to default values
 * @param  [out] pstcAccumConfig        Pointer to a @ref stc_hrpwm_emb_Accum_config_t structure
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EMB_AccumStructInit(stc_hrpwm_emb_Accum_config_t *pstcAccumConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcAccumConfig) {
        pstcAccumConfig->u32AccumEvent     = HRPWM_EMB_ACCUM_EVT_SEL_NONE;
        pstcAccumConfig->u32AccumResetMode = HRPWM_EMB_ACCUM_RST_PERIOD_POINT;
        pstcAccumConfig->u32AccumThreshold = 0UL;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  HRPWM EMB accumulator configuration
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPW_EMBx
 * @param  [in] pstcAccumConfig         Point EMB accumulator Config Pointer
 * @retval int32_t:
 *         - LL_OK:                     Successfully done
 *         - LL_ERR_INVD_PARAM:         Parameter error
 */
int32_t HRPWM_EMB_AccumConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_Accum_config_t *pstcAccumConfig)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    /* Check structure pointer */
    if (NULL != pstcAccumConfig) {
        DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
        DDL_ASSERT(IS_HRPWM_EMB_ACCUM_EVT_SEL(pstcAccumConfig->u32AccumEvent));
        DDL_ASSERT(IS_HRPWM_EMB_ACCUM_RST_MD(pstcAccumConfig->u32AccumResetMode));
        DDL_ASSERT(IS_HRPWM_EMB_ACCUM_THRESHOLD(pstcAccumConfig->u32AccumThreshold));

        WRITE_REG32(HRPWM_EMBx->CTL5, pstcAccumConfig->u32AccumEvent | pstcAccumConfig->u32AccumResetMode | \
                    pstcAccumConfig->u32AccumThreshold);

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @}
 */

#endif /* LL_HRPWM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
