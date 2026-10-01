// SPDX-License-Identifier: MIT
/**
 * @file app.c
 * @brief Bind the CHB controller to PLECS samples and FRAME commands.
 * @details app 采样并计算电网 RMS、PLL 角频率和相位，通过 HAL 提供只读输入。
 *          ctrl 复制同拍输入，维护控制所需的谐波、电流正交观测和控制历史。
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_cfg.h"
#include "chb_ctrl.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "chb_protect.h"
#include "chk_grid.h"
#include "my_math.h"
#include "msogi.h"
#include "pll.h"
#include "sliding_window.h"
#include "bsp_pwm.h"
#include "plecs.h"
#include "pwm.h"
#include "section.h"
#include "shell.h"

#include <float.h>
#include <math.h>
#include <stdint.h>

#define CHB_APP_CURRENT_TRIP_A  200.0f
#define CHB_APP_PRECHARGE_TRIP_A 350.0f /* 10 kV passive precharge reached 261 A before the old 200 A trip opened the inductor path. */
#define CHB_APP_BUS_MIN_V       500.0f
#define CHB_APP_BUS_MAX_V       4200.0f
#define CHB_APP_LOAD_MIN_OHM    1.0f
#define CHB_APP_LOAD_OFF_OHM    1.0e6f /* Near-open load: 10.24 W at 3200 V; avoids ill-conditioned variable resistors. */
#define CHB_APP_LOAD_TASK_MS    1u
#define CHB_APP_GRID_RMS_CAPACITY 1024u /* 固定存储容量；INIT 按额定一周期确定实际窗长。 */
#define CHB_APP_GRID_READY_TICKS  400u /* 与原观测器一致的基波幅值建立等待。 */
#define CHB_APP_GRID_SOGI_K      1.414213562f
#define CHB_APP_PLL_ZETA         0.707106781f
#define CHB_APP_PLL_BANDWIDTH_HZ 20.0f
#define CHB_APP_PLL_UP_HZ        25.0f
#define CHB_APP_PLL_DOWN_HZ      15.0f
#define CHB_APP_GRID_V_MIN_RATIO 0.90f /* 电网启动电压下限，额定 RMS 的 90%。 */
#define CHB_APP_GRID_V_MAX_RATIO 1.10f /* 电网启动电压上限，额定 RMS 的 110%。 */
#define CHB_APP_GRID_F_MIN_HZ    49.0f
#define CHB_APP_GRID_F_MAX_HZ    51.0f
#define CHB_APP_GRID_JUDGE_MS    200u /* 电压、频率须连续满足的资格时间。 */
#define CHB_APP_TASK_MS          1u

_Static_assert(CHB_APP_GRID_JUDGE_MS >= CHB_APP_TASK_MS,
               "Grid qualification must cover at least one app task tick");
_Static_assert(CHB_APP_GRID_JUDGE_MS % CHB_APP_TASK_MS == 0u,
               "Grid qualification time must be an integer number of app task ticks");

_Static_assert(PLECS_INPUT_BUS_1_I == PLECS_INPUT_BUS_1_V + CHB_CELL_COUNT,
               "Bus voltages must precede branch currents");
_Static_assert(PLECS_INPUT_MAX == PLECS_INPUT_BUS_1_I + CHB_CELL_COUNT,
               "PLECS CHB model must provide the configured analog inputs");
_Static_assert(PLECS_OUTPUT_PWM_ENABLE == PLECS_OUTPUT_CHB_1_DUTY + CHB_CELL_COUNT,
               "Duty outputs must precede PWM enable");
_Static_assert(PLECS_OUTPUT_MAX == PLECS_OUTPUT_LOAD_R1 + CHB_CELL_COUNT,
               "PLECS CHB model must consume the configured outputs");

