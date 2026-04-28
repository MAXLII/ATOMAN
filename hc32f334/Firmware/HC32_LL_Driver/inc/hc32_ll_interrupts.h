/**
 *******************************************************************************
 * @file  hc32_ll_interrupts.h
 * @brief This file contains all the functions prototypes of the interrupt driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
   2024-06-30       CDT             Modify INTC filter B macros correspond with RM
   2025-11-03       CDT             Modify macro group: INT_Channel_Sel
                                    Modify macro group: INTC_Event_Channel_Sel
                                    Modify macro group: SWINT_Channel_Sel
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
#ifndef __HC32_LL_INTERRUPTS_H__
#define __HC32_LL_INTERRUPTS_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"

#include "hc32f3xx.h"
#include "hc32f3xx_conf.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_INTERRUPTS
 * @{
 */

#if (LL_INTERRUPTS_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup INTC_Global_Types INTC Global Types
 * @{
 */

/**
 * @brief  Interrupt registration structure definition
 */
typedef struct {
    en_int_src_t enIntSrc;  /*!< Peripheral interrupt number, can be any value @ref en_int_src_t    */
    IRQn_Type enIRQn;       /*!< Peripheral IRQ type, can be INT000_IRQn~INT127_IRQn @ref IRQn_Type */
    func_ptr_t pfnCallback; /*!< Callback function for corresponding peripheral IRQ                 */
} stc_irq_signin_config_t;

/**
 * @brief  NMI initialize configuration structure definition
 */
typedef struct {
    uint32_t u32Src;            /*!< NMI trigger source, @ref NMI_TriggerSrc_Sel for details */
} stc_nmi_init_t;

/**
 * @brief  EXTINT initialize configuration structure definition
 */
typedef struct {
    uint32_t u32Filter;         /*!< ExtInt filter (A) function setting, @ref EXTINT_FilterClock_Sel for details */
    uint32_t u32FilterClock;    /*!< ExtInt filter (A) clock division, @ref EXTINT_FilterClock_Div for details */
    uint32_t u32Edge;           /*!< ExtInt trigger edge, @ref EXTINT_Trigger_Sel for details */
    uint32_t u32FilterB;        /*!< ExtInt filter B function setting, @ref EXTINT_FilterBClock_Sel for details */
    uint32_t u32FilterBClock;   /*!< ExtInt filter B time, @ref EXTINT_FilterBTim_Sel for details */
} stc_extint_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup INTC_Global_Macros INTC Global Macros
 * @{
 */
/**
 * @defgroup INTC_Priority_Sel Interrupt Priority Level
 * @{
 */
#define DDL_IRQ_PRIO_00                 (0U)
#define DDL_IRQ_PRIO_01                 (1U)
#define DDL_IRQ_PRIO_02                 (2U)
#define DDL_IRQ_PRIO_03                 (3U)
#define DDL_IRQ_PRIO_04                 (4U)
#define DDL_IRQ_PRIO_05                 (5U)
#define DDL_IRQ_PRIO_06                 (6U)
#define DDL_IRQ_PRIO_07                 (7U)
#define DDL_IRQ_PRIO_08                 (8U)
#define DDL_IRQ_PRIO_09                 (9U)
#define DDL_IRQ_PRIO_10                 (10U)
#define DDL_IRQ_PRIO_11                 (11U)
#define DDL_IRQ_PRIO_12                 (12U)
#define DDL_IRQ_PRIO_13                 (13U)
#define DDL_IRQ_PRIO_14                 (14U)
#define DDL_IRQ_PRIO_15                 (15U)

#define DDL_IRQ_PRIO_DEFAULT            (DDL_IRQ_PRIO_15)

/**
 * @}
 */

/**
 * @defgroup NMI_TriggerSrc_Sel NMI Trigger Source Selection
 * @{
 */
#define NMI_SRC_SWDT                    (INTC_NMIFR_SWDTF)
#define NMI_SRC_LVD1                    (INTC_NMIFR_PVD1F)
#define NMI_SRC_LVD2                    (INTC_NMIFR_PVD2F)
#define NMI_SRC_XTAL                    (INTC_NMIFR_XTALSTPF)
#define NMI_SRC_SRAM_PARITY             (INTC_NMIFR_RPARERRF)
#define NMI_SRC_SRAM_ECC                (INTC_NMIFR_RECCERRF)
#define NMI_SRC_BUS_ERR                 (INTC_NMIFR_BUSERRF)
#define NMI_SRC_WDT                     (INTC_NMIFR_WDTF)
#define NMI_SRC_ALL                     (NMI_SRC_SWDT   | NMI_SRC_LVD1      | NMI_SRC_LVD2          |   \
                                        NMI_SRC_XTAL    | NMI_SRC_BUS_ERR   | NMI_SRC_SRAM_PARITY   |   \
                                        NMI_SRC_WDT     | NMI_SRC_SRAM_ECC)

/**
 * @}
 */

/**
 * @defgroup EXTINT_Channel_Sel External Interrupt Channel Selection
 * @{
 */
#define EXTINT_CH00                     (1UL << 0U)
#define EXTINT_CH01                     (1UL << 1U)
#define EXTINT_CH02                     (1UL << 2U)
#define EXTINT_CH03                     (1UL << 3U)
#define EXTINT_CH04                     (1UL << 4U)
#define EXTINT_CH05                     (1UL << 5U)
#define EXTINT_CH06                     (1UL << 6U)
#define EXTINT_CH07                     (1UL << 7U)
#define EXTINT_CH08                     (1UL << 8U)
#define EXTINT_CH09                     (1UL << 9U)
#define EXTINT_CH10                     (1UL << 10U)
#define EXTINT_CH11                     (1UL << 11U)
#define EXTINT_CH12                     (1UL << 12U)
#define EXTINT_CH13                     (1UL << 13U)
#define EXTINT_CH14                     (1UL << 14U)
#define EXTINT_CH15                     (1UL << 15U)
#define EXTINT_CH_ALL                   (EXTINT_CH00 | EXTINT_CH01 | EXTINT_CH02 | EXTINT_CH03 |    \
                                         EXTINT_CH04 | EXTINT_CH05 | EXTINT_CH06 | EXTINT_CH07 |    \
                                         EXTINT_CH08 | EXTINT_CH09 | EXTINT_CH10 | EXTINT_CH11 |    \
                                         EXTINT_CH12 | EXTINT_CH13 | EXTINT_CH14 | EXTINT_CH15)
