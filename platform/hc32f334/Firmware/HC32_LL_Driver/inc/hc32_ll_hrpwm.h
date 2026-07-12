/**
 *******************************************************************************
 * @file  hc32_ll_hrpwm.h
 * @brief This file contains all the functions prototypes of the HRPWM driver
 *        library.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
   2024-06-30       CDT             Modify API about status register for coupling risk
   2024-09-13       CDT             Add special match event for group HRPWM_HW_Start_Condition_Define and group HRPWM_HW_Clear_Condition_Define
   2025-09-18       CDT             Fix bug for HRPWM_GetStatus()
                                    Modify macro definition for group HRPWM_Idle_BM_Output_Follow_Func_Define and HRPWM_EVT_Filter_Clock_Define
   2025-11-03       CDT             Fix bug for API HRPWM_IDLE_BM_SetUnitCountReset()
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
#ifndef __HC32_LL_HRPWM_H__
#define __HC32_LL_HRPWM_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
#include "hc32_ll_def.h"
#include "hc32_ll_utility.h"

#include "hc32f3xx.h"
#include "hc32f3xx_conf.h"

/**
 * @addtogroup LL_Driver
 * @{
 */

/**
 * @addtogroup LL_HRPWM
 * @{
 */

#if (LL_HRPWM_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup HRPWM_Global_Types HRPWM Global Types
 * @{
 */

/**
 * @brief HRPWM count function structure definition
 */
typedef struct {
    uint32_t u32CountMode;          /*!< Count mode, @ref HRPWM_Count_Mode_Define */
    uint32_t u32PeriodValue;        /*!< The period reference value, range (0xC0, 0x3FFFC0] */
    uint32_t u32CountReload;        /*!< Count reload after count peak @ref HRPWM_Count_Reload_Define */
} stc_hrpwm_init_t;

/**
 * @brief HRPWM buffer function configuration structure definition
 */
typedef struct {
    uint32_t u32BufTransCond;                       /*!< Normal buffer transfer condition, this parameter can be a value
                                                         of @ref HRPWM_Buf_Trans_Cond_Define  */
    en_functional_state_t enBufTransU1Single;       /*!< Buffer transfer when HRPWM1 buffer single transfer occurs @ref en_functional_state_t
                                                         Valid for HRPWM2 ~ HRPWM6 */
    en_functional_state_t enBufTransAfterU1Single;  /*!< Sawtooth: Buffer transfer when count peak after HRPWM1 buffer single transfer occurs
                                                         Triangle: Buffer transfer when count valley after HRPWM1 buffer single transfer occurs
                                                         @ref en_functional_state_t
                                                         Valid for HRPWM2 ~ HRPWM6 when HRPWM1 buffer single transfer function is enable */
} stc_hrpwm_buf_config_t;

/**
 * @brief HRPWM pwm function structure definition
 */
typedef struct {
    uint32_t u32CompareValue;               /*!< Range (0xC0 ~ 0x3FFFC0] */
    uint32_t u32StartPolarity;              /*!< Polarity when count start @ref HRPWM_Pin_Polarity_Start_Define */
    uint32_t u32StopPolarity;               /*!< Polarity when count stop @ref HRPWM_Pin_Polarity_Stop_Define */
    uint32_t u32PeakPolarity;               /*!< Polarity when count peak @ref HRPWM_Pin_Polarity_Peak_Define */
    uint32_t u32ValleyPolarity;             /*!< Polarity when count valley @ref HRPWM_Pin_Polarity_Valley_Define */
    uint32_t u32UpMatchAPolarity;           /*!< Polarity when count up and match HRGCMAR @ref HRPWM_Pin_Polarity_Up_Match_A_Define */
    uint32_t u32DownMatchAPolarity;         /*!< Polarity when count down and match HRGCMAR @ref HRPWM_Pin_Polarity_Down_Match_A_Define */
    uint32_t u32UpMatchBPolarity;           /*!< Polarity when count up and match HRGCMBR @ref HRPWM_Pin_Polarity_Up_Match_B_Define */
    uint32_t u32DownMatchBPolarity;         /*!< Polarity when count down and match HRGCMBR @ref HRPWM_Pin_Polarity_Down_Match_B_Define */
    uint32_t u32UpMatchEPolarity;           /*!< Polarity when count up and match HRGCMER @ref HRPWM_Pin_Polarity_Up_Match_E_Define */
    uint32_t u32DownMatchEPolarity;         /*!< Polarity when count down and match HRGCMER @ref HRPWM_Pin_Polarity_Down_Match_E_Define */
    uint32_t u32UpMatchFPolarity;           /*!< Polarity when count up and match HRGCMFR @ref HRPWM_Pin_Polarity_Up_Match_F_Define */
    uint32_t u32DownMatchFPolarity;         /*!< Polarity when count down and match HRGCMFR @ref HRPWM_Pin_Polarity_Down_Match_F_Define */
    uint32_t u32UpMatchSpecialAPolarity;    /*!< Polarity when count up and match SCMAR @ref HRPWM_Pin_Polarity_Up_Match_Special_A_Define */
    uint32_t u32DownMatchSpecialAPolarity;  /*!< Polarity when count down and match SCMAR @ref HRPWM_Pin_Polarity_Down_Match_Special_A_Define */
    uint32_t u32UpMatchSpecialBPolarity;    /*!< Polarity when count up and match SCMBR @ref HRPWM_Pin_Polarity_Up_Match_Special_B_Define */
    uint32_t u32DownMatchSpecialBPolarity;  /*!< Polarity when count down and match SCMBR @ref HRPWM_Pin_Polarity_Down_Match_Special_B_Define */
} stc_hrpwm_pwm_init_t;

/**
 * @brief HRPWM pwm output function structure definition
 */
typedef struct {
    uint32_t u32ChSwapMode;         /*!< PWM channel output swap mode @ref HRPWM_PWM_Ch_Swap_Mode_Define */
    uint32_t u32ChSwap;             /*!< PWM channel output swap function @ref HRPWM_PWM_Ch_Swap_Func_Define */
    uint32_t u32ChAInvert;          /*!< PWM channel A output invert @ref HRPWM_PWM_ChA_Invert_Define */
    uint32_t u32ChBInvert;          /*!< PWM channel B output invert @ref HRPWM_PWM_ChB_Invert_Define */
} stc_hrpwm_pwm_output_init_t;

/**
 * @brief HRPWM idle delay function structure definition
 */
typedef struct {
    uint32_t u32TriggerSrc;         /*!< Idle delay trigger source @ref HRPWM_Idle_Delay_Trigger_Src_Define */
    uint32_t u32PeriodPoint;        /*!< Complete period point @ref HRPWM_Complete_Period_Point_Define */
    uint32_t u32IdleOutputChAStatus;/*!< HRPWM Idle Output Channel A status @ref HRPWM_Idle_Output_ChA_Status_Define */
    uint32_t u32IdleOutputChBStatus;/*!< HRPWM Idle Output Channel B status @ref HRPWM_Idle_Output_ChB_Status_Define */
} stc_hrpwm_idle_delay_init_t;

/**
 * @brief HRPWM idle BM function structure definition
 */
typedef struct {
    uint32_t u32Mode;               /*!< BM action mode @ref HRPWM_Idle_BM_Action_Mode_Define */
    uint32_t u32CountSrc;           /*!< BM count source @ref HRPWM_Idle_BM_Count_Src_Define */
    uint32_t u32CountPclkDiv;       /*!< PCLK0 divider if count source is HRPWM_BM_CNT_SRC_PCLK
                                         @ref HRPWM_Idle_BM_Count_Src_Pclk_Define */
    uint32_t u32PeriodValue;        /*!< BM count period value, range 0x0000 ~ 0xFFFF */
    uint32_t u32CompareValue;       /*!< BM count period value, range 0x0000 ~ 0xFFFF */
    uint32_t u32CountReload;        /*!< Count reload after count peak @ref HRPWM_Idle_BM_Count_Reload_Define */
    uint64_t u64TriggerSrc;         /*!< BM output start trigger source, Can be one or any combination of the values
                                         from @ref HRPWM_Idle_BM_Trigger_Src_Define */
} stc_hrpwm_idle_bm_init_t;

/**
 * @brief HRPWM idle BM output function structure definition
 */
typedef struct {
    uint32_t u32IdleOutputChAStatus;/*!< HRPWM BM Output Channel A status @ref HRPWM_Idle_BM_Output_ChA_Status_Define */
    uint32_t u32IdleOutputChBStatus;/*!< HRPWM BM Output Channel B status @ref HRPWM_Idle_BM_Output_ChB_Status_Define */
    uint32_t u32IdleChAEnterDelay;  /*!< HRPWM channel A BM output enter idle delay function @ref HRPWM_Idle_ChA_BM_Output_Delay_Define */
    uint32_t u32IdleChBEnterDelay;  /*!< HRPWM channel B BM output enter idle delay function @ref HRPWM_Idle_ChB_BM_Output_Delay_Define */
    uint32_t u32Follow;             /*!< HRPWM channel follow function @ref HRPWM_Idle_BM_Output_Follow_Func_Define */
    uint32_t u32UnitCountReset;     /*!< HRPWM count stop and reset in the BM idle state @ref HRPWM_Idle_BM_Unit_Count_Reset_Define */
} stc_hrpwm_idle_bm_output_init_t;

/**
 * @brief HRPWM Valid period function configuration structure definition
 */
typedef struct {
    uint32_t u32CountCond;          /*!< The count condition, and this parameter can be a value of
                                          @ref HRPWM_Valid_Period_Count_Cond_Define */
    uint32_t u32Interval;           /*!< The interval of the valid period, range [0, 31] */
    uint32_t u32SpecialA;           /*!< Valid period function for special A @ref HRPWM_Valid_Period_Special_A_Define*/
    uint32_t u32SpecialB;           /*!< Valid period function for special B @ref HRPWM_Valid_Period_Special_B_Define*/
} stc_hrpwm_valid_period_config_t;

/**
 * @brief HRPWM External event configuration structure definition
 */
typedef struct {
    uint32_t u32EventSrc;           /*!< External event source set @ref HRPWM_EVT_Src_Define */
    uint32_t u32EventSrc2;          /*!< External event source 2 set (valid when u32EventSrc is HRPWM_EVT_SRC2) @ref HRPWM_EVT_Src2_Define */
    uint32_t u32ValidAction;        /*!< External event valid action @ref HRPWM_EVT_Valid_Action_Define */
    uint32_t u32ValidLevel;         /*!< External event valid level @ref HRPWM_EVT_Valid_Level_Define */
    uint32_t u32FastAsyncMode;      /*!< External event fast asynchronous mode @ref HRPWM_EVT_Fast_Async_Define
                                         Only valid for event 1 ~ event 5 */
    uint32_t u32FilterClock;        /*!< External event filter clock @ref HRPWM_EVT_Filter_Clock_Define
                                         Only valid for event 6 ~ event 10 */
    uint32_t u32ExtEvtDetect;       /*!< Generate external event detect event function @ref HRPWM_EVT_Generate_Detect_Fun_Define */
} stc_hrpwm_evt_config_t;

/**
 * @brief HRPWM External event filter configuration structure definition
 */
typedef struct {
    uint32_t u32Mode;               /*!< Filter mode @ref HRPWM_EVT_Filter_Mode_Define */
    uint32_t u32Latch;              /*!< Event Latch function @ref HRPWM_EVT_Filter_Latch_Func_Define */
    uint32_t u32WindowTimeout;      /*!< Timeout function for window mode, valid when latch function disable @ref HRPWM_EVT_Filter_Timeout_Func_Define */
} stc_hrpwm_evt_filter_config_t;

/**
 * @brief HRPWM unit External event filter configuration structure definition
 */
typedef struct {
    uint32_t u32InitPolarity;       /*!< Event filter signal initial polarity @ref HRPWM_EVT_Init_Polarity_Define */
    uint32_t u32Offset;             /*!< Offset register value, range 0x00 ~ 0x3FFFFF */
    uint32_t u32OffsetDir;          /*!< Offset direction, @ref HRPWM_EVT_Filter_Offset_Dir_Define */
    uint32_t u32Window;             /*!< window register value, range 0x00 ~ 0x3FFFFF */
    uint32_t u32WindowDir;          /*!< window direction, @ref HRPWM_EVT_Filter_Window_Dir_Define */
} stc_hrpwm_evt_filter_signal_config_t;

/**
 * @brief HRPWM synchronous output config structure definition
 */
typedef struct {
    uint32_t u32Src;                /*!< Synchronous output source @ref HRPWM_Sync_Output_Src_Define */
    uint32_t u32MatchBDir;          /*!< Count direction when match special B @ref HRPWM_Sync_Output_Match_B_Dir_Define
                                         Up and down for triangle waveform, up for sawtooth waveform */
    uint32_t u32Pulse;              /*!< Synchronous output pulse @ref HRPWM_Sync_Output_Pulse_Define */
    uint32_t u32PulseWidth;         /*!< Synchronous output pulse width, range 0x10 ~ 0xFF
                                         Width = T(PCLK0) * u32PulseWidth */
} stc_hrpwm_sync_output_config_t;

/**
 * @brief HRPWM DAC synchronous trigger structure definition
 */
typedef struct {
    uint32_t u32Src;                /*!< DAC trigger source @ref HRPWM_DAC_Trigger_Src_Define */
    uint32_t u32DacCh1Dest;         /*!< Trigger destination for DAC channel 1 @ref HRPWM_DAC_Ch1_Trigger_Dest_Define */
    uint32_t u32DacCh2Dest;         /*!< Trigger destination for DAC channel 2 @ref HRPWM_DAC_Ch2_Trigger_Dest_Define */
} stc_hrpwm_dac_trigger_config_t;

/**
 * @brief HRPWM phase match function structure definition
 */
typedef struct {
    uint32_t u32PhaseIndex;         /*!< Select phase match event index from HRPWM master unit for specified HRPWM unit
                                         @ref HRPWM_PH_Index_Define */
    uint32_t u32ForceChA;           /*!< Force the ChA output low when Phase match event occurs after master unit period
                                         point @ref HRPWM_PH_Force_ChA_Func_Define */
    uint32_t u32ForceChB;           /*!< Force the ChB output low when Phase match event occurs after master unit period
                                         point @ref HRPWM_PH_Force_ChB_Func_Define */
    uint32_t u32PeriodLink;         /*!< Period link function @ref HRPWM_PWM_PH_Period_Link_Define */
} stc_hrpwm_ph_config_t;

/**
 * @brief HRPWM Dead time function configuration structure definition
 */
typedef struct {
    uint32_t u32EqualUpDown;        /*!< Down count dead time register equal to up count dead time register
                                         @ref HRPWM_DeadTime_Reg_Equal_Func_Define */
    uint32_t u32BufUp;              /*!< Buffer transfer for up count dead time register (DTUBR-->DTUAR)
                                         @ref HRPWM_DeadTime_CountUp_Buf_Func_Define*/
    uint32_t u32BufDown;            /*!< Buffer transfer for down count dead time register (DTDBR-->DTDAR)
                                         @ref HRPWM_DeadTime_CountDown_Buf_Func_Define*/
} stc_hrpwm_deadtime_config_t;

/**
 * @brief HRPWM EMB configuration structure definition
 */
typedef struct {
    uint32_t u32ValidCh;            /*!< Valid EMB event channel @ref HRPWM_Emb_Ch_Define */
    uint32_t u32ReleaseMode;        /*!< Pin release mode when EMB event invalid @ref HRPWM_Emb_Release_Mode_Define */
    uint32_t u32PinStatus;          /*!< Pin output status when EMB event valid @ref HRPWM_Emb_Pin_Status_Define */
} stc_hrpwm_emb_config_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/**
 * @defgroup HRPWM_Global_Macros HRPWM Global Macros
 * @{
 */

/* About 1mS timeout */
#define HRPWM_CALIB_TIMEOUT                 (HCLK_VALUE / 1000UL)

#define HRPWM_REG_VALUE_3FFFC0              (0x003FFFC0UL)

#define PCNAR1_REG_POLARITY_CFG_MASK        (0x0000FFFFUL)
#define PCNAR2_REG_POLARITY_CFG_MASK        (0x0FF00000UL)
#define PCNAR3_REG_POLARITY_CFG_MASK        (0x0FF00000UL)
#define PCNBR1_REG_POLARITY_CFG_MASK        (0x0000FFFFUL)
#define PCNBR2_REG_POLARITY_CFG_MASK        (0x0FF00000UL)
#define PCNBR3_REG_POLARITY_CFG_MASK        (0x0FF00000UL)

#define HRPWM_EXTEVT1_CONFIG_MASK           (HRPWM_COMMON_EECR1_EE1SRC | HRPWM_COMMON_EECR1_EE1POL | HRPWM_COMMON_EECR1_EE1SNS | HRPWM_COMMON_EECR1_EE1FAST)
#define HRPWM_EXTEVT6_CONFIG_MASK           (HRPWM_COMMON_EECR2_EE6SRC | HRPWM_COMMON_EECR2_EE6POL | HRPWM_COMMON_EECR2_EE6SNS)
#define HRPWM_EXTEVT1_FILTER_CONFIG_MASK    (HRPWM_EEFLTCR1_EE1LAT | HRPWM_EEFLTCR1_EE1FM | HRPWM_EEFLTCR1_EE1TMO)

#define HRPWM_STFLR2_CLR_MASK               (HRPWM_STFLR2_OSTOVF | HRPWM_PH_FLAG_ALL | HRPWM_IDLE_DELAY_FLAG_CLR_ALL)
#define HRPWM_BMCR_BMTMR_MASK               (0x3FUL << HRPWM_COMMON_BMCR_BMTMR1_POS)

/**
 * @defgroup HRPWM_Mul_Unit_Define HRPWM Multiple Unit Define
 * @{
 */
#define HRPWM_UNIT1                         (0x01UL)
#define HRPWM_UNIT2                         (0x01UL << 1U)
#define HRPWM_UNIT3                         (0x01UL << 2U)
#define HRPWM_UNIT4                         (0x01UL << 3U)
#define HRPWM_UNIT5                         (0x01UL << 4U)
#define HRPWM_UNIT6                         (0x01UL << 5U)
#define HRPWM_UNIT_ALL                      (0x3FUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sw_Sync_Unit_Define HRPWM Unit Define For Software Synchronous function
 * @{
 */
#define HRPWM_SW_SYNC_UNIT1                 (0x03UL)
#define HRPWM_SW_SYNC_UNIT2                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP2A_POS)
#define HRPWM_SW_SYNC_UNIT3                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP3A_POS)
#define HRPWM_SW_SYNC_UNIT4                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP4A_POS)
#define HRPWM_SW_SYNC_UNIT5                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP5A_POS)
#define HRPWM_SW_SYNC_UNIT6                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP6A_POS)
#define HRPWM_SW_SYNC_UNIT_ALL              (0x0FFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sw_Sync_Ch_Define HRPWM Channel Define For Software Synchronous function
 * @{
 */
#define HRPWM_SW_SYNC_CHA                   (0x00000555UL)
#define HRPWM_SW_SYNC_CHB                   (0x00000AAAUL)
#define HRPWM_SW_SYNC_CH_ALL                (HRPWM_SW_SYNC_CHA | HRPWM_SW_SYNC_CHB)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Define HRPWM Input And Output Pin Define
 * @{
 */
#define HRPWM_IO_PWMA                       (0x00UL)    /*!< Pin HRPWM_<t>_PWMA */
#define HRPWM_IO_PWMB                       (0x01UL)    /*!< Pin HRPWM_<t>_PWMB */
#define HRPWM_INPUT_TRIGA                   (0x02UL)    /*!< Input pin HRPWM_TRIGA */
#define HRPWM_INPUT_TRIGB                   (0x03UL)    /*!< Input pin HRPWM_TRIGB */
#define HRPWM_INPUT_TRIGC                   (0x04UL)    /*!< Input pin HRPWM_TRIGC */
#define HRPWM_INPUT_TRIGD                   (0x05UL)    /*!< Input pin HRPWM_TRIGD */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Calib_Flag_Define HRPWM Calibrate Status Flag Define
 * @{
 */
#define HRPWM_CALIB_FLAG_END                (HRPWM_COMMON_CALCR_CALENF)    /*!< HRPWM calibrate end flag */
#define HRPWM_CALIB_FLAG_ERR                (HRPWM_COMMON_CALCR_ERRF)      /*!< HRPWM calibrate error flag */
#define HRPWM_CALIB_FLAG_ALL                (HRPWM_CALIB_FLAG_END | HRPWM_CALIB_FLAG_ERR)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Calib_Period_Define HRPWM Calibrate Period Define
 * @{
 */
#define HRPWM_CALIB_PERIOD_8P7_MS           (0x00UL)    /*!< Calibrate period 8.7 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL2      (0x01UL)    /*!< Calibrate period 8.7*2 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL4      (0x02UL)    /*!< Calibrate period 8.7*4 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL8      (0x03UL)    /*!< Calibrate period 8.7*8 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL16     (0x04UL)    /*!< Calibrate period 8.7*16 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL32     (0x05UL)    /*!< Calibrate period 8.7*32 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL64     (0x06UL)    /*!< Calibrate period 8.7*64 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL128    (0x07UL)    /*!< Calibrate period 8.7*128 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL256    (0x08UL)    /*!< Calibrate period 8.7*256 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL512    (0x09UL)    /*!< Calibrate period 8.7*512 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL1024   (0x0AUL)    /*!< Calibrate period 8.7*1024 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL2048   (0x0BUL)    /*!< Calibrate period 8.7*2048 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL4096   (0x0CUL)    /*!< Calibrate period 8.7*4096 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL8192   (0x0DUL)    /*!< Calibrate period 8.7*8192 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL16384  (0x0EUL)    /*!< Calibrate period 8.7*16384 mS */
#define HRPWM_CALIB_PERIOD_8P7_MS_MUL32768  (0x0FUL)    /*!< Calibrate period 8.7*32768 mS */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Count_Mode_Define HRPWM Base Counter Function Mode Define
 * @{
 */
#define HRPWM_MD_SAWTOOTH                   (0x00UL)
#define HRPWM_MD_TRIANGLE                   (HRPWM_GCONR_MODE)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Count_Reload_Define HRPWM Count Stop After Count Peak Or Valley Function Define
 * @{
 */
#define HRPWM_CNT_RELOAD_ON                 (0x00UL)
#define HRPWM_CNT_RELOAD_OFF                (HRPWM_GCONR_OVSTP)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Int_Flag_Define HRPWM Interrupt Flag Define
 * @{
 */
#define HRPWM_INT_MATCH_A                   (HRPWM_ICONR_INTENA)     /*!< Match HRGCMAR register */
#define HRPWM_INT_MATCH_B                   (HRPWM_ICONR_INTENB)     /*!< Match HRGCMBR register */
#define HRPWM_INT_MATCH_C                   (HRPWM_ICONR_INTENC)     /*!< Match HRGCMCR register */
#define HRPWM_INT_MATCH_D                   (HRPWM_ICONR_INTEND)     /*!< Match HRGCMDR register */
#define HRPWM_INT_MATCH_E                   (HRPWM_ICONR_INTENE)     /*!< Match HRGCMER register */
#define HRPWM_INT_MATCH_F                   (HRPWM_ICONR_INTENF)     /*!< Match HRGCMFR register */
#define HRPWM_INT_CNT_PEAK                  (HRPWM_ICONR_INTENOVF)   /*!< Count peak */
#define HRPWM_INT_CNT_VALLEY                (HRPWM_ICONR_INTENUDF)   /*!< Count valley */
#define HRPWM_INT_CAPT_A                    (HRPWM_ICONR_INTENCAPA)  /*!< Capture A */
#define HRPWM_INT_CAPT_B                    (HRPWM_ICONR_INTENCAPB)  /*!< Capture B */
#define HRPWM_INT_UP_MATCH_SPECIAL_A        (HRPWM_ICONR_INTENSAU)   /*!< Match SCMAR register when count up */
#define HRPWM_INT_DOWN_MATCH_SPECIAL_A      (HRPWM_ICONR_INTENSAD)   /*!< Match SCMAR register when count down */
#define HRPWM_INT_UP_MATCH_SPECIAL_B        (HRPWM_ICONR_INTENSBU)   /*!< Match SCMBR register when count up */
#define HRPWM_INT_DOWN_MATCH_SPECIAL_B      (HRPWM_ICONR_INTENSBD)   /*!< Match SCMBR register when count down */
#define HRPWM_INT_ALL                       (HRPWM_INT_MATCH_A | HRPWM_INT_MATCH_B | HRPWM_INT_MATCH_C | \
                                            HRPWM_INT_MATCH_D | HRPWM_INT_MATCH_E | HRPWM_INT_MATCH_F |  \
                                            HRPWM_INT_CNT_PEAK | HRPWM_INT_CNT_VALLEY | HRPWM_INT_CAPT_A | \
                                            HRPWM_INT_CAPT_B | HRPWM_INT_UP_MATCH_SPECIAL_A | \
                                            HRPWM_INT_DOWN_MATCH_SPECIAL_A | HRPWM_INT_UP_MATCH_SPECIAL_B | \
                                            HRPWM_INT_DOWN_MATCH_SPECIAL_B)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Buf_Trans_Cond_Define HRPWM Buffer Transfer Condition Define
 * @{
 */
#define HRPWM_BUF_TRANS_INVD                (0x00UL)  /*!< Buffer don't transfer */
#define HRPWM_BUF_TRANS_PEAK                (0x01UL)  /*!< Dead time buffer and phase buffer transfer when count peak, other buf as follows:
                                                           Sawtooth: Buffer transfer when count peak or hardware clearing occurs
                                                           Triangle: Buffer transfer when count up and match the value (period-64) */
#define HRPWM_BUF_TRANS_VALLEY              (0x02UL)  /*!< Dead time buffer and phase buffer transfer when count valley, other buf as follows:
                                                           Sawtooth: Buffer transfer when count valley
                                                           Triangle: Buffer transfer when count down and match 64 or hardware clearing occurs */
#define HRPWM_BUF_TRANS_PEAK_VALLEY         (0x03UL)  /*!< Dead time buffer and phase buffer transfer when count peak and valley, other buf as follows:
                                                           Sawtooth: Buffer transfer when count peak, valley, hardware clearing occurs
                                                           Triangle: Buffer transfer when count up and match the value (period-64),
                                                                                          count down and match 64,
                                                                                          hardware clearing occurs */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Buf_U1_Single_Trans_Define HRPWM Buffer U1 single transfer Configuration Define
 * @{
 */
#define HRPWM_BUF_U1_SINGLE_TRANS_OFF       (0x00UL)
#define HRPWM_BUF_U1_SINGLE_TRANS_ON        (0x01UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Start_Define HRPWM Pin Polarity For Count Start Define
 * @{
 */
#define HRPWM_PWM_START_LOW                 (0x00UL)
#define HRPWM_PWM_START_HIGH                (0x01UL)
#define HRPWM_PWM_START_HOLD                (0x02UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Stop_Define HRPWM Pin Polarity For Count Stop Define
 * @{
 */
#define HRPWM_PWM_STOP_LOW                  (0x00UL << HRPWM_PCNAR1_STPCA_POS)
#define HRPWM_PWM_STOP_HIGH                 (0x01UL << HRPWM_PCNAR1_STPCA_POS)
#define HRPWM_PWM_STOP_HOLD                 (0x02UL << HRPWM_PCNAR1_STPCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Peak_Define HRPWM Pin Polarity For Count Peak (or Hardware Clear for Sawtooth) Define
 * @{
 */
#define HRPWM_PWM_PEAK_LOW                  (0x00UL << HRPWM_PCNAR1_OVFCA_POS)
#define HRPWM_PWM_PEAK_HIGH                 (0x01UL << HRPWM_PCNAR1_OVFCA_POS)
#define HRPWM_PWM_PEAK_HOLD                 (0x02UL << HRPWM_PCNAR1_OVFCA_POS)
#define HRPWM_PWM_PEAK_INVT                 (0x03UL << HRPWM_PCNAR1_OVFCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Valley_Define HRPWM Pin Polarity For Count Valley Define
 * @{
 */
#define HRPWM_PWM_VALLEY_LOW                (0x00UL << HRPWM_PCNAR1_UDFCA_POS)
#define HRPWM_PWM_VALLEY_HIGH               (0x01UL << HRPWM_PCNAR1_UDFCA_POS)
#define HRPWM_PWM_VALLEY_HOLD               (0x02UL << HRPWM_PCNAR1_UDFCA_POS)
#define HRPWM_PWM_VALLEY_INVT               (0x03UL << HRPWM_PCNAR1_UDFCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_A_Define HRPWM Pin Polarity For Count Up And Match HRGCMAR Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_A_LOW            (0x00UL << HRPWM_PCNAR1_CMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_A_HIGH           (0x01UL << HRPWM_PCNAR1_CMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_A_HOLD           (0x02UL << HRPWM_PCNAR1_CMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_A_INVT           (0x03UL << HRPWM_PCNAR1_CMAUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_A_Define HRPWM Pin Polarity For Count Down And Match HRGCMAR Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_A_LOW          (0x00UL << HRPWM_PCNAR1_CMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_A_HIGH         (0x01UL << HRPWM_PCNAR1_CMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_A_HOLD         (0x02UL << HRPWM_PCNAR1_CMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_A_INVT         (0x03UL << HRPWM_PCNAR1_CMADCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_B_Define HRPWM Pin Polarity For Count Up And Match HRGCMBR Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_B_LOW            (0x00UL << HRPWM_PCNAR1_CMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_B_HIGH           (0x01UL << HRPWM_PCNAR1_CMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_B_HOLD           (0x02UL << HRPWM_PCNAR1_CMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_B_INVT           (0x03UL << HRPWM_PCNAR1_CMBUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_B_Define HRPWM Pin Polarity For Count Down And Match HRGCMBR Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_B_LOW          (0x00UL << HRPWM_PCNAR1_CMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_B_HIGH         (0x01UL << HRPWM_PCNAR1_CMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_B_HOLD         (0x02UL << HRPWM_PCNAR1_CMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_B_INVT         (0x03UL << HRPWM_PCNAR1_CMBDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_E_Define HRPWM Pin Polarity For Count Up And Match HRGCMER Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_E_LOW            (0x00UL << HRPWM_PCNAR2_CMEUCA_POS)
#define HRPWM_PWM_UP_MATCH_E_HIGH           (0x01UL << HRPWM_PCNAR2_CMEUCA_POS)
#define HRPWM_PWM_UP_MATCH_E_HOLD           (0x02UL << HRPWM_PCNAR2_CMEUCA_POS)
#define HRPWM_PWM_UP_MATCH_E_INVT           (0x03UL << HRPWM_PCNAR2_CMEUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_E_Define HRPWM Pin Polarity For Count Down And Match HRGCMER Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_E_LOW          (0x00UL << HRPWM_PCNAR3_CMEDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_E_HIGH         (0x01UL << HRPWM_PCNAR3_CMEDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_E_HOLD         (0x02UL << HRPWM_PCNAR3_CMEDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_E_INVT         (0x03UL << HRPWM_PCNAR3_CMEDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_F_Define HRPWM Pin Polarity For Count Up And Match HRGCMFR Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_F_LOW            (0x00UL << HRPWM_PCNAR2_CMFUCA_POS)
#define HRPWM_PWM_UP_MATCH_F_HIGH           (0x01UL << HRPWM_PCNAR2_CMFUCA_POS)
#define HRPWM_PWM_UP_MATCH_F_HOLD           (0x02UL << HRPWM_PCNAR2_CMFUCA_POS)
#define HRPWM_PWM_UP_MATCH_F_INVT           (0x03UL << HRPWM_PCNAR2_CMFUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_F_Define HRPWM Pin Polarity For Count Down And Match HRGCMFR Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_F_LOW          (0x00UL << HRPWM_PCNAR3_CMFDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_F_HIGH         (0x01UL << HRPWM_PCNAR3_CMFDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_F_HOLD         (0x02UL << HRPWM_PCNAR3_CMFDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_F_INVT         (0x03UL << HRPWM_PCNAR3_CMFDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_Special_A_Define HRPWM Pin Polarity For Count Up And Match SCMAR Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_SPECIAL_A_LOW    (0x00UL << HRPWM_PCNAR2_SCMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_A_HIGH   (0x01UL << HRPWM_PCNAR2_SCMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_A_HOLD   (0x02UL << HRPWM_PCNAR2_SCMAUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_A_INVT   (0x03UL << HRPWM_PCNAR2_SCMAUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_Special_A_Define HRPWM Pin Polarity For Count Down And Match SCMAR Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_A_LOW  (0x00UL << HRPWM_PCNAR3_SCMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_A_HIGH (0x01UL << HRPWM_PCNAR3_SCMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_A_HOLD (0x02UL << HRPWM_PCNAR3_SCMADCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_A_INVT (0x03UL << HRPWM_PCNAR3_SCMADCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_Match_Special_B_Define HRPWM Pin Polarity For Count Up And Match SCMBR Define
 * @{
 */
#define HRPWM_PWM_UP_MATCH_SPECIAL_B_LOW    (0x00UL << HRPWM_PCNAR2_SCMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_B_HIGH   (0x01UL << HRPWM_PCNAR2_SCMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_B_HOLD   (0x02UL << HRPWM_PCNAR2_SCMBUCA_POS)
#define HRPWM_PWM_UP_MATCH_SPECIAL_B_INVT   (0x03UL << HRPWM_PCNAR2_SCMBUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Match_Special_B_Define HRPWM Pin Polarity For Count Down And Match SCMBR Define
 * @{
 */
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_B_LOW  (0x00UL << HRPWM_PCNAR3_SCMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_B_HIGH (0x01UL << HRPWM_PCNAR3_SCMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_B_HOLD (0x02UL << HRPWM_PCNAR3_SCMBDCA_POS)
#define HRPWM_PWM_DOWN_MATCH_SPECIAL_B_INVT (0x03UL << HRPWM_PCNAR3_SCMBDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Ext_Event_Num_Define HRPWM External Event Number Define for Pin Polarity set
 * @{
 */
#define HRPWM_PWM_EXT_EVT_NUM1              (HRPWM_PCNAR2_EXEV1UCA)
#define HRPWM_PWM_EXT_EVT_NUM2              (HRPWM_PCNAR2_EXEV2UCA)
#define HRPWM_PWM_EXT_EVT_NUM3              (HRPWM_PCNAR2_EXEV3UCA)
#define HRPWM_PWM_EXT_EVT_NUM4              (HRPWM_PCNAR2_EXEV4UCA)
#define HRPWM_PWM_EXT_EVT_NUM5              (HRPWM_PCNAR2_EXEV5UCA)
#define HRPWM_PWM_EXT_EVT_NUM6              (HRPWM_PCNAR2_EXEV6UCA)
#define HRPWM_PWM_EXT_EVT_NUM7              (HRPWM_PCNAR2_EXEV7UCA)
#define HRPWM_PWM_EXT_EVT_NUM8              (HRPWM_PCNAR2_EXEV8UCA)
#define HRPWM_PWM_EXT_EVT_NUM9              (HRPWM_PCNAR2_EXEV9UCA)
#define HRPWM_PWM_EXT_EVT_NUM10             (HRPWM_PCNAR2_EXEV10UCA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Ext_Event_Define HRPWM Pin Polarity for External Event Define
 * @{
 */
#define HRPWM_PWM_EXT_EVT_LOW               (0x00000000UL)
#define HRPWM_PWM_EXT_EVT_HIGH              (0x00055555UL)
#define HRPWM_PWM_EXT_EVT_HOLD              (0x000AAAAAUL)
#define HRPWM_PWM_EXT_EVT_INVT              (0x000FFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Force_Polarity_Define HRPWM PWM Pin Force Polarity At Next Complete Period Point Define
 * @{
 */
#define HRPWM_PWM_FORCE_LOW                 (0x02UL << HRPWM_PCNAR1_FORCA_POS)
#define HRPWM_PWM_FORCE_HIGH                (0x03UL << HRPWM_PCNAR1_FORCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PWM_Ch_Swap_Mode_Define HRPWM PWM Channel Swap Mode Define
 * @{
 */
#define HRPWM_PWM_CH_SWAP_MD_NOT_IMMED      (0x00UL)                /*!< Channel swap at complete period point or not swap */
#define HRPWM_PWM_CH_SWAP_MD_IMMED          (HRPWM_GCONR1_SWAPMD)   /*!< Channel swap immediately */
/**
 * @}
 */

/**
 * @defgroup HRPWM_PWM_Ch_Swap_Func_Define HRPWM PWM Channel Swap Function Define
 * @{
 */
#define HRPWM_PWM_CH_SWAP_OFF               (0x00UL)
#define HRPWM_PWM_CH_SWAP_ON                (HRPWM_GCONR1_SWAPEN)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PWM_ChA_Invert_Define HRPWM PWM Channel A Invert Define
 * @{
 */
#define HRPWM_PWM_CHA_INVT_OFF              (0x00UL)
#define HRPWM_PWM_CHA_INVT_ON               (HRPWM_GCONR1_INVCAEN)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PWM_ChB_Invert_Define HRPWM PWM Channel B Invert Define
 * @{
 */
#define HRPWM_PWM_CHB_INVT_OFF              (0x00UL)
#define HRPWM_PWM_CHB_INVT_ON               (HRPWM_GCONR1_INVCBEN)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Match_Special_Event_Define HRPWM Match Special Register Event Define
 * @{
 */
#define HRPWM_EVT_UP_MATCH_SPECIAL_A        (HRPWM_GCONR1_CMSCAUEN)
#define HRPWM_EVT_DOWN_MATCH_SPECIAL_A      (HRPWM_GCONR1_CMSCADEN)
#define HRPWM_EVT_UP_MATCH_SPECIAL_B        (HRPWM_GCONR1_CMSCBUEN)
#define HRPWM_EVT_DOWN_MATCH_SPECIAL_B      (HRPWM_GCONR1_CMSCBDEN)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Complete_Period_Point_Define HRPWM Complete Period Point define
 * @{
 */
#define HRPWM_CPLT_PERIOD_SAWTOOTH_PEAK_TRIANGLE_VALLEY (0x00UL)    /*!< Sawtooth: count peak
                                                                         Triangle: count valley */
#define HRPWM_CPLT_PERIOD_PEAK              (HRPWM_GCONR1_PRDSEL_0) /*!< Count peak */
#define HRPWM_CPLT_PERIOD_VALLEY            (HRPWM_GCONR1_PRDSEL_1) /*!< Count valley */
#define HRPWM_CPLT_PERIOD_PEAK_OR_VALLEY    (HRPWM_GCONR1_PRDSEL)   /*!< Count peak or Count valley */
/**
 * @}
 */

/**
 * @defgroup HRPWM_PWM_PH_Period_Link_Define HRPWM Phase Period Link Function Define
 * @{
 */
#define HRPWM_PH_PERIOD_LINK_OFF            (0x00UL)
#define HRPWM_PH_PERIOD_LINK_ON             (HRPWM_GCONR1_PRDLK)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_Buf_Flag_Src_Define HRPWM Global Buffer Flag Source define
 * @{
 */
#define HRPWM_GLOBAL_BUF_SRC_CNT_PEAK       (0x01UL << HRPWM_GCONR1_BFSEL_POS)  /*!< Count peak */
#define HRPWM_GLOBAL_BUF_SRC_CNT_VALLEY     (0x02UL << HRPWM_GCONR1_BFSEL_POS)  /*!< Count valley */
#define HRPWM_GLOBAL_BUF_SRC_U1_SINGLE      (0x04UL << HRPWM_GCONR1_BFSEL_POS)  /*!< HRPWM1 buffer single transfer occurs */
#define HRPWM_GLOBAL_BUF_SRC_AFTER_U1_SINGLE (0x08UL << HRPWM_GCONR1_BFSEL_POS) /*!< After HRPWM1 buffer single transfer occurs, and \
                                                                                     Sawtooth: count peak \
                                                                                     Triangle: count peak and valley */
#define HRPWM_GLOBAL_BUF_SRC_U1_ALL         (HRPWM_GLOBAL_BUF_SRC_CNT_PEAK | HRPWM_GLOBAL_BUF_SRC_CNT_VALLEY | \
                                             HRPWM_GLOBAL_BUF_SRC_U1_SINGLE)
#define HRPWM_GLOBAL_BUF_SRC_U2_6_ALL       (HRPWM_GLOBAL_BUF_SRC_CNT_PEAK | HRPWM_GLOBAL_BUF_SRC_CNT_VALLEY | \
                                             HRPWM_GLOBAL_BUF_SRC_U1_SINGLE | HRPWM_GLOBAL_BUF_SRC_AFTER_U1_SINGLE)
/**F
 * @}
 */

/**
 * @defgroup HRPWM_Idle_Delay_Trigger_Src_Define HRPWM Idle Delay Output Trigger Source Define
 * @{
 */
#define HRPWM_IDLE_DELAY_TRIG_EVT6          (0x00UL)                               /*!< External event 6 */
#define HRPWM_IDLE_DELAY_TRIG_EVT7          (0x01UL << HRPWM_IDLECR_DLYEVSEL_POS)  /*!< External event 7 */
#define HRPWM_IDLE_DELAY_TRIG_EVT8          (0x02UL << HRPWM_IDLECR_DLYEVSEL_POS)  /*!< External event 8 */
#define HRPWM_IDLE_DELAY_TRIG_EVT9          (0x03UL << HRPWM_IDLECR_DLYEVSEL_POS)  /*!< External event 9 */
#define HRPWM_IDLE_DELAY_TRIG_SW            (0x04UL << HRPWM_IDLECR_DLYEVSEL_POS)  /*!< Software trigger source, DLYSTRG = 1 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_ChA_Level_Define HRPWM Idle Channel A Output Level Define
 * @{
 */
#define HRPWM_IDLE_CHA_LVL_LOW              (0x00UL)
#define HRPWM_IDLE_CHA_LVL_HIGH             (HRPWM_IDLECR_IDLESA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_ChB_Level_Define HRPWM Idle Channel B Output Level Define
 * @{
 */
#define HRPWM_IDLE_CHB_LVL_LOW              (0x00UL)
#define HRPWM_IDLE_CHB_LVL_HIGH             (HRPWM_IDLECR_IDLESB)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_Output_ChA_Status_Define HRPWM Idle Output Channel A status Define
 * @{
 */
#define HRPWM_IDLE_OUTPUT_CHA_OFF           (0x00UL)                /*!< Channel A idle output disable */
#define HRPWM_IDLE_OUTPUT_CHA_ON            (HRPWM_IDLECR_DLYCHA)   /*!< Channel A idle output enable */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_Output_ChB_Status_Define HRPWM Idle Output Channel B status Define
 * @{
 */
#define HRPWM_IDLE_OUTPUT_CHB_OFF           (0x00UL)                /*!< Channel B idle output disable */
#define HRPWM_IDLE_OUTPUT_CHB_ON            (HRPWM_IDLECR_DLYCHB)   /*!< Channel B idle output enable */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_ChA_BM_Output_Delay_Define HRPWM Idle Channel A BM Output Delay Enter Function Define
 * @{
 */
#define HRPWM_BM_DELAY_ENTER_CHA_OFF          (0x00UL)                /*!< Channel enter idle state immediately */
#define HRPWM_BM_DELAY_ENTER_CHA_ON           (HRPWM_IDLECR_DIDLA)    /*!< Channel delay enter idle state */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_ChB_BM_Output_Delay_Define HRPWM Idle Channel B BM Output Delay Enter Function Define
 * @{
 */
#define HRPWM_BM_DELAY_ENTER_CHB_OFF          (0x00UL)                /*!< Channel enter idle state immediately */
#define HRPWM_BM_DELAY_ENTER_CHB_ON           (HRPWM_IDLECR_DIDLB)    /*!< Channel delay enter idle state */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Action_Mode_Define HRPWM BM Action Mode Define
 * @{
 */
#define HRPWM_BM_ACTION_MD1                 (0x00UL)                /*!< BM Action Mode1, BM output idle during (BMPCMAR+1)*T(BM clock source) */
#define HRPWM_BM_ACTION_MD2                 (HRPWM_COMMON_BMCR_BMMD)/*!< BM Action Mode2, BM output idle during BMPCMAR*T(BM clock source)  */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Output_ChA_Status_Define HRPWM Idle BM Output Channel A status Define
 * @{
 */
#define HRPWM_BM_OUTPUT_CHA_OFF             (0x00UL)                /*!< Channel A BM output disable */
#define HRPWM_BM_OUTPUT_CHA_ON              (HRPWM_IDLECR_IDLEBMA)  /*!< Channel A BM output enable */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Output_ChB_Status_Define HRPWM Idle BM Output Channel B status Define
 * @{
 */
#define HRPWM_BM_OUTPUT_CHB_OFF             (0x00UL)                /*!< Channel B BM output disable */
#define HRPWM_BM_OUTPUT_CHB_ON              (HRPWM_IDLECR_IDLEBMB)  /*!< Channel B BM output enable */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Output_Follow_Func_Define HRPWM Idle BM Output channel follow function Define
 * @{
 */
#define HRPWM_BM_FOLLOW_FUNC_OFF            (0x00UL)                /*!< Channel follow function disable */
#define HRPWM_BM_FOLLOW_FUNC_ON             (HRPWM_IDLECR_FOLLOW)   /*!< Channel follow function enable */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Unit_Count_Reset_Define HRPWM Count Reset in BM Output idle state Define
 * @{
 */
#define HRPWM_BM_UNIT_CNT_CONTINUE          (0x00UL)    /*!< HRPWM Unit count continue in the BM output idle state */
#define HRPWM_BM_UNIT_CNT_STP_RST           (0x01UL)    /*!< HRPWM Unit count stop and reset in the BM output idle state */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Input_Filter_Clock_Define HRPWM Input Pin Filter Clock Divider Define
 * @{
 */
#define HRPWM_FILTER_CLK_DIV1               (0x00UL)    /*!< PCLK0 */
#define HRPWM_FILTER_CLK_DIV4               (0x01UL)    /*!< PCLK0 / 4 */
#define HRPWM_FILTER_CLK_DIV16              (0x02UL)    /*!< PCLK0 / 16 */
#define HRPWM_FILTER_CLK_DIV64              (0x03UL)    /*!< PCLK0 / 64 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Count_Status_Flag_Define HRPWM Count Status Flag Define
 * @{
 */
#define HRPWM_FLAG_MATCH_A                  ((uint64_t)HRPWM_STFLR1_CMAF)       /*!< Match HRGCMAR register */
#define HRPWM_FLAG_MATCH_B                  ((uint64_t)HRPWM_STFLR1_CMBF)       /*!< Match HRGCMBR register */
#define HRPWM_FLAG_MATCH_C                  ((uint64_t)HRPWM_STFLR1_CMCF)       /*!< Match HRGCMCR register */
#define HRPWM_FLAG_MATCH_D                  ((uint64_t)HRPWM_STFLR1_CMDF)       /*!< Match HRGCMDR register */
#define HRPWM_FLAG_MATCH_E                  ((uint64_t)HRPWM_STFLR1_CMEF)       /*!< Match HRGCMER register */
#define HRPWM_FLAG_MATCH_F                  ((uint64_t)HRPWM_STFLR1_CMFF)       /*!< Match HRGCMFR register */
#define HRPWM_FLAG_CNT_PEAK                 ((uint64_t)HRPWM_STFLR1_OVFF)       /*!< Count peak */
#define HRPWM_FLAG_CNT_VALLEY               ((uint64_t)HRPWM_STFLR1_UDFF)       /*!< Count valley */
#define HRPWM_FLAG_UP_MATCH_SPECIAL_A       ((uint64_t)HRPWM_STFLR1_CMSAUF)     /*!< Match SCMAR register when count up */
#define HRPWM_FLAG_DOWN_MATCH_SPECIAL_A     ((uint64_t)HRPWM_STFLR1_CMSADF)     /*!< Match SCMAR register when count down */
#define HRPWM_FLAG_UP_MATCH_SPECIAL_B       ((uint64_t)HRPWM_STFLR1_CMSBUF)     /*!< Match SCMBR register when count up */
#define HRPWM_FLAG_DOWN_MATCH_SPECIAL_B     ((uint64_t)HRPWM_STFLR1_CMSBDF)     /*!< Match SCMBR register when count down */
#define HRPWM_FLAG_CAPT_A                   ((uint64_t)HRPWM_STFLR1_CAPAF)      /*!< Capture A */
#define HRPWM_FLAG_CAPT_B                   ((uint64_t)HRPWM_STFLR1_CAPBF)      /*!< Capture B */
#define HRPWM_FLAG_CNT_DIR                  ((uint64_t)HRPWM_STFLR1_DIRF)       /*!< Count direction */
#define HRPWM_FLAG_ONE_SHOT_CPLT            ((uint64_t)HRPWM_STFLR2_OSTOVF << 32U) /*!< Count complete in one shot timer mode */

#define HRPWM_FLAG_CLR_ALL                  (HRPWM_FLAG_MATCH_A | HRPWM_FLAG_MATCH_B | HRPWM_FLAG_MATCH_C | \
                                            HRPWM_FLAG_MATCH_D | HRPWM_FLAG_MATCH_E | HRPWM_FLAG_MATCH_F | \
                                            HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY | HRPWM_FLAG_UP_MATCH_SPECIAL_A | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_A | HRPWM_FLAG_UP_MATCH_SPECIAL_B | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_B | HRPWM_FLAG_CAPT_A | \
                                            HRPWM_FLAG_CAPT_B | HRPWM_FLAG_ONE_SHOT_CPLT)
#define HRPWM_FLAG_ALL                      (HRPWM_FLAG_MATCH_A | HRPWM_FLAG_MATCH_B | HRPWM_FLAG_MATCH_C | \
                                            HRPWM_FLAG_MATCH_D | HRPWM_FLAG_MATCH_E | HRPWM_FLAG_MATCH_F | \
                                            HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY | HRPWM_FLAG_UP_MATCH_SPECIAL_A | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_A | HRPWM_FLAG_UP_MATCH_SPECIAL_B | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_B | HRPWM_FLAG_CAPT_A | \
                                            HRPWM_FLAG_CAPT_B | HRPWM_FLAG_CNT_DIR | HRPWM_FLAG_ONE_SHOT_CPLT)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PH_Status_Flag_Define HRPWM phase Status Flag Define
 * @{
 */
#define HRPWM_PH_FLAG_MATCH_PH_EVT1         (HRPWM_STFLR2_PHSCMP1F)     /*!< Phase match event 1*/
#define HRPWM_PH_FLAG_MATCH_PH_EVT2         (HRPWM_STFLR2_PHSCMP2F)     /*!< Phase match event 2*/
#define HRPWM_PH_FLAG_MATCH_PH_EVT3         (HRPWM_STFLR2_PHSCMP3F)     /*!< Phase match event 3*/
#define HRPWM_PH_FLAG_MATCH_PH_EVT4         (HRPWM_STFLR2_PHSCMP4F)     /*!< Phase match event 4*/
#define HRPWM_PH_FLAG_MATCH_PH_EVT5         (HRPWM_STFLR2_PHSCMP5F)     /*!< Phase match event 5*/
#define HRPWM_PH_FLAG_ALL                   (HRPWM_PH_FLAG_MATCH_PH_EVT1 | HRPWM_PH_FLAG_MATCH_PH_EVT2 | \
                                            HRPWM_PH_FLAG_MATCH_PH_EVT3 | HRPWM_PH_FLAG_MATCH_PH_EVT4 | \
                                            HRPWM_PH_FLAG_MATCH_PH_EVT5)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_Delay_Flag_Define HRPWM Idle Delay Status Flag Define
 * @{
 */
#define HRPWM_IDLE_DELAY_FLAG_CHA_LVL       (HRPWM_STFLR2_OASTAT)       /*!< PWM Channel A level when enter idle state */
#define HRPWM_IDLE_DELAY_FLAG_CHB_LVL       (HRPWM_STFLR2_OBSTAT)       /*!< PWM Channel B level when enter idle state */
#define HRPWM_IDLE_DELAY_FLAG_STAT          (HRPWM_STFLR2_DLYPRT)       /*!< Idle delay status */
#define HRPWM_IDLE_DELAY_FLAG_STAT_CHA      (HRPWM_STFLR2_DLYIDLEA)     /*!< Channel A idle status(If channel A enter idle state) */
#define HRPWM_IDLE_DELAY_FLAG_STAT_CHB      (HRPWM_STFLR2_DLYIDLEB)     /*!< Channel B idle status(If channel B enter idle state) */
#define HRPWM_IDLE_DELAY_FLAG_ALL           (HRPWM_IDLE_DELAY_FLAG_CHA_LVL | HRPWM_IDLE_DELAY_FLAG_CHB_LVL | \
                                             HRPWM_IDLE_DELAY_FLAG_STAT | HRPWM_IDLE_DELAY_FLAG_STAT_CHA | \
                                             HRPWM_IDLE_DELAY_FLAG_STAT_CHB)

#define HRPWM_IDLE_DELAY_FLAG_CLR_ALL       (HRPWM_STFLR2_DLYPRT)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Valid_Period_Count_Cond_Define HRPWM Valid Period Function Count Condition Define
 * @{
 */
#define HRPWM_VALID_PERIOD_INVD             (0x00UL)                /*!< Valid period function off */
#define HRPWM_VALID_PERIOD_CNT_VALLEY       (HRPWM_VPERR_PCNTE_0)   /*!< Sawtooth: count peak or hardware clear occurs\
                                                                         triangular: count valley */
#define HRPWM_VALID_PERIOD_CNT_PEAK         (HRPWM_VPERR_PCNTE_1)   /*!< Sawtooth: count peak or hardware clear occurs\
                                                                         triangular: count peak */
#define HRPWM_VALID_PERIOD_CNT_PEAK_VALLEY  (HRPWM_VPERR_PCNTE)     /*!< Sawtooth: count peak or hardware clear occurs\
                                                                         triangular: count peak and valley */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Valid_Period_Special_A_Define HRPWM Valid Period Function For Special A Define
 * @{
 */
#define HRPWM_VALID_PERIOD_SPECIAL_A_OFF    (0x00UL)                /*!< Valid period function special A function off */
#define HRPWM_VALID_PERIOD_SPECIAL_A_ON     (HRPWM_VPERR_SPPERIA)   /*!< Valid period function special A function on */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Valid_Period_Special_B_Define HRPWM Valid Period Function For Special B Define
 * @{
 */
#define HRPWM_VALID_PERIOD_SPECIAL_B_OFF    (0x00UL)                /*!< Valid period function special B function off */
#define HRPWM_VALID_PERIOD_SPECIAL_B_ON     (HRPWM_VPERR_SPPERIB)   /*!< Valid period function special B function on */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DeadTime_Reg_Equal_Func_Define HRPWM Dead Time Function DTDAR Equal DTUAR
 * @{
 */
#define HRPWM_DEADTIME_EQUAL_OFF            (0x00UL)
#define HRPWM_DEADTIME_EQUAL_ON             (HRPWM_DCONR_SEPA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_DeadTime_CountUp_Buf_Func_Define HRPWM Dead Time Buffer Function For Count Up Stage
 * @{
 */
#define HRPWM_DEADTIME_CNT_UP_BUF_OFF       (0x00UL)
#define HRPWM_DEADTIME_CNT_UP_BUF_ON        (HRPWM_DCONR_DTBENU)
/**
 * @}
 */

/**
 * @defgroup HRPWM_DeadTime_CountDown_Buf_Func_Define HRPWM Dead Time Buffer Function For Count Down Stage
 * @{
 */
#define HRPWM_DEADTIME_CNT_DOWN_BUF_OFF     (0x00UL)
#define HRPWM_DEADTIME_CNT_DOWN_BUF_ON      (HRPWM_DCONR_DTBEND)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Emb_Ch_Define HRPWM EMB Event Channel
 * @{
 */
#define HRPWM_EMB_EVT_CH0                   (0x00UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 0 */
#define HRPWM_EMB_EVT_CH1                   (0x01UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 1 */
#define HRPWM_EMB_EVT_CH2                   (0x02UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 2 */
#define HRPWM_EMB_EVT_CH3                   (0x03UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 3 */
#define HRPWM_EMB_EVT_CH4                   (0x04UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 4 */
#define HRPWM_EMB_EVT_CH5                   (0x05UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 5 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Emb_Release_Mode_Define HRPWM EMB Function Release Mode When EMB Event Invalid
 * @{
 */
#define HRPWM_EMB_RELEASE_IMMED             (0x00UL)                /*!< Release immediately */
#define HRPWM_EMB_RELEASE_PEAK              (HRPWM_PCNAR1_EMBRA_0)  /*!< Release when count peak */
#define HRPWM_EMB_RELEASE_VALLEY            (HRPWM_PCNAR1_EMBRA_1)  /*!< Release when count valley */
#define HRPWM_EMB_RELEASE_PEAK_VALLEY       (HRPWM_PCNAR1_EMBRA)    /*!< Release when count peak and valley */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Emb_Pin_Status_Define HRPWM Pin Output Status When EMB Event Valid
 * @{
 */
#define HRPWM_EMB_PIN_NORMAL                (0x00UL)
#define HRPWM_EMB_PIN_HIZ                   (HRPWM_PCNAR1_EMBCA_0)
#define HRPWM_EMB_PIN_LOW                   (HRPWM_PCNAR1_EMBCA_1)
#define HRPWM_EMB_PIN_HIGH                  (HRPWM_PCNAR1_EMBCA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Start_Condition_Define HRPWM Hardware Start Condition Define
 * @{
 */
#define HRPWM_HW_START_COND_NONE            ((uint64_t)0x00UL)
#define HRPWM_HW_START_COND_INTERN_EVT0     ((uint64_t)HRPWM_HSTAR1_HSTA8)
#define HRPWM_HW_START_COND_INTERN_EVT1     ((uint64_t)HRPWM_HSTAR1_HSTA9)
#define HRPWM_HW_START_COND_INTERN_EVT2     ((uint64_t)HRPWM_HSTAR1_HSTA10)
#define HRPWM_HW_START_COND_INTERN_EVT3     ((uint64_t)HRPWM_HSTAR1_HSTA11)
#define HRPWM_HW_START_COND_TRIGA_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA16)
#define HRPWM_HW_START_COND_TRIGA_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA17)
#define HRPWM_HW_START_COND_TRIGB_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA18)
#define HRPWM_HW_START_COND_TRIGB_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA19)
#define HRPWM_HW_START_COND_TRIGC_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA20)
#define HRPWM_HW_START_COND_TRIGC_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA21)
#define HRPWM_HW_START_COND_TRIGD_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA22)
#define HRPWM_HW_START_COND_TRIGD_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA23)
#define HRPWM_HW_START_COND_EXTEVT1         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV1 << 32U)
#define HRPWM_HW_START_COND_EXTEVT2         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV2 << 32U)
#define HRPWM_HW_START_COND_EXTEVT3         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV3 << 32U)
#define HRPWM_HW_START_COND_EXTEVT4         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV4 << 32U)
#define HRPWM_HW_START_COND_EXTEVT5         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV5 << 32U)
#define HRPWM_HW_START_COND_EXTEVT6         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV6 << 32U)
#define HRPWM_HW_START_COND_EXTEVT7         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV7 << 32U)
#define HRPWM_HW_START_COND_EXTEVT8         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV8 << 32U)
#define HRPWM_HW_START_COND_EXTEVT9         ((uint64_t)HRPWM_HSTAR2_HSTAEXEV9 << 32U)
#define HRPWM_HW_START_COND_EXTEVT10        ((uint64_t)HRPWM_HSTAR2_HSTAEXEV10 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA1 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB1 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA2 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB2 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA3 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB3 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA4 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB4 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA5 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB5 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_A      ((uint64_t)HRPWM_HSTAR2_HSTAGCMA6 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_B      ((uint64_t)HRPWM_HSTAR2_HSTAGCMB6 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB1 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB2 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB3 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB4 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB5 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HSTAR2_HSTASCMB6 << 32U)
#define HRPWM_HW_START_COND_ALL             ((uint64_t)0x00FF0F00UL | (uint64_t)0x0FFFFFFFUL << 32U)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Clear_Condition_Define HRPWM Hardware Clear Condition Define
 * @{
 */
#define HRPWM_HW_CLR_COND_NONE              ((uint64_t)0x00UL)
#define HRPWM_HW_CLR_COND_INTERN_EVT0       ((uint64_t)HRPWM_HCLRR1_HCLE8)
#define HRPWM_HW_CLR_COND_INTERN_EVT1       ((uint64_t)HRPWM_HCLRR1_HCLE9)
#define HRPWM_HW_CLR_COND_INTERN_EVT2       ((uint64_t)HRPWM_HCLRR1_HCLE10)
#define HRPWM_HW_CLR_COND_INTERN_EVT3       ((uint64_t)HRPWM_HCLRR1_HCLE11)
#define HRPWM_HW_CLR_COND_TRIGA_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE16)
#define HRPWM_HW_CLR_COND_TRIGA_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE17)
#define HRPWM_HW_CLR_COND_TRIGB_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE18)
#define HRPWM_HW_CLR_COND_TRIGB_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE19)
#define HRPWM_HW_CLR_COND_TRIGC_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE20)
#define HRPWM_HW_CLR_COND_TRIGC_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE21)
#define HRPWM_HW_CLR_COND_TRIGD_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE22)
#define HRPWM_HW_CLR_COND_TRIGD_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE23)
#define HRPWM_HW_CLR_COND_EXTEVT1           ((uint64_t)HRPWM_HCLRR2_HCLREXEV1 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT2           ((uint64_t)HRPWM_HCLRR2_HCLREXEV2 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT3           ((uint64_t)HRPWM_HCLRR2_HCLREXEV3 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT4           ((uint64_t)HRPWM_HCLRR2_HCLREXEV4 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT5           ((uint64_t)HRPWM_HCLRR2_HCLREXEV5 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT6           ((uint64_t)HRPWM_HCLRR2_HCLREXEV6 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT7           ((uint64_t)HRPWM_HCLRR2_HCLREXEV7 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT8           ((uint64_t)HRPWM_HCLRR2_HCLREXEV8 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT9           ((uint64_t)HRPWM_HCLRR2_HCLREXEV9 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT10          ((uint64_t)HRPWM_HCLRR2_HCLREXEV10 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA1 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB1 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA2 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB2 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA3 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB3 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA4 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB4 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA5 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB5 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_A        ((uint64_t)HRPWM_HCLRR2_HCLRGCMA6 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_B        ((uint64_t)HRPWM_HCLRR2_HCLRGCMB6 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB1 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB2 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB3 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB4 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB5 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLRSCMB6 << 32U)
#define HRPWM_HW_CLR_COND_ALL               ((uint64_t)0x00FF0F00UL | (uint64_t)0x0FFFFFFFUL << 32U)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Capture_Condition_Define HRPWM Hardware Capture Condition Define
 * @{
 */
#define HRPWM_HW_CAPT_COND_NONE             ((uint64_t)0x00UL)
#define HRPWM_HW_CAPT_COND_INTERN_EVT0      ((uint64_t)HRPWM_HCPAR1_HCPA8)
#define HRPWM_HW_CAPT_COND_INTERN_EVT1      ((uint64_t)HRPWM_HCPAR1_HCPA9)
#define HRPWM_HW_CAPT_COND_INTERN_EVT2      ((uint64_t)HRPWM_HCPAR1_HCPA10)
#define HRPWM_HW_CAPT_COND_INTERN_EVT3      ((uint64_t)HRPWM_HCPAR1_HCPA11)
#define HRPWM_HW_CAPT_COND_TRIGA_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA16)
#define HRPWM_HW_CAPT_COND_TRIGA_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA17)
#define HRPWM_HW_CAPT_COND_TRIGB_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA18)
#define HRPWM_HW_CAPT_COND_TRIGB_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA19)
#define HRPWM_HW_CAPT_COND_TRIGC_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA20)
#define HRPWM_HW_CAPT_COND_TRIGC_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA21)
#define HRPWM_HW_CAPT_COND_TRIGD_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA22)
#define HRPWM_HW_CAPT_COND_TRIGD_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA23)
#define HRPWM_HW_CAPT_COND_EXTEVT1          ((uint64_t)HRPWM_HCPAR2_HCPAEEV1 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT2          ((uint64_t)HRPWM_HCPAR2_HCPAEEV2 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT3          ((uint64_t)HRPWM_HCPAR2_HCPAEEV3 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT4          ((uint64_t)HRPWM_HCPAR2_HCPAEEV4 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT5          ((uint64_t)HRPWM_HCPAR2_HCPAEEV5 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT6          ((uint64_t)HRPWM_HCPAR2_HCPAEEV6 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT7          ((uint64_t)HRPWM_HCPAR2_HCPAEEV7 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT8          ((uint64_t)HRPWM_HCPAR2_HCPAEEV8 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT9          ((uint64_t)HRPWM_HCPAR2_HCPAEEV9 << 32U)
#define HRPWM_HW_CAPT_COND_EXTEVT10         ((uint64_t)HRPWM_HCPAR2_HCPAEEV10 << 32U)
#define HRPWM_HW_CAPT_COND_ALL              ((uint64_t)0x00FF0F00UL | (uint64_t)0x000003FFUL << 32U)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Num_Define HRPWM External Event Number Define
 * @{
 */
#define HRPWM_EVT1                          (0x00UL)
#define HRPWM_EVT2                          (0x01UL)
#define HRPWM_EVT3                          (0x02UL)
#define HRPWM_EVT4                          (0x03UL)
#define HRPWM_EVT5                          (0x04UL)
#define HRPWM_EVT6                          (0x05UL)
#define HRPWM_EVT7                          (0x06UL)
#define HRPWM_EVT8                          (0x07UL)
#define HRPWM_EVT9                          (0x08UL)
#define HRPWM_EVT10                         (0x09UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Src_Define HRPWM External Event Source Define
 * @{
 */
/*            _SRC1                _SRC2        _SRC3         _SRC4      */
/*-------------------------------------------------------------------*/
/* EVT1       HRPWM_EEV1(PC12)     EVT1_SRC2    TMR0_1CMPA    ADC1_CMP0  */
/* EVT2       HRPWM_EEV2(PC11)     EVT2_SRC2    TMR0_1CMPB    ADC1_CMP1  */
/* EVT3       HRPWM_EEV3(PB7)      EVT3_SRC2    TMR0_1CMPA               */
/* EVT4       HRPWM_EEV4(PB6)      EVT4_SRC2                  ADC2_CMP0  */
/* EVT5       HRPWM_EEV5(PB9)      EVT5_SRC2                  ADC2_CMP1  */
/* EVT6       HRPWM_EEV6(PB5)      EVT6_SRC2    TMR0_2CMPA               */
/* EVT7       HRPWM_EEV7(PB4)      EVT7_SRC2    TMR0_2CMPB    ADC3_CMP0  */
/* EVT8       HRPWM_EEV8(PB8)      EVT8_SRC2                  ADC3_CMP1  */
/* EVT9       HRPWM_EEV9(PB3)      EVT9_SRC2    TMR0_1CMPB               */
/* EVT10      HRPWM_EEV10(PC6)     EVT10_SRC2                            */

#define HRPWM_EVT_SRC1                      (0x00UL)
#define HRPWM_EVT_SRC2                      (0x01UL)
#define HRPWM_EVT_SRC3                      (0x02UL)
#define HRPWM_EVT_SRC4                      (0x03UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Src2_Define HRPWM External Event Source 2 Define
 * @{
 */
#define HRPWM_EVT_SRC2_CMP1                 (0x00UL)
#define HRPWM_EVT_SRC2_CMP2                 (0x01UL)
#define HRPWM_EVT_SRC2_CMP3                 (0x02UL)
#define HRPWM_EVT_SRC2_NONE                 (0x03UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Valid_Level_Define HRPWM External Event Valid Level Define
 * @{
 */
#define HRPWM_EVT_VALID_LVL_HIGH            (0x00UL)
#define HRPWM_EVT_VALID_LVL_LOW             (HRPWM_COMMON_EECR1_EE1POL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Fast_Async_Define HRPWM External Event Fast Asynchronous Mode Define
 * @{
 */
#define HRPWM_EVT_FAST_ASYNC_OFF            (0x00UL)
#define HRPWM_EVT_FAST_ASYNC_ON             (HRPWM_COMMON_EECR1_EE1FAST)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Generate_Detect_Fun_Define HRPWM External Event Generate Detect function Define
 * @{
 */
#define HRPWM_EVT_GEN_DETECT_OFF            (0x00UL)
#define HRPWM_EVT_GEN_DETECT_ON             (0x01UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Valid_Action_Define HRPWM External Event Valid Action Define
 * @{
 */
#define HRPWM_EVT_VALID_LVL                 (0x00UL)                /*!< The level configured by API HRPWM_EVT_SetValidLevel() */
#define HRPWM_EVT_VALID_RISING              (HRPWM_COMMON_EECR1_EE1SNS_0)  /*!< Rising edge */
#define HRPWM_EVT_VALID_FALLING             (HRPWM_COMMON_EECR1_EE1SNS_1)  /*!< falling edge */
#define HRPWM_EVT_VALID_BOTH                (HRPWM_COMMON_EECR1_EE1SNS)    /*!< Edge */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Eevs_Clock_Define HRPWM External Event Clock EEVS Define
 * @{
 */
#define HRPWM_EVT_EEVS_PCLK0                (0x00UL)
#define HRPWM_EVT_EEVS_PCLK0_DIV2           (HRPWM_COMMON_EECR3_EEVSD_0)
#define HRPWM_EVT_EEVS_PCLK0_DIV4           (HRPWM_COMMON_EECR3_EEVSD_1)
#define HRPWM_EVT_EEVS_PCLK0_DIV8           (HRPWM_COMMON_EECR3_EEVSD)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Clock_Define HRPWM External Event Filter Clock Define
 * @{
 */
#define HRPWM_EVT_FILTER_NONE               (0x00UL)
#define HRPWM_EVT_FILTER_PCLK0_DIV2         (0x01UL)
#define HRPWM_EVT_FILTER_PCLK0_DIV4         (0x02UL)
#define HRPWM_EVT_FILTER_PCLK0_DIV8         (0x03UL)
#define HRPWM_EVT_FILTER_EEVS_DIV12         (0x04UL)
#define HRPWM_EVT_FILTER_EEVS_DIV16         (0x05UL)
#define HRPWM_EVT_FILTER_EEVS_DIV24         (0x06UL)
#define HRPWM_EVT_FILTER_EEVS_DIV32         (0x07UL)
#define HRPWM_EVT_FILTER_EEVS_DIV48         (0x08UL)
#define HRPWM_EVT_FILTER_EEVS_DIV64         (0x09UL)
#define HRPWM_EVT_FILTER_EEVS_DIV80         (0x0AUL)
#define HRPWM_EVT_FILTER_EEVS_DIV96         (0x0BUL)
#define HRPWM_EVT_FILTER_EEVS_DIV128        (0x0CUL)
#define HRPWM_EVT_FILTER_EEVS_DIV160        (0x0DUL)
#define HRPWM_EVT_FILTER_EEVS_DIV192        (0x0EUL)
#define HRPWM_EVT_FILTER_EEVS_DIV256        (0x0FUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Mode_Define HRPWM External Event Filter Mode Define
 * @{
 */
#define HRPWM_EVT_FILTER_OFF                (0x00UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Filter mode off */
#define HRPWM_EVT_FILTER_MD_BLANK           (0x01UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when this HRPWM blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_U1        (0x02UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, \
                                                                                      The event shall be ignored when the HRPWM1 blank signal is low(filter signal replace mode 1) \
                                                                                      The event shall be ignored during count peak and offset value(filter signal replace mode 2 with reference point count peak) \
                                                                                      The event shall be ignored during count valley and offset value(filter signal replace mode 2 with reference point count valley) */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM1_B  (0x03UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, \
                                                                                      The event shall be ignored when the HRPWM1 PWMB signal is low \
                                                                                      The event shall be ignored during count peak and window value(filter signal replace mode 2 with reference point count peak) \
                                                                                      The event shall be ignored during count valley and window value(filter signal replace mode 2 with reference point count valley) */
#define HRPWM_EVT_FILTER_MD_BLANK_U2        (0x04UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM2 blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM2_B  (0x05UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM2 PWMB signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_U3        (0x06UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM3 blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM3_B  (0x07UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM3 PWMB signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_U4        (0x08UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM4 blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM4_B  (0x09UL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM4 PWMB signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_U5        (0x0AUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM5 blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM5_B  (0x0BUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM5 PWMB signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_U6        (0x0CUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM6 blank signal is low */
#define HRPWM_EVT_FILTER_MD_BLANK_HRPWM6_B  (0x0DUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Blank mode, The event shall be ignored when the HRPWM6 PWMB signal is low */
#define HRPWM_EVT_FILTER_MD_WIN             (0x0EUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Window mode, \
                                                                                      The event valid when this HRPWM window signal is high(filter signal replace mode 1) \
                                                                                      The event valid during count peak and offset value(filter signal replace mode 2 with reference point count peak) \
                                                                                      The event valid during count valley and offset value(filter signal replace mode 2 with reference point count valley) */
#define HRPWM_EVT_FILTER_MD_WIN_OTHER       (0x0FUL << HRPWM_EEFLTCR1_EE1FM_POS) /*!< Window mode, \
                                                                                      The event valid when another HRPWM window signal is high(filter signal replace mode 1) \
                                                                                            HRPWM1 use HRPWM2 window signal \
                                                                                            HRPWM2 use HRPWM1 window signal \
                                                                                            HRPWM3 use HRPWM4 window signal \
                                                                                            HRPWM4 use HRPWM3 window signal \
                                                                                            HRPWM5 use HRPWM6 window signal \
                                                                                            HRPWM6 use HRPWM5 window signal \
                                                                                      The event valid during count peak and window value(filter signal replace mode 2 with reference point count peak) \
                                                                                      The event valid during count valley and window value(filter signal replace mode 2 with reference point count valley)*/

/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Signal_Replace_Define HRPWM External Event Filter Signal Replace Mode Define
 * @{
 */
#define HRPWM_EVT_FILTER_REPLACE_MD1        (0x00UL)                            /*!< Filter signal replace mode 1 */
#define HRPWM_EVT_FILTER_REPLACE_MD2_PEAK   (HRPWM_GCONR1_EEFM)                 /*!< Filter signal replace mode 2 with reference point count peak */
#define HRPWM_EVT_FILTER_REPLACE_MD2_VALLEY (HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF) /*!< Filter signal replace mode 2 with reference point count valley */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Init_Polarity_Define HRPWM External Event Filter Signal Initialization Polarity Define
 * @{
 */
#define HRPWM_EVT_FILTER_INIT_POLARITY_LOW  (0x00UL)
#define HRPWM_EVT_FILTER_INIT_POLARITY_HIGH (HRPWM_EEFLTCR1_EEINTPOL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Latch_Func_Define HRPWM External Event Filter Event Latch Function Define
 * @{
 */
#define HRPWM_EVT_FILTER_LATCH_OFF          (0x00UL)
#define HRPWM_EVT_FILTER_LATCH_ON           (HRPWM_EEFLTCR1_EE1LAT)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Timeout_Func_Define HRPWM External Event Filter Event Timeout Function Define
 * @{
 */
#define HRPWM_EVT_FILTER_TIMEOUT_OFF        (0x00UL)
#define HRPWM_EVT_FILTER_TIMEOUT_ON         (HRPWM_EEFLTCR1_EE1TMO)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Offset_Dir_Define HRPWM External Event Filter Offset Direction Define
 * @{
 */
#define HRPWM_EVT_FILTER_OFS_DIR_DOWN       (0x00UL)
#define HRPWM_EVT_FILTER_OFS_DIR_UP         (HRPWM_EEFOFFSETAR_OFFSETDIR)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Window_Dir_Define HRPWM External Event Filter Window Direction Define
 * @{
 */
#define HRPWM_EVT_FILTER_WIN_DIR_DOWN       (0x00UL)
#define HRPWM_EVT_FILTER_WIN_DIR_UP         (HRPWM_EEFWINAR_WINDIR)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Count_Src_Define HRPWM Idle BM Count Source Define
 * @{
 */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U1    (0x00UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM1 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U2    (0x01UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM2 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U3    (0x02UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM3 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U4    (0x03UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM4 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U5    (0x04UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM5 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U6    (0x05UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM6 complete period point */
#define HRPWM_BM_CNT_SRC_TMR0_1_CMPA        (0x06UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< Event TMR0_1_CMPA */
#define HRPWM_BM_CNT_SRC_TMR0_1_CMPB        (0x07UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< Event TMR0_1_CMPB */
#define HRPWM_BM_CNT_SRC_TMR0_2_CMPA        (0x08UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< Event TMR0_2_CMPA */
#define HRPWM_BM_CNT_SRC_TMR0_2_CMPB        (0x09UL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< Event TMR0_2_CMPB */
#define HRPWM_BM_CNT_SRC_PCLK               (0x0AUL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< Pclk0 or divider from pclk0 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Count_Src_Pclk_Define HRPWM Idle BM Count Source Pclk Divider Define
 * @{
 */
#define HRPWM_BM_CNT_SRC_PCLK_DIV1          (0x00UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV2          (0x01UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV4          (0x02UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV8          (0x03UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV16         (0x04UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV32         (0x05UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV64         (0x06UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV128        (0x07UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV256        (0x08UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV512        (0x09UL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV1024       (0x0AUL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV2048       (0x0BUL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV4096       (0x0CUL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV8192       (0x0DUL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV16384      (0x0EUL << HRPWM_COMMON_BMCR_BMPSC_POS)
#define HRPWM_BM_CNT_SRC_PCLK_DIV32768      (0x0FUL << HRPWM_COMMON_BMCR_BMPSC_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Count_Reload_Define HRPWM Idle BM count Stop After Overflow Function Define
 * @{
 */
#define HRPWM_BM_CNT_RELOAD_OFF             (0x00UL)
#define HRPWM_BM_CNT_RELOAD_ON              (HRPWM_COMMON_BMCR_BMCTN)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Trigger_Src_Define HRPWM Idle BM Trigger Source Define
 * @{
 */
#define HRPWM_BM_TRIG_NONE                 ((uint64_t)0x00UL)
#define HRPWM_BM_TRIG_EVT8                 ((uint64_t)HRPWM_COMMON_BMSTRG1_EEV8)        /*!< External event 8(Filter by HRPWM4) */
#define HRPWM_BM_TRIG_EVT7                 ((uint64_t)HRPWM_COMMON_BMSTRG1_EEV7)        /*!< External event 7(Filter by HRPWM1) */
#define HRPWM_BM_TRIG_EVT8_AND_U4_VALLEY   ((uint64_t)HRPWM_COMMON_BMSTRG1_T4UDFEEV8)   /*!< HRPWM4 count valley after external event 8 */
#define HRPWM_BM_TRIG_EVT8_AND_U4_PEAK     ((uint64_t)HRPWM_COMMON_BMSTRG1_T4OVFEEV8)   /*!< HRPWM4 count peak after external event 8 */
#define HRPWM_BM_TRIG_EVT7_AND_U1_VALLEY   ((uint64_t)HRPWM_COMMON_BMSTRG1_T4UDFEEV8)   /*!< HRPWM1 count valley after external event 7 */
#define HRPWM_BM_TRIG_EVT7_AND_U1_PEAK     ((uint64_t)HRPWM_COMMON_BMSTRG1_T4OVFEEV8)   /*!< HRPWM1 count peak after external event 7 */
#define HRPWM_BM_TRIG_U6_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB6)       /*!< HRPWM6 match HRGCMBR*/
#define HRPWM_BM_TRIG_U6_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA6)       /*!< HRPWM6 match HRGCMAR*/
#define HRPWM_BM_TRIG_U6_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF6)        /*!< HRPWM6 count valley */
#define HRPWM_BM_TRIG_U6_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF6)        /*!< HRPWM6 count peak */
#define HRPWM_BM_TRIG_U5_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB5)       /*!< HRPWM5 match HRGCMBR */
#define HRPWM_BM_TRIG_U5_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA5)       /*!< HRPWM5 match HRGCMAR */
#define HRPWM_BM_TRIG_U5_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF5)        /*!< HRPWM5 count valley */
#define HRPWM_BM_TRIG_U5_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF5)        /*!< HRPWM5 count peak */
#define HRPWM_BM_TRIG_U4_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB4)       /*!< HRPWM4 match HRGCMBR */
#define HRPWM_BM_TRIG_U4_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA4)       /*!< HRPWM4 match HRGCMAR */
#define HRPWM_BM_TRIG_U4_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF4)        /*!< HRPWM4 count valley */
#define HRPWM_BM_TRIG_U4_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF4)        /*!< HRPWM4 count peak */
#define HRPWM_BM_TRIG_U3_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB3)       /*!< HRPWM3 match HRGCMBR */
#define HRPWM_BM_TRIG_U3_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA3)       /*!< HRPWM3 match HRGCMAR */
#define HRPWM_BM_TRIG_U3_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF3)        /*!< HRPWM3 count valley */
#define HRPWM_BM_TRIG_U3_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF3)        /*!< HRPWM3 count peak */
#define HRPWM_BM_TRIG_U2_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB2)       /*!< HRPWM2 match HRGCMBR */
#define HRPWM_BM_TRIG_U2_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA2)       /*!< HRPWM2 match HRGCMAR */
#define HRPWM_BM_TRIG_U2_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF2)        /*!< HRPWM2 count valley */
#define HRPWM_BM_TRIG_U2_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF2)        /*!< HRPWM2 count peak */
#define HRPWM_BM_TRIG_U1_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMB1)       /*!< HRPWM1 match HRGCMBR */
#define HRPWM_BM_TRIG_U1_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG1_GCMA1)       /*!< HRPWM1 match HRGCMAR */
#define HRPWM_BM_TRIG_U1_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG1_UDF1)        /*!< HRPWM1 count valley */
#define HRPWM_BM_TRIG_U1_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG1_OVF1)        /*!< HRPWM1 count peak */
#define HRPWM_BM_TRIG_TMR0_2_CMPB          ((uint64_t)HRPWM_COMMON_BMSTRG2_IEV3 << 32U) /*!< TMR0_2_CMPB */
#define HRPWM_BM_TRIG_TMR0_2_CMPA          ((uint64_t)HRPWM_COMMON_BMSTRG2_IEV2 << 32U) /*!< TMR0_2_CMPA */
#define HRPWM_BM_TRIG_TMR0_1_CMPB          ((uint64_t)HRPWM_COMMON_BMSTRG2_IEV1 << 32U) /*!< TMR0_1_CMPB */
#define HRPWM_BM_TRIG_TMR0_1_CMPA          ((uint64_t)HRPWM_COMMON_BMSTRG2_IEV0 << 32U) /*!< TMR0_1_CMPA */
#define HRPWM_BM_TRIG_ALL                  ((uint64_t)0x7FFFFFFEUL | ((uint64_t)0x000000FFUL << 32U))
/**
 * @}
 */

/**
 * @defgroup HRPWM_Idle_BM_Flag_Define HRPWM Idle BM Mode Status Flag Define
 * @{
 */
#define HRPWM_BM_FLAG_PEAK                  (HRPWM_COMMON_BMCR_BMOVFF)       /*!< BM count peak */
#define HRPWM_BM_FLAG_OP                    (HRPWM_COMMON_BMCR_BMOPTF)       /*!< BM mode in operation */
#define HRPWM_BM_FLAG_ALL                   (HRPWM_BM_FLAG_PEAK | HRPWM_BM_FLAG_OP)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PH_Force_ChA_Func_Define HRPWM Phase Match Force channel A Function Define
 * @{
 */
#define HRPWM_PH_MATCH_FORCE_CHA_OFF        (0x00UL)
#define HRPWM_PH_MATCH_FORCE_CHA_ON         (HRPWM_PHSCTL_PHSFORCA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PH_Force_ChB_Func_Define HRPWM Phase Match Force channel B Function Define
 * @{
 */
#define HRPWM_PH_MATCH_FORCE_CHB_OFF        (0x00UL)
#define HRPWM_PH_MATCH_FORCE_CHB_ON         (HRPWM_PHSCTL_PHSFORCB)
/**
 * @}
 */

/**
 * @defgroup HRPWM_PH_Index_Define HRPWM Phase Index Define
 * @{
 */
#define HRPWM_PH_MATCH_IDX1                 (0x00UL)
#define HRPWM_PH_MATCH_IDX2                 (0x01UL)
#define HRPWM_PH_MATCH_IDX3                 (0x02UL)
#define HRPWM_PH_MATCH_IDX4                 (0x03UL)
#define HRPWM_PH_MATCH_IDX5                 (0x04UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sync_Output_Pulse_Define HRPWM Synchronous Output Pulse Define
 * @{
 */
#define HRPWM_SYNC_PULSE_OFF                (0x00UL)                            /*!< Synchronous output invalid */
#define HRPWM_SYNC_PULSE_POSITIVE           (HRPWM_COMMON_SYNOCR_SYNOPLS_1)     /*!< Synchronous output positive pulse */
#define HRPWM_SYNC_PULSE_NEGATIVE           (HRPWM_COMMON_SYNOCR_SYNOPLS)       /*!< Synchronous output negative pulse */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sync_Output_Src_Define HRPWM Synchronous Output Source Define
 * @{
 */
#define HRPWM_SYNC_SRC_U1_CNT_VALLEY        (0x00UL)
#define HRPWM_SYNC_SRC_U2_CNT_VALLEY        (0x01UL)
#define HRPWM_SYNC_SRC_U3_CNT_VALLEY        (0x02UL)
#define HRPWM_SYNC_SRC_U4_CNT_VALLEY        (0x03UL)
#define HRPWM_SYNC_SRC_U5_CNT_VALLEY        (0x04UL)
#define HRPWM_SYNC_SRC_U6_CNT_VALLEY        (0x05UL)
#define HRPWM_SYNC_SRC_U1_MATCH_SPECIAL_B   (0x06UL)
#define HRPWM_SYNC_SRC_U2_MATCH_SPECIAL_B   (0x07UL)
#define HRPWM_SYNC_SRC_U3_MATCH_SPECIAL_B   (0x08UL)
#define HRPWM_SYNC_SRC_U4_MATCH_SPECIAL_B   (0x09UL)
#define HRPWM_SYNC_SRC_U5_MATCH_SPECIAL_B   (0x0AUL)
#define HRPWM_SYNC_SRC_U6_MATCH_SPECIAL_B   (0x0BUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sync_Output_Match_B_Dir_Define HRPWM Synchronous Output Match B Direction Define
 * @{
 */
#define HRPWM_SYNC_MATCH_B_DIR_DOWN         (0x00UL)
#define HRPWM_SYNC_MATCH_B_DIR_UP           (HRPWM_COMMON_SYNOCR_SRCDIR)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Match_Special_A_Event_Src_Define HRPWM Match Special A Event Source Define
 * @{
 */
#define HRPWM_SCMA_SRC_U1_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA1)      /*!< HRPWM1 match special A */
#define HRPWM_SCMA_SRC_U1_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB1)      /*!< HRPWM1 match special B */
#define HRPWM_SCMA_SRC_U1_CNT_PEAK          (HRPWM_SCMASELR_OVF1)       /*!< HRPWM1 count peak */
#define HRPWM_SCMA_SRC_U1_CNT_VALLEY        (HRPWM_SCMASELR_UDF1)       /*!< HRPWM1 count valley */
#define HRPWM_SCMA_SRC_U2_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA2)      /*!< HRPWM2 match special A */
#define HRPWM_SCMA_SRC_U2_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB2)      /*!< HRPWM2 match special B */
#define HRPWM_SCMA_SRC_U2_CNT_PEAK          (HRPWM_SCMASELR_OVF2)       /*!< HRPWM2 count peak */
#define HRPWM_SCMA_SRC_U2_CNT_VALLEY        (HRPWM_SCMASELR_UDF2)       /*!< HRPWM2 count valley */
#define HRPWM_SCMA_SRC_U3_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA3)      /*!< HRPWM3 match special A */
#define HRPWM_SCMA_SRC_U3_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB3)      /*!< HRPWM3 match special B */
#define HRPWM_SCMA_SRC_U3_CNT_PEAK          (HRPWM_SCMASELR_OVF3)       /*!< HRPWM3 count peak */
#define HRPWM_SCMA_SRC_U3_CNT_VALLEY        (HRPWM_SCMASELR_UDF3)       /*!< HRPWM3 count valley */
#define HRPWM_SCMA_SRC_U4_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA4)      /*!< HRPWM4 match special A */
#define HRPWM_SCMA_SRC_U4_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB4)      /*!< HRPWM4 match special B */
#define HRPWM_SCMA_SRC_U4_CNT_PEAK          (HRPWM_SCMASELR_OVF4)       /*!< HRPWM4 count peak */
#define HRPWM_SCMA_SRC_U4_CNT_VALLEY        (HRPWM_SCMASELR_UDF4)       /*!< HRPWM4 count valley */
#define HRPWM_SCMA_SRC_U5_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA5)      /*!< HRPWM5 match special A */
#define HRPWM_SCMA_SRC_U5_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB5)      /*!< HRPWM5 match special B */
#define HRPWM_SCMA_SRC_U5_CNT_PEAK          (HRPWM_SCMASELR_OVF5)       /*!< HRPWM5 count peak */
#define HRPWM_SCMA_SRC_U5_CNT_VALLEY        (HRPWM_SCMASELR_UDF5)       /*!< HRPWM5 count valley */
#define HRPWM_SCMA_SRC_U6_MATCH_SPECIAL_A   (HRPWM_SCMASELR_CMSA6)      /*!< HRPWM6 match special A */
#define HRPWM_SCMA_SRC_U6_MATCH_SPECIAL_B   (HRPWM_SCMASELR_CMSB6)      /*!< HRPWM6 match special B */
#define HRPWM_SCMA_SRC_U6_CNT_PEAK          (HRPWM_SCMASELR_OVF6)       /*!< HRPWM6 count peak */
#define HRPWM_SCMA_SRC_U6_CNT_VALLEY        (HRPWM_SCMASELR_UDF6)       /*!< HRPWM6 count valley */
#define HRPWM_SCMA_SRC_EXT_EVT_DETECT       (HRPWM_SCMASELR_EEVDET)     /*!< HRPWM external event detected */
#define HRPWM_SCMA_SRC_ALL                  (0x1FFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_DAC_Trigger_Src_Define HRPWM DAC Synchronous Trigger Source Define
 * @{
 */
#define HRPWM_DAC_TRIG_SRC_CNT_VALLEY               (0x00UL)                        /*!< Count valley */
#define HRPWM_DAC_TRIG_SRC_CNT_PEAK                 (0x01UL << HRPWM_CR_DACSRC_POS) /*!< Count peak */
#define HRPWM_DAC_TRIG_SRC_UP_MATCH_SPECIAL_A       (0x02UL << HRPWM_CR_DACSRC_POS) /*!< Count up match SCMA register */
#define HRPWM_DAC_TRIG_SRC_DOWN_MATCH_SPECIAL_A     (0x03UL << HRPWM_CR_DACSRC_POS) /*!< Count down match SCMA register */
#define HRPWM_DAC_TRIG_SRC_UP_MATCH_SPECIAL_B       (0x04UL << HRPWM_CR_DACSRC_POS) /*!< Count up match SCMB register */
#define HRPWM_DAC_TRIG_SRC_DOWN_MATCH_SPECIAL_B     (0x05UL << HRPWM_CR_DACSRC_POS) /*!< Count down match SCMB register */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DAC_Ch1_Trigger_Dest_Define HRPWM DAC Trigger Destination for DAC Channel 1 Define
 * @{
 */
#define HRPWM_DAC_CH1_TRIG_DEST_NONE        (0x00UL)                /*!< Do not trigger DAC channel 1 */
#define HRPWM_DAC_CH1_TRIG_DEST_DAC1        (HRPWM_CR_DACSYNC1_0)   /*!< Trigger DAC1 channel 1 */
#define HRPWM_DAC_CH1_TRIG_DEST_DAC2        (HRPWM_CR_DACSYNC1_1)   /*!< Trigger DAC2 channel 1 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DAC_Ch2_Trigger_Dest_Define HRPWM DAC Trigger Destination for DAC Channel 2 Define
 * @{
 */
#define HRPWM_DAC_CH2_TRIG_DEST_NONE        (0x00UL)                /*!< Do not trigger DAC channel 2 */
#define HRPWM_DAC_CH2_TRIG_DEST_DAC1        (HRPWM_CR_DACSYNC2_0)   /*!< Trigger DAC1 channel 2 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Capture_Dir_Define HRPWM Capture Direction Define
 * @{
 */
#define HRPWM_CAPT_DIR_UP                   (0x00UL)
#define HRPWM_CAPT_DIR_DOWN                 (HRPWM_CAPAR_CAPDIRA)
/**
 * @}
 */

/**
 * @}
 */

/**
 * @defgroup HRPWM_Check_Param_Validity HRPWM Check Parameters Validity
 * @{
 */

/*! Parameter valid check for HRPWM unit */
#define IS_HRPWM_UNIT(x)                                                       \
(   ((x) == CM_HRPWM1)                          ||                             \
    ((x) == CM_HRPWM2)                          ||                             \
    ((x) == CM_HRPWM3)                          ||                             \
    ((x) == CM_HRPWM4)                          ||                             \
    ((x) == CM_HRPWM5)                          ||                             \
    ((x) == CM_HRPWM6))

/*! Parameter valid check for HRPWM phase function unit */
#define IS_HRPWM_PH_UNIT(x)                                                    \
(   ((x) == CM_HRPWM2)                          ||                             \
    ((x) == CM_HRPWM3)                          ||                             \
    ((x) == CM_HRPWM4)                          ||                             \
    ((x) == CM_HRPWM5)                          ||                             \
    ((x) == CM_HRPWM6))

/*! Parameter valid check for count Mode */
#define IS_HRPWM_CNT_MD(x)                                                     \
(   ((x) == HRPWM_MD_SAWTOOTH)                  ||                             \
    ((x) == HRPWM_MD_TRIANGLE))

/*! Parameter valid check for count reload mode */
#define IS_HRPWM_CNT_RELOAD_MD(x)                                              \
(   ((x) == HRPWM_CNT_RELOAD_ON)                ||                             \
    ((x) == HRPWM_CNT_RELOAD_OFF))

/*! Parameter valid check for PWM pin status for count start */
#define IS_HRPWM_PWM_POLARITY_START(x)                                         \
(   ((x) == HRPWM_PWM_START_LOW)                ||                             \
    ((x) == HRPWM_PWM_START_HIGH)               ||                             \
    ((x) == HRPWM_PWM_START_HOLD))

/*! Parameter valid check for PWM pin status for count stop */
#define IS_HRPWM_PWM_POLARITY_STOP(x)                                          \
(   ((x) == HRPWM_PWM_STOP_LOW)                 ||                             \
    ((x) == HRPWM_PWM_STOP_HIGH)                ||                             \
    ((x) == HRPWM_PWM_STOP_HOLD))

/*! Parameter valid check for PWM pin status for count peak */
#define IS_HRPWM_PWM_POLARITY_PEAK(x)                                          \
(   ((x) == HRPWM_PWM_PEAK_LOW)                 ||                             \
    ((x) == HRPWM_PWM_PEAK_HIGH)                ||                             \
    ((x) == HRPWM_PWM_PEAK_HOLD)                ||                             \
    ((x) == HRPWM_PWM_PEAK_INVT))

/*! Parameter valid check for PWM pin status for count valley */
#define IS_HRPWM_PWM_POLARITY_VALLEY(x)                                        \
(   ((x) == HRPWM_PWM_VALLEY_LOW)               ||                             \
    ((x) == HRPWM_PWM_VALLEY_HIGH)              ||                             \
    ((x) == HRPWM_PWM_VALLEY_HOLD)              ||                             \
    ((x) == HRPWM_PWM_VALLEY_INVT))

/*! Parameter valid check for PWM pin status for count up match HRGCMAR */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_A(x)                                    \
(   ((x) == HRPWM_PWM_UP_MATCH_A_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_A_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_A_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_A_INVT))

/*! Parameter valid check for PWM pin status for count down match HRGCMAR */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_MATCH_A_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_A_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_A_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_A_INVT))

/*! Parameter valid check for PWM pin status for count up match HRGCMBR */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_B(x)                                    \
(   ((x) == HRPWM_PWM_UP_MATCH_B_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_B_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_B_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_B_INVT))

/*! Parameter valid check for PWM pin status for count down match HRGCMBR */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_MATCH_B_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_B_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_B_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_B_INVT))

/*! Parameter valid check for PWM pin status for count up match HRGCMER */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_E(x)                                    \
(   ((x) == HRPWM_PWM_UP_MATCH_E_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_E_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_E_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_E_INVT))

/*! Parameter valid check for PWM pin status for count down match HRGCMER */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_MATCH_E_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_E_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_E_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_E_INVT))

/*! Parameter valid check for PWM pin status for count up match HRGCMFR */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_F(x)                                    \
(   ((x) == HRPWM_PWM_UP_MATCH_F_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_F_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_F_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_F_INVT))

/*! Parameter valid check for PWM pin status for count down match HRGCMFR */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_MATCH_F_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_F_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_F_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_F_INVT))

/*! Parameter valid check for PWM pin status for count up match SCMAR */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(x)                            \
(   ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_A_LOW)   ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_A_HIGH)  ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_A_HOLD)  ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_A_INVT))

/*! Parameter valid check for PWM pin status for count down match SCMAR */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(x)                          \
(   ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_A_LOW) ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_A_HIGH)||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_A_HOLD)||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_A_INVT))

/*! Parameter valid check for PWM pin status for count up match SCMBR */
#define IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(x)                            \
(   ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_B_LOW)   ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_B_HIGH)  ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_B_HOLD)  ||                             \
    ((x) == HRPWM_PWM_UP_MATCH_SPECIAL_B_INVT))

/*! Parameter valid check for PWM pin status for count down match SCMBR */
#define IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(x)                          \
(   ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_B_LOW) ||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_B_HIGH)||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_B_HOLD)||                             \
    ((x) == HRPWM_PWM_DOWN_MATCH_SPECIAL_B_INVT))

/*! Parameter valid check for external event number in pin polarity set function */
#define IS_HRPWM_PWM_EXT_EVT_NUM(x)                                            \
(   ((x) == HRPWM_PWM_EXT_EVT_NUM1)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM2)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM3)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM4)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM5)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM6)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM7)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM8)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM9)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_NUM10))

/*! Parameter valid check for PWM pin status for external event */
#define IS_HRPWM_PWM_POLARITY_EXT_EVT(x)                                       \
(   ((x) == HRPWM_PWM_EXT_EVT_LOW)              ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_HIGH)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_HOLD)             ||                             \
    ((x) == HRPWM_PWM_EXT_EVT_INVT))

/*! Parameter valid check for force PWM output pin */
#define IS_HRPWM_PWM_FORCE_POLARITY(x)                                         \
(   ((x) == HRPWM_PWM_FORCE_LOW)                ||                             \
    ((x) == HRPWM_PWM_FORCE_HIGH))

/*! Parameter valid check for PWM channel swap mode */
#define IS_HRPWM_PWM_SWAP_MD(x)                                                \
(   ((x) == HRPWM_PWM_CH_SWAP_MD_NOT_IMMED)     ||                             \
    ((x) == HRPWM_PWM_CH_SWAP_MD_IMMED))

/*! Parameter valid check for PWM channel swap function */
#define IS_HRPWM_PWM_SWAP(x)                                                   \
(   ((x) == HRPWM_PWM_CH_SWAP_OFF)              ||                             \
    ((x) == HRPWM_PWM_CH_SWAP_ON))

/*! Parameter valid check for PWM channel A invert function */
#define IS_HRPWM_PWM_CHA_INVT(x)                                               \
(   ((x) == HRPWM_PWM_CHA_INVT_OFF)             ||                             \
    ((x) == HRPWM_PWM_CHA_INVT_ON))

/*! Parameter valid check for PWM channel B invert function */
#define IS_HRPWM_PWM_CHB_INVT(x)                                               \
(   ((x) == HRPWM_PWM_CHB_INVT_OFF)             ||                             \
    ((x) == HRPWM_PWM_CHB_INVT_ON))

/*! Parameter valid check for match special register event */
#define IS_HRPWM_MATCH_SPECIAL_EVT(x)                                          \
(   ((x) == HRPWM_EVT_UP_MATCH_SPECIAL_A)       ||                             \
    ((x) == HRPWM_EVT_DOWN_MATCH_SPECIAL_A)     ||                             \
    ((x) == HRPWM_EVT_UP_MATCH_SPECIAL_B)       ||                             \
    ((x) == HRPWM_EVT_DOWN_MATCH_SPECIAL_B))

/*! Parameter valid check for complete period point */
#define IS_HRPWM_CPLT_PERIOD_POINT(x)                                          \
(   ((x) == HRPWM_CPLT_PERIOD_SAWTOOTH_PEAK_TRIANGLE_VALLEY)    ||             \
    ((x) == HRPWM_CPLT_PERIOD_PEAK)                             ||             \
    ((x) == HRPWM_CPLT_PERIOD_VALLEY)                           ||             \
    ((x) == HRPWM_CPLT_PERIOD_PEAK_OR_VALLEY))

/*! Parameter valid check for period link function */
#define IS_HRPWM_PH_PERIOD_LINK(x)                                             \
(   ((x) == HRPWM_PH_PERIOD_LINK_OFF)           ||                             \
    ((x) == HRPWM_PH_PERIOD_LINK_ON))

/*! Parameter valid check for global buffer flag source */
#define IS_HRPWM_U1_GLOBAL_BUF_FLAG_SRC(x)                                     \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_GLOBAL_BUF_SRC_U1_ALL) == HRPWM_GLOBAL_BUF_SRC_U1_ALL))
#define IS_HRPWM_U2_6_GLOBAL_BUF_FLAG_SRC(x)                                   \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_GLOBAL_BUF_SRC_U2_6_ALL) == HRPWM_GLOBAL_BUF_SRC_U2_6_ALL))

/*! Parameter valid check for trig pin */
#define IS_HRPWM_TRIG_PIN(x)                                                   \
(   ((x) == HRPWM_INPUT_TRIGA)                  ||                             \
    ((x) == HRPWM_INPUT_TRIGB)                  ||                             \
    ((x) == HRPWM_INPUT_TRIGC)                  ||                             \
    ((x) == HRPWM_INPUT_TRIGD))

/*! Parameter valid check for input pin filter clock */
#define IS_HRPWM_FILTER_CLK(x)                                                 \
(   ((x) == HRPWM_FILTER_CLK_DIV1)              ||                             \
    ((x) == HRPWM_FILTER_CLK_DIV4)              ||                             \
    ((x) == HRPWM_FILTER_CLK_DIV16)             ||                             \
    ((x) == HRPWM_FILTER_CLK_DIV64))

/*! Parameter valid check for interrupt source configuration */
#define IS_HRPWM_INT(x)                                                        \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_INT_ALL) == HRPWM_INT_ALL))

/*! Parameter valid check for HRPWM count status bit read */
#define IS_HRPWM_CNT_GET_FLAG(x)                                               \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_FLAG_ALL) == HRPWM_FLAG_ALL))

/*! Parameter valid check for HRPWM count status bit clear */
#define IS_HRPWM_CNT_CLR_FLAG(x)                                               \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_FLAG_CLR_ALL) == HRPWM_FLAG_CLR_ALL))

/*! Parameter valid check for HRPWM phase status bit */
#define IS_HRPWM_PH_FLAG(x)                                                    \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_PH_FLAG_ALL) == HRPWM_PH_FLAG_ALL))

/*! Parameter valid check for buffer transfer condition */
#define IS_HRPWM_BUF_TRANS_COND(x)                                             \
(   ((x) == HRPWM_BUF_TRANS_INVD)               ||                             \
    ((x) == HRPWM_BUF_TRANS_PEAK)               ||                             \
    ((x) == HRPWM_BUF_TRANS_VALLEY)             ||                             \
    ((x) == HRPWM_BUF_TRANS_PEAK_VALLEY))

/*! Parameter valid check for count condition for valid period function */
#define IS_HRPWM_PERIOD_CNT_COND(x)                                            \
(   ((x) == HRPWM_VALID_PERIOD_INVD)            ||                             \
    ((x) == HRPWM_VALID_PERIOD_CNT_VALLEY)      ||                             \
    ((x) == HRPWM_VALID_PERIOD_CNT_PEAK)        ||                             \
    ((x) == HRPWM_VALID_PERIOD_CNT_PEAK_VALLEY))

/*! Parameter valid check for valid period interval */
#define IS_HRPWM_PERIOD_INTERVAL(x)                                            \
(   (x) <= 31U)

/*! Parameter valid check for valid period special A function */
#define IS_HRPWM_PERIOD_SPECIAL_A_FUNC(x)                                      \
(   ((x) == HRPWM_VALID_PERIOD_SPECIAL_A_OFF)   ||                             \
    ((x) == HRPWM_VALID_PERIOD_SPECIAL_A_ON))

/*! Parameter valid check for valid period special B function */
#define IS_HRPWM_PERIOD_SPECIAL_B_FUNC(x)                                      \
(   ((x) == HRPWM_VALID_PERIOD_SPECIAL_B_OFF)   ||                             \
    ((x) == HRPWM_VALID_PERIOD_SPECIAL_B_ON))

/*! Parameter valid check for dead time buffer function for DTUAR and DTUBR register */
#define IS_HRPWM_DEADTIME_BUF_FUNC_DTUAR(x)                                    \
(   ((x) == HRPWM_DEADTIME_CNT_UP_BUF_OFF)      ||                             \
    ((x) == HRPWM_DEADTIME_CNT_UP_BUF_ON))

/*! Parameter valid check for dead time buffer function for DTDAR and DTDBR register */
#define IS_HRPWM_DEADTIME_BUF_FUNC_DTDAR(x)                                    \
(   ((x) == HRPWM_DEADTIME_CNT_DOWN_BUF_OFF)    ||                             \
    ((x) == HRPWM_DEADTIME_CNT_DOWN_BUF_ON))

/*! Parameter valid check for dead time equal function for DTUAR and DTDAR register */
#define IS_HRPWM_DEADTIME_EQUAL_FUNC(x)                                        \
(   ((x) == HRPWM_DEADTIME_EQUAL_OFF)           ||                             \
    ((x) == HRPWM_DEADTIME_EQUAL_ON))

/*! Parameter valid check for EMB event valid channel  */
#define IS_HRPWM_EMB_CH(x)                                                     \
(   ((x) == HRPWM_EMB_EVT_CH0)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH1)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH2)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH3)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH4)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH5))

/*! Parameter valid check for EMB release mode when EMB event invalid   */
#define IS_HRPWM_EMB_RELEASE_MD(x)                                             \
(   ((x) == HRPWM_EMB_RELEASE_IMMED)            ||                             \
    ((x) == HRPWM_EMB_RELEASE_PEAK)             ||                             \
    ((x) == HRPWM_EMB_RELEASE_VALLEY)           ||                             \
    ((x) == HRPWM_EMB_RELEASE_PEAK_VALLEY))

/*! Parameter valid check for pin output status when EMB event valid */
#define IS_HRPWM_EMB_VALID_PIN_STAT(x)                                         \
(   ((x) == HRPWM_EMB_PIN_NORMAL)               ||                             \
    ((x) == HRPWM_EMB_PIN_HIZ)                  ||                             \
    ((x) == HRPWM_EMB_PIN_LOW)                  ||                             \
    ((x) == HRPWM_EMB_PIN_HIGH))

/*! Parameter valid check for hardware start condition   */
#define IS_HRPWM_HW_START_COND(x)                                              \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_START_COND_ALL) == HRPWM_HW_START_COND_ALL)

/*! Parameter valid check for hardware clear condition   */
#define IS_HRPWM_HW_CLR_COND(x)                                                \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_CLR_COND_ALL) == HRPWM_HW_CLR_COND_ALL)

/*! Parameter valid check for hardware capture condition   */
#define IS_HRPWM_HW_CAPT_COND(x)                                               \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_CAPT_COND_ALL) == HRPWM_HW_CAPT_COND_ALL)

/*! Parameter valid check for external event number */
#define IS_HRPWM_EVT_NUM(x)                                                    \
(   (x) <= HRPWM_EVT10)

/*! Parameter valid check for external event number which have fast mode function */
#define IS_HRPWM_EVT_NUM_FAST_MD(x)                                            \
(   (x) <= HRPWM_EVT5)

/*! Parameter valid check for external event number which have filter function */
#define IS_HRPWM_EVT_NUM_FILTER(x)                                             \
(   ((x) >= HRPWM_EVT6)                         &&                             \
    ((x) <= HRPWM_EVT10))

/*! Parameter valid check for External event source */
#define IS_HRPWM_EVT_SRC(x)                                                    \
(   ((x) == HRPWM_EVT_SRC1)                     ||                             \
    ((x) == HRPWM_EVT_SRC2)                     ||                             \
    ((x) == HRPWM_EVT_SRC3)                     ||                             \
    ((x) == HRPWM_EVT_SRC4))

/*! Parameter valid check for External event source 2 */
#define IS_HRPWM_EVT_SRC2(x)                                                   \
(   ((x) == HRPWM_EVT_SRC2_CMP1)                ||                             \
    ((x) == HRPWM_EVT_SRC2_CMP2)                ||                             \
    ((x) == HRPWM_EVT_SRC2_CMP3)                ||                             \
    ((x) == HRPWM_EVT_SRC2_NONE))

/*! Parameter valid check for External event valid polarity  */
#define IS_HRPWM_EVT_VALID_LVL(x)                                              \
(   ((x) == HRPWM_EVT_VALID_LVL_HIGH)           ||                             \
    ((x) == HRPWM_EVT_VALID_LVL_LOW))

/*! Parameter valid check for External event fast asynchronous mode  */
#define IS_HRPWM_EVT_FAST_ASYNC_MD(x)                                          \
(   ((x) == HRPWM_EVT_FAST_ASYNC_OFF)           ||                             \
    ((x) == HRPWM_EVT_FAST_ASYNC_ON))

/*! Parameter valid check for External event generate detect event function  */
#define IS_HRPWM_EVT_GEN_DETECT_EVT_FUNC(x)                                    \
(   ((x) == HRPWM_EVT_GEN_DETECT_OFF)           ||                             \
    ((x) == HRPWM_EVT_GEN_DETECT_ON))

/*! Parameter valid check for External event valid action  */
#define IS_HRPWM_EVT_VALID_ACTION(x)                                           \
(   ((x) == HRPWM_EVT_VALID_LVL)                ||                             \
    ((x) == HRPWM_EVT_VALID_RISING)             ||                             \
    ((x) == HRPWM_EVT_VALID_FALLING)            ||                             \
    ((x) == HRPWM_EVT_VALID_BOTH))

/*! Parameter valid check for External event clock EEVS  */
#define IS_HRPWM_EVT_EEVS_CLK(x)                                               \
(   ((x) == HRPWM_EVT_EEVS_PCLK0)               ||                             \
    ((x) == HRPWM_EVT_EEVS_PCLK0_DIV2)          ||                             \
    ((x) == HRPWM_EVT_EEVS_PCLK0_DIV4)          ||                             \
    ((x) == HRPWM_EVT_EEVS_PCLK0_DIV8))

/*! Parameter valid check for External event filter clock */
#define IS_HRPWM_EVT_FILTER_CLK(x)                                             \
(   (x) <= HRPWM_EVT_FILTER_EEVS_DIV256)

/*! Parameter valid check for External event filter signal replace mode */
#define IS_HRPWM_EVT_FILTER_SIGNAL_REPLACE(x)                                  \
(   ((x) == HRPWM_EVT_FILTER_REPLACE_MD1)       ||                             \
    ((x) == HRPWM_EVT_FILTER_REPLACE_MD2_PEAK)  ||                             \
    ((x) == HRPWM_EVT_FILTER_REPLACE_MD2_VALLEY))

/*! Parameter valid check for External event filter mode */
#define IS_HRPWM_EVT_FILTER_MD(x)                                              \
(   ((x) == HRPWM_EVT_FILTER_OFF)               ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U1)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM1_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U2)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM2_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U3)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM3_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U4)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM4_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U5)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM5_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_U6)       ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLANK_HRPWM6_B) ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_WIN)            ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_WIN_OTHER))

/*! Parameter valid check for External event filter signal initial level */
#define IS_HRPWM_EVT_FILTER_INIT_POLARITY(x)                                   \
(   ((x) == HRPWM_EVT_FILTER_INIT_POLARITY_LOW) ||                             \
    ((x) == HRPWM_EVT_FILTER_INIT_POLARITY_HIGH))

/*! Parameter valid check for External event latch function */
#define IS_HRPWM_EVT_LATCH_FUNC(x)                                             \
(   ((x) == HRPWM_EVT_FILTER_LATCH_OFF)         ||                             \
    ((x) == HRPWM_EVT_FILTER_LATCH_ON))

/*! Parameter valid check for External event timeout function */
#define IS_HRPWM_EVT_TIMEOUT_FUNC(x)                                           \
(   ((x) == HRPWM_EVT_FILTER_TIMEOUT_OFF)       ||                             \
    ((x) == HRPWM_EVT_FILTER_TIMEOUT_ON))

/*! Parameter valid check for External event filter offset direction */
#define IS_HRPWM_EVT_FILTER_OFS_DIR(x)                                         \
(   ((x) == HRPWM_EVT_FILTER_OFS_DIR_DOWN)      ||                             \
    ((x) == HRPWM_EVT_FILTER_OFS_DIR_UP))

/*! Parameter valid check for External event filter window direction */
#define IS_HRPWM_EVT_FILTER_WIN_DIR(x)                                         \
(   ((x) == HRPWM_EVT_FILTER_WIN_DIR_DOWN)      ||                             \
    ((x) == HRPWM_EVT_FILTER_WIN_DIR_UP))

/*! Parameter valid check for channel A output level in idle state */
#define IS_HRPWM_IDLE_CHA_LVL(x)                                               \
(   ((x) == HRPWM_IDLE_CHA_LVL_LOW)             ||                             \
    ((x) == HRPWM_IDLE_CHA_LVL_HIGH))

/*! Parameter valid check for channel B output level in idle state */
#define IS_HRPWM_IDLE_CHB_LVL(x)                                               \
(   ((x) == HRPWM_IDLE_CHB_LVL_LOW)             ||                             \
    ((x) == HRPWM_IDLE_CHB_LVL_HIGH))

/*! Parameter valid check for channel A idle output status */
#define IS_HRPWM_IDLE_OUTPUT_CHA_STAT(x)                                       \
(   ((x) == HRPWM_IDLE_OUTPUT_CHA_OFF)          ||                             \
    ((x) == HRPWM_IDLE_OUTPUT_CHA_ON))

/*! Parameter valid check for channel B idle output status */
#define IS_HRPWM_IDLE_OUTPUT_CHB_STAT(x)                                       \
(   ((x) == HRPWM_IDLE_OUTPUT_CHB_OFF)          ||                             \
    ((x) == HRPWM_IDLE_OUTPUT_CHB_ON))

/*! Parameter valid check for idle delay trigger source */
#define IS_HRPWM_IDLE_DELAY_TRIG_SRC(x)                                        \
(   ((x) == HRPWM_IDLE_DELAY_TRIG_EVT6)         ||                             \
    ((x) == HRPWM_IDLE_DELAY_TRIG_EVT7)         ||                             \
    ((x) == HRPWM_IDLE_DELAY_TRIG_EVT8)         ||                             \
    ((x) == HRPWM_IDLE_DELAY_TRIG_EVT9)         ||                             \
    ((x) == HRPWM_IDLE_DELAY_TRIG_SW))

/*! Parameter valid check for idle delay flag  */
#define IS_HRPWM_IDLE_DELAY_FLAG(x)                                            \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_IDLE_DELAY_FLAG_ALL) == HRPWM_IDLE_DELAY_FLAG_ALL))

/*! Parameter valid check for idle delay clear flag  */
#define IS_HRPWM_IDLE_DELAY_CLR_FLAG(x)                                        \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_IDLE_DELAY_FLAG_CLR_ALL) == HRPWM_IDLE_DELAY_FLAG_CLR_ALL))

/*! Parameter valid check for BM output count source  */
#define IS_HRPWM_BM_CNT_SRC(x)                                                 \
(   ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U1)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U2)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U3)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U4)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U5)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U6)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_TMR0_1_CMPA)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_TMR0_1_CMPB)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_TMR0_2_CMPA)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_TMR0_2_CMPB)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK))

/*! Parameter valid check for BM output count source from pclk0 */
#define IS_HRPWM_BM_CNT_PCLK0_DIV(x)                                           \
(   ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV1)         ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV2)         ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV4)         ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV8)         ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV16)        ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV32)        ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV64)        ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV128)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV256)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV512)       ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV1024)      ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV2048)      ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV4096)      ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV8192)      ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV16384)     ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PCLK_DIV32768))

/*! Parameter valid check for BM delay enter function */
#define IS_HRPWM_BM_CNT_RELOAD(x)                                              \
(   ((x) == HRPWM_BM_CNT_RELOAD_ON)             ||                             \
    ((x) == HRPWM_BM_CNT_RELOAD_OFF))

/*! Parameter valid check for BM action mode */
#define IS_HRPWM_BM_ACTION_MD(x)                                               \
(   ((x) == HRPWM_BM_ACTION_MD1)                ||                             \
    ((x) == HRPWM_BM_ACTION_MD2))

/*! Parameter valid check for channel A BM output status */
#define IS_HRPWM_BM_OUTPUT_CHA_STAT(x)                                         \
(   ((x) == HRPWM_BM_OUTPUT_CHA_OFF)            ||                             \
    ((x) == HRPWM_BM_OUTPUT_CHA_ON))

/*! Parameter valid check for channel B BM output status */
#define IS_HRPWM_BM_OUTPUT_CHB_STAT(x)                                         \
(   ((x) == HRPWM_BM_OUTPUT_CHB_OFF)            ||                             \
    ((x) == HRPWM_BM_OUTPUT_CHB_ON))

/*! Parameter valid check for channel A BM output delay enter function */
#define IS_HRPWM_BM_DELAY_ENTER_CHA_STAT(x)                                    \
(   ((x) == HRPWM_BM_DELAY_ENTER_CHA_OFF)       ||                             \
    ((x) == HRPWM_BM_DELAY_ENTER_CHA_ON))

/*! Parameter valid check for channel B BM output delay enter function */
#define IS_HRPWM_BM_DELAY_ENTER_CHB_STAT(x)                                    \
(   ((x) == HRPWM_BM_DELAY_ENTER_CHB_OFF)       ||                             \
    ((x) == HRPWM_BM_DELAY_ENTER_CHB_ON))

/*! Parameter valid check for BM output follow function */
#define IS_HRPWM_BM_FOLLOW_FUNC(x)                                             \
(   ((x) == HRPWM_BM_FOLLOW_FUNC_OFF)           ||                             \
    ((x) == HRPWM_BM_FOLLOW_FUNC_ON))

/*! Parameter valid check for BM output unit count reset function */
#define IS_HRPWM_BM_UNIT_CNT_RST_FUNC(x)                                       \
(   ((x) == HRPWM_BM_UNIT_CNT_CONTINUE)         ||                             \
    ((x) == HRPWM_BM_UNIT_CNT_STP_RST))

/*! Parameter valid check for idle BM trigger source */
#define IS_HRPWM_BM_TRIG_SRC(x)                                                \
(   ((x) | HRPWM_BM_TRIG_ALL) == HRPWM_BM_TRIG_ALL)

/*! Parameter valid check for HRPWM multiple unit */
#define IS_HRPWM_MUL_UNIT(x)                                                   \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_UNIT_ALL) == HRPWM_UNIT_ALL)

/*! Parameter valid check for HRPWM software synchronous unit */
#define IS_HRPWM_SW_SYNC_UNIT(x)                                               \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_SW_SYNC_UNIT_ALL) == HRPWM_SW_SYNC_UNIT_ALL)

/*! Parameter valid check for HRPWM software synchronous channel */
#define IS_HRPWM_SW_SYNC_CH(x)                                                 \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_SW_SYNC_CH_ALL) == HRPWM_SW_SYNC_CH_ALL)

/*! Parameter valid check for BM flag */
#define IS_HRPWM_BM_FLAG(x)                                                    \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_BM_FLAG_ALL) == HRPWM_BM_FLAG_ALL)

/*! Parameter valid check for clear BM flag */
#define IS_HRPWM_BM_CLR_FLAG(x)                                                \
(   (x) == HRPWM_BM_FLAG_PEAK)

/*! Parameter valid check for phase match event index */
#define IS_HRPWM_PH_MATCH_IDX(x)                                               \
(   (x) <= HRPWM_PH_MATCH_IDX5)

/*! Parameter valid check for phase match force ChA function */
#define IS_HRPWM_PH_MATCH_FORCE_CHA_FUNC(x)                                    \
(   ((x) == HRPWM_PH_MATCH_FORCE_CHA_OFF)       ||                             \
    ((x) == HRPWM_PH_MATCH_FORCE_CHA_ON))

/*! Parameter valid check for phase match force ChB function */
#define IS_HRPWM_PH_MATCH_FORCE_CHB_FUNC(x)                                    \
(   ((x) == HRPWM_PH_MATCH_FORCE_CHB_OFF)       ||                             \
    ((x) == HRPWM_PH_MATCH_FORCE_CHB_ON))

/*! Parameter valid check for calibrate status flag */
#define IS_HRPWM_CALIB_FLAG(x)                                                 \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_CALIB_FLAG_ALL) == HRPWM_CALIB_FLAG_ALL))

/*! Parameter valid check for calibrate period */
#define IS_HRPWM_CALIB_PERIOD(x)                                               \
(   (x) <= HRPWM_CALIB_PERIOD_8P7_MS_MUL32768)

/*! Parameter valid check for synchronous output pulse */
#define IS_HRPWM_SYNC_PULSE(x)                                                 \
(   ((x) == HRPWM_SYNC_PULSE_OFF)               ||                             \
    ((x) == HRPWM_SYNC_PULSE_POSITIVE)          ||                             \
    ((x) == HRPWM_SYNC_PULSE_NEGATIVE))

/*! Parameter valid check for synchronous output pulse width */
#define IS_HRPWM_SYNC_PULSE_WIDTH(x)                                           \
(   ((x) <= 0xFFUL)                             &&                             \
    ((x) > 0x10UL))

/*! Parameter valid check for synchronous output source */
#define IS_HRPWM_SYNC_SRC(x)                                                   \
(   (x) <= HRPWM_SYNC_SRC_U6_MATCH_SPECIAL_B)

/*! Parameter valid check for synchronous output match B direction */
#define IS_HRPWM_SYNC_MATCH_B_DIR(x)                                           \
(   ((x) == HRPWM_SYNC_MATCH_B_DIR_DOWN)        ||                             \
    ((x) == HRPWM_SYNC_MATCH_B_DIR_UP))

/*! Parameter valid check for data register data range */
#define IS_HRPWM_DATA_REG_RANGE(x)              ((x) <= 0x3FFFFFUL)

/*! Parameter valid check for data register data range 1 */
#define IS_HRPWM_DATA_REG_RANGE1(x)                                            \
(   ((x) <= 0x3FFFC0UL)                         &&                             \
    ((x) > 0xC0UL))

/*! Parameter valid check for phase data register data range */
#define IS_HRPWM_PH_DATA_REG_RANGE(x)           ((x) <= 0x7FFFC0UL)

/*! Parameter valid check for BMPERAR register data range */
#define IS_HRPWM_BM_CNT_REG_RANGE(x)            ((x) <= 0xFFFFUL)

/*! Parameter valid check for status bit read */
#define IS_HRPWM_SCMA_SRC(x)                                                   \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_SCMA_SRC_ALL) == HRPWM_SCMA_SRC_ALL))

/*! Parameter valid check for Buffer U1 Single transfer configuration */
#define IS_HRPWM_BUF_U1_SINGLE_TRANS_CONFIG(x)                                 \
(   ((x) == HRPWM_BUF_U1_SINGLE_TRANS_OFF)      ||                             \
    ((x) == HRPWM_BUF_U1_SINGLE_TRANS_ON))

/*! Parameter valid check for DAC synchronous trigger source */
#define IS_HRPWM_DAC_TRIG_SRC(x)                                               \
(   ((x) == HRPWM_DAC_TRIG_SRC_CNT_VALLEY)      ||                             \
    ((x) == HRPWM_DAC_TRIG_SRC_CNT_PEAK)        ||                             \
    ((x) == HRPWM_DAC_TRIG_SRC_UP_MATCH_SPECIAL_A)      ||                     \
    ((x) == HRPWM_DAC_TRIG_SRC_DOWN_MATCH_SPECIAL_A)    ||                     \
    ((x) == HRPWM_DAC_TRIG_SRC_UP_MATCH_SPECIAL_B)      ||                     \
    ((x) == HRPWM_DAC_TRIG_SRC_DOWN_MATCH_SPECIAL_B))

/*! Parameter valid check for DAC channel 1 trigger destination */
#define IS_HRPWM_DAC_CH1_TRIG_DEST(x)                                          \
(   ((x) == HRPWM_DAC_CH1_TRIG_DEST_NONE)       ||                             \
    ((x) == HRPWM_DAC_CH1_TRIG_DEST_DAC1)       ||                             \
    ((x) == HRPWM_DAC_CH1_TRIG_DEST_DAC2))

/*! Parameter valid check for DAC channel 2 trigger destination */
#define IS_HRPWM_DAC_CH2_TRIG_DEST(x)                                          \
(   ((x) == HRPWM_DAC_CH2_TRIG_DEST_NONE)       ||                             \
    ((x) == HRPWM_DAC_CH2_TRIG_DEST_DAC1))

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
 * @addtogroup HRPWM_Global_Functions
 * @{
 */

/**
 * @brief  HRPWM set calibrate period
 * @param  [in] u32Period           Calibrate period, @ref HRPWM_Calib_Period_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_SetPeriod(uint32_t u32Period)
{
    DDL_ASSERT(IS_HRPWM_CALIB_PERIOD(u32Period));
    MODIFY_REG32(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CALPRD | HRPWM_CALIB_FLAG_ALL,
                 (u32Period << HRPWM_COMMON_CALCR_CALPRD_POS) | HRPWM_CALIB_FLAG_ALL);
}

/**
 * @brief  HRPWM single calibrate enable
 * @param  None
 * @retval None
 * @note   The period calibration and single calibration functions cannot be enabled at the same time.
 */
__STATIC_INLINE void HRPWM_CALIB_SingleEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CAL | HRPWM_CALIB_FLAG_ALL);
}

/**
 * @brief  HRPWM single calibrate disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_SingleDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CAL | HRPWM_CALIB_FLAG_ALL, ~HRPWM_COMMON_CALCR_CAL);
}

/**
 * @brief  HRPWM period calibrate enable
 * @param  None
 * @retval None
 * @note   The period calibration and single calibration functions cannot be enabled at the same time.
 */
__STATIC_INLINE void HRPWM_CALIB_PeriodEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CALEN | HRPWM_CALIB_FLAG_ALL);
}

/**
 * @brief  HRPWM period calibrate disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_PeriodDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CALEN | HRPWM_CALIB_FLAG_ALL, ~HRPWM_COMMON_CALCR_CALEN);
}

/**
 * @brief  HRPWM calibrate end interrupt enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_EndIntEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CALIE | HRPWM_CALIB_FLAG_ALL);
}

/**
 * @brief  HRPWM calibrate end interrupt disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_EndIntDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->CALCR, HRPWM_COMMON_CALCR_CALIE | HRPWM_CALIB_FLAG_ALL, ~HRPWM_COMMON_CALCR_CALIE);
}

/**
 * @brief  HRPWM get calibrate status
 * @param  [in] u32Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_Calib_Flag_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_CALIB_GetStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_HRPWM_CALIB_FLAG(u32Flag));
    return (0UL != READ_REG32_BIT(CM_HRPWM_COMMON->CALCR, u32Flag)) ? SET : RESET;
}

/**
 * @brief  HRPWM calibrate clear status flag
 * @param  [in] u32Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_Calib_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_CALIB_ClearFlag(uint32_t u32Flag)
{
    DDL_ASSERT(IS_HRPWM_CALIB_FLAG(u32Flag));
    MODIFY_REG32(CM_HRPWM_COMMON->CALCR, HRPWM_CALIB_FLAG_ALL, ~u32Flag);
}

/**
 * @brief  HRPWM function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_Enable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_EN);
}

/**
 * @brief  HRPWM function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_Disable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->CR, HRPWM_CR_EN);
}

/**
 * @brief  HRPWM count set reload function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32CountReload      Count reload after count peak @ref HRPWM_Count_Reload_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetReload(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32CountReload)
{
    DDL_ASSERT(IS_HRPWM_CNT_RELOAD_MD(u32CountReload));
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_OVSTP, u32CountReload);
}

/**
 * @brief  Set HRPWM base count mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             @ref HRPWM_Count_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCountMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CNT_MD(u32Mode));
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_MODE, u32Mode);
}

/**
 * @brief  HRPWM count start
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CountStart(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->GCONR, HRPWM_GCONR_START);
}

/**
 * @brief  HRPWM count stop
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CountStop(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->GCONR, HRPWM_GCONR_START);
}

/**
 * @brief  HRPWM PWM set output channel swap Mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChSwapMode       PWM output swap mode @ref HRPWM_PWM_Ch_Swap_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_SetChSwapMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwapMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_SWAP_MD(u32ChSwapMode));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_SWAPMD, u32ChSwapMode);
}

/**
 * @brief  HRPWM PWM set output channel swap Mode (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChSwapMode       PWM output swap mode @ref HRPWM_PWM_Ch_Swap_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_SetChSwapMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwapMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_SWAP_MD(u32ChSwapMode));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_SWAPMD, u32ChSwapMode);
}

/**
 * @brief  HRPWM PWM set output channel swap function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChSwap           PWM output swap function @ref HRPWM_PWM_Ch_Swap_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_SetChSwap(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwap)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_SWAP(u32ChSwap));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_SWAPEN, u32ChSwap);
}

/**
 * @brief  HRPWM PWM set output channel swap function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChSwap           PWM output swap function @ref HRPWM_PWM_Ch_Swap_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_SetChSwap_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwap)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_SWAP(u32ChSwap));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_SWAPEN, u32ChSwap);
}

/**
 * @brief  HRPWM PWM set output channel A reserve function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Invert           PWM channel A invert function @ref HRPWM_PWM_ChA_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetInvert(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_CHA_INVT(u32Invert));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_INVCAEN, u32Invert);
}

/**
 * @brief  HRPWM PWM set output channel A reserve function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Invert           PWM channel A invert function @ref HRPWM_PWM_ChA_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetInvert_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_CHA_INVT(u32Invert));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_INVCAEN, u32Invert);
}

/**
 * @brief  HRPWM PWM set output channel B reserve function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Invert           PWM channel B invert function @ref HRPWM_PWM_ChB_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetInvert(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_CHB_INVT(u32Invert));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_INVCBEN, u32Invert);
}

/**
 * @brief  HRPWM PWM set output channel B reserve function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Invert           PWM channel B invert function @ref HRPWM_PWM_ChB_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetInvert_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_CHB_INVT(u32Invert));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_INVCBEN, u32Invert);
}

/**
 * @brief  HRPWM set Channel A polarity for count start
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Start_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityStart(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_STACA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count start (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Start_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityStart_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_STACA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count stop
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Stop_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityStop(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_STPCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count stop (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Stop_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityStop_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_STPCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityPeak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_OVFCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityPeak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_OVFCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityValley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_UDFCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityValley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_UDFCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_CMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_CMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_CMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_CMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_CMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_CMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_CMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_CMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMER
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, HRPWM_PCNAR2_CMEUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMER (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, HRPWM_PCNAR2_CMEUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMER
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR3, HRPWM_PCNAR3_CMEDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMER (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR3, HRPWM_PCNAR3_CMEDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMFR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, HRPWM_PCNAR2_CMFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match HRGCMFR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, HRPWM_PCNAR2_CMFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMFR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR3, HRPWM_PCNAR3_CMFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match HRGCMFR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR3, HRPWM_PCNAR3_CMFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match SCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, HRPWM_PCNAR2_SCMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match SCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, HRPWM_PCNAR2_SCMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match SCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR3, HRPWM_PCNAR3_SCMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match SCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR3, HRPWM_PCNAR3_SCMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match SCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, HRPWM_PCNAR2_SCMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up match SCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, HRPWM_PCNAR2_SCMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match SCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR3, HRPWM_PCNAR3_SCMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down match SCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR3, HRPWM_PCNAR3_SCMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A count up polarity for external event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A count up polarity for external event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A count down polarity for external event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR3, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A count down polarity for external event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR3, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count start
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Start_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityStart(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_STACB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count start (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Start_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityStart_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_START(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_STACB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count stop
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Stop_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityStop(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_STPCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count stop (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Stop_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityStop_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_STOP(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_STPCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityPeak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_OVFCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityPeak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_OVFCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityValley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_UDFCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityValley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_UDFCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_CMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_CMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_CMADCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_CMADCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_CMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_CMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_CMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_CMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMER
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, HRPWM_PCNBR2_CMEUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMER (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR2, HRPWM_PCNBR2_CMEUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMER
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR3, HRPWM_PCNBR3_CMEDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMER (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR3, HRPWM_PCNBR3_CMEDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMFR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, HRPWM_PCNBR2_CMFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match HRGCMFR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR2, HRPWM_PCNBR2_CMFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMFR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR3, HRPWM_PCNBR3_CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match HRGCMFR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR3, HRPWM_PCNBR3_CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match SCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, HRPWM_PCNBR2_SCMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match SCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR2, HRPWM_PCNBR2_SCMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match SCMAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR3, HRPWM_PCNBR3_SCMADCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match SCMAR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR3, HRPWM_PCNBR3_SCMADCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match SCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, HRPWM_PCNBR2_SCMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up match SCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR2, HRPWM_PCNBR2_SCMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match SCMBR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR3, HRPWM_PCNBR3_SCMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down match SCMBR (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_Match_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_MATCH_SPECIAL_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR3, HRPWM_PCNBR3_SCMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B count up polarity for external event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B count up polarity for external event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR2, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B count down polarity for external event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR3, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B count down polarity for external event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            External event number @ref HRPWM_Pin_Polarity_Ext_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Ext_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_EXT_EVT_NUM(u32Event));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_EXT_EVT(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR3, u32Event, u32Polarity);
}

/**
 * @brief  HRPWM set channel A force polarity of next period
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         @ref HRPWM_Force_Polarity_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_FORCE_POLARITY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_FORCA, u32Polarity);
}

/**
 * @brief  HRPWM set channel A force polarity of next period (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         @ref HRPWM_Force_Polarity_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_FORCE_POLARITY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_FORCA, u32Polarity);
}

/**
 * @brief  HRPWM set channel B force polarity of next period
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         @ref HRPWM_Force_Polarity_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_FORCE_POLARITY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_FORCB, u32Polarity);
}

/**
 * @brief  HRPWM set channel B force polarity of next period (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         @ref HRPWM_Force_Polarity_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_FORCE_POLARITY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_FORCB, u32Polarity);
}

/**
 * @brief  Hardware channel A capture condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCPAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel A capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HCPAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel B capture condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPBR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCPBR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel B capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCPBR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HCPBR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel A capture function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->HCPAR2, HRPWM_HCPAR2_HCPAEN);
}

/**
 * @brief  Hardware channel A capture function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->HCPAR2, HRPWM_HCPAR2_HCPAEN);
}

/**
 * @brief  Hardware channel B capture function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->HCPBR2, HRPWM_HCPBR2_HCPBEN);
}

/**
 * @brief  Hardware channel B capture function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->HCPBR2, HRPWM_HCPBR2_HCPBEN);
}

/**
 * @brief  HRPWM capture function software trigger
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Sw_Sync_Unit_Define
 * @param  [in] u32Ch               HRPWM channel @ref HRPWM_Sw_Sync_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_CaptureSWTrigger(uint32_t u32Unit, uint32_t u32Ch)
{
    DDL_ASSERT(IS_HRPWM_SW_SYNC_UNIT(u32Unit));
    DDL_ASSERT(IS_HRPWM_SW_SYNC_CH(u32Ch));
    SET_REG32_BIT(CM_HRPWM_COMMON->SCAPR, u32Unit & u32Ch);
}

/**
 * @brief  HRPWM PWM channel A output enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChAOutputEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->PCNAR1, HRPWM_PCNAR1_OUTENA);
}

/**
 * @brief  HRPWM PWM channel A output enable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChAOutputEnable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BPCNAR1, HRPWM_PCNAR1_OUTENA);
}

/**
 * @brief  HRPWM PWM channel A output disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChAOutputDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->PCNAR1, HRPWM_PCNAR1_OUTENA);
}

/**
 * @brief  HRPWM PWM channel A output disable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChAOutputDisable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BPCNAR1, HRPWM_PCNAR1_OUTENA);
}

/**
 * @brief  HRPWM PWM channel B output enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBOutputEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->PCNBR1, HRPWM_PCNBR1_OUTENB);
}

/**
 * @brief  HRPWM PWM channel B output enable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBOutputEnable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BPCNBR1, HRPWM_PCNBR1_OUTENB);
}

/**
 * @brief  HRPWM PWM channel B output disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBOutputDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->PCNBR1, HRPWM_PCNBR1_OUTENB);
}

/**
 * @brief  HRPWM PWM channel B output disable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBOutputDisable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BPCNBR1, HRPWM_PCNBR1_OUTENB);
}

/**
 * @brief  Get HRPWM status flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_Count_Status_Flag_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_GetStatus(const CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Flag)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CNT_GET_FLAG(u64Flag));

    if ((0UL != READ_REG32_BIT(HRPWMx->STFLR1, u64Flag)) || (0UL != READ_REG32_BIT(HRPWMx->STFLR2, u64Flag >> 32U))) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  Clear HRPWM status flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Flag             Status bit to be clear, Can be one or any combination of the values below:
 *  @arg HRPWM_FLAG_MATCH_A
 *  @arg HRPWM_FLAG_MATCH_B
 *  @arg HRPWM_FLAG_MATCH_C
 *  @arg HRPWM_FLAG_MATCH_D
 *  @arg HRPWM_FLAG_MATCH_E
 *  @arg HRPWM_FLAG_MATCH_F
 *  @arg HRPWM_FLAG_CNT_PEAK
 *  @arg HRPWM_FLAG_CNT_VALLEY
 *  @arg HRPWM_FLAG_UP_MATCH_SPECIAL_A
 *  @arg HRPWM_FLAG_DOWN_MATCH_SPECIAL_A
 *  @arg HRPWM_FLAG_UP_MATCH_SPECIAL_B
 *  @arg HRPWM_FLAG_DOWN_MATCH_SPECIAL_B
 *  @arg HRPWM_FLAG_CAPT_A
 *  @arg HRPWM_FLAG_CAPT_B
 *  @arg HRPWM_FLAG_ONE_SHOT_CPLT
 * @retval None
 */
__STATIC_INLINE void HRPWM_ClearStatus(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Flag)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CNT_CLR_FLAG(u64Flag));

    MODIFY_REG32(HRPWMx->STFLR1, (uint32_t)HRPWM_FLAG_CLR_ALL, ~(uint32_t)u64Flag);
    MODIFY_REG32(HRPWMx->STFLR2, HRPWM_STFLR2_CLR_MASK, ~(uint32_t)(u64Flag >> 32U));
}

/**
 * @brief  HRPWM Half wave mode enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HalfWaveModeEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->GCONR1, HRPWM_GCONR1_HALF);
}

/**
 * @brief  HRPWM Half wave mode enable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HalfWaveModeEnable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BGCONR1, HRPWM_GCONR1_HALF);
}

/**
 * @brief  HRPWM Half wave mode disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HalfWaveModeDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->GCONR1, HRPWM_GCONR1_HALF);
}

/**
 * @brief  HRPWM Half wave mode disable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HalfWaveModeDisable_Buf(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BGCONR1, HRPWM_GCONR1_HALF);
}

/**
 * @brief  HRPWM interrupt enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32IntType          HRPWM interrupt source
 *         This parameter can be any composed value of the macros group @ref HRPWM_Int_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IntEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_INT(u32IntType));
    SET_REG32_BIT(HRPWMx->ICONR, u32IntType);
}

/**
 * @brief  HRPWM interrupt enable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32IntType          HRPWM interrupt source
 *         This parameter can be any composed value of the macros group @ref HRPWM_Int_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IntEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_INT(u32IntType));
    SET_REG32_BIT(HRPWMx->BICONR, u32IntType);
}

/**
 * @brief  HRPWM interrupt disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32IntType          HRPWM interrupt source
 *         This parameter can be any composed value of the macros group @ref HRPWM_Int_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IntDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_INT(u32IntType));
    CLR_REG32_BIT(HRPWMx->ICONR, u32IntType);
}

/**
 * @brief  HRPWM interrupt disable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32IntType          HRPWM interrupt source
 *         This parameter can be any composed value of the macros group @ref HRPWM_Int_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IntDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_INT(u32IntType));
    CLR_REG32_BIT(HRPWMx->BICONR, u32IntType);
}

/**
 * @brief  HRPWM match special register event enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Match special register event @ref HRPWM_Match_Special_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialEventEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MATCH_SPECIAL_EVT(u32Event));
    SET_REG32_BIT(HRPWMx->GCONR1, u32Event);
}

/**
 * @brief  HRPWM match special register event enable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Match special register event @ref HRPWM_Match_Special_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialEventEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MATCH_SPECIAL_EVT(u32Event));
    SET_REG32_BIT(HRPWMx->BGCONR1, u32Event);
}

/**
 * @brief  HRPWM match special register event disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Match special register event @ref HRPWM_Match_Special_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialEventDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MATCH_SPECIAL_EVT(u32Event));
    CLR_REG32_BIT(HRPWMx->GCONR1, u32Event);
}

/**
 * @brief  HRPWM match special register event disable (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Match special register event @ref HRPWM_Match_Special_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialEventDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MATCH_SPECIAL_EVT(u32Event));
    CLR_REG32_BIT(HRPWMx->BGCONR1, u32Event);
}

/**
 * @brief  Match Special register A event source enable
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Src                  Event source, Can be one or any combination of the values from
 *                                      @ref HRPWM_Match_Special_A_Event_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialAEventSrcEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_SCMA_SRC(u32Src));
    SET_REG32_BIT(HRPWMx->SCMASELR, u32Src);
}

/**
 * @brief  Match Special register A event source disable
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Src                  Event source, Can be one or any combination of the values from
 *                                      @ref HRPWM_Match_Special_A_Event_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MatchSpecialAEventSrcDisbale(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_SCMA_SRC(u32Src));
    CLR_REG32_BIT(HRPWMx->SCMASELR, u32Src);
}

/**
 * @brief  HRPWM dead time function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTCEN);
}

/**
 * @brief  HRPWM dead time function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTCEN);
}

/**
 * @brief  HRPWM Dead time set equal function, HRDTDAR automatically equal HRDTUAR
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EqualUpDown      HRPWM equal function @ref HRPWM_DeadTime_Reg_Equal_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeSetEqualUpDown(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EqualUpDown)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DEADTIME_EQUAL_FUNC(u32EqualUpDown));
    MODIFY_REG32(HRPWMx->DCONR, HRPWM_DCONR_SEPA, u32EqualUpDown);
}

/**
 * @brief  HRPWM general compare register A and E buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_GeneralAEBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENAE);
}

/**
 * @brief  HRPWM general compare register A and E buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_GeneralAEBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENAE);
}

/**
 * @brief  HRPWM general compare register B and F buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_GeneralBFBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENBF);
}

/**
 * @brief  HRPWM general compare register B and F buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_GeneralBFBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENBF);
}

/**
 * @brief  HRPWM period buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_PeriodBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENP);
}

/**
 * @brief  HRPWM period buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_PeriodBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENP);
}

/**
 * @brief  HRPWM special compare register A buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_SpecialABufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENSPA);
}

/**
 * @brief  HRPWM special compare register A buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_SpecialABufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENSPA);
}

/**
 * @brief  HRPWM special compare register B buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_SpecialBBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENSPB);
}

/**
 * @brief  HRPWM special compare register B buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_SpecialBBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENSPB);
}

/**
 * @brief  HRPWM external event filter window register buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_WindowBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENEEFWIN);
}

/**
 * @brief  HRPWM external event filter window register buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_WindowBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENEEFWIN);
}

/**
 * @brief  HRPWM external event filter offset register buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_OffsetBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENEEFOFF);
}

/**
 * @brief  HRPWM external event filter offset register buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_OffsetBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENEEFOFF);
}

/**
 * @brief  HRPWM control register buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_ControlRegBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENCTL);
}

/**
 * @brief  HRPWM control register buffer function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_ControlRegBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR2, HRPWM_BCONR2_BENCTL);
}

/**
 * @brief  Dead time buffer transfer for up count dead time register (DTUBR-->DTUAR) function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeUpBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBENU);
}

/**
 * @brief  Dead time buffer transfer for up count dead time register (DTUBR-->DTUAR) function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeUpBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBENU);
}

/**
 * @brief  Dead time buffer transfer for down count dead time register (DTDBR-->DTDAR) function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeDownBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBEND);
}

/**
 * @brief  Dead time buffer transfer for down count dead time register (DTDBR-->DTDAR) function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeDownBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBEND);
}

/**
 * @brief  HRPWM BM period register buffer function enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_PeriodBufEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMP | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM BM period register buffer function disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_PeriodBufDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMP | HRPWM_BM_FLAG_ALL, ~HRPWM_COMMON_BMCR_BENBMP);
}

/**
 * @brief  HRPWM BM compare register buffer function enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_CompareBufEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMCMP | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM BM compare register buffer function disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_CompareBufDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMCMP | HRPWM_BM_FLAG_ALL, ~HRPWM_COMMON_BMCR_BENBMCMP);
}

/**
 * @brief  HRPWM Set Phase function buffer transfer condition
 * @param  [in] u32BufTransCond     Buffer send condition @ref HRPWM_Buf_Trans_Cond_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetBufCond(uint32_t u32BufTransCond)
{
    DDL_ASSERT(IS_HRPWM_BUF_TRANS_COND(u32BufTransCond));
    MODIFY_REG32(CM_HRPWM1->PHSCTL, HRPWM_PHSCTL_BTRDPHS | HRPWM_PHSCTL_BTRUPHS,
                 u32BufTransCond << HRPWM_PHSCTL_BTRUPHS_POS);
}

/**
 * @brief  HRPWM1 Phase register buffer function enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_BufEnable(void)
{
    WRITE_REG32(bCM_HRPWM1->PHSCTL_b.BENPHS, 0x01UL);
}

/**
 * @brief  HRPWM1 Phase register buffer function disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_BufDisable(void)
{
    WRITE_REG32(bCM_HRPWM1->PHSCTL_b.BENPHS, 0x00UL);
}

/**
 * @brief  HRPWM1 buffer single transfer function configuration
 * @param  [in] u32Config           Buffer single transfer function @ref HRPWM_Buf_U1_Single_Trans_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_U1SingleTransBufConfig(uint32_t u32Config)
{
    DDL_ASSERT(IS_HRPWM_BUF_U1_SINGLE_TRANS_CONFIG(u32Config));
    WRITE_REG32(bCM_HRPWM_COMMON->GBCONR_b.OSTENU1, u32Config);
}

/**
 * @brief  HRPWM trigger HRPWM1 buffer single transfer point
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_U1SingleTransBufTrigger(void)
{
    WRITE_REG32(bCM_HRPWM_COMMON->GBCONR_b.OSTBTRU1, 0x01UL);
}

/**
 * @brief  HRPWM unit buffer function global enable
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_UnitGlobalBufEnable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    SET_REG32_BIT(CM_HRPWM_COMMON->GBCONR, u32Unit);
}

/**
 * @brief  HRPWM unit buffer function global disable
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_UnitGlobalBufDisable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    CLR_REG32_BIT(CM_HRPWM_COMMON->GBCONR, u32Unit);
}

/**
 * @brief  HRPWM Trigger the unit buffer transfer
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 * @note The function valid when HRPWM count stop
 */
__STATIC_INLINE void HRPWM_UnitSWBufTrigger(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    SET_REG32_BIT(CM_HRPWM_COMMON->GBCONR, u32Unit << HRPWM_COMMON_GBCONR_BTRU1SFT_POS);
}

/**
 * @brief  HRPWM get unit global buffer status flag
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_GetUnitGlobalBufStatus(uint32_t u32Unit)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));

    if (0UL != READ_REG32_BIT(CM_HRPWM_COMMON->GBSFLR, u32Unit)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  HRPWM get unit global buffer flag source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Src              Global buffer flag source, Can be one or any combination of the values from
 *                                  @ref HRPWM_Global_Buf_Flag_Src_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_GetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_U1_GLOBAL_BUF_FLAG_SRC(u32Src));
    } else {
        DDL_ASSERT(IS_HRPWM_U2_6_GLOBAL_BUF_FLAG_SRC(u32Src));
    }

    if (0UL != READ_REG32_BIT(HRPWMx->GCONR1, u32Src)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  HRPWM set unit global buffer flag source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Src              Global buffer flag source, Can be one or any combination of the values from
 *                                  @ref HRPWM_Global_Buf_Flag_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_U1_GLOBAL_BUF_FLAG_SRC(u32Src));
    } else {
        DDL_ASSERT(IS_HRPWM_U2_6_GLOBAL_BUF_FLAG_SRC(u32Src));
    }
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_BFSEL, u32Src);
}

/**
 * @brief  HRPWM clear unit global buffer status
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 * @note Please confirm that HRPWM specified unit buffer function is disable before clear status
 */
__STATIC_INLINE void HRPWM_ClearUnitGlobalBufStatus(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    MODIFY_REG32(CM_HRPWM_COMMON->GBSFLR, HRPWM_UNIT_ALL, ~u32Unit);
}

/**
 * @brief  HRPWM Idle set complete period point
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PeriodPoint      Complete period point @ref HRPWM_Complete_Period_Point_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_SetCompletePeriod(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodPoint)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CPLT_PERIOD_POINT(u32PeriodPoint));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_PRDSEL, u32PeriodPoint);
}

/**
 * @brief  HRPWM Idle set complete period point (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PeriodPoint      Complete period point @ref HRPWM_Complete_Period_Point_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_SetCompletePeriod_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodPoint)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CPLT_PERIOD_POINT(u32PeriodPoint));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_PRDSEL, u32PeriodPoint);
}

/**
 * @brief  HRPWM Set channel A output level in idle state
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Level            Output Level in idle state @ref HRPWM_Idle_ChA_Level_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_SetChAIdleLevel(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Level)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_CHA_LVL(u32Level));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_IDLESA, u32Level);
}

/**
 * @brief  HRPWM Set channel B output level in idle state
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Level            Output Level in idle state @ref HRPWM_Idle_ChB_Level_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_SetChBIdleLevel(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Level)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_CHB_LVL(u32Level));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_IDLESB, u32Level);
}

/**
 * @brief  HRPWM enter idle state immediately
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Sw_Sync_Unit_Define
 * @param  [in] u32Ch               HRPWM channel @ref HRPWM_Sw_Sync_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_EnterImmediate(uint32_t u32Unit, uint32_t u32Ch)
{
    DDL_ASSERT(IS_HRPWM_SW_SYNC_UNIT(u32Unit));
    DDL_ASSERT(IS_HRPWM_SW_SYNC_CH(u32Ch));
    SET_REG32_BIT(CM_HRPWM_COMMON->SSTAIDLR, u32Unit & u32Ch);
}

/**
 * @brief  HRPWM exit idle state
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Sw_Sync_Unit_Define
 * @param  [in] u32Ch               HRPWM channel @ref HRPWM_Sw_Sync_Ch_Define
 * @retval None
 * @note  PWM exit immediately idle state immediately, exit delay idle state after PWM signal match
 */
__STATIC_INLINE void HRPWM_IDLE_Exit(uint32_t u32Unit, uint32_t u32Ch)
{
    DDL_ASSERT(IS_HRPWM_SW_SYNC_UNIT(u32Unit));
    DDL_ASSERT(IS_HRPWM_SW_SYNC_CH(u32Ch));
    SET_REG32_BIT(CM_HRPWM_COMMON->SSTARUNR1, u32Unit & u32Ch);
}

/**
 * @brief  HRPWM exit idle state, the HRPWM will enter run state when count start
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Sw_Sync_Unit_Define
 * @param  [in] u32Ch               HRPWM channel @ref HRPWM_Sw_Sync_Ch_Define
 * @retval None
 * @note   The function valid when HRPWM count stop
 */
__STATIC_INLINE void HRPWM_IDLE_ExitForCountStart(uint32_t u32Unit, uint32_t u32Ch)
{
    DDL_ASSERT(IS_HRPWM_SW_SYNC_UNIT(u32Unit));
    DDL_ASSERT(IS_HRPWM_SW_SYNC_CH(u32Ch));
    SET_REG32_BIT(CM_HRPWM_COMMON->SSTARUNR2, u32Unit & u32Ch);
}

/**
 * @brief  HRPWM get channel status, run or idle
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Sw_Sync_Unit_Define
 * @param  [in] u32Ch               HRPWM channel @ref HRPWM_Sw_Sync_Ch_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 *  @arg SET: The channel is in run status
 *  @arg RESET: The channel is in idle status
 * @note  Only for immediately idle state and BM idle state
 */
__STATIC_INLINE en_flag_status_t HRPWM_IDLE_GetChStatus(uint32_t u32Unit, uint32_t u32Ch)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_SW_SYNC_UNIT(u32Unit));
    DDL_ASSERT(IS_HRPWM_SW_SYNC_CH(u32Ch));

    if (0UL != READ_REG32_BIT(CM_HRPWM_COMMON->SSTARUNR1, u32Unit & u32Ch)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  HRPWM idle delay function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_Enable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_DLYPRTEN);
}

/**
 * @brief  HRPWM idle delay function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_Disable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_DLYPRTEN);
}

/**
 * @brief  HRPWM Set idle delay trigger source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32TriggerSrc       Idle delay output trigger source @ref HRPWM_Idle_Delay_Trigger_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_SetTriggerSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32TriggerSrc)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_DELAY_TRIG_SRC(u32TriggerSrc));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_DLYEVSEL, u32TriggerSrc);
}

/**
 * @brief  HRPWM set idle delay output channel A status
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChStatus         Idle delay channel output status @ref HRPWM_Idle_Output_ChA_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_SetOutputChAStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_OUTPUT_CHA_STAT(u32ChStatus));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_DLYCHA, u32ChStatus);
}

/**
 * @brief  HRPWM set idle delay output channel B status
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChStatus         Idle delay channel output status @ref HRPWM_Idle_Output_ChB_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_SetOutputChBStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_OUTPUT_CHB_STAT(u32ChStatus));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_DLYCHB, u32ChStatus);
}

/**
 * @brief  HRPWM idle delay function interrupt enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_IntEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_INTENDLYPRT);
}

/**
 * @brief  HRPWM idle delay function interrupt disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_IntDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_INTENDLYPRT);
}

/**
 * @brief  Get HRPWM idle delay status flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_Idle_Delay_Flag_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_IDLE_DELAY_GetStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_DELAY_FLAG(u32Flag));

    if (0UL != READ_REG32_BIT(HRPWMx->STFLR2, u32Flag)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  Clear HRPWM idle delay status flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Flag             Status bit to be clear
 *  @arg HRPWM_IDLE_DELAY_FLAG_STAT
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_ClearStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Flag)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_IDLE_DELAY_CLR_FLAG(u32Flag));
    MODIFY_REG32(HRPWMx->STFLR2, HRPWM_STFLR2_CLR_MASK, ~u32Flag);
}

/**
 * @brief  HRPWM idle delay output software trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 * @note The function valid when the software trigger source is enable for idle delay function, which is configured by
 *       API HRPWM_IDLE_DELAY_SetTriggerSrc() or HRPWM_IDLE_DELAY_Init()
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_SWTrigger(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_DLYSTRG);
}

/**
 * @brief  HRPWM idle delay output multi-unit software trigger
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 * @note The function valid when the software trigger source is enable for idle delay function, which is configured by
 *       API HRPWM_IDLE_DELAY_SetTriggerSrc() or HRPWM_IDLE_DELAY_Init()
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_MultiUnitSWTrigger(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    SET_REG32_BIT(CM_HRPWM_COMMON->SSTADIDLR, u32Unit);
}

/**
 * @brief  HRPWM idle BM function enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_Enable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMEN | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM function disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_Disable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMEN | HRPWM_BM_FLAG_ALL, ~HRPWM_COMMON_BMCR_BMEN);
}

/**
 * @brief  HRPWM idle BM set action mode
 * @param  [in] u32Mode                 BM action mode @ref HRPWM_Idle_BM_Action_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetActionMode(uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_BM_ACTION_MD(u32Mode));
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMMD | HRPWM_BM_FLAG_ALL, u32Mode | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM set count source
 * @param  [in] u32CountSrc             BM count source @ref HRPWM_Idle_BM_Count_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetCountSrc(uint32_t u32CountSrc)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_SRC(u32CountSrc));
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMCLKS | HRPWM_BM_FLAG_ALL, u32CountSrc | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM set PCLK divider
 * @param  [in] u32PclkDiv              PCLK divider @ref HRPWM_Idle_BM_Count_Src_Pclk_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetPclkDiv(uint32_t u32PclkDiv)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_PCLK0_DIV(u32PclkDiv));
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMPSC | HRPWM_BM_FLAG_ALL, u32PclkDiv | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM set count reload
 * @param  [in] u32CountReload          Count reload after count peak, @ref HRPWM_Idle_BM_Count_Reload_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetCountReload(uint32_t u32CountReload)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_RELOAD(u32CountReload));
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMCTN | HRPWM_BM_FLAG_ALL, u32CountReload | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM output start trigger source
 * @param  [in] u64TriggerSrc           BM output start trigger source 1, Can be one or any combination of the values
                                        from @ref HRPWM_Idle_BM_Trigger_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetTriggerSrc(uint64_t u64TriggerSrc)
{
    DDL_ASSERT(IS_HRPWM_BM_TRIG_SRC(u64TriggerSrc));
    MODIFY_REG32(CM_HRPWM_COMMON->BMSTRG1, HRPWM_BM_TRIG_ALL, u64TriggerSrc);
    WRITE_REG32(CM_HRPWM_COMMON->BMSTRG2, u64TriggerSrc >> 32U);
}

/**
 * @brief  HRPWM set idle BM output channel A status
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChStatus             BM Output Channel status @ref HRPWM_Idle_BM_Output_ChA_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetOutputChAStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_OUTPUT_CHA_STAT(u32ChStatus));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_IDLEBMA, u32ChStatus);
}

/**
 * @brief  HRPWM set idle BM output channel B status
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ChStatus             BM Output Channel status @ref HRPWM_Idle_BM_Output_ChB_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetOutputChBStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_OUTPUT_CHB_STAT(u32ChStatus));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_IDLEBMB, u32ChStatus);
}

/**
 * @brief  HRPWM channel A BM output enter idle delay function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EnterDelay           BM output enter idle delay function @ref HRPWM_Idle_ChA_BM_Output_Delay_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetChAEnterDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EnterDelay)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_DELAY_ENTER_CHA_STAT(u32EnterDelay));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_DIDLA, u32EnterDelay);
}

/**
 * @brief  HRPWM channel B BM output enter idle delay function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EnterDelay           BM output enter idle delay function @ref HRPWM_Idle_ChB_BM_Output_Delay_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetChBEnterDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EnterDelay)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_DELAY_ENTER_CHB_STAT(u32EnterDelay));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_DIDLB, u32EnterDelay);
}

/**
 * @brief  HRPWM BM set channel follow function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Follow               BM channel follow function @ref HRPWM_Idle_BM_Output_Follow_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetChFollow(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Follow)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_FOLLOW_FUNC(u32Follow));
    MODIFY_REG32(HRPWMx->IDLECR, HRPWM_IDLECR_FOLLOW, u32Follow);
}

/**
 * @brief  HRPWM  set unit count stop and reset status in BM output state
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32UnitCountReset       BM output count stop and reset function @ref HRPWM_Idle_BM_Unit_Count_Reset_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetUnitCountReset(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32UnitCountReset)
{
    uint32_t u32UnitNum;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_BM_UNIT_CNT_RST_FUNC(u32UnitCountReset));
    u32UnitNum = ((uint32_t)HRPWMx - CM_HRPWM1_BASE) / (CM_HRPWM2_BASE - CM_HRPWM1_BASE);
    if (HRPWM_BM_UNIT_CNT_CONTINUE == u32UnitCountReset) {
        MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_BMCR_BMTMR_MASK | HRPWM_BM_FLAG_ALL,
                     ~(HRPWM_COMMON_BMCR_BMTMR1 << u32UnitNum));
    } else {
        SET_REG32_BIT(CM_HRPWM_COMMON->BMCR, (HRPWM_COMMON_BMCR_BMTMR1 << u32UnitNum) | HRPWM_BM_FLAG_ALL);
    }
}

/**
 * @brief  HRPWM idle BM set period register
 * @param  [in] u32Value                BM count period register value, range [0x0000, 0xFFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetPeriodReg(uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(u32Value));
    WRITE_REG32(CM_HRPWM_COMMON->BMPERAR, u32Value);
}

/**
 * @brief  HRPWM idle BM set period register (buffer)
 * @param  [in] u32Value                BM count period register value, range [0x0000, 0xFFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetPeriodReg_Buf(uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(u32Value));
    WRITE_REG32(CM_HRPWM_COMMON->BMPERBR, u32Value);
}

/**
 * @brief  HRPWM idle BM set compare register
 * @param  [in] u32Value                BM compare register value, range [0x0000, 0xFFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetCompareReg(uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(u32Value));
    WRITE_REG32(CM_HRPWM_COMMON->BMCMAR, u32Value);
}

/**
 * @brief  HRPWM idle BM set compare register (buffer)
 * @param  [in] u32Value                BM compare register value, range [0x0000, 0xFFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SetCompareReg_Buf(uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_BM_CNT_REG_RANGE(u32Value));
    WRITE_REG32(CM_HRPWM_COMMON->BMCMBR, u32Value);
}

/**
 * @brief  HRPWM idle BM count peak interrupt enable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_PeakIntEnable(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_INTENBMOVF | HRPWM_BM_FLAG_ALL);
}

/**
 * @brief  HRPWM idle BM count peak interrupt disable
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_PeakIntDisable(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_INTENBMOVF | HRPWM_BM_FLAG_ALL, ~HRPWM_COMMON_BMCR_INTENBMOVF);
}

/**
 * @brief  Get HRPWM BM status flag
 * @param  [in] u32Flag                 Status bit to be read, Can be one or any combination of the values from
 *                                      @ref HRPWM_Idle_BM_Flag_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_IDLE_BM_GetStatus(uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_BM_FLAG(u32Flag));

    if (0UL != READ_REG32_BIT(CM_HRPWM_COMMON->BMCR, u32Flag)) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  Clear HRPWM BM status flag
 * @param  [in] u32Flag                 Status bit to be clear,
 *   @arg HRPWM_BM_FLAG_PEAK
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_HRPWM_BM_CLR_FLAG(u32Flag));
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_BM_FLAG_ALL, ~u32Flag);
}

/**
 * @brief  HRPWM idle BM output software trigger
 * @param  None
 * @retval None
 * @note Please confirm that BM output function is enabled by API HRPWM_IDLE_BM_Enable()
 */
__STATIC_INLINE void HRPWM_IDLE_BM_SWTrigger(void)
{
    SET_REG32_BIT(CM_HRPWM_COMMON->BMSTRG1, HRPWM_COMMON_BMSTRG1_SSTRG);
}

/**
 * @brief  HRPWM exit BM output state
 * @param  None
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BM_Exit(void)
{
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_BM_FLAG_ALL, ~HRPWM_BM_FLAG_OP);
}

/**
 * @brief  HRPWM valid period set special A function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32SpecialA             Special A function @ref HRPWM_Valid_Period_Special_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetValidPeriodSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialA)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PERIOD_SPECIAL_A_FUNC(u32SpecialA));
    MODIFY_REG32(HRPWMx->VPERR, HRPWM_VPERR_SPPERIA, u32SpecialA);
}

/**
 * @brief  HRPWM valid period set special B function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32SpecialB             Special B function @ref HRPWM_Valid_Period_Special_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetValidPeriodSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialB)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PERIOD_SPECIAL_B_FUNC(u32SpecialB));
    MODIFY_REG32(HRPWMx->VPERR, HRPWM_VPERR_SPPERIB, u32SpecialB);
}

/**
 * @brief  HRPWM valid period set interval
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Interval             Period count interval, range from 0 ~ 31
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetValidPeriodInterval(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Interval)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PERIOD_INTERVAL(u32Interval));
    MODIFY_REG32(HRPWMx->VPERR, HRPWM_VPERR_PCNTS, u32Interval << HRPWM_VPERR_PCNTS_POS);
}

/**
 * @brief  HRPWM valid period set count condition
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32CountCond            Count condition @ref HRPWM_Valid_Period_Count_Cond_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetValidPeriodCountCond(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32CountCond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PERIOD_CNT_COND(u32CountCond));
    MODIFY_REG32(HRPWMx->VPERR, HRPWM_VPERR_PCNTE, u32CountCond);
}

/**
 * @brief  Get HRPWM period number when valid period function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Data for periods number
 */
__STATIC_INLINE uint32_t HRPWM_GetValidPeriodPeriodNum(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return (READ_REG32_BIT(HRPWMx->STFLR1, HRPWM_STFLR1_VPERNUM) >> HRPWM_STFLR1_VPERNUM_POS);
}

/**
 * @brief  Hardware start condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware start, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Start_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND(u64Cond));
    SET_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HSTAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware start condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware start, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Start_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND(u64Cond));
    CLR_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HSTAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware clear condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND(u64Cond));
    SET_REG32_BIT(HRPWMx->HCLRR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCLRR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware clear condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCLRR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HCLRR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware start function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 * @note The software synchronous start function invalid when hardware start function enable
 */
__STATIC_INLINE void HRPWM_HWStartEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->HSTAR1, HRPWM_HSTAR1_STAS);
}

/**
 * @brief  Hardware start function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->HSTAR1, HRPWM_HSTAR1_STAS);
}

/**
 * @brief  Hardware clear function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 * @note The software synchronous clear function invalid when hardware clear function enable
 */
__STATIC_INLINE void HRPWM_HWClearEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->HCLRR1, HRPWM_HCLRR1_CLES);
}