static float    v_grid_raw;               /* Instantaneous grid voltage at the DLL input. */
/* Entity: app 独占电网测量与 PLL 状态；HAL 只借用输出地址。
 * Prior: INIT 验证窗口及锁相配置，启动等待及窗口填充完成前 RMS 为 NAN，FSM 保持 INIT。
 * Time: 每拍原始采样 -> RMS / PLL -> ctrl 快照 -> 保护 -> 发波，按 0 / 1 / 2 / 3 执行。 */
typedef struct
{
    struct
    {
        const float *p_v_grid_raw; /* 本拍电网瞬时电压，V。 */
    } input;
    struct
    {
        float v_grid_sample;     /* 电网原始输入的本拍副本，V。 */
        float v_grid_squared;    /* 滑窗输入：MSOGI 基波电压平方，V^2。 */
        float v_grid_square_history[CHB_APP_GRID_RMS_CAPACITY];
        sliding_window_t rms_window; /* 平方电压的均值及历史。 */
        msogi_t msogi;               /* 保留原 PLL 前端的直流、奇次谐波分离方式。 */
        pll_t pll;                   /* 连续运行的锁相及 PI 历史。 */
        uint32_t ticks;              /* 有界启动计数，基波观测建立后停止递增。 */
        uint8_t ready;               /* 初始化成功后才允许执行算法。 */
    } inter;
    struct
    {
        float v_grid_rms; /* 额定一周期基波电压的有效值，V。 */
        float omega_grid; /* PLL 角频率，rad/s，即 2*pi*f。 */
        float theta_grid; /* PLL 推进到下一控制作用时刻的相位，rad。 */
    } output;
} chb_app_grid_t;

static chb_app_grid_t grid_measurement = {.input.p_v_grid_raw = &v_grid_raw};
/* Entity: app 独占电网资格状态；用户请求与送往 FSM 的有效请求分开。
 * Prior: 使用实测 RMS 与 PLL 角频率；条件不成立不启动，异常撤销资格。
 * Time: 每 1 ms 检查一次，连续 200 ms 合格；等待电网时只轮询，不阻塞且不授予运行许可。 */
static chk_grid_t grid_check;
static float    i_grid_alpha_raw;         /* Mean input inductor current supplied by the model, A. */
static float    v_bus_raw[CHB_CELL_COUNT];
static float    i_load_raw[CHB_CELL_COUNT];               /* 负载支路电流，母线流向负载为正，不含电容电流。 */
static uint8_t  run_request;                              /* FRAME writable; applied by the 1 ms app task. */
static uint32_t run_state;                                /* FRAME readable FSM state. */
static float    duty[CHB_CELL_COUNT];                     /* Last modulation frame for FRAME inspection. */
static float    v_pwm_cmd[CHB_CELL_COUNT];                /* 各桥最终调制电压，V。 */
static float    r_load_cmd[CHB_CELL_COUNT];               /* FRAME staging values; LOAD_APPLY captures every cell. */
static float    r_load_pending[CHB_CELL_COUNT];           /* Snapshot consumed by the dedicated load task. */
static uint8_t  load_apply_pending;                       /* Shell and tasks run in the same PLECS scheduler context. */

