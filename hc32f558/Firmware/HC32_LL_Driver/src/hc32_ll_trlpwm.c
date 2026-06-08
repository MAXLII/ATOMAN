/**
 *******************************************************************************
 * @file  hc32_ll_trlpwm.c
 * @brief This file provides firmware functions to manage the Three level PWM
 *        Unit(TRLPWM).
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
#include "hc32_ll_trlpwm.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_TRLPWM TRLPWM
 * @brief Three level PWM Unit Driver Library
 * @{
 */

#if (LL_TRLPWM_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup TRLPWM_Local_Macros TRLPWM Local Macros
 * @{
 */

#define TRLPWM_RMU_TIMEOUT                (100U)

#define TRLPWM_LEGx_REG(reg_base, unit)   (__IO uint32_t *)((uint32_t)(&(reg_base)) + ((unit) * 0x20UL))

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
 * @defgroup TRLPWM_Global_Functions TRLPWM Global Functions
 * @{
 */

/**
 * @brief  Set the fields of structure stc_trlpwm_base_t to default values
 * @param  [out] pstcInit           Pointer to a @ref stc_trlpwm_base_t structure
 * @retval int32_t:
 *           - LL_OK:               Initialize successfully
 *           - LL_ERR_INVD_PARAM:   The pointer pstcInit value is NULL
 */
int32_t TRLPWM_StructInit(stc_trlpwm_base_t *pstcInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcInit) {
        pstcInit->enCBCEn = DISABLE;
        pstcInit->enResumeAlignEn = DISABLE;
        pstcInit->enOutPutPolarity = HIGH;
        pstcInit->enBrakePolarity = HIGH;
        pstcInit->u16CBCDelayDiv = 127U;
        pstcInit->u16OpenShutDownDelayDiv = 11U;

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initializes TRLPWM Base function
 * @param  [in] TRLPWMx            Pointer to TRLPWM instance register base
 * @param  [in] pstcInit           Pointer to a @ref stc_trlpwm_base_t structure
 * @retval int32_t:
 *           - LL_OK: Initializes success
 *           - LL_ERR_INVD_PARAM: pstcInit == NULL
 */
int32_t TRLPWM_Init(CM_TRLPWM_TypeDef *TRLPWMx, const stc_trlpwm_base_t *pstcInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        /* Check parameters */
        DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->enCBCEn));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->enResumeAlignEn));
        DDL_ASSERT(IS_LEVEL_STATE(pstcInit->enOutPutPolarity));
        DDL_ASSERT(IS_LEVEL_STATE(pstcInit->enBrakePolarity));
        DDL_ASSERT(IS_CBCDLY_DIV(pstcInit->u16CBCDelayDiv));
        DDL_ASSERT(IS_OPENSHUTDOWNDLY_DIV(pstcInit->u16OpenShutDownDelayDiv));

        MODIFY_REG32(TRLPWMx->CR, (TRLPWM_CR_CBCEB | TRLPWM_CR_RESUMEALIGNEN | TRLPWM_CR_POLARITY | \
                                   TRLPWM_CR_BRAKEPOL | TRLPWM_CR_CBCPREDIV | TRLPWM_CR_OPENSHUTDOWNPREDIV), \
                     ((uint32_t)pstcInit->enCBCEn << TRLPWM_CR_CBCEB_POS | \
                      (uint32_t)pstcInit->enResumeAlignEn << TRLPWM_CR_RESUMEALIGNEN_POS | \
                      (uint32_t)pstcInit->enOutPutPolarity << TRLPWM_CR_POLARITY_POS | \
                      (uint32_t)pstcInit->enBrakePolarity << TRLPWM_CR_BRAKEPOL_POS | \
                      (uint32_t)pstcInit->u16CBCDelayDiv << TRLPWM_CR_CBCPREDIV_POS | \
                      (uint32_t)pstcInit->u16OpenShutDownDelayDiv << TRLPWM_CR_OPENSHUTDOWNPREDIV_POS));
    }

    return i32Ret;
}

/**
 * @brief  Set the fields of structure stc_trlpwm_leg_t to default values
 * @param  [out] pstcInit           Pointer to a @ref stc_trlpwm_leg_t structure
 * @retval int32_t:
 *           - LL_OK:               Initialize successfully
 *           - LL_ERR_INVD_PARAM:   The pointer pstcInit value is NULL
 */