/**
 * @}
 */

/**
 * @defgroup INT_Channel_Sel Interrupt Channel Selection
 * @{
 */
#define INTC_INT0                       (1UL << 0U)
#define INTC_INT1                       (1UL << 1U)
#define INTC_INT2                       (1UL << 2U)
#define INTC_INT3                       (1UL << 3U)
#define INTC_INT4                       (1UL << 4U)
#define INTC_INT5                       (1UL << 5U)
#define INTC_INT6                       (1UL << 6U)
#define INTC_INT7                       (1UL << 7U)
#define INTC_INT8                       (1UL << 8U)
#define INTC_INT9                       (1UL << 9U)
#define INTC_INT10                      (1UL << 10U)
#define INTC_INT11                      (1UL << 11U)
#define INTC_INT12                      (1UL << 12U)
#define INTC_INT13                      (1UL << 13U)
#define INTC_INT14                      (1UL << 14U)
#define INTC_INT15                      (1UL << 15U)
#define INTC_INT16                      (1UL << 16U)
#define INTC_INT17                      (1UL << 17U)
#define INTC_INT18                      (1UL << 18U)
#define INTC_INT19                      (1UL << 19U)
#define INTC_INT20                      (1UL << 20U)
#define INTC_INT21                      (1UL << 21U)
#define INTC_INT22                      (1UL << 22U)
#define INTC_INT23                      (1UL << 23U)
#define INTC_INT24                      (1UL << 24U)
#define INTC_INT25                      (1UL << 25U)
#define INTC_INT26                      (1UL << 26U)
#define INTC_INT27                      (1UL << 27U)
#define INTC_INT28                      (1UL << 28U)
#define INTC_INT29                      (1UL << 29U)
#define INTC_INT30                      (1UL << 30U)
#define INTC_INT31                      (1UL << 31U)
#define INTC_INT_ALL                    (0xFFFFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup INTC_Event_Channel_Sel Event Channel Selection
 * @{
 */
#define INTC_EVT0                       (1UL << 0U)
#define INTC_EVT1                       (1UL << 1U)
#define INTC_EVT2                       (1UL << 2U)
#define INTC_EVT3                       (1UL << 3U)
#define INTC_EVT4                       (1UL << 4U)
#define INTC_EVT5                       (1UL << 5U)
#define INTC_EVT6                       (1UL << 6U)
#define INTC_EVT7                       (1UL << 7U)
#define INTC_EVT8                       (1UL << 8U)
#define INTC_EVT9                       (1UL << 9U)
#define INTC_EVT10                      (1UL << 10U)
#define INTC_EVT11                      (1UL << 11U)
#define INTC_EVT12                      (1UL << 12U)
#define INTC_EVT13                      (1UL << 13U)
#define INTC_EVT14                      (1UL << 14U)
#define INTC_EVT15                      (1UL << 15U)
#define INTC_EVT16                      (1UL << 16U)
#define INTC_EVT17                      (1UL << 17U)
#define INTC_EVT18                      (1UL << 18U)
#define INTC_EVT19                      (1UL << 19U)
#define INTC_EVT20                      (1UL << 20U)
#define INTC_EVT21                      (1UL << 21U)
#define INTC_EVT22                      (1UL << 22U)
#define INTC_EVT23                      (1UL << 23U)
#define INTC_EVT24                      (1UL << 24U)
#define INTC_EVT25                      (1UL << 25U)
#define INTC_EVT26                      (1UL << 26U)
#define INTC_EVT27                      (1UL << 27U)
#define INTC_EVT28                      (1UL << 28U)
#define INTC_EVT29                      (1UL << 29U)
#define INTC_EVT30                      (1UL << 30U)
#define INTC_EVT31                      (1UL << 31U)
#define INTC_EVT_ALL                    (0xFFFFFFFFUL)

/**
 * @}
 */

/**
 * @defgroup SWINT_Channel_Sel Software Interrupt Channel Selection
 * @{
 */
#define SWINT_CH00                      (1UL << 0U)
#define SWINT_CH01                      (1UL << 1U)
#define SWINT_CH02                      (1UL << 2U)
#define SWINT_CH03                      (1UL << 3U)
#define SWINT_CH04                      (1UL << 4U)
#define SWINT_CH05                      (1UL << 5U)
#define SWINT_CH06                      (1UL << 6U)
#define SWINT_CH07                      (1UL << 7U)
#define SWINT_CH08                      (1UL << 8U)
#define SWINT_CH09                      (1UL << 9U)
#define SWINT_CH10                      (1UL << 10U)
#define SWINT_CH11                      (1UL << 11U)
#define SWINT_CH12                      (1UL << 12U)
#define SWINT_CH13                      (1UL << 13U)
#define SWINT_CH14                      (1UL << 14U)
#define SWINT_CH15                      (1UL << 15U)
#define SWINT_CH16                      (1UL << 16U)
#define SWINT_CH17                      (1UL << 17U)
#define SWINT_CH18                      (1UL << 18U)
#define SWINT_CH19                      (1UL << 19U)
#define SWINT_CH20                      (1UL << 20U)
#define SWINT_CH21                      (1UL << 21U)
#define SWINT_CH22                      (1UL << 22U)
#define SWINT_CH23                      (1UL << 23U)
#define SWINT_CH24                      (1UL << 24U)
#define SWINT_CH25                      (1UL << 25U)
#define SWINT_CH26                      (1UL << 26U)
#define SWINT_CH27                      (1UL << 27U)
#define SWINT_CH28                      (1UL << 28U)
#define SWINT_CH29                      (1UL << 29U)
#define SWINT_CH30                      (1UL << 30U)
#define SWINT_CH31                      (1UL << 31U)
#define SWINT_ALL                       (0xFFFFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup EXTINT_FilterClock_Sel External Interrupt Filter A Function Selection
 * @{
 */
#define EXTINT_FILTER_OFF               (0UL)
#define EXTINT_FILTER_ON                (INTC_EIRQCR_EFEN)

/**
 * @}
 */

/**
 * @defgroup EXTINT_FilterBClock_Sel External Interrupt Filter B Function Selection
 * @{
 */
#define EXTINT_FILTER_B_OFF             (0UL)
#define EXTINT_FILTER_B_ON              (INTC_EIRQCR_NOCEN)
/**
 * @}
 */

/**
 * @defgroup EXTINT_FilterClock_Div External Interrupt Filter A Sampling Clock Division Selection
 * @{
 */
#define EXTINT_FCLK_DIV1                (0UL)
#define EXTINT_FCLK_DIV8                (INTC_EIRQCR_EISMPCLK_0)
#define EXTINT_FCLK_DIV32               (INTC_EIRQCR_EISMPCLK_1)
#define EXTINT_FCLK_DIV64               (INTC_EIRQCR_EISMPCLK)

/**
 * @}
 */

/**
 * @defgroup EXTINT_FilterBTim_Sel External Interrupt Filter B Time Selection
 * @{
 */
#define EXTINT_FILTER_B_LVL1            (0UL)
#define EXTINT_FILTER_B_LVL2            (INTC_EIRQCR_NOCSEL_0)
#define EXTINT_FILTER_B_LVL3            (INTC_EIRQCR_NOCSEL_1)
#define EXTINT_FILTER_B_LVL4            (INTC_EIRQCR_NOCSEL)
/**
 * @}
 */

/**
 * @defgroup EXTINT_Trigger_Sel External Interrupt Trigger Edge Selection
 * @{
 */
#define EXTINT_TRIG_FALLING             (0UL)
#define EXTINT_TRIG_RISING              (INTC_EIRQCR_EIRQTRG_0)
#define EXTINT_TRIG_BOTH                (INTC_EIRQCR_EIRQTRG_1)
#define EXTINT_TRIG_LOW                 (INTC_EIRQCR_EIRQTRG)

/**
 * @}
 */

/**
 * @defgroup INTC_Stop_Wakeup_Source_Sel Stop Mode Wakeup Source Selection
 * @{
 */
#define INTC_STOP_WKUP_EXTINT_CH0       INTC_WKEN_EIRQWKEN_0
#define INTC_STOP_WKUP_EXTINT_CH1       INTC_WKEN_EIRQWKEN_1
#define INTC_STOP_WKUP_EXTINT_CH2       INTC_WKEN_EIRQWKEN_2
#define INTC_STOP_WKUP_EXTINT_CH3       INTC_WKEN_EIRQWKEN_3
#define INTC_STOP_WKUP_EXTINT_CH4       INTC_WKEN_EIRQWKEN_4
#define INTC_STOP_WKUP_EXTINT_CH5       INTC_WKEN_EIRQWKEN_5
#define INTC_STOP_WKUP_EXTINT_CH6       INTC_WKEN_EIRQWKEN_6
#define INTC_STOP_WKUP_EXTINT_CH7       INTC_WKEN_EIRQWKEN_7
#define INTC_STOP_WKUP_EXTINT_CH8       INTC_WKEN_EIRQWKEN_8
#define INTC_STOP_WKUP_EXTINT_CH9       INTC_WKEN_EIRQWKEN_9
#define INTC_STOP_WKUP_EXTINT_CH10      INTC_WKEN_EIRQWKEN_10
#define INTC_STOP_WKUP_EXTINT_CH11      INTC_WKEN_EIRQWKEN_11
#define INTC_STOP_WKUP_EXTINT_CH12      INTC_WKEN_EIRQWKEN_12
#define INTC_STOP_WKUP_EXTINT_CH13      INTC_WKEN_EIRQWKEN_13
#define INTC_STOP_WKUP_EXTINT_CH14      INTC_WKEN_EIRQWKEN_14
#define INTC_STOP_WKUP_EXTINT_CH15      INTC_WKEN_EIRQWKEN_15
#define INTC_STOP_WKUP_SWDT             INTC_WKEN_SWDTWKEN
#define INTC_STOP_WKUP_CMP1             INTC_WKEN_CMP1WKEN
#define INTC_STOP_WKUP_WKTM             INTC_WKEN_WKTMWKEN
#define INTC_STOP_WKUP_RTC_ALM          INTC_WKEN_RTCALMWKEN
#define INTC_STOP_WKUP_RTC_PRD          INTC_WKEN_RTCPRDWKEN
#define INTC_STOP_WKUP_TMR0_CMP         INTC_WKEN_TMR0CMPWKEN
#define INTC_STOP_WKUP_USART1_RX        INTC_WKEN_RXWKEN
#define INTC_STOP_WKUP_CMP2             INTC_WKEN_CMP2WKEN
#define INTC_STOP_WKUP_CMP3             INTC_WKEN_CMP3WKEN
#define INTC_WUPEN_ALL                  (INTC_WKEN_EIRQWKEN    | INTC_WKEN_SWDTWKEN           |           \
                                         INTC_WKEN_CMP1WKEN    | INTC_WKEN_WKTMWKEN           |           \
                                         INTC_WKEN_RTCALMWKEN  | INTC_WKEN_RTCPRDWKEN         |           \
                                         INTC_WKEN_TMR0CMPWKEN | INTC_WKEN_RXWKEN             |           \
                                         INTC_WKEN_CMP2WKEN    | INTC_WKEN_CMP3WKEN)
/**
 * @}
 */

/**
 * @defgroup INTC_FPU_Interrupt_Selection INTC FPU Interrupt selection
 * @{
 */
#define INTC_FPU_IOCI                   (INTC_FPUIER_IOCIEN)    /*!< Invalid operations */
#define INTC_FPU_DZCI                   (INTC_FPUIER_DZCIEN)    /*!< Divide zero        */
#define INTC_FPU_OFCI                   (INTC_FPUIER_OFCIEN)    /*!< Over flow          */
#define INTC_FPU_UFCI                   (INTC_FPUIER_UFCIEN)    /*!< Under flow         */
#define INTC_FPU_IXCI                   (INTC_FPUIER_IXCIEN)    /*!< Inexact result     */
#define INTC_FPU_IDCI                   (INTC_FPUIER_IDCIEN)    /*!< Input non-standard */
#define INTC_FPU_ALL                    (0x3FUL)
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
 * @addtogroup INTC_Global_Functions
 * @{
 */

int32_t INTC_IrqSignIn(const stc_irq_signin_config_t *pstcIrqSignConfig);
int32_t INTC_IrqSignOut(IRQn_Type enIRQn);
int32_t INTC_IrqInstallHandle(IRQn_Type enIRQn, en_int_src_t enIntSrc, uint16_t u16Prio, func_ptr_t pfnCallback);
void INTC_WakeupSrcCmd(uint32_t u32WakeupSrc, en_functional_state_t enNewState);
void INTC_EventCmd(uint32_t u32Event, en_functional_state_t enNewState);
void INTC_IntCmd(uint32_t u32Int, en_functional_state_t enNewState);
void INTC_SWIntInit(uint32_t u32Ch, const func_ptr_t pfnCallback, uint32_t u32Priority);
void INTC_SWIntCmd(uint32_t u32SWInt, en_functional_state_t enNewState);

int32_t NMI_Init(const stc_nmi_init_t *pstcNmiInit);
int32_t NMI_StructInit(stc_nmi_init_t *pstcNmiInit);
en_flag_status_t NMI_GetNmiStatus(uint32_t u32Src);
void NMI_NmiSrcCmd(uint32_t u32Src, en_functional_state_t enNewState);
void NMI_ClearNmiStatus(uint32_t u32Src);

int32_t EXTINT_Init(uint32_t u32Ch, const stc_extint_init_t *pstcExtIntInit);
int32_t EXTINT_StructInit(stc_extint_init_t *pstcExtIntInit);
en_flag_status_t EXTINT_GetExtIntStatus(uint32_t u32ExtIntCh);
void EXTINT_ClearExtIntStatus(uint32_t u32ExtIntCh);
void INTC_IntSrcCmd(en_int_src_t enIntSrc, en_functional_state_t enNewState);
en_functional_state_t INTC_GetIntSrcState(en_int_src_t enIntSrc);

void INTC_FPU_IntCmd(uint32_t u32FpuInt, en_functional_state_t enNewState);

void IRQ000_Handler(void);
void IRQ001_Handler(void);
void IRQ002_Handler(void);
void IRQ003_Handler(void);
void IRQ004_Handler(void);
void IRQ005_Handler(void);
void IRQ006_Handler(void);
void IRQ007_Handler(void);

void IRQ008_Handler(void);
void IRQ009_Handler(void);
void IRQ010_Handler(void);
void IRQ011_Handler(void);
void IRQ012_Handler(void);
void IRQ013_Handler(void);
void IRQ014_Handler(void);
void IRQ015_Handler(void);

/**
 * @}
 */

#endif /* LL_INTERRUPTS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_INTERRUPTS_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
