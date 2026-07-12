/**
 *******************************************************************************
 * @file  hc32_ll_sdfm.h
 * @brief This file contains all the functions prototypes of the SDFM driver
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
#ifndef __HC32_LL_SDFM_H__
#define __HC32_LL_SDFM_H__

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
 * @addtogroup LL_SDFM
 * @{
 */

#if (LL_SDFM_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup SDFM_Global_Types SDFM Global Types
 * @{
 */
/**
 * @brief SDFM channel initialization structure
 */
typedef struct {
    uint32_t u32InputMux;                       /*!< Specifies input data multiplexer.
                                                     This parameter can be a value of @ref SDFM_Ch_Input_Data_Mux */
    uint32_t u32DataPacking;                    /*!< Specifies data packing mode for input data.
                                                     This parameter can be a value of @ref SDFM_Ch_Input_Data_Packing */
    uint32_t u32InputSel;                       /*!< Specifies channel input selection.
                                                     This parameter can be a value of @ref SDFM_Ch_Input */
    uint32_t u32SerialIntfType;                 /*!< Specifies serial interface type.
                                                     This parameter can be a value of @ref SDFM_Ch_Serial_Intf_Type
                                                     The internal SDFM clock must be at least 4 times faster than the external
                                                     serial clock if standard SPI coding is used, and 6 times faster than the
                                                     external serial clock if Manchester coding is used. */
    uint32_t u32SpiClock;                       /*!< Specifies SPI clock when u32SerialIntfType is SDFM_CH_SERIAL_INTF_SPI_xxxx
                                                     This parameter can be a value of @ref SDFM_Ch_SPI_Clk */
    int32_t  i32CalOffset;                      /*!< Specifies calibration offset.
                                                     This parameter must be a number between -8388608 and 8388607. */
    uint32_t u32RightBitShift;                  /*!< Specifies right bit-shift.
                                                     The value of this parameter is related to the parameters of @ref stc_sdfm_filter_sinc_t
                                                     This parameter must be a number between 0 and 31.
                                                     0-31: Defines the shift of the data result coming from the integrator - how many
                                                     bit shifts to the right will be performed to have final results. Bit-shift is performed
                                                     before offset correction. The data shift is rounding the result to nearest integer value.
                                                     The sign of shifted result is maintained(to have valid 24-bit signed format of result data). */
    uint32_t u32ClockSel;                       /*!< Channel clock selection.
                                                     This parameter can be a value of @ref SDFM_Ch_Clk_Sel */
    uint32_t u32ClockSync;                      /*!< Enable/Disable clock synchronization.
                                                     This parameter can be a value of @ref SDFM_Ch_Clk_Sync_Ctrl */
    uint32_t u32DataSync;                       /*!< Enable/Disable data synchronization.
                                                     This parameter can be a value of @ref SDFM_Ch_Data_Sync_Ctrl */
    int16_t  i16CrossZeroThreshold;             /*!< Specifies cross zero threshold.
                                                     The channel value after filtered by fast sinc filter, minus this threshold,
                                                     can generate cross zero interrupt if crossing zero occurred and the interrupt enabled.
                                                     This parameter must be a number between -32768 and 32767 */
} stc_sdfm_ch_init_t;

/**
 * @brief SDFM filter AWD parameters structure
 */
typedef struct {
    uint32_t u32DataSrc;                /*!< Values from digital filter or from channel watchdog filter.
                                             This parameter can be a value of @ref SDFM_Filter_Awd_Data_Src */
    int32_t i32HighThreshold;           /*!< Analog watchdog high threshold.
                                             This parameter must be a number between -8388608 and 8388607 */
    int32_t i32LowThreshold;            /*!< Analog watchdog low threshold.
                                             This parameter must be a number between -8388608 and 8388607 */
    int16_t i16HighThreshold2;          /*!< Analog watchdog high threshold2.
                                             This parameter must be a number between -32768 and 32767 */
    int16_t i16LowThreshold2;           /*!< Analog watchdog low threshold2.
                                             This parameter must be a number between -32768 and 32767 */
    uint32_t u32HighBreakSignal;        /*!< Break signal assignment to analog watchdog high threshold event.
                                             This parameter is also used to assign CEVT1 to break signal.
                                             This parameter can be any combination of @ref SDFM_Break_Signal */
    uint32_t u32LowBreakSignal;         /*!< Break signal assignment to analog watchdog low threshold event.
                                             This parameter is also used to assign CEVT2 to break signal.
                                             This parameter can be any combination of @ref SDFM_Break_Signal */
    uint32_t u32CrossZeroBreakSignal;   /*!< Break signal assignment to cross zero event.
                                             This parameter can be any combination of @ref SDFM_Break_Signal */
} stc_sdfm_filter_awd_t;

/**
 * @brief SDFM filter regular conversion parameters structure
 */
typedef struct {
    uint32_t u32RegularCh;                      /*!< Regular channel selection.
                                                     This parameter can be a value of @ref SDFM_Channel */
    uint32_t u32FastMode;                       /*!< Enable/disable fast mode for regular conversion.
                                                     This parameter can be a value of @ref SDFM_Filter_Regu_Fast_Mode_Ctrl */
    uint32_t u32DmaRead;                        /*!< Enable/disable DMA channel to read data for the regular conversion.
                                                     This parameter can be a value of @ref SDFM_Filter_Regu_Dma_Ctrl */
    uint32_t u32SyncStart;                      /*!< Enable/disable to launch regular conversion synchronously with SDFM_FLT0.
                                                     This parameter can be a value of @ref SDFM_Filter_Regu_Sync_Ctrl */
    uint32_t u32ContMode;                       /*!< Enable/Disable continuous mode selection for regular conversions.
                                                     This parameter can be a value of @ref SDFM_Filter_Regu_Cont_Ctrl */
} stc_sdfm_filter_regular_conv_t;

/**
 * @brief SDFM filter injected conversion parameters structure
 */
typedef struct {
    uint32_t u32InjectedCh;                     /*!< Injected channel group selection.
                                                     This parameter is ignored if u32Type == SDFM_FILTER_INJ_TYPE_FIFO, the channel
                                                     specified by u32RegularCh of @ref stc_sdfm_filter_regular_conv_t is used as injected channel.
                                                     This parameter can be any combination of @ref SDFM_Channel_Mult */
    uint32_t u32ExtTrigger;                     /*!< Trigger signal selection for launching injected conversions.
                                                     This parameter can be a value of @ref SDFM_Filter_Ext_Trigger */
    uint32_t u32ExtTriggerEdge;                 /*!< Trigger enable and trigger edge selection for injected conversions.
                                                     External trigger edge: rising, falling or both
                                                     This parameter can be a value of @ref SDFM_Filter_Ext_Trigger_Edge */
    uint32_t u32DmaRead;                        /*!< Enable/disable DMA channel to read data for the injected channel group.
                                                     This parameter can be a value of @ref SDFM_Filter_Inj_Dma_Ctrl */
    uint32_t u32ScanMode;                       /*!< Scanning conversion mode for injected conversions.
                                                     This parameter can be a value of @ref SDFM_Filter_Inj_Scan_Mode_Ctrl */
    uint32_t u32SyncStart;                      /*!< Enable/disable to launch an injected conversion synchronously with the SDFM_FLT0 JSWSTART trigger.
                                                     This parameter can be a value of @ref SDFM_Filter_Inj_Sync_Ctrl */
    uint32_t u32Type;                           /*!< Injected Conversion type.
                                                     This parameter can be a value of @ref SDFM_Filter_Inj_Type */
} stc_sdfm_filter_injected_conv_t;

/**
 * @brief SDFM filter control parameter structure
 */
typedef struct {

    uint32_t u32SincOrder;                      /*!< Sinc filter order.
                                                     This parameter can be a value of @ref SDFM_Filter_Sinc_Order */
    uint32_t u32SincFOSR;                       /*!< Sinc filter oversampling ratio (decimation rate).
                                                     This parameter must be a number between 1 and 1024:
                                                     1~1024: for FastSinc, Sinc1~Sinc3
                                                     1~215: for Sinc4
                                                     1~73: for Sinc5 */
    uint32_t u32SincIOSR;                       /*!< Integrator oversampling ratio (averaging length).
                                                     This parameter must be a number between 1 and 256 */
} stc_sdfm_filter_sinc_t;

/**
 * @brief SDFM filter FIFO configuration structure
 */
typedef struct {
    uint32_t u32IntLevel;                       /*!< Interrupt level.
                                                     The FIFO will generate an interrupt when the FIFO status (SDFFST) > fifo level (SDFFIL).
                                                     This parameter can be a value between 0 and 15. */
    uint32_t u32WaitForSync;                    /*!< FIFO Wait for synchronization.
                                                     This parameter can be a value of @ref SDFM_Filter_Fifo_Wait_For_Sync */
    uint32_t u32ClearOnSync;                    /*!< Clear FIFO on synchronization.
                                                     This parameter can be a value of @ref SDFM_Filter_Fifo_Clear_On_Sync */
    uint32_t u32FlagClearOnInt;                 /*!< Wait-for-Sync Flag FIFO on interrupt.
                                                     This parameter can be a value of @ref SDFM_Filter_Fifo_Flag_Clear_On_Int */
} stc_sdfm_filter_fifo_config_t;

/**
 * @brief SDFM filter comparator event configuration structure
 */
typedef struct {
    en_functional_state_t enIntState;               /*!< Enable/Disable comparator event interrupt */
    uint32_t u32InputSel;                           /*!< Comparator event input selection.
                                                         This parameter can be a value of @ref SDFM_Filter_Cevt_Input_Sel */
    uint32_t u32OutputSel;                          /*!< Comparator event output selection.
                                                         This parameter can be a value of @ref SDFM_Filter_Cevt_Output_Sel */
    uint32_t u32SampleWindow;                       /*!< Filter sample window size.
                                                         This parameter must be a number between 1 and 32.
                                                         The CEVT latches the register and sequentially inputs the auxiliary filter's
                                                         output into a 32-bit FIFO with a depth of u32SampleWindow.
                                                         If the count of 1s is not less than u32VotingThreshold, the filter outputs 1.
                                                         If the count of 0s is not less than u32VotingThreshold, the filter outputs 0. */
    uint32_t u32VotingThreshold;                    /*!< Filter majority voting threshold.
                                                         This parameter must be a number between 0 and 31.
                                                         For proper operation, this parameter must be greater than u32SampleWindow/2,
                                                         and less than or equal to u32SampleWindow. */
    /* Do assign CEVT break signal via parameter 'u32HighBreakSignal' and 'u32LowBreakSignal' of @ref stc_sdfm_filter_awd_t */
} stc_sdfm_filter_cevt_config_t;

/**
 * @brief SDFM filter initialization structure
 */
typedef struct {
    /* Configures the conversion that needed, regular conversion or injected conversion or both */
    stc_sdfm_filter_regular_conv_t  stcReguConv;        /*!< Regular conversion parameters structure. */

    stc_sdfm_filter_injected_conv_t stcInjConv;         /*!< Injected conversion parameters structure. */

    stc_sdfm_filter_sinc_t stcSinc;                     /*!< Sinc filter parameters structure. */

    uint32_t u32OvfDeal;                                /*!< Filter overflow deal.
                                                             This parameter can be a value of @ref SDFM_Filter_Ovf_Deal */
} stc_sdfm_filter_init_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup SDFM_Global_Macros SDFM Global Macros
 * @{
 */

/**
 * @defgroup SDFM_Channel SDFM Channel Definition
 * @{
 */
#define SDFM_CH0                        (0U)                        /*!< SDFM channel 0. */
#define SDFM_CH1                        (1U)                        /*!< SDFM channel 1. */
#define SDFM_CH2                        (2U)                        /*!< SDFM channel 2. */
#define SDFM_CH3                        (3U)                        /*!< SDFM channel 3. */
#define SDFM_CH4                        (4U)                        /*!< SDFM channel 4. */
#define SDFM_CH5                        (5U)                        /*!< SDFM channel 5. */
#define SDFM_CH6                        (6U)                        /*!< SDFM channel 6. */
#define SDFM_CH7                        (7U)                        /*!< SDFM channel 7. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Channel_Mult SDFM Channel Multiple Definition
 * @{
 */
#define SDFM_MX_CH0                     (1UL << SDFM_CH0)          /*!< Select SDFM channel 0. */
#define SDFM_MX_CH1                     (1UL << SDFM_CH1)          /*!< Select SDFM channel 1. */
#define SDFM_MX_CH2                     (1UL << SDFM_CH2)          /*!< Select SDFM channel 2. */
#define SDFM_MX_CH3                     (1UL << SDFM_CH3)          /*!< Select SDFM channel 3. */
#define SDFM_MX_CH4                     (1UL << SDFM_CH4)          /*!< Select SDFM channel 4. */
#define SDFM_MX_CH5                     (1UL << SDFM_CH5)          /*!< Select SDFM channel 5. */
#define SDFM_MX_CH6                     (1UL << SDFM_CH6)          /*!< Select SDFM channel 6. */
#define SDFM_MX_CH7                     (1UL << SDFM_CH7)          /*!< Select SDFM channel 7. */
#define SDFM_MX_CH_ALL                  (0xFFUL)                   /*!< Select all channels. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter SDFM Filter Definition
 * @{
 */
#define SDFM_FILTER0                    (0U)                        /*!< Filter 0 of SDFM. */
#define SDFM_FILTER1                    (1U)                        /*!< Filter 1 of SDFM. */
#define SDFM_FILTER2                    (2U)                        /*!< Filter 2 of SDFM. */
#define SDFM_FILTER3                    (3U)                        /*!< Filter 3 of SDFM. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Cevt SDFM Filter Comparator Event
 * @{
 */
#define SDFM_FILTER_CEVT1               (0U)                        /*!< SDFM Filter Comparator Event 1. */
#define SDFM_FILTER_CEVT2               (1U)                        /*!< SDFM Filter Comparator Event 2. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Output_Clk_Src SDFM Channel Output Clock Source
 * @{
 */
#define SDFM_CH_OUTPUT_CLK_SYS          (0x0U)                      /*!< Source for output clock is from system clock */
#define SDFM_CH_OUTPUT_CLK_AUDIO        (SDFM_CHCFGR1_CKOUTSRC)     /*!< Source for output clock is from audio clock */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Input_Data_Mux SDFM Channel Input Data Multiplexer
 * @{
 */
#define SDFM_CH_INPUT_DATA_EXT          (0x0U)                      /*!< Data to channel y are taken from external serial inputs as 1-bit values. */
#define SDFM_CH_INPUT_DATA_INTERN_REG   (SDFM_CHCFGR1_DATMPX_1)     /*!< Data to channel y are taken from internal SDFM_CHyDATINR register by direct CPU/DMA write. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Input_Data_Packing SDFM Channel Input Data Packing
 * @{
 */
#define SDFM_CH_INPUT_DATA_PACKING_STD              (0x0U)                      /*!< Standard data packing mode.
                                                                                     Input data in SDFM_CHyDATINR register are stored only in INDAT0[15:0].
                                                                                     To empty SDFM_CHyDATINR register, one sample must be read by the SDFM filter from channel y. */
#define SDFM_CH_INPUT_DATA_PACKING_INTERLEAVED      (SDFM_CHCFGR1_DATPACK_0)    /*!< Interleaved data packing mode.
                                                                                     Input data in SDFM_CHyDATINR register are stored as two samples:
                                                                                     - first sample in INDAT0[15:0] (assigned to channel y),
                                                                                     - second sample INDAT1[15:0] (assigned to channel y).
                                                                                     To empty SDFM_CHyDATINR register, two samples must be read by the digital filter from channel y
                                                                                     (INDAT0[15:0] part is read as first sample and then INDAT1[15:0] part is read as next sample). */
#define SDFM_CH_INPUT_DATA_PACKING_DUAL             (SDFM_CHCFGR1_DATPACK_1)    /*!< Dual data packing mode.
                                                                                     Input data in SDFM_CHyDATINR register are stored as two samples:
                                                                                     - first sample INDAT0[15:0] (assigned to channel y),
                                                                                     - second sample INDAT1[15:0] (assigned to channel y+1).
                                                                                     To empty SDFM_CHyDATINR register, first sample must be read by the digital filter from channel y
                                                                                     and second sample must be read by another digital filter from channel y+1.
                                                                                     Dual mode is available only on even channel numbers (y = 0, 2, 4, 6),
                                                                                     for odd channel numbers (y = 1, 3, 5, 7) SDFM_CHyDATINR is write protected.
                                                                                     If an even channel is set to dual mode then the following odd channel must be set into standard
                                                                                     mode (DATPACK[1:0]=0) for correct cooperation with even channel. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Input SDFM Channel Input Selection
 * @{
 */
#define SDFM_CH_INPUT_PIN_SAME_CH_PIN               (0x0U)                      /*!< Channel inputs are taken from pins of the same channel */
#define SDFM_CH_INPUT_PIN_FOLLOW_CH_PIN             (SDFM_CHCFGR1_CHINSEL)      /*!< Channel inputs are taken from pins of the following channel (channel (y+1) modulo 8) */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Serial_Intf_Type SDFM Channel Serial Interface Type
 * @{
 */
#define SDFM_CH_SERIAL_INTF_SPI_RISING              (0x0U)                      /*!< SPI with rising edge to strobe data */
#define SDFM_CH_SERIAL_INTF_SPI_FALLING             (SDFM_CHCFGR1_SITP_0)       /*!< SPI with falling edge to strobe data */
#define SDFM_CH_SERIAL_INTF_MANCHESTER_RISING       (SDFM_CHCFGR1_SITP_1)       /*!< Manchester coded input on DATINy pin: rising edge = logic 0, falling edge = logic 1 */
#define SDFM_CH_SERIAL_INTF_MANCHESTER_FALLING      (SDFM_CHCFGR1_SITP)         /*!< Manchester coded input on DATINy pin: rising edge = logic 1, falling edge = logic 0 */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_SPI_Clk SDFM Channel SPI Clock
 * @{
 */
#define SDFM_CH_SPI_CLK_EXT                         (0x0U)                      /*!< SPI clock coming from external CKINy input - sampling point according SITP[1:0] */
#define SDFM_CH_SPI_CLK_INTERN                      (SDFM_CHCFGR1_SPICKSEL_0)   /*!< SPI clock coming from internal CKOUT output - sampling point according SITP[1:0] */
#define SDFM_CH_SPI_CLK_INTERN_DIV2_FALLING         (SDFM_CHCFGR1_SPICKSEL_1)   /*!< SPI clock coming from internal CKOUT - sampling point on each second CKOUT falling edge. */
#define SDFM_CH_SPI_CLK_INTERN_DIV2_RISING          (SDFM_CHCFGR1_SPICKSEL)     /*!< SPI clock coming from internal CKOUT output - sampling point on each second CKOUT rising edge. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Clk_Sel SDFM Channel Clock Selection
 * @{
 */
#define SDFM_CH_CLK_SEL_SELF                        (0x0U)                      /*!< Clock source is its channel clock */
#define SDFM_CH_CLK_SEL_CH0                         (SDFM_CHCFGR1_SDCLKSEL)     /*!< Clock source is channel 0 clock */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Clk_Sync_Ctrl SDFM Channel Clock Synchronization Control
 * @{
 */
#define SDFM_CH_CLK_SYNC_DISABLE                    (0x0U)                      /*!< Channel clock is not passed through a synchronizer */
#define SDFM_CH_CLK_SYNC_ENABLE                     (SDFM_CHCFGR1_SDCLKSYNC)    /*!< Channel clock is passed through a synchronizer */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Data_Sync_Ctrl SDFM Channel Data Synchronization Control
 * @{
 */
#define SDFM_CH_DATA_SYNC_DISABLE                   (0x0U)                      /*!< Channel data is not passed through a synchronizer */
#define SDFM_CH_DATA_SYNC_ENABLE                    (SDFM_CHCFGR1_SDDATASYNC)   /*!< Channel data is passed through a synchronizer */
/**
 * @}
 */

/**
 * @defgroup SDFM_Ch_Awd_Sinc_Order SDFM Channel Analog Watchdog Filter Order
 * @{
 */
#define SDFM_CH_AWD_SINC_ORDER_FASTSINC             (0x0U)                      /*!< FastSinc filter type */
#define SDFM_CH_AWD_SINC_ORDER_SINC1                (SDFM_CHAWSCDR_AWFORD_0)    /*!< Sinc1 filter type */
#define SDFM_CH_AWD_SINC_ORDER_SINC2                (SDFM_CHAWSCDR_AWFORD_1)    /*!< Sinc2 filter type */
#define SDFM_CH_AWD_SINC_ORDER_SINC3                (SDFM_CHAWSCDR_AWFORD)      /*!< Sinc3 filter type */
/**
 * @}
 */

/**
 * @defgroup SDFM_Break_Signal SDFM Break Signal
 * @{
 */
#define SDFM_BREAK_SIGNAL_NONE                      (0x0U)                      /*!< No break signal */
#define SDFM_BREAK_SIGNAL0                          (0x1UL)                     /*!< Break signal 0 */
#define SDFM_BREAK_SIGNAL1                          (0x2UL)                     /*!< Break signal 1 */
#define SDFM_BREAK_SIGNAL2                          (0x4UL)                     /*!< Break signal 2 */
#define SDFM_BREAK_SIGNAL3                          (0x8UL)                     /*!< Break signal 3 */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Ext_Trigger SDFM Filter External Trigger
 * @{
 */
#define SDFM_FILTER_EXT_TRIG_JTRG0                  (0x00U)                     /*!< HRPWM_GLOB_AOS0 */
#define SDFM_FILTER_EXT_TRIG_JTRG1                  (0x01U)                     /*!< HRPWM_GLOB_AOS1 */
#define SDFM_FILTER_EXT_TRIG_JTRG2                  (0x02U)                     /*!< HRPWM_GLOB_AOS2 */
#define SDFM_FILTER_EXT_TRIG_JTRG3                  (0x03U)                     /*!< HRPWM_GLOB_AOS3 */
#define SDFM_FILTER_EXT_TRIG_JTRG4                  (0x04U)                     /*!< HRPWM_GLOB_AOS4 */
#define SDFM_FILTER_EXT_TRIG_JTRG5                  (0x05U)                     /*!< HRPWM_GLOB_AOS5 */
#define SDFM_FILTER_EXT_TRIG_JTRG6                  (0x06U)                     /*!< HRPWM_GLOB_AOS6 */
#define SDFM_FILTER_EXT_TRIG_JTRG7                  (0x07U)                     /*!< HRPWM_GLOB_AOS7 */
#define SDFM_FILTER_EXT_TRIG_JTRG8                  (0x08U)                     /*!< HRPWM_GLOB_AOS8 */
#define SDFM_FILTER_EXT_TRIG_JTRG9                  (0x09U)                     /*!< TMR6_1_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG10                 (0x0AU)                     /*!< TMR6_2_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG11                 (0x0BU)                     /*!< TMR6_3_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG12                 (0x0CU)                     /*!< TMR6_4_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG13                 (0x0DU)                     /*!< TMR6_5_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG14                 (0x0EU)                     /*!< TMR6_6_SCMA */
#define SDFM_FILTER_EXT_TRIG_JTRG15                 (0x0FU)                     /*!< TMR6_1_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG16                 (0x10U)                     /*!< TMR6_2_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG17                 (0x11U)                     /*!< TMR6_3_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG18                 (0x12U)                     /*!< TMR6_4_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG19                 (0x13U)                     /*!< TMR6_5_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG20                 (0x14U)                     /*!< TMR6_6_SCMB */
#define SDFM_FILTER_EXT_TRIG_JTRG24                 (0x18U)                     /*!< EXTI1 */
#define SDFM_FILTER_EXT_TRIG_JTRG25                 (0x19U)                     /*!< EXTI2 */
#define SDFM_FILTER_EXT_TRIG_JTRG26                 (0x1AU)                     /*!< TMR0_1_CMPA */
#define SDFM_FILTER_EXT_TRIG_JTRG27                 (0x1BU)                     /*!< TMR0_1_CMPB */
#define SDFM_FILTER_EXT_TRIG_JTRG28                 (0x1CU)                     /*!< TMR0_1_OVFA */
#define SDFM_FILTER_EXT_TRIG_JTRG29                 (0x1DU)                     /*!< TMR0_2_CMPA */
#define SDFM_FILTER_EXT_TRIG_JTRG30                 (0x1EU)                     /*!< TMR0_2_CMPB */
#define SDFM_FILTER_EXT_TRIG_JTRG31                 (0x1FU)                     /*!< TMR0_2_OVFA */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Ext_Trigger_Edge SDFM Filter External Trigger Edge
 * @{
 */
#define SDFM_FILTER_EXT_TRIG_DISABLE                (0x0U)                      /*!< Trigger detection is disabled */
#define SDFM_FILTER_EXT_TRIG_EDGE_RISING            (SDFM_FLTCR1_JEXTEN_0)      /*!< Each rising edge on the selected trigger makes a request to launch an injected conversion */
#define SDFM_FILTER_EXT_TRIG_EDGE_FALLING           (SDFM_FLTCR1_JEXTEN_1)      /*!< Each falling edge on the selected trigger makes a request to launch an injected conversion */
#define SDFM_FILTER_EXT_TRIG_EDGE_BOTH              (SDFM_FLTCR1_JEXTEN)        /*!< Both rising edges and falling edges on the selected trigger make requests to launch injected conversions */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Regu_Fast_Mode_Ctrl SDFM Filter Regular Fast Mode Control
 * @{
 */
#define SDFM_FILTER_REGU_FAST_MD_DISABLE            (0x0U)                      /*!< Fast conversion mode for regular conversion is disabled. */
#define SDFM_FILTER_REGU_FAST_MD_ENABLE             (SDFM_FLTCR1_FAST)          /*!< Fast conversion mode for regular conversion is enabled. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Regu_Dma_Ctrl SDFM Filter Regular DMA Control
 * @{
 */
#define SDFM_FILTER_REGU_DMA_DISABLE                (0x0U)                      /*!< The DMA channel is not enabled to read regular data. */
#define SDFM_FILTER_REGU_DMA_ENABLE                 (SDFM_FLTCR1_RDMAEN)        /*!< The DMA channel is enabled to read regular data. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Regu_Sync_Ctrl SDFM Filter Regular Sync Control
 * @{
 */
#define SDFM_FILTER_REGU_SYNC_START_DISABLE         (0x0U)                      /*!< Do not launch a regular conversion synchronously with SDFM_FLT0. */
#define SDFM_FILTER_REGU_SYNC_START_ENABLE          (SDFM_FLTCR1_RSYNC)         /*!< Launch a regular conversion in this SDFM_FLTx at the very moment when
                                                                                     a regular conversion is launched in SDFM_FLT0. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Regu_Cont_Ctrl SDFM Filter Regular Continuous Mode Control
 * @{
 */
#define SDFM_FILTER_REGU_CONT_MD_DISABLE            (0x0U)                      /*!< The regular channel is converted just once for each conversion request. */
#define SDFM_FILTER_REGU_CONT_MD_ENABLE             (SDFM_FLTCR1_RCONT)         /*!< The regular channel is converted repeatedly after each conversion request. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Inj_Sync_Ctrl SDFM Filter Injected Sync Control
 * @{
 */
#define SDFM_FILTER_INJ_SYNC_START_DISABLE          (0x0U)                       /*!< Do not launch an injected conversion synchronously with SDFM_FLT0. */
#define SDFM_FILTER_INJ_SYNC_START_ENABLE           (SDFM_FLTCR1_JSYNC)          /*!< Launch an injected conversion in this SDFM_FLTx at the very moment when an injected
                                                                                     conversion is launched in SDFM_FLT0 by its JSWSTART trigger. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Inj_Scan_Mode_Ctrl SDFM Filter Injected Scan Mode Control
 * @{
 */
#define SDFM_FILTER_INJ_SCAN_MD_DISABLE         (0x0U)                          /*!< One channel conversion is performed from the injected channel group and next
                                                                                     the selected channel from this group is selected. */
#define SDFM_FILTER_INJ_SCAN_MD_ENABLE          (SDFM_FLTCR1_JSCAN)             /*!< The series of conversions for the injected group channels is executed,
                                                                                     starting over with the lowest selected channel. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Inj_Dma_Ctrl SDFM Filter Injected DMA
 * @{
 */
#define SDFM_FILTER_INJ_DMA_DISABLE            (0x0U)                           /*!< The DMA channel is not enabled to read injected data. */
#define SDFM_FILTER_INJ_DMA_ENABLE             (SDFM_FLTCR1_JDMAEN)             /*!< The DMA channel is enabled to read injected data. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Inj_Type SDFM Filter Injected Conversion Type
 * @{
 */
#define SDFM_FILTER_INJ_TYPE_COMM              (0x0U)                           /*!< Common injected conversion. */
#define SDFM_FILTER_INJ_TYPE_FIFO              (SDFM_FLTCR1_JSCAN_TYPE)         /*!< FIFO injected conversion. */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Sinc_Order SDFM Filter Sinc Order
 * @{
 */
#define SDFM_FILTER_SINC_ORDER_FASTSINC              (0x0U)                          /*!< FastSinc filter type */
#define SDFM_FILTER_SINC_ORDER_SINC1                 (1UL << SDFM_FLTFCR_FORD_POS)   /*!< Sinc1 filter type */
#define SDFM_FILTER_SINC_ORDER_SINC2                 (2UL << SDFM_FLTFCR_FORD_POS)   /*!< Sinc2 filter type */
#define SDFM_FILTER_SINC_ORDER_SINC3                 (3UL << SDFM_FLTFCR_FORD_POS)   /*!< Sinc3 filter type */
#define SDFM_FILTER_SINC_ORDER_SINC4                 (4UL << SDFM_FLTFCR_FORD_POS)   /*!< Sinc4 filter type */
#define SDFM_FILTER_SINC_ORDER_SINC5                 (5UL << SDFM_FLTFCR_FORD_POS)   /*!< Sinc5 filter type */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Awd_Data_Src SDFM Filter Analog Watchdog Data Source
 * @{
 */
#define SDFM_FILTER_AWD_FILTER_DATA                 (0x0U)                      /*!< Analog watchdog on data output value (after the digital filter).
                                                                                     The comparison is done after offset correction and shift */
#define SDFM_FILTER_AWD_CH_DATA                     (SDFM_FLTCR1_AWFSEL)        /*!< Analog watchdog on channel transceivers value (after watchdog filter) */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Ovf_Deal SDFM Filter Overflow Deal
 * @{
 */
#define SDFM_FILTER_OVF_CLAMPING                    (0x0U)                      /*!< If the conversion result is greater than the maximum value, then take the maximum value.
                                                                                     If the conversion result is less than the minimum value, then take the minimum value. */
#define SDFM_FILTER_OVF_CUTTING                     (SDFM_FLTCR1_OVDEL)         /*!< Cut off the highest position */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Int SDFM Filter Interrupt
 * @{
 */
#define SDFM_FILTER_INT_INJ_EOC                     (SDFM_FLTCR2_JEOCIE)        /*!< Injected end of conversion interrupt */
#define SDFM_FILTER_INT_REGU_EOC                    (SDFM_FLTCR2_REOCIE)        /*!< Regular end of conversion interrupt */
#define SDFM_FILTER_INT_INJ_OVERRUN                 (SDFM_FLTCR2_JOVRIE)        /*!< Injected data overrun interrupt */
#define SDFM_FILTER_INT_REGU_OVERRUN                (SDFM_FLTCR2_ROVRIE)        /*!< Regular data overrun interrupt */
#define SDFM_FILTER_INT_AWD                         (SDFM_FLTCR2_AWDIE)         /*!< Analog watchdog interrupt */
#define SDFM_FILTER_INT_CH_CROSS_ZERO               (1UL << 31U)                /*!< Cross zero interrupt of the channel which the filter used */

#define SDFM_FILTER_INT_ALL                         (SDFM_FILTER_INT_INJ_EOC | SDFM_FILTER_INT_REGU_EOC | \
                                                     SDFM_FILTER_INT_INJ_OVERRUN | SDFM_FILTER_INT_REGU_OVERRUN | \
                                                     SDFM_FILTER_INT_AWD | SDFM_FILTER_INT_CH_CROSS_ZERO)
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Ch_Int SDFM Filter Channel Interrupt
 * @{
 */
#define SDFM_FILTER_CH_INT_SCD                 (SDFM_FLTCR2_SCDIE)              /*!< Short-circuit detector interrupt */
#define SDFM_FILTER_CH_INT_CKAB                (SDFM_FLTCR2_CKABIE)             /*!< Clock absence interrupt */
#define SDFM_FILTER_CH_INT_ALL                 (SDFM_FILTER_CH_INT_SCD | SDFM_FILTER_CH_INT_CKAB)
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Status_Flag SDFM Filter Status Flag
 * @{
 */
#define SDFM_FILTER_FLAG_INJ_EOC                    (SDFM_FLTISR_JEOCF)         /*!< End of injected conversion flag.
                                                                                     Need cleared by software only when FIFO injected conversion used(SDFM_FLTxCR1.JSCAN_TYPE=1) */
#define SDFM_FILTER_FLAG_REGU_EOC                   (SDFM_FLTISR_REOCF)         /*!< End of regular conversion flag */
#define SDFM_FILTER_FLAG_INJ_OVERRUN                (SDFM_FLTISR_JOVRF)         /*!< Injected conversion overrun flag */
#define SDFM_FILTER_FLAG_REGU_OVERRUN               (SDFM_FLTISR_ROVRF)         /*!< Regular conversion overrun flag */
#define SDFM_FILTER_FLAG_AWD                        (SDFM_FLTISR_AWDF)          /*!< The analog watchdog block detected voltage which crosses the value
                                                                                     programmed in the SDFM_FLTxAWLTR or SDFM_FLTxAWHTR registers.
                                                                                     This bit is set by hardware. It is cleared by software by clearing
                                                                                     all source flag bits AWHTF[7:0] and AWLTF[7:0] in SDFM_FLTxAWSR
                                                                                     register (by writing ‘1’ into the clear bits in SDFM_FLTxAWCFR register). */
#define SDFM_FILTER_FLAG_INJ_BUSY                   (SDFM_FLTISR_JCIP)          /*!< Injected conversion in progress */
#define SDFM_FILTER_FLAG_REGU_BUSY                  (SDFM_FLTISR_RCIP)          /*!< Regular conversion in progress */

#define SDFM_FILTER_FLAG_ALL                        (SDFM_FILTER_FLAG_INJ_EOC | SDFM_FILTER_FLAG_REGU_EOC | \
                                                     SDFM_FILTER_FLAG_INJ_OVERRUN | SDFM_FILTER_FLAG_REGU_OVERRUN | \
                                                     SDFM_FILTER_FLAG_AWD | SDFM_FILTER_FLAG_INJ_BUSY | \
                                                     SDFM_FILTER_FLAG_REGU_BUSY)
#define SDFM_FILTER_FLAG_CLR_ALL                    (SDFM_FILTER_FLAG_INJ_EOC | SDFM_FILTER_FLAG_INJ_OVERRUN | \
                                                     SDFM_FILTER_FLAG_REGU_OVERRUN)
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Ch_Status_Flag SDFM Filter Channel Status Flag
 * @{
 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH0                (1UL << 16U)                /*!< Clock absence is detected on channel 0 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH1                (1UL << 17U)                /*!< Clock absence is detected on channel 1 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH2                (1UL << 18U)                /*!< Clock absence is detected on channel 2 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH3                (1UL << 19U)                /*!< Clock absence is detected on channel 3 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH4                (1UL << 20U)                /*!< Clock absence is detected on channel 4 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH5                (1UL << 21U)                /*!< Clock absence is detected on channel 5 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH6                (1UL << 22U)                /*!< Clock absence is detected on channel 6 */
#define SDFM_FILTER_CH_FLAG_CKAB_CH7                (1UL << 23U)                /*!< Clock absence is detected on channel 7 */
#define SDFM_FILTER_CH_FLAG_CKAB_ALL                (0x00FF0000UL)

#define SDFM_FILTER_CH_FLAG_SCD_CH0                 (1UL << 24U)                /*!< Short-circuit detector event occurred on channel 0 */
#define SDFM_FILTER_CH_FLAG_SCD_CH1                 (1UL << 25U)                /*!< Short-circuit detector event occurred on channel 1 */
#define SDFM_FILTER_CH_FLAG_SCD_CH2                 (1UL << 26U)                /*!< Short-circuit detector event occurred on channel 2 */
#define SDFM_FILTER_CH_FLAG_SCD_CH3                 (1UL << 27U)                /*!< Short-circuit detector event occurred on channel 3 */
#define SDFM_FILTER_CH_FLAG_SCD_CH4                 (1UL << 28U)                /*!< Short-circuit detector event occurred on channel 4 */
#define SDFM_FILTER_CH_FLAG_SCD_CH5                 (1UL << 29U)                /*!< Short-circuit detector event occurred on channel 5 */
#define SDFM_FILTER_CH_FLAG_SCD_CH6                 (1UL << 30U)                /*!< Short-circuit detector event occurred on channel 6 */
#define SDFM_FILTER_CH_FLAG_SCD_CH7                 (1UL << 31U)                /*!< Short-circuit detector event occurred on channel 7 */
#define SDFM_FILTER_CH_FLAG_SCD_ALL                 (0xFF000000UL)

#define SDFM_FILTER_CH_FLAG_ALL                     (SDFM_FILTER_CH_FLAG_CKAB_ALL | SDFM_FILTER_CH_FLAG_SCD_ALL)
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Awd_Status_Flag SDFM Filter Analog Watchdog Status Flag
 * @{
 */
#define SDFM_FILTER_AWD_FLAG_LT_CH0                 (1UL << 0U)                 /*!< A low threshold error on channel 0 */
#define SDFM_FILTER_AWD_FLAG_LT_CH1                 (1UL << 1U)                 /*!< A low threshold error on channel 1 */
#define SDFM_FILTER_AWD_FLAG_LT_CH2                 (1UL << 2U)                 /*!< A low threshold error on channel 2 */
#define SDFM_FILTER_AWD_FLAG_LT_CH3                 (1UL << 3U)                 /*!< A low threshold error on channel 3 */
#define SDFM_FILTER_AWD_FLAG_LT_CH4                 (1UL << 4U)                 /*!< A low threshold error on channel 4 */
#define SDFM_FILTER_AWD_FLAG_LT_CH5                 (1UL << 5U)                 /*!< A low threshold error on channel 5 */
#define SDFM_FILTER_AWD_FLAG_LT_CH6                 (1UL << 6U)                 /*!< A low threshold error on channel 6 */
#define SDFM_FILTER_AWD_FLAG_LT_CH7                 (1UL << 7U)                 /*!< A low threshold error on channel 7 */

#define SDFM_FILTER_AWD_FLAG_HT_CH0                 (1UL << 8U)                 /*!< A high threshold error on channel 0 */
#define SDFM_FILTER_AWD_FLAG_HT_CH1                 (1UL << 9U)                 /*!< A high threshold error on channel 1 */
#define SDFM_FILTER_AWD_FLAG_HT_CH2                 (1UL << 10U)                /*!< A high threshold error on channel 2 */
#define SDFM_FILTER_AWD_FLAG_HT_CH3                 (1UL << 11U)                /*!< A high threshold error on channel 3 */
#define SDFM_FILTER_AWD_FLAG_HT_CH4                 (1UL << 12U)                /*!< A high threshold error on channel 4 */
#define SDFM_FILTER_AWD_FLAG_HT_CH5                 (1UL << 13U)                /*!< A high threshold error on channel 5 */
#define SDFM_FILTER_AWD_FLAG_HT_CH6                 (1UL << 14U)                /*!< A high threshold error on channel 6 */
#define SDFM_FILTER_AWD_FLAG_HT_CH7                 (1UL << 15U)                /*!< A high threshold error on channel 7 */

#define SDFM_FILTER_AWD_FLAG_HZF_CH0                (1UL << 16U)                /*!< A crossing zero on channel 0 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH1                (1UL << 17U)                /*!< A crossing zero on channel 1 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH2                (1UL << 18U)                /*!< A crossing zero on channel 2 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH3                (1UL << 19U)                /*!< A crossing zero on channel 3 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH4                (1UL << 20U)                /*!< A crossing zero on channel 4 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH5                (1UL << 21U)                /*!< A crossing zero on channel 5 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH6                (1UL << 22U)                /*!< A crossing zero on channel 6 */
#define SDFM_FILTER_AWD_FLAG_HZF_CH7                (1UL << 23U)                /*!< A crossing zero on channel 7 */

#define SDFM_FILTER_AWD_FLAG_HT_ALL                 (0x0000FFUL)
#define SDFM_FILTER_AWD_FLAG_LT_ALL                 (0x00FF00UL)
#define SDFM_FILTER_AWD_FLAG_HZF_ALL                (0xFF0000UL)
#define SDFM_FILTER_AWD_FLAG_ALL                    (0xFFFFFFUL)
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Cevt_Input_Sel SDFM Filter Comparator Event Input Selection
 * @{
 */
#define SDFM_FILTER_CEVT1_INPUT_COMPH1              (0x0UL)                     /*!< Comparator event1 input selects COMPH1 */
#define SDFM_FILTER_CEVT1_INPUT_COMPH1_OR_COMPL1    (0x1UL)                     /*!< Comparator event1 input selects COMPH1 or COMPL1 */
#define SDFM_FILTER_CEVT1_INPUT_COMPH2              (0x2UL)                     /*!< Comparator event1 input selects COMPH2 */
#define SDFM_FILTER_CEVT1_INPUT_COMPH2_OR_COMPL2    (0x3UL)                     /*!< Comparator event1 input selects COMPH2 or COMPL2 */

#define SDFM_FILTER_CEVT2_INPUT_COMPL1              (0x0UL)                     /*!< Comparator event2 input selects COMPL1 */
#define SDFM_FILTER_CEVT2_INPUT_COMPL1_OR_COMPH1    (0x1UL)                     /*!< Comparator event2 input selects COMPL1 or COMPH1 */
#define SDFM_FILTER_CEVT2_INPUT_COMPL2              (0x2UL)                     /*!< Comparator event2 input selects COMPL2 */
#define SDFM_FILTER_CEVT2_INPUT_COMPL2_OR_COMPH2    (0x3UL)                     /*!< Comparator event2 input selects COMPL2 or COMPH2 */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Cevt_Output_Sel SDFM Filter Comparator Event Output Selection
 * @{
 */
#define SDFM_FILTER_CEVT_OUTPUT_RAW                 (0x0UL)                     /*!< Comparator event output selects the raw event not filtered */
#define SDFM_FILTER_CEVT_OUTPUT_FILTERED            (0x1UL)                     /*!< Comparator event output selects the raw event after filtered */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Fifo_Wait_For_Sync SDFM Filter FIFO Wait For Synchronization
 * @{
 */
#define SDFM_FILTER_FIFO_WAIT_FOR_SYNC_DISABLE      (0x0UL)                         /*!< Incoming Data written to SDFIFO on every Data-Ready(DR) Event */
#define SDFM_FILTER_FIFO_WAIT_FOR_SYNC_ENABLE       (SDFM_FLTFIFOCTL_WTSYNCEN)      /*!< Incoming Data written to SDFIFO on DR event only after SDFM_JTRG event occurs */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Fifo_Clear_On_Sync SDFM Filter FIFO Clear On Synchronization
 * @{
 */
#define SDFM_FILTER_FIFO_CLR_ON_SYNC_DISABLE        (0x0UL)                         /*!< FIFO is not automatically cleared upon receiving SDFM_JTRG */
#define SDFM_FILTER_FIFO_CLR_ON_SYNC_ENABLE         (SDFM_FLTFIFOCTL_FFSYNCLREN)    /*!< FIFO is automatically cleared upon receiving SDFM_JTRG */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Fifo_Flag_Clear_On_Int SDFM Filter FIFO Flag Clear On Interrupt
 * @{
 */
#define SDFM_FILTER_FIFO_FLAG_CLR_ON_INT_DISABLE    (0x0UL)                         /*!< WTSYNFLAG can only be cleared manually */
#define SDFM_FILTER_FIFO_FLAG_CLR_ON_INT_ENABLE     (SDFM_FLTFIFOCTL_WTSCLREN)      /*!< WTSYNFLAG is cleared automatically on JCIP(JSCAN_TYPE==1) */
/**
 * @}
 */

/**
 * @defgroup SDFM_Filter_Fifo_Flag SDFM Filter FIFO Flag
 * @{
 */
#define SDFM_FILTER_FIFO_FLAG_WAIT_FOR_SYNC        (SDFM_FLTFIFOCTL_WTSYNFLG)
#define SDFM_FILTER_FIFO_FLAG_ALL                  (SDFM_FILTER_FIFO_FLAG_WAIT_FOR_SYNC)
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
 * @addtogroup SDFM_Global_Functions
 * @{
 */
int32_t SDFM_CH_Init(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, const stc_sdfm_ch_init_t *pstcSdfmChInit);
int32_t SDFM_CH_StructInit(stc_sdfm_ch_init_t *pstcSdfmChInit);
void SDFM_CH_Cmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState);
int32_t SDFM_FILTER_Init(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_init_t *pstcSdfmFilterInit);
int32_t SDFM_FILTER_StructInit(stc_sdfm_filter_init_t *pstcSdfmFilterInit);
void SDFM_FILTER_IntCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FilterInt, en_functional_state_t enNewState);
void SDFM_FILTER_ChIntCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChInt, en_functional_state_t enNewState);
void SDFM_FILTER_Cmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_Cmd(CM_SDFM_TypeDef *SDFMx, en_functional_state_t enNewState);
int32_t SDFM_DeInit(CM_SDFM_TypeDef *SDFMx);

/***********************************************************************************************************************
***********************************************************************************************************************/
void SDFM_CH_ClockOutConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32ClockOutSrc, uint32_t u32ClockOutDiv);
void SDFM_CH_DisableClockOut(CM_SDFM_TypeDef *SDFMx);
int32_t SDFM_CH_ClockAbsenceDetectorCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState);
void SDFM_CH_ShortCircuitDetectorConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32Threshold, uint32_t u32BreakSignal);
void SDFM_CH_ShortCircuitDetectorCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState);
void SDFM_CH_SelectInputMux(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32InputMux);
void SDFM_CH_SetDataPacking(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32DataPacking);
void SDFM_CH_SelectInput(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32InputSel);
void SDFM_CH_SetSerialIntfType(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32IntfType);
void SDFM_CH_SetSpiClock(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32SpiClk);
void SDFM_CH_AwdConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32AwdSincOrder, uint32_t u32AwdSincFOSR);
int16_t SDFM_CH_GetAwdValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch);
void SDFM_CH_SetCalOffset(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int32_t i32CalOffset);
void SDFM_CH_WriteInputDataStdMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16InputData);
void SDFM_CH_WriteInputDataInterleavedMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16FirstSample, int16_t i16SecondSample);
void SDFM_CH_WriteInputDataDualMode(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, int16_t i16YSample, int16_t i16Y1Sample);
void SDFM_CH_SetDelay(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint8_t u8Delay);

void SDFM_CH_ClockSelect(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, uint32_t u32ClockSel);
void SDFM_CH_ClockSyncCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState);
void SDFM_CH_DataSyncCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Ch, en_functional_state_t enNewState);

/***********************************************************************************************************************
***********************************************************************************************************************/
en_flag_status_t SDFM_FILTER_GetStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag);
en_flag_status_t SDFM_FILTER_GetChStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChFlag);
void SDFM_FILTER_ClearStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag);
int32_t SDFM_FILTER_ClearChStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32ChFlag);
int32_t SDFM_FILTER_RegularConvConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_regular_conv_t *pstcRegu);
void SDFM_FILTER_RegularConvContModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_RegularConvSyncStartCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_RegularConvDmaReadCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_RegularConvFastModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_SetRegularConvCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32ReguCh);
void SDFM_FILTER_StartRegularConv(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
int32_t SDFM_FILTER_GetRegularConvValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch);
int32_t SDFM_FILTER_InjectedConvConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_injected_conv_t *pstcInj);
void SDFM_FILTER_InjectedConvSyncStartCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_InjectedConvScanModeCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_InjectedConvDmaReadCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);
void SDFM_FILTER_SetInjectedConvType(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32InjType);
void SDFM_FILTER_SetInjectedConvExtTrigger(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32ExtTrigger, uint32_t u32ExtTriggerEdge);
void SDFM_FILTER_SetInjectedConvCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32InjCh);
void SDFM_FILTER_StartInjectedConv(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
int32_t SDFM_FILTER_GetInjectedConvValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch);

/***********************************************************************************************************************
***********************************************************************************************************************/
int32_t SDFM_FILTER_FifoConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_fifo_config_t *pstcFifo);
int32_t SDFM_FILTER_FifoStructInit(stc_sdfm_filter_fifo_config_t *pstcFifo);
uint32_t SDFM_FILTER_GetFifoStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
int32_t SDFM_FILTER_ReadFifo(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch);
en_flag_status_t SDFM_FILTER_GetFifoFlagStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FifoFlag);
void SDFM_FILTER_ClearFifoFlagStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32FifoFlag);

/***********************************************************************************************************************
***********************************************************************************************************************/
int32_t SDFM_FILTER_CevtConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Cevt, const stc_sdfm_filter_cevt_config_t *pstcCevt);
int32_t SDFM_FILTER_CevtStructInit(stc_sdfm_filter_cevt_config_t *pstcCevt);
void SDFM_FILTER_ResetCevtDigitalFifo(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
void SDFM_FILTER_CevtDigitalFilterSetInputCh(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Ch);
void SDFM_FILTER_CevtDigitalFilterCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, en_functional_state_t enNewState);

/***********************************************************************************************************************
***********************************************************************************************************************/
void SDFM_FILTER_ExtremesChCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32ExtrCh, en_functional_state_t enNewState);
void SDFM_FILTER_ResetExtremes(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
int32_t SDFM_FILTER_GetExtremeMaxValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch);
int32_t SDFM_FILTER_GetExtremeMinValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t *pu32Ch);
uint32_t SDFM_FILTER_GetConvTimeValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
uint32_t SDFM_FILTER_GetConvThreValue(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter);
int32_t SDFM_FILTER_AwdConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_awd_t *pstcAwd);
int32_t SDFM_FILTER_AwdStructInit(stc_sdfm_filter_awd_t *pstcAwd);
void SDFM_FILTER_AwdChCmd(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32AwdCh, en_functional_state_t enNewState);
void SDFM_FILTER_SetAwdDataSrc(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32DataSrc);
void SDFM_FILTER_SetAwdThreshold(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, int32_t i32HighThreshold, int32_t i32LowThreshold);
void SDFM_FILTER_SetAwdThreshold2(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, int16_t i16HighThreshold2, int16_t i16LowThreshold2);
void SDFM_FILTER_SetAwdBreakSignal(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter,
                                   uint32_t u32HighBreakSignal, uint32_t u32LowBreakSignal,
                                   uint32_t u32CrossZeroBreakSignal);
en_flag_status_t SDFM_FILTER_GetAwdStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag);
void SDFM_FILTER_ClearAwdStatus(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, uint32_t u32Flag);
int32_t SDFM_FILTER_SincConfig(CM_SDFM_TypeDef *SDFMx, uint32_t u32Filter, const stc_sdfm_filter_sinc_t *pstcSinc);

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

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_SDFM_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
