// SPDX-License-Identifier: MIT
/**
 * @file chb_ctrl.c
 * @brief CHB per-cell energy/power loops and minimum-current dq allocation.
 * @details Sampling, protection and PWM execute at priorities 1, 2 and 3 in one
 *          serialized control period. This module owns grid/current SOGI/MSOGI
 *          observers and PLL state ahead of protection. Initial gains are from
 *          the 6 kV/10 kHz precharged MATLAB study, not hardware validation.
 * @author Max.Li
 * @date 2026-09-25
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "chb_ctrl.h"
#include "chb_cfg.h"
#include "chb_fsm.h"
#include "chb_hal.h"
#include "msogi.h"
#include "my_math.h"
#include "notch.h"
#include "pll.h"
#include "section.h"
#include "sogi.h"
#if defined(PLATFORM_PLECS)
#include "shell.h"
#include <float.h>
#endif

#include <math.h>
#include <stdbool.h>
#include <string.h>

#define CHB_VOLTAGE_PHASE_POINTS   256u /* 覆盖九次谐波陡变，网格间误差由二阶导数上界覆盖。 */
#define CHB_PWM_BASE_DELAY_PERIODS 1.5f /* BSP 一拍加单次更新三角 PWM 的半周期。 */

#define CHB_OBSERVER_SOGI_K            1.414213562f
#define CHB_OBSERVER_CURRENT_MSOGI_K   0.05f
#define CHB_OBSERVER_RMS_READY_TICKS   400u
#define CHB_OBSERVER_PLL_ZETA          0.707106781f
#define CHB_OBSERVER_PLL_BANDWIDTH_HZ  20.0f
#define CHB_OBSERVER_PLL_UP_HZ         25.0f
#define CHB_OBSERVER_PLL_DOWN_HZ       15.0f
#define CHB_OBSERVER_HARM_SETTLE_TICKS 2000u
#define CHB_OBSERVER_HARM_RESTORE_STEP 0.0005f

static chb_observer_sample_t sample                       = {0};  /* 保护和控制共用的本拍观测结果。 */
static float                 raw_grid_v                   = 0.0f; /* 从 HAL 输入指针取得的本拍电网电压，V。 */
static float                 raw_current_a                = 0.0f; /* 从 HAL 输入指针取得的本拍电感电流，A。 */
static float                 raw_bus_v[CHB_CELL_COUNT]    = {0};  /* 本拍各桥母线原始采样，V。 */
static float                 raw_load_i_a[CHB_CELL_COUNT] = {0};  /* 本拍各桥负载电流原始采样，A。 */
static msogi_t               grid_msogi                   = {0};
static msogi_t               current_msogi                = {0};
static sogi_t                current_sogi                 = {0};
static sogi_t                current_quadrature_sogi      = {0};
static pll_t                 grid_pll                     = {0};
static float                 current_input_a              = 0.0f; /* SOGI 的可写绑定源，仅保存本拍原始电流。 */
static float                 current_ac_a                 = 0.0f;
static float                 grid_frequency_filtered      = 0.0f;
static uint32_t              observer_ticks               = 0u;
static uint32_t              harmonic_settle_ticks        = 0u;
static uint8_t               ready                        = 0u;

/** @brief 控制器内部用于生成最终逐级发波电压的同拍计划。 */
typedef struct
{
    float bus_v[CHB_CELL_COUNT];
    float cell_d_v[CHB_CELL_COUNT];
    float cell_q_v[CHB_CELL_COUNT];
    float i_comp_ref_a;
    float i_comp_beta_ref_a;
    float theta_rad;
    float harmonic_alpha_v[CHB_HARMONIC_COUNT];
    float harmonic_beta_v[CHB_HARMONIC_COUNT];
    float grid_hz;
} chb_pwm_plan_t;

/** @brief Bind all library observers after the platform has published its CHB configuration. */
static void chb_observer_init(void)
{
    const chb_ctrl_cfg_t *p_cfg = chb_cfg_get_ctrl_cfg();
    float center_omega          = 0.0f;

    ready = 0u;
    (void)memset(&sample, 0, sizeof(sample));
    observer_ticks        = 0u;
    harmonic_settle_ticks = 0u;
    raw_grid_v            = 0.0f;
    raw_current_a         = 0.0f;
    current_input_a       = 0.0f;
    current_ac_a          = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        raw_bus_v[cell]    = 0.0f;
        raw_load_i_a[cell] = 0.0f;
    }

    if (    (chb_cfg_is_ready() == 0u)
         || (chb_hal_is_ready() == 0u))
    {
        return;
    }

    center_omega            = M_2PI * p_cfg->grid_hz;
    sample.grid_rms_v       = p_cfg->grid_rms_nominal_v;
    sample.grid_hz          = p_cfg->grid_hz;
    grid_frequency_filtered = p_cfg->grid_hz;

    if (    (msogi_init(&grid_msogi,
                        p_cfg->ts,
                        center_omega,
                        CHB_OBSERVER_SOGI_K,
                        &raw_grid_v) == 0u)
         || (msogi_init(&current_msogi,
                        p_cfg->ts,
                        center_omega,
                        CHB_OBSERVER_CURRENT_MSOGI_K,
                        &raw_current_a) == 0u))
    {
        return;
    }
    sogi_init(&current_sogi,
              p_cfg->ts,
              center_omega,
              CHB_OBSERVER_SOGI_K,
              &current_input_a);
    sogi_init(&current_quadrature_sogi,
              p_cfg->ts,
              center_omega,
              CHB_OBSERVER_SOGI_K,
              &current_ac_a);

    if (!pll_init(&grid_pll,
                  p_cfg->ts,
                  center_omega,
                  center_omega + M_2PI * CHB_OBSERVER_PLL_UP_HZ,
                  center_omega - M_2PI * CHB_OBSERVER_PLL_DOWN_HZ,
                  M_SQRT2 * p_cfg->grid_rms_nominal_v,
                  CHB_OBSERVER_PLL_ZETA,
                  M_2PI * CHB_OBSERVER_PLL_BANDWIDTH_HZ,
                  M_2PI * CHB_OBSERVER_PLL_UP_HZ,
                  -M_2PI * CHB_OBSERVER_PLL_DOWN_HZ,
                  0.0f,
                  &grid_msogi.alpha[0],
                  &grid_msogi.beta[0]))
    {
        return;
    }
    ready = 1u;
}
REG_INIT(1, chb_observer_init)

uint8_t chb_observer_is_ready(void)
{
    return ready;
}

static void FUNC_RAM chb_observer_update(void)
{
    float measured_angle = 0.0f;

    sample.grid_v    = raw_grid_v;
    sample.i_alpha_a = raw_current_a;
    current_input_a  = raw_current_a;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        sample.bus_v[cell]    = raw_bus_v[cell];
        sample.load_i_a[cell] = raw_load_i_a[cell];
    }

    msogi_update_frequency(&grid_msogi, M_2PI * sample.grid_hz);
    msogi_cal(&grid_msogi);
    msogi_update_frequency(&current_msogi, M_2PI * sample.grid_hz);
    msogi_cal(&current_msogi);
    sogi_update_frequency(&current_sogi, M_2PI * sample.grid_hz);
    sogi_update_frequency(&current_quadrature_sogi, M_2PI * sample.grid_hz);
    sogi_cal(&current_sogi);
    current_ac_a = current_sogi.osg_u[0];
    sogi_cal(&current_quadrature_sogi);
    sample.i_beta_a = current_quadrature_sogi.osg_qu[0];

    measured_angle = atan2f(grid_msogi.beta[0], grid_msogi.alpha[0]);

    if (observer_ticks < CHB_OBSERVER_RMS_READY_TICKS)
    {
        pll_reset(&grid_pll, measured_angle);
        sample.theta_rad            = grid_pll.output.theta;
        sample.grid_phase_error_rad = 0.0f;
        ++observer_ticks;
    }
    else if (pll_cal(&grid_pll))
    {
        sample.theta_rad            = grid_pll.output.theta;
        sample.grid_hz              = grid_pll.output.omega / M_2PI;
        sample.grid_phase_error_rad = atan2f(grid_pll.inter.vq, grid_pll.inter.vd);
        const float measured_rms_v  = hypotf(grid_msogi.alpha[0], grid_msogi.beta[0]) / M_SQRT2;
        sample.grid_rms_v += 0.01f * (measured_rms_v - sample.grid_rms_v);
    }
    else
    {
        sample.grid_hz = NAN; /* Protection rejects a failed observer before PWM. */
        return;
    }

    sample.grid_frequency_rate_hz_s = (sample.grid_hz - grid_frequency_filtered) / 0.01f;
    grid_frequency_filtered += 0.01f * (sample.grid_hz - grid_frequency_filtered);

    if (    (observer_ticks < CHB_OBSERVER_RMS_READY_TICKS)
         || (fabsf(sample.grid_frequency_rate_hz_s) > 10.0f)
         || (fabsf(sample.grid_phase_error_rad) > 0.005f))
    {
        harmonic_settle_ticks           = 0u;
        sample.harmonic_feedback_weight = 0.0f;
    }
    else if (harmonic_settle_ticks < CHB_OBSERVER_HARM_SETTLE_TICKS)
    {
        ++harmonic_settle_ticks;
    }
    else
    {
        sample.harmonic_feedback_weight =
            fminf(1.0f, sample.harmonic_feedback_weight + CHB_OBSERVER_HARM_RESTORE_STEP);
    }

    sample.grid_fundamental_beta_v = grid_msogi.beta[0];

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const uint32_t channel                    = harmonic + 1u;
        sample.grid_harmonic_alpha_v[harmonic]    = grid_msogi.alpha[channel];
        sample.grid_harmonic_beta_v[harmonic]     = grid_msogi.beta[channel];
        sample.current_harmonic_alpha_a[harmonic] = current_msogi.alpha[channel];
        sample.current_harmonic_beta_a[harmonic]  = current_msogi.beta[channel];
        sample.grid_harmonic_peak_v[harmonic]     = hypotf(grid_msogi.alpha[channel], grid_msogi.beta[channel]);
        sample.current_harmonic_peak_a[harmonic] =
            hypotf(current_msogi.alpha[channel], current_msogi.beta[channel]);
    }
}

