/**
 *******************************************************************************
 * @file  hc32_ll_sdfm.c
 * @brief This file provides firmware functions to manage the Sigma-Delta Filter
 *        Modulator (SDFM).
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
#include "hc32_ll_sdfm.h"
#include "hc32_ll_utility.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @defgroup LL_SDFM SDFM
 * @brief SDFM Driver Library
 * @{
 */

#if (LL_SDFM_ENABLE == DDL_ON)

/*******************************************************************************
 * Local type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Local pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SDFM_Local_Macros SDFM Local Macros
 * @{
 */
/* Delay count for RMU reset timeout */
#define SDFM_RMU_TIMEOUT                    (100UL)

#define SDFM_CKAB_TIMEOUT                   (50000U)

/* The maximum channel/filter number */
#define SDFM_CH_MAX                         (SDFM_CH7)
#define SDFM_FILTER_MAX                     (SDFM_FILTER3)

/* Initialization mask */
#define SDFM_CHCFGR1_INIT_MASK              (SDFM_CHCFGR1_DATPACK | SDFM_CHCFGR1_DATMPX | SDFM_CHCFGR1_CHINSEL | \
                                             SDFM_CHCFGR1_SITP | SDFM_CHCFGR1_SPICKSEL | \
                                             SDFM_CHCFGR1_SDCLKSEL | SDFM_CHCFGR1_SDCLKSYNC | SDFM_CHCFGR1_SDDATASYNC)
#define SDFM_CHCFGR2_INIT_MASK              (SDFM_CHCFGR2_OFFSET | SDFM_CHCFGR2_DTRBS)

#define SDFM_FLTCR1_INIT_MASK               (SDFM_FLTCR1_AWFSEL | SDFM_FLTCR1_FAST | SDFM_FLTCR1_RCH | \
                                             SDFM_FLTCR1_RDMAEN | SDFM_FLTCR1_RSYNC | SDFM_FLTCR1_RCONT | \
                                             SDFM_FLTCR1_JDMAEN | SDFM_FLTCR1_JEXTEN | SDFM_FLTCR1_JEXTSEL | \
                                             SDFM_FLTCR1_JSCAN | SDFM_FLTCR1_JSYNC | SDFM_FLTCR1_JSCAN_TYPE | \
                                             SDFM_FLTCR1_OVDEL)

#define SDFM_FLTCR1_REGU_CFG_MASK           (SDFM_FLTCR1_FAST | SDFM_FLTCR1_RCH | SDFM_FLTCR1_RDMAEN | \
                                             SDFM_FLTCR1_RSYNC | SDFM_FLTCR1_RCONT)

#define SDFM_FLTCR1_INJ_CFG_MASK            (SDFM_FLTCR1_JDMAEN | SDFM_FLTCR1_JEXTEN | SDFM_FLTCR1_JEXTSEL | \
                                             SDFM_FLTCR1_JSCAN | SDFM_FLTCR1_JSYNC | SDFM_FLTCR1_JSCAN_TYPE)

#define SDFM_FLTFIFOCTL_INIT_MASK           (SDFM_FLTFIFOCTL_SDFFIL | SDFM_FLTFIFOCTL_WTSYNCEN | SDFM_FLTFIFOCTL_FFSYNCLREN | \
                                             SDFM_FLTFIFOCTL_WTSCLREN)

#define SDFM_FLTEVTFLTCTL_INIT_MASK         (SDFM_FLTEVTFITCTL_SAMPWIN1 | SDFM_FLTEVTFITCTL_THRESH1)

#define SDFM_FLTCPARM_INIT_MASK             (SDFM_FLTCPARM_CEVT1DIGFILTSEL | SDFM_FLTCPARM_CEVT1SEL | SDFM_FLTCPARM_EN_CEVT1)

/**
 * @defgroup SDFM_Check_Parameters_Validity SDFM Check Parameters Validity
 * @{
 */
#define IS_SDFM_BIT_MASK(x, mask)           (((x) != 0U) && (((x) | (mask)) == (mask)))

#define IS_SDFM(x)                          ((x) == CM_SDFM)

#define IS_SDFM_CH(x)                       ((x) <= SDFM_CH_MAX)

#define IS_SDFM_MX_CH(x)                    IS_SDFM_BIT_MASK(x, SDFM_MX_CH_ALL)

#define IS_SDFM_FILTER(x)                   ((x) <= SDFM_FILTER_MAX)

#define IS_SDFM_SYNC_FILTER(x)              (((x) > SDFM_FILTER0) && ((x) <= SDFM_FILTER_MAX))

#define IS_SDFM_EVEN_CH(x)                  (((x) == SDFM_CH0) || ((x) == SDFM_CH2) || ((x) == SDFM_CH4) || ((x) == SDFM_CH6))

#define IS_SDFM_CH_SCD_THRESHOLD(x)         ((x) <= 0xFFU)

#define IS_SDFM_BREAK_SIGNAL(x)             ((x) <= 0xFU)

#define IS_SDFM_CH_DELAY(x)                 ((x) <= 63U)

#define IS_SDFM_CH_CKOUT_SRC(x)             (((x) == SDFM_CH_OUTPUT_CLK_SYS) || ((x) == SDFM_CH_OUTPUT_CLK_AUDIO))

#define IS_SDFM_CH_CKOUT_DIV(x)             (((x) >= 2U) && ((x) <= 256U))

#define IS_SDFM_CH_INPUT_MUX(x)                                                 \
(   ((x) == SDFM_CH_INPUT_DATA_EXT)         ||                                  \
    ((x) == SDFM_CH_INPUT_DATA_INTERN_REG))

#define IS_SDFM_CH_DATA_PACKING(x)                                              \
(   ((x) == SDFM_CH_INPUT_DATA_PACKING_STD)             ||                      \
    ((x) == SDFM_CH_INPUT_DATA_PACKING_INTERLEAVED)     ||                      \
    ((x) == SDFM_CH_INPUT_DATA_PACKING_DUAL))

#define IS_SDFM_CH_INPUT(x)                                                     \
(   ((x) == SDFM_CH_INPUT_PIN_SAME_CH_PIN)              ||                      \
    ((x) == SDFM_CH_INPUT_PIN_FOLLOW_CH_PIN))

#define IS_SDFM_CH_CAD(x)                                                       \
(   ((x) == SDFM_CH_CKAB_DISABLE)                       ||                      \
    ((x) == SDFM_CH_CKAB_ENABLE))

#define IS_SDFM_CH_SCD(x)                                                       \
(   ((x) == SDFM_CH_SCD_DISABLE)                        ||                      \
    ((x) == SDFM_CH_SCD_ENABLE))

#define IS_SDFM_CH_SERIAL_INTF_TYPE(x)                                          \
(   ((x) == SDFM_CH_SERIAL_INTF_SPI_RISING)             ||                      \
    ((x) == SDFM_CH_SERIAL_INTF_SPI_FALLING)            ||                      \
    ((x) == SDFM_CH_SERIAL_INTF_MANCHESTER_RISING)      ||                      \
    ((x) == SDFM_CH_SERIAL_INTF_MANCHESTER_FALLING))

#define IS_SDFM_CH_SPI_CLK(x)                                                   \
(   ((x) == SDFM_CH_SPI_CLK_EXT)                        ||                      \
    ((x) == SDFM_CH_SPI_CLK_INTERN)                     ||                      \
    ((x) == SDFM_CH_SPI_CLK_INTERN_DIV2_FALLING)        ||                      \
    ((x) == SDFM_CH_SPI_CLK_INTERN_DIV2_RISING))

#define IS_SDFM_CH_CLK_SEL(x)                                                   \
(   ((x) == SDFM_CH_CLK_SEL_SELF)                       ||                      \
    ((x) == SDFM_CH_CLK_SEL_CH0))

#define IS_SDFM_CH_CLK_SYNC(x)                                                  \
(   ((x) == SDFM_CH_CLK_SYNC_DISABLE)                   ||                      \
    ((x) == SDFM_CH_CLK_SYNC_ENABLE))

#define IS_SDFM_CH_DATA_SYNC(x)                                                 \
(   ((x) == SDFM_CH_DATA_SYNC_DISABLE)                  ||                      \
    ((x) == SDFM_CH_DATA_SYNC_ENABLE))

#define IS_SDFM_CH_AWD_SINC_ORDER(x)                                            \
(   ((x) == SDFM_CH_AWD_SINC_ORDER_FASTSINC)            ||                      \
    ((x) == SDFM_CH_AWD_SINC_ORDER_SINC1)               ||                      \
    ((x) == SDFM_CH_AWD_SINC_ORDER_SINC2)               ||                      \
    ((x) == SDFM_CH_AWD_SINC_ORDER_SINC3))

#define IS_SDFM_CH_AWD_SINC_FOSR(x)                 (((x) >= 1U) && ((x) <= 32U))

#define IS_SDFM_REGU_FAST_MD(x)                                                 \
(   ((x) == SDFM_FILTER_REGU_FAST_MD_DISABLE)           ||                      \
    ((x) == SDFM_FILTER_REGU_FAST_MD_ENABLE))

#define IS_SDFM_REGU_DMA(x)                                                     \
(   ((x) == SDFM_FILTER_REGU_DMA_DISABLE)               ||                      \
    ((x) == SDFM_FILTER_REGU_DMA_ENABLE))

#define IS_SDFM_REGU_SYNC_START(x)                                              \
(   ((x) == SDFM_FILTER_REGU_SYNC_START_DISABLE)        ||                      \
    ((x) == SDFM_FILTER_REGU_SYNC_START_ENABLE))

#define IS_SDFM_REGU_CONT_MD(x)                                                 \
(   ((x) == SDFM_FILTER_REGU_CONT_MD_DISABLE)           ||                      \
    ((x) == SDFM_FILTER_REGU_CONT_MD_ENABLE))

#define IS_SDFM_INJ_EXT_TRIG(x)                                                \
(   ((x) <= SDFM_FILTER_EXT_TRIG_JTRG20)                ||                      \
    (((x) >= SDFM_FILTER_EXT_TRIG_JTRG24) && ((x) <= SDFM_FILTER_EXT_TRIG_JTRG31)))

#define IS_SDFM_INJ_EXT_TRIG_EDGE(x)                                            \
(   ((x) == SDFM_FILTER_EXT_TRIG_DISABLE)               ||                      \
    ((x) == SDFM_FILTER_EXT_TRIG_EDGE_RISING)           ||                      \
    ((x) == SDFM_FILTER_EXT_TRIG_EDGE_FALLING)          ||                      \
    ((x) == SDFM_FILTER_EXT_TRIG_EDGE_BOTH))

#define IS_SDFM_INJ_SCAN_MD(x)                                                  \
(   ((x) == SDFM_FILTER_INJ_SCAN_MD_DISABLE)            ||                      \
    ((x) == SDFM_FILTER_INJ_SCAN_MD_ENABLE))

#define IS_SDFM_INJ_SYNC_START(x)                                               \
(   ((x) == SDFM_FILTER_INJ_SYNC_START_DISABLE)         ||                      \
    ((x) == SDFM_FILTER_INJ_SYNC_START_ENABLE))

