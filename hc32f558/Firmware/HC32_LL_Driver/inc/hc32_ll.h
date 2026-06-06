/**
 *******************************************************************************
 * @file  hc32_ll.h
 * @brief This file contains HC32 Series Device Driver Library file call
 *        management.
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
#ifndef __HC32_LL_H__
#define __HC32_LL_H__

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
 * @addtogroup LL_Global
 * @{
 */

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup LL_Global_Macros LL Global Macros
 * @{
 */

/**
 * @defgroup Peripheral_Register_WP_Global_Macros Peripheral Register Write Protection Global Macros
 * @{
 */
#define LL_PERIPH_EFM           (1UL << 0U)
#define LL_PERIPH_FCG           (1UL << 1U)
#define LL_PERIPH_GPIO          (1UL << 2U)
#define LL_PERIPH_INTC          (1UL << 3U)
#define LL_PERIPH_LVD           (1UL << 4U)
#define LL_PERIPH_MPU           (1UL << 5U)
#define LL_PERIPH_PWC_CLK_RMU   (1UL << 6U)
#define LL_PERIPH_SRAM          (1UL << 7U)
#define LL_PERIPH_EFM_OTP       (1UL << 8U)
#define LL_PERIPH_DEFM          (1UL << 9U)
#define LL_PERIPH_DEFM_OTP      (1UL << 10U)

#define LL_PERIPH_ALL           (LL_PERIPH_FCG | LL_PERIPH_GPIO | LL_PERIPH_INTC  | LL_PERIPH_LVD | LL_PERIPH_MPU | \
                                 LL_PERIPH_SRAM | LL_PERIPH_PWC_CLK_RMU | LL_PERIPH_EFM | LL_PERIPH_EFM_OTP| LL_PERIPH_DEFM| \
                                 LL_PERIPH_DEFM_OTP)
/**
 * @}
 */

/**
 * @defgroup Timer_SW_Sync_Start_Global_Macros Timer Software Sync Start Global Macros
 * @{
 */
#define LL_SW_SYNC_START_TMR0_1_CH_A        PERIC_TMR_SYNENR_TMR0_1_A
#define LL_SW_SYNC_START_TMR0_1_CH_B        PERIC_TMR_SYNENR_TMR0_1_B
#define LL_SW_SYNC_START_TMR0_2_CH_A        PERIC_TMR_SYNENR_TMR0_2_A
#define LL_SW_SYNC_START_TMR0_2_CH_B        PERIC_TMR_SYNENR_TMR0_2_B
#define LL_SW_SYNC_START_HRPWM_1            PERIC_TMR_SYNENR_HRPWM_1
#define LL_SW_SYNC_START_HRPWM_2            PERIC_TMR_SYNENR_HRPWM_2
#define LL_SW_SYNC_START_HRPWM_3            PERIC_TMR_SYNENR_HRPWM_3
#define LL_SW_SYNC_START_HRPWM_4            PERIC_TMR_SYNENR_HRPWM_4
#define LL_SW_SYNC_START_HRPWM_5            PERIC_TMR_SYNENR_HRPWM_5
#define LL_SW_SYNC_START_HRPWM_6            PERIC_TMR_SYNENR_HRPWM_6
#define LL_SW_SYNC_START_HRPWM_7            PERIC_TMR_SYNENR_HRPWM_7
#define LL_SW_SYNC_START_HRPWM_8            PERIC_TMR_SYNENR_HRPWM_8
#define LL_SW_SYNC_START_TMR6_1             PERIC_TMR_SYNENR_TMR6_1
#define LL_SW_SYNC_START_TMR6_2             PERIC_TMR_SYNENR_TMR6_2
#define LL_SW_SYNC_START_TMR6_3             PERIC_TMR_SYNENR_TMR6_3
#define LL_SW_SYNC_START_TMR6_4             PERIC_TMR_SYNENR_TMR6_4
#define LL_SW_SYNC_START_TMR6_5             PERIC_TMR_SYNENR_TMR6_5
#define LL_SW_SYNC_START_TMR6_6             PERIC_TMR_SYNENR_TMR6_6
#define LL_SW_SYNC_START_TMR_ALL            (LL_SW_SYNC_START_TMR0_1_CH_A | LL_SW_SYNC_START_TMR0_1_CH_B | \
                                             LL_SW_SYNC_START_TMR0_2_CH_A | LL_SW_SYNC_START_TMR0_2_CH_B | \
                                             LL_SW_SYNC_START_HRPWM_1     | LL_SW_SYNC_START_HRPWM_2     | \
                                             LL_SW_SYNC_START_HRPWM_3     | LL_SW_SYNC_START_HRPWM_4     | \
                                             LL_SW_SYNC_START_HRPWM_5     | LL_SW_SYNC_START_HRPWM_6     | \
                                             LL_SW_SYNC_START_HRPWM_7     | LL_SW_SYNC_START_HRPWM_8     | \
                                             LL_SW_SYNC_START_TMR6_1      | LL_SW_SYNC_START_TMR6_2      | \
                                             LL_SW_SYNC_START_TMR6_3      | LL_SW_SYNC_START_TMR6_4      | \
                                             LL_SW_SYNC_START_TMR6_5      | LL_SW_SYNC_START_TMR6_6)