REG_SHELL_VAR(RUN_REQUEST, run_request, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(RUN_STATE, run_state, SHELL_UINT32, 5u, 0u, NULL, SHELL_STA_NULL)
/* 仿真测试只读此现有时钟；停波期间也能按仿真时间验证资格与撤销请求。 */
REG_SHELL_VAR(SIM_TICK_100US, plecs_time_100us, SHELL_UINT32, UINT32_MAX, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_V, v_grid_raw, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(GRID_PLL_Q_ERROR_V, grid_measurement.inter.pll.inter.vq,
              SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_L, i_grid_alpha_raw, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_1_V, v_bus_raw[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_2_V, v_bus_raw[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_3_V, v_bus_raw[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_4_V, v_bus_raw[3], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_5_V, v_bus_raw[4], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_AUTO)
REG_SHELL_VAR(BUS_1_I, i_load_raw[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_2_I, i_load_raw[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_3_I, i_load_raw[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_4_I, i_load_raw[3], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(BUS_5_I, i_load_raw[4], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_1, duty[0], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_2, duty[1], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_3, duty[2], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_4, duty[3], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DUTY_5, duty[4], SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_1_V, v_pwm_cmd[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_2_V, v_pwm_cmd[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_3_V, v_pwm_cmd[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_4_V, v_pwm_cmd[3], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(VPWM_5_V, v_pwm_cmd[4], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R1, r_load_cmd[0], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R2, r_load_cmd[1], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R3, r_load_cmd[2], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R4, r_load_cmd[3], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(LOAD_R5, r_load_cmd[4], SHELL_FP32, CHB_APP_LOAD_OFF_OHM, CHB_APP_LOAD_MIN_OHM, NULL,
              SHELL_STA_NULL)

/** @brief Validate and snapshot a load step at the simulation command boundary. */
static void load_apply_command(shell_core_io_t *p_io)
{
    (void)p_io;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        if (!(    (r_load_cmd[cell] >= CHB_APP_LOAD_MIN_OHM)
               && (r_load_cmd[cell] <= CHB_APP_LOAD_OFF_OHM)))
        {
            PLECS_LOG("LOAD_APPLY rejected: resistance outside 1..1e6 ohm\n");
            return;
        }
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        r_load_pending[cell] = r_load_cmd[cell];
    }
    load_apply_pending = 1u;
}
REG_SHELL_CMD(LOAD_APPLY, load_apply_command)

static void clear_fault_command(shell_core_io_t *p_io)
{
    (void)p_io;
    (void)chb_fsm_clear_fault();
    (void)chb_protect_clear_latch();
}
REG_SHELL_CMD(CLEAR_FAULT, clear_fault_command)

static void pwm_write_cell(uint32_t                cell,
                           float v_pwm_ref,
                           float v_bus_sample,
                           chb_pwm_deadtime_flag_t deadtime_flag)
{
    float modulation = bsp_pwm_deadtime_compensate(v_pwm_ref / v_bus_sample, (int8_t)deadtime_flag);
    UP_DN_LMT(modulation, 1.0f, -1.0f);
    v_pwm_cmd[cell] = v_pwm_ref;
    duty[cell]      = 0.5f * (modulation + 1.0f);
    chb_pwm_set_cell(cell, v_pwm_ref, v_bus_sample, deadtime_flag);
}

static void pwm_write_cell_1(float v_pwm_ref, float v_bus_sample, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(0u, v_pwm_ref, v_bus_sample, deadtime_flag);
}

static void pwm_write_cell_2(float v_pwm_ref, float v_bus_sample, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(1u, v_pwm_ref, v_bus_sample, deadtime_flag);
}

static void pwm_write_cell_3(float v_pwm_ref, float v_bus_sample, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(2u, v_pwm_ref, v_bus_sample, deadtime_flag);
}

static void pwm_write_cell_4(float v_pwm_ref, float v_bus_sample, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(3u, v_pwm_ref, v_bus_sample, deadtime_flag);
}

static void pwm_write_cell_5(float v_pwm_ref, float v_bus_sample, chb_pwm_deadtime_flag_t deadtime_flag)
{
    pwm_write_cell(4u, v_pwm_ref, v_bus_sample, deadtime_flag);
    float theta_grid_observed;
    float i_grid_beta;

    if ((plecs_time_100us % 5u) == 0u)
    {
        chb_ctrl_read_phase(&theta_grid_observed, &i_grid_beta);
        PLECS_LOG("PWM vg=%g il=%g ib=%g th=%g bus=%g,%g,%g,%g,%g vcmd=%g duty=%g,%g,%g,%g,%g vpwm=%g,%g,%g,%g,%g\n",
                  (double)v_grid_raw,
                  (double)i_grid_alpha_raw,
                  (double)i_grid_beta,
                  (double)theta_grid_observed,
                  (double)v_bus_raw[0],
                  (double)v_bus_raw[1],
                  (double)v_bus_raw[2],
                  (double)v_bus_raw[3],
                  (double)v_bus_raw[4],
                  (double)(v_pwm_cmd[0] + v_pwm_cmd[1] + v_pwm_cmd[2] + v_pwm_cmd[3] + v_pwm_cmd[4]),
                  (double)duty[0],
                  (double)duty[1],
                  (double)duty[2],
                  (double)duty[3],
                  (double)duty[4],
                  (double)v_pwm_cmd[0],
                  (double)v_pwm_cmd[1],
                  (double)v_pwm_cmd[2],
                  (double)v_pwm_cmd[3],
                  (double)v_pwm_cmd[4]);
    }
}

static void pwm_disable(void)
{
    chb_pwm_disable();

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        duty[cell]      = 0.0f;
        v_pwm_cmd[cell] = 0.0f;
    }
}

static void soft_start_relay_on(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 1.0f);
}

static void soft_start_relay_off(void)
{
    plecs_set_output(PLECS_OUTPUT_SOFT_START_RELAY, 0.0f);
}

static void main_relay_on(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 1.0f);
}

static void main_relay_off(void)
{
    plecs_set_output(PLECS_OUTPUT_MAIN_RELAY, 0.0f);
}

/**
 * @brief 初始化 app 电网测量模块的滑窗、正交发生器及 PLL。
 * @param[in,out] p_module 本模块的 input / inter / output；输入地址在整个运行期有效。
 * @param[in] p_cfg 已由 cfg 接口验证的 INIT 配置，仅在此处读取。
 * @return 1：依赖及窗口均已初始化；0：配置不可用，输出保持 NAN。
 * @details 窗长取额定一周期的采样点数并限制在固定容量内，周期计算不遍历窗口。
 *          PLL 的 MSOGI 前端、带宽及频率上下界沿用原设置，不重置每拍锁相历史。
 */
static uint8_t app_grid_init(chb_app_grid_t *p_module, const chb_ctrl_cfg_t *p_cfg)
{
    p_module->inter.ready = 0u;
    p_module->inter.v_grid_sample = 0.0f;
    p_module->inter.v_grid_squared = 0.0f;
    p_module->inter.ticks = 0u;
    p_module->output.v_grid_rms = NAN;
    p_module->output.omega_grid = NAN;
    p_module->output.theta_grid = NAN;

    const float window_points = 1.0f / (p_cfg->f_grid * p_cfg->t_ctrl_period);

    if (!(    (window_points >= 1.0f)
           && (window_points <= (float)CHB_APP_GRID_RMS_CAPACITY)))
    {
        return 0u;
    }
    const uint32_t window_length = (uint32_t)(window_points + 0.5f);
    const float omega_center = M_2PI * p_cfg->f_grid;
    const float v_grid_peak = M_SQRT2 * p_cfg->v_grid_rms_nominal;
    const float omega_pll_n = M_2PI * CHB_APP_PLL_BANDWIDTH_HZ;

    if (sliding_window_init(&p_module->inter.rms_window,
                            p_module->inter.v_grid_square_history, window_length,
                            &p_module->inter.v_grid_squared) == 0u)
    {
        return 0u;
    }
    if (msogi_init(&p_module->inter.msogi, p_cfg->t_ctrl_period, omega_center,
                   CHB_APP_GRID_SOGI_K, &p_module->inter.v_grid_sample) == 0u)
    {
        return 0u;
    }

    if (!pll_init(&p_module->inter.pll,
                  p_cfg->t_ctrl_period, omega_center,
                  omega_center + M_2PI * CHB_APP_PLL_UP_HZ,
                  omega_center - M_2PI * CHB_APP_PLL_DOWN_HZ,
                  v_grid_peak, CHB_APP_PLL_ZETA, omega_pll_n,
                  M_2PI * CHB_APP_PLL_UP_HZ, -M_2PI * CHB_APP_PLL_DOWN_HZ,
                  0.0f, &p_module->inter.msogi.alpha[0], &p_module->inter.msogi.beta[0]))
    {
        return 0u;
    }
    /* 通用 PLL 初始化后，通过现有系数入口应用 cfg.h 宏；仅在 INIT 执行。 */
    if (!pll_update_pi(&p_module->inter.pll,
                       CHB_PLL_PI_KP_CALC(v_grid_peak, omega_pll_n, CHB_APP_PLL_ZETA),
                       CHB_PLL_PI_KI_CALC(v_grid_peak, omega_pll_n)))
    {
        return 0u;
    }
    p_module->output.omega_grid = omega_center;
    p_module->output.theta_grid = p_module->inter.pll.output.theta;
    p_module->inter.ready = 1u;
    return 1u;
}

/**
 * @brief 计算本拍电网电压 RMS 和 PLL 的角频率、相位。
 * @param[in,out] p_module 已初始化的本模块；每拍读取自己的输入并推进一次历史。
 * @details MSOGI 基波电压平方 -> 固定一周期滑窗均值 -> 开方，启动等待和窗口填充均完成后发布 RMS。
 *          MSOGI 用上一拍 PLL 角频率生成正交电压，PLL 输出直接发布给 HAL。
 *          PLL 执行失败将输出标为 NAN 并记录错误，后续由同拍保护禁止发波。
 */
static void app_grid_cal(chb_app_grid_t *p_module)
{
    p_module->inter.v_grid_sample = *p_module->input.p_v_grid_raw;
    msogi_update_frequency(&p_module->inter.msogi, p_module->output.omega_grid);
    msogi_cal(&p_module->inter.msogi); /* 本拍电压 -> 基波及 PLL 正交输入。 */
    p_module->inter.v_grid_squared = p_module->inter.msogi.alpha[0] * p_module->inter.msogi.alpha[0];
    sliding_window_cal(&p_module->inter.rms_window); /* 电压平方 -> 一周期均值。 */

    if (p_module->inter.ticks < CHB_APP_GRID_READY_TICKS)
    {
        ++p_module->inter.ticks;
    }
    else if (p_module->inter.rms_window.inter.count == p_module->inter.rms_window.input.length)
    {
        float v_grid_mean_square = p_module->inter.rms_window.output.mean;
        DN_LMT(v_grid_mean_square, 0.0f); /* 限制累计舍入误差，NAN 不被隐藏。 */
        p_module->output.v_grid_rms = sqrtf(v_grid_mean_square);
    }
    if (!pll_cal(&p_module->inter.pll))
    {
        p_module->output.omega_grid = NAN;
        p_module->output.theta_grid = NAN;
        p_module->inter.ready = 0u; /* 失败后不重复执行，保留现场至重新初始化。 */
        PLECS_LOG("CHB grid PLL calculation failed\n");
        return;
    }
    p_module->output.omega_grid = p_module->inter.pll.output.omega;
    p_module->output.theta_grid = p_module->inter.pll.output.theta;
}

static void app_init(void)
{
    /* 发布新一轮初始化前先撤销旧测量，失败时不得沿用上一轮锁相结果。 */
    grid_measurement.inter.ready = 0u;
    grid_measurement.output.v_grid_rms = NAN;
    grid_measurement.output.omega_grid = NAN;
    grid_measurement.output.theta_grid = NAN;
    chk_grid_reset(&grid_check); /* 新一轮启动不得继承上次电网资格。 */

    const chb_ctrl_hal_t binding = {
        .p_v_grid_raw           = &v_grid_raw,
        .p_v_grid_rms           = &grid_measurement.output.v_grid_rms,
        .p_omega_grid           = &grid_measurement.output.omega_grid,
        .p_theta_grid           = &grid_measurement.output.theta_grid,
        .p_i_grid_alpha_raw     = &i_grid_alpha_raw,
        .p_v_bus_raw            = {&v_bus_raw[0], &v_bus_raw[1], &v_bus_raw[2], &v_bus_raw[3], &v_bus_raw[4]},
        .p_i_load_raw           = {&i_load_raw[0], &i_load_raw[1], &i_load_raw[2], &i_load_raw[3], &i_load_raw[4]},
        .p_set_pwm_func[0]      = pwm_write_cell_1,
        .p_set_pwm_func[1]      = pwm_write_cell_2,
        .p_set_pwm_func[2]      = pwm_write_cell_3,
        .p_set_pwm_func[3]      = pwm_write_cell_4,
        .p_set_pwm_func[4]      = pwm_write_cell_5,
        .p_pwm_disable          = pwm_disable,
        .p_soft_start_relay_on  = soft_start_relay_on,
        .p_soft_start_relay_off = soft_start_relay_off,
        .p_main_relay_on        = main_relay_on,
        .p_main_relay_off       = main_relay_off,
    };
    chb_ctrl_cfg_t cfg = chb_cfg_default();                       /* 平台确定采样链延迟，其余沿用控制默认值。 */
    cfg.t_current_sample_delay = 4.5f * cfg.t_ctrl_period / 8.0f; /* 模型 z^-1..z^-8 求平均的群延迟。 */

    run_request              = 0u;
    run_state                = (uint32_t)CHB_RUN_STATE_INIT;
    load_apply_pending       = 0u;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        v_bus_raw[cell]      = 0.0f;
        i_load_raw[cell]     = 0.0f;
        r_load_cmd[cell]     = CHB_APP_LOAD_OFF_OHM;
        r_load_pending[cell] = CHB_APP_LOAD_OFF_OHM;
        /* Valid positive resistance is required at t=0, before the first task tick. */
        plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_LOAD_R1 + cell), CHB_APP_LOAD_OFF_OHM);
    }
    (void)chb_cfg_set_run_request(0u);
    if (chb_cfg_set_ctrl_cfg(&cfg) == 0u)
    {
        PLECS_LOG("CHB control configuration rejected\n");
        pwm_disable();
        main_relay_off();
        return;
    }
    if (app_grid_init(&grid_measurement, &cfg) == 0u)
    {
        PLECS_LOG("CHB grid measurement initialization failed\n");
        pwm_disable();
        main_relay_off();
        return;
    }
    /* chk_grid 的计数判据为 > judge_time，199 对应 200 个 1 ms 调用。
     * 正常与异常使用相同区间，异常计数阈值为 0，首个不合格调用即撤销资格。
     * 频率输入直接绑定 rad/s，初始化时将 Hz 阈值乘 2*pi，周期内不重复换算。 */
    const float v_grid_min = CHB_APP_GRID_V_MIN_RATIO * cfg.v_grid_rms_nominal;
    const float v_grid_max = CHB_APP_GRID_V_MAX_RATIO * cfg.v_grid_rms_nominal;
    const float omega_grid_min = M_2PI * CHB_APP_GRID_F_MIN_HZ;
    const float omega_grid_max = M_2PI * CHB_APP_GRID_F_MAX_HZ;
    chk_grid_init(&grid_check, &grid_measurement.output.v_grid_rms,
                   &grid_measurement.output.omega_grid,
                   CHB_APP_GRID_JUDGE_MS / CHB_APP_TASK_MS - 1u, 0u,
                   v_grid_max, v_grid_min, v_grid_max, v_grid_min,
                   omega_grid_max, omega_grid_min, omega_grid_max, omega_grid_min);
    (void)chb_protect_configure(CHB_APP_CURRENT_TRIP_A, CHB_APP_PRECHARGE_TRIP_A,
                                CHB_APP_BUS_MIN_V, CHB_APP_BUS_MAX_V);
    (void)chb_hal_bind(&binding);
    pwm_disable();
    main_relay_off();
}
REG_INIT(0, app_init)

/** @brief 采集模型瞬时输入并完成 app 的 RMS、PLL 计算，再由 ctrl 读取 HAL。 */
static void app_sample(void)
{
    v_grid_raw       = plecs_get_input(PLECS_INPUT_GRID_V);
    i_grid_alpha_raw = plecs_get_input(PLECS_INPUT_I_L);
    v_bus_raw[0]     = plecs_get_input(PLECS_INPUT_BUS_1_V);
    v_bus_raw[1]     = plecs_get_input(PLECS_INPUT_BUS_2_V);
    v_bus_raw[2]     = plecs_get_input(PLECS_INPUT_BUS_3_V);
    v_bus_raw[3]     = plecs_get_input(PLECS_INPUT_BUS_4_V);
    v_bus_raw[4]     = plecs_get_input(PLECS_INPUT_BUS_5_V);
    i_load_raw[0]    = plecs_get_input(PLECS_INPUT_BUS_1_I);
    i_load_raw[1]    = plecs_get_input(PLECS_INPUT_BUS_2_I);
    i_load_raw[2]    = plecs_get_input(PLECS_INPUT_BUS_3_I);
    i_load_raw[3]    = plecs_get_input(PLECS_INPUT_BUS_4_I);
    i_load_raw[4]    = plecs_get_input(PLECS_INPUT_BUS_5_I);

    if (grid_measurement.inter.ready != 0u)
    {
        app_grid_cal(&grid_measurement); /* 瞬时电压 -> RMS、角频率及相位。 */
    }
}

REG_INTERRUPT(0, app_sample)

/**
 * @brief 判断电网电压、频率资格，再向 FSM 发布有效开停机请求。
 * @details 每 1 ms 持续检查 app 已完成的 RMS / PLL 结果，不受 Shell 启动请求影响。
 *          连续合格 200 ms 才置位 grid_check.output.is_ok；Shell 停止时仍持续判断。
 *          初始化/PLL 执行失败时复位资格；电网越界在本次任务撤销有效请求。
 *          电网未合格或保护闭锁时清除 Shell 的 RUN_REQUEST；恢复后须由 Shell 重新请求启动。
 *          app 只发布请求，预充、主继电器时序和最终运行许可仍由 FSM 控制。
 */
static void app_task(void)
{
    if (grid_measurement.inter.ready != 0u)
    {
        chk_grid_func(&grid_check); /* 实测 RMS、角频率 -> 电网启动资格。 */
    }
    else
    {
        chk_grid_reset(&grid_check);
    }

    if (    (grid_check.output.is_ok == 0u)
         || (chb_protect_is_tripped() != 0u))
    {
        if (run_request != 0u)
        {
            PLECS_LOG("START rejected grid_ok=%u protection=%u count=%u rms=%g hz=%g\n",
                      (unsigned int)grid_check.output.is_ok,
                      (unsigned int)chb_protect_is_tripped(),
                      (unsigned int)grid_check.inter.is_ok_cnt,
                      (double)grid_measurement.output.v_grid_rms,
                      (double)(grid_measurement.output.omega_grid / M_2PI));
        }
        run_request = 0u; /* 失效的 Shell 启动请求作废，电网恢复后不会自动重新启动。 */
    }
    const uint8_t run_effective = ((run_request != 0u) && (grid_check.output.is_ok != 0u)) ? 1u : 0u;
    (void)chb_cfg_set_run_request(run_effective); /* 用户请求且电网合格，才允许 FSM 开始启动。 */
    run_state = (uint32_t)chb_fsm_get_run_state();
}
REG_TASK_MS(CHB_APP_TASK_MS, app_task)

/** @brief Apply all cell resistances in one callback, independently of PWM and the FSM. */
static void app_load_task(void)
{
    if (load_apply_pending != 0u)
    {
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            plecs_set_output((PLECS_OUTPUT_E)(PLECS_OUTPUT_LOAD_R1 + cell), r_load_pending[cell]);
        }
        load_apply_pending = 0u;
        PLECS_LOG("LOAD r=%g,%g,%g,%g,%g ohm\n",
                  (double)r_load_pending[0],
                  (double)r_load_pending[1],
                  (double)r_load_pending[2],
                  (double)r_load_pending[3],
                  (double)r_load_pending[4]);
    }
}
REG_TASK_MS(CHB_APP_LOAD_TASK_MS, app_load_task)