#define IS_SDFM_INJ_DMA(x)                                                      \
(   ((x) == SDFM_FILTER_INJ_DMA_DISABLE)                ||                      \
    ((x) == SDFM_FILTER_INJ_DMA_ENABLE))

#define IS_SDFM_INJ_TYPE(x)                                                     \
(   ((x) == SDFM_FILTER_INJ_TYPE_COMM)                  ||                      \
    ((x) == SDFM_FILTER_INJ_TYPE_FIFO))

#define IS_SDFM_FILTER_SINC_FOSR_ORDER(osr, odr)                                \
(   (((odr) == SDFM_FILTER_SINC_ORDER_FASTSINC) && (((osr) >= 1U) && ((osr) <= 1024U))) || \
    (((odr) == SDFM_FILTER_SINC_ORDER_SINC1)    && (((osr) >= 1U) && ((osr) <= 1024U))) || \
    (((odr) == SDFM_FILTER_SINC_ORDER_SINC2)    && (((osr) >= 1U) && ((osr) <= 1024U))) || \
    (((odr) == SDFM_FILTER_SINC_ORDER_SINC3)    && (((osr) >= 1U) && ((osr) <= 1024U))) || \
    (((odr) == SDFM_FILTER_SINC_ORDER_SINC4)    && (((osr) >= 1U) && ((osr) <= 215U)))  || \
    (((odr) == SDFM_FILTER_SINC_ORDER_SINC5)    && (((osr) >= 1U) && ((osr) <= 73U))))

#define IS_SDFM_FILTER_SINC_IOSR(x)         (((x) >= 1U) && ((x) <= 256U))

#define IS_SDFM_FILTER_AWD_DATA_SRC(x)                                          \
(   ((x) == SDFM_FILTER_AWD_FILTER_DATA)                ||                      \
    ((x) == SDFM_FILTER_AWD_CH_DATA))

#define IS_SDFM_FILTER_OVF_DEAL(x)                                              \
(   ((x) == SDFM_FILTER_OVF_CLAMPING)                   ||                      \
    ((x) == SDFM_FILTER_OVF_CUTTING))

#define IS_SDFM_FILTER_INT(x)               IS_SDFM_BIT_MASK(x, SDFM_FILTER_INT_ALL)

#define IS_SDFM_FILTER_CH_INT(x)            IS_SDFM_BIT_MASK(x, SDFM_FILTER_CH_INT_ALL)

#define IS_SDFM_FILTER_FLAG(x)              IS_SDFM_BIT_MASK(x, SDFM_FILTER_FLAG_ALL)

#define IS_SDFM_FILTER_CH_FLAG(x)           IS_SDFM_BIT_MASK(x, SDFM_FILTER_CH_FLAG_ALL)

#define IS_SDFM_FILTER_FLAG_CLR(x)          IS_SDFM_BIT_MASK(x, SDFM_FILTER_FLAG_CLR_ALL)

#define IS_SDFM_FILTER_AWD_THRESHOLD(x)     (((x) >= -8388608) && ((x) <= 8388607))

#define IS_SDFM_FILTER_AWD_FLAG(x)          IS_SDFM_BIT_MASK(x, SDFM_FILTER_AWD_FLAG_ALL)

#define IS_SDFM_CH_CAL_OFFSET(x)            (((x) >= -8388608) && ((x) <= 8388607))

#define IS_SDFM_CH_RIGHT_BIT_SHIFT(x)       ((x) <= 31U)

#define IS_SDFM_FILTER_FIFO_INT_LEVEL(x)    ((x) <= 15U)

#define IS_SDFM_FILTER_FIFO_WAIT_FOR_SYNC(x)                                    \
(   ((x) == SDFM_FILTER_FIFO_WAIT_FOR_SYNC_DISABLE)     ||                      \
    ((x) == SDFM_FILTER_FIFO_WAIT_FOR_SYNC_ENABLE))

#define IS_SDFM_FILTER_FIFO_CLR_ON_SYNC(x)                                      \
(   ((x) == SDFM_FILTER_FIFO_CLR_ON_SYNC_DISABLE)       ||                      \
    ((x) == SDFM_FILTER_FIFO_CLR_ON_SYNC_ENABLE))

#define IS_SDFM_FILTER_FIFO_FLAG_CLR_ON_INT(x)                                  \
(   ((x) == SDFM_FILTER_FIFO_FLAG_CLR_ON_INT_DISABLE)    ||                     \
    ((x) == SDFM_FILTER_FIFO_FLAG_CLR_ON_INT_ENABLE))

#define IS_SDFM_FILTER_FIFO_FLAG(x)     IS_SDFM_BIT_MASK(x, SDFM_FILTER_FIFO_FLAG_ALL)

#define IS_SDFM_FILTER_CEVT(x)          (((x) == SDFM_FILTER_CEVT1) || ((x) == SDFM_FILTER_CEVT2))

#define IS_SDFM_FILTER_CEVT_INPUT(x)    ((x) <= 0x3UL)

#define IS_SDFM_FILTER_CEVT_OUTPUT(x)                                           \
(   ((x) == SDFM_FILTER_CEVT_OUTPUT_RAW)                ||                      \
    ((x) == SDFM_FILTER_CEVT_OUTPUT_FILTERED))

#define IS_SDFM_FILTER_CEVT_DF_SPL_WINDOW(x)            (((x) >= 1U) && ((x) <= 32U))

#define IS_SDFM_FILTER_CEVT_DF_VOTING_THRESHOLD(x)      ((x) <= 31U)

/**
 * @}
 */

/**
 * @defgroup SDFM_Register_Def SDFM Register Definition
 * @{
 */
#define SDFM_CHyCFGR1(ch)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x00UL + (ch) * 0x20UL))
#define SDFM_CHyCFGR2(ch)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x04UL + (ch) * 0x20UL))
#define SDFM_CHyAWSCDR(ch)          (*(__IO uint32_t *)(CM_SDFM_BASE + 0x08UL + (ch) * 0x20UL))
#define SDFM_CHyWDATR(ch)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x0CUL + (ch) * 0x20UL))
#define SDFM_CHyDATINR(ch)          (*(__IO uint32_t *)(CM_SDFM_BASE + 0x10UL + (ch) * 0x20UL))
#define SDFM_CHyDLYR(ch)            (*(__IO uint32_t *)(CM_SDFM_BASE + 0x14UL + (ch) * 0x20UL))
#define SDFM_CHyHLTZ(ch)            (*(__IO uint32_t *)(CM_SDFM_BASE + 0x18UL + (ch) * 0x20UL))

#define SDFM_FLTxCR1(flt)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x100UL + (flt) * 0x80UL))
#define SDFM_FLTxCR2(flt)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x104UL + (flt) * 0x80UL))
#define SDFM_FLTxISR(flt)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x108UL + (flt) * 0x80UL))
#define SDFM_FLTxICR(flt)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x10CUL + (flt) * 0x80UL))

#define SDFM_FLTxJCHGR(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x110UL + (flt) * 0x80UL))
#define SDFM_FLTxFCR(flt)           (*(__IO uint32_t *)(CM_SDFM_BASE + 0x114UL + (flt) * 0x80UL))
#define SDFM_FLTxJDATAR(flt)        (*(__IO uint32_t *)(CM_SDFM_BASE + 0x118UL + (flt) * 0x80UL))
#define SDFM_FLTxRDATAR(flt)        (*(__IO uint32_t *)(CM_SDFM_BASE + 0x11CUL + (flt) * 0x80UL))

#define SDFM_FLTxAWHTR(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x120UL + (flt) * 0x80UL))
#define SDFM_FLTxAWLTR(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x124UL + (flt) * 0x80UL))
#define SDFM_FLTxAWSR(flt)          (*(__IO uint32_t *)(CM_SDFM_BASE + 0x128UL + (flt) * 0x80UL))
#define SDFM_FLTxAWCFR(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x12CUL + (flt) * 0x80UL))

#define SDFM_FLTxEXMAX(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x130UL + (flt) * 0x80UL))
#define SDFM_FLTxEXMIN(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x134UL + (flt) * 0x80UL))
#define SDFM_FLTxCNVTIMR(flt)       (*(__IO uint32_t *)(CM_SDFM_BASE + 0x138UL + (flt) * 0x80UL))
#define SDFM_FLTxCNVTHRE(flt)       (*(__IO uint32_t *)(CM_SDFM_BASE + 0x13CUL + (flt) * 0x80UL))

#define SDFM_FLTxFIFOCTL(flt)       (*(__IO uint32_t *)(CM_SDFM_BASE + 0x140UL + (flt) * 0x80UL))
#define SDFM_FLTxDATFIFO(flt)       (*(__IO uint32_t *)(CM_SDFM_BASE + 0x144UL + (flt) * 0x80UL))
#define SDFM_FLTxAWHTR2(flt)        (*(__IO uint32_t *)(CM_SDFM_BASE + 0x148UL + (flt) * 0x80UL))
#define SDFM_FLTxAWLTR2(flt)        (*(__IO uint32_t *)(CM_SDFM_BASE + 0x14CUL + (flt) * 0x80UL))

#define SDFM_FLTxCPARM(flt)         (*(__IO uint32_t *)(CM_SDFM_BASE + 0x150UL + (flt) * 0x80UL))
#define SDFM_FLTxEVTFITCTL(flt)     (*(__IO uint32_t *)(CM_SDFM_BASE + 0x154UL + (flt) * 0x80UL))

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
 * @defgroup SDFM_Global_Functions SDFM Global Functions
 * @{
 */

/**
 * @brief  Initializes the specified SDFM channel according to the specified parameters
 *         in the structure pointed by pstcSdfmChInit.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  pstcSdfmChInit         Pointer to a @ref stc_sdfm_ch_init_t structure value that
 *                                      contains the configuration information for the SDFM channel.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSdfmChInit == NULL
 * @note   Some register bits can be modified only when CHyCFGR1.CHEN is zero.
 */
int32_t SDFM_CH_Init(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, const stc_sdfm_ch_init_t *pstcSdfmChInit)
{
    if (pstcSdfmChInit == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_INPUT_MUX(pstcSdfmChInit->u32InputMux));
    DDL_ASSERT(IS_SDFM_CH_DATA_PACKING(pstcSdfmChInit->u32DataPacking));
    DDL_ASSERT(IS_SDFM_CH_INPUT(pstcSdfmChInit->u32InputSel));

    DDL_ASSERT(IS_SDFM_CH_SERIAL_INTF_TYPE(pstcSdfmChInit->u32SerialIntfType));
    DDL_ASSERT(IS_SDFM_CH_SPI_CLK(pstcSdfmChInit->u32SpiClock));
    DDL_ASSERT(IS_SDFM_CH_CAL_OFFSET(pstcSdfmChInit->i32CalOffset));
    DDL_ASSERT(IS_SDFM_CH_RIGHT_BIT_SHIFT(pstcSdfmChInit->u32RightBitShift));

    DDL_ASSERT(IS_SDFM_CH_CLK_SEL(pstcSdfmChInit->u32ClockSel));
    DDL_ASSERT(IS_SDFM_CH_CLK_SYNC(pstcSdfmChInit->u32ClockSync));
    DDL_ASSERT(IS_SDFM_CH_DATA_SYNC(pstcSdfmChInit->u32DataSync));

    /* Set channel input parameters; Set serial interface parameters */
    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_INIT_MASK, \
                 (pstcSdfmChInit->u32InputMux | pstcSdfmChInit->u32DataPacking | \
                  pstcSdfmChInit->u32InputSel | pstcSdfmChInit->u32SerialIntfType | \
                  pstcSdfmChInit->u32SpiClock | pstcSdfmChInit->u32ClockSync | \
                  pstcSdfmChInit->u32DataSync | pstcSdfmChInit->u32ClockSel));

    /* Set channel offset and right bit shift */
    MODIFY_REG32(SDFM_CHyCFGR2(u32Ch), SDFM_CHCFGR2_INIT_MASK, \
                 ((uint32_t)pstcSdfmChInit->i32CalOffset << SDFM_CHCFGR2_OFFSET_POS) | \
                 (pstcSdfmChInit->u32RightBitShift << SDFM_CHCFGR2_DTRBS_POS));

    WRITE_REG32(SDFM_CHyHLTZ(u32Ch), (uint16_t)pstcSdfmChInit->i16CrossZeroThreshold);

    return LL_OK;
}

