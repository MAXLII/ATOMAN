/**
 *******************************************************************************
 * @file  hc32_ll_dsogi_pll.c
 * @brief This file provides firmware functions to manage the DSOGI_PLL.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2026-04-16       CDT             First version
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

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_dsogi_pll.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_DSOGI_PLL DSOGI_PLL
 * @brief DSOGI_PLL Driver Library
 * @{
 */

#if (LL_DSOGI_PLL_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup DSOGI_PLL_Local_Macros DSOGI_PLL Local Macros
 * @{
 */

#define DSOGI_PLL_FLOAT_CONV(x)  (*(__IO float32_t *)((uint32_t)&x))

#define FLOAT_WRITE_REG32(reg, value)   WRITE_REG32((reg), (*(__IO uint32_t *)((uint32_t)&(value))));

/**
 * @defgroup DSOGI_PLL_Check_Parameters_Validity DSOGI_PLL Check Parameters Validity
 * @{
 */
#define IS_DSOGI_PLL_PERIPH(x)                   ((x) == CM_DSOGI_PLL)

#define IS_DSOGI_PLL_FUNC(x)                                                    \
(   ((x) == DSOGI_PLL_THREE_PHASE_DSOGI)                                     || \
    ((x) == DSOGI_PLL_THREE_PHASE_SYNC)                                      || \
    ((x) == DSOGI_PLL_SINGLE_PHASE)                                          || \
    ((x) == DSOGI_PLL_CLARK)                                                 || \
    ((x) == DSOGI_PLL_SOGI)                                                  || \
    ((x) == DSOGI_PLL_PARK)                                                  || \
    ((x) == DSOGI_PLL_PLL))

#define IS_DSOGI_PLL_COMB_FUNC(x)                                              \
(   ((x) == DSOGI_PLL_COMB_THREE_PHASE_DSOGI)                               || \
    ((x) == DSOGI_PLL_COMB_THREE_PHASE_SYNC)                                || \
    ((x) == DSOGI_PLL_COMB_SINGLE_PHASE))

#define IS_DSOGI_PLL_CLARK_SRC(x)                                               \
(   ((x) == DSOGI_PLL_CLARK_ABC)                                             || \
    ((x) == DSOGI_PLL_CLARK_ADC1_234)                                        || \
    ((x) == DSOGI_PLL_CLARK_ADC123_2))

#define IS_DSOGI_PLL_CLARK_VOLT(x)                                              \
(   ((x) == DSOGI_PLL_CLARK_PHASE_VOLT_AB)                                   || \
    ((x) == DSOGI_PLL_CLARK_PHASE_VOLT_BC)                                   || \
    ((x) == DSOGI_PLL_CLARK_PHASE_VOLT_AC)                                   || \
    ((x) == DSOGI_PLL_CLARK_PHASE_VOLT_ABC)                                  || \
    ((x) == DSOGI_PLL_CLARK_LINE_VOLT_AC_BC)                                 || \
    ((x) == DSOGI_PLL_CLARK_LINE_VOLT_AB_AC)                                 || \
    ((x) == DSOGI_PLL_CLARK_LINE_VOLT_AB_BC))

#define IS_DSOGI_PLL_CLARK_ADC_GAIN(x)                                          \
(   ((x) == DSOGI_PLL_ADC_DIV1)                                              || \
    ((x) == DSOGI_PLL_ADC_DIV256)                                            || \
    ((x) == DSOGI_PLL_ADC_DIV1024)                                           || \
    ((x) == DSOGI_PLL_ADC_DIV2048))

#define IS_DSOGI_PLL_SOGI_CALC(x)                                               \
(   ((x) == DSOGI_PLL_SOGI_CAUL_1)                                           || \
    ((x) == DSOGI_PLL_SOGI_CAUL_2))

#define IS_DSOGI_PLL_SOGI_SIN_COS(x)                                            \
(   ((x) == DSOGI_PLL_SOGI_SIN_COS_DISABLE)                                  || \
    ((x) == DSOGI_PLL_SOGI_SIN_COS_ENABLE))

#define IS_DSOGI_PLL_FLL_STATE(x)                                          \
(   ((x) == DSOGI_PLL_FLL_DISABLE)                                      || \
    ((x) == DSOGI_PLL_FLL_ENABLE))

#define IS_DSOGI_PLL_PARK_THETA(x)                                              \
(   ((x) == DSOGI_PLL_PARK_THETA_Q)                                          || \
    ((x) == DSOGI_PLL_PARK_THETA_D))

#define IS_DSOGI_PLL_PLL_PI_STATE(x)                                            \
(   ((x) == DSOGI_PLL_PI_DISABLE)                                        || \
    ((x) == DSOGI_PLL_PI_ENABLE))

#define IS_DSOGI_PLL_PLL_STATE(x)                                               \
(   ((x) == DSOGI_PLL_PLL_DISABLE)                                           || \
    ((x) == DSOGI_PLL_PLL_ENABLE))

#define IS_DSOGI_PLL_VOLT_JUDGE_STATE(x)                                        \
(   ((x) == DSOGI_PLL_JUDGE_DISABLE)                                         || \
    ((x) == DSOGI_PLL_JUDGE_ENABLE))

#define IS_DSOGI_PLL_DELAYTIME(x)                ((x) <= DSOGI_PLL_DELAYTIME_MAX)

#define IS_DSOGI_PLL_INT(x)                                                     \
(   ((x) != 0UL)                                                            && \
    ((x) | DSOGI_PLL_INT_MASK) == DSOGI_PLL_INT_MASK)

#define IS_DSOGI_PLL_FLAG(x)                                                    \
(   ((x) != 0UL)                                                            && \
    ((x) | DSOGI_PLL_FLAG_MASK) == DSOGI_PLL_FLAG_MASK)

#define IS_DSOGI_PLL_CLR_FLAG(x)                                                \
(   ((x) != 0UL)                                                            && \
    ((x) | DSOGI_PLL_CLR_FLAG_MASK) == DSOGI_PLL_CLR_FLAG_MASK)

#define IS_DSOGI_PLL_PWC_UNLOCKED()              ((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1)

/* DSOGI_PLL reset timeout */
#define DSOGI_PLL_RMU_TIMEOUT                    (100UL)

#define DSOGI_PLL_SAMPLE_TIME_DEFAULT            (1.0f / 10000.0f)

#define DSOGI_PLL_BASE_FREQUENCY_DEFAULT         (50.00f)

#define DSOGI_PLL_PI                             (3.14159265359f)

#define DSOGI_PLL_CENTER_ANGLE_FREQ              (2.0f * DSOGI_PLL_PI * DSOGI_PLL_BASE_FREQUENCY_DEFAULT)
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
 * @defgroup DSOGI_PLL_Global_Functions DSOGI_PLL Global Functions
 * @{
 */

/**
 * @brief  De-Initialize DSOGI_PLL unit base function.
 * @param  [in] DSOGI_PLLx               Pointer to DSOGI_PLL unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_TIMEOUT:          Works timeout.
 */
int32_t DSOGI_PLL_DeInit(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;

    /* Check parameters */
    DDL_ASSERT(IS_DSOGI_PLL_PWC_UNLOCKED());
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    CLR_REG32_BIT(CM_RMU->FRST0, RMU_FRST0_DSOGI_PLL);
    /* Ensure reset procedure is completed */
    while (RMU_FRST0_DSOGI_PLL != READ_REG32_BIT(CM_RMU->FRST0, RMU_FRST0_DSOGI_PLL)) {
        u32TimeOut++;
        if (u32TimeOut > DSOGI_PLL_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  SOGI unit software reset.
 * @param  [in] DSOGI_PLLx               Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              The new state of SOGI software reset.
 *         This parameter can be: ENABLE or DISABLE.
 * @retval None
 */
void DSOGI_PLL_SWReset(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_SRST);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_SRST);
    }
}

/**
 * @brief  Set the fields of structure stc_sogi_pll_init_t to default values..
 * @param  [in] pstcBaseInit            Pointer to a @ref stc_base_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcBaseInit is NULL
 */
int32_t DSOGI_PLL_Base_StructInit(stc_base_init_t *pstcBaseInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcBaseInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcBaseInit->f32Ts          = DSOGI_PLL_SAMPLE_TIME_DEFAULT;
        pstcBaseInit->f32BaseFreq    = DSOGI_PLL_BASE_FREQUENCY_DEFAULT;
    }

    return i32Ret;
}

/**
 * @brief  Initialize SOGI_PLL system calculation parameters.
 * @param  [in] DSOGI_PLLx              Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcBaseInit            Pointer to a @ref stc_base_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcBaseInit is NULL
 */
int32_t DSOGI_PLL_Base_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_base_init_t *pstcBaseInit)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcBaseInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->TS, pstcBaseInit->f32Ts);
        FLOAT_WRITE_REG32(DSOGI_PLLx->FREBASE, pstcBaseInit->f32BaseFreq);
    }

    return i32Ret;
}

