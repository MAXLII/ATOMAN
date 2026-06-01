/*!
 * @file        apm32f402_403_dbgmcu.c
 *
 * @brief       This file provides all the DEBUG firmware functions
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

#include "apm32f402_403_dbgmcu.h"

/** @addtogroup APM32F402_403_StdPeriphDriver
  @{
*/

/** @addtogroup DBGMCU_Driver DBGMCU Driver
  * @brief DBGMCU driver modules
  @{
*/

/** @defgroup DBGMCU_Functions Functions
  @{
*/

/*!
 * @brief  Returns the device identifier.
 *
 * @param  None
 *
 * @retval Device identifier
 */
uint32_t DBGMCU_ReadDEVID(void)
{
    return (DBGMCU->IDCODE_B.EQR);
}

/*!
 * @brief  Returns the device revision identifier.
 *
 * @param  None
 *
 * @retval Device revision identifier
 */
uint32_t DBGMCU_ReadREVID(void)
{
    return (DBGMCU->IDCODE_B.WVR);
}

/*!
 * @brief       Enable the specified peripheral and low power mode behavior
 *              when the MCU under Debug mode
 *
 * @param       periph: Specifies the peripheral and low power mode
 *                      This parameter can be any combination of the following values:
 *                      @arg DBGMCU_SLEEP             : Keep debugger connection during SLEEP mode
 *                      @arg DBGMCU_STOP              : Keep debugger connection during STOP mode
 *                      @arg DBGMCU_STANDBY           : Keep debugger connection during STANDBY mode
 *                      @arg DBGMCU_IWDT_STOP         : Debug IWDT stopped when Core is halted
 *                      @arg DBGMCU_WWDT_STOP         : Debug WWDT stopped when Core is halted
 *                      @arg DBGMCU_TMR1_STOP         : TMR1 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR2_STOP         : TMR2 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR3_STOP         : TMR3 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR4_STOP         : TMR4 counter stopped when Core is halted
 *                      @arg DBGMCU_CAN1_STOP         : Debug CAN1 stopped when Core is halted
 *                      @arg DBGMCU_I2C1_SMBUS_TIMEOUT: I2C1 SMBUS timeout mode stopped when Core is halted
 *                      @arg DBGMCU_TMR5_STOP         : TMR5 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR8_STOP         : TMR8 counter stopped when Core is halted
 *                      @arg DBGMCU_CAN2_STOP         : Debug CAN2 stopped when Core is halted
 *
 * @retval      None
 */
void DBGMCU_Enable(uint32_t periph)
{
    DBGMCU->CFG |= periph;
}

/*!
 * @brief       Enable the specified peripheral and low power mode behavior
 *              when the MCU under Debug mode
 *
 * @param       periph: Specifies the peripheral and low power mode
 *                      This parameter can be any combination of the following values:
 *                      @arg DBGMCU_SLEEP             : Keep debugger connection during SLEEP mode
 *                      @arg DBGMCU_STOP              : Keep debugger connection during STOP mode
 *                      @arg DBGMCU_STANDBY           : Keep debugger connection during STANDBY mode
 *                      @arg DBGMCU_IWDT_STOP         : Debug IWDT stopped when Core is halted
 *                      @arg DBGMCU_WWDT_STOP         : Debug WWDT stopped when Core is halted
 *                      @arg DBGMCU_TMR1_STOP         : TMR1 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR2_STOP         : TMR2 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR3_STOP         : TMR3 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR4_STOP         : TMR4 counter stopped when Core is halted
 *                      @arg DBGMCU_CAN1_STOP         : Debug CAN1 stopped when Core is halted
 *                      @arg DBGMCU_I2C1_SMBUS_TIMEOUT: I2C1 SMBUS timeout mode stopped when Core is halted
 *                      @arg DBGMCU_TMR5_STOP         : TMR5 counter stopped when Core is halted
 *                      @arg DBGMCU_TMR8_STOP         : TMR8 counter stopped when Core is halted
 *                      @arg DBGMCU_CAN2_STOP         : Debug CAN2 stopped when Core is halted
 *
 * @retval      None
 */
void DBGMCU_Disable(uint32_t periph)
{
    DBGMCU->CFG &= ~periph;
}

/**@} end of group DBGMCU_Functions*/
/**@} end of group DBGMCU_Driver */
/**@} end of group APM32F402_403_StdPeriphDriver */
