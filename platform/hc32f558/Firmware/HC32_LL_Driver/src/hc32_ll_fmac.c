/**
 *******************************************************************************
 * @file  hc32_ll_fmac.c
 * @brief This file provides firmware functions to manage the Filter Math
 *        Accelerate (FMAC).
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
#include "hc32_ll_fmac.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_FMAC FMAC
 * @brief FMAC Driver Library
 * @{
 */

#if (LL_FMAC_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup FMAC_Local_Macros FMAC Local Macros
 * @{
 */
#define FMAC_RMU_TIMEOUT                        (100U)

/**
 * @defgroup FMAC_Check_Parameters_Validity FMAC Check Parameters Validity
 * @{
 */
#define IS_FMAC_PWC_UNLOCKED()                  ((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1)

#define IS_FMAC_UNIT(x)                         ((x) == CM_FMAC)

#define IS_FMAC_FIR_ORDER(x)        (((x) != 0UL) && ((x) <= FMAC_FIR_MAX_ORDER))

#define IS_FMAC_FIR_SRC(x)                                                     \
(   ((x) == FAMC_FIR_SRC_REG)                                               || \
    ((x) == FAMC_FIR_SRC_ADC1_CH2)                                          || \
    ((x) == FAMC_FIR_SRC_ADC2_CH5)                                          || \
    ((x) == FAMC_FIR_SRC_ADC3_CH5))

#define IS_FMAC_CALC_MODE(x)                                                   \
(   ((x) == FMAC_CALC_MODE_SOFT)                                            || \
    ((x) == FMAC_CALC_MODE_HARD))

#define IS_FMAC_OUT_TIME(x)                                                    \
(   ((x) == FMAC_DATA_OUT_IMMED)                                            || \
    ((x) == FMAC_DATA_OUT_WAIT))

#define IS_FMAC_AMP_LIMIT_STAT(x)                                              \
(   ((x) == FMAC_AMP_LIMIT_ENABLE)                                          || \
    ((x) == FMAC_AMP_LIMIT_DISABLE))

#define IS_FMAC_INT_STAT(x)                                                    \
(   ((x) == FMAC_INT_ENABLE)                                                || \
    ((x) == FMAC_INT_DISABLE))

#define IS_FMAC_FIR_FUNC(x)                                                    \
(   ((x) == FMAC_FIR_FUNC_FIR)                                              || \
    ((x) == FMAC_FIR_FUNC_MAF))

#define IS_FMAC_FIR_FLAG(x)                                                    \
(   ((x) != 0UL)                                                            && \
    ((x) | FMAC_FIR_FLAG_ALL) == FMAC_FIR_FLAG_ALL)

#define IS_FMAC_FIR_FLAG_CLR(x)                                                \
(   ((x) != 0UL)                                                            && \
    ((x) | FMAC_FIR_FLAG_CLR_ALL) == FMAC_FIR_FLAG_CLR_ALL)

#define IS_FMAC_FIR_READ_TYPE(x)                                               \
(   ((x) == FMAC_READ_TYPE_X)                                               || \
    ((x) == FMAC_READ_TYPE_C))

#define IS_FMAC_IIR_SRC(x)                          ((x) <= FAMC_IIR_SRC_PID2_U)

#define IS_FMAC_IIR_CH(x)                                                      \
(   ((x) == FMAC_IIR_CH1)                                                   || \
    ((x) == FMAC_IIR_CH2)                                                   || \
    ((x) == FMAC_IIR_CH3)                                                   || \
    ((x) == FMAC_IIR_CH4)                                                   || \
    ((x) == FMAC_IIR_CH5)                                                   || \
    ((x) == FMAC_IIR_CH6))

#define IS_FMAC_IIR_LP_CH(x)                                                   \
(   ((x) == FMAC_IIR_CH1)                                                   || \
    ((x) == FMAC_IIR_CH2))

#define IS_FMAC_IIR_ORDER(x)        (((x) != 0UL) && ((x) <= FMAC_IIR_MAX_ORDER))

#define IS_FMAC_IIR_FLAG(x)                                                    \
(   ((x) != 0UL)                                                            && \
    ((x) | FMAC_IIR_FLAG_ALL) == FMAC_IIR_FLAG_ALL)

#define IS_FMAC_IIR_FLAG_CLR(x)                                                \
(   ((x) != 0UL)                                                            && \
    ((x) | FMAC_IIR_FLAG_CLR_ALL) == FMAC_IIR_FLAG_CLR_ALL)

#define IS_FMAC_IIR_READ_TYPE(x)                                               \
(   ((x) == FMAC_READ_TYPE_X)                                               || \
    ((x) == FMAC_READ_TYPE_C)                                               || \
    ((x) == FMAC_READ_TYPE_Y))

#define IS_FMAC_ECC_UNIT(x)                         ((x) == CM_FMAC_ECC)

#define IS_FMAC_RAM_ECC_RAM(x)                                                 \
(   ((x) == FMAC_ECC_RAM0)                                                  || \
    ((x) == FMAC_ECC_RAM1))

#define IS_FMAC_ECC_FLAG(x)                                                    \
(   ((x) != 0UL)                                                            && \
    ((x) | FMAC_ECC_FLAG_ALL) == FMAC_ECC_FLAG_ALL)

#define IS_FMAC_RAM_ECC_MD(x)                                                  \
(   ((x) == FMAC_RAM_ECC_INVD)                                              || \
    ((x) == FMAC_RAM_ECC_MD1)                                               || \
    ((x) == FMAC_RAM_ECC_MD2)                                               || \
    ((x) == FMAC_RAM_ECC_MD3))
/**
 * @}
 */

/* FMAC IIR Maximum Channel */
#define FMAC_IIR_MAX_CH                         (6U)
/* FMAC IIR Limit Amplitude Support Channel */
#define FMAC_IIR_LP_MAX_CH                      (2U)
/* FMAC IIR Register Offset */
#define FMAC_REG_OFFSET                         (4UL)
/* FMAC IIR Status Register Channel Offset */
#define FMAC_IIR_STAT_BIT_OFFSET                (8U)

/**
 * @defgroup FMAC_IIR_Register_Addr FMAC IIR Register Address
 * @{
 */
#define IIR_YR_REG(x)                           ((__I uint32_t *) (&CM_FMAC->IIR_YRDATA1 + FMAC_REG_OFFSET * (x)))
#define IIR_PARM_REG(x)                         ((__IO uint32_t *)(&CM_FMAC->IIR_CH1_PARM + FMAC_REG_OFFSET * (x)))
#define IIR_DATA_CR_REG(x)                      ((__IO uint32_t *)(&CM_FMAC->IIR_DATA_CR1 + FMAC_REG_OFFSET * (x)))
#define IIR_CW_REG(x)                           ((__O uint32_t *) (&CM_FMAC->IIR_CWDATA1 + FMAC_REG_OFFSET * (x)))
#define IIR_XW_REG(x)                           ((__O uint32_t *) (&CM_FMAC->IIR_XWDATA1 + FMAC_REG_OFFSET * (x)))
#define IIR_YW_REG(x)                           ((__O uint32_t *) (&CM_FMAC->IIR_YWDATA1 + FMAC_REG_OFFSET * (x)))
#define IIR_SR_REG(x)                           ((__IO uint32_t *)(&CM_FMAC->IIR_SRADDR1 + FMAC_REG_OFFSET * (x)))
#define IIR_SR_XREG(x)                          ((__I uint32_t *) (&CM_FMAC->IIR_SRDATAX1 + FMAC_REG_OFFSET * (x)))
#define IIR_SR_YREG(x)                          ((__I uint32_t *) (&CM_FMAC->IIR_SRDATAY1 + FMAC_REG_OFFSET * (x)))
#define IIR_SR_CREG(x)                          ((__I uint32_t *) (&CM_FMAC->IIR_SRDATAC1 + FMAC_REG_OFFSET * (x)))

#define RAM_ECC_REG_OFFSET                      (0x20U)

#define RAM_EI_BIT_MASK                         (0x7FFFFFFFFFUL)
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
 * @defgroup FMAC_Local_Func FMAC Local Functions
 * @{
 */

/**
 * @brief  FMAC write data to register
 * @param  [in] pu32Des             Pointer to the Destination address
 * @param  [in] pf32Src             Pointer to the source address
 * @param  [in] u32Size             The number of write data
 * @retval None
 */
static void FAMC_Write_Data(__IO uint32_t *pu32Des, float32_t *pf32Src, uint32_t u32Size)
{
    uint32_t *pu32Src = (uint32_t *)(uint32_t)pf32Src;

    while (u32Size > 0UL) {
        *pu32Des = *pu32Src;
        pu32Src ++;
        u32Size --;
    }
}

/**
 * @brief  FMAC read data to register
 * @param  [out] pf32Des            Pointer to the Destination address
 * @param  [in] pu32Src             Pointer to the source address
 * @param  [in] u32Size             The number of write data
 * @retval None
 */
static void FAMC_Read_Data(float32_t *pf32Des, __I uint32_t *pu32Src, uint32_t u32Size)
{
    uint32_t *pu32Des = (uint32_t *)(uint32_t)pf32Des;

    while (u32Size > 0UL) {
        *pu32Des = *pu32Src;
        pu32Des ++;
        u32Size --;
    }
}
/**
 * @}
 */

/**
 * @defgroup FMAC_Global_Functions FMAC Global Functions
 * @{
 */
/**
 * @brief  De-Initialize FMAC function.
 * @param  [in] FMACx            Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:              FMAC instance register base
 * @retval int32_t:
 *           - LL_OK:           Reset success.
 *           - LL_ERR_TIMEOUT:  Reset time out.
 */
int32_t FMAC_DeInit(CM_FMAC_TypeDef *FMACx)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;

    /* Check parameters */
    DDL_ASSERT(IS_FMAC_PWC_UNLOCKED());

    CLR_REG32_BIT(CM_RMU->FRST1, RMU_FRST1_FMAC);
    /* Ensure reset procedure is completed */
    while (RMU_FRST1_FMAC != READ_REG32_BIT(CM_RMU->FRST1, RMU_FRST1_FMAC)) {
        u32TimeOut++;
        if (u32TimeOut > FMAC_RMU_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  FMAC read parameters initialization structure initialization.
 * @param  [in] pstcFmacReadInit      FMAC read parameters function structure.
 *   @arg  See the structure definition for @ref stc_fmac_read_param_t
 * @retval int32_t:
 *           - LL_OK: Success.
 *           - LL_ERR_INVD_PARAM: Parameter error.
 */
int32_t FMAC_ReadParam_StructInit(stc_fmac_read_param_t *pstcFmacReadInit)
{
    int32_t i32Ret = LL_OK;

    if (NULL == pstcFmacReadInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcFmacReadInit->u8ReadType    = FMAC_READ_TYPE_X;
        pstcFmacReadInit->u32RamAddr    = 0UL;
        pstcFmacReadInit->pf32Data      = NULL;
        pstcFmacReadInit->u16Size       = 0U;
    }

    return i32Ret;
}

/**
 * @brief  FMAC FIR initialization structure initialization.
 * @param  [in] pstcFmacFirInit      FMAC FIR function structure.
 *   @arg  See the structure definition for @ref stc_fmac_fir_init_t
 * @retval int32_t:
 *           - LL_OK: Success.
 *           - LL_ERR_INVD_PARAM: Parameter error.
 */
int32_t FMAC_FIR_StructInit(stc_fmac_fir_init_t *pstcFmacFirInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcFmacFirInit != NULL) {
        pstcFmacFirInit->stcFirInit.u32Src          = FAMC_FIR_SRC_REG;
        pstcFmacFirInit->stcFirInit.u32Order        = 1UL;
        pstcFmacFirInit->stcFirInit.pf32Factor      = NULL;
        pstcFmacFirInit->stcFirInit.u32OutTimeSelect = FMAC_DATA_OUT_WAIT;
        pstcFmacFirInit->stcFirInit.u32LimitAmpCmd  = FMAC_AMP_LIMIT_DISABLE;
        pstcFmacFirInit->stcFirInit.u32IntCmd       = FMAC_INT_ENABLE;
        pstcFmacFirInit->u32Fun                     = FMAC_FIR_FUNC_FIR;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  FMAC FIR function initialize.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] pstcFmacFirInit     FMAC FIR function base parameter structure.
 *   @arg  See the structure definition for @ref stc_fmac_fir_init_t
 * @retval int32_t:
 *           - LL_OK: Success.
 *           - LL_ERR_INVD_PARAM: Parameter error.
 */
int32_t FMAC_FIR_Init(CM_FMAC_TypeDef *FMACx, const stc_fmac_fir_init_t *pstcFmacFirInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Cr = 0UL;

    if (NULL == pstcFmacFirInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_FMAC_UNIT(FMACx));
        DDL_ASSERT(IS_FMAC_FIR_ORDER(pstcFmacFirInit->stcFirInit.u32Order));
        DDL_ASSERT(IS_FMAC_FIR_SRC(pstcFmacFirInit->stcFirInit.u32Src));
        DDL_ASSERT(IS_FMAC_OUT_TIME(pstcFmacFirInit->stcFirInit.u32OutTimeSelect));
        DDL_ASSERT(IS_FMAC_AMP_LIMIT_STAT(pstcFmacFirInit->stcFirInit.u32LimitAmpCmd));
        DDL_ASSERT(IS_FMAC_INT_STAT(pstcFmacFirInit->stcFirInit.u32IntCmd));
        DDL_ASSERT(IS_FMAC_FIR_FUNC(pstcFmacFirInit->u32Fun));
        DDL_ASSERT(NULL != pstcFmacFirInit->stcFirInit.pf32Factor);

        u32Cr |= ((pstcFmacFirInit->stcFirInit.u32OutTimeSelect << FMAC_FIR_CR_UNRD_PRO_POS) | \
                  (pstcFmacFirInit->stcFirInit.u32LimitAmpCmd << FMAC_FIR_CR_TH_EN_POS) | \
                  pstcFmacFirInit->stcFirInit.u32Src | pstcFmacFirInit->u32Fun | FMAC_FIR_CR_DATA_TYPE);
        SET_REG32_BIT(FMACx->FIR_CR, u32Cr);
        WRITE_REG32(FMACx->FIR_INT_CR, pstcFmacFirInit->stcFirInit.u32IntCmd);

        if (FMAC_FIR_FUNC_FIR == pstcFmacFirInit->u32Fun) {
            WRITE_REG32(FMACx->FIR_PARM, (pstcFmacFirInit->stcFirInit.u32Order + 1UL) & FMAC_FIR_PARM_COEF_NUM);
            FAMC_Write_Data(&FMACx->FIR_CWDATA, pstcFmacFirInit->stcFirInit.pf32Factor, pstcFmacFirInit->stcFirInit.u32Order + 1U);
        } else {
            WRITE_REG32(FMACx->FIR_PARM, (pstcFmacFirInit->stcFirInit.u32Order & FMAC_FIR_PARM_COEF_NUM));
            WRITE_REG32(FMACx->FIR_AVG_COEF, *(uint32_t *)(uint32_t)pstcFmacFirInit->stcFirInit.pf32Factor);
        }
    }
    return i32Ret;
}

/**
 * @brief  Set FIR calculate mode.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u32CalcMode         Calculate Mode @ref FMAC_CALC_MODE.
 * @retval None.
 */
void FMAC_FIR_CalcMode(CM_FMAC_TypeDef *FMACx, uint32_t u32CalcMode)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_CALC_MODE(u32CalcMode));

    MODIFY_REG32(FMACx->FIR_CR, FMAC_FIR_CR_CAL_MODE, u32CalcMode << FMAC_FIR_CR_CAL_MODE_POS);
}

/**
 * @brief  Set FIR function enable or disable.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None.
 */
void FMAC_FIR_Cmd(CM_FMAC_TypeDef *FMACx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(FMACx->FIR_CR, FMAC_FIR_CR_ENABLE);
    } else {
        CLR_REG32_BIT(FMACx->FIR_CR, FMAC_FIR_CR_ENABLE);
    }
}