/**
 * @brief  Initialize SOGI PLL Phase-Locked base function.
 * @param  [in] DSOGI_PLLx              Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcSogiPllInit         Pointer to a @ref stc_sogi_pll_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcSogiPllInit is NULL
 */
int32_t DSOGI_PLL_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_pll_init_t *pstcSogiPllInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32CR;

    if (NULL == pstcSogiPllInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
        DDL_ASSERT(IS_DSOGI_PLL_COMB_FUNC(pstcSogiPllInit->u32Func));
        DDL_ASSERT(IS_DSOGI_PLL_CLARK_SRC(pstcSogiPllInit->stcClarkInit.u32SrcSelect));
        DDL_ASSERT(IS_DSOGI_PLL_CLARK_ADC_GAIN(pstcSogiPllInit->stcClarkInit.u32AdcGain));
        DDL_ASSERT(IS_DSOGI_PLL_PARK_THETA(pstcSogiPllInit->stcParkInit.u32ThetaSelect));
        DDL_ASSERT(IS_DSOGI_PLL_PLL_PI_STATE(pstcSogiPllInit->stcPllInit.u32PIState));
        DDL_ASSERT(IS_DSOGI_PLL_PLL_STATE(pstcSogiPllInit->stcPllInit.u32PLLState));
        DDL_ASSERT(IS_DSOGI_PLL_VOLT_JUDGE_STATE(pstcSogiPllInit->stcPllInit.u32JudgeState));
        DDL_ASSERT(IS_DSOGI_PLL_DELAYTIME(pstcSogiPllInit->stcPllInit.u32DelayTime));

        u32CR = (pstcSogiPllInit->stcClarkInit.u32SrcSelect | pstcSogiPllInit->stcParkInit.u32ThetaSelect | pstcSogiPllInit->stcPllInit.u32PIState | \
                 pstcSogiPllInit->stcPllInit.u32PLLState | pstcSogiPllInit->stcPllInit.u32JudgeState | pstcSogiPllInit->stcClarkInit.u32AdcGain | \
                 pstcSogiPllInit->u32Func | (pstcSogiPllInit->stcPllInit.u32DelayTime << DSOGI_PLL_CR_DELAY_TIME_POS));

        if ((DSOGI_PLL_COMB_THREE_PHASE_DSOGI == pstcSogiPllInit->u32Func) || (DSOGI_PLL_COMB_SINGLE_PHASE == pstcSogiPllInit->u32Func)) {
            DDL_ASSERT(IS_DSOGI_PLL_SOGI_SIN_COS(pstcSogiPllInit->stcSogiInit.u32SogiSCSelect));
            DDL_ASSERT(IS_DSOGI_PLL_FLL_STATE(pstcSogiPllInit->stcSogiInit.u32FLLState));

            u32CR |= (pstcSogiPllInit->stcSogiInit.u32SogiSCSelect | pstcSogiPllInit->stcSogiInit.u32FLLState);
        }

        if ((DSOGI_PLL_COMB_THREE_PHASE_DSOGI == pstcSogiPllInit->u32Func) || (DSOGI_PLL_COMB_THREE_PHASE_SYNC == pstcSogiPllInit->u32Func)) {
            DDL_ASSERT(IS_DSOGI_PLL_CLARK_VOLT(pstcSogiPllInit->stcClarkInit.u32VoltageSelect));

            u32CR |= pstcSogiPllInit->stcClarkInit.u32VoltageSelect;
        }

        WRITE_REG32(DSOGI_PLLx->CR, u32CR);
        FLOAT_WRITE_REG32(DSOGI_PLLx->TS, pstcSogiPllInit->stcBaseInit.f32Ts);
        FLOAT_WRITE_REG32(DSOGI_PLLx->FREBASE, pstcSogiPllInit->stcBaseInit.f32BaseFreq);
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_sogi_pll_init_t to default values.
 * @param  [out] pstcSogiPllInit            Pointer to a @ref stc_sogi_pll_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcSogiPllInit is NULL
 */
int32_t DSOGI_PLL_StructInit(stc_sogi_pll_init_t *pstcSogiPllInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcSogiPllInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcSogiPllInit->u32Func                        = DSOGI_PLL_COMB_THREE_PHASE_DSOGI;
        pstcSogiPllInit->stcClarkInit.u32SrcSelect      = DSOGI_PLL_CLARK_ABC;
        pstcSogiPllInit->stcClarkInit.u32AdcGain        = DSOGI_PLL_ADC_DIV1;
        pstcSogiPllInit->stcClarkInit.u32VoltageSelect  = DSOGI_PLL_CLARK_PHASE_VOLT_ABC;
        pstcSogiPllInit->stcSogiInit.u32SogiSCSelect    = DSOGI_PLL_SOGI_SIN_COS_ENABLE;
        pstcSogiPllInit->stcSogiInit.u32FLLState        = DSOGI_PLL_FLL_ENABLE;
        pstcSogiPllInit->stcParkInit.u32ThetaSelect     = DSOGI_PLL_PARK_THETA_D;
        pstcSogiPllInit->stcPllInit.u32PIState          = DSOGI_PLL_PI_ENABLE;
        pstcSogiPllInit->stcPllInit.u32PLLState         = DSOGI_PLL_PLL_ENABLE;
        pstcSogiPllInit->stcPllInit.u32JudgeState       = DSOGI_PLL_JUDGE_DISABLE;
        pstcSogiPllInit->stcPllInit.u32DelayTime        = 1UL;
        pstcSogiPllInit->stcBaseInit.f32Ts              = DSOGI_PLL_SAMPLE_TIME_DEFAULT;
        pstcSogiPllInit->stcBaseInit.f32BaseFreq        = DSOGI_PLL_BASE_FREQUENCY_DEFAULT;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_clark_init_t to default values.
 * @param  [out] pstcClarkInit          Pointer to a @ref stc_clark_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcClarkInit is NULL
 */
int32_t DSOGI_PLL_Clark_StructInit(stc_clark_init_t *pstcClarkInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcClarkInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcClarkInit->u32SrcSelect = DSOGI_PLL_CLARK_ABC;
        pstcClarkInit->u32VoltageSelect  = DSOGI_PLL_CLARK_PHASE_VOLT_ABC;
        pstcClarkInit->u32AdcGain = DSOGI_PLL_ADC_DIV1;
    }

    return i32Ret;
}

/**
 * @brief  Initialize Clark base function.
 * @param  [in] DSOGI_PLLx               Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcClarkInit       Pointer to a @ref stc_clark_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcClarkInit is NULL
 */
int32_t DSOGI_PLL_Clark_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_clark_init_t *pstcClarkInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcClarkInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        MODIFY_REG32(DSOGI_PLLx->CR, (DSOGI_PLL_CR_DIN_SEL | DSOGI_PLL_CR_ADC_SEL | DSOGI_PLL_CR_LVOL_EN | DSOGI_PLL_CR_VOL_SEL | DSOGI_PLL_CR_ADC_GN),
                     pstcClarkInit->u32SrcSelect | pstcClarkInit->u32VoltageSelect | pstcClarkInit->u32AdcGain);
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_clark_config_t to default values.
 * @param  [out] pstcClarkConfig        Pointer to a @ref stc_clark_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcClarkConfig is NULL
 */
int32_t DSOGI_PLL_Clark_ConfigInit(stc_clark_config_t *pstcClarkConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcClarkConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcClarkConfig->clark_k = 1.0f;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_sogi_init_t to default values.
 * @param  [out] pstcSogiInit           Pointer to a @ref stc_sogi_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcSogiInit is NULL
 */
int32_t DSOGI_PLL_Sogi_StructInit(stc_sogi_init_t *pstcSogiInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcSogiInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcSogiInit->u32SogiSCSelect = DSOGI_PLL_SOGI_CAUL_1;
        pstcSogiInit->u32FLLState     = DSOGI_PLL_FLL_ENABLE;
    }

    return i32Ret;
}

/**
 * @brief  Initialize SOGI base function.
 * @param  [in] DSOGI_PLLx               Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcSogiInit        Pointer to a @ref stc_sogi_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcSogiInit is NULL
 */
int32_t DSOGI_PLL_Sogi_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_init_t *pstcSogiInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcSogiInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
        DDL_ASSERT(IS_DSOGI_PLL_SOGI_CALC(pstcSogiInit->u32SogiSCSelect));
        DDL_ASSERT(IS_DSOGI_PLL_FLL_STATE(pstcSogiInit->u32FLLState));

        MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_FLL_EN | DSOGI_PLL_CR_SOGI_SC, \
                     pstcSogiInit->u32SogiSCSelect | pstcSogiInit->u32FLLState);
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_sogi_config_t to default values.
 * @param  [out] pstcSogiConfig         Pointer to a @ref stc_sogi_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcSogiConfig is NULL
 */
