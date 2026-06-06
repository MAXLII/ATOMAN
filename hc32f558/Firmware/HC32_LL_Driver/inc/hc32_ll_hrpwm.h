/**
 *******************************************************************************
 * @file  hc32_ll_hrpwm.h
 * @brief This file contains all the functions prototypes of the HRPWM driver
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

#include "hc32f5xx.h"
#include "hc32f5xx_conf.h"

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
    uint32_t u32PeriodValue;        /*!< The period reference value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX] */
    uint32_t u32CountReload;        /*!< Count reload after count peak @ref HRPWM_Count_Reload_Define */
    uint32_t u32CountDiv;           /*!< Count division, @ref HRPWM_Count_Clock_Divide */
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
                                                         Valid for HRPWM2 ~ HRPWM6 when HRPWM1 buffer single transfer function is enabled */
    en_functional_state_t enGlobalBufTrans;         /*!< Buffer transfer when global buffer transfer occurs
                                                         this parameter can be a value of @ref en_functional_state_t */
} stc_hrpwm_buf_config_t;

/**
 * @brief HRPWM pwm function structure definition
 */
typedef struct {
    uint32_t u32CompareValue;               /*!< Range (HRPWM_REG_VALUE_MIN ~ HRPWM_REG_VALUE_MAX] */
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
    uint32_t u32ValidAction;        /*!< External event valid action @ref HRPWM_EVT_Valid_Action_Define */
    uint32_t u32ValidLevel;         /*!< External event valid level @ref HRPWM_EVT_Valid_Level_Define */
    uint32_t u32FastAsyncMode;      /*!< External event fast asynchronous mode @ref HRPWM_EVT_Fast_Async_Define
                                         Only valid for event 1 ~ event 5 */
    uint32_t u32FilterClock;        /*!< External event filter clock @ref HRPWM_EVT_Filter_Clock_Define
                                         Only valid for event 6 ~ event 10 */
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
 * @brief HRPWM chopping configuration structure definition
 */
typedef struct {
    uint32_t u32CarrierCycle;       /*!< HRPWM carrier cycle @ref HRPWM_Chopping_Carrier_Cycle_Define */
    uint32_t u32DutyCycle;          /*!< HRPWM chopping duty cycle, @ref HRPWM_Chopping_Duty_Cycle_Define */
    uint32_t u32PulseWidth;         /*!< HRPWM first carrier signal pulse width @ref HRPWM_Chopping_Pulse_Width_Define */
} stc_hrpwm_chp_config_t;

/**
 * @brief  HRPWM EMB level & filter mode structure definition
 */
typedef struct {
    union {
        uint32_t CTL2;     /*!< control register 2  */
        struct {
            uint32_t u32Pwm1Level   : 1;    /*!< HRPWM1 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm2Level   : 1;    /*!< HRPWM2 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm3Level   : 1;    /*!< HRPWM3 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm4Level   : 1;    /*!< HRPWM4 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm5Level   : 1;    /*!< HRPWM5 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm6Level   : 1;    /*!< HRPWM6 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm7Level   : 1;    /*!< HRPWM7 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32Pwm8Level   : 1;    /*!< HRPWM8 level, @ref HRPWM_EMB_level_Define          */
            uint32_t u32resvd0      : 8;    /*!< reserved                                           */
            uint32_t u32EmbIn1Level : 1;    /*!< EMB input1 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn2Level : 1;    /*!< EMB input2 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn3Level : 1;    /*!< EMB input3 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn4Level : 1;    /*!< EMB input4 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn5Level : 1;    /*!< EMB input5 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn6Level : 1;    /*!< EMB input6 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn7Level : 1;    /*!< EMB input7 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32EmbIn8Level : 1;    /*!< EMB input8 level, @ref HRPWM_EMB_level_Define      */
            uint32_t u32resvd1      : 7;    /*!< reserved                                           */
            uint32_t u32FilterMode  : 1;    /*!< Noise filter count, @ref HRPWM_EMB_NF_Count_Define */
        } CTL2_f;
    };
} stc_hrpwm_emb_level_filter_t;

/**
 * @brief  HRPWM EMB noise filter structure definition
 */
typedef struct {
    union {
        uint32_t CTL3;
        struct {
            uint32_t u32EmbIn1FilterClock   : 3;            /*!< HRPWM EMB input1 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn1FilterState   : 1;            /*!< HRPWM EMB input1 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn2FilterClock   : 3;            /*!< HRPWM EMB input2 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn2FilterState   : 1;            /*!< HRPWM EMB input2 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn3FilterClock   : 3;            /*!< HRPWM EMB input3 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn3FilterState   : 1;            /*!< HRPWM EMB input3 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn4FilterClock   : 3;            /*!< HRPWM EMB input4 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn4FilterState   : 1;            /*!< HRPWM EMB input4 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn5FilterClock   : 3;            /*!< HRPWM EMB input5 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn5FilterState   : 1;            /*!< HRPWM EMB input5 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn6FilterClock   : 3;            /*!< HRPWM EMB input6 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn6FilterState   : 1;            /*!< HRPWM EMB input6 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn7FilterClock   : 3;            /*!< HRPWM EMB input7 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn7FilterState   : 1;            /*!< HRPWM EMB input7 filter state, @ref HRPWM_EMB_NF_Func_State     */
            uint32_t u32EmbIn8FilterClock   : 3;            /*!< HRPWM EMB input8 filter clock, @ref HRPWM_EMB_NF_Clock_Division */
            uint32_t u32EmbIn8FilterState   : 1;            /*!< HRPWM EMB input8 filter state, @ref HRPWM_EMB_NF_Func_State     */
        } CTL3_f;
    };
} stc_hrpwm_emb_nf_config_t;

/**
 * @brief  HRPWM EMB blank & accumulator edge config
 */
typedef struct {
    union {
        uint32_t CTL4;     /*!< control register 4  */
        struct {
            uint32_t u32resvd0        : 28;   /*!< reserved                                                     */
            uint32_t u32EmbBlankState : 1;    /*!< EMB blank state, @ref en_functional_state_t                  */
            uint32_t u32EmbBlankTime  : 1;    /*!< EMB blank time, @ref HRPWM_EMB_Blank_Time_Define             */
            uint32_t u32EmbAccumEdge  : 1;    /*!< EMB accumulator edge, @ref HRPWM_EMB_Accumulator_Edge_Define */
            uint32_t u32resvd1        : 1;    /*!< reserved                                                     */
        } CTL4_f;
    };
} stc_hrpwm_emb_blank_Accum_t;

/**
 * @brief  HRPWM EMB blank & accumulator edge config
 */
typedef struct {
    uint32_t u32AccumEvent;             /*!< EMB accumulator event, @ref HRPWM_EMB_Accumulator_Event_Selection */
    uint32_t u32AccumResetMode;         /*!< EMB accumulator reset mode, @ref HRPWM_EMB_Accumulator_Reset_Mode */
    uint32_t u32AccumThreshold;         /*!< EMB accumulator threshold, range at 0~15 */
} stc_hrpwm_emb_Accum_config_t;

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

#define HRPWM_REG_VALUE_MAX                 (0x0001FFE0UL)
#define HRPWM_REG_VALUE_MIN                 (0x00000001UL)

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
#define HRPWM_UNIT7                         (0x01UL << 6U)
#define HRPWM_UNIT8                         (0x01UL << 7U)
#define HRPWM_UNIT_ALL                      (0xFFUL)
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
#define HRPWM_SW_SYNC_UNIT7                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP7A_POS)
#define HRPWM_SW_SYNC_UNIT8                 (0x03UL << HRPWM_COMMON_SCAPR_SCAP8A_POS)
#define HRPWM_SW_SYNC_UNIT_ALL              (0xFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Sw_Sync_Ch_Define HRPWM Channel Define For Software Synchronous function
 * @{
 */
#define HRPWM_SW_SYNC_CHA                   (0x00005555UL)
#define HRPWM_SW_SYNC_CHB                   (0x0000AAAAUL)
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
 * @defgroup HRPWM_Count_Clock_Divide HRPWM Count Clock Divider Definition
 * @note Only CR.EN = 1, HRPWM_CNT_CLK_DIV1/2/4/8/16 is valid
 *       if CR.EN = 0, set the HRPWM_CNT_CLK_DIV1/2/4/8/16 is valid, the frequency is equal to HRPWM_CNT_CLK_DIV32
 * @{
 */
#define HRPWM_CNT_CLK_DIV1                     (0x00UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 32MHz = 7.68GHz, and the resolution is 130ps*/
#define HRPWM_CNT_CLK_DIV2                     (0x01UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 16MHz = 3.84GHz, and the resolution is 260ps */
#define HRPWM_CNT_CLK_DIV4                     (0x02UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 8MHz = 1.92GHz, and the resolution is 520ps */
#define HRPWM_CNT_CLK_DIV8                     (0x03UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 4MHz = 960MHz, and the resolution is 1.04ns */
#define HRPWM_CNT_CLK_DIV16                    (0x04UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 2MHz = 480MHz, and the resolution is 2.08ns */
#define HRPWM_CNT_CLK_DIV32                    (0x05UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 1 = 240MHz, and the resolution is 4.16ns */
#define HRPWM_CNT_CLK_DIV64                    (0x06UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 1/2 = 120MHz, and the resolution is 8.32ns */
#define HRPWM_CNT_CLK_DIV128                   (0x07UL << HRPWM_GCONR_CKDIV_POS)    /*!< The equivalent frequency is 240 * 1/4 = 60MHz, and the resolution is 16.64ns */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Trigger_Calculate_Mode HRPWM Trigger Calculate Mode Definition
 * @{
 */
#define HRPWM_TRIG_CALC_MD_ADD                  (HRPWM_GCONR_TRGCALASEN)                                                /*!< Add the value of reg:TRGCALDATA while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_SUB                  (HRPWM_GCONR_TRGCALASEN | HRPWM_GCONR_TRGCALASMD)                                   /*!< Subtract the value of reg:TRGCALDATA while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_DIV2                 (HRPWM_GCONR_TRGCALMDEN)                                                /*!< The value of reg:SCMB is capture value / 2 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_DIV4                 (HRPWM_GCONR_TRGCALMDEN | (0x01UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value / 4 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_DIV8                 (HRPWM_GCONR_TRGCALMDEN | (0x02UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value / 8 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_DIV16                (HRPWM_GCONR_TRGCALMDEN | (0x03UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value / 16 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_MUL2                 (HRPWM_GCONR_TRGCALMDEN | (0x04UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value * 2 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_MUL4                 (HRPWM_GCONR_TRGCALMDEN | (0x05UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value * 4 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_MUL8                 (HRPWM_GCONR_TRGCALMDEN | (0x06UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value * 8 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_MUL16                (HRPWM_GCONR_TRGCALMDEN | (0x07UL << HRPWM_GCONR_TRGCALMDMD_POS))       /*!< The value of reg:SCMB is capture value * 16 while capture event A occurs */
#define HRPWM_TRIG_CALC_MD_MASK                 (HRPWM_GCONR_TRGCALASEN | HRPWM_GCONR_TRGCALASMD | \
                                                 HRPWM_GCONR_TRGCALMDEN | HRPWM_GCONR_TRGCALMDMD)                       /*!< Trigger calculate mode mask */
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Start_Condition_Mux_Sel HRPWM harware start condition multiplexer selection
 * @{
 */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT3     (0x00UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event3 to trigger hardware start */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT4     (0x01UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event4 to trigger hardware start */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT5     (0x02UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event5 to trigger hardware start */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT6     (0x03UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event6 to trigger hardware start */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT7     (0x04UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event7 to trigger hardware start */
#define HRPWM_HW_START_COND_MUX_INTERN_EVT8     (0x05UL << HRPWM_GCONR_STAAOSMUX_POS)   /*!< Select the internal event8 to trigger hardware start */
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Clear_Condition_Mux_Sel HRPWM harware clear condition multiplexer selection
 * @{
 */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT3       (0x00UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event3 to trigger hardware clear */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT4       (0x01UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event4 to trigger hardware clear */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT5       (0x02UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event5 to trigger hardware clear */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT6       (0x03UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event6 to trigger hardware clear */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT7       (0x04UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event7 to trigger hardware clear */
#define HRPWM_HW_CLR_COND_MUX_INTERN_EVT8       (0x05UL << HRPWM_GCONR_CLRAOSMUX_POS)   /*!< Select the internal event8 to trigger hardware clear */
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_CaptureA_Condition_Mux_Sel HRPWM harware captureA condition multiplexer selection
 * @{
 */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT3    (0x00UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event3 to trigger hardware capture A */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT4    (0x01UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event4 to trigger hardware capture A */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT5    (0x02UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event5 to trigger hardware capture A */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT6    (0x03UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event6 to trigger hardware capture A */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT7    (0x04UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event7 to trigger hardware capture A */
#define HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT8    (0x05UL << HRPWM_GCONR_CAPAAOSMUX_POS)  /*!< Slect the internal event8 to trigger hardware capture A */
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_CaptureB_Condition_Mux_Sel HRPWM harware CaptureB Condition Mux Selection Definition
 * @{
 */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT3    (0x00UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event3 to trigger hardware capture B */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT4    (0x01UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event4 to trigger hardware capture B */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT5    (0x02UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event5 to trigger hardware capture B */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT6    (0x03UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event6 to trigger hardware capture B */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT7    (0x04UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event7 to trigger hardware capture B */
#define HRPWM_HW_CAPT_B_COND_MUX_INTERN_EVT8    (0x05UL << HRPWM_GCONR_CAPBAOSMUX_POS)  /*!< Slect the internal event8 to trigger hardware capture B */
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
#define HRPWM_INT_PERIOD_INTERVAL_MATCH     (HRPWM_ICONR_INTENVPE)   /*!< Match period interval counter when it equals 0 */
#define HRPWM_INT_ALL                       (HRPWM_INT_MATCH_A  | HRPWM_INT_MATCH_B    | HRPWM_INT_MATCH_C | \
                                             HRPWM_INT_MATCH_D  | HRPWM_INT_MATCH_E    | HRPWM_INT_MATCH_F |  \
                                             HRPWM_INT_CNT_PEAK | HRPWM_INT_CNT_VALLEY | HRPWM_INT_CAPT_A  | \
                                             HRPWM_INT_CAPT_B   | HRPWM_INT_UP_MATCH_SPECIAL_A             | \
                                             HRPWM_INT_DOWN_MATCH_SPECIAL_A | HRPWM_INT_UP_MATCH_SPECIAL_B | \
                                             HRPWM_INT_DOWN_MATCH_SPECIAL_B | HRPWM_INT_PERIOD_INTERVAL_MATCH)
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
 * @defgroup HRPWM_Pin_Polarity_Up_Sw_Trigger_Define HRPWM Pin Polarity For Count Up And software trigger Define
 * @{
 */
#define HRPWM_PWM_UP_SW_TRIG_LOW        (0x00UL << HRPWM_PCNAR2_SFTUCA_POS)
#define HRPWM_PWM_UP_SW_TRIG_HIGH       (0x01UL << HRPWM_PCNAR2_SFTUCA_POS)
#define HRPWM_PWM_UP_SW_TRIG_HOLD       (0x02UL << HRPWM_PCNAR2_SFTUCA_POS)
#define HRPWM_PWM_UP_SW_TRIG_INVT       (0x03UL << HRPWM_PCNAR2_SFTUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_Sw_Trigger_Define HRPWM Pin Polarity For Count Down And software trigger Define
 * @{
 */
#define HRPWM_PWM_DOWN_SW_TRIG_LOW      (0x00UL << HRPWM_PCNAR3_SFTDCA_POS)
#define HRPWM_PWM_DOWN_SW_TRIG_HIGH     (0x01UL << HRPWM_PCNAR3_SFTDCA_POS)
#define HRPWM_PWM_DOWN_SW_TRIG_HOLD     (0x02UL << HRPWM_PCNAR3_SFTDCA_POS)
#define HRPWM_PWM_DOWN_SW_TRIG_INVT     (0x03UL << HRPWM_PCNAR3_SFTDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_U1_Pin_Polarity_Timer_Event_Num_Define HRPWM Timer Event Number Define for Pin Polarity set
 * @{
 */
#define HRPWM_PWM_U1_TMR_EVT_NUM1       (HRPWM_PCNAR14_TMEVT1UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM2       (HRPWM_PCNAR14_TMEVT2UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM3       (HRPWM_PCNAR14_TMEVT3UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM4       (HRPWM_PCNAR14_TMEVT4UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM5       (HRPWM_PCNAR14_TMEVT5UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM6       (HRPWM_PCNAR14_TMEVT6UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM7       (HRPWM_PCNAR14_TMEVT7UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM8       (HRPWM_PCNAR14_TMEVT8UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM9       (HRPWM_PCNAR14_TMEVT9UCA)
#define HRPWM_PWM_U1_TMR_EVT_NUM10      (HRPWM_PCNAR14_TMEVT10UCA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define HRPWM Timer Event Number Define for Pin Polarity set
 * @{
 */
#define HRPWM_PWM_U2_8_TMR_EVT_NUM1     (HRPWM_PCNAR24_TMEVT1UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM2     (HRPWM_PCNAR24_TMEVT2UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM3     (HRPWM_PCNAR24_TMEVT3UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM4     (HRPWM_PCNAR24_TMEVT4UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM5     (HRPWM_PCNAR24_TMEVT5UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM6     (HRPWM_PCNAR24_TMEVT6UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM7     (HRPWM_PCNAR24_TMEVT7UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM8     (HRPWM_PCNAR24_TMEVT8UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM9     (HRPWM_PCNAR24_TMEVT9UCA)
#define HRPWM_PWM_U2_8_TMR_EVT_NUM10    (HRPWM_PCNAR24_TMEVT10UCA)
/**
 * @}
 */

/**
 * @defgroup HRPWM_U1_Pin_Polarity_Timer_Event_Define HRPWM Pin Polarity for Timer Event Define
 * @{
 */
#define HRPWM_PWM_U1_TMR_EVT_LOW        (0x00000000UL)
#define HRPWM_PWM_U1_TMR_EVT_HIGH       (0x00055555UL)
#define HRPWM_PWM_U1_TMR_EVT_HOLD       (0x000AAAAAUL)
#define HRPWM_PWM_U1_TMR_EVT_INVT       (0x000FFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_U2_8_Pin_Polarity_Timer_Event_Define HRPWM Pin Polarity for Timer Event Define
 * @{
 */
#define HRPWM_PWM_U2_8_TMR_EVT_LOW      (0x00000000UL)
#define HRPWM_PWM_U2_8_TMR_EVT_HIGH     (0x55555000UL)
#define HRPWM_PWM_U2_8_TMR_EVT_HOLD     (0xAAAAA000UL)
#define HRPWM_PWM_U2_8_TMR_EVT_INVT     (0xFFFFF000UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Peak_Define HRPWM Pin Polarity For Count Up And U1 Peak Define
 * @{
 */
#define HRPWM_PWM_UP_U1_PEAK_LOW            (0x00UL << HRPWM_PCNAR24_U1OVFUCA_POS)
#define HRPWM_PWM_UP_U1_PEAK_HIGH           (0x01UL << HRPWM_PCNAR24_U1OVFUCA_POS)
#define HRPWM_PWM_UP_U1_PEAK_HOLD           (0x02UL << HRPWM_PCNAR24_U1OVFUCA_POS)
#define HRPWM_PWM_UP_U1_PEAK_INVT           (0x03UL << HRPWM_PCNAR24_U1OVFUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Valley_Define HRPWM Pin Polarity For Count Up And U1 Valley Define
 * @{
 */
#define HRPWM_PWM_UP_U1_VALLEY_LOW          (0x00UL << HRPWM_PCNAR24_U1UDFUCA_POS)
#define HRPWM_PWM_UP_U1_VALLEY_HIGH         (0x01UL << HRPWM_PCNAR24_U1UDFUCA_POS)
#define HRPWM_PWM_UP_U1_VALLEY_HOLD         (0x02UL << HRPWM_PCNAR24_U1UDFUCA_POS)
#define HRPWM_PWM_UP_U1_VALLEY_INVT         (0x03UL << HRPWM_PCNAR24_U1UDFUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Match_F_Define HRPWM Pin Polarity For Count Up And U1 Match F Define
 * @{
 */
#define HRPWM_PWM_UP_U1_MATCH_F_LOW         (0x00UL << HRPWM_PCNAR24_U1CMFUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_F_HIGH        (0x01UL << HRPWM_PCNAR24_U1CMFUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_F_HOLD        (0x02UL << HRPWM_PCNAR24_U1CMFUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_F_INVT        (0x03UL << HRPWM_PCNAR24_U1CMFUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Match_E_Define HRPWM Pin Polarity For Count Up And U1 Match E Define
 * @{
 */
#define HRPWM_PWM_UP_U1_MATCH_E_LOW         (0x00UL << HRPWM_PCNAR24_U1CMEUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_E_HIGH        (0x01UL << HRPWM_PCNAR24_U1CMEUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_E_HOLD        (0x02UL << HRPWM_PCNAR24_U1CMEUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_E_INVT        (0x03UL << HRPWM_PCNAR24_U1CMEUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Match_B_Define HRPWM Pin Polarity For Count Up And U1 Match B Define
 * @{
 */
#define HRPWM_PWM_UP_U1_MATCH_B_LOW         (0x00UL << HRPWM_PCNAR24_U1CMBUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_B_HIGH        (0x01UL << HRPWM_PCNAR24_U1CMBUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_B_HOLD        (0x02UL << HRPWM_PCNAR24_U1CMBUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_B_INVT        (0x03UL << HRPWM_PCNAR24_U1CMBUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Up_U1_Match_A_Define HRPWM Pin Polarity For Count Up And U1 Match A Define
 * @{
 */
#define HRPWM_PWM_UP_U1_MATCH_A_LOW         (0x00UL << HRPWM_PCNAR24_U1CMAUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_A_HIGH        (0x01UL << HRPWM_PCNAR24_U1CMAUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_A_HOLD        (0x02UL << HRPWM_PCNAR24_U1CMAUCA_POS)
#define HRPWM_PWM_UP_U1_MATCH_A_INVT        (0x03UL << HRPWM_PCNAR24_U1CMAUCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Peak_Define HRPWM Pin Polarity For Count Down And U1 Peak Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_PEAK_LOW          (0x00UL << HRPWM_PCNAR25_U1OVFDCA_POS)
#define HRPWM_PWM_DOWN_U1_PEAK_HIGH         (0x01UL << HRPWM_PCNAR25_U1OVFDCA_POS)
#define HRPWM_PWM_DOWN_U1_PEAK_HOLD         (0x02UL << HRPWM_PCNAR25_U1OVFDCA_POS)
#define HRPWM_PWM_DOWN_U1_PEAK_INVT         (0x03UL << HRPWM_PCNAR25_U1OVFDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Valley_Define HRPWM Pin Polarity For Count Down And U1 Valley Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_VALLEY_LOW        (0x00UL << HRPWM_PCNAR25_U1UDFDCA_POS)
#define HRPWM_PWM_DOWN_U1_VALLEY_HIGH       (0x01UL << HRPWM_PCNAR25_U1UDFDCA_POS)
#define HRPWM_PWM_DOWN_U1_VALLEY_HOLD       (0x02UL << HRPWM_PCNAR25_U1UDFDCA_POS)
#define HRPWM_PWM_DOWN_U1_VALLEY_INVT       (0x03UL << HRPWM_PCNAR25_U1UDFDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Match_F_Define HRPWM Pin Polarity For Count Down And U1 Match F Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_MATCH_F_LOW       (0x00UL << HRPWM_PCNAR25_U1CMFDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_F_HIGH      (0x01UL << HRPWM_PCNAR25_U1CMFDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_F_HOLD      (0x02UL << HRPWM_PCNAR25_U1CMFDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_F_INVT      (0x03UL << HRPWM_PCNAR25_U1CMFDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Match_E_Define HRPWM Pin Polarity For Count Down And U1 Match E Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_MATCH_E_LOW       (0x00UL << HRPWM_PCNAR25_U1CMEDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_E_HIGH      (0x01UL << HRPWM_PCNAR25_U1CMEDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_E_HOLD      (0x02UL << HRPWM_PCNAR25_U1CMEDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_E_INVT      (0x03UL << HRPWM_PCNAR25_U1CMEDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Match_B_Define HRPWM Pin Polarity For Count Down And U1 Match B Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_MATCH_B_LOW       (0x00UL << HRPWM_PCNAR25_U1CMBDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_B_HIGH      (0x01UL << HRPWM_PCNAR25_U1CMBDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_B_HOLD      (0x02UL << HRPWM_PCNAR25_U1CMBDCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_B_INVT      (0x03UL << HRPWM_PCNAR25_U1CMBDCA_POS)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Pin_Polarity_Down_U1_Match_A_Define HRPWM Pin Polarity For Count Down And U1 Match A Define
 * @{
 */
#define HRPWM_PWM_DOWN_U1_MATCH_A_LOW       (0x00UL << HRPWM_PCNAR25_U1CMADCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_A_HIGH      (0x01UL << HRPWM_PCNAR25_U1CMADCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_A_HOLD      (0x02UL << HRPWM_PCNAR25_U1CMADCA_POS)
#define HRPWM_PWM_DOWN_U1_MATCH_A_INVT      (0x03UL << HRPWM_PCNAR25_U1CMADCA_POS)
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
 * @defgroup HRPWM_Unit_Buf_Flag_Src_Define HRPWM Unit Buffer Flag Source define
 * @{
 */
#define HRPWM_UNIT_BUF_SRC_CNT_PEAK        (0x01UL << HRPWM_GCONR1_BFSEL_POS)   /*!< Count peak */
#define HRPWM_UNIT_BUF_SRC_CNT_VALLEY      (0x02UL << HRPWM_GCONR1_BFSEL_POS)   /*!< Count valley */
#define HRPWM_UNIT_BUF_SRC_SINGLE_BUF      (0x04UL << HRPWM_GCONR1_BFSEL_POS)   /*!< Single buffer transfer occurs */
#define HRPWM_UNIT_BUF_SRC_AFTER_U1_SINGLE (0x08UL << HRPWM_GCONR1_BFSEL_POS)   /*!< After HRPWM1 single buffer transfer occurs, and \
                                                                                     Sawtooth: count peak \
                                                                                     Triangle: count peak and valley */
#define HRPWM_UNIT_BUF_SRC_GLOBAL_BUF      (0x10UL << HRPWM_GCONR1_BFSEL_POS)   /*!< Global buffer transfer occurs */
#define HRPWM_UNIT_BUF_SRC_U1_ALL           (HRPWM_UNIT_BUF_SRC_CNT_PEAK   | HRPWM_UNIT_BUF_SRC_CNT_VALLEY | \
                                             HRPWM_UNIT_BUF_SRC_SINGLE_BUF | HRPWM_UNIT_BUF_SRC_GLOBAL_BUF)
#define HRPWM_UNIT_BUF_SRC_U2_8_ALL         (HRPWM_UNIT_BUF_SRC_CNT_PEAK   | HRPWM_UNIT_BUF_SRC_CNT_VALLEY      | \
                                             HRPWM_UNIT_BUF_SRC_SINGLE_BUF | HRPWM_UNIT_BUF_SRC_AFTER_U1_SINGLE | \
                                             HRPWM_UNIT_BUF_SRC_GLOBAL_BUF)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_Buf_Event_Define HRPWM Global Buffer event define
 * @{
 */
#define HRPWM_GLOBAL_BUF_EVT_U1_GLOBAL_BUF  (HRPWM_GBCR_U1U)
#define HRPWM_GLOBAL_BUF_EVT_U2_GLOBAL_BUF  (HRPWM_GBCR_U2U)
#define HRPWM_GLOBAL_BUF_EVT_U3_GLOBAL_BUF  (HRPWM_GBCR_U3U)
#define HRPWM_GLOBAL_BUF_EVT_U4_GLOBAL_BUF  (HRPWM_GBCR_U4U)
#define HRPWM_GLOBAL_BUF_EVT_U5_GLOBAL_BUF  (HRPWM_GBCR_U5U)
#define HRPWM_GLOBAL_BUF_EVT_U6_GLOBAL_BUF  (HRPWM_GBCR_U6U)
#define HRPWM_GLOBAL_BUF_EVT_U7_GLOBAL_BUF  (HRPWM_GBCR_U7U)
#define HRPWM_GLOBAL_BUF_EVT_U8_GLOBAL_BUF  (HRPWM_GBCR_U8U)
#define HRPWM_GLOBAL_BUF_EVT_VALID_PERIOD   (HRPWM_GBCR_VPEU)
#define HRPWM_GLOBAL_BUF_EVT_VALLEY         (HRPWM_GBCR_UDFU)
#define HRPWM_GLOBAL_BUF_EVT_PEAK           (HRPWM_GBCR_OVFU)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_Buf_Cond_Define HRPWM Global Buffer condition define
 * @{
 */
#define HRPWM_GLOBAL_BUF_COND_BUF_EVT           (0x00UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when buffer event (@ref HRPWM_Global_Buf_Event_Define) occurs  */
#define HRPWM_GLOBAL_BUF_COND_DMA_CPLT          (0x01UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when DMA transfer completes  */
#define HRPWM_GLOBAL_BUF_COND_AFTER_DMA_BUF_EVT (0x02UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when the buffer event (@ref HRPWM_Global_Buf_Event_Define) occurs which is after DMA transfer completes */
#define HRPWM_GLOBAL_BUF_COND_UPD_IN3           (0x03UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer at hrpwm_upd_in3 rising edge  */
#define HRPWM_GLOBAL_BUF_COND_UPD_IN2           (0x04UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer at hrpwm_upd_in2 rising edge  */
#define HRPWM_GLOBAL_BUF_COND_UPD_IN1           (0x05UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer at hrpwm_upd_in1 rising edge  */
#define HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN3_EVT (0x06UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when buffer event (@ref HRPWM_Global_Buf_Event_Define) occurs which is after hrpwm_upd_in3 rising edge */
#define HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN2_EVT (0x07UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when buffer event (@ref HRPWM_Global_Buf_Event_Define) occurs which is after hrpwm_upd_in2 rising edge */
#define HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN1_EVT (0x08UL << HRPWM_GBCR_UPDGAT_POS)       /*!< Global buffer transfer when buffer event (@ref HRPWM_Global_Buf_Event_Define) occurs which is after hrpwm_upd_in1 rising edge */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_Buf_Sync_Mode HRPWM Global Buffer synchronization mode define
 * @{
 */
#define HRPWM_GLOBAL_BUF_SYNC_MD_IMMED          (0x00UL << HRPWM_GBCR_RSYNCMD_POS)      /*!< Global buffer transfer immediately when other unit global transfer occurs or software buffer trigger */
#define HRPWM_GLOBAL_BUF_SYNC_MD_VALLEY         (0x01UL << HRPWM_GBCR_RSYNCMD_POS)      /*!< Global buffer transfer at next valley when other unit global transfer occurs or software buffer trigger  */
#define HRPWM_GLOBAL_BUF_SYNC_MD_PEAK           (0x02UL << HRPWM_GBCR_RSYNCMD_POS)      /*!< Global buffer transfer at next peak when other unit global transfer occurs or software buffer trigger */
#define HRPWM_GLOBAL_BUF_SYNC_MD_PEAK_OR_VALLEY (0x03UL << HRPWM_GBCR_RSYNCMD_POS)      /*!< Global buffer transfer at next valley or peak when other unit global transfer occurs or software buffer trigger */
/**
 * @}
 */

/**
 * @defgroup HRPWM_SW_Buf_Burst_Mode HRPWM Software Buffer Burst Mode define
 * @{
 */
#define HRPWM_SW_BUF_BURST_MD_CNT_STOP          (0x00UL << HRPWM_GBCR_SBTRUSFTMD_POS)   /*!< Software buffer burst only valid when count is stopped             */
#define HRPWM_SW_BUF_BURST_MD_CNT_STOP_OR_CNT   (0x01UL << HRPWM_GBCR_SBTRUSFTMD_POS)   /*!< Software buffer burst valid whether count is stopped or counting   */
/**
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
#define HRPWM_BM_UNIT_CNT_CONTINUE          (0x00UL)                            /*!< HRPWM Unit count continue in the BM output idle state */
#define HRPWM_BM_UNIT_CNT_STP_RST           (HRPWM_COMMON_BMCR_BMTMR1)          /*!< HRPWM Unit count stop and reset in the BM output idle state */
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
#define HRPWM_FLAG_VALID_PERIOD             ((uint64_t)HRPWM_STFLR1_VPEF)       /*!< Valid period */
#define HRPWM_FLAG_ONE_SHOT_CPLT            ((uint64_t)HRPWM_STFLR2_OSTOVF << 32U) /*!< Count complete in one shot timer mode */
#define HRPWM_FLAG_CLR_ALL                  (HRPWM_FLAG_MATCH_A | HRPWM_FLAG_MATCH_B | HRPWM_FLAG_MATCH_C | \
                                            HRPWM_FLAG_MATCH_D | HRPWM_FLAG_MATCH_E | HRPWM_FLAG_MATCH_F | \
                                            HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY | HRPWM_FLAG_UP_MATCH_SPECIAL_A | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_A | HRPWM_FLAG_UP_MATCH_SPECIAL_B | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_B | HRPWM_FLAG_CAPT_A | \
                                            HRPWM_FLAG_CAPT_B | HRPWM_FLAG_ONE_SHOT_CPLT | HRPWM_FLAG_VALID_PERIOD)
#define HRPWM_FLAG_ALL                      (HRPWM_FLAG_MATCH_A | HRPWM_FLAG_MATCH_B | HRPWM_FLAG_MATCH_C | \
                                            HRPWM_FLAG_MATCH_D | HRPWM_FLAG_MATCH_E | HRPWM_FLAG_MATCH_F | \
                                            HRPWM_FLAG_CNT_PEAK | HRPWM_FLAG_CNT_VALLEY | HRPWM_FLAG_UP_MATCH_SPECIAL_A | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_A | HRPWM_FLAG_UP_MATCH_SPECIAL_B | \
                                            HRPWM_FLAG_DOWN_MATCH_SPECIAL_B | HRPWM_FLAG_CAPT_A | HRPWM_FLAG_VALID_PERIOD | \
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
#define HRPWM_PH_FLAG_MATCH_PH_EVT6         (HRPWM_STFLR2_PHSCMP6F)     /*!< Phase match event 6*/
#define HRPWM_PH_FLAG_MATCH_PH_EVT7         (HRPWM_STFLR2_PHSCMP7F)     /*!< Phase match event 7*/
#define HRPWM_PH_FLAG_ALL                   (HRPWM_PH_FLAG_MATCH_PH_EVT1 | HRPWM_PH_FLAG_MATCH_PH_EVT2 | \
                                             HRPWM_PH_FLAG_MATCH_PH_EVT3 | HRPWM_PH_FLAG_MATCH_PH_EVT4 | \
                                             HRPWM_PH_FLAG_MATCH_PH_EVT5 | HRPWM_PH_FLAG_MATCH_PH_EVT6 | \
                                             HRPWM_PH_FLAG_MATCH_PH_EVT7)
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
#define HRPWM_IDLE_DELAY_FLAG_STAT_PP       (HRPWM_STFLR2_IPPSTAT)      /*!< Idle delay protection push-pull status */
#define HRPWM_IDLE_DELAY_FLAG_ALL           (HRPWM_IDLE_DELAY_FLAG_CHA_LVL  | HRPWM_IDLE_DELAY_FLAG_CHB_LVL  | \
                                             HRPWM_IDLE_DELAY_FLAG_STAT     | HRPWM_IDLE_DELAY_FLAG_STAT_CHA | \
                                             HRPWM_IDLE_DELAY_FLAG_STAT_CHB | HRPWM_IDLE_DELAY_FLAG_STAT_PP)

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
 * @defgroup HRPWM_Valid_Period_Valley_Define HRPWM Valid Period Function For Valley Define
 * @{
 */
#define HRPWM_VALID_PERIOD_VALLEY_OFF       (0x00UL)                /*!< Valid period function valley function off */
#define HRPWM_VALID_PERIOD_VALLEY_ON        (HRPWM_VPERR_UDFPERI)   /*!< Valid period function valley function on */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Valid_Period_Peak_Define HRPWM Valid Period Function For Peak Define
 * @{
 */
#define HRPWM_VALID_PERIOD_PEAK_OFF         (0x00UL)                /*!< Valid period function peak function off */
#define HRPWM_VALID_PERIOD_PEAK_ON          (HRPWM_VPERR_OVFPERI)   /*!< Valid period function peak function on */
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
 * @defgroup HRPWM_DeadTime_Mode_Define HRPWM Dead Time Mode Define
 * @{
 */
#define HRPWM_DEADTIME_MD_OVERLAP_INVD      (0x00UL)                                /*!< HRPWM dead time mode: overlap function invalid */
#define HRPWM_DEADTIME_MD_OVERLAP_RISING    (HRPWM_DCONR_SDTR)                      /*!< HRPWM dead time mode: overlap at rising edge */
#define HRPWM_DEADTIME_MD_OVERLAP_FALLING   (HRPWM_DCONR_SDTF)                      /*!< HRPWM dead time mode: overlap at falling edge */
#define HRPWM_DEADTIME_MD_OVERLAP_BOTH      (HRPWM_DCONR_SDTR | HRPWM_DCONR_SDTF)   /*!< HRPWM dead time mode: overlap at both rising and falling edges */
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
#define HRPWM_EMB_EVT_CH6                   (0x06UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 6 */
#define HRPWM_EMB_EVT_CH7                   (0x07UL << HRPWM_PCNAR1_EMBSA_POS)  /*!< EMB event channel 7 */
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
 * @defgroup HRPWM_HW_Start_Condition_1_Define  HRPWM Hardware Start Condition Internal Event and Trigger Define
 * @{
 */
#define HRPWM_HW_START_COND_NONE            ((uint64_t)0x00UL)
#define HRPWM_HW_START_COND_INTERN_EVT0     ((uint64_t)HRPWM_HSTAR1_HSTA8)
#define HRPWM_HW_START_COND_INTERN_EVT1     ((uint64_t)HRPWM_HSTAR1_HSTA9)
#define HRPWM_HW_START_COND_INTERN_EVT2     ((uint64_t)HRPWM_HSTAR1_HSTA10)
#define HRPWM_HW_START_COND_INTERN_EVT3     ((uint64_t)HRPWM_HSTAR1_HSTA11)
#define HRPWM_HW_START_COND_INTERN_EVT4     ((uint64_t)HRPWM_HSTAR1_HSTA11 | (HRPWM_HW_START_COND_MUX_INTERN_EVT4 << 32U))
#define HRPWM_HW_START_COND_INTERN_EVT5     ((uint64_t)HRPWM_HSTAR1_HSTA11 | (HRPWM_HW_START_COND_MUX_INTERN_EVT5 << 32U))
#define HRPWM_HW_START_COND_INTERN_EVT6     ((uint64_t)HRPWM_HSTAR1_HSTA11 | (HRPWM_HW_START_COND_MUX_INTERN_EVT6 << 32U))
#define HRPWM_HW_START_COND_INTERN_EVT7     ((uint64_t)HRPWM_HSTAR1_HSTA11 | (HRPWM_HW_START_COND_MUX_INTERN_EVT7 << 32U))
#define HRPWM_HW_START_COND_INTERN_EVT8     ((uint64_t)HRPWM_HSTAR1_HSTA11 | (HRPWM_HW_START_COND_MUX_INTERN_EVT8 << 32U))
#define HRPWM_HW_START_COND_TRIGA_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA16)
#define HRPWM_HW_START_COND_TRIGA_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA17)
#define HRPWM_HW_START_COND_TRIGB_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA18)
#define HRPWM_HW_START_COND_TRIGB_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA19)
#define HRPWM_HW_START_COND_TRIGC_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA20)
#define HRPWM_HW_START_COND_TRIGC_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA21)
#define HRPWM_HW_START_COND_TRIGD_RISING    ((uint64_t)HRPWM_HSTAR1_HSTA22)
#define HRPWM_HW_START_COND_TRIGD_FALLING   ((uint64_t)HRPWM_HSTAR1_HSTA23)
#define HRPWM_HW_START_COND_1_ALL           (0x00FF0F00UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Start_Condition_2_Define  HRPWM Hardware Start Condition Match and External Event Define
 * @{
 */
#define HRPWM_HW_START_COND_EXTEVT1             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV1 << 32U)
#define HRPWM_HW_START_COND_EXTEVT2             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV2 << 32U)
#define HRPWM_HW_START_COND_EXTEVT3             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV3 << 32U)
#define HRPWM_HW_START_COND_EXTEVT4             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV4 << 32U)
#define HRPWM_HW_START_COND_EXTEVT5             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV5 << 32U)
#define HRPWM_HW_START_COND_EXTEVT6             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV6 << 32U)
#define HRPWM_HW_START_COND_EXTEVT7             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV7 << 32U)
#define HRPWM_HW_START_COND_EXTEVT8             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV8 << 32U)
#define HRPWM_HW_START_COND_EXTEVT9             ((uint64_t)HRPWM_HSTAR2_HSTAEXEV9 << 32U)
#define HRPWM_HW_START_COND_EXTEVT10            ((uint64_t)HRPWM_HSTAR2_HSTAEXEV10 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA1 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB1 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA2 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB2 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA3 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB3 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA4 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB4 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA5 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB5 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_A          ((uint64_t)HRPWM_HSTAR2_HSTAGCMA6 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_B          ((uint64_t)HRPWM_HSTAR2_HSTAGCMB6 << 32U)
#define HRPWM_HW_START_COND_U7_MATCH_A          ((uint64_t)HRPWM_HSTAR1_HSTAGCMA7)
#define HRPWM_HW_START_COND_U7_MATCH_B          ((uint64_t)HRPWM_HSTAR1_HSTAGCMB7)
#define HRPWM_HW_START_COND_U8_MATCH_A          ((uint64_t)HRPWM_HSTAR1_HSTAGCMA8)
#define HRPWM_HW_START_COND_U8_MATCH_B          ((uint64_t)HRPWM_HSTAR1_HSTAGCMB8)
#define HRPWM_HW_START_COND_U1_MATCH_E          ((uint64_t)HRPWM_HSTAR2_HSTAGCME1 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_F          ((uint64_t)HRPWM_HSTAR2_HSTAGCMF1 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF2)
#define HRPWM_HW_START_COND_U3_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF3)
#define HRPWM_HW_START_COND_U4_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF4)
#define HRPWM_HW_START_COND_U5_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF5)
#define HRPWM_HW_START_COND_U6_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF6)
#define HRPWM_HW_START_COND_U7_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF7)
#define HRPWM_HW_START_COND_U8_MATCH_F          ((uint64_t)HRPWM_HSTAR1_HSTAGCMF8)
#define HRPWM_HW_START_COND_U1_PEAK             ((uint64_t)HRPWM_HSTAR2_HSTAOVF1 << 32U)
#define HRPWM_HW_START_COND_U1_VALLY            ((uint64_t)HRPWM_HSTAR2_HSTAUDF1 << 32U)
#define HRPWM_HW_START_COND_U1_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB1 << 32U)
#define HRPWM_HW_START_COND_U2_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB2 << 32U)
#define HRPWM_HW_START_COND_U3_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB3 << 32U)
#define HRPWM_HW_START_COND_U4_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB4 << 32U)
#define HRPWM_HW_START_COND_U5_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB5 << 32U)
#define HRPWM_HW_START_COND_U6_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR2_HSTASCMB6 << 32U)
#define HRPWM_HW_START_COND_U7_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR1_HSTASCMB7)
#define HRPWM_HW_START_COND_U8_MATCH_SPECIAL_B  ((uint64_t)HRPWM_HSTAR1_HSTASCMB8)
#define HRPWM_HW_START_COND_2_ALL               ((uint64_t)0x7F00300FUL | (uint64_t)0xFFFFFFFFUL << 32U)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Clear_Condition_1_Define HRPWM Hardware Clear Condition internal event & trigger Define
 * @{
 */
#define HRPWM_HW_CLR_COND_NONE              ((uint64_t)0x00UL)
#define HRPWM_HW_CLR_COND_INTERN_EVT0       ((uint64_t)HRPWM_HCLRR1_HCLE8)
#define HRPWM_HW_CLR_COND_INTERN_EVT1       ((uint64_t)HRPWM_HCLRR1_HCLE9)
#define HRPWM_HW_CLR_COND_INTERN_EVT2       ((uint64_t)HRPWM_HCLRR1_HCLE10)
#define HRPWM_HW_CLR_COND_INTERN_EVT3       ((uint64_t)HRPWM_HCLRR1_HCLE11)
#define HRPWM_HW_CLR_COND_INTERN_EVT4       ((uint64_t)HRPWM_HCLRR1_HCLE11 | (HRPWM_HW_CLR_COND_MUX_INTERN_EVT4 << 32U))
#define HRPWM_HW_CLR_COND_INTERN_EVT5       ((uint64_t)HRPWM_HCLRR1_HCLE11 | (HRPWM_HW_CLR_COND_MUX_INTERN_EVT5 << 32U))
#define HRPWM_HW_CLR_COND_INTERN_EVT6       ((uint64_t)HRPWM_HCLRR1_HCLE11 | (HRPWM_HW_CLR_COND_MUX_INTERN_EVT6 << 32U))
#define HRPWM_HW_CLR_COND_INTERN_EVT7       ((uint64_t)HRPWM_HCLRR1_HCLE11 | (HRPWM_HW_CLR_COND_MUX_INTERN_EVT7 << 32U))
#define HRPWM_HW_CLR_COND_INTERN_EVT8       ((uint64_t)HRPWM_HCLRR1_HCLE11 | (HRPWM_HW_CLR_COND_MUX_INTERN_EVT8 << 32U))
#define HRPWM_HW_CLR_COND_TRIGA_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE16)
#define HRPWM_HW_CLR_COND_TRIGA_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE17)
#define HRPWM_HW_CLR_COND_TRIGB_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE18)
#define HRPWM_HW_CLR_COND_TRIGB_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE19)
#define HRPWM_HW_CLR_COND_TRIGC_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE20)
#define HRPWM_HW_CLR_COND_TRIGC_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE21)
#define HRPWM_HW_CLR_COND_TRIGD_RISING      ((uint64_t)HRPWM_HCLRR1_HCLE22)
#define HRPWM_HW_CLR_COND_TRIGD_FALLING     ((uint64_t)HRPWM_HCLRR1_HCLE23)
#define HRPWM_HW_CLR_COND_1_ALL             (0x00FF0F00UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Clear_Condition_2_Define HRPWM Hardware Clear Condition external event & compare match Define
 * @{
 */
#define HRPWM_HW_CLR_COND_EXTEVT1               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV1 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT2               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV2 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT3               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV3 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT4               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV4 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT5               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV5 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT6               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV6 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT7               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV7 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT8               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV8 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT9               ((uint64_t)HRPWM_HCLRR2_HCLEEXEV9 << 32U)
#define HRPWM_HW_CLR_COND_EXTEVT10              ((uint64_t)HRPWM_HCLRR2_HCLEEXEV10 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA1 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB1 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA2 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB2 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA3 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB3 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA4 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB4 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA5 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB5 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_A            ((uint64_t)HRPWM_HCLRR2_HCLEGCMA6 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_B            ((uint64_t)HRPWM_HCLRR2_HCLEGCMB6 << 32U)
#define HRPWM_HW_CLR_COND_U7_MATCH_A            ((uint64_t)HRPWM_HCLRR1_HCLEGCMA7)
#define HRPWM_HW_CLR_COND_U7_MATCH_B            ((uint64_t)HRPWM_HCLRR1_HCLEGCMB7)
#define HRPWM_HW_CLR_COND_U8_MATCH_A            ((uint64_t)HRPWM_HCLRR1_HCLRGCMA8)
#define HRPWM_HW_CLR_COND_U8_MATCH_B            ((uint64_t)HRPWM_HCLRR1_HCLRGCMB8)
#define HRPWM_HW_CLR_COND_U1_MATCH_E            ((uint64_t)HRPWM_HCLRR2_HCLEGCME1 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_F            ((uint64_t)HRPWM_HCLRR2_HCLEGCMF1 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF2)
#define HRPWM_HW_CLR_COND_U3_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF3)
#define HRPWM_HW_CLR_COND_U4_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF4)
#define HRPWM_HW_CLR_COND_U5_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF5)
#define HRPWM_HW_CLR_COND_U6_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF6)
#define HRPWM_HW_CLR_COND_U7_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF7)
#define HRPWM_HW_CLR_COND_U8_MATCH_F            ((uint64_t)HRPWM_HCLRR1_HCLEGCMF8)
#define HRPWM_HW_CLR_COND_U1_PEAK               ((uint64_t)HRPWM_HCLRR2_HCLEAOVF1 << 32U)
#define HRPWM_HW_CLR_COND_U1_VALLY              ((uint64_t)HRPWM_HCLRR2_HCLEAUDF1 << 32U)
#define HRPWM_HW_CLR_COND_U1_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB1 << 32U)
#define HRPWM_HW_CLR_COND_U2_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB2 << 32U)
#define HRPWM_HW_CLR_COND_U3_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB3 << 32U)
#define HRPWM_HW_CLR_COND_U4_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB4 << 32U)
#define HRPWM_HW_CLR_COND_U5_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB5 << 32U)
#define HRPWM_HW_CLR_COND_U6_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR2_HCLESCMB6 << 32U)
#define HRPWM_HW_CLR_COND_U7_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR1_HCLESCMB7)
#define HRPWM_HW_CLR_COND_U8_MATCH_SPECIAL_B    ((uint64_t)HRPWM_HCLRR1_HCLESCMB8)
#define HRPWM_HW_CLR_COND_2_ALL                 ((uint64_t)0x7F00300FUL | (uint64_t)0xFFFFFFFFUL << 32U)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Capture_Condition_1_Define HRPWM Hardware Capture Condition internal event & trigger Define
 * @{
 */
#define HRPWM_HW_CAPT_COND_NONE             ((uint64_t)0x00UL)
#define HRPWM_HW_CAPT_COND_INTERN_EVT0      ((uint64_t)HRPWM_HCPAR1_HCPA8)
#define HRPWM_HW_CAPT_COND_INTERN_EVT1      ((uint64_t)HRPWM_HCPAR1_HCPA9)
#define HRPWM_HW_CAPT_COND_INTERN_EVT2      ((uint64_t)HRPWM_HCPAR1_HCPA10)
#define HRPWM_HW_CAPT_COND_INTERN_EVT3      ((uint64_t)HRPWM_HCPAR1_HCPA11)
#define HRPWM_HW_CAPT_COND_INTERN_EVT4      ((uint64_t)HRPWM_HCPAR1_HCPA11 | (HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT4 << 32U))
#define HRPWM_HW_CAPT_COND_INTERN_EVT5      ((uint64_t)HRPWM_HCPAR1_HCPA11 | (HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT5 << 32U))
#define HRPWM_HW_CAPT_COND_INTERN_EVT6      ((uint64_t)HRPWM_HCPAR1_HCPA11 | (HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT6 << 32U))
#define HRPWM_HW_CAPT_COND_INTERN_EVT7      ((uint64_t)HRPWM_HCPAR1_HCPA11 | (HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT7 << 32U))
#define HRPWM_HW_CAPT_COND_INTERN_EVT8      ((uint64_t)HRPWM_HCPAR1_HCPA11 | (HRPWM_HW_CAPT_A_COND_MUX_INTERN_EVT8 << 32U))
#define HRPWM_HW_CAPT_COND_TRIGA_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA16)
#define HRPWM_HW_CAPT_COND_TRIGA_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA17)
#define HRPWM_HW_CAPT_COND_TRIGB_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA18)
#define HRPWM_HW_CAPT_COND_TRIGB_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA19)
#define HRPWM_HW_CAPT_COND_TRIGC_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA20)
#define HRPWM_HW_CAPT_COND_TRIGC_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA21)
#define HRPWM_HW_CAPT_COND_TRIGD_RISING     ((uint64_t)HRPWM_HCPAR1_HCPA22)
#define HRPWM_HW_CAPT_COND_TRIGD_FALLING    ((uint64_t)HRPWM_HCPAR1_HCPA23)
#define HRPWM_HW_CAPT_COND_1_ALL            (0x00FF0F00UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_HW_Capture_Condition_2_Define HRPWM Hardware Capture Condition external event & compare match Define
 * @{
 */
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
#define HRPWM_HW_CAPT_COND_U1_MATCH_A       ((uint64_t)HRPWM_HCPAR2_HCPAGCMA1 << 32U)
#define HRPWM_HW_CAPT_COND_U1_MATCH_B       ((uint64_t)HRPWM_HCPAR2_HCPAGCMB1 << 32U)
#define HRPWM_HW_CAPT_COND_U2_MATCH_A       ((uint64_t)HRPWM_HCPAR2_HCPAGCMA2 << 32U)
#define HRPWM_HW_CAPT_COND_U2_MATCH_B       ((uint64_t)HRPWM_HCPAR2_HCPAGCMB2 << 32U)
#define HRPWM_HW_CAPT_COND_U3_MATCH_A       ((uint64_t)HRPWM_HCPAR2_HCPAGCMA3 << 32U)
#define HRPWM_HW_CAPT_COND_U3_MATCH_B       ((uint64_t)HRPWM_HCPAR2_HCPAGCMB3 << 32U)
#define HRPWM_HW_CAPT_COND_U4_MATCH_A       ((uint64_t)HRPWM_HCPAR2_HCPAGCMA4 << 32U)
#define HRPWM_HW_CAPT_COND_U4_MATCH_B       ((uint64_t)HRPWM_HCPAR2_HCPAGCMB4 << 32U)
#define HRPWM_HW_CAPT_COND_U5_MATCH_A       ((uint64_t)HRPWM_HCPAR2_HCPAGCMA5 << 32U)
#define HRPWM_HW_CAPT_COND_U5_MATCH_B       ((uint64_t)HRPWM_HCPAR2_HCPAGCMB5 << 32U)
#define HRPWM_HW_CAPT_COND_U6_MATCH_A       ((uint64_t)HRPWM_HCPAR1_HCPAGCMA6)
#define HRPWM_HW_CAPT_COND_U6_MATCH_B       ((uint64_t)HRPWM_HCPAR1_HCPAGCMB6)
#define HRPWM_HW_CAPT_COND_U7_MATCH_A       ((uint64_t)HRPWM_HCPAR1_HCPAGCMA7)
#define HRPWM_HW_CAPT_COND_U7_MATCH_B       ((uint64_t)HRPWM_HCPAR1_HCPAGCMB7)
#define HRPWM_HW_CAPT_COND_U8_MATCH_A       ((uint64_t)HRPWM_HCPAR1_HCPAGCMA8)
#define HRPWM_HW_CAPT_COND_U8_MATCH_B       ((uint64_t)HRPWM_HCPAR1_HCPAGCMB8)
#define HRPWM_HW_CAPT_COND_U1_SET_A         ((uint64_t)HRPWM_HCPAR2_HCPAR1 << 32U)
#define HRPWM_HW_CAPT_COND_U1_CLR_A         ((uint64_t)HRPWM_HCPAR2_HCPAF1 << 32U)
#define HRPWM_HW_CAPT_COND_U2_SET_A         ((uint64_t)HRPWM_HCPAR2_HCPAR2 << 32U)
#define HRPWM_HW_CAPT_COND_U2_CLR_A         ((uint64_t)HRPWM_HCPAR2_HCPAF2 << 32U)
#define HRPWM_HW_CAPT_COND_U3_SET_A         ((uint64_t)HRPWM_HCPAR2_HCPAR3 << 32U)
#define HRPWM_HW_CAPT_COND_U3_CLR_A         ((uint64_t)HRPWM_HCPAR2_HCPAF3 << 32U)
#define HRPWM_HW_CAPT_COND_U4_SET_A         ((uint64_t)HRPWM_HCPAR2_HCPAR4 << 32U)
#define HRPWM_HW_CAPT_COND_U4_CLR_A         ((uint64_t)HRPWM_HCPAR2_HCPAF4 << 32U)
#define HRPWM_HW_CAPT_COND_U5_SET_A         ((uint64_t)HRPWM_HCPAR2_HCPAR5 << 32U)
#define HRPWM_HW_CAPT_COND_U5_CLR_A         ((uint64_t)HRPWM_HCPAR2_HCPAF5 << 32U)
#define HRPWM_HW_CAPT_COND_U6_SET_A         ((uint64_t)HRPWM_HCPAR1_HCPAR6)
#define HRPWM_HW_CAPT_COND_U6_CLR_A         ((uint64_t)HRPWM_HCPAR1_HCPAF6)
#define HRPWM_HW_CAPT_COND_U7_SET_A         ((uint64_t)HRPWM_HCPAR1_HCPAR7)
#define HRPWM_HW_CAPT_COND_U7_CLR_A         ((uint64_t)HRPWM_HCPAR1_HCPAF7)
#define HRPWM_HW_CAPT_COND_U8_SET_A         ((uint64_t)HRPWM_HCPAR1_HCPAR8)
#define HRPWM_HW_CAPT_COND_U8_CLR_A         ((uint64_t)HRPWM_HCPAR1_HCPAF8)
#define HRPWM_HW_CAPT_COND_2_ALL            ((uint64_t)0x0F0000FFUL | (uint64_t)0xFFFFF7FFUL << 32U)
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
/*            _SRC1                  _SRC2           _SRC3          _SRC4       */
/*-------------------------------------------------------------------*/
/* EVT1       HRPWM_EEV1(PC12)   HRPWMEVT1[0]    HRPWMEVT2[0]    HRPWMEVT3[0]   */
/* EVT2       HRPWM_EEV2(PC11)   HRPWMEVT1[1]    HRPWMEVT2[1]    HRPWMEVT3[1]   */
/* EVT3       HRPWM_EEV3(PB7)    HRPWMEVT1[2]    HRPWMEVT2[2]    HRPWMEVT3[2]   */
/* EVT4       HRPWM_EEV4(PB6)    HRPWMEVT1[3]    HRPWMEVT2[3]    HRPWMEVT3[3]   */
/* EVT5       HRPWM_EEV5(PB9)    HRPWMEVT1[4]    HRPWMEVT2[4]    HRPWMEVT3[4]   */
/* EVT6       HRPWM_EEV6(PB5)    HRPWMEVT1[5]    HRPWMEVT2[5]    HRPWMEVT3[5]   */
/* EVT7       HRPWM_EEV7(PB4)    HRPWMEVT1[6]    HRPWMEVT2[6]    HRPWMEVT3[6]   */
/* EVT8       HRPWM_EEV8(PB8)    HRPWMEVT1[7]    HRPWMEVT2[7]    HRPWMEVT3[7]   */
/* EVT9       HRPWM_EEV9(PB3)    HRPWMEVT1[8]    HRPWMEVT2[8]    HRPWMEVT3[8]   */
/* EVT10      HRPWM_EEV10(PC6)   HRPWMEVT1[9]    HRPWMEVT2[9]    HRPWMEVT3[9]   */
/* HRPWMEVT1 can any choose 52 GPIO
   HRPWMEVT2 can any choose CMP1 ~ CMP8
   HRPWMEVT3 can any choose DFSDM_BK0~3, DFSDM_CEV1/2 0~3, DFSDM_ZCD 0~3, ADC0~7,
            TMR6_1_CMA/B/C/D/E/F/OVF/UDF/SCMA/B, PLA 1~16, HRPWM 1~8, SOGI 1~2  */

#define HRPWM_EVT_SRC1                      (0x00UL)
#define HRPWM_EVT_SRC2                      (0x01UL)
#define HRPWM_EVT_SRC3                      (0x02UL)
#define HRPWM_EVT_SRC4                      (0x03UL)
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
 * @defgroup HRPWM_EVT_Valid_Action_Define HRPWM External Event Valid Action Define
 * @{
 */
#define HRPWM_EVT_VALID_LVL                 (0x00UL)                        /*!< The level configured by API HRPWM_EVT_SetValidLevel() */
#define HRPWM_EVT_VALID_RISING              (HRPWM_COMMON_EECR1_EE1SNS_0)   /*!< Rising edge */
#define HRPWM_EVT_VALID_FALLING             (HRPWM_COMMON_EECR1_EE1SNS_1)   /*!< falling edge */
#define HRPWM_EVT_VALID_BOTH                (HRPWM_COMMON_EECR1_EE1SNS)     /*!< Edge */
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
 * @brief
 * **************************  blank mode  *****************************
 * HRPWMx_GCONR1.EEFM = 0
 *              U1      |        U2      |        U3      |        U4      |        U5      |        U6      |        U7      |        U8      |
 *     | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  | OFFWIN | PWMB  |
 * U1  |   1    |   2   |    3   |   4   |    5   |   -   |   6    |   -   |   7    |   12  |   8    |   13  |   9    |   -   |   10   |   11  |
 * U2  |   10   |   11  |    1   |   2   |    3   |   4   |   5    |   -   |   6    |   -   |   7    |   12  |   8    |   13  |   9    |   -   |
 * U3  |   9    |   -   |    10  |   11  |    1   |   2   |   3    |   4   |   5    |   -   |   6    |   -   |   7    |   12  |   8    |   13  |
 * U4  |   8    |   13  |    9   |   -   |    10  |   11  |   1    |   2   |   3    |   4   |   5    |   -   |   6    |   -   |   7    |   12  |
 * U5  |   7    |   12  |    8   |   13  |    9   |   -   |   10   |   11  |   1    |   2   |   3    |   4   |   5    |   -   |   6    |   -   |
 * U6  |   6    |   -   |    7   |   12  |    8   |   13  |   9    |   -   |   10   |   11  |   1    |   2   |   3    |   4   |   5    |   -   |
 * U7  |   5    |   -   |    6   |   -   |    7   |   12  |   8    |   13  |   9    |   -   |   10   |   11  |   1    |   2   |   3    |   4   |
 * U8  |   3    |   4   |    5   |   -   |    6   |   -   |   7    |   12  |   8    |   13  |   9    |   -   |   10   |   11  |   1    |   2   |
 * HRPWMx_GCONR1.EEFM = 1
 *               U1         |        U2          |         U3         |         U4         |         U5         |         U6         |         U7         |         U8         |
 *     |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |  OFF | WIN | PWMB  |
 * U1  |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |
 * U2  |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |
 * U3  |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |
 * U4  |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |
 * U5  |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |
 * U6  |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |   7  |  -  |  -    |
 * U7  |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |   4  |  5  |  6    |
 * U8  |   4  |  5  |  6    |   7  |  -  |  -    |   -  |  8  |  -    |   9  |  -  |  -    |   -  |  10 |  -    |   11 |  -  |  -    |   -  |  12 |  13   |   1  |  2  |  3    |
 *
 * **************************  windows mode  *****************************
 * HRPWMx_GCONR1.EEFM = 0   windows time is offset to windows
 * HRPWMx_GCONR1.EEFM = 1   windows time is reference point to windows
 * @{
 */
#define HRPWM_EVT_FILTER_OFF            (0x00UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Filter mode off */
#define HRPWM_EVT_FILTER_MD_BLK_1       (0x01UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_2       (0x02UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_3       (0x03UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_4       (0x04UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_5       (0x05UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_6       (0x06UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_7       (0x07UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_8       (0x08UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_9       (0x09UL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_10      (0x0AUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_11      (0x0BUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_12      (0x0CUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_BLK_13      (0x0DUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Blank mode : refer the table above */
#define HRPWM_EVT_FILTER_MD_WIN_OWN     (0x0EUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Windows mode : own unit            */
#define HRPWM_EVT_FILTER_MD_WIN_OTHER   (0x0FUL << HRPWM_EEFLTCR1_EE1FM_POS)    /*!< Windows mode : other unit \
                                                                                     U1  |  U2  |  U3  |  U4  |  U5  |  U6  |  U7  |  U8  | \
                                                                                     U2  |  U1  |  U4  |  U3  |  U6  |  U5  |  U8  |  U7  | */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EVT_Filter_Signal_Replace_Define HRPWM External Event Filter Signal Replace Mode Define
 * @{
 */
#define HRPWM_EVT_FILTER_REPLACE_MD1            (0x00UL)                                        /*!< Filter signal replace mode 1 */
#define HRPWM_EVT_FILTER_REPLACE_MD2_PEAK       (HRPWM_GCONR1_EEFM)                             /*!< Filter signal replace mode 2 with reference point, triangle: count peak; sawtooth: count peak or HW clear */
#define HRPWM_EVT_FILTER_REPLACE_MD2_VALLEY     (HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF_0)     /*!< Filter signal replace mode 2 with reference point count valley */
#define HRPWM_EVT_FILTER_REPLACE_MD2_VALLEY2    (HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF_1)     /*!< Filter signal replace mode 2 with reference point, triangle: count peak and count valley; sawtooth: count peak or HW clear */
#define HRPWM_EVT_FILTER_REPLACE_MD2_PEAK2      (HRPWM_GCONR1_EEFM | HRPWM_GCONR1_EEFREF)       /*!< Filter signal replace mode 2 with reference point, triangle: count peak and count valley; sawtooth: count valley */
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
 * @defgroup HRPWM_EVT_Count_Mode HRPWM External Event Count Mode
 * @{
 */
#define HRPWM_EVT_CNT_MD_NO_ACCUM           (0x00UL)                            /*!< The external event counter reseted at full period */
#define HRPWM_EVT_CNT_MD_ACCUM              (HRPWM_EEFLTCR3_EEVARSTM)          /*!< The external event counter reseted at full period, only if no event occurred in the last period  */
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
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U7    (0x0BUL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM7 complete period point */
#define HRPWM_BM_CNT_SRC_PEROID_POINT_U8    (0x0CUL << HRPWM_COMMON_BMCR_BMCLKS_POS)   /*!< HRPWM8 complete period point */
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
#define HRPWM_BM_TRIG_EVT7_AND_U1_VALLEY   ((uint64_t)HRPWM_COMMON_BMSTRG1_T1UDFEEV7)   /*!< HRPWM1 count valley after external event 7 */
#define HRPWM_BM_TRIG_EVT7_AND_U1_PEAK     ((uint64_t)HRPWM_COMMON_BMSTRG1_T1UDFEEV7)   /*!< HRPWM1 count peak after external event 7 */
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
#define HRPWM_BM_TRIG_U8_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG2_GCMB8 << 32U) /*!< HRPWM1 match HRGCMBR */
#define HRPWM_BM_TRIG_U8_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG2_GCMA8 << 32U) /*!< HRPWM1 match HRGCMAR */
#define HRPWM_BM_TRIG_U8_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG2_UDF8 << 32U)  /*!< HRPWM1 count valley */
#define HRPWM_BM_TRIG_U8_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG2_OVF8 << 32U)  /*!< HRPWM1 count peak */
#define HRPWM_BM_TRIG_U7_MATCH_B           ((uint64_t)HRPWM_COMMON_BMSTRG2_GCMB7 << 32U) /*!< HRPWM1 match HRGCMBR */
#define HRPWM_BM_TRIG_U7_MATCH_A           ((uint64_t)HRPWM_COMMON_BMSTRG2_GCMA7 << 32U) /*!< HRPWM1 match HRGCMAR */
#define HRPWM_BM_TRIG_U7_VALLEY            ((uint64_t)HRPWM_COMMON_BMSTRG2_UDF7 << 32U)  /*!< HRPWM1 count valley */
#define HRPWM_BM_TRIG_U7_PEAK              ((uint64_t)HRPWM_COMMON_BMSTRG2_OVF7 << 32U)  /*!< HRPWM1 count peak */
#define HRPWM_BM_TRIG_ALL                  ((uint64_t)0x7FFFFFFEUL | ((uint64_t)0x00FF000FUL << 32U))
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
#define HRPWM_PH_MATCH_IDX6                 (0x05UL)
#define HRPWM_PH_MATCH_IDX7                 (0x06UL)
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
#define HRPWM_SYNC_SRC_U7_CNT_VALLEY        (0x0CUL)
#define HRPWM_SYNC_SRC_U7_MATCH_SPECIAL_B   (0x0DUL)
#define HRPWM_SYNC_SRC_U8_CNT_VALLEY        (0x0EUL)
#define HRPWM_SYNC_SRC_U8_MATCH_SPECIAL_B   (0x0FUL)
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
 * @defgroup HRPWM_DAC_Ramp_Reset_Source HRPWM DAC Ramp Generator Reset Source
 * @{
 */
#define HRPWM_DAC_RAMP_RST_SRC_VALLEY           (0x00UL)                /*!< DAC ramp generator reset at valley */
#define HRPWM_DAC_RAMP_RST_SRC_PEAK             (HRPWM_CR_RAMPRS_0)     /*!< DAC ramp generator reset at peak   */
#define HRPWM_DAC_RAMP_RST_SRC_PEAK_OR_VALLEY   (HRPWM_CR_RAMPRS_1)     /*!< DAC ramp generator reset at peak or valley */
#define HRPWM_DAC_RAMP_RST_SRC_PWMA_RISING      (HRPWM_CR_RAMPRS)       /*!< DAC ramp generator reset at PWMA_ORG rising edge */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DAC_Ramp_Step_Source HRPWM DAC Ramp Generator Step Source
 * @{
 */
#define HRPWM_DAC_RAMP_STEP_SRC_MATCH_B         (0x00UL)            /*!< DAC ramp generator step at compare match B occurs */
#define HRPWM_DAC_RAMP_STEP_SRC_PWMA_FALLING    (HRPWM_CR_RAMPSS)   /*!< DAC ramp generator step at PWMA_ORG falling edge  */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Interleaving_Mode HRPWM Interleaving Mode Define
 * @{
 */
#define HRPWM_INTLV_MD_INVD                 (0x00UL)                /*!< Interleaving mode invalid */
#define HRPWM_INTLV_MD_3_PHASE              (HRPWM_CR_INTLVD_0)     /*!< 3-phase interleaving mode */
#define HRPWM_INTLV_MD_4_PHASE              (HRPWM_CR_INTLVD_1)     /*!< 4-phase interleaving mode */
/**
 * @}
 */

/**
 * @defgroup HRPWM_U1_Interleaving_Mode HRPWM Interleaving Mode Define
 * @{
 */
#define HRPWM_U1_INTLV_MD_INVD                 (0x00UL << HRPWM_PHSCTL_MINTLVD_POS)    /*!< HRPWM U1 Interleaving mode invalid */
#define HRPWM_U1_INTLV_MD_2_PHASE              (0x01UL << HRPWM_PHSCTL_MINTLVD_POS)    /*!< HRPWM U1 2-phase interleaving mode */
#define HRPWM_U1_INTLV_MD_3_PHASE              (0x02UL << HRPWM_PHSCTL_MINTLVD_POS)    /*!< HRPWM U1 3-phase interleaving mode */
#define HRPWM_U1_INTLV_MD_4_PHASE              (0x03UL << HRPWM_PHSCTL_MINTLVD_POS)    /*!< HRPWM U1 4-phase interleaving mode */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Compare_A_Delay_Mode HRPWM Compare A Delay Mode Define
 * @{
 */
#define HRPWM_CMPA_DLY_MD_OFF               (0x00UL)                /*!< Not delay, the GCMAR compare with counter immediately */
#define HRPWM_CMPA_DLY_MD_CAPTB             (HRPWM_CR_DELGCMA_0)    /*!< After capture B occurs, the GCMAR restart and then compare with counter */
#define HRPWM_CMPA_DLY_MD_CAPTB_OR_CMPF     (HRPWM_CR_DELGCMA_1)    /*!< After capture B occurs or compare F matches, the GCMAR restart and then compare with counter */
#define HRPWM_CMPA_DLY_MD_CAPTB_OR_CMPB     (HRPWM_CR_DELGCMA)      /*!< After capture B occurs or compare B matches, the GCMAR restart and then compare with counter */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Compare_E_Delay_Mode HRPWM Compare E Delay Mode Define
 * @{
 */
#define HRPWM_CMPE_DLY_MD_OFF               (0x00UL)                /*!< Not delay, the GCMER compare with counter immediately */
#define HRPWM_CMPE_DLY_MD_CAPTA             (HRPWM_CR_DELGCME_0)    /*!< After capture A occurs, the GCMER restart and then compare with counter */
#define HRPWM_CMPE_DLY_MD_CAPTA_OR_CMPF     (HRPWM_CR_DELGCME_1)    /*!< After capture A occurs or compare F matches, the GCMER restart and then compare with counter */
#define HRPWM_CMPE_DLY_MD_CAPTA_OR_CMPB     (HRPWM_CR_DELGCME)      /*!< After capture A occurs or compare B matches, the GCMER restart and then compare with counter */
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
 * @defgroup HRPWM_AOS_Event_Sel HRPWM AOS Event Selection
 * @{
 */
#define HRPWM_AOS_EVT_DLY_PROTECT               (HRPWM_AOSEOUTSELR_AOSENDLYPRT)     /*!< HRPWM delay protection event */
#define HRPWM_AOS_EVT_SPECIAL_PERIOD_INTERVAL   (HRPWM_AOSEOUTSELR_AOSENVPE)        /*!< HRPWM special period interval event */
#define HRPWM_AOS_EVT_CAPT_B                    (HRPWM_AOSEOUTSELR_AOSENCAPB)       /*!< HRPWM capture B event */
#define HRPWM_AOS_EVT_CAPT_A                    (HRPWM_AOSEOUTSELR_AOSENCAPA)       /*!< HRPWM capture A event */
#define HRPWM_AOS_EVT_VALLEY                    (HRPWM_AOSEOUTSELR_AOSENUDF)        /*!< HRPWM count valley event */
#define HRPWM_AOS_EVT_PEAK                      (HRPWM_AOSEOUTSELR_AOSENOVF)        /*!< HRPWM count peak event */
#define HRPWM_AOS_EVT_MATCH_F                   (HRPWM_AOSEOUTSELR_AOSENF)          /*!< HRPWM count match F (GCMFR) event */
#define HRPWM_AOS_EVT_MATCH_E                   (HRPWM_AOSEOUTSELR_AOSENE)          /*!< HRPWM count match E (GCMER) event */
#define HRPWM_AOS_EVT_MATCH_D                   (HRPWM_AOSEOUTSELR_AOSEND)          /*!< HRPWM count match D (GCMDR) event */
#define HRPWM_AOS_EVT_MATCH_C                   (HRPWM_AOSEOUTSELR_AOSENC)          /*!< HRPWM count match C (GCMCR) event */
#define HRPWM_AOS_EVT_MATCH_B                   (HRPWM_AOSEOUTSELR_AOSENB)          /*!< HRPWM count match B (GCMBR) event */
#define HRPWM_AOS_EVT_MATCH_A                   (HRPWM_AOSEOUTSELR_AOSENA)          /*!< HRPWM count match A (GCMAR) event */
#define HRPWM_AOS_EVT_MATCH_SPECIAL_B           (HRPWM_AOSEOUTSELR_AOSENSCMB)       /*!< HRPWM count match special B (SCMBR) event */
#define HRPWM_AOS_EVT_MATCH_SPECIAL_A           (HRPWM_AOSEOUTSELR_AOSENSCMA)       /*!< HRPWM count match special A (SCMAR) event */
#define HRPWM_AOS_EVT_MSK                       (0x3FFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Chopping_Pulse_Width_Define HRPWM Chopping fisrt pulse width definition
 * @{
 */
#define HRPWM_CHP_PULSE_WIDTH_16            (0x00UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 16  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_32            (0x01UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 32  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_48            (0x02UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 48  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_64            (0x03UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 64  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_80            (0x04UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 80  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_96            (0x05UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 96  * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_112           (0x06UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 112 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_128           (0x07UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 128 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_144           (0x08UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 144 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_160           (0x09UL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 160 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_176           (0x0AUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 176 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_192           (0x0BUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 192 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_208           (0x0CUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 208 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_224           (0x0DUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 224 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_240           (0x0EUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 240 * T(hrpwm) */
#define HRPWM_CHP_PULSE_WIDTH_256           (0x0FUL << HRPWM_CHPCR_STRPW_POS)   /*!< HRPWM first carrier signal pulse width is 256 * T(hrpwm) */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Chopping_Duty_Cycle_Define HRPWM Chopping duty cycle definition
 * @{
 */
#define HRPWM_CHP_DUTY_CYCLE_0_8            (0x00UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 0/8 */
#define HRPWM_CHP_DUTY_CYCLE_1_8            (0x01UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 1/8 */
#define HRPWM_CHP_DUTY_CYCLE_2_8            (0x02UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 2/8 */
#define HRPWM_CHP_DUTY_CYCLE_3_8            (0x03UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 3/8 */
#define HRPWM_CHP_DUTY_CYCLE_4_8            (0x04UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 4/8 */
#define HRPWM_CHP_DUTY_CYCLE_5_8            (0x05UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 5/8 */
#define HRPWM_CHP_DUTY_CYCLE_6_8            (0x06UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 6/8 */
#define HRPWM_CHP_DUTY_CYCLE_7_8            (0x07UL << HRPWM_CHPCR_CARDTY_POS)  /*!< HRPWM chopping duty cycle is 7/8 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Chopping_Carrier_Cycle_Define HRPWM Chopping duty cycle definition
 * @{
 */
#define HRPWM_CHP_CARRIER_CYCLE_16          (0x00UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 16  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_32          (0x01UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 32  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_48          (0x02UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 48  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_64          (0x03UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 64  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_80          (0x04UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 80  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_96          (0x05UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 96  * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_112         (0x06UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 112 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_128         (0x07UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 128 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_144         (0x08UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 144 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_160         (0x09UL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 160 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_176         (0x0AUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 176 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_192         (0x0BUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 192 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_208         (0x0CUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 208 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_224         (0x0DUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 224 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_240         (0x0EUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 240 * T(hrpwm) */
#define HRPWM_CHP_CARRIER_CYCLE_256         (0x0FUL << HRPWM_CHPCR_CARFRQ_POS)  /*!< HRPWM chopping carrier cycle is 256 * T(hrpwm) */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Link_Unit_Define HRPWM Link Unit definition
 * @{
 */
#define HRPWM_LINK_UNIT_U1                  (0x00UL)
#define HRPWM_LINK_UNIT_U2                  (0x01UL)
#define HRPWM_LINK_UNIT_U3                  (0x02UL)
#define HRPWM_LINK_UNIT_U4                  (0x03UL)
#define HRPWM_LINK_UNIT_U5                  (0x04UL)
#define HRPWM_LINK_UNIT_U6                  (0x05UL)
#define HRPWM_LINK_UNIT_U7                  (0x06UL)
#define HRPWM_LINK_UNIT_U8                  (0x07UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_DE_Active_Flag_Mode   HRPWM DE Active Mode Define
 * @{
 */
#define HRPWM_DE_ACT_FLAG_MD_CBC            (0x00UL)                            /*!< HRPWM DE active flag cycle by cycle */
#define HRPWM_DE_ACT_FLAG_MD_ONE_SHOT       (HRPWM_DECTL_MODE)                  /*!< HRPWM DE active flag one shot       */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DE_TRIP_Out_Source   HRPWM DE TRIP Out Source Define
 * @{
 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_0     (HRPWM_TRIPOUTSEL_EMBEVT0)          /*!< HRPWM DE TRIP out source: EMB event 0 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_1     (HRPWM_TRIPOUTSEL_EMBEVT1)          /*!< HRPWM DE TRIP out source: EMB event 1 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_2     (HRPWM_TRIPOUTSEL_EMBEVT2)          /*!< HRPWM DE TRIP out source: EMB event 2 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_3     (HRPWM_TRIPOUTSEL_EMBEVT3)          /*!< HRPWM DE TRIP out source: EMB event 3 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_4     (HRPWM_TRIPOUTSEL_EMBEVT4)          /*!< HRPWM DE TRIP out source: EMB event 4 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_5     (HRPWM_TRIPOUTSEL_EMBEVT5)          /*!< HRPWM DE TRIP out source: EMB event 5 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_6     (HRPWM_TRIPOUTSEL_EMBEVT6)          /*!< HRPWM DE TRIP out source: EMB event 6 */
#define HRPWM_DE_TRIP_OUT_SRC_EMB_EVT_7     (HRPWM_TRIPOUTSEL_EMBEVT7)          /*!< HRPWM DE TRIP out source: EMB event 7 */
#define HRPWM_DE_TRIP_OUT_SRC_EXT_EVT_1     (HRPWM_TRIPOUTSEL_EEV1)             /*!< HRPWM DE TRIP out source: external event 1 */
#define HRPWM_DE_TRIP_OUT_SRC_EXT_EVT_2     (HRPWM_TRIPOUTSEL_EEV2)             /*!< HRPWM DE TRIP out source: external event 2 */
#define HRPWM_DE_TRIP_OUT_SRC_EXT_EVT_3     (HRPWM_TRIPOUTSEL_EEV3)             /*!< HRPWM DE TRIP out source: external event 3 */
#define HRPWM_DE_TRIP_OUT_SRC_EXT_EVT_4     (HRPWM_TRIPOUTSEL_EEV4)             /*!< HRPWM DE TRIP out source: external event 4 */
#define HRPWM_DE_TRIP_OUT_SRC_EXT_EVT_5     (HRPWM_TRIPOUTSEL_EEV5)             /*!< HRPWM DE TRIP out source: external event 5 */
#define HRPWM_DE_TRIP_OUT_SRC_MASK          (0x3EFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_DE_TRIP_Source   HRPWM DE TRIP Source Define
 * @{
 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_1         (0UL)                               /*!< CMP out 1 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_2         (1UL)                               /*!< CMP out 2 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_3         (2UL)                               /*!< CMP out 3 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_4         (3UL)                               /*!< CMP out 4 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_5         (4UL)                               /*!< CMP out 5 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_6         (5UL)                               /*!< CMP out 6 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_7         (6UL)                               /*!< CMP out 7 */
#define HRPWM_DE_TRIP_SRC_CMP_OUT_8         (7UL)                               /*!< CMP out 8 */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_0       (8UL)                               /*!< HRPWMEVT1[0], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_1       (9UL)                               /*!< HRPWMEVT1[1], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_2       (10UL)                              /*!< HRPWMEVT1[2], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_3       (11UL)                              /*!< HRPWMEVT1[3], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_4       (12UL)                              /*!< HRPWMEVT1[4], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_5       (13UL)                              /*!< HRPWMEVT1[5], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_6       (14UL)                              /*!< HRPWMEVT1[6], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_7       (15UL)                              /*!< HRPWMEVT1[7], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_8       (16UL)                              /*!< HRPWMEVT1[8], from XBAR */
#define HRPWM_DE_TRIP_SRC_HRPWMEVT1_9       (17UL)                              /*!< HRPWMEVT1[9], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_0        (18UL)                              /*!< HEMBEVT1[0], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_1        (19UL)                              /*!< HEMBEVT1[1], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_2        (20UL)                              /*!< HEMBEVT1[2], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_3        (21UL)                              /*!< HEMBEVT1[3], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_4        (22UL)                              /*!< HEMBEVT1[4], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_5        (23UL)                              /*!< HEMBEVT1[5], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_6        (24UL)                              /*!< HEMBEVT1[6], from XBAR */
#define HRPWM_DE_TRIP_SRC_HEMBEVT1_7        (25UL)                              /*!< HEMBEVT1[7], from XBAR */
#define HRPWM_DE_TRIP_SRC_PLA0OUT           (32UL)                              /*!< PLA0OUT */
#define HRPWM_DE_TRIP_SRC_PLA1OUT           (33UL)                              /*!< PLA1OUT */
#define HRPWM_DE_TRIP_SRC_PLA2OUT           (34UL)                              /*!< PLA2OUT */
#define HRPWM_DE_TRIP_SRC_PLA3OUT           (35UL)                              /*!< PLA3OUT */
#define HRPWM_DE_TRIP_SRC_PLA4OUT           (36UL)                              /*!< PLA4OUT */
#define HRPWM_DE_TRIP_SRC_PLA5OUT           (37UL)                              /*!< PLA5OUT */
#define HRPWM_DE_TRIP_SRC_PLA6OUT           (38UL)                              /*!< PLA6OUT */
#define HRPWM_DE_TRIP_SRC_PLA7OUT           (39UL)                              /*!< PLA7OUT */
#define HRPWM_DE_TRIP_SRC_PLA8OUT           (40UL)                              /*!< PLA8OUT */
#define HRPWM_DE_TRIP_SRC_PLA9OUT           (41UL)                              /*!< PLA9OUT */
#define HRPWM_DE_TRIP_SRC_PLA10OUT          (42UL)                              /*!< PLA10OUT */
#define HRPWM_DE_TRIP_SRC_PLA11OUT          (43UL)                              /*!< PLA11OUT */
#define HRPWM_DE_TRIP_SRC_PLA12OUT          (44UL)                              /*!< PLA12OUT */
#define HRPWM_DE_TRIP_SRC_PLA13OUT          (45UL)                              /*!< PLA13OUT */
#define HRPWM_DE_TRIP_SRC_PLA14OUT          (46UL)                              /*!< PLA14OUT */
#define HRPWM_DE_TRIP_SRC_PLA15OUT          (47UL)                              /*!< PLA15OUT */
#define HRPWM_DE_TRIP_SRC_DSOGI_PPL1        (48UL)                              /*!< DSOGI_PPL1 */
#define HRPWM_DE_TRIP_SRC_DSOGI_PPL2        (49UL)                              /*!< DSOGI_PPL2 */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT1_0      (50UL)                              /*!< SDFM_CEVT1[0] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT1_1      (51UL)                              /*!< SDFM_CEVT1[1] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT1_2      (52UL)                              /*!< SDFM_CEVT1[2] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT1_3      (53UL)                              /*!< SDFM_CEVT1[3] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT2_0      (54UL)                              /*!< SDFM_CEVT2[0] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT2_1      (55UL)                              /*!< SDFM_CEVT2[1] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT2_2      (56UL)                              /*!< SDFM_CEVT2[2] */
#define HRPWM_DE_TRIP_SRC_SDFM_CEVT2_3      (57UL)                              /*!< SDFM_CEVT2[3] */
#define HRPWM_DE_TRIP_SRC_SDFM_ZCD_0        (58UL)                              /*!< SDFM_ZCD[0] */
#define HRPWM_DE_TRIP_SRC_SDFM_ZCD_1        (59UL)                              /*!< SDFM_ZCD[1] */
#define HRPWM_DE_TRIP_SRC_SDFM_ZCD_2        (60UL)                              /*!< SDFM_ZCD[2] */
#define HRPWM_DE_TRIP_SRC_SDFM_ZCD_3        (61UL)                              /*!< SDFM_ZCD[3] */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DE_PWM_Level_Mode   HRPWM DE PWM Level Mode Define
 * @{
 */
#define HRPWM_DE_PWM_LEVEL_TRIPSEL          (0x00UL)                            /*!< HRPWM DE, PWM level sync with the TRIP signal */
#define HRPWM_DE_PWM_LEVEL_INVT_TRIPSEL     (HRPWM_DEACTCTL_PWMA_0)             /*!< HRPWM DE, PWM level sync with the inverted TRIP signal */
#define HRPWM_DE_PWM_LEVEL_LOW              (HRPWM_DEACTCTL_PWMA_1)             /*!< HRPWM DE, PWM level fix low while DEACTIVE flag set */
#define HRPWM_DE_PWM_LEVEL_HIGH             (HRPWM_DEACTCTL_PWMA)               /*!< HRPWM DE, PWM level fix high while DEACTIVE flag set */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DE_PWM_TRIP_Sel   HRPWM DE PWM TRIP selection
 * @{
 */
#define HRPWM_DE_PWM_TRIP_SEL_TRIPH         (0x00UL)        /*!< HRPWM DE, select TRIPH as TRIP signal */
#define HRPWM_DE_PWM_TRIP_SEL_TRIPL         (0x01UL)        /*!< HRPWM DE, select TRIPL as TRIP signal */
/**
 * @}
 */

/**
 * @defgroup HRPWM_MINDB_PWM_Block_Sel   HRPWM minimum dead time PWM block signal selection
 * @{
 */
#define HRPWM_MINDB_PWM_BLK_SEL_A           (0x00UL)        /*!< HRPWM MINDB, select blockA as PWM block signal */
#define HRPWM_MINDB_PWM_BLK_SEL_B           (0x01UL)        /*!< HRPWM MINDB, select blockB as PWM block signal */
/**
 * @}
 */

/**
 * @defgroup HRPWM_MINDB_REF_Invert_Define HRPWM minimum dead time reference signal invert define
 * @{
 */
#define HRPWM_MINDB_REF_INVT_OFF            (0x00UL)        /*!< HRPWM MINDB, reference signal polarity not inverted */
#define HRPWM_MINDB_REF_INVT_ON             (0x01UL)        /*!< HRPWM MINDB, reference signal polarity inverted */
/**
 * @}
 */

/**
 * @defgroup HRPWM_MINDB_Block_Invert_Define HRPWM minimum dead time block signal invert define
 * @{
 */
#define HRPWM_MINDB_BLK_INVT_OFF            (0x00UL)        /*!< HRPWM MINDB, block signal polarity not inverted */
#define HRPWM_MINDB_BLK_INVT_ON             (0x01UL)        /*!< HRPWM MINDB, block signal polarity inverted */
/**
 * @}
 */

/**
 * @defgroup HRPWM_MINDB_REF_Sel HRPWM minimum dead time reference signal selection
 * @{
 */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_1      (0x00UL)        /*!< Select MINDBEVT_1 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_2      (0x01UL)        /*!< Select MINDBEVT_2 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_3      (0x02UL)        /*!< Select MINDBEVT_3 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_4      (0x03UL)        /*!< Select MINDBEVT_4 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_5      (0x04UL)        /*!< Select MINDBEVT_5 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_6      (0x05UL)        /*!< Select MINDBEVT_6 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_7      (0x06UL)        /*!< Select MINDBEVT_7 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_8      (0x07UL)        /*!< Select MINDBEVT_8 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_9      (0x08UL)        /*!< Select MINDBEVT_9 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_10     (0x09UL)        /*!< Select MINDBEVT_10 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_11     (0x0AUL)        /*!< Select MINDBEVT_11 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_12     (0x0BUL)        /*!< Select MINDBEVT_12 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_13     (0x0CUL)        /*!< Select MINDBEVT_13 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_14     (0x0DUL)        /*!< Select MINDBEVT_14 as reference signal */
#define HRPWM_MINDB_REF_SEL_MINDBEVT_15     (0x0EUL)        /*!< Select MINDBEVT_15 as reference signal */
#define HRPWM_MINDB_REF_SEL_PWM_DE_NO_HR    (0x0FUL)        /*!< Select PWM_DE_NO_HR as reference signal */
/**
 * @}
 */

/**
 * @defgroup HRPWM_LUT_Input3_Source      HRPWM LUT Input 3 Source
 * @{
 */
#define HRPWM_LUT_INT_SRC_ICLEVT_1          (0x00UL)        /*!< Select ICLEVT_1 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_2          (0x01UL)        /*!< Select ICLEVT_2 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_3          (0x02UL)        /*!< Select ICLEVT_3 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_4          (0x03UL)        /*!< Select ICLEVT_4 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_5          (0x04UL)        /*!< Select ICLEVT_5 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_6          (0x05UL)        /*!< Select ICLEVT_6 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_7          (0x06UL)        /*!< Select ICLEVT_7 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_8          (0x07UL)        /*!< Select ICLEVT_8 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_9          (0x08UL)        /*!< Select ICLEVT_9 as LUT input 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_10         (0x09UL)        /*!< Select ICLEVT_10 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_11         (0x0AUL)        /*!< Select ICLEVT_11 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_12         (0x0BUL)        /*!< Select ICLEVT_12 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_13         (0x0CUL)        /*!< Select ICLEVT_13 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_14         (0x0DUL)        /*!< Select ICLEVT_14 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_15         (0x0EUL)        /*!< Select ICLEVT_15 as LUT inupt 3 */
#define HRPWM_LUT_INT_SRC_ICLEVT_16         (0x0FUL)        /*!< Select ICLEVT_16 as LUT inupt 3 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_1_2_3_Event_Sel      HRPWM global AOS 1/2/3 event selection
 * @{
 */
#define HRPWM_GLAOS_1_2_3_EVT_U1_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA1)     /*!< HRPWM U1 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U1_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB1)     /*!< HRPWM U1 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U1_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF1)      /*!< HRPWM U1 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U1_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF1)      /*!< HRPWM U1 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_U2_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA2)     /*!< HRPWM U2 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U2_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB2)     /*!< HRPWM U2 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U2_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF2)      /*!< HRPWM U2 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U2_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF2)      /*!< HRPWM U2 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_U3_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA3)     /*!< HRPWM U3 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U3_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB3)     /*!< HRPWM U3 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U3_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF3)      /*!< HRPWM U3 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U3_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF3)      /*!< HRPWM U3 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_U4_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA4)     /*!< HRPWM U4 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U4_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB4)     /*!< HRPWM U4 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U4_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF4)      /*!< HRPWM U4 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U4_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF4)      /*!< HRPWM U4 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_U5_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA5)     /*!< HRPWM U5 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U5_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB5)     /*!< HRPWM U5 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U5_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF5)      /*!< HRPWM U5 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U5_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF5)      /*!< HRPWM U5 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_U6_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR1_CMSA6)     /*!< HRPWM U6 special match A */
#define HRPWM_GLAOS_1_2_3_EVT_U6_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR1_CMSB6)     /*!< HRPWM U6 special match B */
#define HRPWM_GLAOS_1_2_3_EVT_U6_PEAK               (HRPWM_COMMON_GLAOSSELR1_OVF6)      /*!< HRPWM U6 peak            */
#define HRPWM_GLAOS_1_2_3_EVT_U6_VALLEY             (HRPWM_COMMON_GLAOSSELR1_UDF6)      /*!< HRPWM U6 valley          */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_1              (HRPWM_COMMON_GLAOSSELR1_EEV1)      /*!< HRPWM external event 1   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_2              (HRPWM_COMMON_GLAOSSELR1_EEV2)      /*!< HRPWM external event 2   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_3              (HRPWM_COMMON_GLAOSSELR1_EEV3)      /*!< HRPWM external event 3   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_4              (HRPWM_COMMON_GLAOSSELR1_EEV4)      /*!< HRPWM external event 4   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_5              (HRPWM_COMMON_GLAOSSELR1_EEV5)      /*!< HRPWM external event 5   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_6              (HRPWM_COMMON_GLAOSSELR1_EEV6)      /*!< HRPWM external event 6   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_7              (HRPWM_COMMON_GLAOSSELR1_EEV7)      /*!< HRPWM external event 7   */
#define HRPWM_GLAOS_1_2_3_EVT_EXTEVT_8              (HRPWM_COMMON_GLAOSSELR1_EEV8)      /*!< HRPWM external event 8   */
#define HRPWM_GLAOS_1_2_3_EVT_ALL                   (0xFFFFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_4_5_6_Event_Sel      HRPWM global AOS 4/6/7 event selection
 * @{
 */
#define HRPWM_GLAOS_4_5_6_EVT_U4_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR4_CMSA4)     /*!< HRPWM U4 special match A */
#define HRPWM_GLAOS_4_5_6_EVT_U4_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR4_CMSB4)     /*!< HRPWM U4 special match B */
#define HRPWM_GLAOS_4_5_6_EVT_U4_PEAK               (HRPWM_COMMON_GLAOSSELR4_OVF4)      /*!< HRPWM U4 peak            */
#define HRPWM_GLAOS_4_5_6_EVT_U4_VALLEY             (HRPWM_COMMON_GLAOSSELR4_UDF4)      /*!< HRPWM U4 valley          */
#define HRPWM_GLAOS_4_5_6_EVT_U5_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR4_CMSA5)     /*!< HRPWM U5 special match A */
#define HRPWM_GLAOS_4_5_6_EVT_U5_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR4_CMSB5)     /*!< HRPWM U5 special match B */
#define HRPWM_GLAOS_4_5_6_EVT_U5_PEAK               (HRPWM_COMMON_GLAOSSELR4_OVF5)      /*!< HRPWM U5 peak            */
#define HRPWM_GLAOS_4_5_6_EVT_U5_VALLEY             (HRPWM_COMMON_GLAOSSELR4_UDF5)      /*!< HRPWM U5 valley          */
#define HRPWM_GLAOS_4_5_6_EVT_U6_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR4_CMSA6)     /*!< HRPWM U6 special match A */
#define HRPWM_GLAOS_4_5_6_EVT_U6_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR4_CMSB6)     /*!< HRPWM U6 special match B */
#define HRPWM_GLAOS_4_5_6_EVT_U6_PEAK               (HRPWM_COMMON_GLAOSSELR4_OVF6)      /*!< HRPWM U6 peak            */
#define HRPWM_GLAOS_4_5_6_EVT_U6_VALLEY             (HRPWM_COMMON_GLAOSSELR4_UDF6)      /*!< HRPWM U6 valley          */
#define HRPWM_GLAOS_4_5_6_EVT_U7_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR4_CMSA7)     /*!< HRPWM U7 special match A */
#define HRPWM_GLAOS_4_5_6_EVT_U7_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR4_CMSB7)     /*!< HRPWM U7 special match B */
#define HRPWM_GLAOS_4_5_6_EVT_U7_PEAK               (HRPWM_COMMON_GLAOSSELR4_OVF7)      /*!< HRPWM U7 peak            */
#define HRPWM_GLAOS_4_5_6_EVT_U7_VALLEY             (HRPWM_COMMON_GLAOSSELR4_UDF7)      /*!< HRPWM U7 valley          */
#define HRPWM_GLAOS_4_5_6_EVT_U8_MATCH_SPECIAL_A    (HRPWM_COMMON_GLAOSSELR4_CMSA8)     /*!< HRPWM U8 special match A */
#define HRPWM_GLAOS_4_5_6_EVT_U8_MATCH_SPECIAL_B    (HRPWM_COMMON_GLAOSSELR4_CMSB8)     /*!< HRPWM U8 special match B */
#define HRPWM_GLAOS_4_5_6_EVT_U8_PEAK               (HRPWM_COMMON_GLAOSSELR4_OVF8)      /*!< HRPWM U8 peak            */
#define HRPWM_GLAOS_4_5_6_EVT_U8_VALLEY             (HRPWM_COMMON_GLAOSSELR4_UDF8)      /*!< HRPWM U8 valley          */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_3              (HRPWM_COMMON_GLAOSSELR4_EEV3)      /*!< HRPWM external event 3   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_4              (HRPWM_COMMON_GLAOSSELR4_EEV4)      /*!< HRPWM external event 4   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_5              (HRPWM_COMMON_GLAOSSELR4_EEV5)      /*!< HRPWM external event 5   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_6              (HRPWM_COMMON_GLAOSSELR4_EEV6)      /*!< HRPWM external event 6   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_7              (HRPWM_COMMON_GLAOSSELR4_EEV7)      /*!< HRPWM external event 7   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_8              (HRPWM_COMMON_GLAOSSELR4_EEV8)      /*!< HRPWM external event 8   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_9              (HRPWM_COMMON_GLAOSSELR4_EEV9)      /*!< HRPWM external event 9   */
#define HRPWM_GLAOS_4_5_6_EVT_EXTEVT_10             (HRPWM_COMMON_GLAOSSELR4_EEV10)     /*!< HRPWM external event 10  */
#define HRPWM_GLAOS_4_5_6_EVT_ALL                   (0xFF0FFFFFUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_7_8_Event_Sel      HRPWM global AOS 7/8 event selection
 * @{
 */
#define HRPWM_GLAOS_7_8_EVT_U1_MATCH_SPECIAL_A      (0x00UL)                    /*!< HRPWM U1 special match A */
#define HRPWM_GLAOS_7_8_EVT_U1_MATCH_SPECIAL_B      (0x01UL)                    /*!< HRPWM U1 special match B */
#define HRPWM_GLAOS_7_8_EVT_U1_PEAK                 (0x02UL)                    /*!< HRPWM U1 peak            */
#define HRPWM_GLAOS_7_8_EVT_U1_VALLEY               (0x03UL)                    /*!< HRPWM U1 valley          */
#define HRPWM_GLAOS_7_8_EVT_U2_MATCH_SPECIAL_A      (0x04UL)                    /*!< HRPWM U2 special match A */
#define HRPWM_GLAOS_7_8_EVT_U2_MATCH_SPECIAL_B      (0x05UL)                    /*!< HRPWM U2 special match B */
#define HRPWM_GLAOS_7_8_EVT_U2_PEAK                 (0x06UL)                    /*!< HRPWM U2 peak            */
#define HRPWM_GLAOS_7_8_EVT_U2_VALLEY               (0x07UL)                    /*!< HRPWM U2 valley          */
#define HRPWM_GLAOS_7_8_EVT_U3_MATCH_SPECIAL_A      (0x08UL)                    /*!< HRPWM U3 special match A */
#define HRPWM_GLAOS_7_8_EVT_U3_MATCH_SPECIAL_B      (0x09UL)                    /*!< HRPWM U3 special match B */
#define HRPWM_GLAOS_7_8_EVT_U3_PEAK                 (0x0AUL)                    /*!< HRPWM U3 peak            */
#define HRPWM_GLAOS_7_8_EVT_U3_VALLEY               (0x0BUL)                    /*!< HRPWM U3 valley          */
#define HRPWM_GLAOS_7_8_EVT_U4_MATCH_SPECIAL_A      (0x0CUL)                    /*!< HRPWM U4 special match A */
#define HRPWM_GLAOS_7_8_EVT_U4_MATCH_SPECIAL_B      (0x0DUL)                    /*!< HRPWM U4 special match B */
#define HRPWM_GLAOS_7_8_EVT_U4_PEAK                 (0x0EUL)                    /*!< HRPWM U4 peak            */
#define HRPWM_GLAOS_7_8_EVT_U4_VALLEY               (0x0FUL)                    /*!< HRPWM U4 valley          */
#define HRPWM_GLAOS_7_8_EVT_U5_MATCH_SPECIAL_A      (0x10UL)                    /*!< HRPWM U5 special match A */
#define HRPWM_GLAOS_7_8_EVT_U5_MATCH_SPECIAL_B      (0x11UL)                    /*!< HRPWM U5 special match B */
#define HRPWM_GLAOS_7_8_EVT_U5_PEAK                 (0x12UL)                    /*!< HRPWM U5 peak            */
#define HRPWM_GLAOS_7_8_EVT_U5_VALLEY               (0x13UL)                    /*!< HRPWM U5 valley          */
#define HRPWM_GLAOS_7_8_EVT_U6_MATCH_SPECIAL_A      (0x14UL)                    /*!< HRPWM U6 special match A */
#define HRPWM_GLAOS_7_8_EVT_U6_MATCH_SPECIAL_B      (0x15UL)                    /*!< HRPWM U6 special match B */
#define HRPWM_GLAOS_7_8_EVT_U6_PEAK                 (0x16UL)                    /*!< HRPWM U6 peak            */
#define HRPWM_GLAOS_7_8_EVT_U6_VALLEY               (0x17UL)                    /*!< HRPWM U6 valley          */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_1                (0x18UL)                    /*!< HRPWM external event 1   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_2                (0x19UL)                    /*!< HRPWM external event 2   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_3                (0x1AUL)                    /*!< HRPWM external event 3   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_4                (0x1BUL)                    /*!< HRPWM external event 4   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_5                (0x1CUL)                    /*!< HRPWM external event 5   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_6                (0x1DUL)                    /*!< HRPWM external event 6   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_7                (0x1EUL)                    /*!< HRPWM external event 7   */
#define HRPWM_GLAOS_7_8_EVT_EXTEVT_8                (0x1FUL)                    /*!< HRPWM external event 8   */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_9_10_Event_Sel      HRPWM global AOS 9/10 event selection
 * @{
 */
#define HRPWM_GLAOS_9_10_EVT_U4_MATCH_SPECIAL_A     (0x00UL)                    /*!< HRPWM U4 special match A */
#define HRPWM_GLAOS_9_10_EVT_U4_MATCH_SPECIAL_B     (0x01UL)                    /*!< HRPWM U4 special match B */
#define HRPWM_GLAOS_9_10_EVT_U4_PEAK                (0x02UL)                    /*!< HRPWM U4 peak            */
#define HRPWM_GLAOS_9_10_EVT_U4_VALLEY              (0x03UL)                    /*!< HRPWM U4 valley          */
#define HRPWM_GLAOS_9_10_EVT_U5_MATCH_SPECIAL_A     (0x04UL)                    /*!< HRPWM U5 special match A */
#define HRPWM_GLAOS_9_10_EVT_U5_MATCH_SPECIAL_B     (0x05UL)                    /*!< HRPWM U5 special match B */
#define HRPWM_GLAOS_9_10_EVT_U5_PEAK                (0x06UL)                    /*!< HRPWM U5 peak            */
#define HRPWM_GLAOS_9_10_EVT_U5_VALLEY              (0x07UL)                    /*!< HRPWM U5 valley          */
#define HRPWM_GLAOS_9_10_EVT_U6_MATCH_SPECIAL_A     (0x08UL)                    /*!< HRPWM U6 special match A */
#define HRPWM_GLAOS_9_10_EVT_U6_MATCH_SPECIAL_B     (0x09UL)                    /*!< HRPWM U6 special match B */
#define HRPWM_GLAOS_9_10_EVT_U6_PEAK                (0x0AUL)                    /*!< HRPWM U6 peak            */
#define HRPWM_GLAOS_9_10_EVT_U6_VALLEY              (0x0BUL)                    /*!< HRPWM U6 valley          */
#define HRPWM_GLAOS_9_10_EVT_U7_MATCH_SPECIAL_A     (0x0CUL)                    /*!< HRPWM U7 special match A */
#define HRPWM_GLAOS_9_10_EVT_U7_MATCH_SPECIAL_B     (0x0DUL)                    /*!< HRPWM U7 special match B */
#define HRPWM_GLAOS_9_10_EVT_U7_PEAK                (0x0EUL)                    /*!< HRPWM U7 peak            */
#define HRPWM_GLAOS_9_10_EVT_U7_VALLEY              (0x0FUL)                    /*!< HRPWM U7 valley          */
#define HRPWM_GLAOS_9_10_EVT_U8_MATCH_SPECIAL_A     (0x10UL)                    /*!< HRPWM U8 special match A */
#define HRPWM_GLAOS_9_10_EVT_U8_MATCH_SPECIAL_B     (0x11UL)                    /*!< HRPWM U8 special match B */
#define HRPWM_GLAOS_9_10_EVT_U8_PEAK                (0x12UL)                    /*!< HRPWM U8 peak            */
#define HRPWM_GLAOS_9_10_EVT_U8_VALLEY              (0x13UL)                    /*!< HRPWM U8 valley          */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_3               (0x18UL)                    /*!< HRPWM external event 3   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_4               (0x19UL)                    /*!< HRPWM external event 4   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_5               (0x1AUL)                    /*!< HRPWM external event 5   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_6               (0x1BUL)                    /*!< HRPWM external event 6   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_7               (0x1CUL)                    /*!< HRPWM external event 7   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_8               (0x1DUL)                    /*!< HRPWM external event 8   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_9               (0x1EUL)                    /*!< HRPWM external event 9   */
#define HRPWM_GLAOS_9_10_EVT_EXTEVT_10              (0x1FUL)                    /*!< HRPWM external event 10  */
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_Event_Num_Define      HRPWM global AOS event numer definition
 * @{
 */
#define HRPWM_GLAOS_EVT_1                   (0x00UL)
#define HRPWM_GLAOS_EVT_2                   (0x01UL)
#define HRPWM_GLAOS_EVT_3                   (0x02UL)
#define HRPWM_GLAOS_EVT_4                   (0x03UL)
#define HRPWM_GLAOS_EVT_5                   (0x04UL)
#define HRPWM_GLAOS_EVT_6                   (0x05UL)
#define HRPWM_GLAOS_EVT_7                   (0x06UL)
#define HRPWM_GLAOS_EVT_8                   (0x07UL)
#define HRPWM_GLAOS_EVT_9                   (0x08UL)
#define HRPWM_GLAOS_EVT_10                  (0x09UL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_Global_AOS_Event_Buf      HRPWM global AOS event buffer
 * @{
 */
#define HRPWM_GLAOS_EVT_BUF_INVD                (0x00UL)    /*!< HRPWM global AOS event buffer invalid */
#define HRPWM_GLAOS_EVT_BUF_U1                  (0x01UL)    /*!< HRPWM global AOS event buffer update when U1 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U2                  (0x02UL)    /*!< HRPWM global AOS event buffer update when U2 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U3                  (0x03UL)    /*!< HRPWM global AOS event buffer update when U3 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U4                  (0x04UL)    /*!< HRPWM global AOS event buffer update when U4 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U5                  (0x05UL)    /*!< HRPWM global AOS event buffer update when U5 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U6                  (0x06UL)    /*!< HRPWM global AOS event buffer update when U6 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U7                  (0x07UL)    /*!< HRPWM global AOS event buffer update when U7 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U8                  (0x08UL)    /*!< HRPWM global AOS event buffer update when U8 global AOS occurs */
#define HRPWM_GLAOS_EVT_BUF_U9                  (0x09UL)    /*!< HRPWM global AOS event buffer update when U9 global AOS occurs */
/**
 * @}
 */

/**
 * @defgroup HRPWM_DMA_Update_Reg      HRPWM DMA update register
 * @{
 */
#define HRPWM_DMA_UPDATE_REG_CNT                    ((uint64_t)HRPWM_COMMON_BDUPR1_CNTER)           /*!< Update reg HRPWMx_CNTER by burst DMA */
#define HRPWM_DMA_UPDATE_REG_UPDATE_VALUE           ((uint64_t)HRPWM_COMMON_BDUPR1_UPDAR)           /*!< Update reg HRPWMx_UPDAR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_PERIOD_VALUE           ((uint64_t)HRPWM_COMMON_BDUPR1_HRPERAR)         /*!< Update reg HRPWMx_HRPERAR when HRPWMx_BCONR1.BENP is 0, update reg HRPWMx_HRPERBR when HRPWMx_BCONR1.BENP is 1 */
#define HRPWM_DMA_UPDATE_REG_CMP_VALUE_A            ((uint64_t)HRPWM_COMMON_BDUPR1_HRGCMAR)         /*!< Update reg HRPWMx_HRGCMAR when HRPWMx_BCONR1.BENAE is 0, update reg HRPWMx_HRGCMCR when HRPWMx_BCONR1.BENAE is 1 */
#define HRPWM_DMA_UPDATE_REG_CMP_VALUE_B            ((uint64_t)HRPWM_COMMON_BDUPR1_HRGCMBR)         /*!< Update reg HRPWMx_HRGCMBR when HRPWMx_BCONR1.BENBF is 0, update reg HRPWMx_HRGCMDR when HRPWMx_BCONR1.BENBF is 1 */
#define HRPWM_DMA_UPDATE_REG_CMP_VALUE_E            ((uint64_t)HRPWM_COMMON_BDUPR1_HRGCMER)         /*!< Update reg HRPWMx_HRGCMER when HRPWMx_BCONR1.BENAE is 0, update reg HRPWMx_HRGCMGR when HRPWMx_BCONR1.BENAE is 1 */
#define HRPWM_DMA_UPDATE_REG_CMP_VALUE_F            ((uint64_t)HRPWM_COMMON_BDUPR1_HRGCMFR)         /*!< Update reg HRPWMx_HRGCMFR when HRPWMx_BCONR1.BENBF is 0, update reg HRPWMx_HRGCMHR when HRPWMx_BCONR1.BENBF is 1 */
#define HRPWM_DMA_UPDATE_REG_SPECIAL_CMP_VALUE_A    ((uint64_t)HRPWM_COMMON_BDUPR1_SCMAR)           /*!< Update reg HRPWMx_SCMAR when HRPWMx_BCONR1.BENSPA is 0, update reg HRPWMx_SCMCR when HRPWMx_BCONR1.BENSPA is 1 */
#define HRPWM_DMA_UPDATE_REG_SPECIAL_CMP_VALUE_B    ((uint64_t)HRPWM_COMMON_BDUPR1_SCMBR)           /*!< Update reg HRPWMx_SCMBR when HRPWMx_BCONR1.BENSPB is 0, update reg HRPWMx_SCMDR when HRPWMx_BCONR1.BENSPB is 1 */
#define HRPWM_DMA_UPDATE_REG_DEAD_TIME_UP           ((uint64_t)HRPWM_COMMON_BDUPR1_HRDTUAR)         /*!< Update reg HRPWMx_HRDTUAR when HRPWMx_DCONR.DTBENU is 0, update reg HRPWMx_HRDTUBR when HRPWMx_DCONR.DTBENU is 1 */
#define HRPWM_DMA_UPDATE_REG_DEAD_TIME_DOWN         ((uint64_t)HRPWM_COMMON_BDUPR1_HRDTDAR)         /*!< Update reg HRPWMx_HRDTDAR when HRPWMx_DCONR.DTBEND is 0, update reg HRPWMx_HRDTDBR when HRPWMx_DCONR.DTBEND is 1 */
#define HRPWM_DMA_UPDATE_REG_EXTEVT_FILTER_OFFSET   ((uint64_t)HRPWM_COMMON_BDUPR1_EEOFSETAR)       /*!< Update reg HRPWMx_EEOFFSETAR when HRPWMx_BCONR2.BENEEFOFF is 0, update reg HRPWMx_EEFOFFSETBR when HRPWMx_BCONR2.BENEEFOFF is 1 */
#define HRPWM_DMA_UPDATE_REG_EXTEVT_FILTER_WIN      ((uint64_t)HRPWM_COMMON_BDUPR1_EEWINAR)         /*!< Upd ate reg HRPWMx_EEWINAR when HRPWMx_BCONR2.BENEEFWIN is 0, update reg HRPWMx_EEWINBR when HRPWMx_BCONR2.BENEEFWIN is 1 */
#define HRPWM_DMA_UPDATE_REG_VALID_PERIOD_VALUE     ((uint64_t)HRPWM_COMMON_BDUPR1_VPERR)           /*!< Update reg HRPWMx_VPERR when HRPWMx_BCONR1.BENVPE is 0, update reg HRPWMx_BVPERR when HRPWMx_BCONR1.BENVPE is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_A1            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNAR1)          /*!< Update reg HRPWMx_PCNAR1 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNAR1 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_A2            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNAR2)          /*!< Update reg HRPWMx_PCNAR2 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNAR2 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_A3            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNAR3)          /*!< Update reg HRPWMx_PCNAR3 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNAR3 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_A4            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNAR4)          /*!< Update reg HRPWMx_PCNAR4 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNAR4 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_A5            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNAR5)          /*!< Update reg HRPWMx_PCNAR5 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNAR5 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_B1            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNBR1)          /*!< Update reg HRPWMx_PCNBR1 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNBR1 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_B2            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNBR2)          /*!< Update reg HRPWMx_PCNBR2 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNBR2 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_B3            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNBR3)          /*!< Update reg HRPWMx_PCNBR3 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNBR3 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_B4            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNBR4)          /*!< Update reg HRPWMx_PCNBR4 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNBR4 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_PORT_CTL_B5            ((uint64_t)HRPWM_COMMON_BDUPR1_PCNBR5)          /*!< Update reg HRPWMx_PCNBR5 when HRPWMx_BCONR1.BENPCN is 0, update reg HRPWMx_BPCNBR5 when HRPWMx_BCONR1.BENPCN is 1 */
#define HRPWM_DMA_UPDATE_REG_INTC                   ((uint64_t)HRPWM_COMMON_BDUPR1_ICONR)           /*!< Update reg HRPWMx_ICONR when HRPWMx_BCONR1.BENCTL is 0, update reg HRPWMx_BICONR when HRPWMx_BCONR1.BENCTL is 1 */
#define HRPWM_DMA_UPDATE_REG_AOS_EVT_OUT            ((uint64_t)HRPWM_COMMON_BDUPR1_AOSEOUTSELR)     /*!< Update reg HRPWMx_AOSEOUTSELR when HRPWMx_BCONR1.BENCTL is 0, update reg HRPWMx_BAOSEOUTSELR when HRPWMx_BCONR1.BENCTL is 1 */
#define HRPWM_DMA_UPDATE_REG_DEAD_TIME              ((uint64_t)HRPWM_COMMON_BDUPR1_DCONR)           /*!< Update reg HRPWMx_DCONR when HRPWMx_DCONR.DTBENSD is 0, update reg HRPWMx_BDCONR when HRPWMx_DCONR.DTBENSD is 1 */
#define HRPWM_DMA_UPDATE_REG_GENERAL_CTL_1          ((uint64_t)HRPWM_COMMON_BDUPR1_GCONR1)          /*!< Update reg HRPWMx_GCONR1 when HRPWMx_BCONR1.BENCTL is 0, update reg HRPWMx_BGCONR1 when HRPWMx_BCONR1.BENCTL is 1 */
#define HRPWM_DMA_UPDATE_REG_GENERAL_CTL            ((uint64_t)HRPWM_COMMON_BDUPR1_GCONR)           /*!< Update reg HRPWMx_GCONR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_BUF_CTL_1              ((uint64_t)HRPWM_COMMON_BDUPR1_BCONR1)          /*!< Update reg HRPWMx_BCONR1 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_BUF_CTL_2              ((uint64_t)HRPWM_COMMON_BDUPR2_BCONR2 << 32U)   /*!< Update reg HRPWMx_BCONR2 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_GLOBAL_BUF             ((uint64_t)HRPWM_COMMON_BDUPR2_GBCR << 32U)     /*!< Update reg HRPWMx_GBCR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_GLOBAL_BUF_CFG         ((uint64_t)HRPWM_COMMON_BDUPR2_GBCFG << 32U)    /*!< Update reg HRPWMx_GBCFG by burst DMA */
#define HRPWM_DMA_UPDATE_REG_IDLE                   ((uint64_t)HRPWM_COMMON_BDUPR2_IDLECR << 32U)   /*!< Update reg HRPWMx_IDLECR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_CHP                    ((uint64_t)HRPWM_COMMON_BDUPR2_CHPCR << 32U)    /*!< Update reg HRPWMx_CHPCR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_HW_START_1             ((uint64_t)HRPWM_COMMON_BDUPR2_HASTR1 << 32U)   /*!< Update reg HRPWMx_HASTR1 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_HW_START_2             ((uint64_t)HRPWM_COMMON_BDUPR2_HASTR2 << 32U)   /*!< Update reg HRPWMx_HASTR2 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_HW_CLR_1               ((uint64_t)HRPWM_COMMON_BDUPR2_HCLR1 << 32U)    /*!< Update reg HRPWMx_HCLRR1 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_HW_CLR_2               ((uint64_t)HRPWM_COMMON_BDUPR2_HCLR2 << 32U)    /*!< Update reg HRPWMx_HCLRR2 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_EXTEVT_FILTER_1        ((uint64_t)HRPWM_COMMON_BDUPR2_EEFLTR1 << 32U)  /*!< Update reg HRPWMx_EEFLTR1 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_EXTEVT_FILTER_2        ((uint64_t)HRPWM_COMMON_BDUPR2_EEFLTR2 << 32U)  /*!< Update reg HRPWMx_EEFLTR2 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_EXTEVT_FILTER_3        ((uint64_t)HRPWM_COMMON_BDUPR2_EEFLTR3 << 32U)  /*!< Update reg HRPWMx_EEFLTR3 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_LINK_1                 ((uint64_t)HRPWM_COMMON_BDUPR2_LINCR1 << 32U)   /*!< Update reg HRPWMx_LINCR1 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_LINK_2                 ((uint64_t)HRPWM_COMMON_BDUPR2_LINCR2 << 32U)   /*!< Update reg HRPWMx_LINCR2 by burst DMA */
#define HRPWM_DMA_UPDATE_REG_CR                     ((uint64_t)HRPWM_COMMON_BDUPR2_CR << 32U)       /*!< Update reg HRPWMx_CR by burst DMA */
#define HRPWM_DMA_UPDATE_REG_PHASE                  ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCTL << 32U)   /*!< Update reg HRPWMx_PHSCTL by burst DMA */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_1      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP1A << 32U) /*!< Update reg HRPWMx_PHSCMP1A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP1B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_2      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP2A << 32U) /*!< Update reg HRPWMx_PHSCMP2A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP2B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_3      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP3A << 32U) /*!< Update reg HRPWMx_PHSCMP3A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP3B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_4      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP4A << 32U) /*!< Update reg HRPWMx_PHSCMP4A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP4B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_5      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP5A << 32U) /*!< Update reg HRPWMx_PHSCMP5A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP5B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_6      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP6A << 32U) /*!< Update reg HRPWMx_PHSCMP6A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP6B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_PHASE_CMP_VALUE_7      ((uint64_t)HRPWM_COMMON_BDUPR2_PHSCMP7A << 32U) /*!< Update reg HRPWMx_PHSCMP7A when HRPWMx_PHSCTL.BENPHS is 0, update reg HRPWMx_PHSCMP7B when HRPWMx_PHSCTL.BENPHS is 1 */
#define HRPWM_DMA_UPDATE_REG_ALL_U1                 ((uint64_t)0xFFFFFFF3UL | ((uint64_t)0x007FFFFFUL << 32U))
#define HRPWM_DMA_UPDATE_REG_ALL_U2_8               ((uint64_t)0xFFFFFFF3UL | ((uint64_t)0x0000FFFFUL << 32U))
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Event      HRPWM EMB event definition
 * @{
 */
#define HRPWM_EMB_EVT_SYS               (HRPWM_EMB_CTL1_SYSEN)                  /*!< System error */
#define HRPWM_EMB_EVT_PWM_1             (HRPWM_EMB_CTL1_PWMSEN0)                /*!< HPWM1_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_2             (HRPWM_EMB_CTL1_PWMSEN1)                /*!< HPWM2_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_3             (HRPWM_EMB_CTL1_PWMSEN2)                /*!< HPWM3_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_4             (HRPWM_EMB_CTL1_PWMSEN3)                /*!< HPWM4_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_5             (HRPWM_EMB_CTL1_PWMSEN4)                /*!< HPWM5_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_6             (HRPWM_EMB_CTL1_PWMSEN5)                /*!< HPWM6_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_7             (HRPWM_EMB_CTL1_PWMSEN6)                /*!< HPWM7_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_PWM_8             (HRPWM_EMB_CTL1_PWMSEN7)                /*!< HPWM8_PWMA/B  short-circuit output */
#define HRPWM_EMB_EVT_EMB_IN1           (HRPWM_EMB_CTL1_EMBINEN1)               /*!< EMB input 1 input */
#define HRPWM_EMB_EVT_EMB_IN2           (HRPWM_EMB_CTL1_EMBINEN2)               /*!< EMB input 2 input */
#define HRPWM_EMB_EVT_EMB_IN3           (HRPWM_EMB_CTL1_EMBINEN3)               /*!< EMB input 3 input */
#define HRPWM_EMB_EVT_EMB_IN4           (HRPWM_EMB_CTL1_EMBINEN4)               /*!< EMB input 4 input */
#define HRPWM_EMB_EVT_EMB_IN5           (HRPWM_EMB_CTL1_EMBINEN5)               /*!< EMB input 5 input */
#define HRPWM_EMB_EVT_EMB_IN6           (HRPWM_EMB_CTL1_EMBINEN6)               /*!< EMB input 6 input */
#define HRPWM_EMB_EVT_EMB_IN7           (HRPWM_EMB_CTL1_EMBINEN7)               /*!< EMB input 7 input */
#define HRPWM_EMB_EVT_EMB_IN8           (HRPWM_EMB_CTL1_EMBINEN8)               /*!< EMB input 8 input */
#define HRPWM_EMB_EVT_SYS_XTAL_STOP     (HRPWM_EMB_CTL1_SYSFAULTEN1)            /*!< System error : xtal stop */
#define HRPWM_EMB_EVT_SYS_SRAM_ECC_ERR  (HRPWM_EMB_CTL1_SYSFAULTEN2)            /*!< System error : sram ECC error */
#define HRPWM_EMB_EVT_SYS_FLASH_ECC_ERR (HRPWM_EMB_CTL1_SYSFAULTEN3)            /*!< System error : flash ECC error */
#define HRPWM_EMB_EVT_SYS_LOCK_UP       (HRPWM_EMB_CTL1_SYSFAULTEN4)            /*!< System error : lockup */
#define HRPWM_EMB_EVT_SYS_PVD           (HRPWM_EMB_CTL1_SYSFAULTEN5)            /*!< System error : PVD */
#define HRPWM_EMB_EVT_ALL               (0xF8FF1FE2UL)                          /*!< All events */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_level_Define      HRPWM EMB level definition
 * @{
 */
#define HRPWM_EMB_LVL_HIGH              (0x00UL)    /*!< High level is valid */
#define HRPWM_EMB_LVL_LOW               (0x01UL)    /*!< Low level is valid */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_NF_Count_Define      HRPWM EMB noise filter count definition
 * @{
 */
#define HRPWM_EMB_NF_CNT_3              (0x00UL)    /*!< Noise filter count is 3 */
#define HRPWM_EMB_NF_CNT_2              (0x01UL)    /*!< Noise filter count is 2 */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Flag_State      HRPWM EMB flag state definition
 * @{
 */
#define HRPWM_EMB_FLAG_PWM              (HRPWM_EMB_STAT_PWMSF)                  /*!< HRPWM PWM out put in-phase */
#define HRPWM_EMB_FLAG_SYS_ERR          (HRPWM_EMB_STAT_SYSF)                   /*!< System error occurs */
#define HRPWM_EMB_FLAG_EMB_IN1          (HRPWM_EMB_STAT_EMBINF1)                /*!< EMB input 1 valid level */
#define HRPWM_EMB_FLAG_EMB_IN2          (HRPWM_EMB_STAT_EMBINF2)                /*!< EMB input 2 valid level */
#define HRPWM_EMB_FLAG_EMB_IN3          (HRPWM_EMB_STAT_EMBINF3)                /*!< EMB input 3 valid level */
#define HRPWM_EMB_FLAG_EMB_IN4          (HRPWM_EMB_STAT_EMBINF4)                /*!< EMB input 4 valid level */
#define HRPWM_EMB_FLAG_EMB_IN5          (HRPWM_EMB_STAT_EMBINF5)                /*!< EMB input 5 valid level */
#define HRPWM_EMB_FLAG_EMB_IN6          (HRPWM_EMB_STAT_EMBINF6)                /*!< EMB input 6 valid level */
#define HRPWM_EMB_FLAG_EMB_IN7          (HRPWM_EMB_STAT_EMBINF7)                /*!< EMB input 7 valid level */
#define HRPWM_EMB_FLAG_EMB_IN8          (HRPWM_EMB_STAT_EMBINF8)                /*!< EMB input 8 valid level */
#define HRPWM_EMB_FLAG_XTAL_STOP        (HRPWM_EMB_STAT_SYSFAULTF1)             /*!< System crystal stop    */
#define HRPWM_EMB_FLAG_SRAM_ECC_ERR     (HRPWM_EMB_STAT_SYSFAULTF2)             /*!< SRAM ECC error occurs */
#define HRPWM_EMB_FLAG_FLASH_ECC_ERR    (HRPWM_EMB_STAT_SYSFAULTF3)             /*!< FLASH ECC error occurs */
#define HRPWM_EMB_FLAG_LOCK_UP          (HRPWM_EMB_STAT_SYSFAULTF4)             /*!< Lock-up error occurs */
#define HRPWM_EMB_FLAG_PVD              (HRPWM_EMB_STAT_SYSFAULTF5)             /*!< Power voltage detection error occurs */

#define HRPWM_EMB_STAT_PWM              (HRPWM_EMB_STAT_PWMST)                  /*!< HRPWM PWM out put in-phase */
#define HRPWM_EMB_STAT_SYS_ERR          (HRPWM_EMB_STAT_SYSST)                  /*!< System error occurs */
#define HRPWM_EMB_STAT_EMB_IN1          (HRPWM_EMB_STAT_EMBINST1)               /*!< EMB input 1 valid level */
#define HRPWM_EMB_STAT_EMB_IN2          (HRPWM_EMB_STAT_EMBINST2)               /*!< EMB input 2 valid level */
#define HRPWM_EMB_STAT_EMB_IN3          (HRPWM_EMB_STAT_EMBINST3)               /*!< EMB input 3 valid level */
#define HRPWM_EMB_STAT_EMB_IN4          (HRPWM_EMB_STAT_EMBINST4)               /*!< EMB input 4 valid level */
#define HRPWM_EMB_STAT_EMB_IN5          (HRPWM_EMB_STAT_EMBINST5)               /*!< EMB input 5 valid level */
#define HRPWM_EMB_STAT_EMB_IN6          (HRPWM_EMB_STAT_EMBINST6)               /*!< EMB input 6 valid level */
#define HRPWM_EMB_STAT_EMB_IN7          (HRPWM_EMB_STAT_EMBINST7)               /*!< EMB input 7 valid level */
#define HRPWM_EMB_STAT_EMB_IN8          (HRPWM_EMB_STAT_EMBINST8)               /*!< EMB input 8 valid level */

#define HRPWM_EMB_FLAG_ALL              (0xF9FEFF1FUL)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Flag_Clear      HRPWM EMB flag celar definition
 * @{
 */
#define HRPWM_EMB_FLAG_CLR_PWM              (HRPWM_EMB_STATCLR_PWMSFCLR)        /*!< HRPWM PWM out put in-phase */
#define HRPWM_EMB_FLAG_CLR_SYS_ERR          (HRPWM_EMB_STATCLR_SYSFCLR)         /*!< System error occurs */
#define HRPWM_EMB_FLAG_CLR_EMB_IN1          (HRPWM_EMB_STATCLR_EMBINFCLR1)      /*!< EMB input 1 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN2          (HRPWM_EMB_STATCLR_EMBINFCLR2)      /*!< EMB input 2 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN3          (HRPWM_EMB_STATCLR_EMBINFCLR3)      /*!< EMB input 3 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN4          (HRPWM_EMB_STATCLR_EMBINFCLR4)      /*!< EMB input 4 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN5          (HRPWM_EMB_STATCLR_EMBINFCLR5)      /*!< EMB input 5 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN6          (HRPWM_EMB_STATCLR_EMBINFCLR6)      /*!< EMB input 6 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN7          (HRPWM_EMB_STATCLR_EMBINFCLR7)      /*!< EMB input 7 valid level */
#define HRPWM_EMB_FLAG_CLR_EMB_IN8          (HRPWM_EMB_STATCLR_EMBINFCLR8)      /*!< EMB input 8 valid level */
#define HRPWM_EMB_FLAG_CLR_XTAL_STOP        (HRPWM_EMB_STATCLR_SYSFAULTF1CLR)   /*!< System crystal stop    */
#define HRPWM_EMB_FLAG_CLR_SRAM_ECC_ERR     (HRPWM_EMB_STATCLR_SYSFAULTF2CLR)   /*!< SRAM ECC error occurs */
#define HRPWM_EMB_FLAG_CLR_FLASH_ECC_ERR    (HRPWM_EMB_STATCLR_SYSFAULTF3CLR)   /*!< FLASH ECC error occurs */
#define HRPWM_EMB_FLAG_CLR_LOCK_UP          (HRPWM_EMB_STATCLR_SYSFAULTF4CLR)   /*!< Lock-up error occurs */
#define HRPWM_EMB_FLAG_CLR_PVD              (HRPWM_EMB_STATCLR_SYSFAULTF5CLR)   /*!< Power voltage detection error occurs */
#define HRPWM_EMB_FLAG_CLR_ALL              (HRPWM_EMB_FLAG_CLR_PWM           | HRPWM_EMB_FLAG_CLR_SYS_ERR | \
                                             HRPWM_EMB_FLAG_CLR_EMB_IN1       | HRPWM_EMB_FLAG_CLR_EMB_IN2 | \
                                             HRPWM_EMB_FLAG_CLR_EMB_IN3       | HRPWM_EMB_FLAG_CLR_EMB_IN4 | \
                                             HRPWM_EMB_FLAG_CLR_EMB_IN5       | HRPWM_EMB_FLAG_CLR_EMB_IN6 | \
                                             HRPWM_EMB_FLAG_CLR_EMB_IN7       | HRPWM_EMB_FLAG_CLR_EMB_IN8 | \
                                             HRPWM_EMB_FLAG_CLR_XTAL_STOP     | HRPWM_EMB_FLAG_CLR_SRAM_ECC_ERR | \
                                             HRPWM_EMB_FLAG_CLR_FLASH_ECC_ERR | HRPWM_EMB_FLAG_CLR_LOCK_UP | \
                                             HRPWM_EMB_FLAG_CLR_PVD)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_INT      HRPWM EMB interrupt definition
 * @{
 */
#define HRPWM_EMB_INT_PWM               (HRPWM_EMB_INTEN_PWMSINTEN)             /*!< HRPWM PWM out put in-phase interrupt */
#define HRPWM_EMB_INT_SYS_ERR           (HRPWM_EMB_INTEN_SYSINTEN)              /*!< System error occurs interrupt */
#define HRPWM_EMB_INT_EMB_IN1           (HRPWM_EMB_INTEN_EMBININTEN1)           /*!< EMB input1 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN2           (HRPWM_EMB_INTEN_EMBININTEN2)           /*!< EMB input2 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN3           (HRPWM_EMB_INTEN_EMBININTEN3)           /*!< EMB input3 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN4           (HRPWM_EMB_INTEN_EMBININTEN4)           /*!< EMB input4 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN5           (HRPWM_EMB_INTEN_EMBININTEN5)           /*!< EMB input5 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN6           (HRPWM_EMB_INTEN_EMBININTEN6)           /*!< EMB input6 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN7           (HRPWM_EMB_INTEN_EMBININTEN7)           /*!< EMB input7 valid level interrupt */
#define HRPWM_EMB_INT_EMB_IN8           (HRPWM_EMB_INTEN_EMBININTEN8)           /*!< EMB input8 valid level interrupt */
#define HRPWM_EMB_INT_ALL               (HRPWM_EMB_INT_PWM     | HRPWM_EMB_INT_SYS_ERR | HRPWM_EMB_INT_EMB_IN1 | \
                                         HRPWM_EMB_INT_EMB_IN2 | HRPWM_EMB_INT_EMB_IN3 | HRPWM_EMB_INT_EMB_IN4 | \
                                         HRPWM_EMB_INT_EMB_IN5 | HRPWM_EMB_INT_EMB_IN6 | HRPWM_EMB_INT_EMB_IN7 | \
                                         HRPWM_EMB_INT_EMB_IN8)
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Release_PWM_Condition  HRPWM EMB Release PWM Condition
 * @{
 */
#define HRPWM_EMB_RELEASE_PWM_COND_FLAG_ZERO      (0UL)                         /*!< EMB release PWM condition flag is reset */
#define HRPWM_EMB_RELEASE_PWM_COND_STAT_ZERO      (1UL)                         /*!< EMB release PWM condition status is invalid */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_NF_Clock_Division      HRPWM EMB Noise Filter Clock Division
 * @{
 */
#define HRPWM_EMB_NF_CLK_DIV1           (0UL)               /*!< No clock division for noise filter   */
#define HRPWM_EMB_NF_CLK_DIV2           (4UL)               /*!< Clock division by 2 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV4           (5UL)               /*!< Clock division by 4 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV8           (1UL)               /*!< Clock division by 8 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV16          (6UL)               /*!< Clock division by 16 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV32          (2UL)               /*!< Clock division by 32 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV64          (7UL)               /*!< Clock division by 64 for noise filter */
#define HRPWM_EMB_NF_CLK_DIV128         (3UL)               /*!< Clock division by 128 for noise filter */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_NF_Func_State      HRPWM EMB Noise Filter Function State
 * @{
 */
#define HRPWM_EMB_NF_OFF                (0UL)               /*!< Noise filter is off */
#define HRPWM_EMB_NF_ON                 (1UL)               /*!< Noise filter is on */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Input_Number_Define      HRPWM EMB Input number Selection
 * @{
 */
#define HRPWM_EMB_INPUT_1               (3UL << HRPWM_EMB_CTL6_EMBIN1S_POS)               /*!< EMB input x correspond to port x */
#define HRPWM_EMB_INPUT_2               (3UL << HRPWM_EMB_CTL6_EMBIN2S_POS)               /*!< EMB input x correspond to EMBEVT1[x] */
#define HRPWM_EMB_INPUT_3               (3UL << HRPWM_EMB_CTL6_EMBIN3S_POS)               /*!< EMB input x correspond to EMBEVT2[x] */
#define HRPWM_EMB_INPUT_4               (3UL << HRPWM_EMB_CTL6_EMBIN4S_POS)               /*!< EMB input x correspond to EMBEVT3[x] */
#define HRPWM_EMB_INPUT_5               (3UL << HRPWM_EMB_CTL6_EMBIN4S_POS)               /*!< EMB input x correspond to EMBEVT3[x] */
#define HRPWM_EMB_INPUT_6               (3UL << HRPWM_EMB_CTL6_EMBIN4S_POS)               /*!< EMB input x correspond to EMBEVT3[x] */
#define HRPWM_EMB_INPUT_7               (3UL << HRPWM_EMB_CTL6_EMBIN4S_POS)               /*!< EMB input x correspond to EMBEVT3[x] */
#define HRPWM_EMB_INPUT_8               (3UL << HRPWM_EMB_CTL6_EMBIN4S_POS)               /*!< EMB input x correspond to EMBEVT3[x] */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Input_Event_Sel          HRPWM EMB Input Event Selection
 * @{
 */
#define HRPWM_EMB_INPUT_SEL_POTT        (0UL)                                                           /*!< EMB input x correspond to port x */
#define HRPWM_EMB_INPUT_SEL_EMB_EVT1    (HRPWM_EMB_CTL6_EMBIN1S_0 | HRPWM_EMB_CTL6_EMBIN2S_0 | \
                                         HRPWM_EMB_CTL6_EMBIN3S_0 | HRPWM_EMB_CTL6_EMBIN4S_0 | \
                                         HRPWM_EMB_CTL6_EMBIN5S_0 | HRPWM_EMB_CTL6_EMBIN6S_0 | \
                                         HRPWM_EMB_CTL6_EMBIN7S_0 | HRPWM_EMB_CTL6_EMBIN8S_0)           /*!< EMB input x correspond to EMBEVT1[x] */
#define HRPWM_EMB_INPUT_SEL_EMB_EVT2    (HRPWM_EMB_CTL6_EMBIN1S_1 | HRPWM_EMB_CTL6_EMBIN2S_1 | \
                                         HRPWM_EMB_CTL6_EMBIN3S_1 | HRPWM_EMB_CTL6_EMBIN4S_1 | \
                                         HRPWM_EMB_CTL6_EMBIN5S_1 | HRPWM_EMB_CTL6_EMBIN6S_1 | \
                                         HRPWM_EMB_CTL6_EMBIN7S_1 | HRPWM_EMB_CTL6_EMBIN8S_1)           /*!< EMB input x correspond to EMBEVT2[x] */
#define HRPWM_EMB_INPUT_SEL_EMB_EVT3    (HRPWM_EMB_CTL6_EMBIN1S | HRPWM_EMB_CTL6_EMBIN2S | \
                                         HRPWM_EMB_CTL6_EMBIN3S | HRPWM_EMB_CTL6_EMBIN4S | \
                                         HRPWM_EMB_CTL6_EMBIN5S | HRPWM_EMB_CTL6_EMBIN6S | \
                                         HRPWM_EMB_CTL6_EMBIN7S | HRPWM_EMB_CTL6_EMBIN8S)              /*!< EMB input x correspond to EMBEVT3[x] */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Blank_Time_Define      HRPWM EMB Blank Time Definition
 * @{
 */
#define HRPWM_EMB_BLANK_TIME_REF        (0UL)               /*!< Blank time is the reference point to windows */
#define HRPWM_EMB_BLANK_TIME_OFFSET     (1UL)               /*!< Blank time is the offset to windows */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Accumulator_Event_Selection      HRPWM EMB Accumulator Event Selection
 * @{
 */
#define HRPWM_EMB_ACCUM_EVT_SEL_NONE    (0UL)
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN1 (1UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input1 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN2 (2UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input2 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN3 (3UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input3 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN4 (4UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input4 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN5 (5UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input5 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN6 (6UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input6 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN7 (7UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input7 as the Accumulator count event */
#define HRPWM_EMB_ACCUM_EVT_SEL_EMB_IN8 (8UL << HRPWM_EMB_CTL5_EMBASEL_POS)     /*!< Select EMB input8 as the Accumulator count event */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Accumulator_Reset_Mode      HRPWM EMB Accumulator Reset Mode
 * @{
 */
#define HRPWM_EMB_ACCUM_RST_PERIOD_POINT                    (0UL)               /*!< EMB Accumulator reset at the period point */
#define HRPWM_EMB_ACCUM_RST_PERIOD_POINT_NO_EVT             (1UL)               /*!< EMB Accumulator reset at the period point when there is no event occurs at last period */
/**
 * @}
 */

/**
 * @defgroup HRPWM_EMB_Accumulator_Edge_Define      HRPWM EMB Accumulator Edge Definition
 * @{
 */
#define HRPWM_EMB_ACCUM_EDGE_FALLING                        (0UL)               /*!< EMB Accumulator count on falling edge */
#define HRPWM_EMB_ACCUM_EDGE_RISING                         (1UL)               /*!< EMB Accumulator count on rising edge */
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
    ((x) == CM_HRPWM6)                          ||                             \
    ((x) == CM_HRPWM7)                          ||                             \
    ((x) == CM_HRPWM8))

/*! Parameter valid check for HRPWM phase function unit */
#define IS_HRPWM_PH_UNIT(x)                                                    \
(   ((x) == CM_HRPWM2)                          ||                             \
    ((x) == CM_HRPWM3)                          ||                             \
    ((x) == CM_HRPWM4)                          ||                             \
    ((x) == CM_HRPWM5)                          ||                             \
    ((x) == CM_HRPWM6)                          ||                             \
    ((x) == CM_HRPWM7)                          ||                             \
    ((x) == CM_HRPWM8))

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

/*! Parameter valid check for PWM pin status for count up software trigger */
#define IS_HRPWM_PWM_POLARITY_UP_SW_TRIG(x)                                    \
(   ((x) == HRPWM_PWM_UP_SW_TRIG_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_SW_TRIG_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_SW_TRIG_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_SW_TRIG_INVT))

/*! Parameter valid check for PWM pin status for count down software trigger */
#define IS_HRPWM_PWM_POLARITY_DOWN_SW_TRIG(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_SW_TRIG_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_SW_TRIG_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_SW_TRIG_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_SW_TRIG_INVT))

/*! Parameter valid check for timer event number in pin polarity set function */
#define IS_HRPWM_PWM_U1_TMR_EVT_NUM(x)                                         \
(   ((x) == HRPWM_PWM_U1_TMR_EVT_NUM1)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM2)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM3)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM4)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM5)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM6)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM7)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM8)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM9)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_NUM10))

/*! Parameter valid check for timer event number in pin polarity set function */
#define IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(x)                                       \
(   ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM1)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM2)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM3)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM4)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM5)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM6)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM7)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM8)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM9)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_NUM10))

/*! Parameter valid check for PWM pin status for timer event */
#define IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(x)                                    \
(   ((x) == HRPWM_PWM_U1_TMR_EVT_LOW)           ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_HIGH)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_HOLD)          ||                             \
    ((x) == HRPWM_PWM_U1_TMR_EVT_INVT))

/*! Parameter valid check for PWM pin status for timer event */
#define IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(x)                                  \
(   ((x) == HRPWM_PWM_U2_8_TMR_EVT_LOW)         ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_HIGH)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_HOLD)        ||                             \
    ((x) == HRPWM_PWM_U2_8_TMR_EVT_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 peak */
#define IS_HRPWM_PWM_POLARITY_UP_U1_PEAK(x)                                    \
(   ((x) == HRPWM_PWM_UP_U1_PEAK_LOW)           ||                             \
    ((x) == HRPWM_PWM_UP_U1_PEAK_HIGH)          ||                             \
    ((x) == HRPWM_PWM_UP_U1_PEAK_HOLD)          ||                             \
    ((x) == HRPWM_PWM_UP_U1_PEAK_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 valley */
#define IS_HRPWM_PWM_POLARITY_UP_U1_VALLEY(x)                                  \
(   ((x) == HRPWM_PWM_UP_U1_VALLEY_LOW)         ||                             \
    ((x) == HRPWM_PWM_UP_U1_VALLEY_HIGH)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_VALLEY_HOLD)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_VALLEY_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 match F */
#define IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_F(x)                                 \
(   ((x) == HRPWM_PWM_UP_U1_MATCH_F_LOW)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_F_HIGH)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_F_HOLD)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_F_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 match E */
#define IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_E(x)                                 \
(   ((x) == HRPWM_PWM_UP_U1_MATCH_E_LOW)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_E_HIGH)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_E_HOLD)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_E_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 match B */
#define IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_B(x)                                 \
(   ((x) == HRPWM_PWM_UP_U1_MATCH_B_LOW)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_B_HIGH)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_B_HOLD)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_B_INVT))

/*! Parameter valid check for PWM pin status for count up and U1 match A */
#define IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_A(x)                                 \
(   ((x) == HRPWM_PWM_UP_U1_MATCH_A_LOW)        ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_A_HIGH)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_A_HOLD)       ||                             \
    ((x) == HRPWM_PWM_UP_U1_MATCH_A_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 peak */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_PEAK(x)                                  \
(   ((x) == HRPWM_PWM_DOWN_U1_PEAK_LOW)         ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_PEAK_HIGH)        ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_PEAK_HOLD)        ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_PEAK_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 valley */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_VALLEY(x)                                \
(   ((x) == HRPWM_PWM_DOWN_U1_VALLEY_LOW)       ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_VALLEY_HIGH)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_VALLEY_HOLD)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_VALLEY_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 match F */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_F(x)                               \
(   ((x) == HRPWM_PWM_DOWN_U1_MATCH_F_LOW)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_F_HIGH)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_F_HOLD)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_F_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 match E */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_E(x)                               \
(   ((x) == HRPWM_PWM_DOWN_U1_MATCH_E_LOW)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_E_HIGH)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_E_HOLD)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_E_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 match B */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_B(x)                               \
(   ((x) == HRPWM_PWM_DOWN_U1_MATCH_B_LOW)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_B_HIGH)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_B_HOLD)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_B_INVT))

/*! Parameter valid check for PWM pin status for count down and U1 match A */
#define IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_A(x)                                \
(   ((x) == HRPWM_PWM_DOWN_U1_MATCH_A_LOW)      ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_A_HIGH)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_A_HOLD)     ||                             \
    ((x) == HRPWM_PWM_DOWN_U1_MATCH_A_INVT))

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

/*! Parameter valid check for HRPWM unit buffer flag source */
#define IS_HRPWM_U1_UNIT_BUF_FLAG_SRC(x)                                       \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_UNIT_BUF_SRC_U1_ALL) == HRPWM_UNIT_BUF_SRC_U1_ALL))

#define IS_HRPWM_U2_8_UNIT_BUF_FLAG_SRC(x)                                     \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_UNIT_BUF_SRC_U2_8_ALL) == HRPWM_UNIT_BUF_SRC_U2_8_ALL))

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

/*!< Parameter valid check for HRPWM dead time mode */
#define IS_HRPWM_DEADTIME_MD(x)                                                \
(   ((x) == HRPWM_DEADTIME_MD_OVERLAP_INVD)     ||                             \
    ((x) == HRPWM_DEADTIME_MD_OVERLAP_RISING)   ||                             \
    ((x) == HRPWM_DEADTIME_MD_OVERLAP_FALLING)  ||                             \
    ((x) == HRPWM_DEADTIME_MD_OVERLAP_BOTH))

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
    ((x) == HRPWM_EMB_EVT_CH5)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH6)                  ||                             \
    ((x) == HRPWM_EMB_EVT_CH7))

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

/*! Parameter valid check for hardware start condition 1  */
#define IS_HRPWM_HW_START_COND_1(x)                                            \
(   ((x) != 0UL)                                &&                             \
    ((((uint32_t)(x) | HRPWM_HW_START_COND_1_ALL) == HRPWM_HW_START_COND_1_ALL) || \
    ((((x) >> 32U) <= 0x05U) && (((x) & HRPWM_HSTAR1_HSTA11) == HRPWM_HSTAR1_HSTA11))))

/*! Parameter valid check for hardware start condition 2  */
#define IS_HRPWM_HW_START_COND_2(x)                                            \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_START_COND_2_ALL) == HRPWM_HW_START_COND_2_ALL)

/*! Parameter valid check for hardware clear condition 1  */
#define IS_HRPWM_HW_CLR_COND_1(x)                                              \
(   ((x) != 0UL)                                &&                             \
    ((((uint32_t)(x) | HRPWM_HW_CLR_COND_1_ALL) == HRPWM_HW_CLR_COND_1_ALL) || \
    ((((x) >> 32U) <= 0x05U) && (((x) & HRPWM_HCLRR1_HCLE11) == HRPWM_HCLRR1_HCLE11))))

/*! Parameter valid check for hardware clear condition 2  */
#define IS_HRPWM_HW_CLR_COND_2(x)                                              \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_CLR_COND_2_ALL) == HRPWM_HW_CLR_COND_2_ALL)

/*! Parameter valid check for hardware capture condition 1  */
#define IS_HRPWM_HW_CAPT_COND_1(x)                                             \
(   ((x) != 0UL)                                &&                             \
    ((((uint32_t)(x) | HRPWM_HW_CAPT_COND_1_ALL) == HRPWM_HW_CAPT_COND_1_ALL) || \
    ((((x) >> 32U) <= 0x05U) && (((x) & HRPWM_HCPAR1_HCPA11) == HRPWM_HCPAR1_HCPA11))))

/*! Parameter valid check for hardware capture condition 2  */
#define IS_HRPWM_HW_CAPT_COND_2(x)                                             \
(   ((x) != 0UL)                                &&                             \
    ((x) | HRPWM_HW_CAPT_COND_2_ALL) == HRPWM_HW_CAPT_COND_2_ALL)

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

/*! Parameter valid check for External event valid polarity  */
#define IS_HRPWM_EVT_VALID_LVL(x)                                              \
(   ((x) == HRPWM_EVT_VALID_LVL_HIGH)           ||                             \
    ((x) == HRPWM_EVT_VALID_LVL_LOW))

/*! Parameter valid check for External event fast asynchronous mode  */
#define IS_HRPWM_EVT_FAST_ASYNC_MD(x)                                          \
(   ((x) == HRPWM_EVT_FAST_ASYNC_OFF)           ||                             \
    ((x) == HRPWM_EVT_FAST_ASYNC_ON))

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
    ((x) == HRPWM_EVT_FILTER_MD_BLK_1)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_2)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_3)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_4)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_5)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_6)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_7)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_8)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_9)          ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_10)         ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_11)         ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_12)         ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_BLK_13)         ||                             \
    ((x) == HRPWM_EVT_FILTER_MD_WIN_OWN)        ||                             \
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
    ((x) == HRPWM_BM_CNT_SRC_PCLK)              ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U7)   ||                             \
    ((x) == HRPWM_BM_CNT_SRC_PEROID_POINT_U8))

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
#define IS_HRPWM_PH_MATCH_IDX(x)                ((x) <= HRPWM_PH_MATCH_IDX7)

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
(   (x) <= HRPWM_SYNC_SRC_U8_MATCH_SPECIAL_B)

/*! Parameter valid check for synchronous output match B direction */
#define IS_HRPWM_SYNC_MATCH_B_DIR(x)                                           \
(   ((x) == HRPWM_SYNC_MATCH_B_DIR_DOWN)        ||                             \
    ((x) == HRPWM_SYNC_MATCH_B_DIR_UP))

/*! Parameter valid check for data register data range */
#define IS_HRPWM_DATA_REG_RANGE(x)              ((x) <= 0x3FFFFFUL)

/*! Parameter valid check for data register data range 1 */
#define IS_HRPWM_DATA_REG_RANGE1(x)                                            \
(   ((x) <= HRPWM_REG_VALUE_MAX)                &&                             \
    ((x) > HRPWM_REG_VALUE_MIN))

/*! Parameter valid check for phase data register data range */
#define IS_HRPWM_PH_DATA_REG_RANGE(x)           ((x) <= 0x7FFFC0UL)

/*! Parameter valid check for BMPERAR register data range */
#define IS_HRPWM_BM_CNT_REG_RANGE(x)            ((x) <= HRPWM_COMMON_BMPERAR_BMPERAR)

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

/*! Parameter valid check for DAC ramp generator reset source */
#define IS_HRPWM_DAC_RAMP_RST_SRC(x)                                           \
(   ((x) == HRPWM_DAC_RAMP_RST_SRC_VALLEY)          ||                         \
    ((x) == HRPWM_DAC_RAMP_RST_SRC_PEAK)            ||                         \
    ((x) == HRPWM_DAC_RAMP_RST_SRC_PEAK_OR_VALLEY)  ||                         \
    ((x) == HRPWM_DAC_RAMP_RST_SRC_PWMA_RISING))

/*! Parameter valid check for DAC ramp generator step source */
#define IS_HRPWM_DAC_RAMP_STEP_SRC(x)                                          \
(   ((x) == HRPWM_DAC_RAMP_STEP_SRC_MATCH_B)    ||                             \
    ((x) == HRPWM_DAC_RAMP_STEP_SRC_PWMA_FALLING))

/*!< Parameter valid check for HRPWM Interleaving Mode */
#define IS_HRPWM_INTLV_MD(x)                                                   \
(   ((x) == HRPWM_INTLV_MD_INVD)                ||                             \
    ((x) == HRPWM_INTLV_MD_3_PHASE)             ||                             \
    ((x) == HRPWM_INTLV_MD_4_PHASE))

/*!< Parameter valid check for HRPWM1 Interleaving Mode */
#define IS_HRPWM_U1_INTLV_MD(x)                                                \
(   ((x) == HRPWM_U1_INTLV_MD_INVD)             ||                             \
    ((x) == HRPWM_U1_INTLV_MD_4_PHASE)          ||                             \
    ((x) == HRPWM_U1_INTLV_MD_3_PHASE)          ||                             \
    ((x) == HRPWM_U1_INTLV_MD_4_PHASE))

/*!< Parameter valid check for HRPWM compare A delay mode */
#define IS_HRPWM_CMPA_DLY_MD(x)                                                \
(   ((x) == HRPWM_CMPA_DLY_MD_OFF)              ||                             \
    ((x) == HRPWM_CMPA_DLY_MD_CAPTB)            ||                             \
    ((x) == HRPWM_CMPA_DLY_MD_CAPTB_OR_CMPF)    ||                             \
    ((x) == HRPWM_CMPA_DLY_MD_CAPTB_OR_CMPB))

/*!< Parameter valid check for HRPWM compare E auto delay mode */
#define IS_HRPWM_CMPE_DLY_MD(x)                                                \
(   ((x) == HRPWM_CMPE_DLY_MD_OFF)              ||                             \
    ((x) == HRPWM_CMPE_DLY_MD_CAPTA)            ||                             \
    ((x) == HRPWM_CMPE_DLY_MD_CAPTA_OR_CMPF)    ||                             \
    ((x) == HRPWM_CMPE_DLY_MD_CAPTA_OR_CMPB))

/*!< Parameter valid check for HRPWM event count mode */
#define IS_HRPWM_EVT_CNT_MD(x)                                                 \
(   ((x) == HRPWM_EVT_CNT_MD_NO_ACCUM)          ||                             \
    ((x) == HRPWM_EVT_CNT_MD_ACCUM))

/*!< Parameter valid check for HRPWM event count threshold */
#define IS_HRPWM_EVT_CNT_THRESHOLD(x)           ((x) <= (HRPWM_EEFLTCR3_EEVACNT >> HRPWM_EEFLTCR3_EEVACNT_POS))

/*!< Parameter valid check for HRPWM AOS event */
#define IS_HRPWM_AOS_EVT(x)                                                    \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_AOS_EVT_MSK) == HRPWM_AOS_EVT_MSK))

/*!< Parameter valid check for HRPWM global transfer buffer condition */
#define IS_HRPWM_GLOBAL_BUF_COND(x)                                            \
(   ((x) == HRPWM_GLOBAL_BUF_COND_BUF_EVT)              ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_DMA_CPLT)             ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_AFTER_DMA_BUF_EVT)    ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_UPD_IN3)              ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_UPD_IN2)              ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_UPD_IN1)              ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN3_EVT)    ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN2_EVT)    ||                     \
    ((x) == HRPWM_GLOBAL_BUF_COND_AFTER_UPD_IN1_EVT))

/*!< Parameter valid check for HRPWM global buffer synchronous mode */
#define IS_HRPWM_GLOBAL_BUF_SYNC_MD(x)                                         \
(   ((x) == HRPWM_GLOBAL_BUF_SYNC_MD_IMMED)     ||                             \
    ((x) == HRPWM_GLOBAL_BUF_SYNC_MD_VALLEY)    ||                             \
    ((x) == HRPWM_GLOBAL_BUF_SYNC_MD_PEAK)      ||                             \
    ((x) == HRPWM_GLOBAL_BUF_SYNC_MD_PEAK_OR_VALLEY))

/*!< Parameter valid check for HRPWM software buffer burst mode */
#define IS_HRPWM_SW_BUF_BURST_MD(x)                                            \
(   ((x) == HRPWM_SW_BUF_BURST_MD_CNT_STOP)     ||                             \
    ((x) == HRPWM_SW_BUF_BURST_MD_CNT_STOP_OR_CNT))

/*!< Parameter valid check for HRPWM link unit */
#define IS_HRPWM_LINK_UNIT(x)                   ((x) <= HRPWM_LINK_UNIT_U8)

/*!< Parameter valid check for HRPWM trigger calculate mode */
#define IS_HRPWM_TRIG_CALC_MD(x)                                               \
(   ((x) == HRPWM_TRIG_CALC_MD_ADD)                                 ||         \
    ((x) == HRPWM_TRIG_CALC_MD_SUB)                                 ||         \
    ((x) == HRPWM_TRIG_CALC_MD_DIV2)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_DIV4)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_DIV8)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_DIV16)                               ||         \
    ((x) == HRPWM_TRIG_CALC_MD_MUL2)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_MUL4)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_MUL8)                                ||         \
    ((x) == HRPWM_TRIG_CALC_MD_MUL16)                               ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_DIV2))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_DIV4))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_DIV8))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_DIV16))    ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_MUL2))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_MUL4))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_MUL8))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_ADD | HRPWM_TRIG_CALC_MD_MUL16))    ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_DIV2))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_DIV4))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_DIV8))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_DIV16))    ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_MUL2))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_MUL4))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_MUL8))     ||         \
    ((x) == (HRPWM_TRIG_CALC_MD_SUB | HRPWM_TRIG_CALC_MD_MUL16)))