void FUNC_RAM chb_ctrl_update_sample(void)
{
    const chb_ctrl_hal_t *p_hal = chb_hal_get_ctrl(); /* INIT 已验证并锁定的采样源。 */

    raw_grid_v    = *p_hal->p_grid_v;
    raw_current_a = *p_hal->p_i_alpha_a;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        raw_bus_v[cell]    = *p_hal->p_bus_v[cell];
        raw_load_i_a[cell] = *p_hal->p_load_i_a[cell];
    }
    chb_observer_update(); /* 所有派生量均基于上面取得的同拍静态副本。 */
}

const chb_observer_sample_t *chb_observer_get_sample(void)
{
    return &sample;
}

/* 无功规划仅接收本拍求解所需的量；数组指针在同步调用期间有效。 */
typedef struct chb_q_plan_input
{
    const float *p_id_ref_a;       /* 已限幅有功电流给定，A 峰值。 */
    const float *p_grid_rms_v;     /* 实测电网基波有效值，V。 */
    const float *p_delta_power_w;  /* 各桥相对平均值的零和有功请求，W。 */
    const float *p_bus_filtered_v; /* 各桥用于容量判定的母线电压，V。 */
    const float *p_grid_omega;     /* 实测电网角频率，rad/s。 */
} chb_q_plan_input_t;

typedef struct chb_q_plan_inter
{
    float    magnitude_ref_a; /* 经斜坡的负 q 电流幅值，A 峰值。 */
    float    target_a;        /* 周期可行域目标幅值，A 峰值。 */
    uint32_t ticks;           /* 距上次完整搜索的控制拍数。 */
    bool     zero_feasible;   /* 本次搜索中零无功的可行性。 */
} chb_q_plan_inter_t;

typedef struct chb_q_plan_output
{
    float ref; /* 唯一输出：带符号 q 轴电流给定，A 峰值。 */
} chb_q_plan_output_t;

typedef struct chb_ctrl_inter
{
    float              bus_notch_input_v[CHB_CELL_COUNT];   /* 陷波器使用的本拍母线电压副本，V。 */
    notch_t            bus_notch[CHB_CELL_COUNT];           /* 三路独立的二倍工频陷波状态。 */
    float              bus_filtered_v[CHB_CELL_COUNT];      /* 抑制二倍工频纹波后的各级电压，V。 */
    float              power_notch_input_w[CHB_CELL_COUNT]; /* 同拍母线电压乘负载电流，W。 */
    notch_t            power_notch[CHB_CELL_COUNT];         /* 各桥负载功率二倍工频陷波。 */
    float              load_power_w[CHB_CELL_COUNT];        /* 陷波及低通后的负载功率，W。 */
    float              energy_integral_w[CHB_CELL_COUNT];   /* 每桥能量环积分输出，W。 */
    float              power_integral_w[CHB_CELL_COUNT];    /* 每桥功率上限环积分输出，W。 */
    chb_q_plan_inter_t q_plan;                  /* 无功规划独立的搜索与斜坡状态。 */
    float              current_phase_cos;       /* 已知采样延迟对应的工频相位余弦。 */
    float              current_phase_sin;       /* 已知采样延迟对应的工频相位正弦。 */
    float              current_integral_d_v;    /* d 轴内环积分输出，V。 */
    float              current_integral_q_v;    /* q 轴内环积分输出，V。 */
    float              bus_filter_weight;       /* 每控制拍的一阶低通离散权重。 */
    float              bus_ref_ramped_v;        /* 每级母线正在执行的电压给定，V。 */
    float              harmonic_reserve_pu;     /* 各桥按母线比例预留的谐波峰值预算。 */
    float              harmonic_scale;          /* 谐波前馈在物理容量内的统一缩放。 */
    float              harmonic_feedback_scale; /* 基波与电网前馈优先后的电流谐波反馈比例。 */
    float              grid_omega;              /* 本拍观测的电网基波角频率，rad/s。 */
    float              harmonic_wave_pu[CHB_VOLTAGE_PHASE_POINTS]; /* 每伏母线对应的完整谐波波形。 */
    float              harmonic_curvature_pu; /* 谐波对电角度的二阶导数幅值上界。 */
} chb_ctrl_inter_t;

static chb_ctrl_cfg_t   cfg            = {0};                   /* INIT 后只读的控制系数副本。 */
static chb_ctrl_inter_t inter          = {0};                   /* 控制阶段唯一写入的积分与滤波动态。 */
static bool             sample_allowed = false;                 /* 本拍采样已完成且未被应用保护禁止。 */
static float            phase_cosine[CHB_VOLTAGE_PHASE_POINTS]; /* INIT 生成的基波约束节点。 */
static float            phase_sine[CHB_VOLTAGE_PHASE_POINTS];
static float            harmonic_cosine[CHB_HARMONIC_COUNT][CHB_VOLTAGE_PHASE_POINTS];
static float            harmonic_sine[CHB_HARMONIC_COUNT][CHB_VOLTAGE_PHASE_POINTS];

#if defined(PLATFORM_PLECS)
typedef struct chb_ctrl_diag
{
    float   bus_sum_v;           /* 同拍三路母线原始电压和，V。 */
    float   bus_filtered_sum_v;  /* 三路陷波及低通后的母线电压和，V。 */
    float   bus_error_v;         /* 总母线外环误差，V。 */
    float   bus_ref_ramped_v;    /* 每级母线当前斜坡给定，V。 */
    float   id_ref_a;            /* 限幅后的 d 轴电流参考，A 峰值。 */
    float   iq_ref_a;            /* 轻载均衡电流的 q 轴参考，A 峰值。 */
    float   balance_scale;       /* 差模电压共用限幅系数，0..1。 */
    float   balance_utilization; /* 调节无功电流使用的容量利用率。 */
    float   q_min_a;             /* 规划器要求的最小无功电流幅值，A。 */
    uint8_t pf_zero_feasible;    /* 零无功电流的静态可行性。 */
    float   id_a;                /* d 轴电流反馈，A 峰值。 */
    float   iq_a;                /* q 轴电流反馈，A 峰值。 */
    float   vd_pwm_v;            /* 总桥 d 轴电压指令，V。 */
    float   vq_pwm_v;            /* 总桥 q 轴电压指令，V。 */
    float   vpwm_v;              /* 逆 Park 后的总桥瞬时电压指令，V。 */
    float   balance_error_v[CHB_CELL_COUNT]; /* 各桥相对平均母线的误差，V。 */
    float   power_request_w[CHB_CELL_COUNT]; /* 每桥限功率后的有功请求，W。 */
    uint8_t power_limited[CHB_CELL_COUNT];   /* 功率选择器接管该桥稳压。 */
    uint8_t total_limited;                   /* 总桥矢量限幅标志。 */
    uint8_t cell_limited; /* 至少一桥系数限幅标志。 */
} chb_ctrl_diag_t;