int32_t DSOGI_PLL_Sogi_ConfigInit(stc_sogi_config_t *pstcSogiConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcSogiConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcSogiConfig->sogi_k      = 1.0f;
        pstcSogiConfig->sogi_d_k    = 1.0f;
        pstcSogiConfig->sogi_q_k    = 1.0f;
        pstcSogiConfig->sogi_fll_k  = -0.001274f;
        pstcSogiConfig->sogi_max    = 15.0f;
        pstcSogiConfig->sogi_min    = -15.0f;
        pstcSogiConfig->sogi_w      = DSOGI_PLL_CENTER_ANGLE_FREQ;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_park_init_t to default values.
 * @param  [out] pstcParkInit           Pointer to a @ref stc_park_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcParkInit is NULL
 * @note   The function is invalid when the Park independently work.
 */
int32_t DSOGI_PLL_Park_StructInit(stc_park_init_t *pstcParkInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcParkInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcParkInit->u32ThetaSelect = DSOGI_PLL_PARK_THETA_Q;
    }

    return i32Ret;
}

/**
 * @brief  Initialize Park base function.
 * @param  [in] DSOGI_PLLx               Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcParkInit        Pointer to a @ref stc_park_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcParkInit is NULL
 * @note   The function is invalid when the Park independently work.
 */
