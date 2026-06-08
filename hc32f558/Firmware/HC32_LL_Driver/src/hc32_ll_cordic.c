/**
 *******************************************************************************
 * @file  hc32_ll_cordic.c
 * @brief This file provides firmware functions manage the CORDIC.
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
#include "hc32_ll_cordic.h"
#include "hc32_ll_utility.h"

/**
 * @defgroup LL_Driver LL Driver
 * @{
 */

/**
 * @defgroup LL_CORDIC CORDIC
 * @brief Coordinate Rotation Digital Computing Unit Driver Library
 * @{
 */
#if (LL_CORDIC_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup CORDIC_Local_Macros CORDIC Local Macros
 * @{
 */

/**
 * @defgroup CORDIC_Check_Parameters_Validity CORDIC Check Parameters Validity
 * @{
 */
/* Parameter valid check for CORDIC function */
#define IS_CORDIC_FUNC(x)                                                      \
(   ((x) == CORDIC_FUNC_COS)                      ||                           \
    ((x) == CORDIC_FUNC_SIN)                      ||                           \
    ((x) == CORDIC_FUNC_PHASE)                    ||                           \
    ((x) == CORDIC_FUNC_MOD)                      ||                           \
    ((x) == CORDIC_FUNC_ATAN)                     ||                           \
    ((x) == CORDIC_FUNC_COSH)                     ||                           \
    ((x) == CORDIC_FUNC_SINH)                     ||                           \
    ((x) == CORDIC_FUNC_ATANH)                    ||                           \
    ((x) == CORDIC_FUNC_LN)                       ||                           \
    ((x) == CORDIC_FUNC_SQRT))

/* Parameter valid check for CORDIC read interface */
#define IS_CORDIC_READ_IF(x)                                                   \
(   ((x) == CORDIC_READ_IF_AHB)                   ||                           \
    ((x) == CORDIC_READ_IF_EXT))

/* Parameter valid check for CORDIC data type */
#define IS_CORDIC_DATA_TYPE(x)                                                 \
(   ((x) == CORDIC_DATA_TYPE_FLOAT)               ||                           \
    ((x) == CORDIC_DATA_TYPE_Q))

/* Parameter valid check for CORDIC write data number */
#define IS_CORDIC_WRITE_NUM(x)                                                 \
(   ((x) == CORDIC_WRITE_NUM_1)                   ||                           \
    ((x) == CORDIC_WRITE_NUM_2))

/* Parameter valid check for CORDIC read data number */
#define IS_CORDIC_READ_NUM(x)                                                  \
(   ((x) == CORDIC_READ_NUM_1)                    ||                           \
    ((x) == CORDIC_READ_NUM_2))

/* Parameter valid check for CORDIC input data size */
#define IS_CORDIC_IN_SIZE(x)                                                   \
(   ((x) == CORDIC_IN_SIZE_16BIT)                 ||                           \
    ((x) == CORDIC_IN_SIZE_32BIT))

/* Parameter valid check for CORDIC result data size  */
#define IS_CORDIC_OUT_SIZE(x)                                                  \
(   ((x) == CORDIC_OUT_SIZE_16BIT)                ||                           \
    ((x) == CORDIC_OUT_SIZE_32BIT))

/* Parameter valid check for CORDIC DMA type  */
#define IS_CORDIC_DMA_TYPE(x)                                                  \
(   ((x) != 0U) && (((x) & (CORDIC_DMA_WRITE | CORDIC_DMA_READ)) != 0U))

/* Parameter valid check for CORDIC precision  */
#define IS_CORDIC_PRECISION(x)          (((x) >= CORDIC_PRECISION_CYCLE1) && ((x) <= CORDIC_PRECISION_CYCLE15))

/* Parameter valid check for CORDIC scale  */
#define IS_CORDIC_SCALE(x)              ((x) <= CORDIC_SCALE_MAX)

/* Parameter valid check for CORDIC interrupt */
#define IS_CORDIC_INT(x)                                                       \
(   ((x) == CORDIC_INT_ARG1_OVF)            ||                                 \
    ((x) == CORDIC_INT_RES1_OVF)            ||                                 \
    ((x) == CORDIC_INT_RES1_UDF)            ||                                 \
    ((x) == CORDIC_INT_RES2_OVF)            ||                                 \
    ((x) == CORDIC_INT_RES2_UDF)            ||                                 \
    ((x) == CORDIC_INT_WRITE_DATA_LOSS))

/* Parameter valid check for CORDIC interrupt flag */
#define IS_CORDIC_FLAG(x)                                                      \
(   ((x) == CORDIC_FLAG_ARG1_OVF)           ||                                 \
    ((x) == CORDIC_FLAG_RES1_OVF)           ||                                 \
    ((x) == CORDIC_FLAG_RES1_UDF)           ||                                 \
    ((x) == CORDIC_FLAG_RES2_OVF)           ||                                 \
    ((x) == CORDIC_FLAG_RES2_UDF)           ||                                 \
    ((x) == CORDIC_FLAG_WRITE_DATA_LOSS))

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
 * @defgroup CORDIC_Global_Functions CORDIC Global Functions
 * @{
 */
/**
 * @brief  Init CORDIC initial structure with default value.
 * @param  [in] pstcInit                Specifies the parameter of CORDIC initial structure.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t CORDIC_StructInit(stc_cordic_init_t *pstcInit)
{
    int32_t i32Ret = LL_OK;

    /* Check if pointer is NULL */
    if (NULL == pstcInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        pstcInit->u32IntEn     = (uint32_t)DISABLE;
        pstcInit->u32Func      = CORDIC_FUNC_COS;
        pstcInit->u32Precision = CORDIC_PRECISION_CYCLE5;
        pstcInit->u32Scale     = CORDIC_SCALE_0;
        pstcInit->u32ReadIF    = CORDIC_READ_IF_AHB;
        pstcInit->u32DataType  = CORDIC_DATA_TYPE_Q;
        pstcInit->u32WriteNum  = CORDIC_WRITE_NUM_1;
        pstcInit->u32ReadNum   = CORDIC_READ_NUM_1;
        pstcInit->u32InSize    = CORDIC_IN_SIZE_32BIT;
        pstcInit->u32OutSize   = CORDIC_OUT_SIZE_32BIT;
    }

    return i32Ret;
}

/**
 * @brief  CORDIC initialize
 * @param  [in] pstcInit specifies the CORDIC initial config.
 * @retval int32_t:
 *         - LL_OK: Initialize success
 *         - LL_ERR_INVD_PARAM: NULL pointer
 */
int32_t CORDIC_Init(stc_cordic_init_t *pstcInit)
{
    int32_t i32Ret = LL_OK;
    uint32_t u32Value;

    /* Check if pointer is NULL */
    if (NULL == pstcInit) {
        i32Ret = LL_ERR_INVD_PARAM;
    } else {
        DDL_ASSERT(IS_CORDIC_FUNC(pstcInit->u32Func));
        DDL_ASSERT(IS_CORDIC_READ_IF(pstcInit->u32ReadIF));
        DDL_ASSERT(IS_CORDIC_DATA_TYPE(pstcInit->u32DataType));
        DDL_ASSERT(IS_CORDIC_WRITE_NUM(pstcInit->u32WriteNum));
        DDL_ASSERT(IS_CORDIC_READ_NUM(pstcInit->u32ReadNum));
        DDL_ASSERT(IS_CORDIC_IN_SIZE(pstcInit->u32InSize));
        DDL_ASSERT(IS_CORDIC_OUT_SIZE(pstcInit->u32OutSize));
        DDL_ASSERT(IS_CORDIC_PRECISION(pstcInit->u32Precision));
        DDL_ASSERT(IS_CORDIC_SCALE(pstcInit->u32Scale));
        DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcInit->u32IntEn));

        u32Value = ((pstcInit->u32Func | pstcInit->u32ReadIF | pstcInit->u32DataType | pstcInit->u32WriteNum |      \
                     pstcInit->u32ReadNum | pstcInit->u32InSize | pstcInit->u32OutSize) | \
                    (pstcInit->u32Precision << CORDIC_CSR_PRECISION_POS) | \
                    (pstcInit->u32Scale << CORDIC_CSR_SCALE_POS) | \
                    ((uint32_t)pstcInit->u32IntEn << CORDIC_CSR_IEN_POS));

        WRITE_REG32(CM_CORDIC->CSR, u32Value);
    }

    return i32Ret;
}

