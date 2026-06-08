/**
 *******************************************************************************
 * @file  hc32_ll_dac.h
 * @brief This file contains all the functions prototypes of the DAC driver
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
#ifndef __HC32_LL_DAC_H__
#define __HC32_LL_DAC_H__

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
 * @addtogroup LL_DAC
 * @{
 */

#if (LL_DAC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup DAC_Global_Types DAC Global Types
 * @{
 */

/**
 * @brief Structure definition of DAC initialization.
 */
typedef struct {
    uint16_t u16Align;               /*!< Specify the data alignment
                                     This parameter can be a value of @ref DAC_DATAREG_ALIGN_PATTERN */
    en_functional_state_t enOutput;  /*!< Enable or disable analog output
                                     This parameter can be a value of @ref en_functional_state_t */
} stc_dac_init_t;

/**
 * @brief Structure definition of DAC ramp initialization.
 */
typedef struct {
    uint16_t u16Dir;                /*!< Direct of ramp counter
                                     This parameter can be a value of @ref DAC_RAMP_CNT_DIR */
    uint16_t u16RampStep;           /*!< Step of ramp for incremental/decreasing */

    uint16_t u16RampLimit;          /*!< Upper/Lower limit of ramp count */

    uint16_t u16RampResetDelay;     /*!< Value of ramp reset delay */

    uint32_t u32RampStepSelect;     /*!< Trigger event of ramp step select
                                     This parameter can be a value of @ref DAC_RAMP_STEP_EVT */
    uint32_t u32RampResetSelect;    /*!< Trigger event of ramp reset select
                                     This parameter can be a value of @ref DAC_RAMP_RST_EVT */
    uint32_t u32RampState;          /*!< Enable or disable ramp
                                     This parameter can be a value of @ref DAC_RAMP_STATE */
    uint16_t u16CmpCtrl;            /*!< Enable or disable cmp invert control
                                     This parameter can be a value of @ref DAC_RAMP_CMP_CTRL */
} stc_dac_ramp_init_t;

/**
 * @brief Structure definition of DAC diode initialization.
 */
typedef struct {
    uint32_t u32DiodeTransSelect;   /*!< Trigger event of diode data transfer select
                                     This parameter can be a value of @ref DAC_DIODE_TRANS_EVT */
    uint16_t u16DiodeState;         /*!< Enable or disable diode
                                     This parameter can be a value of @ref DAC_DIODE_STATE */
    uint16_t u16DiodeValue;         /*!< Upper/Lower limit of ramp count */
} stc_dac_diode_init_t;
/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup DAC_Global_Macros DAC Global Macros
 * @{
 */

/**
 * @defgroup DAC_CH DAC channel
 * @{
 */
#define DAC_CH1                             (0U)
#define DAC_CH2                             (1U)
/**
 * @}
 */

/**
 * @defgroup DAC_DATAREG_ALIGN_PATTERN DAC data register alignment pattern
 * @{
 */
#define DAC_DATA_ALIGN_LEFT                 (DAC_DACR_DPSEL)
#define DAC_DATA_ALIGN_RIGHT                (0U)
/**
 * @}
 */

/**
 * @defgroup DAC_RESOLUTION DAC resolution
 * @{
 */
#define DAC_RESOLUTION_12BIT                (12U)
/**
 * @}
 */

/**
 * @defgroup DAC_ADP_SELECT DAC ADCx priority select
 * @{
 */
#define DAC_ADP_SEL_ADC1                    (DAC_DAADPCR_ADCSL1)
#define DAC_ADP_SEL_ADC2                    (DAC_DAADPCR_ADCSL2)
#define DAC_ADP_SEL_ADC3                    (DAC_DAADPCR_ADCSL3)

#define DAC_ADP_SEL_ADC4                    (DAC_DAADPCR_ADCSL4)
#define DAC_ADP_SEL_ALL                     (DAC_DAADPCR_ADCSL1 | DAC_DAADPCR_ADCSL2 | DAC_DAADPCR_ADCSL3 | DAC_DAADPCR_ADCSL4)
/**
 * @}
 */

/**
 * @defgroup DAC_CH_DATA_TRANS_MD DAC Channel Data Transfer Mode
 * @{
 */
#define DAC_CH_DATA_TRANS_NORMAL            (0U)                /* Data transfer immediately */
#define DAC_CH_DATA_TRANS_HRPWM             (DAC_DACR2_LDMD1)   /* Data transfer trigger by HRPWM event */
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_CNT_DIR DAC Ramp Count Direction
 * @{
 */
#define DAC_RAMP_CNT_DIR_DOWN               (0U)
#define DAC_RAMP_CNT_DIR_UP                 (DAC_DACR_RAMPDIR1)
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_STEP_EVT DAC Ramp Step Event
 * @{
 */
#define DAC_RAMP_STEP_EVT_SOFT              (0UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_1                 (1UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_2                 (2UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_3                 (3UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_4                 (4UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_5                 (5UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_6                 (6UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_7                 (7UL << DAC_DACR2_RAMPSTEPSEL1_POS)
#define DAC_RAMP_STEP_EVT_8                 (8UL << DAC_DACR2_RAMPSTEPSEL1_POS)
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_RST_EVT DAC Ramp Reset Event
 * @{
 */
#define DAC_RAMP_RST_EVT_SOFT               (0UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_1                  (1UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_2                  (2UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_3                  (3UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_4                  (4UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_5                  (5UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_6                  (6UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_7                  (7UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_8                  (8UL << DAC_DACR2_RAMPRSTSEL1_POS)
#define DAC_RAMP_RST_EVT_0                  (9UL << DAC_DACR2_RAMPRSTSEL1_POS)
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_STATE DAC Ramp State
 * @{
 */
#define DAC_RAMP_DISABLE                    (0UL)
#define DAC_RAMP_ENABLE                     (DAC_DACR2_RAMPMD1)
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_CMP_CTRL DAC Ramp Cmp Control
 * @{
 */
#define DAC_RAMP_CMP_CTRL_DISABLE           (0U)
#define DAC_RAMP_CMP_CTRL_ENABLE            (DAC_DACR3_CMPSCTL1)
/**
 * @}
 */

/**
 * @defgroup DAC_RAMP_SW_TRIG DAC Ramp SW Trigger
 * @{
 */
#define DAC_RAMP_SW_TRIG_RST                (DAC_RAMPSWTRGR_RSTSWTRG1)
#define DAC_RAMP_SW_TRIG_STEP               (DAC_RAMPSWTRGR_STEPSWTRG1)
/**
 * @}
 */

/**
 * @defgroup DAC_DIODE_TRANS_EVT DAC Diode Transfer Event
 * @{
 */
#define DAC_DIODE_TRANS_EVT_1               (0UL)
#define DAC_DIODE_TRANS_EVT_2               (1UL)
#define DAC_DIODE_TRANS_EVT_3               (2UL)
#define DAC_DIODE_TRANS_EVT_4               (3UL)
#define DAC_DIODE_TRANS_EVT_5               (4UL)
#define DAC_DIODE_TRANS_EVT_6               (5UL)
#define DAC_DIODE_TRANS_EVT_7               (6UL)
#define DAC_DIODE_TRANS_EVT_8               (7UL)
/**
 * @}
 */

/**
 * @defgroup DAC_OFFSET_MODE DAC Offset Mode
 * @{
 */
#define DAC_DIODE_OFT_INVALID               (0UL)
#define DAC_DIODE_OFT_INCREASE              (DAC_DACR3_OFSTDIR1 | DAC_DACR3_OFSTMD1)
#define DAC_DIODE_OFT_DECREASE              (DAC_DACR3_OFSTMD1)
/**
 * @}
 */

/**
 * @defgroup DAC_DIODE_STATE DAC Diode State
 * @{
 */
#define DAC_DIODE_DISABLE                   (0U)
#define DAC_DIODE_ENABLE                    (DAC_DACR2_DEMD1)
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
 * @addtogroup DAC_Global_Functions
 * @{
 */

int32_t DAC_StructInit(stc_dac_init_t *pstcDacInit);
int32_t DAC_Init(CM_DAC_TypeDef *DACx, uint16_t u16Ch, const stc_dac_init_t *pstcDacInit);
int32_t DAC_DeInit(CM_DAC_TypeDef *DACx);

void DAC_DataRegAlignConfig(CM_DAC_TypeDef *DACx, uint16_t u16Align);
void DAC_OutputCmd(CM_DAC_TypeDef *DACx, uint16_t u16Ch, en_functional_state_t enNewState);
void DAC_AMPCmd(CM_DAC_TypeDef *DACx, uint16_t u16Ch, en_functional_state_t enNewState);
void DAC_ADCPrioCmd(CM_DAC_TypeDef *DACx, en_functional_state_t enNewState);
void DAC_ADCPrioConfig(CM_DAC_TypeDef *DACx, uint16_t u16ADCxPrio, en_functional_state_t enNewState);

int32_t DAC_Start(CM_DAC_TypeDef *DACx, uint16_t u16Ch);
int32_t DAC_Stop(CM_DAC_TypeDef *DACx, uint16_t u16Ch);
void DAC_StartDualCh(CM_DAC_TypeDef *DACx);
void DAC_StopDualCh(CM_DAC_TypeDef *DACx);

void DAC_SetChData(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16Data);
void DAC_SetDualChData(CM_DAC_TypeDef *DACx, uint16_t u16Data1, uint16_t u16Data2);
int32_t DAC_GetChConvertState(const CM_DAC_TypeDef *DACx, uint16_t u16Ch);

void DAC_SetChDataTransMode(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16Mode);
uint16_t DAC_GetActiveData(CM_DAC_TypeDef *DACx, uint16_t u16Ch);

int32_t DAC_RampStructInit(stc_dac_ramp_init_t *pstcDacRampInit);
int32_t DAC_RampInit(CM_DAC_TypeDef *DACx, uint16_t u16Ch, const stc_dac_ramp_init_t *pstcDacRampInit);
void DAC_RampSetLimit(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16LimitValue);
void DAC_RampSetStep(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16StepValue);
void DAC_RampSetResetDelay(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16ResetDelayValue);
void DAC_RampSetStepEvt(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint32_t u32StepEvt);
void DAC_RampSetRstEvt(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint32_t u32ResetEvt);
void DAC_RampCmd(CM_DAC_TypeDef *DACx, uint16_t u16Ch, en_functional_state_t enNewState);
void DAC_RampCmpCmd(CM_DAC_TypeDef *DACx, uint16_t u16Ch, en_functional_state_t enNewState);
void DAC_RampSWTrig(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16TrigSingnal);
int32_t DAC_DiodeStructInit(stc_dac_diode_init_t *pstcDacDiodeInit);
int32_t DAC_DiodeInit(CM_DAC_TypeDef *DACx, uint16_t u16Ch, const stc_dac_diode_init_t *pstcDacDiodeInit);
void DAC_DiodeSetData(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16DiodeValue);
void DAC_DiodeSetTransEvt(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint32_t u32DiodeEvt);
void DAC_DiodeCmd(CM_DAC_TypeDef *DACx, uint16_t u16Ch, en_functional_state_t enNewState);
void DAC_SetOffsetMode(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16OffsetMode);
void DAC_SetOffset(CM_DAC_TypeDef *DACx, uint16_t u16Ch, uint16_t u16OffsetValue);
/**
 * @}
 */

#endif /* LL_DAC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_DAC_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
