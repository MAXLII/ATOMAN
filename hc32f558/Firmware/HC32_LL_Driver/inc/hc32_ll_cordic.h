/**
 *******************************************************************************
 * @file  hc32_ll_cordic.h
 * @brief This file contains all the functions prototypes of the CORDIC driver
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
#ifndef __HC32_LL_CORDIC_H__
#define __HC32_LL_CORDIC_H__

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
 * @addtogroup LL_CORDIC
 * @{
 */
#if (LL_CORDIC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup CORDIC_Global_Types CORDIC Global Types
 * @{
 */
typedef struct {
    uint32_t u32IntEn;                  /*!< Specifies the CORDIC interrupt function.
                                            This parameter can be a value of @ref en_functional_state_t   */
    uint32_t u32Func;                   /*!< Specifies the CORDIC function.
                                            This parameter can be a value of @ref CORDIC_FUNC_Sel         */
    uint32_t u32Precision;              /*!< Specifies the number of iterations.
                                            This parameter can be a value of 1~15, and means iterations/4 */
    uint32_t u32Scale;                  /*!< Specifies the scale.
                                            This parameter can be a value of 0~7, and means 2^(-n)        */
    uint32_t u32ReadIF;                 /*!< Specifies the read interface.
                                            This parameter can be a value of @ref CORDIC_Read_IF        */
    uint32_t u32DataType;               /*!< Specifies the data type.
                                            This parameter can be a value of @ref CORDIC_Data_Type        */
    uint32_t u32WriteNum;               /*!< Specifies the number of write data for one calculate.
                                            This parameter can be a value of @ref CORDIC_Write_Num        */
    uint32_t u32ReadNum;                /*!< Specifies the number of read data after one calculate.
                                            This parameter can be a value of @ref CORDIC_Read_Num         */
    uint32_t u32InSize;                 /*!< Specifies the width of input data.
                                            This parameter can be a value of @ref CORDIC_In_Size          */
    uint32_t u32OutSize;                /*!< Specifies the width of output data.
                                            This parameter can be a value of @ref CORDIC_Out_Size         */
} stc_cordic_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup CORDIC_Global_Macros CORDIC Global Macros
 * @{
 */

/**
 * @defgroup CORDIC_FUNC_Sel CORDIC function selection
 * @{
 */
#define CORDIC_FUNC_COS                 (0U)                /*!< cosine */
#define CORDIC_FUNC_SIN                 (1U)                /*!< sine */
#define CORDIC_FUNC_PHASE               (2U)                /*!< phase */
#define CORDIC_FUNC_MOD                 (3U)                /*!< modulus */
#define CORDIC_FUNC_ATAN                (4U)                /*!< arctangent */
#define CORDIC_FUNC_COSH                (5U)                /*!< hyperbolic cosine */
#define CORDIC_FUNC_SINH                (6U)                /*!< hyperbolic sine */
#define CORDIC_FUNC_ATANH               (7U)                /*!< hyperbolic arctangent */
#define CORDIC_FUNC_LN                  (8U)                /*!< nature logarithm */
#define CORDIC_FUNC_SQRT                (9U)                /*!< square root */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Read_IF CORDIC read interface
 * @{
 */
#define CORDIC_READ_IF_AHB              (0U)                /*!< the result read interface is from AHB */
#define CORDIC_READ_IF_EXT              (CORDIC_CSR_EXTMOD) /*!< the result read interface is from external module */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Data_Type CORDIC data type
 * @{
 */
#define CORDIC_DATA_TYPE_FLOAT          (CORDIC_CSR_FLOAT)
#define CORDIC_DATA_TYPE_Q              (0U)
/**
 * @}
 */

/**
 * @defgroup CORDIC_Dma_Type CORDIC data type
 * @{
 */
#define CORDIC_DMA_WRITE                (CORDIC_CSR_DMAWEN)
#define CORDIC_DMA_READ                 (CORDIC_CSR_DMAREN)
/**
 * @}
 */

/**
 * @defgroup CORDIC_Out_Size CORDIC output data size
 * @{
 */
#define CORDIC_OUT_SIZE_16BIT           (CORDIC_CSR_RESSIZE)
#define CORDIC_OUT_SIZE_32BIT           (0U)
/**
 * @}
 */

/**
 * @defgroup CORDIC_In_Size CORDIC input data size
 * @{
 */
#define CORDIC_IN_SIZE_16BIT            (CORDIC_CSR_ARGSIZE)
#define CORDIC_IN_SIZE_32BIT            (0U)
/**
 * @}
 */

/**
 * @defgroup CORDIC_Write_Num CORDIC number of write data
 * @{
 */
#define CORDIC_WRITE_NUM_1              (0U)                /*!< Calculate need 2 32bit data                 */
#define CORDIC_WRITE_NUM_2              (CORDIC_CSR_NARGS)  /*!< Calculate need 2 32bit data                 */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Read_Num CORDIC number of argument
 * @{
 */
#define CORDIC_READ_NUM_1               (0U)                /*!< Result is 1 32bit data or 2 16bit data, the RRDY will be cleared after read result register one time */
#define CORDIC_READ_NUM_2               (CORDIC_CSR_NRES)   /*!< Result if 2 32bit data, the RRDY will be cleared after read result register two times                */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Scale CORDIC scale
 * @{
 */
#define CORDIC_SCALE_0                  (0U)                /*!< Scale is 2^0 */
#define CORDIC_SCALE_1                  (1U)                /*!< Scale is 2^(-1) */
#define CORDIC_SCALE_2                  (2U)                /*!< Scale is 2^(-2) */
#define CORDIC_SCALE_3                  (3U)                /*!< Scale is 2^(-3) */
#define CORDIC_SCALE_4                  (4U)                /*!< Scale is 2^(-4) */
#define CORDIC_SCALE_5                  (5U)                /*!< Scale is 2^(-5) */
#define CORDIC_SCALE_6                  (6U)                /*!< Scale is 2^(-6) */
#define CORDIC_SCALE_7                  (7U)                /*!< Scale is 2^(-7) */
#define CORDIC_SCALE_MAX                (7U)                /*!< Max scale is 2^(-7) */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Precision CORDIC precision
 * @{
 */
#define CORDIC_PRECISION_CYCLE1         (1U)                /*!< 1*4 iteration */
#define CORDIC_PRECISION_CYCLE2         (2U)                /*!< 2*4 iterations */
#define CORDIC_PRECISION_CYCLE3         (3U)                /*!< 3*4 iterations */
#define CORDIC_PRECISION_CYCLE4         (4U)                /*!< 4*4 iterations */
#define CORDIC_PRECISION_CYCLE5         (5U)                /*!< 5*4 iterations */
#define CORDIC_PRECISION_CYCLE6         (6U)                /*!< 6*4 iterations */
#define CORDIC_PRECISION_CYCLE7         (7U)                /*!< 7*4 iterations */
#define CORDIC_PRECISION_CYCLE8         (8U)                /*!< 8*4 iterations */
#define CORDIC_PRECISION_CYCLE9         (9U)                /*!< 9*4 iterations */
#define CORDIC_PRECISION_CYCLE10        (10U)               /*!< 10*4 iterations */
#define CORDIC_PRECISION_CYCLE11        (11U)               /*!< 11*4 iterations */
#define CORDIC_PRECISION_CYCLE12        (12U)               /*!< 12*4 iterations */
#define CORDIC_PRECISION_CYCLE13        (13U)               /*!< 13*4 iterations */
#define CORDIC_PRECISION_CYCLE14        (14U)               /*!< 14*4 iterations */
#define CORDIC_PRECISION_CYCLE15        (15U)               /*!< 15*4 iterations */
#define CORDIC_PRECISION_MAX            (15U)               /*!< Max precision is 15*4 iterations */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Int_Sel CORDIC interrupt selection
 * @{
 */
#define CORDIC_INT_ARG1_OVF             (1UL << 0U)         /*!< arg 1 overflow */
#define CORDIC_INT_RES1_OVF             (1UL << 4U)         /*!< result1 overflow */
#define CORDIC_INT_RES1_UDF             (1UL << 5U)         /*!< result1 underflow */
#define CORDIC_INT_RES2_OVF             (1UL << 6U)         /*!< result2 overflow */
#define CORDIC_INT_RES2_UDF             (1UL << 7U)         /*!< result2 underflow */
#define CORDIC_INT_WRITE_DATA_LOSS      (1UL << 8U)         /*!< write data loss */
/**
 * @}
 */

/**
 * @defgroup CORDIC_Flag_Sel CORDIC flag selection
 * @{
 */
#define CORDIC_FLAG_ARG1_OVF            (1UL << 0U)         /*!< arg 1 overflow */
#define CORDIC_FLAG_RES1_OVF            (1UL << 4U)         /*!< result1 overflow */
#define CORDIC_FLAG_RES1_UDF            (1UL << 5U)         /*!< result1 underflow */
#define CORDIC_FLAG_RES2_OVF            (1UL << 6U)         /*!< result2 overflow */
#define CORDIC_FLAG_RES2_UDF            (1UL << 7U)         /*!< result2 underflow */
#define CORDIC_FLAG_WRITE_DATA_LOSS     (1UL << 8U)         /*!< write data loss */
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
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup CORDIC_Global_Functions
 * @{
 */
/**
 * @brief  Get CORDIC status, busy to calculate or not start calculate
 * @param  None
 * @retval en_flag_status_t
 *              -SET        : busy to calculate
 *              -RESET      : not start calculate
 */
__STATIC_INLINE en_flag_status_t CORDIC_IsBusy(void)
{
    return ((0x00U != READ_REG32_BIT(CM_CORDIC->CSR, CORDIC_CSR_BUSY)) ? SET : RESET);
}

/**
 * @brief  Get CORDIC status, ready to read result or not ready
 * @param  None
 * @retval en_flag_status_t
 *              -SET        : ready to read result
 *              -RESET      : not ready
 */
__STATIC_INLINE en_flag_status_t CORDIC_IsReady(void)
{
    return ((0x00U != READ_REG32_BIT(CM_CORDIC->CSR, CORDIC_CSR_RRDY)) ? SET : RESET);
}

/*******************************************************************************
 declaration
 ******************************************************************************/
int32_t CORDIC_StructInit(stc_cordic_init_t *pstcInit);
int32_t CORDIC_Init(stc_cordic_init_t *pstcInit);
void CORDIC_SetFunc(uint32_t u32Func);
void CORDIC_DmaCmd(uint32_t u32DmaType, en_functional_state_t enNewState);
void CORDIC_WriteData(uint32_t u32Data);
uint32_t CORDIC_GetResult(void);
void CORDIC_IntCmd(uint32_t u32Int, en_functional_state_t enNewState);
en_flag_status_t CORDIC_GetStatus(uint32_t u32Flag);
void CORDIC_ClearStatus(uint32_t u32Flag);

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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_DDL_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
