// SPDX-License-Identifier: MIT
/**
 * @file chb_ctrl.c
 * @brief CHB per-cell energy/power loops and minimum-current dq allocation.
 * @details Sampling, protection and PWM execute at priorities 1, 2 and 3 in one
 *          serialized control period. The beta current and grid angle are external
 *          observations; this module owns no SOGI or PLL. Initial gains are from
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
#include "my_math.h"
#include "notch.h"
#include "section.h"
#if defined(PLATFORM_PLECS)
#include "shell.h"
#include <float.h>
#endif

#include <math.h>
#include <stdbool.h>
#include <string.h>

typedef struct chb_ctrl_inter
{
    float    bus_notch_input_v[CHB_CELL_COUNT];   /* 陷波器使用的本拍母线电压副本，V。 */
    notch_t  bus_notch[CHB_CELL_COUNT];           /* 三路独立的二倍工频陷波状态。 */
    float    bus_filtered_v[CHB_CELL_COUNT];      /* 抑制二倍工频纹波后的各级电压，V。 */
    float    power_notch_input_w[CHB_CELL_COUNT]; /* 同拍母线电压乘负载电流，W。 */
    notch_t  power_notch[CHB_CELL_COUNT];         /* 各桥负载功率二倍工频陷波。 */
    float    load_power_w[CHB_CELL_COUNT];        /* 陷波及低通后的负载功率，W。 */
    float    energy_integral_w[CHB_CELL_COUNT];   /* 每桥能量环积分输出，W。 */
    float    power_integral_w[CHB_CELL_COUNT];    /* 每桥功率上限环积分输出，W。 */
    float    q_magnitude_ref_a;    /* 经斜坡的最小无功电流幅值，取负 q 方向，A。 */
    float    q_target_a;           /* 可行域规划所得的最小无功电流幅值，A。 */
    uint32_t pf_plan_ticks;        /* 距上次可行域求解经过的控制拍数。 */
    bool     pf_zero_feasible;     /* 当拍功率请求是否允许零网侧无功。 */
    float    current_phase_cos;    /* 已知采样延迟对应的工频相位余弦。 */
    float    current_phase_sin;    /* 已知采样延迟对应的工频相位正弦。 */
    float    current_integral_d_v; /* d 轴内环积分输出，V。 */
    float    current_integral_q_v; /* q 轴内环积分输出，V。 */
    float    bus_filter_weight;    /* 每控制拍的一阶低通离散权重。 */
    float    bus_ref_ramped_v;     /* 每级母线正在执行的电压给定，V。 */
} chb_ctrl_inter_t;

static chb_ctrl_cfg_t   cfg            = {0};   /* INIT 后只读的控制系数副本。 */
static chb_ctrl_inter_t inter          = {0};   /* 控制阶段唯一写入的积分与滤波动态。 */
static bool             sample_allowed = false; /* 本拍采样已完成且未被应用保护禁止。 */

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
    float   id_a;     /* d 轴电流反馈，A 峰值。 */
    float   iq_a;     /* q 轴电流反馈，A 峰值。 */
    float   vd_pwm_v; /* 总桥 d 轴电压指令，V。 */
    float   vq_pwm_v; /* 总桥 q 轴电压指令，V。 */
    float   vpwm_v;   /* 逆 Park 后的总桥瞬时电压指令，V。 */
    float   balance_error_v[CHB_CELL_COUNT]; /* 各桥相对平均母线的误差，V。 */
    float   power_request_w[CHB_CELL_COUNT]; /* 每桥限功率后的有功请求，W。 */
    uint8_t power_limited[CHB_CELL_COUNT];   /* 功率选择器接管该桥稳压。 */
    uint8_t total_limited; /* 总桥矢量限幅标志。 */
    uint8_t cell_limited;  /* 至少一桥系数限幅标志。 */
} chb_ctrl_diag_t;