/*!< Parameter valid check for HRPWM trigger output source */
#define IS_HRPWM_DE_TRIP_OUT_SRC(x)                                            \
(   ((x) != 0U)                                 ||                             \
    (((x) | HRPWM_DE_TRIP_OUT_SRC_MASK) == HRPWM_DE_TRIP_OUT_SRC_MASK))

/*!< Parameter valid check for HRPWM DE active flag mode */
#define IS_HRPWM_DE_ACT_FLAG_MD(x)                                             \
(   ((x) == HRPWM_DE_ACT_FLAG_MD_CBC)           ||                             \
    ((x) == HRPWM_DE_ACT_FLAG_MD_ONE_SHOT))

/*!< Parameter valid check for HRPWM DE trigger source */
#define IS_HRPWM_DE_TRIP_SRC(x)                                                \
(   ((x) <= HRPWM_DE_TRIP_SRC_HEMBEVT1_7)       ||                             \
    (((x) >= HRPWM_DE_TRIP_SRC_PLA0OUT)         &&                             \
    ((x) <= HRPWM_DE_TRIP_SRC_SDFM_ZCD_3)))

/*!< Parameter valid check for HRPWM DE PWM level mode */
#define IS_HRPWM_DE_PWM_LEVEL_MD(x)                                            \
(   ((x) == HRPWM_DE_PWM_LEVEL_TRIPSEL)         ||                             \
    ((x) == HRPWM_DE_PWM_LEVEL_INVT_TRIPSEL)    ||                             \
    ((x) == HRPWM_DE_PWM_LEVEL_LOW)             ||                             \
    ((x) == HRPWM_DE_PWM_LEVEL_HIGH))

