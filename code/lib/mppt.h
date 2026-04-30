// SPDX-License-Identifier: MIT
/**
 * @file    mppt.h
 * @brief   mppt library public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Implement MPPT reference tracking for photovoltaic input control
 *          - Adjust voltage reference direction and step size from power and voltage feedback
 *          - Provide enable, pause, limit, and reference management APIs for MPPT operation
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef MPPT_H_
#define MPPT_H_
#include "stdint.h"
#include "stdbool.h"
#include "my_math.h"

#define CLAMP(val, dn, up) UP_DN_LMT(val, up, dn)

#define MPPT_DIR_INCREASE 1
#define MPPT_DIR_DECREASE (-1)
#define MPPT_VOLT_MIN_COFF 0.2f
#define MPPT_VOLT_MAX_COFF 0.85f

typedef void (*SetMpptRef)(float mpptVoltRef);
typedef float (*GetMpptRef)(void);
typedef float (*GetMpptVoltFdk)(void);
typedef float (*GetMpptPwrFdk)(void);

typedef enum
{
    MPPT_DISTURB = 0,
    MPPT_REV_DISTURB,
    MPPT_DIR_DECIDE,
} MpptSubStep;

typedef struct
{
    SetMpptRef setMpptRef;
    GetMpptRef getMpptRef;
    GetMpptVoltFdk getMpptVoltFdk;
    GetMpptPwrFdk getMpptPwrFdk;

    float mpptStartVolt;
    float stepDeltaVolt;
    float fastStepDeltaVolt;
    float midStepDeltaVolt;
    float slowStepDeltaVolt;
    float fastStepVoltThr;
    float slowStepVoltThr;
    float mpptLoseCtrVoltThres;
    float voltRef;
    float pwrStep1;
    float pwrStep2;
    float pwrStep3;
    float mpptVoc;
    float mpptVoltUplimit;
    float mpptVoltDnlimit;
    uint16_t mpptLoseCtrTimeCnt;
    uint16_t mpptLoseCtrTimeThres;
    uint16_t mpptEnableFlg;
    uint16_t mpptPauseFlg;
    uint16_t mpptTimeCnt;
    uint16_t mpptTimeThres;
    uint16_t mpptSubStep;
    int16_t mpptDir;
} mppt_cfg_para_t;

void MpptProcess(mppt_cfg_para_t *self);

void MpptEnable(mppt_cfg_para_t *self);

void MpptDisable(mppt_cfg_para_t *self);

void MpptPause(mppt_cfg_para_t *self);

void MpptResume(mppt_cfg_para_t *self);

void SetMpptVoltRef(mppt_cfg_para_t *self, float voltRef);

float GetMpptVoltRef(mppt_cfg_para_t *self);

void SetMpptVoc(mppt_cfg_para_t *self, float voltRef);

void SetMpptUpLimitVolt(mppt_cfg_para_t *self, float volt);

void SetMpptDnLimitVolt(mppt_cfg_para_t *self, float volt);

void SetMpptStartVolt(mppt_cfg_para_t *self, float voltRef);

#endif
