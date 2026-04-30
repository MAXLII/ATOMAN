// SPDX-License-Identifier: MIT
/**
 * @file    volt_mt.c
 * @brief   volt_mt library module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Monitor voltage thresholds and timing conditions
 *          - Track configured over-voltage or under-voltage state transitions
 *          - Expose pointer update and periodic monitoring APIs for protection logic
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
#include "volt_mt.h"
#include "string.h"

// 初始化电压监测器
void volt_mt_init(volt_mt_t *mt, const volt_cfg_t *cfg, const float *volt_ptr)
{
    if (mt == NULL || cfg == NULL || volt_ptr == NULL)
        return;

    mt->cfg = *cfg;
    mt->in.volt = volt_ptr;
    mt->in.time = 0;

    mt->out.st = ST_INVALID;
    mt->out.is_vld = 0;
    mt->out.vld_dur = 0;
    mt->out.st_dur = 0;

    mt->inter.st_start = 0;
    mt->inter.last_upd = 0;
    mt->inter.last_volt = 0.0f;
}

// 设置电压指针
void volt_mt_set_ptr(volt_mt_t *mt, const float *volt_ptr)
{
    if (mt != NULL && volt_ptr != NULL)
    {
        mt->in.volt = volt_ptr;
    }
}

// 更新电压监测状态
void volt_mt_upd(volt_mt_t *mt, uint32_t curr_time)
{
    if (mt == NULL || mt->in.volt == NULL)
        return;

    float voltage = *(mt->in.volt);
    mt->in.time = curr_time;
    mt->inter.last_volt = voltage;

    // 更新持续时间
    mt->out.st_dur = curr_time - mt->inter.st_start;
    if (mt->out.is_vld)
    {
        mt->out.vld_dur = mt->out.st_dur;
    }

    // 检查立即退出条件
    if (voltage <= mt->cfg.min_inst || voltage >= mt->cfg.max_inst)
    {
        if (mt->out.st != ST_INVALID)
        {
            mt->out.st = ST_INVALID;
            mt->inter.st_start = curr_time;
            mt->out.is_vld = 0;
            mt->out.vld_dur = 0;
        }
        return;
    }

    // 根据当前状态进行判断
    switch (mt->out.st)
    {
    case ST_INVALID:
        // 检查是否进入有效范围
        if (voltage > mt->cfg.min_vld && voltage < mt->cfg.max_vld)
        {
            mt->out.st = ST_ENTERING;
            mt->inter.st_start = curr_time;
        }
        break;

    case ST_ENTERING:
        // 检查是否仍在有效范围内
        if (voltage > mt->cfg.min_vld && voltage < mt->cfg.max_vld)
        {
            // 检查是否达到进入延时
            if (curr_time - mt->inter.st_start >= mt->cfg.enter_dly)
            {
                mt->out.st = ST_VALID;
                mt->out.is_vld = 1;
                mt->inter.st_start = curr_time; // 重置状态开始时间
            }
        }
        else
        {
            // 退出有效范围，回到无效状态
            mt->out.st = ST_INVALID;
            mt->inter.st_start = curr_time;
        }
        break;

    case ST_VALID:
        // 检查是否退出有效范围
        if (voltage <= mt->cfg.min_exit || voltage >= mt->cfg.max_exit)
        {
            mt->out.st = ST_EXITING;
            mt->inter.st_start = curr_time;
        }
        break;

    case ST_EXITING:
        // 检查是否回到有效范围
        if (voltage > mt->cfg.min_vld && voltage < mt->cfg.max_vld)
        {
            // 回到有效范围
            mt->out.st = ST_VALID;
            mt->inter.st_start = curr_time;
        }
        else if (voltage <= mt->cfg.min_exit || voltage >= mt->cfg.max_exit)
        {
            // 检查是否达到退出延时
            if (curr_time - mt->inter.st_start >= mt->cfg.exit_dly)
            {
                mt->out.st = ST_INVALID;
                mt->out.is_vld = 0;
                mt->out.vld_dur = 0;
            }
        }
        else
        {
            // 在滞回区间内，回到有效状态
            mt->out.st = ST_VALID;
            mt->inter.st_start = curr_time;
        }
        break;
    }

    mt->inter.last_upd = curr_time;
}