/**
 * @brief  Set FIR interrupt enable or disable.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None.
 */
void FMAC_FIR_IntCmd(CM_FMAC_TypeDef *FMACx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(FMACx->FIR_INT_CR, FMAC_FIR_INT_CR_FIR_DONE_EN);
    } else {
        CLR_REG32_BIT(FMACx->FIR_INT_CR, FMAC_FIR_INT_CR_FIR_DONE_EN);
    }
}

/**
 * @brief  Soft start FIR calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @retval None.
 */
void FMAC_FIR_SWStart(CM_FMAC_TypeDef *FMACx)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));

    SET_REG32_BIT(FMACx->FIR_CR, FMAC_FIR_CR_START);
}

/**
 * @brief  Reset FIR.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @retval None.
 */
void FMAC_FIR_Reset(CM_FMAC_TypeDef *FMACx)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));

    SET_REG32_BIT(FMACx->FIR_CR, FMAC_FIR_CR_SOFT_RST);
}

/**
 * @brief  Clear input data.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @retval None.
 */
void FMAC_FIR_DataClear(CM_FMAC_TypeDef *FMACx)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));

    SET_REG32_BIT(FMACx->FIR_CR, FMAC_FIR_CR_DATA_CLR);
}

/**
 * @brief  Write FIR data to calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] pf32Data            Pointer to the address of write data.
 * @param  [in] u16Size             Size of data.
 * @retval None.
 */