/**
 * @brief  Set each member of @ref stc_sdfm_ch_init_t to a default value.
 * @param  [in]  pstcSdfmChInit        Pointer to a @ref stc_sdfm_ch_init_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSdfmChInit == NULL.
 */
int32_t SDFM_CH_StructInit(stc_sdfm_ch_init_t *pstcSdfmChInit)
{
    if (pstcSdfmChInit == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Reset SDFM channel init structure parameters values */
    pstcSdfmChInit->u32InputMux                = SDFM_CH_INPUT_DATA_EXT;
    pstcSdfmChInit->u32DataPacking             = SDFM_CH_INPUT_DATA_PACKING_STD;
    pstcSdfmChInit->u32InputSel                = SDFM_CH_INPUT_PIN_SAME_CH_PIN;
    pstcSdfmChInit->u32SerialIntfType          = SDFM_CH_SERIAL_INTF_SPI_RISING;
    pstcSdfmChInit->u32SpiClock                = SDFM_CH_SPI_CLK_EXT;
    pstcSdfmChInit->i32CalOffset               = 0;
    pstcSdfmChInit->u32RightBitShift           = 0U;

    pstcSdfmChInit->u32ClockSel                = SDFM_CH_CLK_SEL_SELF;
    pstcSdfmChInit->u32ClockSync               = SDFM_CH_CLK_SYNC_DISABLE;
    pstcSdfmChInit->u32DataSync                = SDFM_CH_DATA_SYNC_DISABLE;

    pstcSdfmChInit->i16CrossZeroThreshold      = 0;

    return LL_OK;
}

/**
 * @brief  Enable/Disable SDFM channel.
 *         Once the channel is enabled, it receives serial data from the external sigma-delta modulator or
 *         parallel internal data sources (ADCs or CPU/DMA wire from memory).
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_CH_Cmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState)
{
    __IO uint32_t *CHyCFGR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    CHyCFGR1 = &SDFM_CHyCFGR1(u32Ch);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_CHEN);
    } else {
        CLR_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_CHEN);
    }
}

/**
 * @brief  Initializes the specified SDFM filter according to the specified parameters
 *         in the structure pointed by pstcSdfmFilterInit.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcSdfmFilterInit    Pointer to a @ref stc_sdfm_filter_init_t structure value that
 *                                      contains the configuration information for the SDFM filter.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSdfmChInit == NULL
 * @note   Some register bits can be modified only when FLTxCR1.DFEN is zero.
 */
int32_t SDFM_FILTER_Init(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_init_t *pstcSdfmFilterInit)
{
    if (pstcSdfmFilterInit == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Parameters check */
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    DDL_ASSERT(IS_SDFM_CH(pstcSdfmFilterInit->stcReguConv.u32RegularCh));
    DDL_ASSERT(IS_SDFM_REGU_FAST_MD(pstcSdfmFilterInit->stcReguConv.u32FastMode));
    DDL_ASSERT(IS_SDFM_REGU_DMA(pstcSdfmFilterInit->stcReguConv.u32DmaRead));
    DDL_ASSERT(IS_SDFM_REGU_SYNC_START(pstcSdfmFilterInit->stcReguConv.u32SyncStart));
    DDL_ASSERT(IS_SDFM_REGU_CONT_MD(pstcSdfmFilterInit->stcReguConv.u32ContMode));

    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG(pstcSdfmFilterInit->stcInjConv.u32ExtTrigger));
    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG_EDGE(pstcSdfmFilterInit->stcInjConv.u32ExtTriggerEdge));
    DDL_ASSERT(IS_SDFM_INJ_DMA(pstcSdfmFilterInit->stcInjConv.u32DmaRead));
    DDL_ASSERT(IS_SDFM_INJ_SCAN_MD(pstcSdfmFilterInit->stcInjConv.u32ScanMode));
    DDL_ASSERT(IS_SDFM_INJ_SYNC_START(pstcSdfmFilterInit->stcInjConv.u32SyncStart));
    DDL_ASSERT(IS_SDFM_INJ_TYPE(pstcSdfmFilterInit->stcInjConv.u32Type));
    if (pstcSdfmFilterInit->stcInjConv.u32Type == SDFM_FILTER_INJ_TYPE_COMM) {
        DDL_ASSERT(IS_SDFM_MX_CH(pstcSdfmFilterInit->stcInjConv.u32InjectedCh));
    } /* else {
        NOTE: Injected FOFO mode use the regular channel.
        DDL_ASSERT(IS_SDFM_CH(pstcSdfmFilterInit->stcReguConv.u32RegularCh));
    } */

    DDL_ASSERT(IS_SDFM_FILTER_SINC_FOSR_ORDER(pstcSdfmFilterInit->stcSinc.u32SincFOSR, \
                                              pstcSdfmFilterInit->stcSinc.u32SincOrder));
    DDL_ASSERT(IS_SDFM_FILTER_SINC_IOSR(pstcSdfmFilterInit->stcSinc.u32SincIOSR));

    DDL_ASSERT(IS_SDFM_FILTER_OVF_DEAL(pstcSdfmFilterInit->u32OvfDeal));

    /* Parameters configuration */
    /* Set regular conversion and injected conversion parameters */
    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_INIT_MASK, \
                 pstcSdfmFilterInit->stcReguConv.u32FastMode       | \
                 (pstcSdfmFilterInit->stcReguConv.u32RegularCh << SDFM_FLTCR1_RCH_POS) | \
                 pstcSdfmFilterInit->stcReguConv.u32DmaRead        | \
                 pstcSdfmFilterInit->stcReguConv.u32SyncStart      | \
                 pstcSdfmFilterInit->stcReguConv.u32ContMode       | \
                 (pstcSdfmFilterInit->stcInjConv.u32ExtTrigger << SDFM_FLTCR1_JEXTSEL_POS) | \
                 pstcSdfmFilterInit->stcInjConv.u32ExtTriggerEdge  | \
                 pstcSdfmFilterInit->stcInjConv.u32DmaRead         | \
                 pstcSdfmFilterInit->stcInjConv.u32ScanMode        | \
                 pstcSdfmFilterInit->stcInjConv.u32SyncStart       | \
                 pstcSdfmFilterInit->stcInjConv.u32Type            | \
                 pstcSdfmFilterInit->u32OvfDeal);

    /* Set injected conversion channels */
    WRITE_REG32(SDFM_FLTxJCHGR(u32Filter), pstcSdfmFilterInit->stcInjConv.u32InjectedCh);

    /* Set filter control parameters */
    WRITE_REG32(SDFM_FLTxFCR(u32Filter), pstcSdfmFilterInit->stcSinc.u32SincOrder | \
                ((pstcSdfmFilterInit->stcSinc.u32SincFOSR - 1U) << SDFM_FLTFCR_FOSR_POS) | \
                (pstcSdfmFilterInit->stcSinc.u32SincIOSR - 1U));

    return LL_OK;
}

/**
 * @brief  Set each member of @ref stc_sdfm_filter_init_t to a default value.
 * @param  [in]  pstcSdfmFilterInit    Pointer to a @ref stc_sdfm_filter_init_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSdfmFilterInit == NULL.
 */
int32_t SDFM_FILTER_StructInit(stc_sdfm_filter_init_t *pstcSdfmFilterInit)
{
    if (pstcSdfmFilterInit == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    pstcSdfmFilterInit->stcReguConv.u32RegularCh     = SDFM_CH0;
    pstcSdfmFilterInit->stcReguConv.u32FastMode      = SDFM_FILTER_REGU_FAST_MD_DISABLE;
    pstcSdfmFilterInit->stcReguConv.u32DmaRead       = SDFM_FILTER_REGU_DMA_DISABLE;
    pstcSdfmFilterInit->stcReguConv.u32SyncStart     = SDFM_FILTER_REGU_SYNC_START_DISABLE;
    pstcSdfmFilterInit->stcReguConv.u32ContMode      = SDFM_FILTER_REGU_CONT_MD_DISABLE;

    pstcSdfmFilterInit->stcInjConv.u32InjectedCh     = SDFM_MX_CH0;
    pstcSdfmFilterInit->stcInjConv.u32ExtTrigger     = SDFM_FILTER_EXT_TRIG_JTRG0;
    pstcSdfmFilterInit->stcInjConv.u32ExtTriggerEdge = SDFM_FILTER_EXT_TRIG_DISABLE;
    pstcSdfmFilterInit->stcInjConv.u32DmaRead        = SDFM_FILTER_INJ_DMA_DISABLE;
    pstcSdfmFilterInit->stcInjConv.u32ScanMode       = SDFM_FILTER_INJ_SCAN_MD_DISABLE;
    pstcSdfmFilterInit->stcInjConv.u32SyncStart      = SDFM_FILTER_INJ_SYNC_START_DISABLE;
    pstcSdfmFilterInit->stcInjConv.u32Type           = SDFM_FILTER_INJ_TYPE_COMM;

    pstcSdfmFilterInit->stcSinc.u32SincOrder = SDFM_FILTER_SINC_ORDER_FASTSINC;
    pstcSdfmFilterInit->stcSinc.u32SincFOSR  = 1U;
    pstcSdfmFilterInit->stcSinc.u32SincIOSR  = 1U;

    pstcSdfmFilterInit->u32OvfDeal = SDFM_FILTER_OVF_CLAMPING;

    return LL_OK;
}

/**
 * @brief  Enable/Disable the specified interrupts of the specified filter.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32FilterInt           The interrupts to be enabled/disabled.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Int
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_FILTER_IntCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FilterInt, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR2;
    __IO uint32_t *FLTxCPARM;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_INT(u32FilterInt));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR2   = &SDFM_FLTxCR2(u32Filter);
    FLTxCPARM = &SDFM_FLTxCPARM(u32Filter);

    if (enNewState == ENABLE) {
        if ((u32FilterInt & SDFM_FILTER_INT_CH_CROSS_ZERO) != 0U) {
            SET_REG32_BIT(*FLTxCPARM, (1UL << SDFM_FLTCPARM_HZEN_POS));
        }
        SET_REG32_BIT(*FLTxCR2, u32FilterInt & (~SDFM_FILTER_INT_CH_CROSS_ZERO));
    } else {
        if ((u32FilterInt & SDFM_FILTER_INT_CH_CROSS_ZERO) != 0U) {
            CLR_REG32_BIT(*FLTxCPARM, (1UL << SDFM_FLTCPARM_HZEN_POS));
        }
        CLR_REG32_BIT(*FLTxCR2, u32FilterInt & (~SDFM_FILTER_INT_CH_CROSS_ZERO));
    }
}

/**
 * @brief  Enable/Disable the specified channel interrupts for all FILTERs.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32ChInt               The channel interrupts to be enabled/disabled.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Ch_Int
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_FILTER_ChIntCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChInt, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER_CH_INT(u32ChInt));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG32_BIT(SDFMx->FLT0CR2, u32ChInt);
    } else {
        CLR_REG32_BIT(SDFMx->FLT0CR2, u32ChInt);
    }
}

/**
 * @brief  Enable/Disable the specified filter.
 *         Once FILTERx is enabled (DFEN=1), both Sinc digital filter unit and integrator unit are reinitialized.
 *         By clearing DFEN, any conversion which may be in progress is immediately stopped and FILTERx is put into stop mode.
 *         All register settings remain unchanged except SDFM_FLTxAWSR and SDFM_FLTxISR (which are reset).
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  DISABLE:                     SDFM_FLTx is disabled. All conversions of given SDFM_FLTx are stopped immediately
 *                                      and all SDFM_FLTx functions are stopped.
 *                                      Data which are cleared by setting DFEN = 0:
 *                                      - Register SDFM_FLTxISR is set to the reset state
 *                                      - Register SDFM_FLTxAWSR is set to the reset state
 *   @arg  ENABLE:                      SDFM_FLTx is enabled. If SDFM_FLTx is enabled, then SDFM_FLTx starts operating
 *                                      according to its setting.
 * @retval None
 */
void SDFM_FILTER_Cmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        /* Enable the filter */
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_DFEN);
    } else {
        /* Disable the filter */
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_DFEN);
    }
}