/**
 * @brief  Set the CORDIC function.
 * @param  u32Func                      Specifies the CORDIC function source. @ref CORDIC_FUNC_Sel
 * @retval None.
 */
void CORDIC_SetFunc(uint32_t u32Func)
{
    DDL_ASSERT(IS_CORDIC_FUNC(u32Func));

    MODIFY_REG32(CM_CORDIC->CSR, CORDIC_CSR_FUNC, u32Func);
}

/**
 * @brief  Enable or Disable CORDIC DMA.
 * @param  [in] u32DmaType              Specifies the CORDIC dma type. @ref CORDIC_Dma_Type
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void CORDIC_DmaCmd(uint32_t u32DmaType, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_CORDIC_DMA_TYPE(u32DmaType));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(CM_CORDIC->CSR, u32DmaType);
    } else {
        CLR_REG32_BIT(CM_CORDIC->CSR, u32DmaType);
    }
}

/**
 * @brief  Write CORDIC data.
 * @param  u32Data
 * @retval None.
 */
void CORDIC_WriteData(uint32_t u32Data)
{
    WRITE_REG32(CM_CORDIC->WDATA, u32Data);
}

/**
 * @brief  Get CORDIC result.
 * @param  None
 * @retval uint32_t  the result.
 */
uint32_t CORDIC_GetResult(void)
{
    return READ_REG32(CM_CORDIC->RDATA);
}

/**
 * @brief  Enable or Disable CORDIC interrupt.
 * @param  [in] u32Int                  Specifies the CORDIC interrupt source. @ref CORDIC_Int_Sel
 * @param  [in] enNewState              An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void CORDIC_IntCmd(uint32_t u32Int, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_CORDIC_INT(u32Int));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (ENABLE == enNewState) {
        SET_REG32_BIT(CM_CORDIC->INTEN, u32Int);
    } else {
        CLR_REG32_BIT(CM_CORDIC->INTEN, u32Int);
    }
}

/**
 * @brief  Get CORDIC interrupt status.
 * @param  [in] u32Flag                 Specifies the flag to be read. @ref CORDIC_Flag_Sel
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t CORDIC_GetStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_CORDIC_FLAG(u32Flag));

    return ((0x00U != READ_REG32_BIT(CM_CORDIC->ISR, u32Flag)) ? SET : RESET);
}

/**
 * @brief  Clear CORDIC interrupt status.
 * @param  [in] u32Flag                 Specifies the flag to be cleared. @ref CORDIC_Flag_Sel
 * @retval None
 */
void CORDIC_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_CORDIC_FLAG(u32Flag));

    WRITE_REG32(CM_CORDIC->ICR, u32Flag);
}

/**
 * @}
 */

#endif /* LL_CORDIC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

/******************************************************************************
 * EOF (not truncated)
 *****************************************************************************/
