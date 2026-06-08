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
#ifndef __HC32_HAL_DEF_H__
#define __HC32_HAL_DEF_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/

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
* @defgroup HC32_Series_Hal_Lock_Status_Define HC32 Series HAL lock status definition
* @{
*/
#define HAL_UNLOCKED               (0x00U)
#define HAL_LOCKED                 (0x01U)
/**
 * @}
 */

/**
* @defgroup HC32_Series_Hal_Status_Define HC32 Series HAL status definition
* @{
*/
#define HAL_OK                      (0x00)
#define HAL_ERR                     (0x01)
#define HAL_BUSY                    (0x02)
#define HAL_TIMEOUT                 (0x03)
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
 * Global function prototypes (definition in C source)
 ******************************************************************************/

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

#endif /* __HC32_HAL_DEF_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