static chb_ctrl_diag_t diag = {0};                  /* FRAME 只观察副本，不写控制积分状态。 */
static float           fault_i_a;                   /* 首次保护闭锁时的电感电流，A。 */
static float           fault_bus_v[CHB_CELL_COUNT]; /* 首次保护闭锁时的各桥母线电压，V。 */
static float           fault_id_ref_a;              /* 首次保护闭锁前一拍的 d 轴电流给定，A。 */
static float           fault_id_a;                  /* 首次保护闭锁前一拍的 d 轴电流反馈，A。 */
static float           fault_vpwm_v;                /* 首次保护闭锁前一拍的总调制电压，V。 */
static uint8_t         fault_sample_valid;          /* 首次保护采样已经锁存。 */

REG_SHELL_VAR(CHB_BUS_SUM_V, diag.bus_sum_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_FILT_V, diag.bus_filtered_sum_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_ERR_V, diag.bus_error_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_REF_RAMP_V, diag.bus_ref_ramped_v, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_ID_REF_A, diag.id_ref_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_REF_A, diag.iq_ref_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_SCALE, diag.balance_scale, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_UTIL, diag.balance_utilization, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_MIN_A, diag.q_min_a, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_PF_ZERO_OK, diag.pf_zero_feasible, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_ID_A, diag.id_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_A, diag.iq_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VD_PWM_V, diag.vd_pwm_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VQ_PWM_V, diag.vq_pwm_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VPWM_V, diag.vpwm_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_1, diag.balance_error_v[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_2, diag.balance_error_v[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_3, diag.balance_error_v[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_1_W, inter.load_power_w[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_2_W, inter.load_power_w[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_3_W, inter.load_power_w[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_1_W, diag.power_request_w[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_2_W, diag.power_request_w[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_3_W, diag.power_request_w[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_1, diag.power_limited[0], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_2, diag.power_limited[1], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_3, diag.power_limited[2], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_TOTAL_LIM, diag.total_limited, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_CELL_LIM, diag.cell_limited, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_RESERVE_PU, inter.harmonic_reserve_pu, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_SCALE, inter.harmonic_scale, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_FB_SCALE, inter.harmonic_feedback_scale, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_I_A, fault_i_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_1_V, fault_bus_v[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_2_V, fault_bus_v[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_3_V, fault_bus_v[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_ID_REF_A, fault_id_ref_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_ID_A, fault_id_a, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_VPWM_V, fault_vpwm_v, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_VALID, fault_sample_valid, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
#endif

/** @param value 待限幅值。 @param lower 下界。 @param upper 上界。 @return 限幅结果。 */
static inline float limit_float(float value, float lower, float upper)
{
    return fminf(fmaxf(value, lower), upper);
}

/** @brief 清除积分历史及本拍许可；不访问未绑定的 PWM 回调。 */
static void reset_states(void)
{
    (void)memset(&inter, 0, sizeof(inter));
#if defined(PLATFORM_PLECS)
    (void)memset(&diag, 0, sizeof(diag));
#endif
    sample_allowed = false;
}

/** @brief 初始化数值配置，不读取尚未绑定的 ADC 指针。 */
static void chb_ctrl_init(void)
{
    cfg = *chb_cfg_get_ctrl_cfg();
    reset_states();

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float angle = M_2PI * (float)point / (float)CHB_VOLTAGE_PHASE_POINTS;
        phase_cosine[point] = cosf(angle);
        phase_sine[point]   = sinf(angle);

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            const float order = (float)(2u * harmonic + 3u);
            harmonic_cosine[harmonic][point] = cosf(order * angle);
            harmonic_sine[harmonic][point]   = sinf(order * angle);
        }
    }
#if defined(PLATFORM_PLECS)
    fault_i_a = 0.0f;
    (void)memset(fault_bus_v, 0, sizeof(fault_bus_v));
    fault_id_ref_a     = 0.0f;
    fault_id_a         = 0.0f;
    fault_vpwm_v       = 0.0f;
    fault_sample_valid = 0u;
#endif
    inter.bus_filter_weight = 1.0f - expf(-M_2PI * cfg.bus_filter_hz * cfg.ts);
    inter.current_phase_cos = cosf(M_2PI * cfg.grid_hz * cfg.current_sample_delay_s);
    inter.current_phase_sin = sinf(M_2PI * cfg.grid_hz * cfg.current_sample_delay_s);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        notch_init(&inter.bus_notch[cell],
                   M_2PI * 2.0f * cfg.grid_hz,
                   M_2PI * CHB_BUS_NOTCH_BANDWIDTH_HZ,
                   cfg.ts,
                   &inter.bus_notch_input_v[cell]);
        notch_init(&inter.power_notch[cell],
                   M_2PI * 2.0f * cfg.grid_hz,
                   M_2PI * CHB_BUS_NOTCH_BANDWIDTH_HZ,
                   cfg.ts,
                   &inter.power_notch_input_w[cell]);
    }
}
REG_INIT(2, chb_ctrl_init)

void chb_ctrl_prepare_run(void)
{
    const chb_observer_sample_t *p_sample = chb_observer_get_sample(); /* 进入 RUN 前的最新快照。 */
    chb_hal_get_ctrl()->p_pwm_disable(); /* 重启前先确保全部桥臂关闭。 */
    chb_ctrl_init(); /* 新一轮运行不继承上次积分。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        inter.bus_notch_input_v[cell]   = p_sample->bus_v[cell];
        inter.power_notch_input_w[cell] = p_sample->bus_v[cell] * p_sample->load_i_a[cell];

        for (uint32_t history = 0u; history < 3u; ++history)
        {
            inter.bus_notch[cell].inter.x[history]   = p_sample->bus_v[cell];
            inter.bus_notch[cell].inter.y[history]   = p_sample->bus_v[cell];
            inter.power_notch[cell].inter.x[history] = inter.power_notch_input_w[cell];
            inter.power_notch[cell].inter.y[history] = inter.power_notch_input_w[cell];
        }
        inter.bus_notch[cell].output.val   = p_sample->bus_v[cell]; /* 预充电压无扰进入滤波。 */
        inter.bus_filtered_v[cell]         = p_sample->bus_v[cell];
        inter.power_notch[cell].output.val = inter.power_notch_input_w[cell];
        inter.load_power_w[cell]           = inter.power_notch_input_w[cell];
        inter.bus_ref_ramped_v += p_sample->bus_v[cell] / (float)CHB_CELL_COUNT;
    }
}

void chb_ctrl_stop(void)
{
    chb_hal_get_ctrl()->p_pwm_disable();
    reset_states();
}

void chb_ctrl_inhibit(void)
{
#if defined(PLATFORM_PLECS)

    if (fault_sample_valid == 0u)
    {
        const chb_observer_sample_t *p_sample = chb_observer_get_sample(); /* 首次不健康的同拍采样。 */
        fault_i_a                             = p_sample->i_alpha_a;
        fault_id_ref_a                        = diag.id_ref_a;
        fault_id_a                            = diag.id_a;
        fault_vpwm_v                          = diag.vpwm_v;

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            fault_bus_v[cell] = p_sample->bus_v[cell];
        }
        fault_sample_valid = 1u;
    }
#endif
    sample_allowed = false; /* 应用保护已下发停波命令，本拍不再执行控制。 */
}

/** @brief 中断第一阶段形成一次输入快照。 */
static void FUNC_RAM chb_ctrl_sample(void)
{
    sample_allowed = false;

    if (chb_fsm_get_run_state() == CHB_RUN_STATE_INIT)
    {
        return;
    }
    chb_ctrl_update_sample();
    sample_allowed = true;
}
REG_INTERRUPT(1, chb_ctrl_sample)

/** @brief 将同拍谐波旋转到电网角度坐标，并生成每伏母线的周期约束。 */
static void FUNC_RAM voltage_wave_prepare(const chb_pwm_plan_t *p_command, float theta, float bus_sum)
{
    (void)memset(inter.harmonic_wave_pu, 0, sizeof(inter.harmonic_wave_pu));
    inter.harmonic_curvature_pu = 0.0f;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float order = (float)(2u * harmonic + 3u);
        const float cosine = cosf(order * theta);
        const float sine   = sinf(order * theta);
        const float alpha  = p_command->harmonic_alpha_v[harmonic] / bus_sum;
        const float beta   = p_command->harmonic_beta_v[harmonic] / bus_sum;
        const float d = alpha * cosine + beta * sine;
        const float q = -alpha * sine + beta * cosine;
        inter.harmonic_curvature_pu += order * order * hypotf(d, q);

        for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
        {
            inter.harmonic_wave_pu[point] += d * harmonic_cosine[harmonic][point]
                                           - q * harmonic_sine[harmonic][point];
        }
    }
}

/** @brief 线性插值误差 <= max|v''|*步长平方/8，覆盖节点间可能漏掉的波峰。 */
static float FUNC_RAM voltage_wave_guard(float bus_v)
{
    const float step = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;
    return (3.0f * cfg.modulation_limit + inter.harmonic_curvature_pu) * bus_v * step * step * 0.125f;
}

/** @brief 含相位电网前馈的周期峰值上界，用于确定选频电流反馈剩余容量。 */
static float FUNC_RAM grid_feedforward_peak(const chb_observer_sample_t *p_sample)
{
    float d[CHB_HARMONIC_COUNT] = {0};
    float q[CHB_HARMONIC_COUNT] = {0};
    const float fundamental     = M_SQRT2 * p_sample->grid_rms_v;
    float curvature             = fundamental;
    float peak                  = 0.0f;
    const float step            = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float order = (float)(2u * harmonic + 3u);
        const float cosine = cosf(order * p_sample->theta_rad);
        const float sine   = sinf(order * p_sample->theta_rad);
        const float alpha  = p_sample->harmonic_feedback_weight * p_sample->grid_harmonic_alpha_v[harmonic];
        const float beta   = p_sample->harmonic_feedback_weight * p_sample->grid_harmonic_beta_v[harmonic];
        d[harmonic] = alpha * cosine + beta * sine;
        q[harmonic] = -alpha * sine + beta * cosine;
        curvature += order * order * hypotf(alpha, beta);
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        float voltage = fundamental * phase_cosine[point];

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            voltage += d[harmonic] * harmonic_cosine[harmonic][point]
                     - q[harmonic] * harmonic_sine[harmonic][point];
        }
        peak = fmaxf(peak, fabsf(voltage));
    }
    return peak + curvature * step * step * 0.125f;
}

/** @brief 固定有功投影后，求完整波形允许的正交电压区间，保持各桥正交分量同向。 */
static bool FUNC_RAM voltage_orth_range(float  parallel,
                                        float  unit_d,
                                        float  unit_q,
                                        float  bus_v,
                                        float  reserve,
                                        float  total_orthogonal,
                                        float *p_lower,
                                        float *p_upper)
{
    const float bound = 2.0f * cfg.modulation_limit * bus_v;
    const float limit = cfg.modulation_limit * bus_v - reserve - voltage_wave_guard(bus_v);
    *p_lower = (total_orthogonal >= 0.0f) ? 0.0f : -bound;
    *p_upper = (total_orthogonal >= 0.0f) ? bound : 0.0f;

    if (fabsf(parallel) > bound)
    {
        return false;
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float along = unit_d * phase_cosine[point] - unit_q * phase_sine[point];
        const float across = -unit_q * phase_cosine[point] - unit_d * phase_sine[point];
        const float offset = parallel * along + bus_v * inter.harmonic_wave_pu[point];

        if (fabsf(across) < 1.0e-6f)
        {
            if (fabsf(offset) > limit)
            {
                return false;
            }
        }
        else
        {
            const float first = (-limit - offset) / across;
            const float second = (limit - offset) / across;
            *p_lower = fmaxf(*p_lower, fminf(first, second));
            *p_upper = fminf(*p_upper, fmaxf(first, second));

            if (*p_lower > *p_upper)
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 检查负 q 候选电流下，各桥的功率投影和完整周期电压容量。
 * @param p_input 本拍无功规划输入；地址仅在同步求解期间使用。
 * @param q_abs 无功参考绝对值，A 峰值；实际采用负 q。
 * @return true：候选点在预留电压余量后的可行域内。
 */
static bool FUNC_RAM q_candidate_feasible(const chb_q_plan_input_t *p_input, float q_abs)
{
    const float id_ref = *p_input->p_id_ref_a;                /* 有功电流峰值，A。 */
    const float grid_peak = M_SQRT2 * *p_input->p_grid_rms_v; /* 电网基波峰值，V。 */
    float magnitude = hypotf(id_ref, q_abs);                  /* 候选电流峰值，A。 */
    float divisor   = fmaxf(magnitude, 1.0e-6f);              /* 零电流点只用于可行性判断。 */
    float parallel_sum = grid_peak * id_ref / divisor - cfg.r_grid_ohm * magnitude;
    float orthogonal_sum = grid_peak * q_abs / divisor - *p_input->p_grid_omega * cfg.l_grid_h * magnitude;
    const float equal_orthogonal = orthogonal_sum / (float)CHB_CELL_COUNT; /* 等无功对应的每桥正交电压。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float parallel = parallel_sum / (float)CHB_CELL_COUNT + 2.0f * p_input->p_delta_power_w[cell] / divisor;
        float lower = 0.0f;
        float upper = 0.0f;

        if (!voltage_orth_range(parallel,
                                id_ref / divisor,
                                -q_abs / divisor,
                                p_input->p_bus_filtered_v[cell],
                                CHB_PF_VOLTAGE_RESERVE_V,
                                orthogonal_sum,
                                &lower,
                                &upper))
        {
            return false;
        }

        if (    (equal_orthogonal < lower)
             || (equal_orthogonal > upper))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 在电流额定圆与各级周期电压容量内求最小负 q 给定。
 * @param p_input 本拍已完成采样、外环和谐波电压预算的必要输入。
 * @param p_inter 仅由 100 us 控制中断写入的搜索及斜坡状态。
 * @param p_output 本拍唯一 q 轴电流参考输出。
 */
static void FUNC_RAM chb_q_plan_run(const chb_q_plan_input_t *p_input,
                                    chb_q_plan_inter_t       *p_inter,
                                    chb_q_plan_output_t      *p_output)
{
    const float id_ref = *p_input->p_id_ref_a; /* 有功电流峰值，A。 */
    const float q_max = sqrtf(fmaxf(0.0f,
                                    cfg.current_limit_pk_a * cfg.current_limit_pk_a - id_ref * id_ref));
    const float q_floor = sqrtf(fmaxf(0.0f,
                                      CHB_BALANCE_CURRENT_MIN_PK_A * CHB_BALANCE_CURRENT_MIN_PK_A
                                          - id_ref * id_ref));

    if (p_inter->ticks == 0u)
    {
        p_inter->zero_feasible = q_candidate_feasible(p_input, 0.0f);
        p_inter->target_a      = q_floor;

        if (!q_candidate_feasible(p_input, q_floor))
        {
            float lower       = q_floor; /* 已知不可行的无功幅值下界，A。 */
            p_inter->target_a = q_max;

            for (uint32_t scan = 1u; scan <= CHB_PF_SCAN_STEPS; ++scan)
            {
                float upper = q_floor + (q_max - q_floor) * (float)scan / (float)CHB_PF_SCAN_STEPS;

                if (q_candidate_feasible(p_input, upper))
                {
                    for (uint32_t iteration = 0u; iteration < CHB_PF_REFINE_STEPS; ++iteration)
                    {
                        float middle = 0.5f * (lower + upper); /* 二分候选无功幅值。 */

                        if (q_candidate_feasible(p_input, middle))
                        {
                            upper = middle;
                        }
                        else
                        {
                            lower = middle;
                        }
                    }
                    p_inter->target_a = upper;
                    break;
                }
                lower = upper;
            }
        }
    }
    p_inter->ticks = (p_inter->ticks + 1u) % CHB_PF_PLAN_TICKS;
    p_inter->magnitude_ref_a += limit_float(p_inter->target_a - p_inter->magnitude_ref_a,
                                            -CHB_PF_Q_SLEW_A_PER_S * cfg.ts,
                                            CHB_PF_Q_SLEW_A_PER_S * cfg.ts);
    p_output->ref = -fminf(q_max, fmaxf(q_floor, p_inter->magnitude_ref_a));
}

/** @brief 单次控制拍中各环节共享的临时量。 */
typedef struct
{
    chb_pwm_plan_t command;       /* 作用时刻预测前的三桥电压及母线快照。 */
    float          bus_sum;       /* 三路实时母线电压之和，V。 */
    float          id_raw;        /* 限幅前有功电流给定，A 峰值。 */
    float          id_ref;        /* 限幅后有功电流给定，A 峰值。 */
    float          iq_ref;        /* 差模分配所需的无功电流，A 峰值。 */
    float          balance_scale; /* 三路差模电压共用的限幅系数。 */
    float          id;            /* 公共电流 d 轴反馈，A 峰值。 */
    float          iq;            /* 公共电流 q 轴反馈，A 峰值。 */
    float          id_error;      /* d 轴电流误差，A。 */
    float          iq_error;      /* q 轴电流误差，A。 */
    float          cosine;        /* 电网相角余弦。 */
    float          sine;          /* 电网相角正弦。 */
    float          v_bridge_d;    /* 级联桥 d 轴总端口电压，V。 */
    float          v_bridge_q;    /* 级联桥 q 轴总端口电压，V。 */
    float          power_request[CHB_CELL_COUNT];     /* 各桥选择器输出，W。 */
    float          energy_error[CHB_CELL_COUNT];      /* 电容能量误差，J。 */
    float          voltage_power[CHB_CELL_COUNT];     /* 未限幅的能量环功率，W。 */
    float          power_error[CHB_CELL_COUNT];       /* 负载功率上限减反馈，W。 */
    float          power_ceiling_raw[CHB_CELL_COUNT]; /* 未限幅的功率环上界，W。 */
    float          power_ceiling[CHB_CELL_COUNT];     /* 功率环上界，W。 */
    float          power_capacity[CHB_CELL_COUNT];    /* 电流及调制能力允许的桥侧功率幅值，W。 */
    float          delta_power[CHB_CELL_COUNT];       /* 各桥去除共模后的功率请求，W。 */
    float          base_parallel[CHB_CELL_COUNT];     /* 暂态限幅的可行锚点，沿电流投影，V。 */
    float          base_orthogonal[CHB_CELL_COUNT];   /* 可行锚点的正交投影，V。 */
    float          power_voltage[CHB_CELL_COUNT];     /* 功率校正对应的平行电压，V。 */
    float          balance_utilization;               /* 本拍分配容量利用率。 */
    float          current_magnitude;                 /* 参考电流矢量模长，A。 */
    float          current_unit_d;    /* 沿电流方向的 d 轴单位分量。 */
    float          current_unit_q;    /* 沿电流方向的 q 轴单位分量。 */
    bool           total_limited;     /* 总桥矢量触及母线幅值界限。 */
    bool           cell_limited;      /* 差模校正触及至少一桥的电压幅值界限。 */
    float          current_voltage_d; /* 电流环当拍 d 轴共模请求，V。 */
    float          current_voltage_q; /* 电流环当拍 q 轴共模请求，V。 */
} chb_ctrl_step_t;

/**
 * @brief 整理本拍相角、母线电压和负载功率反馈，供后续控制环节使用。
 * @details 各桥反馈先抑制单相功率造成的二倍频脉动，再低通估计慢变量；
 *          实时母线电压单独求和，供本拍调制容量计算使用。
 * @param p_step 本拍控制临时量，写入相角三角函数和实时母线电压和。
 * @param p_sample 已完成应用保护检查的同拍采样快照。
 */
static void FUNC_RAM chb_ctrl_observe(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    /* 电压控制采用本拍电网相角；电流反馈另按已知采样延迟旋转到对应时刻。 */
    p_step->cosine          = cosf(p_sample->theta_rad);
    p_step->sine            = sinf(p_sample->theta_rad);
    inter.grid_omega        = M_2PI * p_sample->grid_hz;
    inter.current_phase_cos = cosf(inter.grid_omega * cfg.current_sample_delay_s);
    inter.current_phase_sin = sinf(inter.grid_omega * cfg.current_sample_delay_s);

    /* 三桥各自保留陷波和低通状态，避免某一级的母线或负载扰动串入其他级。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 电压和功率陷波器均跟踪 2 倍电网频率，抑制单相二倍频纹波。 */
        inter.bus_notch_input_v[cell] = p_sample->bus_v[cell];
        notch_update_freq(&inter.bus_notch[cell], 2.0f * inter.grid_omega);
        notch_update_freq(&inter.power_notch[cell], 2.0f * inter.grid_omega);
        notch_cal(&inter.bus_notch[cell]);
        /* 陷波后的电压再低通，作为各桥能量环和功率容量的平滑反馈。 */
        inter.bus_filtered_v[cell] += inter.bus_filter_weight
                                    * (inter.bus_notch[cell].output.val - inter.bus_filtered_v[cell]);
        /* 同拍电压与负载电流相乘得到瞬时负载功率，再提取其慢变化部分。 */
        inter.power_notch_input_w[cell] = p_sample->bus_v[cell] * p_sample->load_i_a[cell];
        notch_cal(&inter.power_notch[cell]);
        inter.load_power_w[cell] += inter.bus_filter_weight
                                  * (inter.power_notch[cell].output.val - inter.load_power_w[cell]);
        /* 总桥电压约束使用实时母线电压，不使用已滤波的能量环反馈。 */
        p_step->bus_sum += p_sample->bus_v[cell];
    }
}

static void FUNC_RAM chb_ctrl_harmonic_compensation(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    {
        float harmonic_peak_pu = 0.0f; /* 含相位的谐波合成峰值上界，每伏母线。 */
        const float phase_step = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;
        const float interpolation_weight = phase_step * phase_step * 0.125f;
        const float feedforward_peak = grid_feedforward_peak(p_sample); /* 完整电网前馈周期峰值，V。 */
        float feedback_peak          = 0.0f; /* 电流反馈附加电压预算，V。 */

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            const float angle = (float)(2u * harmonic + 3u) * inter.grid_omega
                              * cfg.current_sample_delay_s; /* 恢复到电压采样时刻。 */
            const float cos_delay = cosf(angle);            /* 该次谐波的采样相位余弦。 */
            const float sin_delay = sinf(angle);            /* 该次谐波的采样相位正弦。 */
            const float current_alpha = p_sample->current_harmonic_alpha_a[harmonic] * cos_delay
                                      - p_sample->current_harmonic_beta_a[harmonic] * sin_delay;
            const float current_beta = p_sample->current_harmonic_alpha_a[harmonic] * sin_delay
                                     + p_sample->current_harmonic_beta_a[harmonic] * cos_delay;
            /* 整流方向下，提高同相桥电压使对应谐波电流下降。 */
            p_step->command.harmonic_alpha_v[harmonic] = CHB_HARMONIC_CURRENT_GAIN * current_alpha;
            p_step->command.harmonic_beta_v[harmonic]  = CHB_HARMONIC_CURRENT_GAIN * current_beta;
            feedback_peak += hypotf(p_step->command.harmonic_alpha_v[harmonic],
                                    p_step->command.harmonic_beta_v[harmonic]);
        }
        /* 启机低母线时优先建立基波电压，附加反馈只能使用其余容量。 */
        inter.harmonic_feedback_scale = p_sample->harmonic_feedback_weight;

        if (feedback_peak > 0.0f)
        {
            const float available = fmaxf(0.0f,
                                          cfg.modulation_limit * p_step->bus_sum
                                              - feedforward_peak);
            inter.harmonic_feedback_scale = fminf(p_sample->harmonic_feedback_weight, available / feedback_peak);
        }

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            p_step->command.harmonic_alpha_v[harmonic] = p_sample->harmonic_feedback_weight
                                                           * p_sample->grid_harmonic_alpha_v[harmonic]
                                                       + inter.harmonic_feedback_scale
                                                             * p_step->command.harmonic_alpha_v[harmonic];
            p_step->command.harmonic_beta_v[harmonic] = p_sample->harmonic_feedback_weight
                                                          * p_sample->grid_harmonic_beta_v[harmonic]
                                                      + inter.harmonic_feedback_scale
                                                            * p_step->command.harmonic_beta_v[harmonic];
        }
        voltage_wave_prepare(&p_step->command, p_sample->theta_rad, p_step->bus_sum);

        for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
        {
            harmonic_peak_pu = fmaxf(harmonic_peak_pu, fabsf(inter.harmonic_wave_pu[point]));
        }
        harmonic_peak_pu += inter.harmonic_curvature_pu * interpolation_weight;
        /* 零基波须为可行锚点；其余容量由完整波形约束分配，不按谐波幅值之和截断。 */
        inter.harmonic_scale = 1.0f;

        if (harmonic_peak_pu > 0.0f)
        {
            const float available_pu = cfg.modulation_limit * (1.0f - 3.0f * interpolation_weight);
            inter.harmonic_scale = fminf(1.0f, available_pu / harmonic_peak_pu);
        }
        inter.harmonic_reserve_pu = inter.harmonic_scale * harmonic_peak_pu;

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            p_step->command.harmonic_alpha_v[harmonic] *= inter.harmonic_scale;
            p_step->command.harmonic_beta_v[harmonic] *= inter.harmonic_scale;
        }

        for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
        {
            inter.harmonic_wave_pu[point] *= inter.harmonic_scale;
        }
        inter.harmonic_curvature_pu *= inter.harmonic_scale;
    }
}

static void FUNC_RAM chb_ctrl_power_reference(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    float power_sum = 0.0f; /* 各桥功率请求之和，W。 */
    {
        float ramp_step_v  = cfg.bus_ref_ramp_v_per_s * cfg.ts; /* 每控制拍最大给定增量。 */
        float ramp_error_v = cfg.bus_ref_v - inter.bus_ref_ramped_v;
        inter.bus_ref_ramped_v += limit_float(ramp_error_v, -ramp_step_v, ramp_step_v);
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float power_limit = cfg.output_power_limit_w; /* 所有桥共用的单级持续功率上限。 */
        p_step->power_capacity[cell] = 0.5f * cfg.modulation_limit * inter.bus_filtered_v[cell]
                                     * cfg.current_limit_pk_a;
        p_step->energy_error[cell] = 0.5f * cfg.bus_capacitance_f
                                   * (inter.bus_ref_ramped_v * inter.bus_ref_ramped_v
                                      - inter.bus_filtered_v[cell] * inter.bus_filtered_v[cell]);
        p_step->voltage_power[cell] = inter.load_power_w[cell] + cfg.energy_kp * p_step->energy_error[cell]
                                    + inter.energy_integral_w[cell];
        p_step->power_error[cell] = power_limit - inter.load_power_w[cell];
        p_step->power_ceiling_raw[cell] = power_limit + cfg.power_limit_kp * p_step->power_error[cell]
                                        + inter.power_integral_w[cell];
        /* 输出负载上限由反馈环实现，桥侧请求还须承担充电及分配误差补偿。 */
        p_step->power_ceiling[cell] = limit_float(p_step->power_ceiling_raw[cell],
                                                  -p_step->power_capacity[cell],
                                                  p_step->power_capacity[cell]);
        p_step->power_request[cell] = limit_float(fminf(p_step->voltage_power[cell], p_step->power_ceiling[cell]),
                                                  -p_step->power_capacity[cell],
                                                  p_step->power_capacity[cell]);
        power_sum += p_step->power_request[cell];
    }
    p_step->id_raw = 2.0f * power_sum / (M_SQRT2 * p_sample->grid_rms_v);
    p_step->id_ref = limit_float(p_step->id_raw, -cfg.current_limit_pk_a, cfg.current_limit_pk_a);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float current_scale = (fabsf(p_step->id_raw) > cfg.current_limit_pk_a) ? p_step->id_ref / p_step->id_raw : 1.0f;
        p_step->delta_power[cell] = current_scale * (p_step->power_request[cell] - power_sum / (float)CHB_CELL_COUNT);
    }
}

static void FUNC_RAM chb_ctrl_reactive_reference(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    float current_ref_squared = 0.0f; /* 电流参考矢量模方，A^2。 */
    {
        const chb_q_plan_input_t input = {
            /* 每个指针均指向本拍有效且必要的规划量。 */
            .p_id_ref_a       = &p_step->id_ref,
            .p_grid_rms_v     = &p_sample->grid_rms_v,
            .p_delta_power_w  = p_step->delta_power,
            .p_bus_filtered_v = inter.bus_filtered_v,
            .p_grid_omega     = &inter.grid_omega,
        };
        chb_q_plan_output_t output = {0}; /* q 轴规划的单一结果。 */

        chb_q_plan_run(&input, &inter.q_plan, &output);
        p_step->iq_ref = output.ref;
    }
    current_ref_squared = p_step->id_ref * p_step->id_ref + p_step->iq_ref * p_step->iq_ref;
    p_step->current_magnitude = sqrtf(current_ref_squared);
    p_step->current_unit_d    = p_step->id_ref / p_step->current_magnitude;
    p_step->current_unit_q    = p_step->iq_ref / p_step->current_magnitude;
}

static void FUNC_RAM chb_ctrl_current_loop(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    {
        float current_cosine = p_step->cosine * inter.current_phase_cos + p_step->sine * inter.current_phase_sin;
        float current_sine = p_step->sine * inter.current_phase_cos - p_step->cosine * inter.current_phase_sin;
        /* 旋转相角回到电流采样时刻，把延迟反馈还原为本拍工频矢量。 */
        p_step->id = p_sample->i_alpha_a * current_cosine + p_sample->i_beta_a * current_sine;
        p_step->iq = -p_sample->i_alpha_a * current_sine + p_sample->i_beta_a * current_cosine;
    }
    p_step->id_error = p_step->id_ref - p_step->id;
    p_step->iq_error = p_step->iq_ref - p_step->iq;
    /* 原始电压减去已单独前馈的谐波，保留 MSOGI 的变频残差，避免 PLL 滞后成为电压扰动。 */
    float grid_base_alpha = p_sample->grid_v;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        grid_base_alpha -= p_sample->harmonic_feedback_weight * p_sample->grid_harmonic_alpha_v[harmonic];
    }
    const float grid_d = grid_base_alpha * p_step->cosine + p_sample->grid_fundamental_beta_v * p_step->sine;
    const float grid_q = -grid_base_alpha * p_step->sine + p_sample->grid_fundamental_beta_v * p_step->cosine;
    p_step->v_bridge_d = grid_d - cfg.r_grid_ohm * p_step->id
                       + inter.grid_omega * cfg.l_grid_h * p_step->iq
                       - cfg.current_kp * p_step->id_error - inter.current_integral_d_v;
    p_step->v_bridge_q = grid_q - cfg.r_grid_ohm * p_step->iq - inter.grid_omega * cfg.l_grid_h * p_step->id
                       - cfg.current_kp * p_step->iq_error
                       - inter.current_integral_q_v;

    p_step->current_voltage_d = p_step->v_bridge_d; /* 电流环的当拍共模请求，不等同于稳态正弦幅值。 */
    p_step->current_voltage_q = p_step->v_bridge_q;
}

static void FUNC_RAM chb_ctrl_total_voltage_limit(chb_ctrl_step_t *p_step)
{
    {
        const float limit = cfg.modulation_limit * p_step->bus_sum - voltage_wave_guard(p_step->bus_sum);
        const float magnitude = hypotf(p_step->v_bridge_d, p_step->v_bridge_q);
        float scale = (magnitude > 0.0f) ? fminf(1.0f, 2.0f * cfg.modulation_limit * p_step->bus_sum / magnitude)
                                         : 1.0f;

        for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
        {
            const float fundamental = p_step->v_bridge_d * phase_cosine[point] - p_step->v_bridge_q * phase_sine[point];
            const float harmonic = p_step->bus_sum * inter.harmonic_wave_pu[point];

            if (fabsf(fundamental) > 1.0e-6f)
            {
                const float allowance = limit - ((fundamental > 0.0f) ? harmonic : -harmonic);
                scale = fminf(scale, fmaxf(0.0f, allowance / fabsf(fundamental)));
            }
        }
        p_step->v_bridge_d *= scale;
        p_step->v_bridge_q *= scale;
        p_step->total_limited = scale < 0.99999f;
    }
}

static void FUNC_RAM chb_ctrl_balance_capacity(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float capacity_fraction = p_sample->bus_v[cell] / p_step->bus_sum;
        float base_d_v          = capacity_fraction * p_step->v_bridge_d; /* 母线不等时也可行的限幅锚点。 */
        float base_q_v          = capacity_fraction * p_step->v_bridge_q;

        p_step->base_parallel[cell] = base_d_v * p_step->current_unit_d + base_q_v * p_step->current_unit_q;
        p_step->base_orthogonal[cell] = -base_d_v * p_step->current_unit_q + base_q_v * p_step->current_unit_d;
        /* 单相平均功率为 0.5 * (vd * id + vq * iq)。 */
        p_step->power_voltage[cell] = (p_step->v_bridge_d * p_step->current_unit_d
                                       + p_step->v_bridge_q * p_step->current_unit_q)
                                        / (float)CHB_CELL_COUNT
                                    + 2.0f * p_step->delta_power[cell] / p_step->current_magnitude
                                    - p_step->base_parallel[cell];
    }
    {
        float scale_lower = 0.0f;
        float scale_upper = 1.0f;

        /* 平行分量决定有功；正交分量在各桥间零和流转，不改变任何一级有功。 */

        for (uint32_t iteration = 0u; iteration < 24u; ++iteration)
        {
            float candidate = (iteration == 0u) ? 1.0f
                                                : 0.5f * (scale_lower + scale_upper);
            float lower_sum = 0.0f;
            float upper_sum = 0.0f;
            bool feasible   = true;

            for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
            {
                float parallel = p_step->base_parallel[cell] + candidate * p_step->power_voltage[cell];
                float lower = 0.0f;
                float upper = 0.0f;
                const float total_orthogonal = -p_step->v_bridge_d * p_step->current_unit_q
                                             + p_step->v_bridge_q * p_step->current_unit_d;

                if (!voltage_orth_range(parallel,
                                        p_step->current_unit_d,
                                        p_step->current_unit_q,
                                        p_sample->bus_v[cell],
                                        0.0f,
                                        total_orthogonal,
                                        &lower,
                                        &upper))
                {
                    feasible = false;
                    break;
                }
                lower_sum += lower - p_step->base_orthogonal[cell];
                upper_sum += upper - p_step->base_orthogonal[cell];
            }

            if (    feasible
                 && (lower_sum <= 0.0f)
                 && (upper_sum >= 0.0f))
            {
                scale_lower = candidate;

                if (iteration == 0u)
                {
                    break;
                }
            }
            else
            {
                scale_upper = candidate;
            }
        }
        p_step->balance_scale = scale_lower;
    }
}

static void FUNC_RAM chb_ctrl_cell_voltage_allocation(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    float orth_lower[CHB_CELL_COUNT] = {0}; /* 完整波形允许的正交分量下界。 */
    float orth_upper[CHB_CELL_COUNT] = {0}; /* 完整波形允许的正交分量上界。 */
    {
        float lower_sum = 0.0f;
        float upper_sum = 0.0f;
        float total_orthogonal = -p_step->v_bridge_d * p_step->current_unit_q
                               + p_step->v_bridge_q * p_step->current_unit_d;
        const float equal_orthogonal = total_orthogonal / (float)CHB_CELL_COUNT; /* 等无功的每桥正交电压。 */
        float orth_fraction          = 0.0f;
        bool equal_q_feasible        = true; /* 完整波形约束下是否可由每桥平均承担基波无功。 */

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            float parallel = p_step->base_parallel[cell] + p_step->balance_scale * p_step->power_voltage[cell];

            if (!voltage_orth_range(parallel,
                                    p_step->current_unit_d,
                                    p_step->current_unit_q,
                                    p_sample->bus_v[cell],
                                    0.0f,
                                    total_orthogonal,
                                    &orth_lower[cell],
                                    &orth_upper[cell]))
            {
                equal_q_feasible = false;
            }
            lower_sum += orth_lower[cell];
            upper_sum += orth_upper[cell];

            if (    (equal_orthogonal < orth_lower[cell])  /* 某桥所需无功低于电压可行区间。 */
                 || (equal_orthogonal > orth_upper[cell])) /* 某桥所需无功高于电压可行区间。 */
            {
                equal_q_feasible = false;
            }
        }
        /* 等正交电压即等无功；受各桥电压容量限制时退回可行区间分配。 */

        if (upper_sum > lower_sum)
        {
            orth_fraction = limit_float((total_orthogonal - lower_sum) / (upper_sum - lower_sum), 0.0f, 1.0f);
        }

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            float parallel = p_step->base_parallel[cell] + p_step->balance_scale * p_step->power_voltage[cell];
            float orthogonal = orth_lower[cell] + orth_fraction * (orth_upper[cell] - orth_lower[cell]);

            if (equal_q_feasible)
            {
                orthogonal = equal_orthogonal;
            }

            if (orth_upper[cell] > orth_lower[cell])
            {
                p_step->balance_utilization = fmaxf(
                    p_step->balance_utilization,
                    fabsf((orthogonal - orth_lower[cell]) / (orth_upper[cell] - orth_lower[cell])));
            }

            p_step->command.cell_d_v[cell] = parallel * p_step->current_unit_d - orthogonal * p_step->current_unit_q;
            p_step->command.cell_q_v[cell] = parallel * p_step->current_unit_q + orthogonal * p_step->current_unit_d;
        }
    }
    p_step->cell_limited = p_step->balance_scale < 0.99999f;

    /* 周期可行域约束均衡分配；剩余共模电流调节按母线容量分摊，由实际 PWM 电压限幅。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float share   = p_sample->bus_v[cell] / p_step->bus_sum;
        const float delta_d = p_step->current_voltage_d - p_step->v_bridge_d;
        const float delta_q = p_step->current_voltage_q - p_step->v_bridge_q;
        const float delta_parallel = delta_d * p_step->current_unit_d + delta_q * p_step->current_unit_q;
        const float delta_orthogonal = -delta_d * p_step->current_unit_q + delta_q * p_step->current_unit_d;
        /* 电流调节余量的无功分量平均给各桥，有功分量沿母线容量分配。 */
        p_step->command.cell_d_v[cell] += share * delta_parallel * p_step->current_unit_d
                                        - delta_orthogonal * p_step->current_unit_q / (float)CHB_CELL_COUNT;
        p_step->command.cell_q_v[cell] += share * delta_parallel * p_step->current_unit_q
                                        + delta_orthogonal * p_step->current_unit_d / (float)CHB_CELL_COUNT;
        /* 保留差模有功请求，逐桥波形容量由发波时的三桥联合投影约束。 */
        p_step->command.cell_d_v[cell] += (1.0f - p_step->balance_scale) * p_step->power_voltage[cell]
                                        * p_step->current_unit_d;
        p_step->command.cell_q_v[cell] += (1.0f - p_step->balance_scale) * p_step->power_voltage[cell]
                                        * p_step->current_unit_q;
    }
    p_step->v_bridge_d = p_step->current_voltage_d;
    p_step->v_bridge_q = p_step->current_voltage_q;
}

