/**
 *******************************************************************************
 * @file  hc32f3xx.h
 * @brief This file contains the common part of the HC32 series.
 @verbatim
   Change Logs:
   Date             Author          Notes
   2024-01-15       CDT             First version
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
#ifndef __HC32F3XX_H__
#define __HC32F3XX_H__

/* C binding of definitions if building with C++ compiler */
#ifdef __cplusplus
extern "C"
{
#endif

/*******************************************************************************
 * Include files
 ******************************************************************************/
/**
 * @brief HC32F3xx Device Include
 */
#if defined(HC32F334)
#include "hc32f334.h"
#include "system_hc32f334.h"
#else
#error "Please select first the target HC32xxxx device used in your application (in hc32xxxx.h file)"
#endif

/*******************************************************************************
 * Global type definitions ('typedef')
 ******************************************************************************/

/*******************************************************************************
 * Global pre-processor symbols/macros ('#define')
 ******************************************************************************/

/*******************************************************************************
 * Global variable definitions ('extern')
 ******************************************************************************/

/*******************************************************************************
 * Global function prototypes (definition in C source)
 ******************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* __HC32F3XX_H__ */

/*******************************************************************************
 * EOF (not truncated)
 ******************************************************************************/