/**
 * @brief  Global enable/disable for SDFM interface.
 *         SDFM must be globally disabled (by set CH0CFGR1.SDFMEN to zero) before
 *         stopping the system clock to enter in the STOP mode of the device.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  ENABLE:                      SDFM interface enabled.
 *                                      If SDFM interface is enabled, then it is started to operate according to
 *                                      enabled y channels and enabled x filters settings (CHEN bit in SDFM_CHyCFGR1
 *                                      and DFEN bit in SDFM_FLTxCR1).
 *   @arg  DISABLE:                     SDFM interface disabled.
 *                                      Data cleared by setting CH0CFGR1.SDFMEN to zero:
 *                                      - All registers SDFM_FLTxISR are set to reset state
 *                                      - All registers SDFM_FLTxAWSR are set to reset state
 * @retval None
 * @note   SDFMEN is present only in SDFM_CH0CFGR1 register
 */
void SDFM_Cmd(CM_SDFM_TypeDef *SDFMx, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    if (enNewState == ENABLE) {
        SET_REG32_BIT(SDFMx->CH0CFGR1, SDFM_CHCFGR1_SDFMEN);
    } else {
        CLR_REG32_BIT(SDFMx->CH0CFGR1, SDFM_CHCFGR1_SDFMEN);
    }
}

/**
 * @brief  De-Initialize SDFM.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @retval int32_t:
 *           - LL_OK:                   De-Initialize success.
 *           - LL_ERR_TIMEOUT:          Timeout.
 */
int32_t SDFM_DeInit(CM_SDFM_TypeDef *SDFMx)
{
    __IO uint32_t u32TimeOut = 0U;

    /* Check FRST register protect */
    DDL_ASSERT((CM_PWC->FPRC & PWC_FPRC_FPRCB1) == PWC_FPRC_FPRCB1);

    /* Reset */
    WRITE_REG32(bCM_RMU->FRST0_b.SDFM, 0UL);
    /* Ensure reset procedure is completed */
    while (READ_REG32(bCM_RMU->FRST0_b.SDFM) != 1UL) {
        u32TimeOut++;
        if (u32TimeOut > SDFM_RMU_TIMEOUT) {
            return LL_ERR_TIMEOUT;
        }
    }

    return LL_OK;
}

/***********************************************************************************************************************
***********************************************************************************************************************/
/**
 * @brief  Configures output serial clock(Max 20MHz).
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32ClockOutSrc         Output serial clock source.
 *                                      This parameter can be a value of @ref SDFM_Ch_Output_Clk_Src
 * @param  [in]  u32ClockOutDiv         Output serial clock divider.
 *                                      This parameter must be a number between 2 and 256.
 * @retval None
 * @note   Calling this function will enable serial clock output.
 * @note   These register bits can be modified only when CH0CFGR1.SDFMEN is zero.
 */
void SDFM_CH_ClockOutConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32ClockOutSrc, uint32_t u32ClockOutDiv)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH_CKOUT_SRC(u32ClockOutSrc));
    DDL_ASSERT(IS_SDFM_CH_CKOUT_DIV(u32ClockOutDiv));

    MODIFY_REG32(SDFMx->CH0CFGR1, SDFM_CHCFGR1_CKOUTSRC | SDFM_CHCFGR1_CKOUTDIV, \
                 u32ClockOutSrc | (u32ClockOutDiv - 1U) << SDFM_CHCFGR1_CKOUTDIV_POS);
}

/**
 * @brief  Disable output serial clock.
 *         Output clock generation is disabled(CKOUT signal is set to low state).
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @retval None
 * @note   These register bits can be modified only when CH0CFGR1.SDFMEN is zero.
 */
void SDFM_CH_DisableClockOut(CM_SDFM_TypeDef *SDFMx)
{
    DDL_ASSERT(IS_SDFM(SDFMx));

    CLR_REG32_BIT(SDFMx->CH0CFGR1, SDFM_CHCFGR1_CKOUTDIV);
}

/**
 * @brief  Enable/Disable clock absence detector.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval int32_t:
 *           - LL_OK:                   Command success.
 *           - LL_ERR_TIMEOUT:          Timeout.
 */
int32_t SDFM_CH_ClockAbsenceDetectorCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t *CHyCFGR1;
    __IO uint32_t u32TimeOut = 0U;
    uint32_t u32CkabFlag = 1UL << (u32Ch + SDFM_FLTISR_CKABF_POS);

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    CHyCFGR1 = &SDFM_CHyCFGR1(u32Ch);
    if (enNewState == ENABLE) {
        while ((READ_REG32_BIT(SDFMx->FLT0ISR, u32CkabFlag)) != 0U) {
            WRITE_REG32(SDFMx->FLT0ICR, u32CkabFlag);
            u32TimeOut++;
            if (u32TimeOut > SDFM_CKAB_TIMEOUT) {
                i32Ret = LL_ERR_TIMEOUT;
                break;
            }
        }
        if (i32Ret == LL_OK) {
            SET_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_CKABEN);
        }
    } else {
        CLR_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_CKABEN);
        WRITE_REG32(SDFMx->FLT0ICR, u32CkabFlag);
    }

    return i32Ret;
}

/**
 * @brief  Configures short-circuit detector.
 *         Set threshold and break signals.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32Threshold           Short circuit detector threshold.
 *                                      This parameter must be a number between 0 and 255.
 * @param  [in]  u32BreakSignal         Break signals assigned to short circuit event.
 *                                      This parameter can be any combination of @ref SDFM_Break_Signal
 *                                      BKSCD[i] = 0: Break i signal not assigned to short-circuit detector on the channel
 *                                      BKSCD[i] = 1: Break i signal assigned to short-circuit detector on the channel
 * @retval None
 */
void SDFM_CH_ShortCircuitDetectorConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32Threshold, uint32_t u32BreakSignal)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_SCD_THRESHOLD(u32Threshold));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(u32BreakSignal));

    /* Configure threshold and break signals */
    MODIFY_REG32(SDFM_CHyAWSCDR(u32Ch), SDFM_CHAWSCDR_BKSCD | SDFM_CHAWSCDR_SCDT, \
                 (u32BreakSignal << SDFM_CHAWSCDR_BKSCD_POS) | u32Threshold);
}

/**
 * @brief  Enable/Disable short-circuit detector.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_CH_ShortCircuitDetectorCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState)
{
    __IO uint32_t *CHyCFGR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));

    CHyCFGR1 = &SDFM_CHyCFGR1(u32Ch);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_SCDEN);
    } else {
        CLR_REG32_BIT(*CHyCFGR1, SDFM_CHCFGR1_SCDEN);
    }
}

/**
 * @brief  Specifies input data multiplexer for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32InputMux            Input data multiplexer for the specified channel.
 *                                      This parameter can be a value of @ref SDFM_Ch_Input_Data_Mux
 * @retval None
 */
void SDFM_CH_SelectInputMux(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32InputMux)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_INPUT_MUX(u32InputMux));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_DATMPX, u32InputMux);
}

/**
 * @brief  Specifies data packing mode in SDFM_CHyDATINR register.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32DataPacking         Data packing mode.
 *                                      This parameter can be a value of @ref SDFM_Ch_Input_Data_Packing
 * @retval None
 */
void SDFM_CH_SetDataPacking(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32DataPacking)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_DATA_PACKING(u32DataPacking));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_DATPACK, u32DataPacking);
}

/**
 * @brief  Selects input pins for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32InputSel            Channel inputs selection.
 *                                      This parameter can be a value of @ref SDFM_Ch_Input
 * @retval None
 */
void SDFM_CH_SelectInput(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32InputSel)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_INPUT(u32InputSel));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_CHINSEL, u32InputSel);
}

/**
 * @brief  Spepcifies serial interface type for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32IntfType            Serial interface type for the specified channel.
 *                                      This parameter can be a value of @ref SDFM_Ch_Serial_Intf_Type
 * @retval None
 */
void SDFM_CH_SetSerialIntfType(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32IntfType)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_SERIAL_INTF_TYPE(u32IntfType));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_SITP, u32IntfType);
}

/**
 * @brief  Specifies SPI clock for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32SpiClk              SPI clock select for the specified channel.
 *                                      This parameter can be a value of @ref SDFM_Ch_SPI_Clk
 * @retval None
 */
void SDFM_CH_SetSpiClock(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32SpiClk)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_SPI_CLK(u32SpiClk));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_SPICKSEL, u32SpiClk);
}

/**
 * @brief  Configures Analog watchdog for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32AwdSincOrder        Analog watchdog Sinc filter order.
                                        This parameter can be a value of @ref SDFM_Ch_Awd_Sinc_Order
 * @param  [in]  u32AwdSincFOSR         Analog watchdog filter oversampling ratio (decimation rate).
                                        This parameter must be a number between 1 and 32
 * @retval None
 */
void SDFM_CH_AwdConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32AwdSincOrder, uint32_t u32AwdSincFOSR)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_AWD_SINC_ORDER(u32AwdSincOrder));
    DDL_ASSERT(IS_SDFM_CH_AWD_SINC_FOSR(u32AwdSincFOSR));

    MODIFY_REG32(SDFM_CHyAWSCDR(u32Ch), (SDFM_CHAWSCDR_AWFOSR | SDFM_CHAWSCDR_AWFORD), \
                 ((u32AwdSincFOSR - 1U) << SDFM_CHAWSCDR_AWFOSR_POS) | u32AwdSincOrder);
}