/** @brief 在同一作用时刻联合约束三桥电压，并按剩余调制裕量修正串联电压和。 */
static void FUNC_RAM chb_ctrl_project_pwm_voltage(const chb_pwm_plan_t *p_plan,
                                                  float                 phase_advance,
                                                  float                 p_voltage[CHB_CELL_COUNT])
{
    const float theta = p_plan->theta_rad + phase_advance;
    float harmonic    = 0.0f;
    float requested   = 0.0f;
    float applied     = 0.0f;
    float capacity    = 0.0f;
    float headroom    = 0.0f;
    float bus_sum     = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        bus_sum += p_plan->bus_v[cell];
    }

    for (uint32_t harmonic_index = 0u; harmonic_index < CHB_HARMONIC_COUNT; ++harmonic_index)
    {
        const float angle = (float)(2u * harmonic_index + 3u) * phase_advance;
        harmonic += p_plan->harmonic_alpha_v[harmonic_index] * cosf(angle)
                  - p_plan->harmonic_beta_v[harmonic_index] * sinf(angle);
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float bound = cfg.modulation_limit * p_plan->bus_v[cell];
        const float voltage = p_plan->cell_d_v[cell] * cosf(theta)
                            - p_plan->cell_q_v[cell] * sinf(theta)
                            + p_plan->bus_v[cell] * harmonic / bus_sum;
        requested += voltage;
        capacity += bound;
        p_voltage[cell] = fminf(bound, fmaxf(-bound, voltage));
        applied += p_voltage[cell];
    }

    const float residual = fminf(capacity, fmaxf(-capacity, requested)) - applied;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        headroom += cfg.modulation_limit * p_plan->bus_v[cell]
                  - ((residual >= 0.0f) ? p_voltage[cell] : -p_voltage[cell]);
    }

    if (headroom > 0.0f)
    {
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            const float room = cfg.modulation_limit * p_plan->bus_v[cell]
                             - ((residual >= 0.0f) ? p_voltage[cell] : -p_voltage[cell]);
            p_voltage[cell] += residual * room / headroom;
        }
    }
}