/**
 * @}
 */

/* Defined use Device Driver Library */
#if !defined (USE_DDL_DRIVER)
/**
 * @brief Comment the line below if you will not use the Device Driver Library.
 * In this case, the application code will be based on direct access to
 * peripherals registers.
 */
/* #define USE_DDL_DRIVER */
#endif /* USE_DDL_DRIVER */

/**
* @defgroup HC32_Series_DDL_Release_Version HC32 Series DDL Release Version
* @{
*/
#define HC32_DDL_REV_MAIN               0x00U  /*!< [31:24] main version  */
#define HC32_DDL_REV_SUB1               0x00U  /*!< [23:16] sub1 version  */
#define HC32_DDL_REV_SUB2               0x01U  /*!< [15:8]  sub2 version  */
#define HC32_DDL_REV_PATCH              0x0BU  /*!< [7:0]   patch version */
#define HC32_DDL_REV                    ((HC32_DDL_REV_MAIN << 24) | (HC32_DDL_REV_SUB1 << 16) | \
                                         (HC32_DDL_REV_SUB2 << 8 ) | (HC32_DDL_REV_PATCH))
/**
 * @}
 */

/**
 * @}
 */

/* Use Device Driver Library */
#if defined (USE_DDL_DRIVER)

/**
 * @brief Include peripheral module's header file
 */
#if (LL_ADC_ENABLE == DDL_ON)
#include "hc32_ll_adc.h"
#endif /* LL_ADC_ENABLE */

#if (LL_AOS_ENABLE == DDL_ON)
#include "hc32_ll_aos.h"
#endif /* LL_AOS_ENABLE */

#if (LL_CLK_ENABLE == DDL_ON)
#include "hc32_ll_clk.h"
#endif /* LL_CLK_ENABLE */

#if (LL_CMP_ENABLE == DDL_ON)
#include "hc32_ll_cmp.h"
#endif /* LL_CMP_ENABLE */

#if (LL_CORDIC_ENABLE == DDL_ON)
#include "hc32_ll_cordic.h"
#endif /* LL_CORDIC_ENABLE */

#if (LL_CRC_ENABLE == DDL_ON)
#include "hc32_ll_crc.h"
#endif /* LL_CRC_ENABLE */

#if (LL_CTC_ENABLE == DDL_ON)
#include "hc32_ll_ctc.h"
#endif /* LL_CTC_ENABLE */

#if (LL_DAC_ENABLE == DDL_ON)
#include "hc32_ll_dac.h"
#endif /* LL_DAC_ENABLE */

#if (LL_DBGC_ENABLE == DDL_ON)
#include "hc32_ll_dbgc.h"
#endif /* LL_DBGC_ENABLE */

#if (LL_DMA_ENABLE == DDL_ON)
#include "hc32_ll_dma.h"
#endif /* LL_DMA_ENABLE */

#if (LL_EFM_ENABLE == DDL_ON)
#include "hc32_ll_efm.h"
#endif /* LL_EFM_ENABLE */

#if (LL_DSOGI_PLL_ENABLE == DDL_ON)
#include "hc32_ll_dsogi_pll.h"
#endif /* LL_DSOGI_PLL_ENABLE */

#if (LL_EMB_ENABLE == DDL_ON)
#include "hc32_ll_emb.h"
#endif /* LL_EMB_ENABLE */

#if (LL_ERMU_ENABLE == DDL_ON)
#include "hc32_ll_ermu.h"
#endif /* LL_ERMU_ENABLE */

#if (LL_EVENT_PORT_ENABLE == DDL_ON)
#include "hc32_ll_event_port.h"
#endif /* LL_EVENT_PORT_ENABLE */

#if (LL_FCG_ENABLE == DDL_ON)
#include "hc32_ll_fcg.h"
#endif /* LL_FCG_ENABLE */

#if (LL_FCM_ENABLE == DDL_ON)
#include "hc32_ll_fcm.h"
#endif /* LL_FCM_ENABLE */