/**
 * @brief  Hardware clear function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->HCLRR1, HRPWM_HCLRR1_CLES);
}

/**
 * @brief  Software synchronous start
 * @param  [in]  u32Unit            Software Sync units, This parameter can be one or any combination of the parameter
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 * @note The software synchronous start function invalid when hardware start function enable by API HRPWM_HWStartEnable()
 */
__STATIC_INLINE void HRPWM_SWSyncStart(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    WRITE_REG32(CM_HRPWM_COMMON->SSTAR, u32Unit);
}

/**
 * @brief  Software synchronous stop
 * @param  [in]  u32Unit            Software Sync units, This parameter can be one or any combination of the parameter
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SWSyncStop(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    WRITE_REG32(CM_HRPWM_COMMON->SSTPR, u32Unit);
}

/**
 * @brief  Software synchronous clear
 * @param  [in]  u32Unit            Software Sync units, This parameter can be one or any combination of the parameter
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 * @note The software synchronous clear function invalid when hardware clear function enable by API HRPWM_HWClearEnable()
 */
__STATIC_INLINE void HRPWM_SWSyncClear(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    WRITE_REG32(CM_HRPWM_COMMON->SCLRR, u32Unit);
}

/**
 * @brief  Software synchronous update
 * @param  [in]  u32Unit            Software Sync units, This parameter can be one or any combination of the parameter
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SWSyncUpdate(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    WRITE_REG32(CM_HRPWM_COMMON->SUPDR, u32Unit);
}

/**
 * @brief  HRPWM External event set event source
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32EventSrc         External event source @ref HRPWM_EVT_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetSrc(uint32_t u32EventNum, uint32_t u32EventSrc)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_SRC(u32EventSrc));
    if (u32EventNum <= HRPWM_EVT5) {
        /* Event1 ~ Event5 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR1, HRPWM_COMMON_EECR1_EE1SRC << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum),
                     u32EventSrc << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum));
    } else {
        /* Event6 ~ Event10 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR2, HRPWM_COMMON_EECR2_EE6SRC << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)),
                     u32EventSrc << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  External event set event source 2
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32EventSrc2        External event source @ref HRPWM_EVT_Src2_Define
 * @retval None
 * @note The function valid when External event source is HRPWM_EVT_SRC2
 */