/**
 * @brief  Get analog watchdog filter data of the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @retval Channel analog watchdog value.
 * @note   Same mode has to be used for all channels
 */
int16_t SDFM_CH_GetAwdValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));

    return (int16_t)SDFM_CHyWDATR(u32Ch);
}

/**
 * @brief  Set calibration offset for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  i32CalOffset           Calibration offset value.
 *                                      This parameter must be a number between -8388608 and 8388607.
 * @retval None
 * @note   Same mode has to be used for all channels
 */
void SDFM_CH_SetCalOffset(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int32_t i32CalOffset)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_CAL_OFFSET(i32CalOffset));

    /* Modify channel offset */
    MODIFY_REG32(SDFM_CHyCFGR2(u32Ch), SDFM_CHCFGR2_OFFSET, ((uint32_t)i32CalOffset << SDFM_CHCFGR2_OFFSET_POS));
}

/**
 * @brief  Write input data in standard mode.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  i16InputData           Input data (assigned to channel y)
 * @retval None
 */
void SDFM_CH_WriteInputDataStdMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16InputData)
{
    uint16_t u16Data = (uint16_t)i16InputData;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));

    WRITE_REG32(SDFM_CHyDATINR(u32Ch), u16Data);
}

/**
 * @brief  Write input data in interleaved mode.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  i16FirstSample         First sample (assigned to channel y)
 * @param  [in]  i16SecondSample        Second sample (assigned to channel y)
 * @retval None
 */
void SDFM_CH_WriteInputDataInterleavedMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16FirstSample, int16_t i16SecondSample)
{
    uint16_t u16Data1 = (uint16_t)i16FirstSample;
    uint16_t u16Data2 = (uint16_t)i16SecondSample;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));

    WRITE_REG32(SDFM_CHyDATINR(u32Ch), ((uint32_t)u16Data2 << 16U) | u16Data1);
}

/**
 * @brief  Write input data in dual mode.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  i16YSample             First sample (assigned to channel y)
 * @param  [in]  i16Y1Sample            Second sample (assigned to channel y+1)
 * @retval None
 */
void SDFM_CH_WriteInputDataDualMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16YSample, int16_t i16Y1Sample)
{
    uint16_t u16DataY  = (uint16_t)i16YSample;
    uint16_t u16DataY1 = (uint16_t)i16Y1Sample;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));

    WRITE_REG32(SDFM_CHyDATINR(u32Ch), ((uint32_t)u16DataY1 << 16U) | u16DataY);
}

/**
 * @brief  Set delay for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u8Delay                Delay for the specified channel.
 *                                      This parameter must be a number between 0 and 63.
 * @retval None
 */
void SDFM_CH_SetDelay(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint8_t u8Delay)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_DELAY(u8Delay));

    WRITE_REG32(SDFM_CHyDLYR(u32Ch), u8Delay);
}

/**
 * @brief  Select clock for the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  u32ClockSel            Clock selection.
 *                                      This parameter can be a value of @ref SDFM_Ch_Clk_Sel
 * @retval None
 */
void SDFM_CH_ClockSelect(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32ClockSel)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_SDFM_CH_CLK_SEL(u32ClockSel));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_SDCLKSEL, u32ClockSel);
}

/**
 * @brief  Enable/Disable clock synchronization of the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration type value.
 * @retval None
 */
void SDFM_CH_ClockSyncCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_SDCLKSYNC, (uint32_t)enNewState << SDFM_CHCFGR1_SDCLKSYNC_POS);
}

/**
 * @brief  Enable/Disable data synchronization of the specified channel.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Ch                  SDFM channel.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration type value.
 * @retval None
 */
void SDFM_CH_DataSyncCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_CH(u32Ch));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(SDFM_CHyCFGR1(u32Ch), SDFM_CHCFGR1_SDDATASYNC, (uint32_t)enNewState << SDFM_CHCFGR1_SDDATASYNC_POS);
}

/***********************************************************************************************************************
***********************************************************************************************************************/
/**
 * @brief Get the status of the specified filter flag.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Flag                SDFM filter flag.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t SDFM_FILTER_GetStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_FLAG(u32Flag));

    /* Return SET if one of the specified flags is SET. */
    return (en_flag_status_t)(READ_REG32_BIT(SDFM_FLTxISR(u32Filter), u32Flag) != 0U);
}

/**
 * @brief Get the status of the specified filter channel flag.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32ChFlag              SDFM filter channel flag.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Ch_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t SDFM_FILTER_GetChStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChFlag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER_CH_FLAG(u32ChFlag));

    /* Clock absence flags and short-circuit detector flags are present only in SDFM_FLT0ISR register */
    /* Return SET if one of the specified flags is SET. */
    return (en_flag_status_t)(READ_REG32_BIT(SDFMx->FLT0ISR, u32ChFlag) != 0U);
}

/**
 * @brief Clear the specified filter flag.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Flag                SDFM filter flag.
 *                                      This parameter can be any combination that included in
 *                                      SDFM_FILTER_FLAG_CLR_ALL of @ref SDFM_Filter_Status_Flag
 * @retval None
 */
void SDFM_FILTER_ClearStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_FLAG_CLR(u32Flag));

    /* Clear the selected flags */
    WRITE_REG32(SDFM_FLTxICR(u32Filter), u32Flag);
}

/**
 * @brief Clear the specified filter channel flag.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32ChFlag              SDFM filter channel flag.
 *                                      This parameter can be any combination that included in
 *                                      SDFM_FILTER_FLAG_CLR_ALL of @ref SDFM_Filter_Ch_Status_Flag
 * @retval int32_t:
 *           - LL_OK:                   Operation success.
 *           - LL_ERR_TIMEOUT:          Timeout.
 */
int32_t SDFM_FILTER_ClearChStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChFlag)
{
    int32_t i32Ret = LL_OK;
    __IO uint32_t u32TimeOut = 0U;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER_CH_FLAG(u32ChFlag));

    /* Clock absence flags and short-circuit detector flags are present only in SDFM_FLT0ICR register */
    /* Clear the selected flags */
    while (1U) {
        WRITE_REG32(SDFMx->FLT0ICR, u32ChFlag);
        if ((READ_REG32_BIT(SDFMx->FLT0ISR, u32ChFlag)) == 0U) {
            break;
        }
        u32TimeOut++;
        if (u32TimeOut > SDFM_CKAB_TIMEOUT) {
            i32Ret = LL_ERR_TIMEOUT;
            break;
        }
    }

    return i32Ret;
}

/**
 * @brief  Configures regular conversion.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcRegu               Pointer to a @ref stc_sdfm_filter_regular_conv_t structure value that
 *                                      contains the configuration information for regular conversion.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcRegu == NULL.
 * @note   Some register bits can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
int32_t SDFM_FILTER_RegularConvConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter,
                                      const stc_sdfm_filter_regular_conv_t *pstcRegu)
{
    if (pstcRegu == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    DDL_ASSERT(IS_SDFM_CH(pstcRegu->u32RegularCh));
    DDL_ASSERT(IS_SDFM_REGU_FAST_MD(pstcRegu->u32FastMode));
    DDL_ASSERT(IS_SDFM_REGU_DMA(pstcRegu->u32DmaRead));
    DDL_ASSERT(IS_SDFM_REGU_SYNC_START(pstcRegu->u32SyncStart));
    DDL_ASSERT(IS_SDFM_REGU_CONT_MD(pstcRegu->u32ContMode));

    /* Parameters configuration */
    /* Set regular conversion conversion parameters */
    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_REGU_CFG_MASK, \
                 pstcRegu->u32FastMode | pstcRegu->u32DmaRead | \
                 pstcRegu->u32SyncStart | pstcRegu->u32ContMode | \
                 (pstcRegu->u32RegularCh << SDFM_FLTCR1_RCH_POS));

    return LL_OK;
}

/**
 * @brief  Enable/Disable continuous mode selection for regular conversions.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  DISABLE:                     Disable continuous mode selection for regular conversions.
 *                                      The regular channel is converted just once for each conversion request.
 *   @arg  ENABLE:                      Enable continuous mode selection for regular conversions.
 *                                      The regular channel is converted repeatedly after each conversion request.
 * @retval None
 * @note   Disable continuous mode while a continuous regular conversion is already in progress stops the continuous
 *         mode immediately.
 */
void SDFM_FILTER_RegularConvContModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_RCONT, (uint32_t)enNewState << SDFM_FLTCR1_RCONT_POS);
}

/**
 * @brief  Enable/Disable launch regular conversion synchronously with SDFM_FLT.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  DISABLE:                     Do not launch a regular conversion synchronously with SDFM_FLT.
 *   @arg  ENABLE:                      Launch a regular conversion in this SDFM_FLTx(x > 0) at the very moment when a
 *                                      regular conversion is launched in SDFM_FLT.
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_RegularConvSyncStartCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_SYNC_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_RSYNC);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_RSYNC);
    }
}

/**
 * @brief  Enable/Disable DMA channel enabled to read data for the regular conversion.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_RegularConvDmaReadCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_SYNC_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_RDMAEN);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_RDMAEN);
    }
}

/**
 * @brief  Enable/Disable fast conversion mode selection for regular conversions.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   When converting a regular conversion in continuous mode, having enabled the fast mode causes each conversion
 *         (except the first) to execute faster than in standard mode. This bit has no effect on conversions which are
 *         not continuous.
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_RegularConvFastModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_SYNC_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_FAST);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_FAST);
    }
}

/**
 * @brief  Set one channel for regular conversion.
 *         Calling this function when RCIP=1 takes effect when the next regular conversion begins. This is especially
 *         useful in continuous mode (when RCONT=1). It also affects regular conversions which are pending (due to
 *         ongoing injected conversion).
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32ReguCh              Channel for regular conversion.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @retval None
 */
void SDFM_FILTER_SetRegularConvCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32ReguCh)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_CH(u32ReguCh));

    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_RCH, u32ReguCh << SDFM_FLTCR1_RCH_POS);
}

/**
 * @brief  Software start a regular conversion on the specified SDFM filter.
 *         Call this function to make a request to start a conversion on the regular channel and causes RCIP to become '1'.
 *         If RCIP=1 already or RSYNC=1, calling this function has no effect.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval None
 */
void SDFM_FILTER_StartRegularConv(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    SET_REG32_BIT(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_RSWSTART);
}

/**
 * @brief  Get regular conversion value.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [out] pu32Ch                 Pointer to an uint32_t type address to store corresponding channel of regular conversion.
 *                                      The output value can be a value of @ref SDFM_Channel
 * @retval Regular conversion value.
 */
int32_t SDFM_FILTER_GetRegularConvValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch)
{
    uint32_t u32Reg;
    int32_t i32Value;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    u32Reg = SDFM_FLTxRDATAR(u32Filter);

    /* Extract channel and regular conversion value */
    if (pu32Ch != NULL) {
        *pu32Ch = u32Reg & SDFM_FLTRDATAR_RDATACH;
    }
    /* Regular conversion value is a signed value located on 24 MSB of register */
    /* So after applying a mask on these bits we have to perform a division by 256 (2 raised to the power of 8) */
    u32Reg &= SDFM_FLTRDATAR_RDATA;
    i32Value = (int32_t)u32Reg / 256;

    return i32Value;
}