/** @brief 生成包含作用时刻预测、联合限幅和死区补偿的最终逐级发波电压。 */
static void FUNC_RAM chb_ctrl_prepare_pwm_voltage(const chb_pwm_plan_t   *p_plan,
                                                  float                   p_v_pwm_v[CHB_CELL_COUNT],
                                                  chb_pwm_deadtime_flag_t p_deadtime_flag[CHB_CELL_COUNT])
{
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float delay_periods = CHB_PWM_BASE_DELAY_PERIODS
                                  + (float)cell / (2.0f * (float)CHB_CELL_COUNT);
        const float phase_advance = M_2PI * p_plan->grid_hz * cfg.ts * delay_periods;
        float projected_voltage[CHB_CELL_COUNT] = {0.0f};
        const float predicted_current = p_plan->i_comp_ref_a * cosf(phase_advance)
                                      - p_plan->i_comp_beta_ref_a * sinf(phase_advance);

        chb_ctrl_project_pwm_voltage(p_plan, phase_advance, projected_voltage);
        p_v_pwm_v[cell] = projected_voltage[cell];

        if (predicted_current > 0.0f)
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_POSITIVE;
        }
        else if (predicted_current < 0.0f)
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_NEGATIVE;
        }
        else
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_OFF;
        }
    }
}