static chb_ctrl_diag_t diag = {0}; /* FRAME 只观察副本，不写控制积分状态。 */
static float           fault_i_a;  /* 首次保护闭锁时的电感电流，A。 */
static float           fault_bus_v[CHB_CELL_COUNT]; /* 首次保护闭锁时的各桥母线电压，V。 */
static float           fault_id_ref_a;     /* 首次保护闭锁前一拍的 d 轴电流给定，A。 */
static float           fault_id_a;         /* 首次保护闭锁前一拍的 d 轴电流反馈，A。 */
static float           fault_vpwm_v;       /* 首次保护闭锁前一拍的总调制电压，V。 */
static uint8_t         fault_sample_valid; /* 首次保护采样已经锁存。 */

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
    const chb_hal_sample_t *p_sample = chb_hal_get_sample(); /* 进入 RUN 前的最新快照。 */
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
        const chb_hal_sample_t *p_sample = chb_hal_get_sample(); /* 首次不健康的同拍采样。 */
        fault_i_a                        = p_sample->i_alpha_a;
        fault_id_ref_a                   = diag.id_ref_a;
        fault_id_a                       = diag.id_a;
        fault_vpwm_v                     = diag.vpwm_v;

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
    chb_hal_sample();
    sample_allowed = true;
}
REG_INTERRUPT(1, chb_ctrl_sample)

/**
 * @brief 检查负 q 候选电流下，各桥的功率投影和正交电压容量。
 * @param id_ref 有功参考，A 峰值。
 * @param q_abs 无功参考绝对值，A 峰值；实际采用负 q。
 * @param grid_peak 电网电压峰值，V。
 * @param p_delta_power 零和差模功率请求，W。
 * @return true：候选点在预留电压余量后的可行域内。
 */
static bool FUNC_RAM pf_candidate_feasible(float        id_ref,
                                           float        q_abs,
                                           float        grid_peak,
                                           const float *p_delta_power)
{
    float magnitude = hypotf(id_ref, q_abs);     /* 候选电流峰值，A。 */
    float divisor   = fmaxf(magnitude, 1.0e-6f); /* 零电流点只用于可行性判断。 */
    float parallel_sum = grid_peak * id_ref / divisor - cfg.r_grid_ohm * magnitude;
    float orthogonal_sum = grid_peak * q_abs / divisor - M_2PI * cfg.grid_hz * cfg.l_grid_h * magnitude;
    float capacity_sum = 0.0f; /* 各桥可用正交容量之和，V。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float parallel = parallel_sum / (float)CHB_CELL_COUNT + 2.0f * p_delta_power[cell] / divisor;
        float limit = fmaxf(0.0f,
                            cfg.modulation_limit * inter.bus_filtered_v[cell]
                                - CHB_PF_VOLTAGE_RESERVE_V);
        float remaining = limit * limit - parallel * parallel; /* 正交电压允许幅值的平方。 */

        if (remaining < 0.0f)
        {
            return false;
        }
        capacity_sum += sqrtf(remaining);
    }
    return fabsf(orthogonal_sum) <= capacity_sum;
}