void FMAC_FIR_WriteData(CM_FMAC_TypeDef *FMACx, float32_t *pf32Data, uint16_t u16Size)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(NULL != pf32Data);
    DDL_ASSERT(u16Size <= (FMAC_FIR_MAX_ORDER + 1UL));

    FAMC_Write_Data(&FMACx->FIR_XWDATA, pf32Data, u16Size);
}

/**
 * @brief  Write one FIR data to calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] f32Data             Write data.
 * @retval None.
 */
void FMAC_FIR_WriteOneData(CM_FMAC_TypeDef *FMACx, float32_t f32Data)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));

    uint32_t *pu32Data = (uint32_t *)(uint32_t)&f32Data;

    WRITE_REG32(FMACx->FIR_XWDATA, *pu32Data);
}

/**
 * @brief  Read FIR calculate result.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [out] pf32Data           Pointer to the address for store data.
 * @retval None.
 */
void FMAC_FIR_GetResult(CM_FMAC_TypeDef *FMACx, float32_t *pf32Data)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(NULL != pf32Data);

    uint32_t *pu32Data = (uint32_t *)(uint32_t)pf32Data;

    *pu32Data = READ_REG32(FMACx->FIR_YRDATA);
}

/**
 * @brief  Get FIR Status flag.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u32Flag             FIR state flag. Can be one or any.
 *                                  combination of the parameter of @ref FMAC_FIR_FLAG
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t FMAC_FIR_GetStatus(CM_FMAC_TypeDef *FMACx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_FIR_FLAG(u32Flag));

    return (0UL == READ_REG32_BIT(FMACx->FIR_STA, u32Flag)) ? RESET : SET;
}

/**
 * @brief  Clear FIR state flag.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u32Flag             FIR state flag. Can be one or any.
 *   @arg  FMAC_FIR_FLAG_OVER
 *   @arg  FMAC_FIR_FLAG_INVALID
 *   @arg  FMAC_FIR_FLAG_INFINITE
 *   @arg  FMAC_FIR_FLAG_ERR
 *   @arg  FMAC_FIR_FLAG_DATA_RDY
 * @retval None.
 */
