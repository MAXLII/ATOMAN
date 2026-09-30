// SPDX-License-Identifier: MIT
/**
 * @file test_chb_closed_loop.c
 * @brief Exercise the production CHB FSM and closed loop against a three-cell averaged plant.
 * @details Uses the production observer and the 6 kV/12 mH/600 uF averaged plant.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_cfg.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "chb_protect.h"
#include "section.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define TEST_TS 0.0001f /* 10 kHz 控制周期，s。 */
#define TEST_OMEGA 314.1592653589793f /* 50 Hz 电角频率，rad/s。 */
#define TEST_VPK 8485.28137423857f /* 6 kV RMS 电网基波峰值，V。 */

uint32_t section_testbench_time_100us = 0u; /* Section 主机测试时钟。 */
static float grid_v = 0.0f; /* 继电器电网侧电压，V。 */
static float i_alpha_a = 14.1421356f; /* 网侧物理电流，A。 */
static float bus_v[CHB_CELL_COUNT] = {3200.0f, 3200.0f, 3200.0f}; /* 预充母线，V。 */
static float load_i_a[CHB_CELL_COUNT] = {6.25f, 6.25f, 6.25f}; /* 电阻负载支路的实际电流，A。 */
static float last_v_pwm_v[CHB_CELL_COUNT] = {0.0f}; /* 控制器输出的逐级最终发波电压，V。 */
static float last_bus_v[CHB_CELL_COUNT] = {0.0f}; /* 与逐级发波电压同拍的母线电压，V。 */
static chb_pwm_deadtime_flag_t last_deadtime_flag[CHB_CELL_COUNT] = {CHB_PWM_DEADTIME_OFF};
static uint8_t pwm_enabled = 0u; /* 模拟桥臂使能状态。 */
static uint32_t pwm_calls = 0u; /* 成功发波次数。 */
static uint8_t soft_relay_on = 0u; /* 模拟软起继电器。 */
static uint8_t main_relay_on = 0u; /* 模拟主继电器。 */

static void apply_pwm_cell(uint32_t cell, float v_pwm_v, float bus_voltage,
                           chb_pwm_deadtime_flag_t deadtime_flag)
{
    last_v_pwm_v[cell] = v_pwm_v;
    last_bus_v[cell] = bus_voltage;
    last_deadtime_flag[cell] = deadtime_flag;
}

/** @param condition 测试必须成立的条件。 @param p_message 失败说明。 */
static void check(int condition, const char *p_message)
{
    if (condition == 0)
    {
        fprintf(stderr, "CHB closed-loop failure: %s\n", p_message);
        exit(EXIT_FAILURE);
    }
}

static void apply_pwm_cell_1(float v_pwm_v, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    apply_pwm_cell(0u, v_pwm_v, bus_voltage, deadtime_flag);
}

static void apply_pwm_cell_2(float v_pwm_v, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    apply_pwm_cell(1u, v_pwm_v, bus_voltage, deadtime_flag);
}

static void apply_pwm_cell_3(float v_pwm_v, float bus_voltage, chb_pwm_deadtime_flag_t deadtime_flag)
{
    apply_pwm_cell(2u, v_pwm_v, bus_voltage, deadtime_flag);
    pwm_enabled = 1u;
    ++pwm_calls;
}

/** @brief 模拟硬件即时停波。 */
static void disable_pwm(void)
{
    pwm_enabled = 0u;
}

static void soft_relay_on_func(void)
{
    check(main_relay_on == 0u, "main relay off before soft relay on");
    soft_relay_on = 1u;
}

static void soft_relay_off_func(void)
{
    soft_relay_on = 0u;
}

static void main_relay_on_func(void)
{
    check(soft_relay_on != 0u, "soft relay stays on until main relay on is confirmed");
    main_relay_on = 1u;
}

static void main_relay_off_func(void)
{
    main_relay_on = 0u;
}

/** @brief 以 1 ms 的真实 FSM 调度间隔推进静态输入测试。 */
static void step_fsm_ms(void)
{
    for (uint32_t tick = 0u; tick < 10u; ++tick)
    {
        section_interrupt();
        ++section_testbench_time_100us;
        run_task();
    }
}