/*!< Parameter valid check for HRPWM DE TRIP select */
#define IS_HRPWM_DE_TRIP_SEL(x)                                                \
(   ((x) == HRPWM_DE_PWM_TRIP_SEL_TRIPH)        ||                             \
    ((x) == HRPWM_DE_PWM_TRIP_SEL_TRIPL))

/*!< Parameter valid check for HRPWM DE monitor threshold */
#define IS_HRPWM_DE_MON_THRESHOLD(x)            (((x) <= HRPWM_DEMONRHRES_THRESHOLD))

/*!< Parameter valid check for HRPWM minimum dead time block signal selection */
#define IS_HRPWM_MINDB_PWM_BLK_SEL(x)                                          \
(   ((x) == HRPWM_MINDB_PWM_BLK_SEL_A)          ||                             \
    ((x) == HRPWM_MINDB_PWM_BLK_SEL_B))

/*!< Parameter valid check for HRPWM minimum dead time reference signal invert definition */
#define IS_HRPWM_MINDB_REF_INVT(x)                                             \
(   ((x) == HRPWM_MINDB_REF_INVT_OFF)           ||                             \
    ((x) == HRPWM_MINDB_REF_INVT_ON))

/*!< Parameter valid check for HRPWM minimum dead time block signal invert definition */
#define IS_HRPWM_MINDB_BLK_INVT(x)                                             \
(   ((x) == HRPWM_MINDB_BLK_INVT_OFF)           ||                             \
    ((x) == HRPWM_MINDB_BLK_INVT_ON))