int32_t DSOGI_PLL_Park_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_park_init_t *pstcParkInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcParkInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
        DDL_ASSERT(IS_DSOGI_PLL_PARK_THETA(pstcParkInit->u32ThetaSelect));

        MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_THETA_SEL, pstcParkInit->u32ThetaSelect);
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_park_config_t to default values.
 * @param  [out] pstcParkConfig         Pointer to a @ref stc_park_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcParkConfig is NULL
 */
int32_t DSOGI_PLL_Park_ConfigInit(stc_park_config_t *pstcParkConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcParkConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcParkConfig->park_theta = 0.0f;
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_pll_init_t to default values.
 * @param  [out] pstcPllInit            Pointer to a @ref stc_pll_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcPllInit is NULL
 * @note   The parameters u32PLLState and u32JudgeState are invalid when the PLL independently work.
 */
int32_t DSOGI_PLL_Pll_StructInit(stc_pll_init_t *pstcPllInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcPllInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcPllInit->u32PIState = DSOGI_PLL_PI_ENABLE;
        pstcPllInit->u32PLLState = DSOGI_PLL_PLL_DISABLE;
        pstcPllInit->u32DelayTime = 1UL;
        pstcPllInit->u32JudgeState = DSOGI_PLL_JUDGE_DISABLE;
    }

    return i32Ret;
}