/** @brief 总外环、PF 可行域规划、dq 内环和各桥直接电压分配。 */
static void FUNC_RAM chb_ctrl_run(void)
{
    const chb_hal_sample_t *p_sample        = chb_hal_get_sample(); /* 应用保护已检查的同拍量。 */
    chb_pwm_command_t command               = {0};   /* 回调同步消费的三桥电压及母线快照。 */
    float bus_sum                           = 0.0f;  /* 三路实时母线电压之和，V。 */
    float id_raw                            = 0.0f;  /* 限幅前有功电流给定，A 峰值。 */
    float id_ref                            = 0.0f;  /* 限幅后有功电流给定，A 峰值。 */
    float iq_ref                            = 0.0f;  /* 差模分配所需的无功电流，A 峰值。 */
    float current_ref_squared               = 0.0f;  /* 电流参考矢量模方，A^2。 */
    float balance_scale                     = 1.0f;  /* 三路差模电压共用的限幅系数。 */
    float id                                = 0.0f;  /* 公共电流 d 轴反馈，A 峰值。 */
    float iq                                = 0.0f;  /* 公共电流 q 轴反馈，A 峰值。 */
    float id_error                          = 0.0f;  /* d 轴电流误差，A。 */
    float iq_error                          = 0.0f;  /* q 轴电流误差，A。 */
    float cosine                            = 0.0f;  /* 电网相角余弦。 */
    float sine                              = 0.0f;  /* 电网相角正弦。 */
    float v_bridge_d                        = 0.0f;  /* 级联桥 d 轴总端口电压，V。 */
    float v_bridge_q                        = 0.0f;  /* 级联桥 q 轴总端口电压，V。 */
    float magnitude                         = 0.0f;  /* 级联桥总基波矢量幅值，V。 */
    float magnitude_budget                  = 0.0f;  /* 三路可用调制电压总幅值，V。 */
    float power_request[CHB_CELL_COUNT]     = {0};   /* 各桥选择器输出，W。 */
    float energy_error[CHB_CELL_COUNT]      = {0};   /* 电容能量误差，J。 */
    float voltage_power[CHB_CELL_COUNT]     = {0};   /* 未限幅的能量环功率，W。 */
    float power_error[CHB_CELL_COUNT]       = {0};   /* 负载功率上限减反馈，W。 */
    float power_ceiling_raw[CHB_CELL_COUNT] = {0};   /* 未限幅的功率环上界，W。 */
    float power_ceiling[CHB_CELL_COUNT]     = {0};   /* 功率环上界，W。 */
    float power_capacity[CHB_CELL_COUNT]    = {0};   /* 电流及调制能力允许的桥侧功率幅值，W。 */
    float delta_power[CHB_CELL_COUNT]       = {0};   /* 各桥去除共模后的功率请求，W。 */
    float base_parallel[CHB_CELL_COUNT]     = {0};   /* 暂态限幅的可行锚点，沿电流投影，V。 */
    float base_orthogonal[CHB_CELL_COUNT]   = {0};   /* 可行锚点的正交投影，V。 */
    float power_voltage[CHB_CELL_COUNT]     = {0};   /* 功率校正对应的平行电压，V。 */
    float orth_capacity[CHB_CELL_COUNT]     = {0};   /* 有功分配后各桥可用的正交电压幅值，V。 */
    float balance_utilization               = 0.0f;  /* 本拍分配容量利用率。 */
    float current_magnitude                 = 0.0f;  /* 参考电流矢量模长，A。 */
    float current_unit_d                    = 0.0f;  /* 沿电流方向的 d 轴单位分量。 */
    float current_unit_q                    = 0.0f;  /* 沿电流方向的 q 轴单位分量。 */
    float power_sum                         = 0.0f;  /* 各桥功率请求之和，W。 */
    float total_v_pwm                       = 0.0f;  /* 逆 Park 后的单相总瞬时电压，V。 */
    bool total_limited                      = false; /* 总桥矢量触及母线幅值界限。 */
    bool cell_limited                       = false; /* 差模校正触及至少一桥的电压幅值界限。 */

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

    cosine = cosf(p_sample->theta_rad);
    sine   = sinf(p_sample->theta_rad);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        inter.bus_notch_input_v[cell] = p_sample->bus_v[cell];
        notch_cal(&inter.bus_notch[cell]);
        inter.bus_filtered_v[cell] += inter.bus_filter_weight
                                    * (inter.bus_notch[cell].output.val - inter.bus_filtered_v[cell]);
        inter.power_notch_input_w[cell] = p_sample->bus_v[cell] * p_sample->load_i_a[cell];
        notch_cal(&inter.power_notch[cell]);
        inter.load_power_w[cell] += inter.bus_filter_weight
                                  * (inter.power_notch[cell].output.val - inter.load_power_w[cell]);
        bus_sum += p_sample->bus_v[cell];
    }
    {
        float ramp_step_v  = cfg.bus_ref_ramp_v_per_s * cfg.ts; /* 每控制拍最大给定增量。 */
        float ramp_error_v = cfg.bus_ref_v - inter.bus_ref_ramped_v;
        inter.bus_ref_ramped_v += limit_float(ramp_error_v, -ramp_step_v, ramp_step_v);
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float power_limit = cfg.output_power_limit_w[cell]; /* 每桥独立持续功率上限。 */
        power_capacity[cell] = 0.5f * cfg.modulation_limit * inter.bus_filtered_v[cell]
                             * cfg.current_limit_pk_a;
        energy_error[cell] = 0.5f * cfg.bus_capacitance_f[cell]
                           * (inter.bus_ref_ramped_v * inter.bus_ref_ramped_v
                              - inter.bus_filtered_v[cell] * inter.bus_filtered_v[cell]);
        voltage_power[cell] = inter.load_power_w[cell] + cfg.energy_kp * energy_error[cell]
                            + inter.energy_integral_w[cell];
        power_error[cell] = power_limit - inter.load_power_w[cell];
        power_ceiling_raw[cell] = power_limit + cfg.power_limit_kp * power_error[cell]
                                + inter.power_integral_w[cell];
        /* 输出负载上限由反馈环实现，桥侧请求还须承担充电及分配误差补偿。 */
        power_ceiling[cell] = limit_float(power_ceiling_raw[cell],
                                          -power_capacity[cell],
                                          power_capacity[cell]);
        power_request[cell] = limit_float(fminf(voltage_power[cell], power_ceiling[cell]),
                                          -power_capacity[cell],
                                          power_capacity[cell]);
        power_sum += power_request[cell];
    }
    id_raw = 2.0f * power_sum / (M_SQRT2 * p_sample->grid_rms_v);
    id_ref = limit_float(id_raw, -cfg.current_limit_pk_a, cfg.current_limit_pk_a);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float current_scale = (fabsf(id_raw) > cfg.current_limit_pk_a) ? id_ref / id_raw : 1.0f;
        delta_power[cell] = current_scale * (power_request[cell] - power_sum / (float)CHB_CELL_COUNT);
    }
    {
        float q_max = sqrtf(fmaxf(0.0f, cfg.current_limit_pk_a * cfg.current_limit_pk_a - id_ref * id_ref));
        float q_floor = sqrtf(fmaxf(0.0f,
                                    CHB_BALANCE_CURRENT_MIN_PK_A * CHB_BALANCE_CURRENT_MIN_PK_A - id_ref * id_ref));
        float grid_peak = M_SQRT2 * p_sample->grid_rms_v; /* 本拍测得的工频电压峰值。 */

        if (inter.pf_plan_ticks == 0u)
        {
            inter.pf_zero_feasible = pf_candidate_feasible(id_ref, 0.0f, grid_peak, delta_power);
            inter.q_target_a       = q_floor;

            if (!pf_candidate_feasible(id_ref, q_floor, grid_peak, delta_power))
            {
                float lower      = q_floor; /* 已知不可行的无功幅值下界，A。 */
                inter.q_target_a = q_max;

                for (uint32_t scan = 1u; scan <= CHB_PF_SCAN_STEPS; ++scan)
                {
                    float upper = q_floor + (q_max - q_floor) * (float)scan / (float)CHB_PF_SCAN_STEPS;

                    if (pf_candidate_feasible(id_ref, upper, grid_peak, delta_power))
                    {
                        for (uint32_t iteration = 0u; iteration < CHB_PF_REFINE_STEPS; ++iteration)
                        {
                            float middle = 0.5f * (lower + upper); /* 二分候选无功幅值。 */

                            if (pf_candidate_feasible(id_ref, middle, grid_peak, delta_power))
                            {
                                upper = middle;
                            }
                            else
                            {
                                lower = middle;
                            }
                        }
                        inter.q_target_a = upper;
                        break;
                    }
                    lower = upper;
                }
            }
        }
        inter.pf_plan_ticks = (inter.pf_plan_ticks + 1u) % CHB_PF_PLAN_TICKS;
        inter.q_magnitude_ref_a += limit_float(inter.q_target_a - inter.q_magnitude_ref_a,
                                               -CHB_PF_Q_SLEW_A_PER_S * cfg.ts,
                                               CHB_PF_Q_SLEW_A_PER_S * cfg.ts);
        iq_ref = -fminf(q_max, fmaxf(q_floor, inter.q_magnitude_ref_a));
    }
    current_ref_squared = id_ref * id_ref + iq_ref * iq_ref;
    current_magnitude = sqrtf(current_ref_squared);
    current_unit_d    = id_ref / current_magnitude;
    current_unit_q    = iq_ref / current_magnitude;

    {
        float current_cosine = cosine * inter.current_phase_cos + sine * inter.current_phase_sin;
        float current_sine = sine * inter.current_phase_cos - cosine * inter.current_phase_sin;
        /* 旋转相角回到电流采样时刻，把延迟反馈还原为本拍工频矢量。 */
        id = p_sample->i_alpha_a * current_cosine + p_sample->i_beta_a * current_sine;
        iq = -p_sample->i_alpha_a * current_sine + p_sample->i_beta_a * current_cosine;
    }
    id_error = id_ref - id;
    iq_error = iq_ref - iq;
    v_bridge_d = M_SQRT2 * p_sample->grid_rms_v - cfg.r_grid_ohm * id
               + M_2PI * cfg.grid_hz * cfg.l_grid_h * iq
               - cfg.current_kp * id_error - inter.current_integral_d_v;
    v_bridge_q = -cfg.r_grid_ohm * iq - M_2PI * cfg.grid_hz * cfg.l_grid_h * id
               - cfg.current_kp * iq_error
               - inter.current_integral_q_v;

    magnitude        = hypotf(v_bridge_d, v_bridge_q);
    magnitude_budget = cfg.modulation_limit * bus_sum;

    if (magnitude > magnitude_budget)
    {
        float scale = magnitude_budget / magnitude; /* 保留总矢量方向的限幅比例。 */
        v_bridge_d *= scale;
        v_bridge_q *= scale;
        magnitude     = magnitude_budget;
        total_limited = true;
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float capacity_fraction = cfg.modulation_limit * p_sample->bus_v[cell] / magnitude_budget;
        float base_d_v = capacity_fraction * v_bridge_d; /* 母线不等时也可行的限幅锚点。 */
        float base_q_v = capacity_fraction * v_bridge_q;

        base_parallel[cell] = base_d_v * current_unit_d + base_q_v * current_unit_q;
        base_orthogonal[cell] = -base_d_v * current_unit_q + base_q_v * current_unit_d;
        /* 单相平均功率为 0.5 * (vd * id + vq * iq)。 */
        power_voltage[cell] = (v_bridge_d * current_unit_d + v_bridge_q * current_unit_q)
                                / (float)CHB_CELL_COUNT
                            + 2.0f * delta_power[cell] / current_magnitude - base_parallel[cell];
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
                float parallel = base_parallel[cell] + candidate * power_voltage[cell];
                float cell_limit = cfg.modulation_limit * p_sample->bus_v[cell];
                float remaining = cell_limit * cell_limit - parallel * parallel;

                if (remaining < 0.0f)
                {
                    feasible = false;
                    break;
                }
                {
                    float orth_limit = sqrtf(remaining);
                    lower_sum += -orth_limit - base_orthogonal[cell];
                    upper_sum += orth_limit - base_orthogonal[cell];
                }
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
        balance_scale = scale_lower;
    }
    {
        float capacity_sum = 0.0f;
        float total_orthogonal = -v_bridge_d * current_unit_q + v_bridge_q * current_unit_d;
        float orth_fraction = 0.0f;

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            float parallel = base_parallel[cell] + balance_scale * power_voltage[cell];
            float cell_limit = cfg.modulation_limit * p_sample->bus_v[cell];
            orth_capacity[cell] = sqrtf(fmaxf(0.0f,
                                              cell_limit * cell_limit
                                                  - parallel * parallel));
            capacity_sum += orth_capacity[cell];
            balance_utilization = fmaxf(balance_utilization, fabsf(parallel) / cell_limit);
        }
        /* 按剩余电压容量分配正交分量，使三桥共享同一正交容量利用率。 */

        if (capacity_sum > 0.0f)
        {
            orth_fraction = total_orthogonal / capacity_sum;
        }
        balance_utilization = fmaxf(balance_utilization, fabsf(orth_fraction));

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            float parallel = base_parallel[cell] + balance_scale * power_voltage[cell];
            float orthogonal = orth_fraction * orth_capacity[cell];

            command.cell_d_v[cell] = parallel * current_unit_d - orthogonal * current_unit_q;
            command.cell_q_v[cell] = parallel * current_unit_q + orthogonal * current_unit_d;
        }
    }
    cell_limited = balance_scale < 0.99999f;

    total_v_pwm = v_bridge_d * cosine - v_bridge_q * sine;
    command.total_v_pwm_v = total_v_pwm;
    command.total_d_v     = v_bridge_d;
    command.total_q_v     = v_bridge_q;
    command.i_comp_ref_a = id_ref * cosine - iq_ref * sine;
    command.i_comp_beta_ref_a = id_ref * sine + iq_ref * cosine;
    command.id_ref_a  = id_ref;
    command.theta_rad = p_sample->theta_rad;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        command.v_pwm_v[cell] = command.cell_d_v[cell] * cosine - command.cell_q_v[cell] * sine;
        command.bus_v[cell] = p_sample->bus_v[cell];
    }