void FMAC_FIR_ClearStatus(CM_FMAC_TypeDef *FMACx, uint32_t u32Flag)
{

    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_FIR_FLAG_CLR(u32Flag));

    WRITE_REG32(FMACx->FIR_STA, u32Flag);
}

/**
 * @brief  Set FMAC FIR limit value of calculate result.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] f32Max              Maximum value.
 * @param  [in] f32Min              Minimum value.
 * @retval None.
 */
void FMAC_FIR_SetLimit(CM_FMAC_TypeDef *FMACx, float32_t f32Max, float32_t f32Min)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(f32Min <= f32Max);

    uint32_t *pu32Max = (uint32_t *)(uint32_t)&f32Max;
    uint32_t *pu32Min = (uint32_t *)(uint32_t)&f32Min;

    WRITE_REG32(FMACx->FIR_YMAX, *pu32Max);
    WRITE_REG32(FMACx->FIR_YMIN, *pu32Min);
}

/**
 * @brief  FMAC read parameters.
 * @param  [in] FMACx                   Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                     FMAC instance register base
 * @param  [in] pstcFmacReadParam       FMAC read parameters function structure.
 *   @arg  See the structure definition for @ref stc_fmac_read_param_t
 * @retval None.
 */
void FMAC_FIR_ReadParam(CM_FMAC_TypeDef *FMACx, stc_fmac_read_param_t *pstcFmacReadParam)
{
    if (NULL == pstcFmacReadParam) {
        return;
    } else {
        DDL_ASSERT(IS_FMAC_UNIT(FMACx));
        DDL_ASSERT((0UL != pstcFmacReadParam->u16Size) && (pstcFmacReadParam->u16Size <= (FMAC_FIR_MAX_ORDER + 1UL)));
        DDL_ASSERT(NULL != pstcFmacReadParam->pf32Data);
        DDL_ASSERT(IS_FMAC_FIR_READ_TYPE(pstcFmacReadParam->u8ReadType));

        if (FMAC_READ_TYPE_X == pstcFmacReadParam->u8ReadType) {
            MODIFY_REG32(FMACx->FIR_SRADDR, FMAC_FIR_SRADDR_FIR_SRADDRX, (pstcFmacReadParam->u32RamAddr & FMAC_FIR_SRADDR_FIR_SRADDRX));
            FAMC_Read_Data(pstcFmacReadParam->pf32Data, &FMACx->FIR_SRDATAX, pstcFmacReadParam->u16Size);
        } else {
            MODIFY_REG32(FMACx->FIR_SRADDR, FMAC_FIR_SRADDR_FIR_SRADDRC,
                         ((pstcFmacReadParam->u32RamAddr << FMAC_FIR_SRADDR_FIR_SRADDRC_POS)  & FMAC_FIR_SRADDR_FIR_SRADDRC));
            FAMC_Read_Data(pstcFmacReadParam->pf32Data, &FMACx->FIR_SRDATAC, pstcFmacReadParam->u16Size);
        }
    }
}

