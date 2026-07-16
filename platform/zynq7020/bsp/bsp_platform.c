// SPDX-License-Identifier: MIT
/**
 * @file    bsp_platform.c
 * @brief   Zynq-7020 platform control implementation.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Unlock the Zynq PS system-level control registers
 *          - Request a full processing-system software reset
 *          - Hold execution if reset assertion is delayed
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Reset execution does not return
 *          - Hardware access is isolated in the Zynq BSP
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "bsp_platform.h"

#include "xil_io.h"
#include "xpseudo_asm.h"

#include <stdint.h>

#define BSP_ZYNQ_SLCR_UNLOCK_ADDR 0xF8000008UL /* SLCR 写保护解锁寄存器地址。 */
#define BSP_ZYNQ_SLCR_UNLOCK_KEY 0x0000DF0DUL  /* SLCR 写保护解锁键值。 */
#define BSP_ZYNQ_PSS_RST_CTRL_ADDR 0xF8000200UL /* PS 软件复位控制寄存器地址。 */
#define BSP_ZYNQ_PSS_RST_MASK 0x00000001UL      /* PS 全局软件复位请求位。 */

void bsp_platform_reset(void)
{
    Xil_Out32(BSP_ZYNQ_SLCR_UNLOCK_ADDR, BSP_ZYNQ_SLCR_UNLOCK_KEY); /* 允许写入系统级控制寄存器。 */
    dsb();                                                          /* 确保解锁写入先于复位请求生效。 */
    Xil_Out32(BSP_ZYNQ_PSS_RST_CTRL_ADDR, BSP_ZYNQ_PSS_RST_MASK);  /* 请求处理系统完整复位。 */
    dsb();                                                          /* 提交复位寄存器写入。 */
    isb();                                                          /* 丢弃复位请求前预取的指令。 */

    for (;;)
    {
        /* 等待硬件复位，不继续执行共享业务代码。 */
    }
}