/**
 * @brief  Configures injected conversion.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcInj                Pointer to a @ref stc_sdfm_filter_injected_conv_t structure value that
 *                                      contains the configuration information for injected conversion.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcInj == NULL.
 * @note   Some register bits can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
int32_t SDFM_FILTER_InjectedConvConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter,
                                       const stc_sdfm_filter_injected_conv_t *pstcInj)
{
    if (pstcInj == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Parameters check */
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    DDL_ASSERT(IS_SDFM_MX_CH(pstcInj->u32InjectedCh));
    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG(pstcInj->u32ExtTrigger));
    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG_EDGE(pstcInj->u32ExtTriggerEdge));
    DDL_ASSERT(IS_SDFM_INJ_DMA(pstcInj->u32DmaRead));
    DDL_ASSERT(IS_SDFM_INJ_SCAN_MD(pstcInj->u32ScanMode));
    DDL_ASSERT(IS_SDFM_INJ_SYNC_START(pstcInj->u32SyncStart));
    DDL_ASSERT(IS_SDFM_INJ_TYPE(pstcInj->u32Type));

    /* Parameters configuration */
    /* Set injected conversion parameters */
    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_INJ_CFG_MASK, \
                 pstcInj->u32DmaRead | pstcInj->u32ScanMode | pstcInj->u32SyncStart | \
                 pstcInj->u32ExtTriggerEdge | pstcInj->u32Type | \
                 (pstcInj->u32ExtTrigger << SDFM_FLTCR1_JEXTSEL_POS));

    /* Set injected conversion channels */
    WRITE_REG32(SDFM_FLTxJCHGR(u32Filter), pstcInj->u32InjectedCh);

    return LL_OK;
}

/**
 * @brief  Enable/Disable launch injected conversion synchronously with SDFM_FLT.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg      DISABLE:                 Do not launch an injected conversion synchronously with SDFM_FLT.
 *   @arg      ENABLE:                  Launch an injected conversion in this SDFM_FLTx at the very moment when an injected
 *                                      conversion is launched in SDFM_FLT by its JSWSTART trigger.
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_InjectedConvSyncStartCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JSYNC);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JSYNC);
    }
}

/**
 * @brief  Enable/Disable injected conversion scan mode.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 *   @arg  DISABLE:                     Disable scanning mode.
 *                                      Only one channel is converted from the selected channels(SDFM_FLTxJCHGR.JCHG),
 *                                      and the channel selection is moved to the next channel.
 *   @arg  ENABLE:                      Enable scanning mode.
 *                                      Each of the selected channels(SDFM_FLTxJCHGR.JCHG) is converted, one after another.
 *                                      The lowest channel (channel 0, if selected) is converted first and the sequence
 *                                      ends at the highest selected channel
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_InjectedConvScanModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JSCAN);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JSCAN);
    }
}

/**
 * @brief  Enable/Disable DMA channel enabled to read data for the injected channel group.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_InjectedConvDmaReadCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_SYNC_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JDMAEN);
    } else {
        CLR_REG32_BIT(*FLTxCR1, SDFM_FLTCR1_JDMAEN);
    }
}

/**
 * @brief  Set injected conversion type.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32InjType             Injected Conversion type.
 *                                      This parameter can be a value of @ref SDFM_Filter_Inj_Type
 * @retval None
 * @note   This register bit can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_SetInjectedConvType(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32InjType)
{
    __IO uint32_t *FLTxCR1;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_INJ_TYPE(u32InjType));

    FLTxCR1 = &SDFM_FLTxCR1(u32Filter);
    MODIFY_REG32(*FLTxCR1, SDFM_FLTCR1_JSCAN_TYPE, u32InjType);
}

/**
 * @brief  Set external trigger for injected conversion.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32ExtTrigger          Trigger signal selection for launching injected conversions.
 *                                      This parameter can be a value of @ref SDFM_Filter_Ext_Trigger
 * @param  [in]  u32ExtTriggerEdge      Trigger enable and trigger edge selection for injected conversions.
 *                                      External trigger edge: rising, falling or both
 *                                      This parameter can be a value of @ref SDFM_Filter_Ext_Trigger_Edge
 * @retval None
 * @note   These register bits can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
void SDFM_FILTER_SetInjectedConvExtTrigger(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter,
                                           uint32_t u32ExtTrigger, uint32_t u32ExtTriggerEdge)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG(u32ExtTrigger));
    DDL_ASSERT(IS_SDFM_INJ_EXT_TRIG_EDGE(u32ExtTriggerEdge));

    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), (SDFM_FLTCR1_JEXTSEL | SDFM_FLTCR1_JEXTEN), \
                 (u32ExtTrigger << SDFM_FLTCR1_JEXTSEL_POS) | u32ExtTriggerEdge);
}

/**
 * @brief  Set channels for injected conversion.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32InjCh               Channels for injected conversion.
 *                                      This parameter can be any combination of @ref SDFM_Channel_Mult
 * @retval None
 * @note   Writing JCHG, if JSCAN=0, resets the channel selection to the lowest selected channel.
 * @note   At least one channel must always be selected for the injected group. Writes causing all JCHG bits to be zero
 *         are ignored.
 */
void SDFM_FILTER_SetInjectedConvCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32InjCh)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_MX_CH(u32InjCh));

    WRITE_REG32(SDFM_FLTxJCHGR(u32Filter), u32InjCh);
}

/**
 * @brief  Software start an injected conversion on the specified SDFM filter.
 *
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval None
 */
void SDFM_FILTER_StartInjectedConv(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    SET_REG32_BIT(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_JSWSTART);
}

/**
 * @brief  Get injected conversion value.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [out] pu32Ch                 Pointer to an uint32_t type address to store corresponding channel of injected conversion.
 *                                      The output value can be a value of @ref SDFM_Channel
 * @retval Injected conversion value
 */
int32_t SDFM_FILTER_GetInjectedConvValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch)
{
    uint32_t u32Reg;
    int32_t i32Value;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    u32Reg = SDFM_FLTxJDATAR(u32Filter);

    /* Extract channel and injected conversion value */
    if (pu32Ch != NULL) {
        *pu32Ch = u32Reg & SDFM_FLTJDATAR_JDATACH;
    }
    /* Injected conversion value is a signed value located on 24 MSB of register */
    /* So after applying a mask on these bits we have to perform a division by 256 (2 raised to the power of 8) */
    u32Reg &= SDFM_FLTJDATAR_JDATA;
    i32Value = (int32_t)u32Reg / 256;

    return i32Value;
}

/***********************************************************************************************************************
***********************************************************************************************************************/
/**
 * @brief  Configures the FIFO of the specified SDFM filter according to the specified parameters
 *         in the structure pointed by pstcFifo.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcFifo               Pointer to a @ref stc_sdfm_filter_fifo_config_t structure value that
 *                                      contains the configuration information for the FIFO of SDFM filter.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcFifo == NULL.
 * @note   Some register bits can be modified only when SDFM_FLT0CR1.DFEN is zero.
 * @note   The FIFO need to be configured only when FIFO injected conversion used(FLTxCR1.JSCAN_TYPE = 1).
 */
int32_t SDFM_FILTER_FifoConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_fifo_config_t *pstcFifo)
{
    if (pstcFifo == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Parameters check */
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    DDL_ASSERT(IS_SDFM_FILTER_FIFO_INT_LEVEL(pstcFifo->u32IntLevel));
    DDL_ASSERT(IS_SDFM_FILTER_FIFO_WAIT_FOR_SYNC(pstcFifo->u32WaitForSync));
    DDL_ASSERT(IS_SDFM_FILTER_FIFO_CLR_ON_SYNC(pstcFifo->u32ClearOnSync));
    DDL_ASSERT(IS_SDFM_FILTER_FIFO_FLAG_CLR_ON_INT(pstcFifo->u32FlagClearOnInt));

    MODIFY_REG32(SDFM_FLTxFIFOCTL(u32Filter), SDFM_FLTFIFOCTL_INIT_MASK, \
                 pstcFifo->u32IntLevel    | \
                 pstcFifo->u32WaitForSync | \
                 pstcFifo->u32ClearOnSync | \
                 pstcFifo->u32FlagClearOnInt);

    return LL_OK;
}

/**
 * @brief  Set each member of @ref stc_sdfm_filter_fifo_config_t to a default value.
 * @param  [in]  pstcFifo               Pointer to a @ref stc_sdfm_filter_fifo_config_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcFifo == NULL.
 */
int32_t SDFM_FILTER_FifoStructInit(stc_sdfm_filter_fifo_config_t *pstcFifo)
{
    if (pstcFifo == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    pstcFifo->u32IntLevel       = 0U;
    pstcFifo->u32WaitForSync    = SDFM_FILTER_FIFO_WAIT_FOR_SYNC_DISABLE;
    pstcFifo->u32ClearOnSync    = SDFM_FILTER_FIFO_CLR_ON_SYNC_DISABLE;
    pstcFifo->u32FlagClearOnInt = SDFM_FILTER_FIFO_FLAG_CLR_ON_INT_DISABLE;

    return LL_OK;
}

/**
 * @brief  Get FIFO status.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval The number of word(s) in the FIFO. 0 indicates the FIFO is empty.
 */
uint32_t SDFM_FILTER_GetFifoStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    return (READ_REG32_BIT(SDFM_FLTxFIFOCTL(u32Filter), SDFM_FLTFIFOCTL_SDFFST) >> SDFM_FLTFIFOCTL_SDFFST_POS);
}

/**
 * @brief  Read FIFO data.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [out] pu32Ch                 Pointer to an uint32_t type address to store corresponding channel of injected conversion.
 *                                      The output value can be a value of @ref SDFM_Channel
 * @retval An uint32_t type value:
 */
int32_t SDFM_FILTER_ReadFifo(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch)
{
    uint32_t u32Reg;
    int32_t i32Value;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    u32Reg = SDFM_FLTxDATFIFO(u32Filter);

    /* Extract channel and injected conversion value */
    if (pu32Ch != NULL) {
        *pu32Ch = u32Reg & SDFM_FLTDATFIFO_FIFO_JDATACH;
    }
    /* Injected conversion value is a signed value located on 24 MSB of register */
    /* So after applying a mask on these bits we have to perform a division by 256 (2 raised to the power of 8) */
    u32Reg &= SDFM_FLTDATFIFO_FIFO_JDATA;
    i32Value = (int32_t)u32Reg / 256;

    return i32Value;
}

/**
 * @brief  Get the status of the FIFO flag bit.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32FifoFlag            FIFO flag bit.
 *                                      This parameter can be a value of @ref SDFM_Filter_Fifo_Flag
 * @retval An en_flag_status_t type value:
 *         - SET: The status of the flag bit is SET.
 *         - RESET: The status of the flag bit is NOT SET.
 */
en_flag_status_t SDFM_FILTER_GetFifoFlagStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FifoFlag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_FIFO_FLAG(u32FifoFlag));

    return (en_flag_status_t)(READ_REG32_BIT(SDFM_FLTxFIFOCTL(u32Filter), u32FifoFlag) != 0U);
}