/**
 * @brief  FMAC FIR read caches coefficient/inputdata number.
 * @param  [in] FMACx                   Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                     FMAC instance register base
 * @param  [in] u8ReadType              Type of read parameters @ref FMAC_READ_PARAM_TYPE.
 * @param  [out] pu16Data               Pointer the address for save coefficient/inputdata number.
 * @retval None.
 */
void FMAC_FIR_ReadCacheNum(CM_FMAC_TypeDef *FMACx, uint8_t u8ReadType, uint16_t *pu16Data)
{
    if (NULL == pu16Data) {
        return;
    } else {
        DDL_ASSERT(IS_FMAC_UNIT(FMACx));
        DDL_ASSERT(IS_FMAC_FIR_READ_TYPE(u8ReadType));

        if (FMAC_READ_TYPE_X == u8ReadType) {
            *pu16Data = (uint16_t)(READ_REG32(FMACx->FIR_DATA_NUM) & FMAC_FIR_DATA_NUM_RAM0_DATA_NUM);
        } else {
            *pu16Data = (uint16_t)((READ_REG32(FMACx->FIR_DATA_NUM) & FMAC_FIR_DATA_NUM_RAM1_DATA_NUM) >> FMAC_FIR_DATA_NUM_RAM1_DATA_NUM_POS);
        }
    }
}

/**
 * @brief  FMAC IIR initialization structure initialization.
 * @param  [in] pstcFmacIirInit      FMAC IIR function structure.
 *   @arg  See the structure definition for @ref stc_fmac_iir_init_t
 * @retval int32_t:
 *           - LL_OK: Success.
 *           - LL_ERR_INVD_PARAM: Parameter error.
 */
int32_t FMAC_IIR_StructInit(stc_fmac_iir_init_t *pstcFmacIirInit)
{
    int32_t i32Ret = LL_ERR_INVD_PARAM;

    if (pstcFmacIirInit != NULL) {
        pstcFmacIirInit->stcIirInit.u32Src          = FAMC_IIR_SRC_REG;
        pstcFmacIirInit->stcIirInit.u32Order        = 1UL;
        pstcFmacIirInit->stcIirInit.pf32Factor      = NULL;
        pstcFmacIirInit->stcIirInit.u32OutTimeSelect = FMAC_DATA_OUT_WAIT;
        pstcFmacIirInit->stcIirInit.u32LimitAmpCmd  = FMAC_AMP_LIMIT_DISABLE;
        pstcFmacIirInit->stcIirInit.u32IntCmd       = FMAC_INT_ENABLE;
        pstcFmacIirInit->u32channel                 = FMAC_IIR_CH1;
        pstcFmacIirInit->pf32YFactor                = NULL;
        i32Ret = LL_OK;
    }
    return i32Ret;
}

/**
 * @brief  FMAC IIR function initialize.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] pstcFmacIirInit     FMAC IIR function base parameter structure.
 *   @arg  See the structure definition for @ref stc_fmac_iir_init_t
 * @retval int32_t:
 *           - LL_OK: Success.
 *           - LL_ERR_INVD_PARAM: Parameter error.
 */
int32_t FMAC_IIR_Init(CM_FMAC_TypeDef *FMACx, const stc_fmac_iir_init_t *pstcFmacIirInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Cr1, u32Cr2, u32IntCr, u32DataCr, u32Param;

    if (NULL == pstcFmacIirInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_FMAC_UNIT(FMACx));
        DDL_ASSERT(IS_FMAC_IIR_ORDER(pstcFmacIirInit->stcIirInit.u32Order));
        DDL_ASSERT(IS_FMAC_IIR_SRC(pstcFmacIirInit->stcIirInit.u32Src));
        DDL_ASSERT(IS_FMAC_OUT_TIME(pstcFmacIirInit->stcIirInit.u32OutTimeSelect));
        DDL_ASSERT(IS_FMAC_AMP_LIMIT_STAT(pstcFmacIirInit->stcIirInit.u32LimitAmpCmd));
        DDL_ASSERT(IS_FMAC_INT_STAT(pstcFmacIirInit->stcIirInit.u32IntCmd));
        DDL_ASSERT(IS_FMAC_IIR_CH(pstcFmacIirInit->u32channel));
        DDL_ASSERT(NULL != pstcFmacIirInit->stcIirInit.pf32Factor);
        DDL_ASSERT(NULL != pstcFmacIirInit->pf32YFactor);

        u32Cr1 = (FMAC_IIR_CR1_CH1_DATA_TYPE << pstcFmacIirInit->u32channel);
        u32Cr2 = ((pstcFmacIirInit->stcIirInit.u32OutTimeSelect << FMAC_IIR_CR2_CH1_UNRD_PRO_POS) << pstcFmacIirInit->u32channel);
        if (pstcFmacIirInit->u32channel < FMAC_IIR_LP_MAX_CH) {
            u32Cr2 |= ((pstcFmacIirInit->stcIirInit.u32LimitAmpCmd << FMAC_IIR_CR2_CH1_TH_EN_POS) << pstcFmacIirInit->u32channel);
        }
        u32IntCr = ((pstcFmacIirInit->stcIirInit.u32IntCmd << FMAC_IIR_INT_CR_CH1_DONE_EN_POS) << pstcFmacIirInit->u32channel);
        u32Param = (((pstcFmacIirInit->stcIirInit.u32Order * 2U + 1U) << FMAC_IIR_CH1_PARM_C_NUM_POS) | \
                    (pstcFmacIirInit->stcIirInit.u32Order << FMAC_IIR_CH1_PARM_Y_NUM_POS) | \
                    (pstcFmacIirInit->stcIirInit.u32Order + 1U));
        u32DataCr = pstcFmacIirInit->stcIirInit.u32Src;

        SET_REG32_BIT(FMACx->IIR_CR1, u32Cr1);
        SET_REG32_BIT(FMACx->IIR_CR2, u32Cr2);
        SET_REG32_BIT(FMACx->IIR_INT_CR, u32IntCr);
        WRITE_REG32(*IIR_PARM_REG(pstcFmacIirInit->u32channel), u32Param);
        WRITE_REG32(*IIR_DATA_CR_REG(pstcFmacIirInit->u32channel), u32DataCr);

        /* Must priority configure X coefficient */
        FAMC_Write_Data(IIR_CW_REG(pstcFmacIirInit->u32channel), pstcFmacIirInit->stcIirInit.pf32Factor, pstcFmacIirInit->stcIirInit.u32Order + 1U);
        FAMC_Write_Data(IIR_CW_REG(pstcFmacIirInit->u32channel), pstcFmacIirInit->pf32YFactor, pstcFmacIirInit->stcIirInit.u32Order);

    }

    return i32Ret;
}

