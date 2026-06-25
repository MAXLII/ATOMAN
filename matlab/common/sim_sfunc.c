// SPDX-License-Identifier: MIT
/**
 * @file    sim_sfunc.c
 * @brief   MATLAB reusable S-Function module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Implement a reusable Level-2 C-MEX S-Function
 *          - Bind Simulink input and output vectors to the MATLAB-compatible adapter API
 *          - Advance the 100 us section scheduler tick during S-Function output execution
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

#ifndef S_FUNCTION_NAME
#error "S_FUNCTION_NAME must be defined by the project compile script."
#endif

#define S_FUNCTION_LEVEL 2

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "sim_sfunc.h"
#include "simstruc.h"

#if defined(MATLAB_MEX_FILE)
#include "mex.h"
#endif

#include "section.h"

uint32_t sim_time_100us = 0U;

static const double *s_inputs = NULL;
static double *s_outputs = NULL;
static double s_time_s = 0.0;
static double s_time_last_s = 0.0;

static void sim_sfunc_start(void);
static void sim_sfunc_bind_io(const double *inputs, double *outputs, double time_s);
static void sim_sfunc_output_step(void);

float sim_get_input(SIM_INPUT_E num)
{
    if ((s_inputs != NULL) && (num < SIM_INPUT_MAX))
    {
        return (float)s_inputs[num];
    }

    return 0.0f;
}

void sim_set_output(SIM_OUTPUT_E num, float val)
{
    if ((s_outputs != NULL) && (num < SIM_OUTPUT_MAX))
    {
        s_outputs[num] = (double)val;
    }
}

void sim_printf(const char *file, int line, const char *format, ...)
{
    (void)file;
    (void)line;

    va_list args;
    va_start(args, format);
#if defined(MATLAB_MEX_FILE)
    {
        char msg[512];
        (void)vsnprintf(msg, sizeof(msg), format, args);
        mexPrintf("[%.6f] %s", s_time_s, msg);
    }
#else
    (void)vprintf(format, args);
#endif
    va_end(args);
}

static void sim_sfunc_bind_io(const double *inputs, double *outputs, double time_s)
{
    s_inputs = inputs;
    s_outputs = outputs;
    s_time_s = time_s;
}

static void sim_sfunc_start(void)
{
    sim_time_100us = 0U;
    s_time_last_s = 0.0;

    section_runtime_reset();
    section_init();
}

static void sim_sfunc_output_step(void)
{
    const double tick_eps_s = SIM_TICK_STEP_S * 1.0e-6;

    while ((s_time_s - s_time_last_s + tick_eps_s) >= SIM_TICK_STEP_S)
    {
        sim_time_100us += 1U;
        run_task();
        s_time_last_s += SIM_TICK_STEP_S;
    }

    section_interrupt();
}

static void mdlInitializeSizes(SimStruct *S)
{
    ssSetNumSFcnParams(S, 0);
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S))
    {
        return;
    }

    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);

    if (!ssSetNumInputPorts(S, 1))
    {
        return;
    }
    ssSetInputPortWidth(S, 0, SIM_INPUT_NUM);
    ssSetInputPortRequiredContiguous(S, 0, true);
    ssSetInputPortDirectFeedThrough(S, 0, 1);

    if (!ssSetNumOutputPorts(S, 1))
    {
        return;
    }
    ssSetOutputPortWidth(S, 0, SIM_OUTPUT_NUM);

    ssSetNumSampleTimes(S, 1);
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 0);

    ssSetOperatingPointCompliance(S, USE_DEFAULT_OPERATING_POINT);
    ssSetRuntimeThreadSafetyCompliance(S, RUNTIME_THREAD_SAFETY_COMPLIANCE_TRUE);
    ssSetOptions(S, SS_OPTION_EXCEPTION_FREE_CODE);
}

static void mdlInitializeSampleTimes(SimStruct *S)
{
    ssSetSampleTime(S, 0, SIM_SAMPLE_TIME_S);
    ssSetOffsetTime(S, 0, 0.0);
}

#define MDL_START
#if defined(MDL_START)
static void mdlStart(SimStruct *S)
{
    (void)S;
    sim_sfunc_start();
}
#endif

static void mdlOutputs(SimStruct *S, int_T tid)
{
    const real_T *u = (const real_T *)ssGetInputPortSignal(S, 0);
    real_T *y = (real_T *)ssGetOutputPortSignal(S, 0);

    (void)tid;

    sim_sfunc_bind_io(u, y, ssGetT(S));
    sim_sfunc_output_step();
}

static void mdlTerminate(SimStruct *S)
{
    (void)S;
}

#ifdef MATLAB_MEX_FILE
#include "simulink.c"
#else
#include "cg_sfun.h"
#endif