/*!< Parameter valid check for HRPWM minimum dead time reference signal selection */
#define IS_HRPWM_MINDB_REF_SEL(x)               ((x) <= HRPWM_MINDB_REF_SEL_PWM_DE_NO_HR)

/*!< Parameter valid check for HRPWM LUT input 3 source */
#define IS_HRPWM_LUT_INPUT_3_SRC(x)             ((x) <= HRPWM_LUT_INT_SRC_ICLEVT_16)

/*!< Parameter valid check for HRPWM global AOS 1/2/3 event selection */
#define IS_HRPWM_GLAOS_1_2_3_EVT_SEL(x)                                        \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_GLAOS_1_2_3_EVT_ALL) == HRPWM_GLAOS_1_2_3_EVT_ALL))

/*!< Parameter valid check for HRPWM global AOS 4/5/6 event selection */
#define IS_HRPWM_GLAOS_4_5_6_EVT_SEL(x)                                        \
(   ((x) != 0UL)                                &&                             \
    (((x) | HRPWM_GLAOS_4_5_6_EVT_ALL) == HRPWM_GLAOS_4_5_6_EVT_ALL))

/*!< Parameter valid check for HRPWM global AOS 7/8 event selection */
#define IS_HRPWM_GLAOS_7_8_EVT_SEL(x)           ((x) <= HRPWM_GLAOS_7_8_EVT_EXTEVT_8)

