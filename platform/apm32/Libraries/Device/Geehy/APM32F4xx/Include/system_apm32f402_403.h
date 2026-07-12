/**
 *
 * @file        system_apm32f402_403.h
 *
 * @brief       CMSIS Cortex-M4 Device System Source File for APM32F402/403xx devices.
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
#ifndef __SYSTEM_APM32F402_403_H
#define __SYSTEM_APM32F402_403_H

#ifdef __cplusplus
 extern "C" {
#endif 

/** @addtogroup CMSIS
  @{
*/

/** @addtogroup APM32F402_403_System
  @{
*/

/** @defgroup System_Functions
  @{
*/
extern uint32_t SystemCoreClock;          /*!< System Clock Frequency (Core Clock) */

extern void SystemInit(void);
extern void SystemCoreClockUpdate(void);

/**@} end of group System_Functions */
/**@} end of group APM32F402_403_System */
/**@} end of group CMSIS */

#ifdef __cplusplus
}
#endif

#endif /*__SYSTEM_APM32F402_403_H */