/**
 * @brief  Set IIR calculate mode.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] u32CalcMode         Calculate Mode @ref FMAC_CALC_MODE.
 * @retval None.
 */
void FMAC_IIR_CalcMode(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32CalcMode)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_CALC_MODE(u32CalcMode));

    MODIFY_REG32(FMACx->IIR_CR1, FMAC_IIR_CR1_CH1_CAL_MODE << u8Ch, (u32CalcMode << u8Ch) << FMAC_IIR_CR1_CH1_CAL_MODE_POS);
}

/**
 * @brief  Set IIR function enable or disable.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None.
 */
void FMAC_IIR_ChCmd(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(FMACx->IIR_CR1, (FMAC_IIR_CR1_CH1_ENABLE << u8Ch));
    } else {
        CLR_REG32_BIT(FMACx->IIR_CR1, (FMAC_IIR_CR1_CH1_ENABLE << u8Ch));
    }
}

/**
 * @brief  Set IIR interrupt enable or disable.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] enNewState          An @ref en_functional_state_t enumeration value.
 * @retval None.
 */
void FMAC_IIR_IntCmd(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(FMACx->IIR_INT_CR, (FMAC_IIR_INT_CR_CH1_DONE_EN << u8Ch));
    } else {
        CLR_REG32_BIT(FMACx->IIR_INT_CR, (FMAC_IIR_INT_CR_CH1_DONE_EN << u8Ch));
    }
}

/**
 * @brief  Soft start IIR calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @retval None.
 */
void FMAC_IIR_SWStart(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));

    SET_REG32_BIT(FMACx->IIR_CR1, (FMAC_IIR_CR1_CH1_START << u8Ch));
}

/**
 * @brief  Reset IIR.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @retval None.
 */
void FMAC_IIR_Reset(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));

    SET_REG32_BIT(FMACx->IIR_CR2, (FMAC_IIR_CR2_CH1_RST << u8Ch));
}

/**
 * @brief  Clear IIR input & output data.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @retval None.
 */
void FMAC_IIR_DataClear(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));

    SET_REG32_BIT(FMACx->IIR_CR2, (FMAC_IIR_CR2_CH1_DCLR << u8Ch));
}

/**
 * @brief  Get IIR Status flag.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] u32Flag             IIR state flag. @ref FMAC_IIR_FLAG.
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t FMAC_IIR_GetStatus(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32Flag)
{
    uint32_t u32StateBit;
    uint32_t u32BitShift;
    en_flag_status_t enFlag = RESET;

    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(IS_FMAC_IIR_FLAG(u32Flag));

    u32BitShift = 0;
    u32StateBit = (FMAC_IIR_FLAG_STAT1_ALL & u32Flag) / FMAC_IIR_FLAG_ERR;
    while (0UL != u32StateBit) {
        if (0UL != (u32StateBit & 0x01UL)) {
            if (0UL != READ_REG32_BIT(FMACx->IIR_STA1, ((1UL << u32BitShift) << u8Ch))) {
                enFlag = SET;
            }
        }
        u32BitShift += FMAC_IIR_STAT_BIT_OFFSET;
        u32StateBit >>= 1U;
    }

    u32BitShift = 0;
    u32StateBit = (FMAC_IIR_FLAG_STAT2_ALL & u32Flag) / FMAC_IIR_FLAG_INFINITE;
    while (0UL != u32StateBit) {
        if (0UL != (u32StateBit & 0x01UL)) {
            if (0UL != READ_REG32_BIT(FMACx->IIR_STA2, ((1UL << u32BitShift) << u8Ch))) {
                enFlag = SET;
            }
        }
        u32BitShift += FMAC_IIR_STAT_BIT_OFFSET;
        u32StateBit >>= 1U;
    }

    u32BitShift = 0;
    u32StateBit = (FMAC_IIR_FLAG_STAT_ALL & u32Flag) / FMAC_IIR_FLAG_XRAM_FULL;
    while (0UL != u32StateBit) {
        if (0UL != (u32StateBit & 0x01UL)) {
            if (0UL != READ_REG32_BIT(FMACx->IIR_BUF_STA, ((1UL << u32BitShift) << u8Ch))) {
                enFlag = SET;
            }
        }
        u32BitShift += FMAC_IIR_STAT_BIT_OFFSET;
        u32StateBit >>= 1U;
    }

    return enFlag;
}

/**
 * @brief  Clear IIR Status flag.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL
 * @param  [in] u32Flag             IIR state flag. @ref FMAC_IIR_FLAG.
 * @retval None.
 */