/*!< Parameter valid check for HRPWM global AOS 9/10 event selection */
#define IS_HRPWM_GLAOS_9_10_EVT_SEL(x)                                         \
(   ((x) <= HRPWM_GLAOS_9_10_EVT_U8_VALLEY)     ||                             \
    (((x) >= HRPWM_GLAOS_9_10_EVT_EXTEVT_3) &&  ((x) <= HRPWM_GLAOS_9_10_EVT_EXTEVT_10)))

/*!< Parameter valid check for HRPWM global AOS event numer */
#define IS_HRPWM_GLAOS_EVT_NUM(x)             ((x) <= HRPWM_GLAOS_EVT_10)

/*!< Parameter valid check for HRPWM global AOS event buf */
#define IS_HRPWM_GLAOS_EVT_BUF(x)             ((x) <= HRPWM_GLAOS_EVT_BUF_U9)

/*!< Parameter valid check for HRPWM global AOS event divide */
#define IS_HRPWM_GLAOS_EVT_DIV(x)             ((x) <= HRPWM_COMMON_GLAOSPSCR1_PSC1)

/*!< Parameter valid check for HRPWM DMA update register */
#define IS_HRPWM_DMA_UPDATE_REG_U1(x)                                          \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_DMA_UPDATE_REG_ALL_U1) == HRPWM_DMA_UPDATE_REG_ALL_U1))

