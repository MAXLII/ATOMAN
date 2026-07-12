/**
 *******************************************************************************
 * @file  hc32_ll_ots.h
 * @brief This file contains all the functions prototypes of the OTS driver
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
#ifndef __HC32_LL_OTS_H__
#define __HC32_LL_OTS_H__

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
 * @addtogroup LL_OTS
 * @{
 */

#if (LL_OTS_ENABLE == DDL_ON)

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/
/**
 * @defgroup OTS_Global_Types OTS Global Types
 * @{
 */

/**
 * @brief Calculate the preset constants related to temperature.
 */
typedef struct {
    uint32_t u32C;                          /*!< Preset constant C.
                                                 This parameter can be a uint8_t value */
    uint32_t u32M;                          /*!< Preset constant M.
                                                 This parameter can be a uint24_t value */
    uint16_t u16K;                          /*!< Preset constant K.
                                                 This parameter can be a uint16_t value */
} stc_ots_preset_constant_t;

/**
 * @}
 */

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
  Global function prototypes (definition in C source)
 ******************************************************************************/
/**
 * @addtogroup OTS_Global_Functions
 * @{
 */

int32_t OTS_DeInit(void);

int32_t OTS_GetPresetConstant(stc_ots_preset_constant_t *pstcPresetConstant);
void OTS_Cmd(en_functional_state_t enNewState);

/**
 * @}
 */

#endif /* LL_OTS_ENABLE */

/**
 * @}
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __HC32_LL_OTS_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