/**
 * @brief  Initialize PLL base function.
 * @param  [in] DSOGI_PLLx               Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcPllInit         Pointer to a @ref stc_pll_init_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcPllInit is NULL
 * @note   The parameters u32PLLState and u32JudgeState are invalid when the PLL independently work.
 */
int32_t DSOGI_PLL_Pll_Init(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_pll_init_t *pstcPllInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcPllInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
        DDL_ASSERT(IS_DSOGI_PLL_PLL_PI_STATE(pstcPllInit->u32PIState));
        DDL_ASSERT(IS_DSOGI_PLL_PLL_STATE(pstcPllInit->u32PLLState));
        DDL_ASSERT(IS_DSOGI_PLL_VOLT_JUDGE_STATE(pstcPllInit->u32JudgeState));
        DDL_ASSERT(IS_DSOGI_PLL_DELAYTIME(pstcPllInit->u32DelayTime));

        MODIFY_REG32(DSOGI_PLLx->CR, (DSOGI_PLL_CR_PI_EN | DSOGI_PLL_CR_PLL_EN | DSOGI_PLL_CR_JUDGE_EN | DSOGI_PLL_CR_DELAY_TIME), \
                     pstcPllInit->u32PIState | pstcPllInit->u32PLLState | pstcPllInit->u32JudgeState | \
                     pstcPllInit->u32DelayTime << DSOGI_PLL_CR_DELAY_TIME_POS);
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_pll_config_t to default values.
 * @param  [out] pstcPllConfig          Pointer to a @ref stc_pll_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Initialize success
 *           - LL_ERR_INVD_PARAM: pstcPllConfig is NULL
 */
int32_t DSOGI_PLL_Pll_ConfigInit(stc_pll_config_t *pstcPllConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcPllConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcPllConfig->pll_k            = 1.0f;
        pstcPllConfig->pll_kp           = 0.033f;
        pstcPllConfig->pll_ki_ts        = 0.00000152f * 5.0f * 5.0f;
        pstcPllConfig->pll_max          = 15.0f;
        pstcPllConfig->pll_min          = -15.f;
        pstcPllConfig->pll_err          = 1.0f;
        pstcPllConfig->pll_theta_cmp1   = 0.0314159265f;
        pstcPllConfig->pll_theta_cmp2   = 3.110176727f;
        pstcPllConfig->pll_theta_cmp3   = 3.17300858f;
        pstcPllConfig->pll_theta_cmp4   = 6.251769380f;
        pstcPllConfig->pll_v_cmp        = 10.0f;
        pstcPllConfig->pll_theta_step   = DSOGI_PLL_PI / 10.0f;
        pstcPllConfig->pll_w            = DSOGI_PLL_CENTER_ANGLE_FREQ;
    }

    return i32Ret;
}

/**
 * @brief  Configure clark parameters.
* @param  [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcClarkConfig         Pointer to a @ref stc_clark_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Configure success
 *           - LL_ERR_INVD_PARAM: pstcClarkConfig is NULL
 */
int32_t DSOGI_PLL_Clark_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_clark_config_t *pstcClarkConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcClarkConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->CLARKK, pstcClarkConfig->clark_k);
    }

    return i32Ret;
}

/**
 * @brief  Configure sogi parameters.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcSogiConfig          Pointer to a @ref stc_sogi_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Configure success
 *           - LL_ERR_INVD_PARAM: pstcSogiConfig is NULL
 */
int32_t DSOGI_PLL_Sogi_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_sogi_config_t *pstcSogiConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcSogiConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->K, pstcSogiConfig->sogi_k);
        FLOAT_WRITE_REG32(DSOGI_PLLx->DK, pstcSogiConfig->sogi_d_k);
        FLOAT_WRITE_REG32(DSOGI_PLLx->QK, pstcSogiConfig->sogi_q_k);
        FLOAT_WRITE_REG32(DSOGI_PLLx->FLLK, pstcSogiConfig->sogi_fll_k);
        FLOAT_WRITE_REG32(DSOGI_PLLx->MAX, pstcSogiConfig->sogi_max);
        FLOAT_WRITE_REG32(DSOGI_PLLx->MIN, pstcSogiConfig->sogi_min);
        FLOAT_WRITE_REG32(DSOGI_PLLx->W, pstcSogiConfig->sogi_w);
    }

    return i32Ret;
}

/**
 * @brief  Configure park parameters.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcParkConfig          Pointer to a @ref stc_park_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Configure success
 *           - LL_ERR_INVD_PARAM: pstcParkConfig is NULL
 */