/*!< Parameter valid check for HRPWM DMA update register */
#define IS_HRPWM_DMA_UPDATE_REG_U2_8(x)                                        \
(   ((x) != 0U)                                 &&                             \
    (((x) | HRPWM_DMA_UPDATE_REG_ALL_U2_8) == HRPWM_DMA_UPDATE_REG_ALL_U2_8))

/*!< Parameter valid check for HRPWM EMB unit */
#define IS_HRPWM_EMB_UNIT(x)                                                   \
(   ((x) == CM_HRPWM_EMB1)                      ||                             \
    ((x) == CM_HRPWM_EMB2)                      ||                             \
    ((x) == CM_HRPWM_EMB3)                      ||                             \
    ((x) == CM_HRPWM_EMB4)                      ||                             \
    ((x) == CM_HRPWM_EMB5)                      ||                             \
    ((x) == CM_HRPWM_EMB6)                      ||                             \
    ((x) == CM_HRPWM_EMB7)                      ||                             \
    ((x) == CM_HRPWM_EMB8))

/*!< Parameter valid check for HRPWM EMB input number */
#define IS_HRPWM_EMB_INPUT_NUM(x)                                              \
(   ((x) == HRPWM_EMB_INPUT_1)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_2)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_3)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_4)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_5)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_6)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_7)                  ||                             \
    ((x) == HRPWM_EMB_INPUT_8))