__STATIC_INLINE void HRPWM_EVT_SetSrc2(uint32_t u32EventNum, uint32_t u32EventSrc2)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_SRC2(u32EventSrc2));
    if (u32EventNum <= HRPWM_EVT2) {
        MODIFY_REG32(CM_HRPWM_COMMON->GCTLR, HRPWM_COMMON_GCTLR_EE1SRC2 << (HRPWM_COMMON_GCTLR_EE2SRC2_POS * u32EventNum),
                     u32EventSrc2 << (HRPWM_COMMON_GCTLR_EE2SRC2_POS * u32EventNum));
    } else {
        MODIFY_REG32(CM_HRPWM_COMMON->GCTLR, HRPWM_COMMON_GCTLR_EE1SRC2 << (HRPWM_COMMON_GCTLR_EE3SRC2_POS + HRPWM_COMMON_GCTLR_EE2SRC2_POS * (u32EventNum - HRPWM_EVT3)),
                     u32EventSrc2 << (HRPWM_COMMON_GCTLR_EE3SRC2_POS + HRPWM_COMMON_GCTLR_EE2SRC2_POS * (u32EventNum - HRPWM_EVT3)));
    }
}

/**
 * @brief  External event set valid action
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32ValidAction      External event valid action @ref HRPWM_EVT_Valid_Action_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetValidAction(uint32_t u32EventNum, uint32_t u32ValidAction)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_VALID_ACTION(u32ValidAction));
    if (u32EventNum <= HRPWM_EVT5) {
        /* Event1 ~ Event5 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR1, HRPWM_COMMON_EECR1_EE1SNS << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum),
                     u32ValidAction << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum));
    } else {
        /* Event6 ~ Event10 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR2, HRPWM_COMMON_EECR2_EE6SNS << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)),
                     u32ValidAction << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  External event set valid level
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32ValidLevel       External event valid level @ref HRPWM_EVT_Valid_Level_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetValidLevel(uint32_t u32EventNum, uint32_t u32ValidLevel)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_VALID_LVL(u32ValidLevel));
    if (u32EventNum <= HRPWM_EVT5) {
        /* Event1 ~ Event5 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR1, HRPWM_COMMON_EECR1_EE1POL << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum),
                     u32ValidLevel << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum));
    } else {
        /* Event6 ~ Event10 */
        MODIFY_REG32(CM_HRPWM_COMMON->EECR2, HRPWM_COMMON_EECR2_EE6POL << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)),
                     u32ValidLevel << (HRPWM_COMMON_EECR2_EE7SRC_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  External event set asynchronous mode
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32FastAsync        External event fast asynchronous mode @ref HRPWM_EVT_Fast_Async_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetFastAsyncMode(uint32_t u32EventNum, uint32_t u32FastAsync)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM_FAST_MD(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_FAST_ASYNC_MD(u32FastAsync));
    MODIFY_REG32(CM_HRPWM_COMMON->EECR1, HRPWM_COMMON_EECR1_EE1FAST << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum),
                 u32FastAsync << (HRPWM_COMMON_EECR1_EE2SRC_POS * u32EventNum));
}