#if defined(PLATFORM_PLECS)
    float filtered_sum = 0.0f; /* 诊断用三桥滤波电压和，V。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        filtered_sum += inter.bus_filtered_v[cell];
    }
    diag.bus_sum_v          = bus_sum;
    diag.bus_filtered_sum_v = filtered_sum;
    diag.bus_error_v = (float)CHB_CELL_COUNT * inter.bus_ref_ramped_v - filtered_sum;
    diag.bus_ref_ramped_v    = inter.bus_ref_ramped_v;
    diag.id_ref_a            = id_ref;
    diag.iq_ref_a            = iq_ref;
    diag.balance_scale       = balance_scale;
    diag.balance_utilization = balance_utilization;
    diag.q_min_a             = inter.q_target_a;
    diag.pf_zero_feasible    = inter.pf_zero_feasible ? 1u : 0u;
    diag.id_a                = id;
    diag.iq_a                = iq;
    diag.vd_pwm_v            = v_bridge_d;
    diag.vq_pwm_v            = v_bridge_q;
    diag.vpwm_v              = total_v_pwm;
    diag.total_limited       = total_limited ? 1u : 0u;
    diag.cell_limited        = cell_limited ? 1u : 0u;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        diag.balance_error_v[cell] = filtered_sum / (float)CHB_CELL_COUNT - inter.bus_filtered_v[cell];
        diag.power_request_w[cell] = power_request[cell];
        diag.power_limited[cell] = (voltage_power[cell] > power_ceiling[cell]) ? 1u : 0u;
    }
#endif
    chb_hal_get_ctrl()->p_set_pwm_func(&command); /* 同一快照同步下发三桥命令。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float achievable_power = 0.5f
                               * (command.cell_d_v[cell] * id_ref
                                  + command.cell_q_v[cell] * iq_ref);
        float selector_gap = voltage_power[cell] - power_request[cell];
        float actuator_gap = power_request[cell] - achievable_power;
        bool selector_allows = (fabsf(selector_gap) <= 1.0f)
                            || (selector_gap * energy_error[cell] < 0.0f);
        bool actuator_allows = !(    total_limited
                                  || cell_limited
                                  || (fabsf(id_raw) > cfg.current_limit_pk_a))
                            || (fabsf(actuator_gap) <= 1.0f)
                            || (actuator_gap * energy_error[cell] < 0.0f);
        /* 选择器及后级调制/电流约束均参与能量积分抗饱和。 */

        if (    selector_allows
             && actuator_allows)
        {
            inter.energy_integral_w[cell] += cfg.energy_ki * cfg.ts * energy_error[cell];
        }
        /* 未被选择的功率环保持积分，避免轻载时长期积累正功率误差。 */

        if (    (voltage_power[cell] >= power_ceiling[cell])
             && (    (    (power_ceiling_raw[cell] > -power_capacity[cell])
                       && (power_ceiling_raw[cell] < power_capacity[cell]))
                  || (    (power_ceiling_raw[cell] >= power_capacity[cell])
                       && (power_error[cell] < 0.0f))
                  || (    (power_ceiling_raw[cell] <= -power_capacity[cell])
                       && (power_error[cell] > 0.0f))))
        {
            inter.power_integral_w[cell] += cfg.power_limit_ki * cfg.ts * power_error[cell];
        }
    }

    if (!total_limited)
    {
        inter.current_integral_d_v = limit_float(inter.current_integral_d_v
                                                     + cfg.current_ki * cfg.ts * id_error,
                                                 -cfg.current_integral_limit_v,
                                                 cfg.current_integral_limit_v);
        inter.current_integral_q_v = limit_float(inter.current_integral_q_v
                                                     + cfg.current_ki * cfg.ts * iq_error,
                                                 -cfg.current_integral_limit_v,
                                                 cfg.current_integral_limit_v);
    }
}
REG_INTERRUPT(3, chb_ctrl_run)