void FMAC_IIR_ClearStatus(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32Flag)
{
    uint32_t u32StateBit;
    uint32_t u32BitShift;

    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(IS_FMAC_IIR_FLAG_CLR(u32Flag));

    u32BitShift = 0;
    u32StateBit = (FMAC_IIR_FLAG_STAT1_ALL & u32Flag) / FMAC_IIR_FLAG_ERR;
    while (0UL != u32StateBit) {
        if (0UL != (u32StateBit & 0x01UL)) {
            WRITE_REG32(FMACx->IIR_STA1, ((1UL << u32BitShift) << u8Ch));
        }
        u32BitShift += FMAC_IIR_STAT_BIT_OFFSET;
        u32StateBit >>= 1U;
    }

    u32BitShift = 0;
    u32StateBit = (FMAC_IIR_FLAG_STAT2_ALL & u32Flag) / FMAC_IIR_FLAG_INFINITE;
    while (0UL != u32StateBit) {
        if (0UL != (u32StateBit & 0x01UL)) {
            WRITE_REG32(FMACx->IIR_STA2, ((1UL << u32BitShift) << u8Ch));
        }
        u32BitShift += FMAC_IIR_STAT_BIT_OFFSET;
        u32StateBit >>= 1U;
    }
}

/**
 * @brief  Read IIR calculate result.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [out] pf32Data           Pointer to the address for store data.
 * @retval None.
 */
void FMAC_IIR_GetResult(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(NULL != pf32Data);
    uint32_t *pu32Data = (uint32_t *)(uint32_t)pf32Data;

    *pu32Data = READ_REG32(*IIR_YR_REG(u8Ch));
}

/**
 * @brief  Write input data to IIR calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] pf32Data            Pointer to the address of write data.
 * @param  [in] u16Size             Size of data.
 * @retval None.
 */
void FMAC_IIR_X_WriteData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data, uint16_t u16Size)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(NULL != pf32Data);
    DDL_ASSERT(u16Size <= (FMAC_IIR_MAX_ORDER + 1UL));

    FAMC_Write_Data(IIR_XW_REG(u8Ch), pf32Data, u16Size);
}

/**
 * @brief  Write one input data to IIR calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] f32Data             Write data.
 * @retval None.
 */
void FMAC_IIR_X_WriteOneData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t f32Data)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));

    uint32_t *pu32Data = (uint32_t *)(uint32_t)&f32Data;

    WRITE_REG32(*IIR_XW_REG(u8Ch), *pu32Data);
}

/**
 * @brief  Write out data to IIR calculate.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL
 * @param  [in] pf32Data            Pointer to the address of write data.
 * @param  [in] u16Size             Size of data.
 * @retval None.
 */
void FMAC_IIR_Y_WriteData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data, uint16_t u16Size)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_CH(u8Ch));
    DDL_ASSERT(NULL != pf32Data);
    DDL_ASSERT(u16Size <= (FMAC_IIR_MAX_ORDER + 1UL));

    FAMC_Write_Data(IIR_YW_REG(u8Ch), pf32Data, u16Size);
}

/**
 * @brief  Set FMAC IIR limit value of calculate result.
 * @param  [in] FMACx               Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                 FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] f32Max              Maximum value.
 * @param  [in] f32Min              Minimum value.
 * @retval None.
 */
void FMAC_IIR_SetLimit(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t f32Max, float32_t f32Min)
{
    DDL_ASSERT(IS_FMAC_UNIT(FMACx));
    DDL_ASSERT(IS_FMAC_IIR_LP_CH(u8Ch));
    DDL_ASSERT(f32Min <= f32Max);

    uint32_t *pu32Max = (uint32_t *)(uint32_t)&f32Max;
    uint32_t *pu32Min = (uint32_t *)(uint32_t)&f32Min;

    if (FMAC_IIR_CH1 == u8Ch) {
        WRITE_REG32(FMACx->IIR_YMAX1, *pu32Max);
        WRITE_REG32(FMACx->IIR_YMIN1, *pu32Min);
    } else {
        WRITE_REG32(FMACx->IIR_YMAX2, *pu32Max);
        WRITE_REG32(FMACx->IIR_YMIN2, *pu32Min);
    }

}

/**
 * @brief  FMAC IIR read parameters.
 * @param  [in] FMACx                   Pointer to FMAC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC:                     FMAC instance register base
 * @param  [in] u8Ch                IIR channel @ref FMAC_IIR_CH_SEL.
 * @param  [in] pstcFmacReadParam       FMAC read parameters function structure.
 *   @arg  See the structure definition for @ref stc_fmac_read_param_t.
 * @retval None.
 */
void FMAC_IIR_ReadParam(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, stc_fmac_read_param_t *pstcFmacReadParam)
{
    if (NULL == pstcFmacReadParam) {
        return;
    } else {
        DDL_ASSERT(IS_FMAC_UNIT(FMACx));
        DDL_ASSERT((0UL != pstcFmacReadParam->u16Size) && (pstcFmacReadParam->u16Size <= (FMAC_IIR_MAX_ORDER + 1UL)));
        DDL_ASSERT(NULL != pstcFmacReadParam->pf32Data);
        DDL_ASSERT(IS_FMAC_IIR_READ_TYPE(pstcFmacReadParam->u8ReadType));

        if (FMAC_READ_TYPE_X == pstcFmacReadParam->u8ReadType) {
            MODIFY_REG32(*IIR_SR_REG(u8Ch), FMAC_IIR_SRADDR1_IIR_SRADDRX, (pstcFmacReadParam->u32RamAddr & FMAC_IIR_SRADDR1_IIR_SRADDRX));
            FAMC_Read_Data(pstcFmacReadParam->pf32Data, IIR_SR_XREG(u8Ch), pstcFmacReadParam->u16Size);
        } else if (FMAC_READ_TYPE_Y == pstcFmacReadParam->u8ReadType) {
            MODIFY_REG32(*IIR_SR_REG(u8Ch), FMAC_IIR_SRADDR1_IIR_SRADDRY,
                         ((pstcFmacReadParam->u32RamAddr << FMAC_IIR_SRADDR1_IIR_SRADDRY_POS)  & FMAC_IIR_SRADDR1_IIR_SRADDRY));
            FAMC_Read_Data(pstcFmacReadParam->pf32Data, IIR_SR_YREG(u8Ch), pstcFmacReadParam->u16Size);
        } else {
            MODIFY_REG32(*IIR_SR_REG(u8Ch), FMAC_IIR_SRADDR1_IIR_SRADDRC,
                         ((pstcFmacReadParam->u32RamAddr << FMAC_IIR_SRADDR1_IIR_SRADDRC_POS)  & FMAC_IIR_SRADDR1_IIR_SRADDRC));
            FAMC_Read_Data(pstcFmacReadParam->pf32Data, IIR_SR_CREG(u8Ch), pstcFmacReadParam->u16Size);
        }
    }
}

