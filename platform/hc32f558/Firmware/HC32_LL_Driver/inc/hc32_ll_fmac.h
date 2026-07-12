/**
 *******************************************************************************
 * @file  hc32_ll_fmac.h
 * @brief This file contains all the functions prototypes of the FMAC driver
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
#ifndef __HC32_LL_FMAC_H__
#define __HC32_LL_FMAC_H__

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
 * @addtogroup LL_FMAC
 * @{
 */

#if (LL_FMAC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup FMAC_Global_Types FMAC Global Types
 * @{
 */
/**
 * @brief FMAC configuration structure
 */
typedef struct {
    uint32_t u32Src;                    /*!< FMAC fir filter input source select.
                                             This parameter can be a value of @ref FMAC_FIR_INPUT_SRC.*/
    uint32_t u32Order;                  /*!< The fir filter order. fir less than 256, iir less than 5 */

    float32_t *pf32Factor;              /*!< FMAC filter filter factor config. */

    uint32_t u32OutTimeSelect;          /*!< Select time of out after fir calculate completed.
                                             This parameter can be a value of @ref FMAC_DATA_OUT_TIME .*/
    uint32_t u32LimitAmpCmd;            /*!< Enable or disable FMAC fir filter amplitude limit.
                                             This parameter can be a value of @ref FMAC_AMP_LIMIT_STAT .*/
    uint32_t u32IntCmd;                 /*!< Enable or disable FMAC interrupt.
                                             This parameter can be a value of @ref FMAC_Interrupt_Selection.*/
} stc_fmac_init_t;

/**
 * @brief FMAC FIR configuration structure
 */
typedef struct {
    stc_fmac_init_t stcFirInit;         /*!< FIR initialize structure.
                                             This parameter details refer @ref stc_clark_init_t */
    uint32_t u32Fun;                    /*!< FIR filter function select.
                                             This parameter can be a value of @ref FMAC_FIR_FUNC_SEL .*/
} stc_fmac_fir_init_t;

/**
 * @brief FMAC IIR configuration structure
 */
typedef struct {
    stc_fmac_init_t stcIirInit;

    uint32_t u32channel;                /*!< IIR filter channel select.
                                             This parameter can be a value of @ref FMAC_IIR_CH_SEL .*/
    float32_t *pf32YFactor;             /*!< The pointer of IIR filter output factor. */
} stc_fmac_iir_init_t;

/**
 * @brief FMAC limit amplitude configuration structure
 */
typedef struct {
    void *pMax;                         /*!< The fir filter amplitude maximum value, The data format is fixed-point or float */
    void *pMin;                         /*!< The fir filter amplitude minimum value, The data format is fixed-point or float */
} stc_fmac_amplitude_t;

/**
 * @brief FMAC read parameters configuration structure
 */
typedef struct {
    uint8_t u8ReadType;                 /*!< Type of read parameters
                                             This parameter can be a value of @ref FMAC_READ_PARAM_TYPE .*/
    uint32_t u32RamAddr;                /*!< The initial address when read from RAM */
    float32_t *pf32Data;                /*!< Pointer to the address for save readback data */
    uint16_t u16Size;                   /*!< Size of read parameters */
} stc_fmac_read_param_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup FMAC_Global_Macros FMAC Global Macros
 * @{
 */

/* FMAC FIR Maximum Order */
#define FMAC_FIR_MAX_ORDER                      (255UL)
/* FMAC IIR Maximum Order */
#define FMAC_IIR_MAX_ORDER                      (4UL)

/**
 * @defgroup FMAC_FIR_INPUT_SRC FMAC FIR Input Source
 * @{
 */
#define FAMC_FIR_SRC_REG                (0UL << FMAC_FIR_CR_XSEL_POS)
#define FAMC_FIR_SRC_ADC1_CH2           (1UL << FMAC_FIR_CR_XSEL_POS)
#define FAMC_FIR_SRC_ADC2_CH5           (2UL << FMAC_FIR_CR_XSEL_POS)
#define FAMC_FIR_SRC_ADC3_CH5           (3UL << FMAC_FIR_CR_XSEL_POS)
/**
 * @}
 */

/**
 * @defgroup FMAC_IIR_INPUT_SRC FMAC IIR Input Source
 * @{
 */
#define FAMC_IIR_SRC_REG                (0UL)
#define FAMC_IIR_SRC_ADC1_CH0           (1UL)
#define FAMC_IIR_SRC_ADC1_CH1           (2UL)
#define FAMC_IIR_SRC_ADC1_CH2           (3UL)
#define FAMC_IIR_SRC_ADC1_CH3           (4UL)
#define FAMC_IIR_SRC_ADC1_CH4           (5UL)
#define FAMC_IIR_SRC_ADC2_CH0           (6UL)
#define FAMC_IIR_SRC_ADC2_CH1           (7UL)
#define FAMC_IIR_SRC_ADC2_CH2           (8UL)
#define FAMC_IIR_SRC_ADC2_CH3           (9UL)
#define FAMC_IIR_SRC_ADC2_CH4           (10UL)
#define FAMC_IIR_SRC_ADC3_CH0           (11UL)
#define FAMC_IIR_SRC_ADC3_CH1           (12UL)
#define FAMC_IIR_SRC_ADC3_CH2           (13UL)
#define FAMC_IIR_SRC_ADC3_CH3           (14UL)
#define FAMC_IIR_SRC_ADC3_CH4           (15UL)
#define FAMC_IIR_SRC_PID1_ERR           (16UL)
#define FAMC_IIR_SRC_PID2_ERR           (17UL)
#define FAMC_IIR_SRC_PID1_U             (18UL)
#define FAMC_IIR_SRC_PID2_U             (19UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_FIX_POINT_PREC Fixed-point Precision
 * @{
 */
#define FAMC_FIX_POINT_MAX_PREC         (31UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_CALC_MODE FMAC Calculate Mode
 * @{
 */
#define FMAC_CALC_MODE_SOFT             (0UL)
#define FMAC_CALC_MODE_HARD             (1UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_DATA_OUT_TIME FMAC Data Output Time
 * @{
 */
#define FMAC_DATA_OUT_IMMED             (0UL)
#define FMAC_DATA_OUT_WAIT              (1UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_AMP_LIMIT_STAT FMAC Amplitude Limit State
 * @{
 */
#define FMAC_AMP_LIMIT_DISABLE          (0UL)
#define FMAC_AMP_LIMIT_ENABLE           (1UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_Interrupt_Selection FMAC Interrupt Selection
 * @{
 */
#define FMAC_INT_ENABLE                 (1UL)
#define FMAC_INT_DISABLE                (0UL)
/**
 * @}
 */

/**
 * @defgroup FMAC_FIR_FUNC_SEL FMAC FIR Function Select
 * @{
 */
#define FMAC_FIR_FUNC_FIR                (0UL)
#define FMAC_FIR_FUNC_MAF                (FMAC_FIR_CR_FUNC)
/**
 * @}
 */

/**
 * @defgroup FMAC_IIR_CH_SEL FMAC IIR Channel Select
 * @{
 */
#define FMAC_IIR_CH1                    (0U)
#define FMAC_IIR_CH2                    (1U)
#define FMAC_IIR_CH3                    (2U)
#define FMAC_IIR_CH4                    (3U)
#define FMAC_IIR_CH5                    (4U)
#define FMAC_IIR_CH6                    (5U)
/**
 * @}
 */

/**
 * @defgroup FMAC_FIR_FLAG FMAC FIR Flag
 * @{
 */
#define FMAC_FIR_FLAG_OVER              (FMAC_FIR_STA_FIR_OOR)
#define FMAC_FIR_FLAG_INVALID           (FMAC_FIR_STA_FIR_NAN)
#define FMAC_FIR_FLAG_INFINITE          (FMAC_FIR_STA_FIR_INF)
#define FMAC_FIR_FLAG_CRAM_FULL         (FMAC_FIR_STA_FIR_RAM1_FULL)
#define FMAC_FIR_FLAG_XRAM_FULL         (FMAC_FIR_STA_FIR_RAM0_FULL)
#define FMAC_FIR_FLAG_IN_CLR            (FMAC_FIR_STA_FIR_CLR_STA)
#define FMAC_FIR_FLAG_BUSY              (FMAC_FIR_STA_FIR_RUN)
#define FMAC_FIR_FLAG_ERR               (FMAC_FIR_STA_FIR_START_ERR)
#define FMAC_FIR_FLAG_DATA_RDY          (FMAC_FIR_STA_FIR_DATA_RDY)
#define FMAC_FIR_FLAG_ALL               (0x70F03UL)
#define FMAC_FIR_FLAG_CLR_ALL           (FMAC_FIR_FLAG_OVER | FMAC_FIR_FLAG_INVALID | FMAC_FIR_FLAG_INFINITE | \
                                         FMAC_FIR_FLAG_ERR | FMAC_FIR_FLAG_DATA_RDY)
/**
 * @}
 */

/**
 * @defgroup FMAC_IIR_FLAG FMAC IIR Flag
 * @{
 */
#define FMAC_IIR_FLAG_ERR               (0x01U)
#define FMAC_IIR_FLAG_DATA_RDY          (0x02U)
#define FMAC_IIR_FLAG_INFINITE          (0x04U)
#define FMAC_IIR_FLAG_INVALID           (0x08U)
#define FMAC_IIR_FLAG_OVER              (0x10U)
#define FMAC_IIR_FLAG_XRAM_FULL         (0x20U)
#define FMAC_IIR_FLAG_YRAM_FULL         (0x40U)
#define FMAC_IIR_FLAG_CRAM_FULL         (0x80U)
#define FMAC_IIR_FLAG_STAT1_ALL         (0x03UL)
#define FMAC_IIR_FLAG_STAT2_ALL         (0x1CUL)
#define FMAC_IIR_FLAG_STAT_ALL          (0xE0UL)
#define FMAC_IIR_FLAG_ALL               (0xFFUL)
#define FMAC_IIR_FLAG_CLR_ALL           (FMAC_IIR_FLAG_ERR | FMAC_IIR_FLAG_INVALID | FMAC_IIR_FLAG_INFINITE | \
                                         FMAC_IIR_FLAG_OVER | FMAC_IIR_FLAG_DATA_RDY)
/**
 * @}
 */

/**
 * @defgroup FMAC_READ_PARAM_TYPE FMAC Read Parameters Type
 * @{
 */
#define FMAC_READ_TYPE_X                    (1U)
#define FMAC_READ_TYPE_C                    (2U)
#define FMAC_READ_TYPE_Y                    (3U)
/**
 * @}
 */

/**
 * @defgroup FMAC_ECC_RAM FMAC ECC RAM Definition
 * @{
 */
#define FMAC_ECC_RAM0                       (0U)
#define FMAC_ECC_RAM1                       (1U)
/**
 * @}
 */

/**
 * @defgroup FMAC_ECC_Mode FMAC ECC Mode
 * @note     XX_INVD: The ECC mode is invalid
 *           XX_MD1:  When 1-bit error occurs, ECC error corrects. No 1-bit-error status flag setting, no interrupt.
 *                    When 2-bit error occurs, ECC error detects. 2-bit-error status flag sets and interrupt.
 *           XX_MD2:  When 1-bit error occurs, ECC error corrects. 1-bit-error status flag sets, no interrupt.
 *                    When 2-bit error occurs, ECC error detects. 2-bit-error status flag sets and interrupt.
 *           XX_MD3:  When 1-bit error occurs, ECC error corrects. 1-bit-error status flag sets and interrupt.
 *                    When 2-bit error occurs, ECC error detects. 2-bit-error status flag sets and interrupt.
 * @{
 */
#define FMAC_RAM_ECC_INVD             (0x0UL)
#define FMAC_RAM_ECC_MD1              (FMAC_ECC_RAM0_CKCR_ECCMOD_0)
#define FMAC_RAM_ECC_MD2              (FMAC_ECC_RAM0_CKCR_ECCMOD_1)
#define FMAC_RAM_ECC_MD3              (FMAC_ECC_RAM0_CKCR_ECCMOD)
/**
 * @}
 */

/**
 * @defgroup FMAC_ECC_Err_Status_Flag FAMC ECC Error Status Flag
 * @{
 */
#define FMAC_ECC_FLAG_RAM0_1ERR             (FMAC_ECC_RAM0_CKSR_RAM_1ERR)
#define FMAC_ECC_FLAG_RAM0_2ERR             (FMAC_ECC_RAM0_CKSR_RAM_2ERR)
#define FMAC_ECC_FLAG_ALL                   (FMAC_ECC_FLAG_RAM0_1ERR | FMAC_ECC_FLAG_RAM0_2ERR)
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
 * @addtogroup FMAC_Global_Functions
 * @{
 */
int32_t FMAC_DeInit(CM_FMAC_TypeDef *FMACx);
int32_t FMAC_ReadParam_StructInit(stc_fmac_read_param_t *pstcFmacReadInit);
/* FIR functions */
int32_t FMAC_FIR_StructInit(stc_fmac_fir_init_t *pstcFmacFirInit);
int32_t FMAC_FIR_Init(CM_FMAC_TypeDef *FMACx, const stc_fmac_fir_init_t *pstcFmacFirInit);
void FMAC_FIR_CalcMode(CM_FMAC_TypeDef *FMACx, uint32_t u32CalcMode);
void FMAC_FIR_SWStart(CM_FMAC_TypeDef *FMACx);
void FMAC_FIR_Reset(CM_FMAC_TypeDef *FMACx);
void FMAC_FIR_DataClear(CM_FMAC_TypeDef *FMACx);

void FMAC_FIR_Cmd(CM_FMAC_TypeDef *FMACx, en_functional_state_t enNewState);
void FMAC_FIR_IntCmd(CM_FMAC_TypeDef *FMACx, en_functional_state_t enNewState);

void FMAC_FIR_WriteData(CM_FMAC_TypeDef *FMACx, float32_t *pf32Data, uint16_t u16Size);
void FMAC_FIR_WriteOneData(CM_FMAC_TypeDef *FMACx, float32_t f32Data);
void FMAC_FIR_GetResult(CM_FMAC_TypeDef *FMACx, float32_t *pf32Data);

en_flag_status_t FMAC_FIR_GetStatus(CM_FMAC_TypeDef *FMACx, uint32_t u32Flag);
void FMAC_FIR_ClearStatus(CM_FMAC_TypeDef *FMACx, uint32_t u32Flag);

void FMAC_FIR_SetLimit(CM_FMAC_TypeDef *FMACx, float32_t f32Max, float32_t f32Min);

void FMAC_FIR_ReadParam(CM_FMAC_TypeDef *FMACx, stc_fmac_read_param_t *pstcFmacReadParam);
void FMAC_FIR_ReadCacheNum(CM_FMAC_TypeDef *FMACx, uint8_t u8ReadType, uint16_t *pu16Data);
/* IIR functions */
int32_t FMAC_IIR_StructInit(stc_fmac_iir_init_t *pstcFmacIirInit);
int32_t FMAC_IIR_Init(CM_FMAC_TypeDef *FMACx, const stc_fmac_iir_init_t *pstcFmacIirInit);
void FMAC_IIR_CalcMode(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32CalcMode);
void FMAC_IIR_SWStart(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch);
void FMAC_IIR_Reset(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch);
void FMAC_IIR_DataClear(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch);

void FMAC_IIR_ChCmd(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, en_functional_state_t enNewState);
void FMAC_IIR_IntCmd(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, en_functional_state_t enNewState);

en_flag_status_t FMAC_IIR_GetStatus(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32Flag);
void FMAC_IIR_ClearStatus(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, uint32_t u32Flag);

void FMAC_IIR_GetResult(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data);
void FMAC_IIR_X_WriteData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data, uint16_t u16Size);
void FMAC_IIR_X_WriteOneData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t f32Data);
void FMAC_IIR_Y_WriteData(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t *pf32Data, uint16_t u16Size);

void FMAC_IIR_SetLimit(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, float32_t f32Max, float32_t f32Min);

void FMAC_IIR_ReadParam(CM_FMAC_TypeDef *FMACx, uint8_t u8Ch, stc_fmac_read_param_t *pstcFmacReadParam);
/* FMAC ECC */
void FMAC_SetEccMode(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32EccMode);
en_flag_status_t FMAC_GetEccStatus(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32Flag);
void FMAC_ClearStatus(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint32_t u32Flag);
void FMAC_EccErrorInjectCmd(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, en_functional_state_t enNewState);
void FMAC_ErrorInjectBitCmd(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam, uint64_t u64BitSel, en_functional_state_t enNewState);
uint32_t FMAC_GetEccErrorAddr(CM_FMAC_ECC_TypeDef *FMAC_ECCx, uint32_t u32EccRam);
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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_FMAC_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
