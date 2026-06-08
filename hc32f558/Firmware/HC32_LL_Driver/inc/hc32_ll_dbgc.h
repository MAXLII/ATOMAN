/**
 *******************************************************************************
 * @file  hc32_ll_dbgc.h
 * @brief This file contains all the functions prototypes of the DBGC driver
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
#ifndef __HC32_LL_DBGC_H__
#define __HC32_LL_DBGC_H__

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
 * @addtogroup LL_DBGC
 * @{
 */

#if (LL_DBGC_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/
/**
 * @defgroup DBGC_Global_Macros DBGC Global Macros
 * @{
 */

/**
 * @defgroup DBGC_Periph_Sel DBGC Periph Selection
 * @{
 */
#define DBGC_PERIPH_SWDT                    (DBGC_MCUSTPCTL_SWDTSTP)
#define DBGC_PERIPH_WDT                     (DBGC_MCUSTPCTL_WDTSTP)
#define DBGC_PERIPH_PVD0                    (DBGC_MCUSTPCTL_PVD0STP)
#define DBGC_PERIPH_PVD1                    (DBGC_MCUSTPCTL_PVD1STP)
#define DBGC_PERIPH_PVD2                    (DBGC_MCUSTPCTL_PVD2STP)
#define DBGC_PERIPH_TMR0_1                  (DBGC_MCUSTPCTL_M06STP)
#define DBGC_PERIPH_TMR0_2                  (DBGC_MCUSTPCTL_M07STP)
#define DBGC_PERIPH_TMR6_1                  (DBGC_MCUSTPCTL_M15STP)
#define DBGC_PERIPH_TMR6_2                  (DBGC_MCUSTPCTL_M16STP)
#define DBGC_PERIPH_TMR6_3                  (DBGC_MCUSTPCTL_M17STP)
#define DBGC_PERIPH_TMR6_4                  (DBGC_MCUSTPCTL_M18STP)
#define DBGC_PERIPH_TMR6_5                  (DBGC_MCUSTPCTL_M19STP)
#define DBGC_PERIPH_TMR6_6                  (DBGC_MCUSTPCTL_M20STP)
#define DBGC_PERIPH_HRPWM_1                 (DBGC_MCUSTPCTL_M24STP)
#define DBGC_PERIPH_HRPWM_2                 (DBGC_MCUSTPCTL_M25STP)
#define DBGC_PERIPH_HRPWM_3                 (DBGC_MCUSTPCTL_M26STP)
#define DBGC_PERIPH_HRPWM_4                 (DBGC_MCUSTPCTL_M27STP)
#define DBGC_PERIPH_HRPWM_5                 (DBGC_MCUSTPCTL_M28STP)
#define DBGC_PERIPH_HRPWM_6                 (DBGC_MCUSTPCTL_M29STP)
#define DBGC_PERIPH_HRPWM_7                 (DBGC_MCUSTPCTL_M30STP)
#define DBGC_PERIPH_HRPWM_8                 (DBGC_MCUSTPCTL_M31STP)
/**
 * @}
 */

/**
 * @defgroup DBGC_Trace_Mode DBGC trace mode
 * @{
 */
#define DBGC_TRACE_ASYNC                    (0UL)
#define DBGC_TRACE_SYNC_1BIT                (DBGC_MCUTRACECTL_TRACEMODE_0)
#define DBGC_TRACE_SYNC_2BIT                (DBGC_MCUTRACECTL_TRACEMODE_1)
#define DBGC_TRACE_SYNC_4BIT                (DBGC_MCUTRACECTL_TRACEMODE)
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
 * @addtogroup DBGC_Global_Functions
 * @{
 */
void DBGC_PeriphCmd(uint32_t u32Periph, en_functional_state_t enNewState);
void DBGC_TraceIoCmd(en_functional_state_t enNewState);
void DBGC_TraceModeConfig(uint32_t u32TraceMode);
/**
 * @}
 */

#endif /* LL_DBGC_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_DBGC_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