static void FUNC_RAM chb_ctrl_publish_pwm(chb_ctrl_step_t *p_step, const chb_observer_sample_t *p_sample)
{
    float v_pwm_v[CHB_CELL_COUNT]                         = {0.0f};
    chb_pwm_deadtime_flag_t deadtime_flag[CHB_CELL_COUNT] = {CHB_PWM_DEADTIME_OFF};
    float total_v_pwm                                     = 0.0f;
    p_step->command.i_comp_ref_a = p_step->id_ref * p_step->cosine - p_step->iq_ref * p_step->sine;
    p_step->command.i_comp_beta_ref_a = p_step->id_ref * p_step->sine + p_step->iq_ref * p_step->cosine;
    p_step->command.theta_rad = p_sample->theta_rad;
    p_step->command.grid_hz   = p_sample->grid_hz;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        p_step->command.bus_v[cell] = p_sample->bus_v[cell];
    }
    chb_ctrl_prepare_pwm_voltage(&p_step->command, v_pwm_v, deadtime_flag);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        total_v_pwm += v_pwm_v[cell];
    }
#if defined(PLATFORM_PLECS)
    float filtered_sum = 0.0f; /* 诊断用三桥滤波电压和，V。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        filtered_sum += inter.bus_filtered_v[cell];
    }
    diag.bus_sum_v          = p_step->bus_sum;
    diag.bus_filtered_sum_v = filtered_sum;
    diag.bus_error_v = (float)CHB_CELL_COUNT * inter.bus_ref_ramped_v - filtered_sum;
    diag.bus_ref_ramped_v    = inter.bus_ref_ramped_v;
    diag.id_ref_a            = p_step->id_ref;
    diag.iq_ref_a            = p_step->iq_ref;
    diag.balance_scale       = p_step->balance_scale;
    diag.balance_utilization = p_step->balance_utilization;
    diag.q_min_a             = inter.q_plan.target_a;
    diag.pf_zero_feasible    = inter.q_plan.zero_feasible ? 1u : 0u;
    diag.id_a                = p_step->id;
    diag.iq_a                = p_step->iq;
    diag.vd_pwm_v            = p_step->v_bridge_d;
    diag.vq_pwm_v            = p_step->v_bridge_q;
    diag.vpwm_v              = total_v_pwm;
    diag.total_limited       = p_step->total_limited ? 1u : 0u;
    diag.cell_limited        = p_step->cell_limited ? 1u : 0u;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        diag.balance_error_v[cell] = filtered_sum / (float)CHB_CELL_COUNT - inter.bus_filtered_v[cell];
        diag.power_request_w[cell] = p_step->power_request[cell];
        diag.power_limited[cell] = (p_step->voltage_power[cell] > p_step->power_ceiling[cell]) ? 1u : 0u;
    }
#endif

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        chb_hal_get_ctrl()->p_set_pwm_func[cell](v_pwm_v[cell],
                                                 p_step->command.bus_v[cell],
                                                 deadtime_flag[cell]);
    }
}

static void FUNC_RAM chb_ctrl_update_integrators(chb_ctrl_step_t *p_step)
{
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float achievable_power = 0.5f
                               * (p_step->command.cell_d_v[cell] * p_step->id_ref
                                  + p_step->command.cell_q_v[cell] * p_step->iq_ref);
        float selector_gap = p_step->voltage_power[cell] - p_step->power_request[cell];
        float actuator_gap = p_step->power_request[cell] - achievable_power;
        bool selector_allows = (fabsf(selector_gap) <= 1.0f)
                            || (selector_gap * p_step->energy_error[cell] < 0.0f);
        bool actuator_allows = !(    p_step->total_limited
                                  || p_step->cell_limited
                                  || (fabsf(p_step->id_raw) > cfg.current_limit_pk_a))
                            || (fabsf(actuator_gap) <= 1.0f)
                            || (actuator_gap * p_step->energy_error[cell] < 0.0f);
        /* 选择器及后级调制/电流约束均参与能量积分抗饱和。 */

        if (    selector_allows
             && actuator_allows)
        {
            inter.energy_integral_w[cell] += cfg.energy_ki * cfg.ts * p_step->energy_error[cell];
        }
        /* 未被选择的功率环保持积分，避免轻载时长期积累正功率误差。 */

        if (    (p_step->voltage_power[cell] >= p_step->power_ceiling[cell])
             && (    (    (p_step->power_ceiling_raw[cell] > -p_step->power_capacity[cell])
                       && (p_step->power_ceiling_raw[cell] < p_step->power_capacity[cell]))
                  || (    (p_step->power_ceiling_raw[cell] >= p_step->power_capacity[cell])
                       && (p_step->power_error[cell] < 0.0f))
                  || (    (p_step->power_ceiling_raw[cell] <= -p_step->power_capacity[cell])
                       && (p_step->power_error[cell] > 0.0f))))
        {
            inter.power_integral_w[cell] += cfg.power_limit_ki * cfg.ts * p_step->power_error[cell];
        }
    }

    if (!p_step->total_limited)
    {
        inter.current_integral_d_v = limit_float(inter.current_integral_d_v
                                                     + cfg.current_ki * cfg.ts * p_step->id_error,
                                                 -cfg.current_integral_limit_v,
                                                 cfg.current_integral_limit_v);
        inter.current_integral_q_v = limit_float(inter.current_integral_q_v
                                                     + cfg.current_ki * cfg.ts * p_step->iq_error,
                                                 -cfg.current_integral_limit_v,
                                                 cfg.current_integral_limit_v);
    }
}