/**
 * @brief  External event set filter clock (only for EVT6 ~ EVT10)
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32Clock            Filter clock select @ref HRPWM_EVT_Filter_Clock_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetFilterClock(uint32_t u32EventNum, uint32_t u32Clock)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM_FILTER(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_CLK(u32Clock));
    MODIFY_REG32(CM_HRPWM_COMMON->EECR3, HRPWM_COMMON_EECR3_EE6F << (HRPWM_COMMON_EECR3_EE7F_POS * (u32EventNum - HRPWM_EVT6)),
                 u32Clock << (HRPWM_COMMON_EECR3_EE7F_POS * (u32EventNum - HRPWM_EVT6)));
}

/**
 * @brief  External event set generate external event detected event function
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32GenerateEvent    Generate detect event function @ref HRPWM_EVT_Generate_Detect_Fun_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetGenerateEvent(uint32_t u32EventNum, uint32_t u32GenerateEvent)
{
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_GEN_DETECT_EVT_FUNC(u32GenerateEvent));
    MODIFY_REG32(CM_HRPWM_COMMON->EEDSELR, HRPWM_EVT_GEN_DETECT_ON << u32EventNum, u32GenerateEvent << u32EventNum);
}

/**
 * @brief  External event set EEVS clock for filter clock (only for EVT6 ~ EVT10)
 * @param  [in] u32Clock       EEVS clock select @ref HRPWM_EVT_Eevs_Clock_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetEEVSClock(uint32_t u32Clock)
{
    DDL_ASSERT(IS_HRPWM_EVT_EEVS_CLK(u32Clock));
    MODIFY_REG32(CM_HRPWM_COMMON->EECR3, HRPWM_COMMON_EECR3_EEVSD, u32Clock);
}

/**
 * @brief  External event filter set filter replace mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Filter mode @ref HRPWM_EVT_Filter_Signal_Replace_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetReplaceMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_SIGNAL_REPLACE(u32Mode));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF, u32Mode);
}

/**
 * @brief  External event filter set filter replace mode (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Filter mode @ref HRPWM_EVT_Filter_Signal_Replace_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetReplaceMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_SIGNAL_REPLACE(u32Mode));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF, u32Mode);
}

/**
 * @brief  External event filter set filter mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32Mode             Filter mode @ref HRPWM_EVT_Filter_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_MD(u32Mode));
    if (u32EventNum <= HRPWM_EVT5) {
        MODIFY_REG32(HRPWMx->EEFLTCR1, HRPWM_EEFLTCR1_EE1FM << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum),
                     u32Mode << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum));
    } else {
        MODIFY_REG32(HRPWMx->EEFLTCR2, HRPWM_EEFLTCR1_EE1FM << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)),
                     u32Mode << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  External event filter set filter latch function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32Latch            Event Latch function @ref HRPWM_EVT_Filter_Latch_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetLatch(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Latch)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_LATCH_FUNC(u32Latch));
    if (u32EventNum <= HRPWM_EVT5) {
        MODIFY_REG32(HRPWMx->EEFLTCR1, HRPWM_EEFLTCR1_EE1LAT << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum),
                     u32Latch << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum));
    } else {
        MODIFY_REG32(HRPWMx->EEFLTCR2, HRPWM_EEFLTCR1_EE1LAT << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)),
                     u32Latch << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  External event filter set filter time out function for window mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @param  [in] u32Timeout          Timeout function HRPWM_EVT_Filter_Timeout_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetTimeout(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Timeout)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_EVT_TIMEOUT_FUNC(u32Timeout));
    if (u32EventNum <= HRPWM_EVT5) {
        MODIFY_REG32(HRPWMx->EEFLTCR1, HRPWM_EEFLTCR1_EE1TMO << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum),
                     u32Timeout << (HRPWM_EEFLTCR1_EE2LAT_POS * u32EventNum));
    } else {
        MODIFY_REG32(HRPWMx->EEFLTCR2, HRPWM_EEFLTCR1_EE1TMO << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)),
                     u32Timeout << (HRPWM_EEFLTCR1_EE2LAT_POS * (u32EventNum - HRPWM_EVT6)));
    }
}

/**
 * @brief  HRPWM external event filter set offset direction
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32OffsetDir        Offset direction, @ref HRPWM_EVT_Filter_Offset_Dir_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetOffsetDir(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32OffsetDir)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_OFS_DIR(u32OffsetDir));
    MODIFY_REG32(HRPWMx->EEFOFFSETAR, HRPWM_EEFOFFSETAR_OFFSETDIR, u32OffsetDir);
}

/**
 * @brief  HRPWM external event filter set offset direction (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32OffsetDir        Offset direction, @ref HRPWM_EVT_Filter_Offset_Dir_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetOffsetDir_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32OffsetDir)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_OFS_DIR(u32OffsetDir));
    MODIFY_REG32(HRPWMx->EEFOFFSETBR, HRPWM_EEFOFFSETBR_OFFSETDIR, u32OffsetDir);
}

/**
 * @brief  HRPWM external event filter set offset value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Offset           Offset register value, range [0x00, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetOffset(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Offset)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Offset));
    MODIFY_REG32(HRPWMx->EEFOFFSETAR, HRPWM_EEFOFFSETAR_OFFSET, u32Offset);
}

/**
 * @brief  HRPWM external event filter set offset value (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Offset           Offset register value, range [0x00, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetOffset_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Offset)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Offset));
    MODIFY_REG32(HRPWMx->EEFOFFSETBR, HRPWM_EEFOFFSETBR_OFFSET, u32Offset);
}

/**
 * @brief  External event filter set Window direction
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32WindowDir        Window direction, @ref HRPWM_EVT_Filter_Window_Dir_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetWindowDir(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32WindowDir)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_WIN_DIR(u32WindowDir));
    MODIFY_REG32(HRPWMx->EEFWINAR, HRPWM_EEFWINAR_WINDIR, u32WindowDir);
}

/**
 * @brief  External event filter set Window direction (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32WindowDir        Window direction, @ref HRPWM_EVT_Filter_Window_Dir_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetWindowDir_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32WindowDir)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_WIN_DIR(u32WindowDir));
    MODIFY_REG32(HRPWMx->EEFWINBR, HRPWM_EEFWINBR_WINDIR, u32WindowDir);
}

/**
 * @brief  HRPWM external event filter set window value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Window           window register value, range [0x00, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetWindow(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Window)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Window));
    MODIFY_REG32(HRPWMx->EEFWINAR, HRPWM_EEFWINAR_WIN, u32Window);
}

/**
 * @brief  HRPWM external event filter set window value (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Window           window register value, range [0x00, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetWindow_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Window)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Window));
    MODIFY_REG32(HRPWMx->EEFWINBR, HRPWM_EEFWINBR_WIN, u32Window);
}

/**
 * @brief  External event filter set initial polarity
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32InitPolarity     Filter signal initial level @ref HRPWM_EVT_Init_Polarity_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterSetInitPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32InitPolarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_FILTER_INIT_POLARITY(u32InitPolarity));
    MODIFY_REG32(HRPWMx->EEFLTCR1, HRPWM_EEFLTCR1_EEINTPOL, u32InitPolarity);
}

/**
 * @brief  HRPWM external event filter blank delay enable
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterBlankDelayEnable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    SET_REG32_BIT(CM_HRPWM_COMMON->GCTLR, u32Unit << HRPWM_COMMON_GCTLR_BKDLY1_POS);
}

/**
 * @brief  HRPWM external event filter blank delay disable
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_FilterBlankDelayDisable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    CLR_REG32_BIT(CM_HRPWM_COMMON->GCTLR, u32Unit << HRPWM_COMMON_GCTLR_BKDLY1_POS);
}

/**
 * @brief  HRPWM set synchronous output source
 * @param  [in] u32Src              Synchronous output source @ref HRPWM_Sync_Output_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SYNC_SetSrc(uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_SYNC_SRC(u32Src));
    MODIFY_REG32(CM_HRPWM_COMMON->SYNOCR, HRPWM_COMMON_SYNOCR_SYNCSRC, u32Src);
}

/**
 * @brief  HRPWM synchronous output Set count direction when match special B for output source
 * @param  [in] u32MatchBDir        Count direction when match special B @ref HRPWM_Sync_Output_Match_B_Dir_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SYNC_SetMatchBDir(uint32_t u32MatchBDir)
{
    DDL_ASSERT(IS_HRPWM_SYNC_MATCH_B_DIR(u32MatchBDir));
    MODIFY_REG32(CM_HRPWM_COMMON->SYNOCR, HRPWM_COMMON_SYNOCR_SRCDIR, u32MatchBDir);
}

/**
 * @brief  HRPWM set synchronous output pulse
 * @param  [in] u32Pulse            Output pulse function @ref HRPWM_Sync_Output_Pulse_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SYNC_SetPulse(uint32_t u32Pulse)
{
    DDL_ASSERT(IS_HRPWM_SYNC_PULSE(u32Pulse));
    MODIFY_REG32(CM_HRPWM_COMMON->SYNOCR, HRPWM_COMMON_SYNOCR_SYNOPLS, u32Pulse);
}

/**
 * @brief  HRPWM set synchronous output pulse width
 * @param  [in] u32PulseWidth       Output pulse width, range (0x10, 0xFF]
 *                                  Width = T(PCLK0) * u32PulseWidth
 * @retval None
 */
