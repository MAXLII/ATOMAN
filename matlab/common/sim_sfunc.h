// SPDX-License-Identifier: MIT
/**
 * @file    sim_sfunc.h
 * @brief   MATLAB reusable S-Function interface.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Provide MATLAB-compatible input and output accessors for imported control code
 *          - Expose the simulation tick consumed by the section scheduler
 *          - Provide a reusable logging macro for MATLAB simulation builds
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path is simulated by mdlOutputs single-step execution
 *          - Hardware access is abstracted through the project HAL / BSP boundary
 *
 * @author  Max.Li
 * @date    2026-06-25
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef MATLAB_SIM_SFUNC_H
#define MATLAB_SIM_SFUNC_H

#include <stdint.h>

#include "sim_port.h"

float sim_get_input(SIM_INPUT_E num);
void sim_set_output(SIM_OUTPUT_E num, float val);
void sim_printf(const char *file, int line, const char *format, ...);

#define SIM_LOG(...) sim_printf(__FILE__, __LINE__, __VA_ARGS__)

extern uint32_t sim_time_100us;

#endif