#if (LL_FMAC_ENABLE == DDL_ON)
#include "hc32_ll_fmac.h"
#endif /* LL_FMAC_ENABLE */

#if (LL_GPIO_ENABLE == DDL_ON)
#include "hc32_ll_gpio.h"
#endif /* LL_GPIO_ENABLE */

#if (LL_HASH_ENABLE == DDL_ON)
#include "hc32_ll_hash.h"
#endif /* LL_HASH_ENABLE */

#if (LL_HRPWM_ENABLE == DDL_ON)
#include "hc32_ll_hrpwm.h"
#endif /* LL_HRPWM_ENABLE */

#if (LL_I2C_ENABLE == DDL_ON)
#include "hc32_ll_i2c.h"
#endif /* LL_I2C_ENABLE */

#if (LL_ICG_ENABLE == DDL_ON)
#include "hc32_ll_icg.h"
#endif /* LL_ICG_ENABLE */

#if (LL_INTERRUPTS_ENABLE == DDL_ON)
#include "hc32_ll_interrupts.h"
#endif /* LL_INTERRUPTS_ENABLE */

#if (LL_MCAN_ENABLE == DDL_ON)
#include "hc32_ll_mcan.h"
#endif /* LL_MCAN_ENABLE */

#if (LL_MPU_ENABLE == DDL_ON)
#include "hc32_ll_mpu.h"
#endif /* LL_MPU_ENABLE */

#if (LL_OTS_ENABLE == DDL_ON)
#include "hc32_ll_ots.h"
#endif /* LL_OTS_ENABLE */

#if (LL_PID_ENABLE == DDL_ON)
#include "hc32_ll_pid.h"
#endif /* LL_PID_ENABLE */

#if (LL_PLA_ENABLE == DDL_ON)
#include "hc32_ll_pla.h"
#endif /* LL_PLA_ENABLE */

#if (LL_PWC_ENABLE == DDL_ON)
#include "hc32_ll_pwc.h"
#endif /* LL_PWC_ENABLE */

#if (LL_RMU_ENABLE == DDL_ON)
#include "hc32_ll_rmu.h"
#endif /* LL_RMU_ENABLE */

#if (LL_SDFM_ENABLE == DDL_ON)
#include "hc32_ll_sdfm.h"
#endif /* LL_SDFM_ENABLE */

#if (LL_SKE_ENABLE == DDL_ON)
#include "hc32_ll_ske.h"
#endif /* LL_SKE_ENABLE */

#if (LL_SPI_ENABLE == DDL_ON)
#include "hc32_ll_spi.h"
#endif /* LL_SPI_ENABLE */

#if (LL_SRAM_ENABLE == DDL_ON)
#include "hc32_ll_sram.h"
#endif /* LL_SRAM_ENABLE */

#if (LL_SWDT_ENABLE == DDL_ON)
#include "hc32_ll_swdt.h"
#endif /* LL_SWDT_ENABLE */

#if (LL_TMR0_ENABLE == DDL_ON)
#include "hc32_ll_tmr0.h"
#endif /* LL_TMR0_ENABLE */

#if (LL_TMR6_ENABLE == DDL_ON)
#include "hc32_ll_tmr6.h"
#endif /* LL_TMR6_ENABLE */

#if (LL_TRNG_ENABLE == DDL_ON)
#include "hc32_ll_trng.h"
#endif /* LL_TRNG_ENABLE */

#if (LL_TRLPWM_ENABLE == DDL_ON)
#include "hc32_ll_trlpwm.h"
#endif /* LL_TRLPWM_ENABLE */

#if (LL_USART_ENABLE == DDL_ON)
#include "hc32_ll_usart.h"
#endif /* LL_USART_ENABLE */

#if (LL_UTILITY_ENABLE == DDL_ON)
#include "hc32_ll_utility.h"
#endif /* LL_UTILITY_ENABLE */

#if (LL_VREF_ENABLE == DDL_ON)
#include "hc32_ll_vref.h"
#endif /* LL_VREF_ENABLE */

#if (LL_WDT_ENABLE == DDL_ON)
#include "hc32_ll_wdt.h"
#endif /* LL_WDT_ENABLE */

#if (LL_XBAR_ENABLE == DDL_ON)
#include "hc32_ll_xbar.h"
#endif /* LL_WDT_ENABLE */

#endif /* USE_DDL_DRIVER */

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup LL_Global_Functions
 * @{
 */
void LL_PERIPH_WE(uint32_t u32Peripheral);
void LL_PERIPH_WP(uint32_t u32Peripheral);
void LL_TMR_SWSyncStart(uint32_t u32Tmr);
/**
 * @}
 */

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