__STATIC_INLINE void HRPWM_SYNC_SetPulseWidth(uint32_t u32PulseWidth)
{
    DDL_ASSERT(IS_HRPWM_SYNC_PULSE_WIDTH(u32PulseWidth));
    MODIFY_REG32(CM_HRPWM_COMMON->SYNOCR, HRPWM_COMMON_SYNOCR_SYNCMP, u32PulseWidth);
}

/**
 * @brief  HRPWM DAC set synchronous trigger source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Src              DAC trigger source @ref HRPWM_DAC_Trigger_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_SetTriggerSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DAC_TRIG_SRC(u32Src));
    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DACSRC, u32Src);
}

/**
 * @brief  HRPWM DAC set synchronous trigger destination for DAC channel 1
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32DacCh1Dest       Trigger destination for DAC channel 1 @ref HRPWM_DAC_Ch1_Trigger_Dest_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_SetCh1Dest(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32DacCh1Dest)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DAC_CH1_TRIG_DEST(u32DacCh1Dest));
    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DACSYNC1, u32DacCh1Dest);
}

/**
 * @brief  HRPWM DAC set synchronous trigger destination for DAC channel 2
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32DacCh2Dest       Trigger destination for DAC channel 2 @ref HRPWM_DAC_Ch2_Trigger_Dest_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_SetCh2Dest(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32DacCh2Dest)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DAC_CH2_TRIG_DEST(u32DacCh2Dest));
    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DACSYNC2, u32DacCh2Dest);
}

/**
 * @brief  HRPWM Phase function enable for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_Enable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->PHSCTL, HRPWM_PHSCTL_PHSEN);
}

/**
 * @brief  HRPWM Phase function disable for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_Disable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->PHSCTL, HRPWM_PHSCTL_PHSEN);
}

/**
 * @brief  HRPWM Phase set phase match event index from master unit for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] u32PhaseIndex       Phase match event index @ref HRPWM_PH_Index_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetPhaseIndex(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PhaseIndex)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    MODIFY_REG32(HRPWMx->PHSCTL, HRPWM_PHSCTL_PHCMPSEL, u32PhaseIndex << HRPWM_PHSCTL_PHCMPSEL_POS);
}

/**
 * @brief  HRPWM Phase set force Channel A function for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] u32ForceChA         Force channel A function @ref HRPWM_PH_Force_ChA_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetForceChA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ForceChA)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PH_MATCH_FORCE_CHA_FUNC(u32ForceChA));
    MODIFY_REG32(HRPWMx->PHSCTL, HRPWM_PHSCTL_PHSFORCA, u32ForceChA);
}

/**
 * @brief  HRPWM Phase set force Channel B function for specified HRPWM unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] u32ForceChB         Force channel B function @ref HRPWM_PH_Force_ChB_Func_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetForceChB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ForceChB)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PH_MATCH_FORCE_CHB_FUNC(u32ForceChB));
    MODIFY_REG32(HRPWMx->PHSCTL, HRPWM_PHSCTL_PHSFORCB, u32ForceChB);
}

/**
 * @brief  HRPWM Phase set period link function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] u32PeriodLink       Period link function @ref HRPWM_PWM_PH_Period_Link_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetPeriodLink(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodLink)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PH_PERIOD_LINK(u32PeriodLink));
    MODIFY_REG32(HRPWMx->GCONR1, HRPWM_GCONR1_PRDLK, u32PeriodLink);
}

/**
 * @brief  HRPWM Phase set period link function (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWM2 ~ CM_HRPWM6
 * @param  [in] u32PeriodLink       Period link function @ref HRPWM_PWM_PH_Period_Link_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetPeriodLink_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodLink)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PH_PERIOD_LINK(u32PeriodLink));
    MODIFY_REG32(HRPWMx->BGCONR1, HRPWM_GCONR1_PRDLK, u32PeriodLink);
}

/**
 * @brief  HRPWM phase get status flag
 * @param  [in] u32Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_PH_Status_Flag_Define
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_PH_GetStatus(uint32_t u32Flag)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_PH_FLAG(u32Flag));

    if (0UL != (READ_REG32_BIT(CM_HRPWM1->STFLR2, u32Flag))) {
        enStatus = SET;
    }
    return enStatus;
}

/**
 * @brief  HRPWM phase clear status flag
 * @param  [in] u32Flag             Status bit to be read, Can be one or any combination of the values from
 *                                  @ref HRPWM_PH_Status_Flag_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_ClearStatus(uint32_t u32Flag)
{
    DDL_ASSERT(IS_HRPWM_PH_FLAG(u32Flag));
    MODIFY_REG32(CM_HRPWM1->STFLR2, HRPWM_STFLR2_CLR_MASK, ~u32Flag);
}

/**
 * @brief  HRPWM channel A EMB set valid EMB event channel
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ValidCh          Valid EMB event channel @ref HRPWM_Emb_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetValidCh(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_CH(u32ValidCh));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_EMBSA, u32ValidCh);
}

/**
 * @brief  HRPWM channel A EMB set valid EMB event channel (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ValidCh          Valid EMB event channel @ref HRPWM_Emb_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetValidCh_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_CH(u32ValidCh));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_EMBSA, u32ValidCh);
}

/**
 * @brief  HRPWM channel B EMB set valid EMB event channel
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ValidCh          Valid EMB event channel @ref HRPWM_Emb_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetValidCh(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_CH(u32ValidCh));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_EMBSB, u32ValidCh);
}

/**
 * @brief  HRPWM channel B EMB set valid EMB event channel (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ValidCh          Valid EMB event channel @ref HRPWM_Emb_Ch_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetValidCh_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_CH(u32ValidCh));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_EMBSB, u32ValidCh);
}

/**
 * @brief  HRPWM channel A EMB set release mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ReleaseMode      Pin release mode when EMB event invalid @ref HRPWM_Emb_Release_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetReleaseMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(u32ReleaseMode));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_EMBRA, u32ReleaseMode);
}

/**
 * @brief  HRPWM channel A EMB set release mode (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ReleaseMode      Pin release mode when EMB event invalid @ref HRPWM_Emb_Release_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetReleaseMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(u32ReleaseMode));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_EMBRA, u32ReleaseMode);
}

/**
 * @brief  HRPWM channel B EMB set release mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ReleaseMode      Pin release mode when EMB event invalid @ref HRPWM_Emb_Release_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetReleaseMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(u32ReleaseMode));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_EMBRB, u32ReleaseMode);
}

/**
 * @brief  HRPWM channel B EMB set release mode (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32ReleaseMode      Pin release mode when EMB event invalid @ref HRPWM_Emb_Release_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetReleaseMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_RELEASE_MD(u32ReleaseMode));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_EMBRB, u32ReleaseMode);
}

/**
 * @brief  HRPWM channel A output status when EMB event valid
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PinStatus        Pin output status when EMB event valid @ref HRPWM_Emb_Pin_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetPinStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(u32PinStatus));
    MODIFY_REG32(HRPWMx->PCNAR1, HRPWM_PCNAR1_EMBCA, u32PinStatus);
}

/**
 * @brief  HRPWM channel A output status when EMB event valid (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PinStatus        Pin output status when EMB event valid @ref HRPWM_Emb_Pin_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChASetPinStatus_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(u32PinStatus));
    MODIFY_REG32(HRPWMx->BPCNAR1, HRPWM_PCNAR1_EMBCA, u32PinStatus);
}

/**
 * @brief  HRPWM channel B output status when EMB event valid
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PinStatus        Pin output status when EMB event valid @ref HRPWM_Emb_Pin_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetPinStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(u32PinStatus));
    MODIFY_REG32(HRPWMx->PCNBR1, HRPWM_PCNBR1_EMBCB, u32PinStatus);
}

/**
 * @brief  HRPWM channel B output status when EMB event valid (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32PinStatus        Pin output status when EMB event valid @ref HRPWM_Emb_Pin_Status_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ChBSetPinStatus_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EMB_VALID_PIN_STAT(u32PinStatus));
    MODIFY_REG32(HRPWMx->BPCNBR1, HRPWM_PCNBR1_EMBCB, u32PinStatus);
}

/**
 * @brief  HRPWM set counter value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Counter value, range [0x00, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCountValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->CNTER, u32Value);
}

/**
 * @brief  HRPWM get counter value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Counter value
 */
