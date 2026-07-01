// SPDX-License-Identifier: MIT
/**
 * @file    sim_port.h
 * @brief   MATLAB PLL port definition module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Define PLL MATLAB input signal indexes
 *          - Define PLL MATLAB output signal indexes
 *          - Provide port counts and sample timing used by the MATLAB S-Function adapter
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-07-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef MATLAB_PLL_PORT_H
#define MATLAB_PLL_PORT_H

#ifndef CTRL_FREQ
#define CTRL_FREQ (250000.0)
#endif

#include "my_math.h"

#define SIM_SAMPLE_TIME_S (4.0e-6)
#define SIM_TICK_UNIT_US (100U)
#define SIM_TICK_STEP_S ((double)SIM_TICK_UNIT_US * 1.0e-6)
#define SIM_TICKS_PER_SECOND (1000000.0 / (double)SIM_TICK_UNIT_US)

typedef enum
{
    SIM_INPUT_V_A = 0,
    SIM_INPUT_V_B,
    SIM_INPUT_V_C,
    SIM_INPUT_MAX,
} SIM_INPUT_E;

typedef enum
{
    SIM_OUTPUT_OMEGA = 0,       /* 1. PLL output angular frequency. */
    SIM_OUTPUT_THETA,           /* 2. PLL output phase angle. */
    SIM_OUTPUT_ALPHA,           /* 3. Clarke alpha input. */
    SIM_OUTPUT_BETA,            /* 4. Clarke beta input. */
    SIM_OUTPUT_VD,              /* 5. Park d-axis voltage. */
    SIM_OUTPUT_VQ,              /* 6. Park q-axis voltage. */
    SIM_OUTPUT_VF,              /* 7. PI frequency correction output. */
    SIM_OUTPUT_PI_REF,          /* 8. PI reference input. */
    SIM_OUTPUT_PI_ACT,          /* 9. PI feedback input. */
    SIM_OUTPUT_TS,              /* 10. PLL calculation period. */
    SIM_OUTPUT_OMEGA_CENTER,    /* 11. Center angular frequency. */
    SIM_OUTPUT_OMEGA_UP_LMT,    /* 12. Upper angular frequency limit. */
    SIM_OUTPUT_OMEGA_DN_LMT,    /* 13. Lower angular frequency limit. */
    SIM_OUTPUT_VM,              /* 14. PLL input voltage peak value. */
    SIM_OUTPUT_ZETA,            /* 15. PLL damping ratio. */
    SIM_OUTPUT_OMEGA_N,         /* 16. PLL natural angular frequency. */
    SIM_OUTPUT_PI_KP,           /* 17. PI proportional gain. */
    SIM_OUTPUT_PI_KI,           /* 18. PI integral gain. */
    SIM_OUTPUT_PI_UP_LMT,       /* 19. PI output upper limit. */
    SIM_OUTPUT_PI_DN_LMT,       /* 20. PI output lower limit. */
    SIM_OUTPUT_PI_B0,           /* 21. PI Tustin b0 coefficient. */
    SIM_OUTPUT_PI_B1,           /* 22. PI Tustin b1 coefficient. */
    SIM_OUTPUT_MAX,
} SIM_OUTPUT_E;

#define SIM_INPUT_NUM SIM_INPUT_MAX
#define SIM_OUTPUT_NUM SIM_OUTPUT_MAX

#endif