/**
 * @brief  Clear the status of the FIFO flag bit.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32FifoFlag            FIFO flag bit.
 *                                      This parameter can be a value of @ref SDFM_Filter_Fifo_Flag
 * @retval None
 */
void SDFM_FILTER_ClearFifoFlagStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FifoFlag)
{
    uint32_t u32RegVal;
    __IO uint32_t *FLTxFIFOCTL;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_FIFO_FLAG(u32FifoFlag));

    FLTxFIFOCTL = &SDFM_FLTxFIFOCTL(u32Filter);
    u32RegVal  = READ_REG32(*FLTxFIFOCTL);
    u32RegVal &= ~(SDFM_FILTER_FIFO_FLAG_ALL);
    u32RegVal |= u32FifoFlag;

    WRITE_REG32(*FLTxFIFOCTL, u32RegVal);
}

/***********************************************************************************************************************
***********************************************************************************************************************/
/**
 * @brief  Configures the Cevt of the specified SDFM filter according to the specified parameters
 *         in the structure pointed by pstcCevt.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Cevt                Cevt.
 *                                      This parameter can be a value of @ref SDFM_Filter_Cevt
 * @param  [in]  pstcCevt               Pointer to a @ref stc_sdfm_filter_cevt_config_t structure value that
 *                                      contains the configuration information for the Cevt of SDFM filter.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcCevt == NULL.
 * @note   Some register bits can be modified only when SDFM_FLT0CR1.DFEN is zero.
 */
int32_t SDFM_FILTER_CevtConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Cevt, const stc_sdfm_filter_cevt_config_t *pstcCevt)
{
    uint32_t u32CfgVal;
    uint32_t u32Mask;

    if (pstcCevt == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    /* Parameters check */
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_CEVT(u32Cevt));

    DDL_ASSERT(IS_FUNCTIONAL_STATE(pstcCevt->enIntState));
    DDL_ASSERT(IS_SDFM_FILTER_CEVT_INPUT(pstcCevt->u32InputSel));
    DDL_ASSERT(IS_SDFM_FILTER_CEVT_OUTPUT(pstcCevt->u32OutputSel));
    DDL_ASSERT(IS_SDFM_FILTER_CEVT_DF_SPL_WINDOW(pstcCevt->u32SampleWindow));
    DDL_ASSERT(IS_SDFM_FILTER_CEVT_DF_VOTING_THRESHOLD(pstcCevt->u32VotingThreshold));

    u32CfgVal   = (pstcCevt->u32VotingThreshold << SDFM_FLTEVTFITCTL_THRESH1_POS) | (pstcCevt->u32SampleWindow - 1U);
    u32CfgVal <<= (SDFM_FLTEVTFITCTL_SAMPWIN2_POS * u32Cevt);
    u32Mask     = SDFM_FLTEVTFLTCTL_INIT_MASK << (SDFM_FLTEVTFITCTL_SAMPWIN2_POS * u32Cevt);
    MODIFY_REG32(SDFM_FLTxEVTFITCTL(u32Filter), u32Mask, u32CfgVal);

    u32CfgVal  = (uint32_t)pstcCevt->enIntState << u32Cevt;
    u32Mask    = 1UL << u32Cevt;
    if (u32Cevt == SDFM_FILTER_CEVT1) {
        u32CfgVal |= pstcCevt->u32InputSel << SDFM_FLTCPARM_CEVT1SEL_POS;
        u32CfgVal |= pstcCevt->u32OutputSel << SDFM_FLTCPARM_CEVT1DIGFILTSEL_POS;
        u32Mask |= (SDFM_FLTCPARM_CEVT1SEL | SDFM_FLTCPARM_CEVT1DIGFILTSEL);
    } else {
        u32CfgVal |= pstcCevt->u32InputSel << SDFM_FLTCPARM_CEVT2SEL_POS;
        u32CfgVal |= pstcCevt->u32OutputSel << SDFM_FLTCPARM_CEVT2DIGFILTSEL_POS;
        u32Mask |= (SDFM_FLTCPARM_CEVT2SEL | SDFM_FLTCPARM_CEVT2DIGFILTSEL);
    }
    MODIFY_REG32(SDFM_FLTxCPARM(u32Filter), u32Mask, u32CfgVal);

    return LL_OK;
}

/**
 * @brief  Set each member of @ref stc_sdfm_filter_cevt_config_t to a default value.
 * @param  [in]  pstcCevt               Pointer to a @ref stc_sdfm_filter_cevt_config_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcCevt == NULL.
 */
int32_t SDFM_FILTER_CevtStructInit(stc_sdfm_filter_cevt_config_t *pstcCevt)
{
    if (pstcCevt == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    pstcCevt->enIntState         = DISABLE;
    pstcCevt->u32InputSel        = 0U;
    pstcCevt->u32SampleWindow    = 1U;
    pstcCevt->u32VotingThreshold = 0U;
    pstcCevt->u32OutputSel       = SDFM_FILTER_CEVT_OUTPUT_RAW;

    return LL_OK;
}

/**
 * @brief  Reset the digital filter FIFO of high filter(for CEVT1) and low filter(for CEVT2) to zero.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval None
 */
void SDFM_FILTER_ResetCevtDigitalFifo(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    SET_REG32_BIT(SDFM_FLTxCPARM(u32Filter), SDFM_FLTCPARM_FILINIT);
}

/**
 * @brief  Set the channel that it's COMPL1/2 and COMPH1/2 pass through the digital filter core of CEVT1 and CEVT2.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Ch                  Channel for digital filter.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @retval None
 */
void SDFM_FILTER_CevtDigitalFilterSetInputCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Ch)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_MX_CH(u32Ch));

    MODIFY_REG32(SDFM_FLTxCPARM(u32Filter), SDFM_FLTCPARM_FILTER_CH, u32Ch << SDFM_FLTCPARM_FILTER_CH_POS);
}

/**
 * @brief  Enable/Disable the digital filter core of CEVT1 and CEVT2.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_FILTER_CevtDigitalFilterCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    MODIFY_REG32(SDFM_FLTxCPARM(u32Filter), SDFM_FLTCPARM_CEVFILTER_EN, (uint32_t)enNewState << SDFM_FLTCPARM_CEVFILTER_EN_POS);
}

/**
 * @brief  Enable/Disable channels for extremes detector.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32ExtrCh              Channels for extremes detector.
 *                                      This parameter can be any combination of @ref SDFM_Channel_Mult
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_FILTER_ExtremesChCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32ExtrCh,
                               en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR2;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_MX_CH(u32ExtrCh));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR2 = &SDFM_FLTxCR2(u32Filter);

    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR2, u32ExtrCh << SDFM_FLTCR2_EXCH_POS);
    } else {
        CLR_REG32_BIT(*FLTxCR2, u32ExtrCh << SDFM_FLTCR2_EXCH_POS);
    }
}

/**
 * @brief  Reset(Stop) extreme detector feature.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval None
 */
void SDFM_FILTER_ResetExtremes(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    __IO uint32_t u32Data1;
    __IO uint32_t u32Data2;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    /* Reset channels for extreme detector */
    CLR_REG32_BIT(SDFM_FLTxCR2(u32Filter), SDFM_FLTCR2_EXCH);
    /* Clear extreme detector values */
    u32Data1 = SDFM_FLTxEXMAX(u32Filter);
    u32Data2 = SDFM_FLTxEXMIN(u32Filter);
    (void)u32Data1;
    (void)u32Data2;
}

/**
 * @brief  Get extremes detector maximum value.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [out] pu32Ch                 Corresponding channel of extremes detector maximum value.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @retval Extremes detector maximum value. This value is between -8388608 and 8388607.
 */
int32_t SDFM_FILTER_GetExtremeMaxValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch)
{
    uint32_t u32Reg;
    int32_t i32Value;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    u32Reg = SDFM_FLTxEXMAX(u32Filter);

    /* Extract channel and extremes detector maximum value */
    if (pu32Ch != NULL) {
        *pu32Ch = u32Reg & SDFM_FLTEXMAX_EXMAXCHCH;
    }
    /* Extremes detector maximum value is a signed value located on 24 MSB of register */
    /* So after applying a mask on these bits we have to perform a division by 256 (2 raised to the power of 8) */
    u32Reg &= SDFM_FLTEXMAX_EXMAX;
    i32Value = (int32_t)u32Reg / 256;

    return i32Value;
}

/**
 * @brief  Get extremes detector minimum value.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [out] pu32Ch                 Corresponding channel of extremes detector minimum value.
 *                                      This parameter can be a value of @ref SDFM_Channel
 * @retval Extremes detector minimum value. This value is between -8388608 and 8388607.
 */
int32_t SDFM_FILTER_GetExtremeMinValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch)
{
    uint32_t u32Reg;
    int32_t i32Value;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    u32Reg = SDFM_FLTxEXMIN(u32Filter);

    /* Extract channel and extremes detector minimum value */
    if (pu32Ch != NULL) {
        *pu32Ch = u32Reg & SDFM_FLTEXMIN_EXMINCHCH;
    }
    /* Extremes detector minimum value is a signed value located on 24 MSB of register */
    /* So after applying a mask on these bits we have to perform a division by 256 (2 raised to the power of 8) */
    u32Reg &= SDFM_FLTEXMIN_EXMIN;
    i32Value = (int32_t)u32Reg / 256;

    return i32Value;
}

/**
 * @brief  Get conversion time value.
 *         28-bit timer counting conversion time t = CNVTIMR[27:0] / (fSDFMCLK*CNVTHRE)
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval Conversion time value.
 */
uint32_t SDFM_FILTER_GetConvTimeValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    return (READ_REG32_BIT(SDFM_FLTxCNVTIMR(u32Filter), SDFM_FLTCNVTIMR_CNVCNT) >> SDFM_FLTCNVTIMR_CNVCNT_POS);
}

/**
 * @brief  Get converter thre value.
 * @brief  28-bit timer counting conversion time t = CNVTIMR[27:0] / (fSDFMCLK*CNVTHRE)
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @retval Converter thre value, a value between 1 and 36.
 */
uint32_t SDFM_FILTER_GetConvThreValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    return READ_REG32_BIT(SDFM_FLTxCNVTHRE(u32Filter), SDFM_FLTCNVTHRE_CNVTHRE);
}

/**
 * @brief  Configures filter analog watchdog.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcAwd                Pointer to a @ref stc_sdfm_filter_awd_t structure value that
 *                                      contains the configuration information of the AWD.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAwd == NULL.
 */