__STATIC_INLINE uint32_t HRPWM_GetCountValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->CNTER);
}

/**
 * @brief  HRPWM set update value for counter register
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Update value
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetUpdateValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->UPDAR, u32Value);
}
/**
 * @brief  HRPWM get update value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Update value
 */
__STATIC_INLINE uint32_t HRPWM_GetUpdateValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->UPDAR);
}

/**
 * @brief  HRPWM set special compare register A value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Special compare register value, range [0, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetSpecialCompareAValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->SCMAR, u32Value);
}

/**
 * @brief  HRPWM set special compare register A value (buffer, special compare register C)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Special compare register value, range [0, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetSpecialCompareAValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->SCMCR, u32Value);
}

/**
 * @brief  HRPWM get special compare registers A value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Special compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetSpecialCompareAValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->SCMAR);
}

/**
 * @brief  HRPWM set special compare register B value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Special compare register value, range [0, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetSpecialCompareBValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->SCMBR, u32Value);
}

/**
 * @brief  HRPWM set special compare register B value (buffer, special compare register D)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Special compare register value, range [0, 0x3FFFFF]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetSpecialCompareBValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE(u32Value));
    WRITE_REG32(HRPWMx->SCMDR, u32Value);
}

/**
 * @brief  HRPWM get special compare registers B value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Special compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetSpecialCompareBValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->SCMBR);
}

/**
 * @brief  HRPWM get channel A capture value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Capture value
 */