int main(void)
{
    chb_ctrl_hal_t binding = {0}; /* 外部采样和同步发波依赖。 */
    float bus_sum[CHB_CELL_COUNT] = {0}; /* 最后 0.1 s 的母线累计值，V。 */
    float power_sum[CHB_CELL_COUNT] = {0}; /* 每阶段最后 0.1 s 的真实负载功率累计，W。 */
    uint32_t steady_count = 0u; /* 稳态采样数量。 */
    float peak_current = fabsf(i_alpha_a); /* 全程物理电流峰值，A。 */

    binding.p_grid_v = &grid_v;
    binding.p_i_alpha_a = &i_alpha_a;
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        binding.p_bus_v[cell] = &bus_v[cell];
        binding.p_load_i_a[cell] = &load_i_a[cell];
    }
    binding.p_set_pwm_func[0] = apply_pwm_cell_1;
    binding.p_set_pwm_func[1] = apply_pwm_cell_2;
    binding.p_set_pwm_func[2] = apply_pwm_cell_3;
    binding.p_pwm_disable = disable_pwm;
    binding.p_soft_start_relay_on = soft_relay_on_func;
    binding.p_soft_start_relay_off = soft_relay_off_func;
    binding.p_main_relay_on = main_relay_on_func;
    binding.p_main_relay_off = main_relay_off_func;

    check(chb_hal_bind(&binding) != 0u, "HAL binding");
    check(chb_protect_configure(25.0f, 2700.0f, 3600.0f) != 0u,
          "simulation protection configuration");
    load_i_a[0] = NAN; /* 原始负载电流非法；观测器仅复制，不污染 SOGI/PLL 历史。 */
    section_init();
    check(chb_cfg_set_run_request(1u) != 0u, "run request");
    for (uint32_t warmup = 0u; warmup < 20u; ++warmup)
    {
        step_fsm_ms();
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_INIT,
           "INIT rejects non-finite initial load-current data");
    load_i_a[0] = 6.25f;
    bus_v[1] = 0.0f;
    for (uint32_t warmup = 0u; warmup < 20u; ++warmup)
    {
        step_fsm_ms();
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_BUS_SOFT_START,
          "zero bus accepted into passive precharge monitor");
    check(pwm_calls == 0u, "soft start cannot reach PWM");
    check(soft_relay_on != 0u, "soft relay turns on during precharge");
    check(main_relay_on == 0u, "main relay stays off during precharge");
    for (uint32_t wait = 0u; wait < 5000u; ++wait)
    {
        step_fsm_ms();
        if (chb_fsm_get_run_state() == CHB_RUN_STATE_FAULT)
        {
            break;
        }
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_FAULT, "5 s precharge timeout");
    step_fsm_ms();
    check(chb_protect_is_tripped() == 0u, "precharge timeout does not latch application protection");
    check((soft_relay_on == 0u) && (main_relay_on == 0u),
          "fault turns both relays off");
    check(chb_fsm_clear_fault() == 0u, "running request prevents fault clear");
    check(chb_cfg_set_run_request(0u) != 0u, "stop request for fault recovery");
    section_interrupt();
    check(chb_fsm_clear_fault() != 0u, "explicit CHB fault clear request");
    step_fsm_ms();
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_IDLE, "cleared fault returns to IDLE");

    bus_v[1] = 3200.0f;
    check(chb_cfg_set_run_request(1u) != 0u, "new run request");
    step_fsm_ms();
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_BUS_SOFT_START,
          "IDLE enters bus soft start");
    for (uint32_t wait = 0u; wait < 100u; ++wait)
    {
        step_fsm_ms();
    }
    bus_v[1] = 0.0f;
    for (uint32_t wait = 0u; wait < 20u; ++wait)
    {
        step_fsm_ms();
    }
    bus_v[1] = 3200.0f;
    for (uint32_t wait = 0u; wait < 419u; ++wait)
    {
        step_fsm_ms();
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_BUS_SOFT_START,
          "down-count delays soft start completion");
    grid_v = 1000.0f; /* 继电器上游侧尚未与 0 V 占位的下游侧接近。 */
    step_fsm_ms();
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_MAIN_RELAY_WAIT,
          "soft start hands off to main relay wait");
    check((soft_relay_on != 0u) && (main_relay_on == 0u),
          "precharge path stays connected while waiting for main relay");
    check(chb_fsm_run_allowed() == 0u, "no PWM during main relay wait");
    for (uint32_t wait = 0u; wait < 30u; ++wait)
    {
        step_fsm_ms();
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_MAIN_RELAY_WAIT,
          "relay remains off while terminal voltages differ");
    check(main_relay_on == 0u, "rly_on waits for voltage match");
    check(chb_cfg_set_run_request(0u) != 0u, "cancel relay wait");
    step_fsm_ms();
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_IDLE,
          "cancelled relay wait returns to IDLE");
    for (uint32_t wait = 0u; wait < 30u; ++wait)
    {
        step_fsm_ms();
    }
    check(main_relay_on == 0u, "cancelled relay request cannot turn on later");
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        bus_v[cell] = 3000.0f; /* 用低于目标的预充值启动，覆盖母线给定斜坡。 */
    }
    check(chb_cfg_set_run_request(1u) != 0u, "restart after cancelled relay wait");
    step_fsm_ms();
    for (uint32_t wait = 0u; wait < 500u; ++wait)
    {
        step_fsm_ms();
    }
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_MAIN_RELAY_WAIT,
          "restart repeats bus soft start before relay wait");
    check(main_relay_on == 0u, "restart still waits for voltage match");
    grid_v = 0.0f;
    uint32_t main_on_ms = 0u;
    uint32_t run_ms = 0u;
    for (uint32_t wait = 1u; wait <= 100u; ++wait)
    {
        step_fsm_ms();
        if (    (main_relay_on != 0u)
             && (main_on_ms == 0u))
        {
            main_on_ms = wait;
        }
        if (chb_fsm_get_run_state() == CHB_RUN_STATE_RUN)
        {
            run_ms = wait;
            break;
        }
    }
    check(main_on_ms != 0u, "rly_on commands main relay on after voltage match");
    check(run_ms >= main_on_ms + CHB_MAIN_RELAY_WAIT_MS,
          "control waits after main relay on command");
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_RUN, "FSM entered RUN");
    check(soft_relay_on == 0u, "soft relay turns off after main relay confirmation and before RUN");
    check(chb_fsm_run_allowed() == 0u, "RUN permission waits for state entry");
    step_fsm_ms();
    check(chb_fsm_run_allowed() != 0u, "FSM granted permission");

    /* 三桥轮换限功率，再全部解除限功率；每阶段 4 s。 */
    for (uint32_t tick = 0u; tick < 160000u; ++tick)
    {
        float time_s = (float)tick * TEST_TS; /* 当前仿真时间，s。 */
        float cosine = cosf(TEST_OMEGA * time_s); /* 电网同步角余弦。 */
        float grid_voltage = TEST_VPK * cosine; /* 电网瞬时电压，V。 */
        float old_current = i_alpha_a; /* 积分本拍前的物理电流，A。 */
        float load[CHB_CELL_COUNT] = {512.0f, 512.0f, 512.0f}; /* 各级负载，ohm。 */

        grid_v = grid_voltage;
        uint32_t stage = tick / 40000u;
        if ((stage < 3u) && (time_s >= 0.25f))
        {
            load[stage] = 482.0f;
            load[(stage + 2u) % CHB_CELL_COUNT] = 542.0f;
        }
        else if (stage == 3u)
        {
            for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
            {
                load[cell] = 542.0f;
            }
        }
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            load_i_a[cell] = bus_v[cell] / load[cell];
        }
        section_interrupt();
        check(pwm_enabled != 0u, "all bridges enabled after control dispatch");
        const float total_v_pwm_v = last_v_pwm_v[0] + last_v_pwm_v[1] + last_v_pwm_v[2];

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            check(fabsf(last_bus_v[cell] - bus_v[cell]) < 0.01f, "same-period bus voltage");
            check(fabsf(last_v_pwm_v[cell]) <= 0.981f * bus_v[cell],
                  "per-cell voltage limit");
            check((last_deadtime_flag[cell] >= CHB_PWM_DEADTIME_NEGATIVE)
                      && (last_deadtime_flag[cell] <= CHB_PWM_DEADTIME_POSITIVE),
                  "dead-time direction flag");
        }
        i_alpha_a += TEST_TS * (grid_voltage - 0.5f * i_alpha_a
                                - total_v_pwm_v) / 0.012f;
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            bus_v[cell] += TEST_TS * ((last_v_pwm_v[cell] / bus_v[cell])
                                     * old_current - bus_v[cell] / load[cell]) / 600.0e-6f;
            if ((tick % 40000u) >= 39000u)
            {
                bus_sum[cell] += bus_v[cell];
                power_sum[cell] += bus_v[cell] * bus_v[cell] / load[cell];
            }
        }
        peak_current = fmaxf(peak_current, fabsf(i_alpha_a));
        if ((tick % 40000u) >= 39000u)
        {
            ++steady_count;
        }
        ++section_testbench_time_100us;
        run_task();
        if ((tick % 40000u) == 39999u)
        {
            printf("CHB stage %lu: bus %.2f %.2f %.2f V; load power %.2f %.2f %.2f W; Ipeak %.2f A\n",
                   (unsigned long)stage,
                   bus_sum[0] / (float)steady_count, bus_sum[1] / (float)steady_count,
                   bus_sum[2] / (float)steady_count,
                   power_sum[0] / (float)steady_count, power_sum[1] / (float)steady_count,
                   power_sum[2] / (float)steady_count, peak_current);
            for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
            {
                float target = fminf(3200.0f, sqrtf(20000.0f * load[cell]));
                check(fabsf(bus_sum[cell] / (float)steady_count - target) < 12.0f,
                      "each bridge enters power limiting and recovers nominal voltage");
                check(power_sum[cell] / (float)steady_count <= 20050.0f,
                      "each bridge sustained load power within 0.25 percent of limit");
                bus_sum[cell] = 0.0f;
                power_sum[cell] = 0.0f;
            }
            steady_count = 0u;
        }
    }
    check(peak_current < 25.0f, "simulation overcurrent margin");

    i_alpha_a = 30.0f;
    section_interrupt();
    check(chb_protect_is_tripped() != 0u, "overcurrent trip latched");
    check(pwm_enabled == 0u, "same-period PWM inhibition");
    check(chb_cfg_get_run_request() == 0u, "application protection revokes run request");
    step_fsm_ms();
    check(chb_fsm_get_run_state() == CHB_RUN_STATE_IDLE, "application trip returns FSM to IDLE");
    check((soft_relay_on == 0u) && (main_relay_on == 0u),
          "application trip turns both relays off");
    i_alpha_a = 0.0f;
    section_interrupt();
    check(chb_fsm_clear_fault() == 0u, "application trip is not an FSM fault");
    check(chb_protect_clear_latch() != 0u, "explicit application fault clear");
    check(chb_protect_is_tripped() == 0u, "application latch cleared");
    return EXIT_SUCCESS;
}
