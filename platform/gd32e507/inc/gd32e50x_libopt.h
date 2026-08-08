// SPDX-License-Identifier: MIT
/**
 * @file    gd32e50x_libopt.h
 * @brief   GD32E507 standard peripheral library selection.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Collect the GD32E507 standard peripheral driver headers
 *          - Provide the library option header expected by gd32e50x.h
 *          - Keep platform source files independent of vendor include paths
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - The GD32E50X_CL device family is selected by the build system
 *          - Hardware access remains inside the vendor peripheral library
 *
 * @author  Max.Li
 * @date    2026-08-08
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef GD32E50X_LIBOPT_H
#define GD32E50X_LIBOPT_H

#include "gd32e50x_adc.h"
#include "gd32e50x_bkp.h"
#include "gd32e50x_can.h"
#include "gd32e50x_cmp.h"
#include "gd32e50x_crc.h"
#include "gd32e50x_ctc.h"
#include "gd32e50x_dac.h"
#include "gd32e50x_dbg.h"
#include "gd32e50x_dma.h"
#include "gd32e50x_enet.h"
#include "gd32e50x_exmc.h"
#include "gd32e50x_exti.h"
#include "gd32e50x_fmc.h"
#include "gd32e50x_fwdgt.h"
#include "gd32e50x_gpio.h"
#include "gd32e50x_i2c.h"
#include "gd32e50x_misc.h"
#include "gd32e50x_pmu.h"
#include "gd32e50x_rcu.h"
#include "gd32e50x_rtc.h"
#include "gd32e50x_shrtimer.h"
#include "gd32e50x_spi.h"
#include "gd32e50x_sqpi.h"
#include "gd32e50x_timer.h"
#include "gd32e50x_tmu.h"
#include "gd32e50x_usart.h"
#include "gd32e50x_wwdgt.h"

#endif /* GD32E50X_LIBOPT_H */