int32_t SDFM_FILTER_AwdConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_awd_t *pstcAwd)
{
    int32_t i32HighThreshold;
    int32_t i32LowThreshold;

    if (pstcAwd == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    i32HighThreshold = pstcAwd->i32HighThreshold;
    i32LowThreshold  = pstcAwd->i32LowThreshold;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_DATA_SRC(pstcAwd->u32DataSrc));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_THRESHOLD(i32HighThreshold));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_THRESHOLD(i32LowThreshold));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(pstcAwd->u32HighBreakSignal));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(pstcAwd->u32LowBreakSignal));

    if (pstcAwd->u32DataSrc == SDFM_FILTER_AWD_CH_DATA) {
        i32HighThreshold *= 256;
        i32LowThreshold  *= 256;
    }
    /* Set analog watchdog data source */
    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_AWFSEL, pstcAwd->u32DataSrc);
    /* Set thresholds and break signals */
    MODIFY_REG32(SDFM_FLTxAWHTR(u32Filter), (SDFM_FLTAWHTR_AWHT | SDFM_FLTAWHTR_BKAWH), \
                 ((uint32_t)i32HighThreshold << SDFM_FLTAWHTR_AWHT_POS) | pstcAwd->u32HighBreakSignal);
    MODIFY_REG32(SDFM_FLTxAWLTR(u32Filter), (SDFM_FLTAWLTR_AWLT | SDFM_FLTAWLTR_BKAWL), \
                 ((uint32_t)i32LowThreshold << SDFM_FLTAWLTR_AWLT_POS) | pstcAwd->u32LowBreakSignal);

    /* Set analog watchdog thresholds2 */
    WRITE_REG32(SDFM_FLTxAWHTR2(u32Filter), ((uint32_t)pstcAwd->i16HighThreshold2 << SDFM_FLTAWHT2_AWHT2_POS));
    WRITE_REG32(SDFM_FLTxAWLTR2(u32Filter), ((uint32_t)pstcAwd->i16LowThreshold2 << SDFM_FLTAWLT2_AWLT2_POS));

    /* Break signal assignment to cross zero event.
       In case of input channels monitoring (AWFSEL=1), the data for comparison to threshold is taken from channels
       selected by AWDCH[7:0] field (SDFM_FLTxCR2 register). Each of the selected channels filter result is compared
       to zero, generate cross zero break. */
    MODIFY_REG32(SDFM_FLTxCPARM(u32Filter), SDFM_FLTCPARM_BK_CROSS_ZERO, pstcAwd->u32CrossZeroBreakSignal << SDFM_FLTCPARM_BK_CROSS_ZERO_POS);

    return LL_OK;
}

/**
 * @brief  Set each member of @ref stc_sdfm_filter_awd_t to a default value.
 * @param  [in]  pstcAwd                Pointer to a @ref stc_sdfm_filter_awd_t structure
 *                                      whose members will be set to default values.
 * @retval int32_t:
 *           - LL_OK:                   No error occurred.
 *           - LL_ERR_INVD_PARAM:       pstcAwd == NULL.
 */
int32_t SDFM_FILTER_AwdStructInit(stc_sdfm_filter_awd_t *pstcAwd)
{
    if (pstcAwd == NULL) {
        return LL_ERR_INVD_PARAM;
    }
    pstcAwd->u32DataSrc              = SDFM_FILTER_AWD_FILTER_DATA;
    pstcAwd->i32HighThreshold        = 0;
    pstcAwd->i32LowThreshold         = 0;
    pstcAwd->i16HighThreshold2       = 0;
    pstcAwd->i16LowThreshold2        = 0;
    pstcAwd->u32HighBreakSignal      = SDFM_BREAK_SIGNAL_NONE;
    pstcAwd->u32LowBreakSignal       = SDFM_BREAK_SIGNAL_NONE;
    pstcAwd->u32CrossZeroBreakSignal = SDFM_BREAK_SIGNAL_NONE;

    return LL_OK;
}

/**
 * @brief  Enable/Disable channels for analog watchdog.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32AwdCh               Channels for analog watchdog.
 *                                      This parameter can be any combination of @ref SDFM_Channel_Mult
 * @param  [in]  enNewState             An @ref en_functional_state_t enumeration value.
 * @retval None
 */
void SDFM_FILTER_AwdChCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32AwdCh, en_functional_state_t enNewState)
{
    __IO uint32_t *FLTxCR2;

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_MX_CH(u32AwdCh));
    DDL_ASSERT(IS_FUNCTIONAL_STATE(enNewState));

    FLTxCR2 = &SDFM_FLTxCR2(u32Filter);

    if (enNewState == ENABLE) {
        SET_REG32_BIT(*FLTxCR2, u32AwdCh << SDFM_FLTCR2_AWDCH_POS);
    } else {
        CLR_REG32_BIT(*FLTxCR2, u32AwdCh << SDFM_FLTCR2_AWDCH_POS);
    }
}

/**
 * @brief  Set analog watchdog data source.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32DataSrc             Analog watchdog data source.
 *                                      Values from digital filter or from channel watchdog filter.
 *                                      This parameter can be a value of @ref SDFM_Filter_Awd_Data_Src
 * @retval None
 */
void SDFM_FILTER_SetAwdDataSrc(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32DataSrc)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_DATA_SRC(u32DataSrc));

    /* Set analog watchdog data source */
    MODIFY_REG32(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_AWFSEL, u32DataSrc);
}

/**
 * @brief Set analog watchdog high/low threshold.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  i32HighThreshold       Analog watchdog high threshold.
 *                                      This parameter must be a number between -8388608 and 8388607
 * @param  [in]  i32LowThreshold        Analog watchdog low threshold.
 *                                      This parameter must be a number between -8388608 and 8388607
 * @retval None
 * @note Call this function after calling SDFM_FILTER_SetAwdDataSrc().
 */
void SDFM_FILTER_SetAwdThreshold(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, int32_t i32HighThreshold, int32_t i32LowThreshold)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_THRESHOLD(i32HighThreshold));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_THRESHOLD(i32LowThreshold));

    if (READ_REG32_BIT(SDFM_FLTxCR1(u32Filter), SDFM_FLTCR1_AWFSEL) == SDFM_FILTER_AWD_CH_DATA) {
        i32HighThreshold *= 256;
        i32LowThreshold  *= 256;
    }
    /* Set thresholds */
    MODIFY_REG32(SDFM_FLTxAWHTR(u32Filter), SDFM_FLTAWHTR_AWHT, ((uint32_t)i32HighThreshold << SDFM_FLTAWHTR_AWHT_POS));
    MODIFY_REG32(SDFM_FLTxAWLTR(u32Filter), SDFM_FLTAWLTR_AWLT, ((uint32_t)i32LowThreshold << SDFM_FLTAWLTR_AWLT_POS));
}

/**
 * @brief Set analog watchdog high/low threshold2.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  i16HighThreshold2      Analog watchdog high threshold2.
 *                                      This parameter must be a number between -32768 and 32767
 * @param  [in]  i16LowThreshold2       Analog watchdog low threshold2.
 *                                      This parameter must be a number between -32768 and 32767
 * @retval None
 */
void SDFM_FILTER_SetAwdThreshold2(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, int16_t i16HighThreshold2, int16_t i16LowThreshold2)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));

    /* Set analog watchdog thresholds2 */
    WRITE_REG32(SDFM_FLTxAWHTR2(u32Filter), ((uint32_t)i16HighThreshold2 << SDFM_FLTAWHT2_AWHT2_POS));
    WRITE_REG32(SDFM_FLTxAWLTR2(u32Filter), ((uint32_t)i16LowThreshold2 << SDFM_FLTAWLT2_AWLT2_POS));
}

/**
 * @brief Set break signal assignment to analog watchdog high/low threshold event.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32HighBreakSignal     Break signal assignment to analog watchdog high threshold event.
 *                                      This parameter can be any combination of @ref SDFM_Break_Signal
 * @param  [in]  u32LowBreakSignal      Break signal assignment to analog watchdog low threshold event.
 *                                      This parameter can be any combination of @ref SDFM_Break_Signal
 * @param  [in]  u32CrossZeroBreakSignal Break signal assignment to cross zero break event.
 *                                      This parameter can be any combination of @ref SDFM_Break_Signal
 * @retval None
 */
void SDFM_FILTER_SetAwdBreakSignal(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter,
                                   uint32_t u32HighBreakSignal, uint32_t u32LowBreakSignal, uint32_t u32CrossZeroBreakSignal)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(u32HighBreakSignal));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(u32LowBreakSignal));
    DDL_ASSERT(IS_SDFM_BREAK_SIGNAL(u32CrossZeroBreakSignal));

    /* Set break signals */
    MODIFY_REG32(SDFM_FLTxAWHTR(u32Filter), SDFM_FLTAWHTR_BKAWH, u32HighBreakSignal);
    MODIFY_REG32(SDFM_FLTxAWLTR(u32Filter), SDFM_FLTAWLTR_BKAWL, u32LowBreakSignal);
    MODIFY_REG32(SDFM_FLTxCPARM(u32Filter), SDFM_FLTCPARM_BK_CROSS_ZERO, u32CrossZeroBreakSignal << SDFM_FLTCPARM_BK_CROSS_ZERO_POS);
}

/**
 * @brief Get the status of the specified AWD flag of the specified filter.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Flag                Filter AWD flag.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Awd_Status_Flag
 * @retval An @ref en_flag_status_t enumeration type value.
 */
en_flag_status_t SDFM_FILTER_GetAwdStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_FLAG(u32Flag));

    /* Return SET if one of the specified flags is SET. */
    return (en_flag_status_t)(READ_REG32_BIT(SDFM_FLTxAWSR(u32Filter), u32Flag) != 0U);
}

/**
 * @brief Clear the specified AWD flag of the specified filter.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  u32Flag                Filter AWD flag.
 *                                      This parameter can be any combination of @ref SDFM_Filter_Awd_Status_Flag
 * @retval None
 */
void SDFM_FILTER_ClearAwdStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag)
{
    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_AWD_FLAG(u32Flag));

    /* Clear the selected flags */
    WRITE_REG32(SDFM_FLTxAWCFR(u32Filter), u32Flag);
}

/**
 * @brief  Configures sinc filter.
 * @param  [in]  SDFMx                  Pointer to SDFM instance register base.
 *                                      This parameter can be a value of the following:
 *   @arg  CM_SDFM or CM_SDFMx:         SDFM instance register base.
 * @param  [in]  u32Filter              SDFM filter.
 *                                      This parameter can be a value of @ref SDFM_Filter
 * @param  [in]  pstcSinc               Pointer to a @ref stc_sdfm_filter_sinc_t structure value that
 *                                      contains the configuration information of the control register.
 * @retval int32_t:
 *           - LL_OK:                   No errors occurred.
 *           - LL_ERR_INVD_PARAM:       pstcSinc == NULL.
 */
int32_t SDFM_FILTER_SincConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_sinc_t *pstcSinc)
{
    if (pstcSinc == NULL) {
        return LL_ERR_INVD_PARAM;
    }

    DDL_ASSERT(IS_SDFM(SDFMx));
    DDL_ASSERT(IS_SDFM_FILTER(u32Filter));
    DDL_ASSERT(IS_SDFM_FILTER_SINC_FOSR_ORDER(pstcSinc->u32SincFOSR, pstcSinc->u32SincOrder));
    DDL_ASSERT(IS_SDFM_FILTER_SINC_IOSR(pstcSinc->u32SincIOSR));

    /* Configure filter */
    WRITE_REG32(SDFM_FLTxFCR(u32Filter), pstcSinc->u32SincOrder | \
                ((pstcSinc->u32SincFOSR - 1U) << SDFM_FLTFCR_FOSR_POS) | \
                (pstcSinc->u32SincIOSR - 1U));

    return LL_OK;
}

/**
 * @}
 */

#endif /* LL_SDFM_ENABLE */

/**
 * @}
 */

/**
 * @}
 */
/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/