/*!< Parameter valid check for HRPWM EMB input event selection */
#define IS_HRPWM_EMB_INPUT_EVENT_SEL(x)                                        \
(   ((x) == HRPWM_EMB_INPUT_SEL_POTT)           ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT1)       ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT2)       ||                             \
    ((x) == HRPWM_EMB_INPUT_SEL_EMB_EVT3))

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
 * @brief  HRPWM set Channel A polarity for count up software trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Sw_Trigger_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpSwTrigger(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_SW_TRIG(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR2, HRPWM_PCNAR2_SFTUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up software trigger (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Sw_Trigger_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpSwTrigger_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_SW_TRIG(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR2, HRPWM_BPCNAR2_SFTUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up software trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Sw_Trigger_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpSwTrigger(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_SW_TRIG(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR2, HRPWM_PCNBR2_SFTUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up software trigger (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_Sw_Trigger_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpSwTrigger_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_SW_TRIG(u32Polarity));

    MODIFY_REG32(HRPWMx->BPCNBR2, HRPWM_BPCNBR2_SFTUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A count up polarity for timer event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNAR14, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNAR24, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel A count up polarity for timer event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNAR14, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNAR24, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel A count down polarity for timer event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNAR15, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNAR25, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel A count down polarity for timer event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNAR15, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNAR25, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel B count up polarity for timer event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNBR14, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNBR24, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel B count up polarity for timer event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNBR14, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNBR24, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel B count down polarity for timer event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNBR15, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->PCNBR25, u32Event, u32Polarity);
    }

}

/**
 * @brief  HRPWM set Channel B count down polarity for timer event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            Timer event number @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Num_Define
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_U2_8_Pin_Polarity_Timer_Event_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    if (CM_HRPWM1 == HRPWMx) {
        DDL_ASSERT(IS_HRPWM_PWM_U1_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U1_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNBR15, u32Event, u32Polarity);
    } else {
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_TMR_EVT_NUM(u32Event));
        DDL_ASSERT(IS_HRPWM_PWM_U2_8_POLARITY_TMR_EVT(u32Polarity));
        MODIFY_REG32(HRPWMx->BPCNBR25, u32Event, u32Polarity);
    }
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1OVFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1OVFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1OVFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1OVFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1UDFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1UDFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1UDFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1UDFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match F
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1CMFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match F (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1CMFUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match F
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1CMFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match F (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1CMFUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match E
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1CMEUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match E (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1CMEUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match E
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1CMEUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match E (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1CMEUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match B
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1CMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match B (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1CMBUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match B
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1CMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match B (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1CMBUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match A
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR24, HRPWM_PCNAR24_U1CMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count up and U1 match A (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityUpU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR24, HRPWM_BPCNAR24_U1CMAUCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match A
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR24, HRPWM_PCNBR24_U1CMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count up and U1 match A (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Up_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityUpU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_UP_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR24, HRPWM_BPCNBR24_U1CMAUCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1OVFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1OVFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 count peak
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1OVFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 count peak (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Peak_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_PEAK(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1OVFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1UDFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1UDFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 count valley
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1UDFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 count valley (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Valley_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_VALLEY(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1UDFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match F
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1CMFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match F (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1CMFDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match F
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_PH_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match F (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_F_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_F(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match E
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1CMEDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match E (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1CMEDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match E
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match E (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_E_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_E(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1CMFDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match B
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1CMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match B (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1CMBDCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match B
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1CMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match B (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_B_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_B(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1CMBDCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match A
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNAR25, HRPWM_PCNAR25_U1CMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel A polarity for count down and U1 match A (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChASetPolarityDownU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNAR25, HRPWM_BPCNAR25_U1CMADCA, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match A
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->PCNBR25, HRPWM_PCNBR25_U1CMADCB, u32Polarity);
}

/**
 * @brief  HRPWM set Channel B polarity for count down and U1 match A (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Polarity         Pin polarity @ref HRPWM_Pin_Polarity_Down_U1_Match_A_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_PWM_ChBSetPolarityDownU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PWM_POLARITY_DOWN_U1_MATCH_A(u32Polarity));
    MODIFY_REG32(HRPWMx->BPCNBR25, HRPWM_BPCNBR25_U1CMADCB, u32Polarity);
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

/**+
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
 *                                  @ref HRPWM_HW_Capture_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_1(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_CAPAOSMUX, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel A capture condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_2(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCPAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel A capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_1(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
}

/**
 * @brief  Hardware channel A capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChACaptureCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_2(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCPAR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HCPAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel B capture condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_1(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPBR1, u64Cond);
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_CAPBOSMUX, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel B capture condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_2(u64Cond));
    SET_REG32_BIT(HRPWMx->HCPBR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCPBR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware channel B capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_1(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCPBR1, u64Cond);
}

/**
 * @brief  Hardware channel B capture condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware capture, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Capture_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWChBCaptureCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CAPT_COND_2(u64Cond));
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
    WRITE_REG32(CM_HRPWM_COMMON->SCAPR, u32Unit & u32Ch);
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
 * @retval None
 */
__STATIC_INLINE void HRPWM_SpecialBBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENSPB);
}

/**
 * @brief  Enable HRPWM port control buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PortBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENPCN);
}

/**
 * @brief  Disable HRPWM port control buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PortBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENPCN);
}

/**
 * @brief  Enable HRPWM valid period buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_ValidPeriodBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENVPE);
}

/**
 * @brief  Disable HRPWM valid period buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_ValidPeriodBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->BCONR1, HRPWM_BCONR1_BENVPE);
}

/**
 * @brief  Enable HRPWM global buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_GlobalBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_GBEN);
}

/**
 * @brief  Disable HRPWM global buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_GlobalBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_GBEN);
}

/**
 * @brief  HRPWM global buffer function is enabled when count stop
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CountStopBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_STPU);
}

/**
 * @brief  HRPWM global buffer function is disable when count stop
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CountStopBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_STPU);
}

/**
 * @brief  HRPWM clear GBEN bit when global buffer transfer occurs
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_GlobalBufClearEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_GBENHDCLREN);
}

/**
 * @brief  HRPWM not clear GBEN bit when global buffer transfer occurs
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_GlobalBufClearDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->GBCR, HRPWM_GBCR_GBENHDCLREN);
}

/**
 * @brief  Set HRPWM global buffer function transfer condition
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Cond             Speciefies the global buffer transfer condition @ref HRPWM_Global_Buf_Cond_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetGlobalBufCond(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_GLOBAL_BUF_COND(u32Cond));
    MODIFY_REG32(HRPWMx->GBCR, HRPWM_GBCR_UPDGAT, u32Cond);
}

/**
 * @brief  Set HRPWM global buffer function sync mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Speciefies the global buffer sync mode @ref HRPWM_Global_Buf_Sync_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetGlobalBufSyncMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_GLOBAL_BUF_SYNC_MD(u32Mode));
    MODIFY_REG32(HRPWMx->GBCR, HRPWM_GBCR_RSYNCMD, u32Mode);
}

/**
 * @brief  Set HRPWM software buffer burst mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Speciefies the software buffer burst mode @ref HRPWM_SW_Buf_Burst_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetSwBufBurstMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_SW_BUF_BURST_MD(u32Mode));
    MODIFY_REG32(HRPWMx->GBCR, HRPWM_GBCR_SBTRUSFTMD, u32Mode);
}

/**
 * @brief  HRPWM external event filter window register buffer function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
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
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeDownBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBEND);
}

/**
 * @brief  Enable dead time mode buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeModeBufEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBENSD);
}

/**
 * @brief  Disable dead time mode buffer function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeModeBufDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTBENSD);
}

/**
 * @brief  Set dead time mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Dead time mode, @ref HRPWM_DeadTime_Mode_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeSetMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DEADTIME_MD(u32Mode));
    MODIFY_REG32(HRPWMx->DCONR, HRPWM_DCONR_SDTF | HRPWM_DCONR_SDTR, u32Mode);
}

/**
 * @brief  Write protect for dead time register(HRDTDAR,HRDTDBR,HRDTUAR,HRDTUBR)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeRegWP(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTFLK | HRPWM_DCONR_DTRLK);
}

/**
 * @brief  Write enablel for dead time register(HRDTDAR,HRDTDBR,HRDTUAR,HRDTUBR)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeRegWE(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTFLK | HRPWM_DCONR_DTRLK);
}

/**
 * @brief  Write protect for dead time mode(SDTR,SDTF)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeModeWP(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTFSLK | HRPWM_DCONR_DTRSLK);
}

/**
 * @brief  Write enablel for dead time mode(SDTR,SDTF)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DeadTimeModeWE(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->DCONR, HRPWM_DCONR_DTFSLK | HRPWM_DCONR_DTRSLK);
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
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMP | HRPWM_BM_FLAG_ALL, HRPWM_BM_FLAG_ALL);
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
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BENBMCMP | HRPWM_BM_FLAG_ALL, HRPWM_BM_FLAG_ALL);
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
 * @brief  HRPWM ChA software trigger PWM output function enable
 * @param  None
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_ChASwTriggerEnable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));

    SET_REG32_BIT(CM_HRPWM_COMMON->SFTSRR, u32Unit);
}

/**
 * @brief  HRPWM ChA software trigger PWM output function disable
 * @param  None
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_ChASwTriggerDisable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));

    CLR_REG32_BIT(CM_HRPWM_COMMON->SFTSRR, u32Unit);
}

/**
 * @brief  HRPWM ChB software trigger PWM output function enable
 * @param  None
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_ChBSwTriggerEnable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));

    SET_REG32_BIT(CM_HRPWM_COMMON->SFTSRR, u32Unit << HRPWM_COMMON_SFTSRR_SFTSRB1_POS);
}

/**
 * @brief  HRPWM ChB software trigger PWM output function disable
 * @param  None
 * @param  [in] u32Unit             HRPWM unit, Can be one or any combination of the values from
 *                                  @ref HRPWM_Mul_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_ChBSwTriggerDisable(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));

    CLR_REG32_BIT(CM_HRPWM_COMMON->SFTSRR, u32Unit << HRPWM_COMMON_SFTSRR_SFTSRB1_POS);
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
 *                                  @ref HRPWM_Unit_Buf_Flag_Src_Define
 *
 * @retval An @ref en_flag_status_t enumeration type value.
 */
__STATIC_INLINE en_flag_status_t HRPWM_GetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    en_flag_status_t enStatus = RESET;
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_U2_8_UNIT_BUF_FLAG_SRC(u32Src));

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
 *                                  @ref HRPWM_Unit_Buf_Flag_Src_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetUnitGlobalBufFlagSrc(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Src)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_U2_8_UNIT_BUF_FLAG_SRC(u32Src));
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
    WRITE_REG32(CM_HRPWM_COMMON->SSTAIDLR, u32Unit & u32Ch);
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
    WRITE_REG32(CM_HRPWM_COMMON->SSTARUNR1, u32Unit & u32Ch);
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
    WRITE_REG32(CM_HRPWM_COMMON->SSTARUNR2, u32Unit & u32Ch);
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
 * @note The function valid when the software trigger source is enabled for idle delay function, which is configured by
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
 * @note The function valid when the software trigger source is enabled for idle delay function, which is configured by
 *       API HRPWM_IDLE_DELAY_SetTriggerSrc() or HRPWM_IDLE_DELAY_Init()
 */
__STATIC_INLINE void HRPWM_IDLE_DELAY_MultiUnitSWTrigger(uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_MUL_UNIT(u32Unit));
    WRITE_REG32(CM_HRPWM_COMMON->SSTADIDLR, u32Unit);
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
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_BMEN | HRPWM_BM_FLAG_ALL, HRPWM_BM_FLAG_ALL);
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
    /* To avoid clear the flag mistake */
    if (u32UnitNum >= 6U) {
        /* U7 & U8,  HRPWM_COMMON_BMCR_BMTMR7_POS - HRPWM_COMMON_BMCR_BMTMR1_POS - 6U = 8U */
        MODIFY_REG32(CM_HRPWM_COMMON->BMCR, (HRPWM_COMMON_BMCR_BMTMR7 << (u32UnitNum - 6U)) | HRPWM_BM_FLAG_ALL,
                     (u32UnitCountReset << (u32UnitNum + 8U)) | HRPWM_BM_FLAG_ALL);
    } else {
        MODIFY_REG32(CM_HRPWM_COMMON->BMCR, (HRPWM_COMMON_BMCR_BMTMR1 << u32UnitNum) | HRPWM_BM_FLAG_ALL,
                     (u32UnitCountReset << u32UnitNum) | HRPWM_BM_FLAG_ALL);
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
    MODIFY_REG32(CM_HRPWM_COMMON->BMCR, HRPWM_COMMON_BMCR_INTENBMOVF | HRPWM_BM_FLAG_ALL, HRPWM_BM_FLAG_ALL);
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
 * @brief  HRPWM idle balance function enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BAL_Enable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_BIDLE);
}

/**
 * @brief  HRPWM idle balance function disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BAL_Disable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_BIDLE);
}

/**
 * @brief  Exit HRPWM idle balance function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_IDLE_BAL_Exit(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_BIAR);
}

/**
 * @brief  Enable HRPWM Channel A chopping function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CHP_ChAEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_CHPA);
}

/**
 * @brief  Disable HRPWM Channel A chopping function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CHP_ChADisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_CHPA);
}

/**
 * @brief  Enable HRPWM Channel B chopping function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CHP_ChBEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    SET_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_CHPB);
}

/**
 * @brief  Disable HRPWM Channel B chopping function
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CHP_ChBDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    CLR_REG32_BIT(HRPWMx->IDLECR, HRPWM_IDLECR_CHPB);
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
 * @brief  HRPWM valid period set interval (buffer)
 * @param  [in] HRPWMx                  HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Interval             Period count interval, range from 0 ~ 31
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetValidPeriodInterval_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Interval)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_PERIOD_INTERVAL(u32Interval));
    MODIFY_REG32(HRPWMx->BVPERR, HRPWM_BVPERR_PCNTS, u32Interval << HRPWM_BVPERR_PCNTS_POS);
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
 *                                  @ref HRPWM_HW_Start_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND_1(u64Cond));
    SET_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_STAAOSMUX, u64Cond >> 32U);
}

/**
 * @brief  Hardware start condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware start, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Start_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND_2(u64Cond));
    SET_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HSTAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware start condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware start, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Start_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND_1(u64Cond));
    CLR_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
}

/**
 * @brief  Hardware start condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware start, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Start_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWStartCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_START_COND_2(u64Cond));
    CLR_REG32_BIT(HRPWMx->HSTAR1, u64Cond);
    CLR_REG32_BIT(HRPWMx->HSTAR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware clear condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND_1(u64Cond));
    SET_REG32_BIT(HRPWMx->HCLRR1, u64Cond);
    MODIFY_REG32(HRPWMx->GCONR, HRPWM_GCONR_CLRAOSMUX, u64Cond >> 32U);
}

/**
 * @brief  Hardware clear condition enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND_2(u64Cond));
    SET_REG32_BIT(HRPWMx->HCLRR1, u64Cond);
    SET_REG32_BIT(HRPWMx->HCLRR2, u64Cond >> 32U);
}

/**
 * @brief  Hardware clear condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_1_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND_1(u64Cond));
    CLR_REG32_BIT(HRPWMx->HCLRR1, u64Cond);
}

/**
 * @brief  Hardware clear condition disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u64Cond             Events source for hardware clear, maybe one or any combination of the parameter
 *                                  @ref HRPWM_HW_Clear_Condition_2_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_HWClearCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_HW_CLR_COND_2(u64Cond));
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
 * @brief  Set External event counter event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetCountEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));

    MODIFY_REG32(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVASEL, u32EventNum << HRPWM_EEFLTCR3_EEVASEL_POS);
}

/**
 * @brief  Set External event counter threshold
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Threshold        External event counter threshold
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetCountThreshold(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Threshold)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_CNT_THRESHOLD(u32Threshold));

    MODIFY_REG32(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVACNT, u32Threshold << HRPWM_EEFLTCR3_EEVACNT_POS);
}

/**
 * @brief  Reset External event counter value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_ResetCountValue(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVACRES);
}

/**
 * @brief  Enable External event counter
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_EnableCount(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVACE);
}

/**
 * @brief  Disable External event counter
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_DisableCount(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVACE);
}

/**
 * @brief  Set External event counter count mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             External event counter count mode @ref HRPWM_EVT_Count_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_SetCountMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_CNT_MD(u32Mode));

    MODIFY_REG32(HRPWMx->EEFLTCR3, HRPWM_EEFLTCR3_EEVARSTM, u32Mode << HRPWM_EEFLTCR3_EEVARSTM_POS);
}

/**
 * @brief  Enable External event up count hardware trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_UpHwTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));

    SET_REG32_BIT(HRPWMx->UDIRCFG, HRPWM_UDIRCFG_EEV1 << u32EventNum);
}

/**
 * @brief  Disable External event up count hardware trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_UpHwTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));

    CLR_REG32_BIT(HRPWMx->UDIRCFG, HRPWM_UDIRCFG_EEV1 << u32EventNum);
}

/**
 * @brief  Enable External event down count hardware trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_DownHwTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));

    SET_REG32_BIT(HRPWMx->DDIRCFG, HRPWM_DDIRCFG_EEV1 << u32EventNum);
}

/**
 * @brief  Disable External event down count hardware trigger
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32EventNum         External event number @ref HRPWM_EVT_Num_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_EVT_DownHwTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_EVT_NUM(u32EventNum));

    CLR_REG32_BIT(HRPWMx->DDIRCFG, HRPWM_DDIRCFG_EEV1 << u32EventNum);
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
    MODIFY_REG32(CM_HRPWM_COMMON->SYNOCR, HRPWM_COMMON_SYNOCR_SYNCMP, u32PulseWidth << HRPWM_COMMON_SYNOCR_SYNCMP_POS);
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
 * @brief  Enable HRPWM DAC ramp generator
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_RampEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_RAMPEN);
}

/**
 * @brief  Disable HRPWM DAC ramp generator
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_RampDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_RAMPEN);
}

/**
 * @brief  HRPWM DAC set ramp generator reset source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Ramp generator reset source @ref HRPWM_DAC_Ramp_Reset_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_SetRampResetSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DAC_RAMP_RST_SRC(u32Source));

    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_RAMPRS, u32Source);
}

/**
 * @brief  HRPWM DAC set ramp generator step source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Ramp generator step source @ref HRPWM_DAC_Ramp_Step_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DAC_SetRampStepSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DAC_RAMP_STEP_SRC(u32Source));

    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_RAMPSS, u32Source);
}

/**
 * @brief  HRPWM set interleaving mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Interleaving mode @ref HRPWM_Interleaving_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetIntlvMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_INTLV_MD(u32Mode));

    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_INTLVD, u32Mode);
}

/**
 * @brief  HRPWM set interleaving mode
 * @param  [in] u32Mode             Interleaving mode @ref HRPWM_U1_Interleaving_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetU1IntlvMode(uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_U1_INTLV_MD(u32Mode));

    MODIFY_REG32(CM_HRPWM1->PHSCTL, HRPWM_PHSCTL_MINTLVD, u32Mode);
}

/**
 * @brief  Enable HRPWM push-pull mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PushPullEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_PSHPLL);
}

/**
 * @brief  Disable HRPWM push-pull mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_PushPullDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_PSHPLL);
}

/**
 * @brief  Get HRPWM Push-pull status flag
 * @retval An @ref en_flag_status_t enumeration type value.
 *         - SET: PWMA_ORG1 low level.
 *         - RESET: PWMB_ORG1 low level.
 */
__STATIC_INLINE en_flag_status_t HRPWM_GetPushPullState(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    return (READ_REG32_BIT(HRPWMx->STFLR2, HRPWM_STFLR2_CPPSTAT) == 0UL) ? RESET : SET;
}

/**
 * @brief  Enable HRPWM trigger half mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_TriggerHalfEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_TRGHLF);
}

/**
 * @brief  Disable HRPWM trigger half mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_TriggerHalfDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->CR, HRPWM_CR_TRGHLF);
}

/**
 * @brief  Enable HRPWM compare B greater than or match function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CompareBGreaterThanMatchEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_GTGCMB);
}

/**
 * @brief  Disable HRPWM compare B greater than or match function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CompareBGreaterThanMatchDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->CR, HRPWM_CR_GTGCMB);
}

/**
 * @brief  Enable HRPWM compare F greater than or match function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CompareFGreaterThanMatchEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->CR, HRPWM_CR_GTGCMF);
}

/**
 * @brief  Disable HRPWM compare F greater than or match function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_CompareFGreaterThanMatchDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->CR, HRPWM_CR_GTGCMF);
}

/**
 * @brief  HRPWM set compare A delay mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Compare A delay mode @ref HRPWM_Compare_A_Delay_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareADelayMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CMPA_DLY_MD(u32Mode));

    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DELGCMA, u32Mode);
}

/**
 * @brief  HRPWM set compare E delay mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Compare E delay mode @ref HRPWM_Compare_E_Delay_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetCompareEDelayMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_CMPE_DLY_MD(u32Mode));

    MODIFY_REG32(HRPWMx->CR, HRPWM_CR_DELGCME, u32Mode);
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
 * @param  [in] u32Value            Period value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            Period value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            General compare register value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            Dead time count up value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            Dead time count up value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            Dead time count down value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            Dead time count down value, range (HRPWM_REG_VALUE_MIN, HRPWM_REG_VALUE_MAX]
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
 * @param  [in] u32Value            value range [0x00, HRPWM_REG_VALUE_MAX] for sawtooth waveform
 *                                  value range [0x00, 0x7FFFC0] for triangle waveform
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetCompareValue(uint32_t u32PhaseIndex, uint32_t u32Value)
{
    __IO uint32_t *HRPWM_PHSCMPmn;
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    DDL_ASSERT(IS_HRPWM_PH_DATA_REG_RANGE(u32Value));
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1A - 8UL * u32PhaseIndex);
    WRITE_REG32(*HRPWM_PHSCMPmn, u32Value);
}

/**
 * @brief  HRPWM set phase match compare register buffer
 * @param  [in] u32PhaseIndex       Phase match index @ref HRPWM_PH_Index_Define
 * @param  [in] u32Value            value range [0x00, HRPWM_REG_VALUE_MAX] for sawtooth waveform
 *                                  value range [0x00, 0x7FFFC0] for triangle waveform
 * @retval None
 */
__STATIC_INLINE void HRPWM_PH_SetCompareValue_Buf(uint32_t u32PhaseIndex, uint32_t u32Value)
{
    __IO uint32_t *HRPWM_PHSCMPmn;
    DDL_ASSERT(IS_HRPWM_PH_MATCH_IDX(u32PhaseIndex));
    DDL_ASSERT(IS_HRPWM_PH_DATA_REG_RANGE(u32Value));
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1B - 8UL * u32PhaseIndex);
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
    HRPWM_PHSCMPmn = (uint32_t *)((uint32_t)&CM_HRPWM1->PHSCMP1A - 8UL * u32PhaseIndex);
    return READ_REG32(*HRPWM_PHSCMPmn);
}

/**
 * @brief  Enable HRPWM aos event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            AOS event to enable @ref HRPWM_AOS_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_AosEventEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_AOS_EVT(u32Event));

    SET_REG32_BIT(HRPWMx->AOSEOUTSELR, u32Event);
}

/**
 * @brief  Disable HRPWM aos event
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            AOS event to disable @ref HRPWM_AOS_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_AosEventDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_AOS_EVT(u32Event));

    CLR_REG32_BIT(HRPWMx->AOSEOUTSELR, u32Event);
}

/**
 * @brief  Enable HRPWM aos event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            AOS event to enable @ref HRPWM_AOS_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_AosEventEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_AOS_EVT(u32Event));

    SET_REG32_BIT(HRPWMx->BAOSEOUTSELR, u32Event);
}

/**
 * @brief  Disable HRPWM aos event (buffer)
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Event            AOS event to disable @ref HRPWM_AOS_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_AosEventDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_AOS_EVT(u32Event));

    CLR_REG32_BIT(HRPWMx->BAOSEOUTSELR, u32Event);
}

/**
 * @brief  HRPWM set period link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetPeriodUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_PRDLINK, u32Unit << HRPWM_LINKCR1_PRDLINK_POS);
}

/**
 * @brief  HRPWM set compare value A link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetCompareAValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_GCMALINK, u32Unit << HRPWM_LINKCR1_GCMALINK_POS);
}

/**
 * @brief  HRPWM set compare value B link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetCompareBValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_GCMBLINK, u32Unit << HRPWM_LINKCR1_GCMBLINK_POS);
}

/**
 * @brief  HRPWM set compare value E link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetCompareEValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_GCMELINK, u32Unit << HRPWM_LINKCR1_GCMELINK_POS);
}

/**
 * @brief  HRPWM set compare value F link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetCompareFValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_GCMFLINK, u32Unit << HRPWM_LINKCR1_GCMFLINK_POS);
}

/**
 * @brief  HRPWM set special compare value A link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetSpecialCompareAValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_SCMALINK, u32Unit << HRPWM_LINKCR1_SCMALINK_POS);
}

/**
 * @brief  HRPWM set special compare value B link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetSpecialCompareBValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_SCMBLINK, u32Unit << HRPWM_LINKCR1_SCMBLINK_POS);
}

/**
 * @brief  HRPWM set external event filter offset value & window value link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetExternalEventFilterUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR1, HRPWM_LINKCR1_OFFWINLINK, u32Unit << HRPWM_LINKCR1_OFFWINLINK_POS);
}

/**
 * @brief  HRPWM set dead time rising edge link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetUpDeadTimeUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR2, HRPWM_LINKCR2_DTULINK, u32Unit << HRPWM_LINKCR2_DTULINK_POS);
}

/**
 * @brief  HRPWM set dead time falling edge link unit
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Unit             Specifies the link unit @ref HRPWM_Link_Unit_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_LINK_SetDownDeadTimeUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LINK_UNIT(u32Unit));

    MODIFY_REG32(HRPWMx->LINKCR2, HRPWM_LINKCR2_DTDLINK, u32Unit << HRPWM_LINKCR2_DTDLINK_POS);
}

/**
 * @brief  HRPWM set trigger calculate mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Specifies the trigger calculate mode @ref HRPWM_Trigger_Calculate_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetTriggerCalculateMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_TRIG_CALC_MD(u32Mode));

    MODIFY_REG32(HRPWMx->GCONR, HRPWM_TRIG_CALC_MD_MASK, u32Mode);
}

/**
 * @brief  HRPWM set trigger calculate data
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u16Data             Specifies the trigger calculate data
 * @retval None
 */
__STATIC_INLINE void HRPWM_SetTriggerCalculateData(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Data)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    WRITE_REG32(HRPWMx->TRGCALDATR, u16Data);
}

/**
 * @brief  Enable HRPWM DE
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_Enable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->DECTL, HRPWM_DECTL_ENABLE);
}

/**
 * @brief  Disable HRPWM DE
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_Disable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->DECTL, HRPWM_DECTL_ENABLE);
}

/**
 * @brief  HRPWM DE TRIP out source enable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the trigger calculate data @ref HRPWM_DE_TRIP_Out_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_TRIPOutSourceEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_OUT_SRC(u32Source));

    SET_REG32_BIT(HRPWMx->TRIPOUTSEL, u32Source);
}

/**
 * @brief  HRPWM DE TRIP out source disable
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the trigger calculate data @ref HRPWM_DE_TRIP_Out_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_TripOutSourceDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_OUT_SRC(u32Source));

    CLR_REG32_BIT(HRPWMx->TRIPOUTSEL, u32Source);
}

/**
 * @brief  Set HRPWM DE TRIPL source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the TRIPL source @ref HRPWM_DE_TRIP_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetTripLSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_SRC(u32Source));

    MODIFY_REG32(HRPWMx->DECOMPSEL, HRPWM_DECOMPSEL_TRIPL, u32Source << HRPWM_DECOMPSEL_TRIPL_POS);
}

/**
 * @brief  Set HRPWM DE TRIPH source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the TRIPH source @ref HRPWM_DE_TRIP_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetTripHSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_SRC(u32Source));

    MODIFY_REG32(HRPWMx->DECOMPSEL, HRPWM_DECOMPSEL_TRIPH, u32Source << HRPWM_DECOMPSEL_TRIPH_POS);
}

/**
 * @brief  Set HRPWM DE active flag mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Specifies the DE active flag mode @ref HRPWM_DE_Active_Flag_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetActiveFlagMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_ACT_FLAG_MD(u32Mode));

    MODIFY_REG32(HRPWMx->DECTL, HRPWM_DECTL_MODE, u32Mode);
}

/**
 * @brief  Set HRPWM DE re-entry delay
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Delay            Specifies the re-entry delay, range at 0~255
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetReEntryDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Delay)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->DECTL, HRPWM_DECTL_REENTRYDLY, u32Delay << HRPWM_DECTL_REENTRYDLY_POS);
}

/**
 * @brief  Set HRPWM DE ChA PWM level mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Specifies the pwm level mode @ref HRPWM_DE_PWM_Level_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetChAPwmLevelMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_PWM_LEVEL_MD(u32Mode));

    MODIFY_REG32(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_PWMA, u32Mode << HRPWM_DEACTCTL_PWMA_POS);
}

/**
 * @brief  Set HRPWM DE ChB PWM level mode
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Mode             Specifies the pwm level mode @ref HRPWM_DE_PWM_Level_Mode
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetChBPwmLevelMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_PWM_LEVEL_MD(u32Mode));

    MODIFY_REG32(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_PWMB, u32Mode << HRPWM_DEACTCTL_PWMB_POS);
}

/**
 * @brief  Set HRPWM DE ChA TRIP
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Slection         Specifies the trip signal selection @ref HRPWM_DE_PWM_TRIP_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetChATrip(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Slection)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_SEL(u32Slection));

    MODIFY_REG32(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_TRIPSELA, u32Slection << HRPWM_DEACTCTL_TRIPSELA_POS);
}

/**
 * @brief  Set HRPWM DE ChB TRIP
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Slection         Specifies the trip signal selection @ref HRPWM_DE_PWM_TRIP_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetChBTrip(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Slection)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_TRIP_SEL(u32Slection));

    MODIFY_REG32(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_TRIPSELB, u32Slection << HRPWM_DEACTCTL_TRIPSELB_POS);
}

/**
 * @brief  Enable HRPWM DE TRIP out bypass
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_TripOutBypassEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_TRIPENABLE);
}

/**
 * @brief  Disable HRPWM DE TRIP out bypass
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_TripOutBypassDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->DEACTCTL, HRPWM_DEACTCTL_TRIPENABLE);
}

/**
 * @brief  Get HRPWM DE active flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE en_flag_status_t HRPWM_DE_GetActiveFlag(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    return READ_REG32_BIT(HRPWMx->DESTAT, HRPWM_DESTAT_DEACTIVE) ? SET : RESET;
}

/**
 * @brief  Force set HRPWM DE active flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetActiveFlag(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    WRITE_REG32(HRPWMx->DEFRC, HRPWM_DEFRC_DEACTIVE);
}

/**
 * @brief  Clear HRPWM DE active flag
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_ClearActiveFlag(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    WRITE_REG32(HRPWMx->DECLR, HRPWM_DECLR_DEACTIVE);
}

/**
 * @brief  Get HRPWM DE monitorcounter value
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE uint32_t HRPWM_DE_GetMonitorCounter(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    return READ_REG32(HRPWMx->DEMONCNT);
}

/**
 * @brief  Enable HRPWM DE Monitor counter
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_MonitorCounterEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    WRITE_REG32(HRPWMx->DEMONCTL, HRPWM_DEMONCTL_ENABLE);
}

/**
 * @brief  Disable HRPWM DE Monitor counter
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_MonitorCounterDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->DEMONCTL, HRPWM_DEMONCTL_ENABLE);
}

/**
 * @brief  Set HRPWM DE increase step
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Step             Specifies the increase step, range at 0~255
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetIncreaseStep(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Step)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->DEMONSTEP, HRPWM_DEMONSTEP_INCSTEP, u32Step);
}

/**
 * @brief  Set HRPWM DE decrease step
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Step             Specifies the decrease step, range at 0~255
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetDecreaseStep(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Step)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->DEMONSTEP, HRPWM_DEMONSTEP_DECSTEP, u32Step << HRPWM_DEMONSTEP_DECSTEP_POS);
}

/**
 * @brief  Set HRPWM DE monitor threshold
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Threshold        Specifies the monitor threshold, range at 0~65535
 * @retval None
 */