__STATIC_INLINE uint32_t HRPWM_GetChACaptureValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32_BIT(HRPWMx->CAPAR, HRPWM_CAPAR_CAPA);
}

/**
 * @brief  HRPWM get channel A capture direction
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Count direction when capture event occurring @ref HRPWM_Capture_Dir_Define
 */
__STATIC_INLINE uint32_t HRPWM_GetChACaptureDir(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32_BIT(HRPWMx->CAPAR, HRPWM_CAPAR_CAPDIRA);
}

/**
 * @brief  HRPWM get channel B capture value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Capture value
 */
__STATIC_INLINE uint32_t HRPWM_GetChBCaptureValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32_BIT(HRPWMx->CAPBR, HRPWM_CAPBR_CAPB);
}

/**
 * @brief  HRPWM get channel B capture direction
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Count direction when capture event occurring @ref HRPWM_Capture_Dir_Define
 */
__STATIC_INLINE uint32_t HRPWM_GetChBCaptureDir(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32_BIT(HRPWMx->CAPBR, HRPWM_CAPBR_CAPDIRB);
}

/**
 * @brief  HRPWM set period value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Period value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetPeriodValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRPERAR, u32Value);
}

/**
 * @brief  HRPWM set period value (buffer, period register B)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Period value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetPeriodValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRPERBR, u32Value);
}

/**
 * @brief  HRPWM Get period value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Period value
 */