int32_t DSOGI_PLL_Park_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_park_config_t *pstcParkConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcParkConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->PARKTHETA, pstcParkConfig->park_theta);
    }

    return i32Ret;
}

/**
 * @brief  Configure pll parameters.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] pstcPllConfig           Pointer to a @ref stc_pll_config_t structure.
 * @retval int32_t:
 *           - LL_OK: Configure success
 *           - LL_ERR_INVD_PARAM: pstcPllConfig is NULL
 */
int32_t DSOGI_PLL_Pll_Config(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, const stc_pll_config_t *pstcPllConfig)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcPllConfig) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLK, pstcPllConfig->pll_k);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLKP, pstcPllConfig->pll_kp);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLKITS, pstcPllConfig->pll_ki_ts);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLMAX, pstcPllConfig->pll_max);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLMIN, pstcPllConfig->pll_min);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLFINISHERRTH, pstcPllConfig->pll_err);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLTHETACMP1, pstcPllConfig->pll_theta_cmp1);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLTHETACMP2, pstcPllConfig->pll_theta_cmp2);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLTHETACMP3, pstcPllConfig->pll_theta_cmp3);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLTHETACMP4, pstcPllConfig->pll_theta_cmp4);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLVGCMP, pstcPllConfig->pll_v_cmp);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLTHETASTEP, pstcPllConfig->pll_theta_step);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLW, pstcPllConfig->pll_w);
    }

    return i32Ret;
}

/**
 * @brief  Set SOGI run function.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32Func                 Run mode @ref DSOGI_PLL_FUNC_SEL.
 * @retval None
 */
void DSOGI_PLL_Set_Func(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Func)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_FUNC(u32Func));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_FUN_MASK, u32Func);
}

/**
 * @brief  Set PLL enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   When function is Combination, if PLL is disabele the sogi's w be written through owner register.
 */
void DSOGI_PLL_PLL_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        if (DSOGI_PLL_CR_MODE == (DSOGI_PLLx->CR & DSOGI_PLL_CR_MODE)) {
            SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_PLL_EN);
        }
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_PLL_EN);
    }
}

/**
 * @brief  Set SOGI pi enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   When PI is 0, the out freq of pll equal base-frequency.
 */
void DSOGI_PLL_PI_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_PI_EN);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_PI_EN);
    }
}

/**
 * @brief  Set SOGI fll enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   When FLL_EN is 1, SOGI's w be updated with FLL's results.
 */
void DSOGI_PLL_FLL_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_FLL_EN);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_FLL_EN);
    }
}

/**
 * @brief  Set SOGI judge enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   When the PLL operates independently, this signal is inactive.
 */
void DSOGI_PLL_Judge_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        if (DSOGI_PLL_CR_MODE == (DSOGI_PLLx->CR & DSOGI_PLL_CR_MODE)) {
            SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_JUDGE_EN);
        }
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_JUDGE_EN);
    }
}

/**
 * @brief  Set SOGI sogi calculation count.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32SogiSelect           Calculation count @ref DSOGI_PLL_SOGI_SC_SEL.
 * @retval None
 * @note   The function is valid only when the SOGI is independently run.
 */
void DSOGI_PLL_Sogi_Calc_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32SogiSelect)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_SOGI_CALC(u32SogiSelect));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_SOGI_SC, u32SogiSelect);
}

/**
 * @brief  Enable or disable PLL sin/cos calculation.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   The function is valid only when the Double-SOGI and Single-Phase mode.
 */
void DSOGI_PLL_PLL_SC_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_SOGI_SC);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_SOGI_SC);
    }
}

/**
 * @brief  Set SOGI park output select.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32ThetaSelect          Specifies the d/q signal of park output to pll @ref DSOGI_PLL_PARK_THETA_SEL.
 * @retval None
 * @note   The function is inavalid when the park mode is independently.
 */
void DSOGI_PLL_Park_Theta_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32ThetaSelect)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_PARK_THETA(u32ThetaSelect));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_THETA_SEL, u32ThetaSelect);
}

/**
 * @brief  Set SOGI clark input select.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32InputSelect          Specifies the input source of clark of SOGI @ref DSOGI_PLL_CLARK_INPUT_SEL.
 * @retval None
 * @note   When the SOGI function is independently, can only be write via abc registers.
 */
void DSOGI_PLL_Clark_Input_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32InputSelect)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_CLARK_SRC(u32InputSelect));

    MODIFY_REG32(DSOGI_PLLx->CR, (DSOGI_PLL_CR_ADC_SEL | DSOGI_PLL_CR_DIN_SEL), u32InputSelect);
}

/**
 * @brief  Set SOGI clark input voltage select.
* @param   [in] DSOGI_PLLx                   Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32VoltageSelect        Specifies the input voltage source of clark of SOGI @ref DSOGI_PLL_CLARK_VOLT_SEL.
 * @retval None
 */
