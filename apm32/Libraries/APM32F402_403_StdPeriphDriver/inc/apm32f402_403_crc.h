/*!
 * @file        apm32f402_403_crc.h
 *
 * @brief       This file contains all the functions prototypes for the CRC firmware library
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

#ifndef __APM32F402_403_CRC_H
#define __APM32F402_403_CRC_H

/* Includes */
#include "apm32f402_403.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup APM32F402_403_StdPeriphDriver
  @{
*/

/** @addtogroup CRC_Driver CRC Driver
  @{
*/

/** @defgroup CRC_Functions Functions
  @{
*/

/* Reset DATA */
void CRC_ResetDATA(void);

/* Operation functions */
uint32_t CRC_CalculateCRC(uint32_t data);
uint32_t CRC_CalculateBlockCRC(uint32_t* buf, uint32_t bufLen);
uint32_t CRC_ReadCRC(void);
void CRC_WriteIDRegister(uint8_t inData);
uint8_t CRC_ReadIDRegister(void);

/**@} end of group CRC_Functions*/
/**@} end of group CRC_Driver */
/**@} end of group APM32F402_403_StdPeriphDriver */

#ifdef __cplusplus
}
#endif

#endif /* __APM32F402_403_CRC_H */