/** @brief 按采样、给定、限幅分配、发波、积分更新的顺序运行控制环。 */
static void FUNC_RAM chb_ctrl_run(void)
{
    const chb_observer_sample_t *p_sample = chb_observer_get_sample(); /* 应用保护已检查的同拍量。 */
    chb_ctrl_step_t step                  = {0}; /* 单拍控制环节共享的临时量。 */

    if (!sample_allowed)
    {
        return;
    }
    sample_allowed = false; /* 本拍不能重复积分或发波。 */

    if (    (chb_fsm_run_allowed() == 0u)
         || (chb_cfg_get_run_request() == 0u))
    {
        chb_ctrl_stop();
        return;
    }

    chb_ctrl_observe(&step, p_sample);                 /* 更新相角、母线电压及各桥负载功率反馈。 */
    chb_ctrl_harmonic_compensation(&step, p_sample);   /* 生成谐波补偿电压并预留调制裕量。 */
    chb_ctrl_power_reference(&step, p_sample);         /* 计算各桥功率及 d 轴电流给定。 */
    chb_ctrl_reactive_reference(&step, p_sample);      /* 规划 q 轴电流并确定参考电流方向。 */
    chb_ctrl_current_loop(&step, p_sample);            /* 计算 dq 电流误差及总桥电压请求。 */
    chb_ctrl_total_voltage_limit(&step);               /* 按完整波形限制总桥基波电压。 */
    chb_ctrl_balance_capacity(&step, p_sample);        /* 求差模有功电压的可行缩放系数。 */
    chb_ctrl_cell_voltage_allocation(&step, p_sample); /* 分配各桥电压，并平均承担可行无功。 */
    chb_ctrl_publish_pwm(&step, p_sample);             /* 合成并同步下发三桥 PWM 命令。 */
    chb_ctrl_update_integrators(&step);                /* 按限幅状态更新各控制环积分器。 */
}
REG_INTERRUPT(3, chb_ctrl_run)