void DSOGI_PLL_Clark_Voltage_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32VoltageSelect)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_CLARK_VOLT(u32VoltageSelect));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CLARK_VOLT_MASK, u32VoltageSelect);
}

/**
 * @brief  Set SOGI clark adc gain select.
* @param   [in] DSOGI_PLLx                   Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32AdcGainSelect        Specifies the adc gain of clark of SOGI @ref DSOGI_PLL_ADC_GAIN_SEL.
 * @retval None
 * @note   The function is valid only when the clark input source is ADC.
 */
void DSOGI_PLL_Clark_Adc_Gain_Select(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32AdcGainSelect)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_CLARK_ADC_GAIN(u32AdcGainSelect));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_ADC_GN, u32AdcGainSelect);
}

/**
 * @brief  Set SOGI locked phase delaytime.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32DelayTime            Less than SOGI_DELAYTIME_MAX. actual value equal u32DelayTime * 256.
 * @retval None
 */
void DSOGI_PLL_Set_Delaytime(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32DelayTime)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_DELAYTIME(u32DelayTime));

    MODIFY_REG32(DSOGI_PLLx->CR, DSOGI_PLL_CR_DELAY_TIME, u32DelayTime << DSOGI_PLL_CR_DELAY_TIME_POS);
}

/**
 * @brief  Set SOGI interrupt enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32IntType              Specifical interrupt @ref DSOGI_PLL_INT.
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void DSOGI_PLL_Int_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32IntType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_INT(u32IntType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, u32IntType);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, u32IntType);
    }
}

/**
 * @brief  Get SOGI State flags.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32Flag            SOGI state flag. Can be one or any
 *                                  combination of the parameter of @ref DSOGI_PLL_FLAG
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t DSOGI_PLL_GetStatus(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_FLAG(u32Flag));

    return (0UL == (DSOGI_PLLx->ST & u32Flag)) ? RESET : SET;
}

/**
 * @brief  Clear SOGI state flags.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance
 * @param  [in] u32Flag            SOGI state flag. Can be one or any
 *                                  combination of the parameter of @ref DSOGI_PLL_CLR_FLAG
 * @retval None.
 */
void DSOGI_PLL_ClearStatus(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_DSOGI_PLL_CLR_FLAG(u32Flag));

    WRITE_REG32(DSOGI_PLLx->ST, u32Flag);
}

/**
 * @brief  Set SOGI start enable or disable.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void DSOGI_PLL_Start_Cmd(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    /* When mode is 1, it functions as an enable bit (remains active after writing 1 and inactive after writing 0).
       When mode is 0, it functions as a trigger enable bit (activates operation after writing 1; writing 0 has no effect,
       and the bit automatically resets to zero after calculate completes). */
    if (ENABLE == enNewState) {
        SET_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_START);
    } else {
        CLR_REG32_BIT(DSOGI_PLLx->CR, DSOGI_PLL_CR_START);
    }
}

/**
 * @brief  Write SOGI clark input value.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] pstcClarkInput          Pointer to a @ref stc_clark_input_t structure.
 * @retval int32_t:
 *           - LL_OK: Write success
 *           - LL_ERR_INVD_PARAM: pstcClarkInput is NULL
 * @note   The function ia valid when clark input source is abc.
 */
int32_t DSOGI_PLL_Clark_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_clark_input_t *pstcClarkInput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));
    DDL_ASSERT(0UL == (DSOGI_PLLx->CR & DSOGI_PLL_CR_DIN_SEL));

    if (NULL == pstcClarkInput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->CLARKAS, pstcClarkInput->a);
        FLOAT_WRITE_REG32(DSOGI_PLLx->CLARKBS, pstcClarkInput->b);
        FLOAT_WRITE_REG32(DSOGI_PLLx->CLARKCS, pstcClarkInput->c);
    }

    return i32Ret;
}

/**
 * @brief  Read SOGI clark output value.
* @param   [in] DSOGI_PLLx                   Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] pstcClarkOutput         Pointer to a @ref stc_clark_output_t structure.
 * @retval int32_t:
 *           - LL_OK: Read success
 *           - LL_ERR_INVD_PARAM: pstcClarkOutput is NULL
 */
int32_t DSOGI_PLL_Clark_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_clark_output_t *pstcClarkOutput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcClarkOutput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcClarkOutput->alpha      = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->CLARKDS);
        pstcClarkOutput->beta       = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->CLARKQS);
        pstcClarkOutput->a_filter   = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->CLARKFLTA);
        pstcClarkOutput->b_filter   = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->CLARKFLTB);
        pstcClarkOutput->c_filter   = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->CLARKFLTC);
    }

    return i32Ret;
}

/**
 * @brief  Write SOGI sogi input signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] pstcSogiInput           Pointer to a @ref stc_sogi_input_t structure.
 * @retval int32_t:
 *           - LL_OK: Write success
 *           - LL_ERR_INVD_PARAM: pstcSogiInput is NULL
 * @note    The function is valid only when SOGI work on independently mode.
 */