__STATIC_INLINE uint32_t HRPWM_GetPeriodValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRPERAR);
}

/**
 * @brief  HRPWM set general compare register A value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareAValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMAR, u32Value);
}

/**
 * @brief  HRPWM set general compare register A value(buffer, general compare register C)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareAValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMCR, u32Value);
}

/**
 * @brief  HRPWM get general compare registers A value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 General compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetCompareAValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRGCMAR);
}

/**
 * @brief  HRPWM set general compare register B value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareBValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMBR, u32Value);
}

/**
 * @brief  HRPWM set general compare register B value(buffer, general compare register D)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareBValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMDR, u32Value);
}

/**
 * @brief  HRPWM get general compare registers B value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 General compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetCompareBValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRGCMBR);
}

/**
 * @brief  HRPWM set general compare register E value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareEValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMER, u32Value);
}

/**
 * @brief  HRPWM set general compare register E value(buffer, general compare register G)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareEValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMGR, u32Value);
}

/**
 * @brief  HRPWM get general compare registers E value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 General compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetCompareEValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRGCMER);
}

/**
 * @brief  HRPWM set general compare register F value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareFValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMFR, u32Value);
}

/**
 * @brief  HRPWM set general compare register F value(buffer, general compare register H)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            General compare register value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareFValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRGCMHR, u32Value);
}

/**
 * @brief  HRPWM get general compare registers F value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 General compare register value
 */
__STATIC_INLINE uint32_t HRPWM_GetCompareFValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRGCMFR);
}

/**
 * @brief  HRPWM set dead time count up value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Dead time count up value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetDeadTimeUpValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRDTUAR, u32Value);
}

/**
 * @brief  HRPWM set dead time count up value (buffer, dead time count up register B)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Dead time count up value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetDeadTimeUpValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRDTUBR, u32Value);
}

/**
 * @brief  HRPWM get dead time count up value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Dead time count up value
 */
__STATIC_INLINE uint32_t HRPWM_GetDeadTimeUpValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRDTUAR);
}

/**
 * @brief  HRPWM set dead time count down value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Dead time count down value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetDeadTimeDownValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRDTDAR, u32Value);
}

/**
 * @brief  HRPWM set dead time count down value (buffer, dead time count down register B)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Value            Dead time count down value, range (0xC0, 0x3FFFC0]
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetDeadTimeDownValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DATA_REG_RANGE1(u32Value));
    WRITE_REG32(HRPWMx->HRDTDBR, u32Value);
}

/**
 * @brief  HRPWM get dead time count down value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval uint32_t                 Dead time count down value
 */
__STATIC_INLINE uint32_t HRPWM_GetDeadTimeDownValue(const CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    return READ_REG32(HRPWMx->HRDTDAR);
}

/**
 * @brief  HRPWM set phase match compare register
 * @param  [in] u32PhaseIndex       Phase match index @ref HRPWM_PH_Index_Define
 * @param  [in] u32Value            value range [0x00, 0x3FFFC0] for sawtooth waveform
 *                                  value range [0x00, 0x7FFFC0] for triangle waveform
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetCompareValue(uint32_t u32PhaseIndex, uint32_t u32Value)
{
    __IO uint32_t *HRPWM_PHSCMPmn;
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    DDL_ASSERT(IS_HRPWM_PH_DATA_REG_RANGE(u32Value));
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1A + 8UL * u32PhaseIndex);
    WRITE_REG32(*HRPWM_PHSCMPmn, u32Value);
}

/**
 * @brief  HRPWM set phase match compare register buffer
 * @param  [in] u32PhaseIndex       Phase match index @ref HRPWM_PH_Index_Define
 * @param  [in] u32Value            value range [0x00, 0x3FFFC0] for sawtooth waveform
 *                                  value range [0x00, 0x7FFFC0] for triangle waveform
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetCompareValue_Buf(uint32_t u32PhaseIndex, uint32_t u32Value)
{
    __IO uint32_t *HRPWM_PHSCMPmn;
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    DDL_ASSERT(IS_HRPWM_PH_DATA_REG_RANGE(u32Value));
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1B + 8UL * u32PhaseIndex);
    WRITE_REG32(*HRPWM_PHSCMPmn, u32Value);
}

/**
 * @brief  HRPWM get phase match compare register value
 * @param  [in] u32PhaseIndex       Phase match index @ref HRPWM_PH_Index_Define
 * @retval uint32_t                 Data for value of the register
 */
__STATIC_INLINE uint32_t HRPWM_PH_GetCompareValue(uint32_t u32PhaseIndex)
{
    __IO uint32_t *HRPWM_PHSCMPmn;
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1A + 8UL * u32PhaseIndex);
    return READ_REG32(*HRPWM_PHSCMPmn);
}

/******************************************************************************/
/* HRPWM Calibrate function */
int32_t HRPWM_CALIB_ProcessSingle(void);
void HRPWM_CALIB_PeriodInit(uint32_t u32Period);

void HRPWM_CALIB_SetPeriod(uint32_t u32Period);
void HRPWM_CALIB_SingleEnable(void);
void HRPWM_CALIB_SingleDisable(void);
void HRPWM_CALIB_PeriodEnable(void);
void HRPWM_CALIB_PeriodDisable(void);
void HRPWM_CALIB_EndIntEnable(void);
void HRPWM_CALIB_EndIntDisable(void);
en_flag_status_t HRPWM_CALIB_GetStatus(uint32_t u32Flag);
void HRPWM_CALIB_ClearFlag(uint32_t u32Flag);

/******************************************************************************/
/* HRPWM command */
void HRPWM_Enable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_Disable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Timer base count */
int32_t HRPWM_StructInit(stc_hrpwm_init_t *pstcHrpwmInit);
int32_t HRPWM_Init(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_init_t *pstcHrpwmInit);
void HRPWM_DeInit(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CountStart(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CountStop(CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetReload(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32CountReload);
void HRPWM_SetCountMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);

/******************************************************************************/
/* PWM initialize */
int32_t HRPWM_PWM_StructInit(stc_hrpwm_pwm_init_t *pstcPwmInit);
int32_t HRPWM_PWM_ChAInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit);
int32_t HRPWM_PWM_ChAInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit);
int32_t HRPWM_PWM_ChBInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit);
int32_t HRPWM_PWM_ChBInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_init_t *pstcPwmInit);
/* PWM output config */
int32_t HRPWM_PWM_OutputStructInit(stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit);
int32_t HRPWM_PWM_OutputInit(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit);
int32_t HRPWM_PWM_OutputInit_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_pwm_output_init_t *pstcPwmOutputInit);
void HRPWM_PWM_SetChSwapMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwapMode);
void HRPWM_PWM_SetChSwapMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwapMode);
void HRPWM_PWM_SetChSwap(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwap);
void HRPWM_PWM_SetChSwap_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChSwap);
void HRPWM_PWM_ChASetInvert(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert);
void HRPWM_PWM_ChASetInvert_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert);
void HRPWM_PWM_ChBSetInvert(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert);
void HRPWM_PWM_ChBSetInvert_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Invert);
/* PWM Polarity set for channel A */
void HRPWM_PWM_ChASetPolarityStart(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityStart_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityStop(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityStop_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityPeak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityPeak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityValley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityValley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
/* PWM Polarity set for channel B */
void HRPWM_PWM_ChBSetPolarityStart(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityStart_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityStop(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityStop_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityPeak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityPeak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityValley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityValley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchSpecialA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownMatchSpecialB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownExtEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownExtEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
/* PWM force polarity */
void HRPWM_PWM_ChASetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);

/******************************************************************************/
/* Capture */
void HRPWM_HWChACaptureCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChACaptureCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);

void HRPWM_HWChACaptureEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWChACaptureDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWChBCaptureEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWChBCaptureDisable(CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_CaptureSWTrigger(uint32_t u32Unit, uint32_t u32Ch);

/******************************************************************************/
/* Port output enable */
void HRPWM_PWM_ChAOutputEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChAOutputEnable_Buf(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChAOutputDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChAOutputDisable_Buf(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChBOutputEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChBOutputEnable_Buf(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChBOutputDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PWM_ChBOutputDisable_Buf(CM_HRPWM_TypeDef *HRPWMx);
/* Port input filter */
void HRPWM_TriggerPinSetFilterClock(uint32_t u32Pin, uint32_t u32Div);
void HRPWM_TriggerPinFilterEnable(uint32_t u32Pin);
void HRPWM_TriggerPinFilterDisable(uint32_t u32Pin);

/******************************************************************************/
/* Universal */
void HRPWM_CommonDeInit(void);
en_flag_status_t HRPWM_GetStatus(const CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Flag);
void HRPWM_ClearStatus(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Flag);

/******************************************************************************/
/* Interrupt and event */
void HRPWM_IntEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType);
void HRPWM_IntEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType);
void HRPWM_IntDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType);
void HRPWM_IntDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32IntType);

void HRPWM_MatchSpecialEventEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_MatchSpecialEventEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_MatchSpecialEventDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_MatchSpecialEventDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);

void HRPWM_MatchSpecialAEventSrcEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src);
void HRPWM_MatchSpecialAEventSrcDisbale(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src);

/******************************************************************************/
/* Dead time */
int32_t HRPWM_DeadTimeStructInit(stc_hrpwm_deadtime_config_t *pstcDeadTimeConfig);
int32_t HRPWM_DeadTimeConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_deadtime_config_t *pstcDeadTimeConfig);

void HRPWM_DeadTimeEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DeadTimeDisable(CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_DeadTimeSetEqualUpDown(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EqualUpDown);

int32_t HRPWM_DeadTimeBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_DeadTimeUpBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DeadTimeUpBufDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DeadTimeDownBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DeadTimeDownBufDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Buffer function */
int32_t HRPWM_BufStructInit(stc_hrpwm_buf_config_t *pstcBufConfig);

int32_t HRPWM_GeneralAEBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_GeneralAEBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_GeneralAEBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_GeneralBFBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_GeneralBFBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_GeneralBFBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_PeriodBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_PeriodBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PeriodBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_SpecialABufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_SpecialABufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_SpecialABufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_SpecialBBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_SpecialBBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_SpecialBBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_ControlRegBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_ControlRegBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_ControlRegBufDisable(CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_U1SingleTransBufConfig(uint32_t u32Config);
void HRPWM_U1SingleTransBufTrigger(void);

void HRPWM_UnitGlobalBufEnable(uint32_t u32Unit);
void HRPWM_UnitGlobalBufDisable(uint32_t u32Unit);

void HRPWM_UnitSWBufTrigger(uint32_t u32Unit);

void HRPWM_SetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src);
en_flag_status_t HRPWM_GetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src);
en_flag_status_t HRPWM_GetUnitGlobalBufStatus(uint32_t u32Unit);
void HRPWM_ClearUnitGlobalBufStatus(uint32_t u32Unit);

/******************************************************************************/
/* Idle output function */
void HRPWM_IDLE_SetCompletePeriod(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodPoint);
void HRPWM_IDLE_SetCompletePeriod_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodPoint);
void HRPWM_IDLE_SetChAIdleLevel(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Level);
void HRPWM_IDLE_SetChBIdleLevel(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Level);

void HRPWM_IDLE_EnterImmediate(uint32_t u32Unit, uint32_t u32Ch);
void HRPWM_IDLE_Exit(uint32_t u32Unit, uint32_t u32Ch);
void HRPWM_IDLE_ExitForCountStart(uint32_t u32Unit, uint32_t u32Ch);
en_flag_status_t HRPWM_IDLE_GetChStatus(uint32_t u32Unit, uint32_t u32Ch);

/* Idle delay function */
int32_t HRPWM_IDLE_DELAY_StructInit(stc_hrpwm_idle_delay_init_t *pstcIdleDelayInit);
int32_t HRPWM_IDLE_DELAY_Init(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_idle_delay_init_t *pstcIdleDelayInit);

void HRPWM_IDLE_DELAY_Enable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_DELAY_Disable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_DELAY_SetTriggerSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32TriggerSrc);
void HRPWM_IDLE_DELAY_SetOutputChAStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus);
void HRPWM_IDLE_DELAY_SetOutputChBStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus);

void HRPWM_IDLE_DELAY_IntEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_DELAY_IntDisable(CM_HRPWM_TypeDef *HRPWMx);
en_flag_status_t HRPWM_IDLE_DELAY_GetStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Flag);
void HRPWM_IDLE_DELAY_ClearStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Flag);
void HRPWM_IDLE_DELAY_SWTrigger(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_DELAY_MultiUnitSWTrigger(uint32_t u32Unit);

/* Idle BM function */
int32_t HRPWM_IDLE_BM_StructInit(stc_hrpwm_idle_bm_init_t *pstcBMInit);
int32_t HRPWM_IDLE_BM_Init(stc_hrpwm_idle_bm_init_t *pstcBMInit);

void HRPWM_IDLE_BM_Enable(void);
void HRPWM_IDLE_BM_Disable(void);
void HRPWM_IDLE_BM_SetActionMode(uint32_t u32Mode);
void HRPWM_IDLE_BM_SetCountSrc(uint32_t u32CountSrc);
void HRPWM_IDLE_BM_SetPclkDiv(uint32_t u32PclkDiv);
void HRPWM_IDLE_BM_SetCountReload(uint32_t u32CountReload);
void HRPWM_IDLE_BM_SetTriggerSrc(uint64_t u64TriggerSrc);

int32_t HRPWM_IDLE_BM_OutputStructInit(stc_hrpwm_idle_bm_output_init_t *pstcBMOutputInit);
int32_t HRPWM_IDLE_BM_OutputInit(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_idle_bm_output_init_t *pstcBMOutputInit);

void HRPWM_IDLE_BM_SetOutputChAStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus);
void HRPWM_IDLE_BM_SetOutputChBStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ChStatus);
void HRPWM_IDLE_BM_SetChAEnterDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EnterDelay);
void HRPWM_IDLE_BM_SetChBEnterDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EnterDelay);
void HRPWM_IDLE_BM_SetChFollow(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Follow);
void HRPWM_IDLE_BM_SetUnitCountReset(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32UnitCountReset);

void HRPWM_IDLE_BM_SetPeriodReg(uint32_t u32Value);
void HRPWM_IDLE_BM_SetPeriodReg_Buf(uint32_t u32Value);
void HRPWM_IDLE_BM_SetCompareReg(uint32_t u32Value);
void HRPWM_IDLE_BM_SetCompareReg_Buf(uint32_t u32Value);

void HRPWM_IDLE_BM_PeakIntEnable(void);
void HRPWM_IDLE_BM_PeakIntDisable(void);

en_flag_status_t HRPWM_IDLE_BM_GetStatus(uint32_t u32Flag);
void HRPWM_IDLE_BM_ClearStatus(uint32_t u32Flag);

void HRPWM_IDLE_BM_SWTrigger(void);
void HRPWM_IDLE_BM_Exit(void);

void HRPWM_IDLE_BM_PeriodBufEnable(void);
void HRPWM_IDLE_BM_PeriodBufDisable(void);
void HRPWM_IDLE_BM_CompareBufEnable(void);
void HRPWM_IDLE_BM_CompareBufDisable(void);

/******************************************************************************/
/* Valid Period */
int32_t HRPWM_ValidPeriodStructInit(stc_hrpwm_valid_period_config_t *pstcValidperiodConfig);
int32_t HRPWM_ValidPeriodConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_valid_period_config_t *pstcValidperiodConfig);

void HRPWM_SetValidPeriodSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialA);
void HRPWM_SetValidPeriodSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialB);
void HRPWM_SetValidPeriodInterval(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Interval);
void HRPWM_SetValidPeriodCountCond(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32CountCond);

uint32_t HRPWM_GetValidPeriodPeriodNum(const CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Hardware control */
void HRPWM_HWStartCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWStartCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);

void HRPWM_HWStartEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWStartDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWClearEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HWClearDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Software synchronous control */
void HRPWM_SWSyncStart(uint32_t u32Unit);
void HRPWM_SWSyncStop(uint32_t u32Unit);
void HRPWM_SWSyncClear(uint32_t u32Unit);
void HRPWM_SWSyncUpdate(uint32_t u32Unit);

/******************************************************************************/
/* External event configuration */
int32_t HRPWM_EVT_StructInit(stc_hrpwm_evt_config_t *pstcEventConfig);
int32_t HRPWM_EVT_Config(uint32_t u32EventNum, const stc_hrpwm_evt_config_t *pstcEventConfig);

void HRPWM_EVT_SetSrc(uint32_t u32EventNum, uint32_t u32EventSrc);
void HRPWM_EVT_SetSrc2(uint32_t u32EventNum, uint32_t u32EventSrc2);
void HRPWM_EVT_SetValidAction(uint32_t u32EventNum, uint32_t u32ValidAction);
void HRPWM_EVT_SetValidLevel(uint32_t u32EventNum, uint32_t u32ValidLevel);
void HRPWM_EVT_SetFastAsyncMode(uint32_t u32EventNum, uint32_t u32FastAsync);
void HRPWM_EVT_SetFilterClock(uint32_t u32EventNum, uint32_t u32Clock);
void HRPWM_EVT_SetGenerateEvent(uint32_t u32EventNum, uint32_t u32GenerateEvent);
void HRPWM_EVT_SetEEVSClock(uint32_t u32Clock);

/* External event filter function */
int32_t HRPWM_EVT_FilterStructInit(stc_hrpwm_evt_filter_config_t *pstcFilterConfig);
int32_t HRPWM_EVT_FilterConfig(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, const stc_hrpwm_evt_filter_config_t *pstcFilterConfig);

void HRPWM_EVT_FilterSetReplaceMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_EVT_FilterSetReplaceMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_EVT_FilterSetMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Mode);
void HRPWM_EVT_FilterSetLatch(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Latch);
void HRPWM_EVT_FilterSetTimeout(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum, uint32_t u32Timeout);

/* Filter signal configuration */
int32_t HRPWM_EVT_FilterSignalStructInit(stc_hrpwm_evt_filter_signal_config_t *pstcSignalConfig);
int32_t HRPWM_EVT_FilterSignalConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_evt_filter_signal_config_t *pstcSignalConfig);

void HRPWM_EVT_FilterSetOffsetDir(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32OffsetDir);
void HRPWM_EVT_FilterSetOffsetDir_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32OffsetDir);
void HRPWM_EVT_FilterSetOffset(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Offset);
void HRPWM_EVT_FilterSetOffset_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Offset);
void HRPWM_EVT_FilterSetWindowDir(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32WindowDir);
void HRPWM_EVT_FilterSetWindowDir_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32WindowDir);
void HRPWM_EVT_FilterSetWindow(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Window);
void HRPWM_EVT_FilterSetWindow_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Window);
void HRPWM_EVT_FilterSetInitPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32InitPolarity);

void HRPWM_EVT_FilterBlankDelayEnable(uint32_t u32Unit);
void HRPWM_EVT_FilterBlankDelayDisable(uint32_t u32Unit);

int32_t HRPWM_EVT_WindowBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_EVT_WindowBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_EVT_WindowBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_EVT_OffsetBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_EVT_OffsetBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_EVT_OffsetBufDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* HRPWM synchronous output */
int32_t HRPWM_SYNC_StructInit(stc_hrpwm_sync_output_config_t *pstcSyncConfig);
int32_t HRPWM_SYNC_Config(stc_hrpwm_sync_output_config_t *pstcSyncConfig);

void HRPWM_SYNC_SetSrc(uint32_t u32Src);
void HRPWM_SYNC_SetMatchBDir(uint32_t u32MatchBDir);
void HRPWM_SYNC_SetPulse(uint32_t u32Pulse);
void HRPWM_SYNC_SetPulseWidth(uint32_t u32PulseWidth);

/******************************************************************************/
/* DAC trigger function */
int32_t HRPWM_DAC_TriggerStructInit(stc_hrpwm_dac_trigger_config_t *pstcDacTriggerConfig);
int32_t HRPWM_DAC_TriggerConfig(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_dac_trigger_config_t *pstcDacTriggerConfig);

void HRPWM_DAC_SetTriggerSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src);
void HRPWM_DAC_SetCh1Dest(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32DacCh1Dest);
void HRPWM_DAC_SetCh2Dest(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32DacCh2Dest);

/******************************************************************************/
/* Phase */
int32_t HRPWM_PH_StructInit(stc_hrpwm_ph_config_t *pstcPHConfig);
int32_t HRPWM_PH_Config(CM_HRPWM_TypeDef *HRPWMx, stc_hrpwm_ph_config_t *pstcPHConfig);

void HRPWM_PH_Enable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PH_Disable(CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_PH_SetPhaseIndex(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PhaseIndex);
void HRPWM_PH_SetForceChA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ForceChA);
void HRPWM_PH_SetForceChB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ForceChB);
void HRPWM_PH_SetPeriodLink(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodLink);
void HRPWM_PH_SetPeriodLink_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PeriodLink);

en_flag_status_t HRPWM_PH_GetStatus(uint32_t u32Flag);
void HRPWM_PH_ClearStatus(uint32_t u32Flag);

void HRPWM_PH_SetBufCond(uint32_t u32BufTransCond);
void HRPWM_PH_BufEnable(void);
void HRPWM_PH_BufDisable(void);

/******************************************************************************/
/* EMB */
int32_t HRPWM_EMB_StructInit(stc_hrpwm_emb_config_t *pstcEmbConfig);
int32_t HRPWM_EMB_ChAConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig);
int32_t HRPWM_EMB_ChAConfig_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig);
int32_t HRPWM_EMB_ChBConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig);
int32_t HRPWM_EMB_ChBConfig_Buf(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_emb_config_t *pstcEmbConfig);

void HRPWM_EMB_ChASetValidCh(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh);
void HRPWM_EMB_ChASetValidCh_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh);
void HRPWM_EMB_ChBSetValidCh(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh);
void HRPWM_EMB_ChBSetValidCh_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ValidCh);
void HRPWM_EMB_ChASetReleaseMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode);
void HRPWM_EMB_ChASetReleaseMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode);
void HRPWM_EMB_ChBSetReleaseMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode);
void HRPWM_EMB_ChBSetReleaseMode_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32ReleaseMode);
void HRPWM_EMB_ChASetPinStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus);
void HRPWM_EMB_ChASetPinStatus_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus);
void HRPWM_EMB_ChBSetPinStatus(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus);
void HRPWM_EMB_ChBSetPinStatus_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32PinStatus);

/******************************************************************************/
/* Half wave mode */
void HRPWM_HalfWaveModeEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HalfWaveModeEnable_Buf(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HalfWaveModeDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_HalfWaveModeDisable_Buf(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Data register write and read */
void HRPWM_SetCountValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetCountValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetUpdateValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetUpdateValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetSpecialCompareAValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetSpecialCompareAValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetSpecialCompareAValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetSpecialCompareBValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetSpecialCompareBValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetSpecialCompareBValue(const CM_HRPWM_TypeDef *HRPWMx);

uint32_t HRPWM_GetChACaptureValue(const CM_HRPWM_TypeDef *HRPWMx);
uint32_t HRPWM_GetChACaptureDir(const CM_HRPWM_TypeDef *HRPWMx);

uint32_t HRPWM_GetChBCaptureValue(const CM_HRPWM_TypeDef *HRPWMx);
uint32_t HRPWM_GetChBCaptureDir(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetPeriodValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetPeriodValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetPeriodValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetCompareAValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetCompareAValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetCompareAValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetCompareBValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetCompareBValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetCompareBValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetCompareEValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetCompareEValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetCompareEValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetCompareFValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetCompareFValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetCompareFValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetDeadTimeUpValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetDeadTimeUpValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetDeadTimeUpValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_SetDeadTimeDownValue(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
void HRPWM_SetDeadTimeDownValue_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Value);
uint32_t HRPWM_GetDeadTimeDownValue(const CM_HRPWM_TypeDef *HRPWMx);

void HRPWM_PH_SetCompareValue(uint32_t u32PhaseIndex, uint32_t u32Value);
void HRPWM_PH_SetCompareValue_Buf(uint32_t u32PhaseIndex, uint32_t u32Value);
uint32_t HRPWM_PH_GetCompareValue(uint32_t u32PhaseIndex);

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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_HRPWM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