int32_t TRLPWM_LegStructInit(stc_trlpwm_leg_t *pstcInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (NULL != pstcInit) {
        pstcInit->enOutputBypassEn = DISABLE;
        pstcInit->enInputInvEn = DISABLE;
        pstcInit->enANPCEn = DISABLE;
        pstcInit->u16FilterDiv = TRLPWM_FILTER_DIV1;
        pstcInit->u16FilterN = 0U;
        pstcInit->u16ShutDownDelay = 0U;
        pstcInit->u16OpenDelay = 0U;
        pstcInit->u16CBCDelay = 0U;
        pstcInit->u16AlignDelay = 2U;

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief  Initializes TRLPWM leg
 * @param  [in] TRLPWMx         Pointer to TRLPWM instance register base
 * @param  [in] u16LEGx         TRLPWM Leg unit @ref TRLPWM_Leg_Unit
 * @param  [in] pstcInit        Pointer to a @ref stc_trlpwm_leg_t structure
 * @retval int32_t:
 *           - LL_OK: Initializes success
 *           - LL_ERR_INVD_PARAM: pstcInit == NULL
 */
int32_t TRLPWM_LegInit(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, const stc_trlpwm_leg_t *pstcInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;
    uint8_t u8LegIndex = 0U;
    __IO uint32_t *RegAddr;

    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_TRLPWM_LEG_UNIT(u16LEGx));

    if (NULL != pstcInit) {
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->enOutputBypassEn));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->enInputInvEn));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->enANPCEn));
        DDL_ASSERT(IS_FILTER_DIV(pstcInit->u16FilterDiv));
        DDL_ASSERT(IS_FILTER_N(pstcInit->u16FilterN));
        DDL_ASSERT(IS_SHUTDOWN_DELAY(pstcInit->u16ShutDownDelay));
        DDL_ASSERT(IS_OPEN_DELAY(pstcInit->u16OpenDelay));
        DDL_ASSERT(IS_CBC_DELAY(pstcInit->u16CBCDelay));
        DDL_ASSERT(IS_ALIGN_DELAY(pstcInit->u16AlignDelay));

        while (0U != u16LEGx) {
            if (0U != (u16LEGx & 0x01U)) {
                /* CTLLEGx */
                RegAddr = TRLPWM_LEGx_REG(TRLPWMx->CTLLEG0, u8LegIndex);
                MODIFY_REG32(*RegAddr, (TRLPWM_CTLLEG_OUTPUTSTAGEBYPASS | \
                                        TRLPWM_CTLLEG_INPUTPOLINV | TRLPWM_CTLLEG_ANPCEN | \
                                        TRLPWM_CTLLEG_FLTSD | TRLPWM_CTLLEG_FLTSEL), \
                             ((uint32_t)pstcInit->enOutputBypassEn << TRLPWM_CTLLEG_OUTPUTSTAGEBYPASS_POS | \
                              (uint32_t)pstcInit->enInputInvEn << TRLPWM_CTLLEG_INPUTPOLINV_POS | \
                              (uint32_t)pstcInit->enANPCEn << TRLPWM_CTLLEG_ANPCEN_POS | \
                              (uint32_t)pstcInit->u16FilterDiv << TRLPWM_CTLLEG_FLTSD_POS | \
                              (uint32_t)pstcInit->u16FilterN << TRLPWM_CTLLEG_FLTSEL_POS));

                /* DLYLEGx */
                RegAddr = TRLPWM_LEGx_REG(TRLPWMx->DLYLEG0, u8LegIndex);
                MODIFY_REG32(*RegAddr, (TRLPWM_DLYLEG_CBCRESUMEDLY | TRLPWM_DLYLEG_OPENDLY | TRLPWM_DLYLEG_SHUTDOWNDLY), \
                             ((uint32_t)pstcInit->u16CBCDelay << TRLPWM_DLYLEG_CBCRESUMEDLY_POS | \
                              (uint32_t)pstcInit->u16OpenDelay << TRLPWM_DLYLEG_OPENDLY_POS | \
                              (uint32_t)pstcInit->u16ShutDownDelay << TRLPWM_DLYLEG_SHUTDOWNDLY_POS));

                /* MISCCTRLx */
                RegAddr = TRLPWM_LEGx_REG(TRLPWMx->MISCCTRL0, u8LegIndex);
                /* Align Delay */
                MODIFY_REG32(*RegAddr, TRLPWM_MISCCTRL_LEG_ALIGNDLY, (uint32_t)pstcInit->u16AlignDelay << TRLPWM_MISCCTRL_LEG_ALIGNDLY_POS);
            }
            u16LEGx >>= 1U;
            u8LegIndex++;
        }

        i32Ret = LL_OK;
    }

    return i32Ret;
}