/**
 * @brief  Specifies FMAC ECC mode.
 * @param  [in] FMAC_ECCx               Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam              The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @param  [in]  u32EccMode             The ECC mode.
 *                                      This parameter can be a value of @ref FMAC_ECC_Mode
 * @retval None
 */
void FMAC_SetEccMode(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32EccMode)
{
    __IO uint32_t *ckcr;

    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));
    DDL_ASSERT(IS_FMAC_RAM_ECC_MD(u32EccMode));

    ckcr = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_CKCR + u32EccRam * RAM_ECC_REG_OFFSET);
    WRITE_REG32(*ckcr, u32EccMode);
}

/**
 * @brief  Get the status of the specified flag of FMAC RAM.
 * @param  [in] FMAC_ECCx               Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam              The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @param  [in]  u32Flag                The flag of RAM.
 *                                      This parameter can be a value of @ref FMAC_ECC_Err_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t FMAC_GetEccStatus(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32Flag)
{
    __IO uint32_t *cksr;
    en_flag_status_t enStatus = RESET;

    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));
    DDL_ASSERT(IS_FMAC_ECC_FLAG(u32Flag));

    cksr = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_CKSR + u32EccRam * RAM_ECC_REG_OFFSET);
    if (0UL != ((*cksr) & u32Flag)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  Clear the status of the specified flag of FMAC RAM.
 * @param  [in] FMAC_ECCx               Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam             The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @param  [in]  u32Flag                The flag of RAM.
 *                                      This parameter can be values of @ref FMAC_ECC_Err_Status_Flag
 * @retval None
 */
void FMAC_ClearStatus(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32Flag)
{
    __IO uint32_t *cksr;
    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));
    DDL_ASSERT(IS_FMAC_ECC_FLAG(u32Flag));

    cksr = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_CKSR + u32EccRam * RAM_ECC_REG_OFFSET);
    WRITE_REG32(*cksr, u32Flag);
}

/**
 * @brief  Enable or disable error injection.
 * @param  [in] FMAC_ECCx               Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam             The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void FMAC_EccErrorInjectCmd(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, en_functional_state_t enNewState)
{
    __IO uint32_t *eien;

    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    eien = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_EIEN + u32EccRam * RAM_ECC_REG_OFFSET);
    WRITE_REG32(*eien, enNewState);
}

/**
 * @brief  Enable or disable error injection bit of FMAC_ECC_RAM.
 * @param  [in] FMAC_ECCx               Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam             The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @param  [in]  u64BitSel              Bit selection.  Only bit0~bit38 valid.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void FMAC_ErrorInjectBitCmd(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint64_t u64BitSel, en_functional_state_t enNewState)
{
    __IO uint32_t *eibit0;
    __IO uint32_t *eibit1;

    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));
    DDL_ASSERT(u64BitSel <= RAM_EI_BIT_MASK);
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    eibit0 = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_EIBIT0 + u32EccRam * RAM_ECC_REG_OFFSET);
    eibit1 = (__IO uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_EIBIT1 + u32EccRam * RAM_ECC_REG_OFFSET);

    if (ENABLE == enNewState) {
        SET_REG32_BIT(*eibit0, (uint32_t)u64BitSel & FMAC_ECC_RAM0_EIBIT0);
        SET_REG32_BIT(*eibit1, (uint32_t)(u64BitSel >> 32U) & FMAC_ECC_RAM0_EIBIT1_EIBIT);
    } else {
        CLR_REG32_BIT(*eibit0, (uint32_t)u64BitSel);
        CLR_REG32_BIT(*eibit1, (uint32_t)(u64BitSel >> 32U));
    }
}

/**
 * @brief  Get access address when 1bit or 2bit ECC error occurs in RAM_ECC_RAM.
 * @param  [in] FMAC_ECCx                       Pointer to FMAC ECC instance register base.
 * This parameter can be a value of the following:
 *   @arg  CM_FMAC_ECC:                 FMAC ECC instance register base
 * @param  [in]  u32EccRam             The ECC RAM.
 *                                      This parameter can be any combination of @ref FMAC_ECC_RAM
 * @retval Error address.
 */
uint32_t FMAC_GetEccErrorAddr(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam)
{
    uint32_t u32RetAddr;
    __I uint32_t *errAddr;

    DDL_ASSERT(IS_FMAC_ECC_UNIT(FMAC_ECCx));
    DDL_ASSERT(IS_FMAC_RAM_ECC_RAM(u32EccRam));

    errAddr = (__I uint32_t *)((uint32_t)&FMAC_ECCx->RAM0_ECCERRADDR + u32EccRam * RAM_ECC_REG_OFFSET);
    u32RetAddr = READ_REG32(*errAddr);

    return u32RetAddr;
}
/**
 * @}
 */

#endif /* LL_FMAC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

