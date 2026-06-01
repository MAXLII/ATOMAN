/*!
 * @file        apm32f402_403_dbgmcu.h
 *
 * @brief       This file contains all the functions prototypes for the DBUGMCU firmware library
 *
 * @version     V1.0.0
 *
 * @date        2024-12-01
 *
 * @attention
 *
 *  Copyright (C) 2024-2025 Geehy Semiconductor
 *
 *  You may not use this file except in compliance with the
 *  GEEHY COPYRIGHT NOTICE (GEEHY SOFTWARE PACKAGE LICENSE).
 *
 *  The program is only for reference, which is distributed in the hope
 *  that it will be useful and instructional for customers to develop
 *  their software. Unless required by applicable law or agreed to in
 *  writing, the program is distributed on an "AS IS" BASIS, WITHOUT
 *  ANY WARRANTY OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the GEEHY SOFTWARE PACKAGE LICENSE for the governing permissions
 *  and limitations under the License.
 */

/* Define to prevent recursive inclusion */
#ifndef __APM32F402_403_DBGMCU_H
#define __APM32F402_403_DBGMCU_H

/* Includes */
#include "apm32f402_403.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup APM32F402_403_StdPeriphDriver
  @{
*/

/** @addtogroup DBGMCU_Driver DBGMCU Driver
  @{
*/

/** @defgroup DBGMCU_Enumerations Enumerations
  @{
*/

/**
 * @brief DBGMCU_STOP description
 */
enum
{
    DBGMCU_SLEEP                = ((uint32_t)0x00000001),
    DBGMCU_STOP                 = ((uint32_t)0x00000002),
    DBGMCU_STANDBY              = ((uint32_t)0x00000004),
    DBGMCU_IWDT_STOP            = ((uint32_t)0x00000100),
    DBGMCU_WWDT_STOP            = ((uint32_t)0x00000200),
    DBGMCU_TMR1_STOP            = ((uint32_t)0x00000400),
    DBGMCU_TMR2_STOP            = ((uint32_t)0x00000800),
    DBGMCU_TMR3_STOP            = ((uint32_t)0x00001000),
    DBGMCU_TMR4_STOP            = ((uint32_t)0x00002000),
    DBGMCU_CAN1_STOP            = ((uint32_t)0x00004000),
    DBGMCU_I2C1_SMBUS_TIMEOUT   = ((uint32_t)0x00008000),
    DBGMCU_TMR5_STOP            = ((uint32_t)0x00020000),
    DBGMCU_TMR8_STOP            = ((uint32_t)0x00100000),
    DBGMCU_CAN2_STOP            = ((uint32_t)0x00200000),
};

/**@} end of group DBGMCU_Enumerations*/


/** @defgroup DBGMCU_Functions Functions
  @{
*/

uint32_t DBGMCU_ReadDEVID(void);
uint32_t DBGMCU_ReadREVID(void);
void DBGMCU_Enable(uint32_t periph);
void DBGMCU_Disable(uint32_t periph);

/**@} end of group DBGMCU_Functions*/
/**@} end of group DBGMCU_Driver */
/**@} end of group APM32F402_403_StdPeriphDriver */

#ifdef __cplusplus
}
#endif

#endif /* __APM32F402_403_DBGMCU_H */