/**
 * @brief TRLPWM leg command
 * @param [in] TRLPWMx              Pointer to TRLPWM instance register base
 * @param  [in] u16LEGx             TRLPWM Leg unit @ref TRLPWM_Leg_Unit
 * @param [in] enNewState           An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void TRLPWM_LegCmd(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, en_functional_state_t enNewState)
{
    uint8_t u8LegIndex = 0U;
    __IO uint32_t *RegAddr;

    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_TRLPWM_LEG_UNIT(u16LEGx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    while (0U != u16LEGx) {
        if (0U != (u16LEGx & 0x01U)) {
            /* CTLLEGx */
            RegAddr = TRLPWM_LEGx_REG(TRLPWMx->CTLLEG0, u8LegIndex);
            MODIFY_REG32(*RegAddr, TRLPWM_CTLLEG_TRLPWMLEG1EN, (uint32_t)enNewState << TRLPWM_CTLLEG_TRLPWMLEG1EN_POS);
        }
        u16LEGx >>= 1U;
        u8LegIndex++;
    }
}

/**
 * @brief SET the TRLPWM leg mux route
 * @param [in] TRLPWMx          Pointer to TRLPWM instance register base
 * @param [in] u16LEGx          TRLPWM Leg unit @ref TRLPWM_Leg_Unit
 * @param [in] u32Mask          Selects the input signal sources for the T1-T6 devices within the specified leg.
 * @retval None
 */
void TRLPWM_LegMuxRouteMask(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, uint32_t u32Mask)
{
    uint8_t u8LegIndex = 0U;
    __IO uint32_t *RegAddr;

    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_TRLPWM_LEG_UNIT(u16LEGx));
    DDL_ASSERT(IS_MUX_ROUTE_MASK(u32Mask));

    while (0U != u16LEGx) {
        if (0U != (u16LEGx & 0x01U)) {
            /* MISCCTRLx */
            RegAddr = TRLPWM_LEGx_REG(TRLPWMx->MISCCTRL0, u8LegIndex);
            /* Mux Route */
            MODIFY_REG32(*RegAddr, TRLPWM_MISCCTRL_LEGT1IN_SEL | TRLPWM_MISCCTRL_LEGT2IN_SEL |
                         TRLPWM_MISCCTRL_LEGT3IN_SEL | TRLPWM_MISCCTRL_LEGT4IN_SEL |
                         TRLPWM_MISCCTRL_LEGT5IN_SEL | TRLPWM_MISCCTRL_LEGT6IN_SEL, u32Mask);
        }
        u16LEGx >>= 1U;
        u8LegIndex++;
    }
}

/**
 * @brief Get the TRLPWM leg status
 * @param [in] TRLPWMx          Pointer to TRLPWM instance register base
 * @param [in] u16LEGx          TRLPWM Leg unit @ref TRLPWM_Leg_Unit
 * @param [in] u32Flag          Specify the flags to check, This parameter can be any combination of the member from
 *                              @ref TRLPWM_LegStatus_Flag
 * @retval uint32_t             The current operational status of the specified leg @ref TRLPWM_Leg_WorkNow.
 */
uint32_t TRLPWM_LegGetStatus(CM_TRLPWM_TypeDef *TRLPWMx, uint16_t u16LEGx, uint32_t u32Flag)
{
    uint8_t u8LegIndex = 0U;
    __IO uint32_t *RegAddr = NULL;

    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));
    DDL_ASSERT(IS_TRLPWM_LEG_UNIT(u16LEGx));
    DDL_ASSERT(IS_WORKNOW_FLGA(u32Flag));

    while (0U != u16LEGx) {
        if (0U != (u16LEGx & 0x01U)) {
            RegAddr = TRLPWM_LEGx_REG(TRLPWMx->SRLEG0, u8LegIndex);
            break;
        }
        u16LEGx >>= 1U;
        u8LegIndex++;
    }

    return READ_REG32_BIT(*RegAddr, u32Flag);
}

/**
 * @brief  Get the TRLPWM version
 * @param [in] TRLPWMx         Pointer to TRLPWM instance register base
 * @retval uint32_t            TRLPWM version
 */
uint32_t TRLPWM_GetVersion(CM_TRLPWM_TypeDef *TRLPWMx)
{
    DDL_ASSERT(IS_TRLPWM_UNIT(TRLPWMx));

    return (READ_REG32(TRLPWMx->REVIDR) & TRLPWM_REVIDR_REVID);
}

/**
 * @brief  De-Initialize TRLPWM function
 * @param  None
 * @retval int32_t:
 *         - LL_OK:           Reset success
 *         - LL_ERR_TIMEOUT:  Reset time out
 * @note   Call LL_PERIPH_WE(LL_PERIPH_PWC_CLK_RMU) unlock RMU_FRSTx register first
 */
int32_t TRLPWM_DeInit(void)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;

    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);
    CLR_REG32(bCM_RMU->FRST2_b.TRLPWM);
    /* Ensure reset procedure is completed */
    while (1UL != READ_REG32(bCM_RMU->FRST2_b.TRLPWM)) {
        u32TimeOut++;
        if (u32TimeOut > TRLPWM_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @}
 */

#endif /* LL_TRLPWM_ENABLE */

/**
 * @}
 */

/**
* @}
*/

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