int32_t DSOGI_PLL_Sogi_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_sogi_input_t *pstcSogiInput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcSogiInput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->DS, pstcSogiInput->alpha);
        /* SOGI indenpedent run and selcect twice sogi calculatin */
        if (DSOGI_PLL_CR_SOGI_SC == (DSOGI_PLLx->CR & DSOGI_PLL_CR_SOGI_SC)) {
            FLOAT_WRITE_REG32(DSOGI_PLLx->QS, pstcSogiInput->beta);
        }
    }

    return i32Ret;
}

/**
 * @brief  Read SOGI sogi output signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [out] pstcSogiOutput         Pointer to a @ref stc_sogi_output_t structure.
 * @retval int32_t:
 *           - LL_OK: Read success
 *           - LL_ERR_INVD_PARAM: pstcSogiOutput is NULL
 */
int32_t DSOGI_PLL_Sogi_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_sogi_output_t *pstcSogiOutput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcSogiOutput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcSogiOutput->alpha_dv    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->ALPHADV);
        pstcSogiOutput->alpha_qv    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->ALPHAQV);
        pstcSogiOutput->beta_dv     = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->BETADV);
        pstcSogiOutput->beta_qv     = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->BETAQV);
        pstcSogiOutput->alpha_p     = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->VGRIDPDS);
        pstcSogiOutput->beta_p      = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->VGRIDPQS);
        pstcSogiOutput->alpha_n     = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->VGRIDNDS);
        pstcSogiOutput->beta_n      = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->VGRIDNQS);
    }

    return i32Ret;
}

/**
 * @brief  Write SOGI park input signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] pstcParkInput           Pointer to a @ref stc_park_input_t structure.
 * @retval int32_t:
 *           - LL_OK: Write success
 *           - LL_ERR_INVD_PARAM: pstcParkInput is NULL
 * @note    The function is valid only when park work on independently mode.
 */
int32_t DSOGI_PLL_Park_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_park_input_t *pstcParkInput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcParkInput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->PARKDS, pstcParkInput->alpha);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PARKQS, pstcParkInput->beta);
        FLOAT_WRITE_REG32(DSOGI_PLLx->PARKTHETA, pstcParkInput->theta);
    }

    return i32Ret;
}

/**
 * @brief  Read SOGI park output signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [out] pstcParkOutput         Pointer to a @ref stc_park_output_t structure.
 * @retval int32_t:
 *           - LL_OK: Read success
 *           - LL_ERR_INVD_PARAM: pstcParkOutput is NULL
 */
int32_t DSOGI_PLL_Park_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_park_output_t *pstcParkOutput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcParkOutput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcParkOutput->d    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PARKDE);
        pstcParkOutput->q    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PARKQE);
        pstcParkOutput->sin_theta = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PARKSIN);
        pstcParkOutput->cos_theta = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PARKCOS);
    }

    return i32Ret;
}

/**
 * @brief  Write SOGI input signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [in] pstcPllInput            Pointer to a @ref stc_pll_input_t structure.
 * @retval int32_t:
 *           - LL_OK: Write success
 *           - LL_ERR_INVD_PARAM: pstcPllInput is NULL
 * @note    The function is valid only when PLL work on independently mode.
 */
int32_t DSOGI_PLL_Pll_Input(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_pll_input_t *pstcPllInput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcPllInput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        FLOAT_WRITE_REG32(DSOGI_PLLx->PLLDIN, pstcPllInput->din);
    }

    return i32Ret;
}

/**
 * @brief  Read SOGI park output signal.
* @param   [in] DSOGI_PLLx                Pointer to SOGI unit instance
 *         This parameter can be one of the following values:
 *           @arg CM_DSOGI_PLL or CM_DSOGI_PLL_x: SOGI unit instance.
 * @param  [out] pstcPllOutput          Pointer to a @ref stc_pll_output_t structure.
 * @retval int32_t:
 *           - LL_OK: Read success
 *           - LL_ERR_INVD_PARAM: pstcPllOutput is NULL
 */
int32_t DSOGI_PLL_Pll_Output(CM_DSOGI_PLL_TypeDef *DSOGI_PLLx, stc_pll_output_t *pstcPllOutput)
{
    int32_t i32Ret = LL_OK;

    DDL_ASSERT(IS_DSOGI_PLL_PERIPH(DSOGI_PLLx));

    if (NULL == pstcPllOutput) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcPllOutput->w            = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLW);
        pstcPllOutput->theta        = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLTHETA);
        pstcPllOutput->theta_pi     = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLTHETA_PI);
        pstcPllOutput->freq         = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLFREQ);
        pstcPllOutput->p            = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLUP);
        pstcPllOutput->i            = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLUI);
        pstcPllOutput->pi           = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLPI);
        pstcPllOutput->din_filter   = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLDINFLT);
        pstcPllOutput->sin_theta    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLSIN);
        pstcPllOutput->cos_theta    = DSOGI_PLL_FLOAT_CONV(DSOGI_PLLx->PLLCOS);
    }

    return i32Ret;
}

/**
 * @}
 */

#endif /* LL_DSOGI_PLL_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