__STATIC_INLINE void HRPWM_DE_SetMonitorThreshold(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Threshold)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_DE_MON_THRESHOLD(u32Threshold));

    WRITE_REG32(HRPWMx->DEMONRHRES, u32Threshold);
}

/**
 * @brief  Enable HRPWM ChA minimum dead time function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChAEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_ENABLEA);
}

/**
 * @brief  Disable HRPWM ChA minimum dead time function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChADisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_ENABLEA);
}

/**
 * @brief  Set HRPWM ChA minimum dead time reference signal polarity invert on or off
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Ploarity         Specifies the polarity @ref HRPWM_MINDB_REF_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChASetReferencePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_REF_INVT(u32Ploarity));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_INVERTA, u32Ploarity << HRPWM_MINDBCFG_INVERTA_POS);
}

/**
 * @brief  Set HRPWM ChA minimum dead time pwm block signal
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Block            Specifies the pwm block signal @ref HRPWM_MINDB_PWM_Block_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChASetPwmBlock(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Block)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_PWM_BLK_SEL(u32Block));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_SELBLOCKA, u32Block << HRPWM_MINDBCFG_SELBLOCKA_POS);
}

/**
 * @brief  Set HRPWM ChA minimum dead time reference signal
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Signal           Specifies the reference signal @ref HRPWM_MINDB_REF_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChASetReferenceSignal(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Signal)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_REF_SEL(u32Signal));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_SELA, u32Signal << HRPWM_MINDBCFG_SELA_POS);
}

/**
 * @brief  Set HRPWM ChA minimum dead time block signal polarity invert on or off
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Ploarity         Specifies the polarity @ref HRPWM_MINDB_Block_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChASetBlockPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_BLK_INVT(u32Ploarity));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_POLSELA, u32Ploarity << HRPWM_MINDBCFG_POLSELA_POS);
}

/**
 * @brief  Set HRPWM ChA minimum dead time delay
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u16Delay            Specifies the delay time
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChASetDelay(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Delay)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->MINDBDLY, HRPWM_MINDBDLY_DELAYA, u16Delay);
}

/**
 * @brief  Enable HRPWM ChB minimum dead time function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_ENABLEB);
}

/**
 * @brief  Disable HRPWM ChB minimum dead time function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_ENABLEB);
}

/**
 * @brief  Set HRPWM ChB minimum dead time reference signal polarity invert on or off
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Ploarity         Specifies the polarity @ref HRPWM_MINDB_REF_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBSetReferencePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_REF_INVT(u32Ploarity));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_INVERTB, u32Ploarity << HRPWM_MINDBCFG_INVERTB_POS);
}

/**
 * @brief  Set HRPWM ChB minimum dead time pwm block signal
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Block            Specifies the pwm block signal @ref HRPWM_MINDB_PWM_Block_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBSetPwmBlock(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Block)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_PWM_BLK_SEL(u32Block));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_SELBLOCKB, u32Block << HRPWM_MINDBCFG_SELBLOCKB_POS);
}

/**
 * @brief  Set HRPWM ChB minimum dead time reference signal
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Signal           Specifies the reference signal @ref HRPWM_MINDB_REF_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBSetReferenceSignal(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Signal)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_REF_SEL(u32Signal));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_SELB, u32Signal << HRPWM_MINDBCFG_SELB_POS);
}

/**
 * @brief  Set HRPWM ChB minimum dead time block signal polarity invert on or off
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Ploarity         Specifies the polarity @ref HRPWM_MINDB_Block_Invert_Define
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBSetBlockPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_MINDB_BLK_INVT(u32Ploarity));

    MODIFY_REG32(HRPWMx->MINDBCFG, HRPWM_MINDBCFG_POLSELB, u32Ploarity << HRPWM_MINDBCFG_POLSELB_POS);
}

/**
 * @brief  Set HRPWM ChB minimum dead time delay
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u16Delay            Specifies the delay
 * @retval None
 */
__STATIC_INLINE void HRPWM_MINDB_ChBSetDelay(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Delay)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->MINDBDLY, HRPWM_MINDBDLY_DELAYB, u16Delay << HRPWM_MINDBDLY_DELAYB_POS);
}

/**
 * @brief  Enable HRPWM ChA LUT bypass function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChABypassEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->LUTCTLA, HRPWM_LUTCTLA_BYPASS);
}

/**
 * @brief  Disable HRPWM ChA LUT bypass function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChABypassDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->LUTCTLA, HRPWM_LUTCTLA_BYPASS);
}

/**
 * @brief  Set HRPWM ChA LUT logic
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Logic            Specifies the logic, LUTDEC0~7 range at 0 ~ 0xFF
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChASetLogic(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Logic)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->LUTCTLA, HRPWM_LUTCTLA_LUTDEC, u32Logic << HRPWM_LUTCTLA_LUTDEC_POS);
}

/**
 * @brief  Set HRPWM ChA LUT input 3 source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the LUT input3 source @ref HRPWM_LUT_Input3_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChASetInput3Source(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LUT_INPUT_3_SRC(u32Source));

    MODIFY_REG32(HRPWMx->LUTCTLA, HRPWM_LUTCTLA_SELXBAR, u32Source << HRPWM_LUTCTLA_SELXBAR_POS);
}

/**
 * @brief  Enable HRPWM ChB LUT bypass function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChBBypassEnable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    SET_REG32_BIT(HRPWMx->LUTCTLB, HRPWM_LUTCTLB_BYPASS);
}

/**
 * @brief  Disable HRPWM ChB LUT bypass function
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChBBypassDisable(CM_HRPWM_TypeDef *HRPWMx)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    CLR_REG32_BIT(HRPWMx->LUTCTLB, HRPWM_LUTCTLB_BYPASS);
}

/**
 * @brief  Set HRPWM ChA LUT logic
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Logic            Specifies the logic, LUTDEC0~7 range at 0 ~ 0xFF
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChBSetLogic(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Logic)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));

    MODIFY_REG32(HRPWMx->LUTCTLB, HRPWM_LUTCTLB_LUTDEC, u32Logic << HRPWM_LUTCTLB_LUTDEC_POS);
}

/**
 * @brief  Set HRPWM ChB LUT input 3 source
 * @param  [in] HRPWMx              HRPWM unit
 *  @arg CM_HRPWMx
 * @param  [in] u32Source           Specifies the LUT input3 source @ref HRPWM_LUT_Input3_Source
 * @retval None
 */
__STATIC_INLINE void HRPWM_LUT_ChBSetInput3Source(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source)
{
    DDL_ASSERT(IS_HRPWM_UNIT(HRPWMx));
    DDL_ASSERT(IS_HRPWM_LUT_INPUT_3_SRC(u32Source));

    MODIFY_REG32(HRPWMx->LUTCTLB, HRPWM_LUTCTLB_SELXBAR, u32Source << HRPWM_LUTCTLB_SELXBAR_POS);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 1
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_1_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR1, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 1
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_1_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR1, u32Event);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 2
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_2_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR2, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 2
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_2_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR2, u32Event);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 3
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_3_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR3, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 3
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_1_2_3_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_3_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_1_2_3_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR3, u32Event);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 4
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_4_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR4, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 4
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_4_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR4, u32Event);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 5
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_5_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR5, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 5
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_5_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR5, u32Event);
}

/**
 * @brief  Enable specifies event generate HRPWM global AOS event 6
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_6_EventEnable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    SET_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR6, u32Event);
}

/**
 * @brief  Disable specifies event generate HRPWM global AOS event 6
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_4_5_6_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_6_EventDisable(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_4_5_6_EVT_SEL(u32Event));

    CLR_REG32_BIT(CM_HRPWM_COMMON->GLAOSSELR6, u32Event);
}

/**
 * @brief  Set specifies event generate HRPWM global AOS event 7
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_7_8_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_7_SetEvent(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_7_8_EVT_SEL(u32Event));

    MODIFY_REG32(CM_HRPWM_COMMON->GLAOSSELR7, HRPWM_COMMON_GLAOSSELR7_GLAOSSEL7, u32Event);
}

/**
 * @brief  Set specifies event generate HRPWM global AOS event 8
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_7_8_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_8_SetEvent(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_7_8_EVT_SEL(u32Event));

    MODIFY_REG32(CM_HRPWM_COMMON->GLAOSSELR7, HRPWM_COMMON_GLAOSSELR7_GLAOSSEL8, u32Event);
}

/**
 * @brief  Set specifies event generate HRPWM global AOS event 9
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_9_10_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_9_SetEvent(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_9_10_EVT_SEL(u32Event));

    MODIFY_REG32(CM_HRPWM_COMMON->GLAOSSELR7, HRPWM_COMMON_GLAOSSELR7_GLAOSSEL9, u32Event);
}

/**
 * @brief  Set specifies event generate HRPWM global AOS event 9
 * @param  [in] u32Event        Specifies the event @ref HRPWM_Global_AOS_9_10_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_10_SetEvent(uint32_t u32Event)
{
    DDL_ASSERT(IS_HRPWM_GLAOS_9_10_EVT_SEL(u32Event));

    MODIFY_REG32(CM_HRPWM_COMMON->GLAOSSELR7, HRPWM_COMMON_GLAOSSELR7_GLAOSSEL10, u32Event);
}

/**
 * @brief  Set specifies global AOS event buffer
 * @param  [in] u32EventNum     Specifies the event number @ref HRPWM_Global_AOS_Event_Num_Define
 * @param  [in] u32Buffer       Specifies the buffer @ref HRPWM_Global_AOS_Event_Buf
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_SetEventBuffer(uint32_t u32EventNum, uint32_t u32Buffer)
{
    uint32_t u32Offset = 0U;

    DDL_ASSERT(IS_HRPWM_GLAOS_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_GLAOS_EVT_BUF(u32Buffer));

    u32Offset = (u32EventNum % HRPWM_GLAOS_EVT_9) * HRPWM_COMMON_GLAOSUR1_GLAOSU2_POS;

    if (u32EventNum >= HRPWM_GLAOS_EVT_9) {
        MODIFY_REG32(CM_HRPWM_COMMON->GLAOSUR2, HRPWM_COMMON_GLAOSUR2_GLAOSU9 << u32Offset, u32Buffer << u32Offset);
    } else {
        MODIFY_REG32(CM_HRPWM_COMMON->GLAOSUR1, HRPWM_COMMON_GLAOSUR1_GLAOSU1 << u32Offset, u32Buffer << u32Offset);
    }
}

/**
 * @brief  Set specifies global AOS event divide
 * @param  [in] u32EventNum     Specifies the event number @ref HRPWM_Global_AOS_Event_Num_Define
 * @param  [in] u32Divide       Specifies the divide, range at 0~31
 *                              Means The global aos event occurs (u32Divide + 1) times, the global aos event output
 * @retval None
 */
__STATIC_INLINE void HRPWM_GLAOS_SetEventDivide(uint32_t u32EventNum, uint32_t u32Divide)
{
    uint32_t u32Offset = 0U;

    DDL_ASSERT(IS_HRPWM_GLAOS_EVT_NUM(u32EventNum));
    DDL_ASSERT(IS_HRPWM_GLAOS_EVT_DIV(u32Divide));

    u32Offset = (u32EventNum % HRPWM_GLAOS_EVT_6) * HRPWM_COMMON_GLAOSPSCR1_PSC2_POS;

    if (u32EventNum >= HRPWM_GLAOS_EVT_6) {
        MODIFY_REG32(CM_HRPWM_COMMON->GLAOSPSCR2, HRPWM_COMMON_GLAOSPSCR2_PSC6 << u32Offset, u32Divide << u32Offset);
    } else {
        MODIFY_REG32(CM_HRPWM_COMMON->GLAOSPSCR1, HRPWM_COMMON_GLAOSPSCR1_PSC1 << u32Offset, u32Divide << u32Offset);
    }
}

/**
 * @brief  Enable burst DMA to update HRPWM1 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U1(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U1(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR11, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR12, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM1 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U1(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U1(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR11, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR12, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM2 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U2(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR21, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR22, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM2 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U2(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR21, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR22, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM3 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U3(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR31, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR32, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM3 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U3(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR31, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR32, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM4 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U4(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR41, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR42, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM4 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U4(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR41, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR42, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM5 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U5(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR51, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR52, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM5 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U5(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR51, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR52, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM6 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U6(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR61, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR62, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM6 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U6(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR61, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR62, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM7 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U7(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR71, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR72, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM7 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U7(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR71, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR72, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Enable burst DMA to update HRPWM8 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegEnable_U8(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR81, (uint32_t)u64UpdateReg);
    SET_REG32_BIT(CM_HRPWM_COMMON->BDUPR82, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Disable burst DMA to update HRPWM8 registers
 * @param  [in] u64UpdateReg        Specifies the update registers @ref HRPWM_DMA_Update_Reg
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_UpdateRegDisable_U8(uint64_t u64UpdateReg)
{
    DDL_ASSERT(IS_HRPWM_DMA_UPDATE_REG_U2_8(u64UpdateReg));

    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR81, (uint32_t)u64UpdateReg);
    CLR_REG32_BIT(CM_HRPWM_COMMON->BDUPR82, (uint32_t)(u64UpdateReg >> 32U));
}

/**
 * @brief  Start burst DMA to update HRPWM registers
 * @param  [in] u32Value            Specifies the update value, which will be written to the registers specified by burst DMA update reg when burst DMA transfer occurs
 * @retval None
 */
__STATIC_INLINE void HRPWM_DMA_StartUpdate(uint32_t u32Value)
{
    WRITE_REG32(CM_HRPWM_COMMON->BDMADR, u32Value);
}

/**
 * @brief  Stop the pwm by software
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_SwStop(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx)
{
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));

    WRITE_REG32(HRPWM_EMBx->SOE, HRPWM_EMB_SOE_SOE);
}

/**
 * @brief  Start the pwm by software
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_SwStart(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx)
{
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));

    WRITE_REG32(HRPWM_EMBx->SOE, 0U);
}

/**
 * @brief  Reset the HRPWM EMB accumlator
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_ResetAccum(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx)
{
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));

    SET_REG32_BIT(HRPWM_EMBx->CTL5, HRPWM_EMB_CTL5_EMBCRES);
}

/**
 * @brief  Set the HRPWM EMB input event selection
 * @param  [in] HRPWM_EMBx              HRPWM EMB unit
 *  @arg CM_HRPWM_EMBx
 * @param  [in] u32InputNum             Specifies the input event number, @ref HRPWM_EMB_Input_Number_Define
 * @param  [in] u32EventSel             Specifies the event source selection for the Specifies innput @ref HRPWM_EMB_Input_Event_Sel
 * @retval None
 */
__STATIC_INLINE void HRPWM_EMB_SetInputEvent(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32InputNum, uint32_t u32EventSel)
{
    DDL_ASSERT(IS_HRPWM_EMB_UNIT(HRPWM_EMBx));
    DDL_ASSERT(IS_HRPWM_EMB_INPUT_NUM(u32InputNum));
    DDL_ASSERT(IS_HRPWM_EMB_INPUT_EVENT_SEL(u32EventSel));

    MODIFY_REG32(HRPWM_EMBx->CTL6, u32InputNum, u32InputNum & u32EventSel);
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
void HRPWM_PWM_ChASetPolarityUpSwTrigger(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpSwTrigger_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityUpU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetPolarityDownU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
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
void HRPWM_PWM_ChBSetPolarityUpSwTrigger(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpSwTrigger_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownTmrEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownTmrEvent_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityUpU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1Peak(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1Peak_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1Valley(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1Valley_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchF(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchF_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchE(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchE_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchB_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetPolarityDownU1MatchA_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
/* PWM force polarity */
void HRPWM_PWM_ChASetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChASetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetForcePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);
void HRPWM_PWM_ChBSetForcePolarity_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Polarity);

/******************************************************************************/
/* Capture */
void HRPWM_HWChACaptureCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChACaptureCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChACaptureCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChACaptureCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWChBCaptureCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
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

int32_t HRPWM_PortBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_PortBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PortBufDisable(CM_HRPWM_TypeDef *HRPWMx);

int32_t HRPWM_ValidPeriodBufConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_buf_config_t *pstcBufConfig);
void HRPWM_ValidPeriodBufEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_ValidPeriodBufDisable(CM_HRPWM_TypeDef *HRPWMx);

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

void HRPWM_IDLE_BAL_Enable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_BAL_Disable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_IDLE_BAL_Exit(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Valid Period */
int32_t HRPWM_ValidPeriodStructInit(stc_hrpwm_valid_period_config_t *pstcValidperiodConfig);
int32_t HRPWM_ValidPeriodConfig(CM_HRPWM_TypeDef *HRPWMx, const stc_hrpwm_valid_period_config_t *pstcValidperiodConfig);

void HRPWM_SetValidPeriodSpecialA(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialA);
void HRPWM_SetValidPeriodSpecialB(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32SpecialB);
void HRPWM_SetValidPeriodInterval(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Interval);
void HRPWM_SetValidPeriodInterval_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Interval);
void HRPWM_SetValidPeriodCountCond(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32CountCond);

uint32_t HRPWM_GetValidPeriodPeriodNum(const CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Hardware control */
void HRPWM_HWStartCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWStartCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWStartCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWStartCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondInEventTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondExtEventMatchEnable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondInEventTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);
void HRPWM_HWClearCondExtEventMatchDisable(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64Cond);

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
void HRPWM_EVT_SetValidAction(uint32_t u32EventNum, uint32_t u32ValidAction);
void HRPWM_EVT_SetValidLevel(uint32_t u32EventNum, uint32_t u32ValidLevel);
void HRPWM_EVT_SetFastAsyncMode(uint32_t u32EventNum, uint32_t u32FastAsync);
void HRPWM_EVT_SetFilterClock(uint32_t u32EventNum, uint32_t u32Clock);
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

void HRPWM_EVT_SetCountEvent(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum);
void HRPWM_EVT_SetCountThreshold(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Threshold);
void HRPWM_EVT_ResetCountValue(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_EVT_EnableCount(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_EVT_DisableCount(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_EVT_SetCountMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_EVT_UpHwTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum);
void HRPWM_EVT_UpHwTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum);
void HRPWM_EVT_DownHwTriggerEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum);
void HRPWM_EVT_DownHwTriggerDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32EventNum);

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

void HRPWM_DAC_RampEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DAC_RampDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DAC_SetRampResetSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_DAC_SetRampStepSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);

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
int32_t HRPWM_PH_BufConfig(const stc_hrpwm_buf_config_t *pstcBufConfig);

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

/******************************************************************************/
/* Chopping */
void HRPWM_CHP_ChAEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CHP_ChADisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CHP_ChBEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CHP_ChBDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* HRPWM LINK */
void HRPWM_LINK_SetPeriodUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetCompareAValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetCompareBValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetCompareEValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetCompareFValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetSpecialCompareAValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetSpecialCompareBValueUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetExternalEventFilterUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetUpDeadTimeUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_LINK_SetDownDeadTimeUnit(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Unit);
void HRPWM_SetTriggerCalculateMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_SetTriggerCalculateData(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Data);

/******************************************************************************/
/* DE diade emulation */
void HRPWM_DE_Enable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_Disable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_TRIPOutSourceEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_DE_TripOutSourceDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_DE_SetTripLSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_DE_SetTripHSource(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_DE_SetActiveFlagMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_DE_SetReEntryDelay(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Delay);
void HRPWM_DE_SetChAPwmLevelMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_DE_SetChBPwmLevelMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_DE_SetChATrip(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Slection);
void HRPWM_DE_SetChBTrip(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Slection);
void HRPWM_DE_TripOutBypassEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_TripOutBypassDisable(CM_HRPWM_TypeDef *HRPWMx);
en_flag_status_t HRPWM_DE_GetActiveFlag(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_SetActiveFlag(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_ClearActiveFlag(CM_HRPWM_TypeDef *HRPWMx);
uint32_t HRPWM_DE_GetMonitorCounter(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_MonitorCounterEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_MonitorCounterDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_DE_SetIncreaseStep(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Step);
void HRPWM_DE_SetDecreaseStep(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Step);
void HRPWM_DE_SetMonitorThreshold(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Threshold);

/******************************************************************************/
/* Minimum dead time */
void HRPWM_MINDB_ChAEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_MINDB_ChADisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_MINDB_ChASetReferencePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity);
void HRPWM_MINDB_ChASetPwmBlock(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Block);
void HRPWM_MINDB_ChASetReferenceSignal(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Signal);
void HRPWM_MINDB_ChASetBlockPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity);
void HRPWM_MINDB_ChASetDelay(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Delay);
void HRPWM_MINDB_ChBEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_MINDB_ChBDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_MINDB_ChBSetReferencePolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity);
void HRPWM_MINDB_ChBSetPwmBlock(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Block);
void HRPWM_MINDB_ChBSetReferenceSignal(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Signal);
void HRPWM_MINDB_ChBSetBlockPolarity(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Ploarity);
void HRPWM_MINDB_ChBSetDelay(CM_HRPWM_TypeDef *HRPWMx, uint16_t u16Delay);

/******************************************************************************/
/* LUT */
void HRPWM_LUT_ChABypassEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_LUT_ChABypassDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_LUT_ChASetLogic(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Logic);
void HRPWM_LUT_ChASetInput3Source(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);
void HRPWM_LUT_ChBBypassEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_LUT_ChBBypassDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_LUT_ChBSetLogic(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Logic);
void HRPWM_LUT_ChBSetInput3Source(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Source);

/******************************************************************************/
/* AOS */
void HRPWM_AosEventEnable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_AosEventDisable(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_AosEventEnable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);
void HRPWM_AosEventDisable_Buf(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Event);

/******************************************************************************/
/* Global AOS */
void HRPWM_GLAOS_1_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_1_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_2_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_2_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_3_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_3_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_4_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_4_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_5_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_5_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_6_EventEnable(uint32_t u32Event);
void HRPWM_GLAOS_6_EventDisable(uint32_t u32Event);
void HRPWM_GLAOS_7_SetEvent(uint32_t u32Event);
void HRPWM_GLAOS_8_SetEvent(uint32_t u32Event);
void HRPWM_GLAOS_9_SetEvent(uint32_t u32Event);
void HRPWM_GLAOS_10_SetEvent(uint32_t u32Event);
void HRPWM_GLAOS_SetEventBuffer(uint32_t u32EventNum, uint32_t u32Buffer);
void HRPWM_GLAOS_SetEventDivide(uint32_t u32EventNum, uint32_t u32Divide);

/******************************************************************************/
/* DMA */
void HRPWM_DMA_UpdateRegEnable_U1(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U1(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U2(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U2(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U3(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U3(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U4(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U4(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U5(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U5(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U6(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U6(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U7(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U7(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegEnable_U8(uint64_t u64UpdateReg);
void HRPWM_DMA_UpdateRegDisable_U8(uint64_t u64UpdateReg);
void HRPWM_DMA_StartUpdate(uint32_t u32Value);
void HRPWM_DMA_UpdateRegCmd_U2_8(CM_HRPWM_TypeDef *HRPWMx, uint64_t u64UpdateReg, en_functional_state_t enNewState);

/******************************************************************************/
/* HRPWM EMB */
void HRPWM_EMB_DeInit(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx);
void HRPWM_EMB_SwStop(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx);
void HRPWM_EMB_SwStart(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx);
void HRPWM_EMB_ClearStatus(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Flag);
en_flag_status_t HRPWM_EMB_GetStatus(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Flag);
void HRPWM_EMB_EventEnable(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event);
void HRPWM_EMB_IntCmd(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event, en_functional_state_t enNewState);
void HRPWM_EMB_SetAccumThreshold(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Threshold);
void HRPWM_EMB_SetAccumEvent(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Event);
void HRPWM_EMB_ResetAccum(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx);
void HRPWM_EMB_SetAccumResetMode(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, uint32_t u32Mode);
int32_t HRPWM_EMB_LevelAndFilterStructInit(stc_hrpwm_emb_level_filter_t *pstcLevelFilterConfig);
int32_t HRPWM_EMB_LevelAndFilterConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_level_filter_t *pstcLevelFilterConfig);
int32_t HRPWM_EMB_NoiseFilterStructInit(stc_hrpwm_emb_nf_config_t *pstcNfConfig);
int32_t HRPWM_EMB_NoiseFilterConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_nf_config_t *pstcNfConfig);
int32_t HRPWM_EMB_BlankAndAccumStructInit(stc_hrpwm_emb_blank_Accum_t *pstcBlankAccumConfig);
int32_t HRPWM_EMB_BlankAndAccumConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_blank_Accum_t *pstcBlankAccumConfig);
int32_t HRPWM_EMB_AccumStructInit(stc_hrpwm_emb_Accum_config_t *pstcAccumConfig);
int32_t HRPWM_EMB_AccumConfig(CM_HRPWM_EMB_TypeDef* HRPWM_EMBx, stc_hrpwm_emb_Accum_config_t *pstcAccumConfig);

/******************************************************************************/
/* Interleaving */
void HRPWM_SetIntlvMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_SetU1IntlvMode(uint32_t u32Mode);

/******************************************************************************/
/* Push-Pull */
void HRPWM_PushPullEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_PushPullDisable(CM_HRPWM_TypeDef *HRPWMx);
en_flag_status_t HRPWM_GetPushPullState(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* trigger half mode */
void HRPWM_TriggerHalfEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_TriggerHalfDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Compare value greater than match */
void HRPWM_CompareBGreaterThanMatchEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CompareBGreaterThanMatchDisable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CompareFGreaterThanMatchEnable(CM_HRPWM_TypeDef *HRPWMx);
void HRPWM_CompareFGreaterThanMatchDisable(CM_HRPWM_TypeDef *HRPWMx);

/******************************************************************************/
/* Auto Delay */
void HRPWM_SetCompareADelayMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);
void HRPWM_SetCompareEDelayMode(CM_HRPWM_TypeDef *HRPWMx, uint32_t u32Mode);

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
