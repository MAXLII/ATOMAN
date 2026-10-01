// SPDX-License-Identifier: MIT
/**
 * @file chb_ctrl.c
 * @brief 按控制框图组织的 CHB 母线、功率、电流与逐桥 PWM 控制。
 * @details 模块边界对应简图：采样与反馈、母线控制、功率给定、无功规划、
 *          电流控制、电压分配、谐波补偿、PWM；抗饱和作为闭环反馈支路。
 *          每个模块独立封装 input / inter / output，const 输入表描述模块间连线。
 *          采样、应用保护、控制发波按优先级 1 / 2 / 3 串行执行，积分反馈延后一拍。
 *          所有桥数组由 CHB_CELL_COUNT 决定；不在模块接口中固化桥数。
 *          控制参数由 cfg 在 INIT 固定；保护策略和硬件寄存器操作由各自层负责。
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
#include "chb_protect.h"
#include "msogi.h"
#include "my_math.h"
#include "notch.h"
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
#define CHB_OBSERVER_READY_TICKS       400u
#define CHB_OBSERVER_PLL_Q_ERROR_RATIO 0.005000042f /* tan(0.005 rad)，用于 q/d 锁相偏差判定。 */
#define CHB_OBSERVER_HARM_SETTLE_TICKS 2000u
#define CHB_OBSERVER_HARM_RESTORE_STEP 0.0005f

/*
 * Entity: HAL 原始采样和 app 的 RMS、PLL 结果每拍复制到静态变量；ctrl 持有控制观测历史。
 * Prior: INIT 检查绑定；应用保护消费同拍信号，限幅及抗饱和继续生效。
 * Time: app 测量 0 -> ctrl 采样及观测 1 -> 应用保护 2 -> 控制发波 3，串行执行。
 */
static float v_grid_raw;                              /* HAL 电网电压的本拍副本，V。 */
static float i_grid_alpha_raw;                        /* HAL 网侧电流的本拍副本，A。 */
static float v_bus_raw[CHB_CELL_COUNT];                /* HAL 各桥母线电压的本拍副本，V。 */
static float i_load_raw[CHB_CELL_COUNT];               /* HAL 各桥负载电流的本拍副本，A。 */
static float v_grid_rms;                              /* HAL 电网电压有效值的本拍副本，V。 */
static float omega_grid_sampled;                      /* HAL PLL 角频率的本拍副本，rad/s。 */
static float f_grid;                                  /* 本拍角频率转换成 Hz，仅供 FSM、保护及诊断。 */
static float theta_grid;                              /* HAL PLL 相位的本拍副本，rad。 */
static float i_grid_beta_observed;                    /* 电流正交观测，A。 */
static float v_grid_fundamental_beta;                 /* 电网基波正交分量，V。 */
static float v_grid_harmonic_alpha[CHB_HARMONIC_COUNT]; /* 电网各次谐波同相分量，V。 */
static float v_grid_harmonic_beta[CHB_HARMONIC_COUNT];  /* 电网各次谐波正交分量，V。 */
static float i_grid_harmonic_alpha[CHB_HARMONIC_COUNT]; /* 电流各次谐波同相分量，A。 */
static float i_grid_harmonic_beta[CHB_HARMONIC_COUNT];  /* 电流各次谐波正交分量，A。 */
static float v_grid_harmonic_peak[CHB_HARMONIC_COUNT];  /* 谐波电压峰值，仅供诊断，V。 */
static float i_grid_harmonic_peak[CHB_HARMONIC_COUNT];  /* 谐波电流峰值，仅供诊断，A。 */
static float harmonic_feedback_weight;                /* 稳频观测可信度，0..1。 */
static float f_grid_rate;                             /* 观测频率变化率，Hz/s。 */
static msogi_t               grid_msogi                      = {0};
static msogi_t               current_msogi                   = {0};
static sogi_t                current_sogi                    = {0};
static sogi_t                current_quadrature_sogi         = {0};
static float                 i_grid_sogi_input               = 0.0f; /* SOGI 的可写绑定源，仅保存本拍原始电流。 */
static float                 i_grid_ac                       = 0.0f;
static float                 f_grid_filtered                 = 0.0f;
static uint32_t              observer_ticks                  = 0u;
static uint32_t              harmonic_settle_ticks           = 0u;
static uint8_t               ready                           = 0u;

/** @brief 控制器内部用于生成最终逐级发波电压的同拍计划。 */
typedef struct
{
    float v_bus_raw[CHB_CELL_COUNT];
    float v_cell_d[CHB_CELL_COUNT];
    float v_cell_q[CHB_CELL_COUNT];
    float i_comp_alpha_ref;
    float i_comp_beta_ref;
    float theta_grid;
    float v_comp_harmonic_alpha[CHB_HARMONIC_COUNT];
    float v_comp_harmonic_beta[CHB_HARMONIC_COUNT];
    float omega_grid; /* 同拍角频率，rad/s；PWM 延迟补偿直接使用。 */
    float v_bus_total; /* 本拍母线和，只在冻结计划时累计一次，V。 */
    float v_cell_limit[CHB_CELL_COUNT]; /* 本拍逐桥调制上限，供所有作用时刻复用，V。 */
    float v_pwm_capacity_total;         /* 按桥顺序累计的本拍总调制容量，V。 */
} chb_pwm_plan_t;

/**
 * @brief 初始化控制所需的基波、谐波观测及启动可信度状态。
 * @details 由 INIT 读取固定配置，绑定静态原始采样副本；配置或 HAL 未就绪时保持 ready 为 0。
 *          所有观测器初始化成功后才发布 ready，不调用 PWM，也不授予运行许可。
 */
static void chb_observer_init(void)
{
    const chb_ctrl_cfg_t *p_cfg = chb_cfg_get_ctrl_cfg();
    float omega_grid_center     = 0.0f;

    ready                    = 0u;
    v_grid_rms               = 0.0f;
    omega_grid_sampled       = 0.0f;
    f_grid                   = 0.0f;
    theta_grid               = 0.0f;
    i_grid_beta_observed     = 0.0f;
    v_grid_fundamental_beta  = 0.0f;
    harmonic_feedback_weight = 0.0f;
    f_grid_rate              = 0.0f;
    f_grid_filtered          = 0.0f;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        v_grid_harmonic_alpha[harmonic] = 0.0f;
        v_grid_harmonic_beta[harmonic]  = 0.0f;
        i_grid_harmonic_alpha[harmonic] = 0.0f;
        i_grid_harmonic_beta[harmonic]  = 0.0f;
        v_grid_harmonic_peak[harmonic]  = 0.0f;
        i_grid_harmonic_peak[harmonic]  = 0.0f;
    }
    observer_ticks        = 0u;
    harmonic_settle_ticks = 0u;
    v_grid_raw            = 0.0f;
    i_grid_alpha_raw      = 0.0f;
    i_grid_sogi_input     = 0.0f;
    i_grid_ac             = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        v_bus_raw[cell]  = 0.0f;
        i_load_raw[cell] = 0.0f;
    }

    if (    (chb_cfg_is_ready() == 0u)
         || (chb_hal_is_ready() == 0u))
    {
        return;
    }

    /* 地址只在 INIT 连接；任一接收模块拒绝绑定时保持未就绪。 */
    if (    (chb_fsm_set_input(&v_grid_raw, &v_grid_rms, &f_grid, v_bus_raw) == 0u)
         || (chb_protect_set_input(&v_grid_raw, &v_grid_rms, &f_grid, &theta_grid,
                                   &i_grid_alpha_raw, &i_grid_beta_observed,
                                   v_bus_raw, i_load_raw) == 0u))
    {
        return;
    }

    omega_grid_center = M_2PI * p_cfg->f_grid;
    const chb_ctrl_hal_t *p_hal = chb_hal_get_ctrl();
    v_grid_rms         = *p_hal->p_v_grid_rms;
    omega_grid_sampled = *p_hal->p_omega_grid;
    theta_grid         = *p_hal->p_theta_grid;
    f_grid             = omega_grid_sampled / M_2PI;
    f_grid_filtered    = f_grid;

    if (    (msogi_init(&grid_msogi,
                        p_cfg->t_ctrl_period,
                        omega_grid_center,
                        CHB_OBSERVER_SOGI_K,
                        &v_grid_raw) == 0u)
         || (msogi_init(&current_msogi,
                        p_cfg->t_ctrl_period,
                        omega_grid_center,
                        CHB_OBSERVER_CURRENT_MSOGI_K,
                        &i_grid_alpha_raw) == 0u))
    {
        return;
    }
    sogi_init(&current_sogi,
              p_cfg->t_ctrl_period,
              omega_grid_center,
              CHB_OBSERVER_SOGI_K,
              &i_grid_sogi_input);
    sogi_init(&current_quadrature_sogi,
              p_cfg->t_ctrl_period,
              omega_grid_center,
              CHB_OBSERVER_SOGI_K,
              &i_grid_ac);

    ready = 1u;
}
REG_INIT(2, chb_observer_init) /* FSM 先回到 INIT，再连接保护/状态机的只读输入。 */

/**
 * @brief 读取采样观测器的初始化状态。
 * @details 供 FSM INIT 判断能否离开初始化阶段；不代表本拍采样已经通过应用保护。
 * @return 1：观测器初始化完成；0：未完成或初始化失败。
 */
uint8_t chb_ctrl_is_ready(void)
{
    return ready;
}

/**
 * @brief 用 HAL 同拍副本更新控制所需的基波、谐波及可信度。
 * @details RMS、角频率、相位均由 app 计算，ctrl 不再维护 PLL 或 RMS 窗口。
 *          SOGI/MSOGI 跟随输入角频率；启动等待、频率变化和 dq 偏差继续约束谐波反馈。
 *          dq 偏差只是可信度判据，使用上一拍的 PLL 相位，不反算或修改相位。
 */
static void FUNC_RAM chb_observer_update(void)
{
    const float omega_grid_observed = omega_grid_sampled;

    i_grid_sogi_input = i_grid_alpha_raw;

    /* 同周期、同频率的两套 MSOGI 复用预畸变正切；各自增益及动态历史独立。 */
    msogi_update_frequency(&grid_msogi, omega_grid_observed);
    msogi_cal(&grid_msogi);
    msogi_update_frequency_with_tangent(&current_msogi, omega_grid_observed, grid_msogi.tangent);
    msogi_cal(&current_msogi);
    sogi_update_frequency(&current_sogi, omega_grid_observed);
    sogi_update_frequency(&current_quadrature_sogi, omega_grid_observed);
    sogi_cal(&current_sogi);
    i_grid_ac = current_sogi.osg_u[0];
    sogi_cal(&current_quadrature_sogi);
    i_grid_beta_observed = current_quadrature_sogi.osg_qu[0];

    if (observer_ticks < CHB_OBSERVER_READY_TICKS)
    {
        ++observer_ticks; /* 等待控制观测幅值建立。 */
    }

    /* PLL 输出相位已推进一拍；对当前电压做 dq 可信度判断时退回采样相位。
     * 这里只投影电压，不生成另一套相位或锁相环。 */
    const float theta_grid_sample = theta_grid - omega_grid_sampled * grid_msogi.ts;
    const float grid_cosine = cosf(theta_grid_sample);
    const float grid_sine = sinf(theta_grid_sample);
    const float v_grid_d = grid_msogi.alpha[0] * grid_cosine + grid_msogi.beta[0] * grid_sine;
    const float v_grid_q = -grid_msogi.alpha[0] * grid_sine + grid_msogi.beta[0] * grid_cosine;

    f_grid_rate = (f_grid - f_grid_filtered) / 0.01f;
    f_grid_filtered += 0.01f * (f_grid - f_grid_filtered);

    if (    (observer_ticks < CHB_OBSERVER_READY_TICKS)
         || (fabsf(f_grid_rate) > 10.0f)
         || (v_grid_d <= 0.0f) /* 反相及幅值尚未建立时不恢复谐波反馈。 */
         || (fabsf(v_grid_q) > CHB_OBSERVER_PLL_Q_ERROR_RATIO * v_grid_d))
    {
        harmonic_settle_ticks    = 0u;
        harmonic_feedback_weight = 0.0f;
    }
    else if (harmonic_settle_ticks < CHB_OBSERVER_HARM_SETTLE_TICKS)
    {
        ++harmonic_settle_ticks;
    }
    else
    {
        harmonic_feedback_weight += CHB_OBSERVER_HARM_RESTORE_STEP;
        UP_LMT(harmonic_feedback_weight, 1.0f);
    }

    v_grid_fundamental_beta = grid_msogi.beta[0];

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const uint32_t channel                 = harmonic + 1u;
        v_grid_harmonic_alpha[harmonic] = grid_msogi.alpha[channel];
        v_grid_harmonic_beta[harmonic]  = grid_msogi.beta[channel];
        i_grid_harmonic_alpha[harmonic] = current_msogi.alpha[channel];
        i_grid_harmonic_beta[harmonic]  = current_msogi.beta[channel];
        v_grid_harmonic_peak[harmonic]  = hypotf(grid_msogi.alpha[channel], grid_msogi.beta[channel]);
        i_grid_harmonic_peak[harmonic] =
            hypotf(current_msogi.alpha[channel], current_msogi.beta[channel]);
    }
}

/**
 * @brief 将 HAL 原始采样及 app 的电网测量结果复制到 ctrl 静态变量。
 * @details 每个输入地址每拍只读取一次；只将角频率转为 FSM、保护所需的 Hz，不做滤波或锁相。
 *          INIT 已验证 HAL 绑定，后续处理统一使用这里取得的本拍副本。
 */
static inline void update_adc_feedback(void)
{
    const chb_ctrl_hal_t *p_hal = chb_hal_get_ctrl();

    v_grid_raw        = *p_hal->p_v_grid_raw;
    v_grid_rms        = *p_hal->p_v_grid_rms;
    omega_grid_sampled = *p_hal->p_omega_grid;
    theta_grid        = *p_hal->p_theta_grid;
    f_grid            = omega_grid_sampled / M_2PI;
    i_grid_alpha_raw  = *p_hal->p_i_grid_alpha_raw;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        v_bus_raw[cell]  = *p_hal->p_v_bus_raw[cell];
        i_load_raw[cell] = *p_hal->p_i_load_raw[cell];
    }
}

/**
 * @brief 先采样到内部静态变量，再更新控制所需的基波及谐波观测。
 * @details 由第一阶段采样或 FSM INIT 调用；调用方保证绑定及观测器已就绪。
 *          保护、FSM 与控制的只读端口均连接这些静态变量，不复制公共采样对象。
 */
void FUNC_RAM chb_ctrl_update_sample(void)
{
    update_adc_feedback(); /* HAL -> ctrl 内部原始采样。 */
    chb_observer_update(); /* 内部原始采样 -> 同拍观测结果。 */
}

/**
 * @brief 读取 PWM 日志所需的观测量，不参与控制计算。
 * @param[out] p_theta_grid 本拍电网相角，rad，地址由调用方保证有效。
 * @param[out] p_i_grid_beta 本拍正交电流，A，地址由调用方保证有效。
 * @note 在 PLECS 串行调度内复制，不能据此修改内部采样。
 */
void chb_ctrl_read_phase(float *p_theta_grid, float *p_i_grid_beta)
{
    *p_theta_grid = theta_grid;
    *p_i_grid_beta = i_grid_beta_observed;
}

/* 谐波补偿输出的周期波形由下游规划、限幅和分配共同只读使用。 */
typedef struct chb_voltage_wave
{
    float v_harmonic_normalized[CHB_VOLTAGE_PHASE_POINTS]; /* 每伏母线对应的完整谐波波形。 */
    float v_curvature_normalized; /* 谐波对电角度的二阶导数幅值上界。 */
} chb_voltage_wave_t;

typedef struct chb_ctrl_reactive_inter
{
    float    i_grid_q_ramped; /* 经斜坡的负 q 电流幅值，A 峰值。 */
    float    i_grid_q_target; /* 周期可行域目标幅值，A 峰值。 */
    uint32_t ticks;           /* 距上次完整搜索的控制拍数。 */
    bool     zero_feasible;   /* 本次搜索中零无功的可行性。 */
} chb_ctrl_reactive_inter_t;

/*
 * Entity: each control block owns its input ports, private state and output ports.
 * Prior: input ports are const signal pointers; existing limits and protection remain active.
 * Time: wires have static lifetime; upstream outputs precede downstream reads in one tick.
 * inter contains only actual history or scratch; large waveform outputs use static storage.
 */
typedef struct chb_ctrl_feedback_input
{
    const float *p_theta_grid; /* 本拍电网相角，rad。 */
    const float *p_omega_grid; /* 本拍电网角频率，rad/s。 */
    const float *p_v_bus_raw;  /* 各桥实时母线电压，V。 */
    const float *p_i_load_raw; /* 各桥实时负载电流，A。 */
} chb_ctrl_feedback_input_t;

typedef struct chb_ctrl_feedback_inter
{
    float   v_bus_notch_input[CHB_CELL_COUNT];    /* 母线陷波器绑定源，V。 */
    notch_t bus_notch[CHB_CELL_COUNT];            /* 各桥独立二倍工频陷波历史。 */
    float   v_bus_lpf_input_last[CHB_CELL_COUNT]; /* 母线低通上一拍输入，V。 */
    float   v_bus_filtered[CHB_CELL_COUNT];       /* 母线低通上一拍输出，V。 */
    float   pwr_load_notch_input[CHB_CELL_COUNT]; /* 负载功率陷波器绑定源，W。 */
    notch_t power_notch[CHB_CELL_COUNT];          /* 各桥独立负载功率陷波历史。 */
    float   pwr_load_lpf_input_last[CHB_CELL_COUNT]; /* 负载功率低通上一拍输入，W。 */
    float   pwr_load_filtered[CHB_CELL_COUNT];    /* 负载功率低通上一拍输出，W。 */
} chb_ctrl_feedback_inter_t;

typedef struct chb_ctrl_feedback_output
{
    float cosine;     /* 本拍电网相角余弦。 */
    float sine;       /* 本拍电网相角正弦。 */
    float omega_grid; /* 本拍电网角频率，rad/s。 */
    float current_phase_cos; /* 电流采样延迟的相位余弦。 */
    float current_phase_sin; /* 电流采样延迟的相位正弦。 */
    float v_bus_total;       /* 实时母线电压和，V。 */
    float v_bus_filtered[CHB_CELL_COUNT];    /* 各桥滤波母线电压，V。 */
    float pwr_load_filtered[CHB_CELL_COUNT]; /* 各桥滤波负载功率，W。 */
} chb_ctrl_feedback_output_t;

typedef struct chb_ctrl_harmonic_input
{
    const float *p_theta_grid; /* 本拍电网相角，rad。 */
    const float *p_v_grid_rms; /* 电网基波有效值，V。 */
    const float *p_harmonic_feedback_weight; /* 稳频观测可信度，0..1。 */
    const float *p_v_grid_harmonic_alpha;    /* 电网谐波同相分量，V。 */
    const float *p_v_grid_harmonic_beta;     /* 电网谐波正交分量，V。 */
    const float *p_i_grid_harmonic_alpha;    /* 网侧谐波电流同相分量，A。 */
    const float *p_i_grid_harmonic_beta;     /* 网侧谐波电流正交分量，A。 */
    const float *p_omega_grid;  /* 本拍电网角频率，rad/s。 */
    const float *p_v_bus_total; /* 实时母线电压和，V。 */
} chb_ctrl_harmonic_input_t;

typedef struct chb_ctrl_harmonic_output
{
    float              v_comp_harmonic_alpha[CHB_HARMONIC_COUNT]; /* 谐波补偿电压同相分量，V。 */
    float              v_comp_harmonic_beta[CHB_HARMONIC_COUNT];  /* 谐波补偿电压正交分量，V。 */
    chb_voltage_wave_t voltage_wave; /* 供下游容量求解使用的完整周期波形。 */
    float              v_harmonic_reserve_normalized; /* 谐波峰值预算，每伏母线。 */
    float              harmonic_scale;          /* 物理容量允许的统一谐波缩放。 */
    float              harmonic_feedback_scale; /* 基波前馈优先后的谐波反馈比例。 */
} chb_ctrl_harmonic_output_t;

/** @brief 母线控制输入：滤波反馈及上一拍发布的两个外环积分端口。 */
typedef struct chb_ctrl_bus_input
{
    const float *p_v_bus_filtered;      /* 各桥滤波母线电压，V。 */
    const float *p_pwr_load_filtered;   /* 各桥滤波负载功率，W。 */
    const float *p_pwr_energy_integral; /* 上一拍能量环积分，W。 */
    const float *p_pwr_limit_integral;  /* 上一拍负载功率限制环积分，W。 */
} chb_ctrl_bus_input_t;

/** @brief 母线控制私有状态；目标及斜率由固定 cfg 提供。 */
typedef struct chb_ctrl_bus_inter
{
    float v_bus_ref_ramped; /* 由预充电压开始的每桥参考斜坡历史，V。 */
} chb_ctrl_bus_inter_t;

/** @brief 母线控制输出：向功率给定送请求/上界，向抗饱和反馈送误差。 */
typedef struct chb_ctrl_bus_output
{
    float v_bus_ref_ramped;            /* 本拍每桥母线给定，V。 */
    float e_bus_error[CHB_CELL_COUNT]; /* 电容能量误差，J。 */
    float pwr_energy_ref[CHB_CELL_COUNT];     /* 能量环总功率请求，W。 */
    float pwr_load_error[CHB_CELL_COUNT];     /* 输出功率上限与负载功率之差，W。 */
    float pwr_cell_upper_raw[CHB_CELL_COUNT]; /* 功率限制环未限幅的请求上界，W。 */
    float pwr_cell_upper[CHB_CELL_COUNT];     /* 受物理容量约束的请求上界，W。 */
    float pwr_cell_capacity[CHB_CELL_COUNT];  /* 各桥调制及电流允许的功率幅值，W。 */
} chb_ctrl_bus_output_t;

typedef struct chb_ctrl_power_input
{
    const float *p_v_grid_rms;        /* 电网基波有效值，V。 */
    const float *p_pwr_energy_ref;    /* 各桥能量环请求，W。 */
    const float *p_pwr_cell_upper;    /* 各桥功率限制环上界，W。 */
    const float *p_pwr_cell_capacity; /* 各桥物理功率容量，W。 */
} chb_ctrl_power_input_t;

typedef struct chb_ctrl_power_output
{
    float pwr_cell_ref[CHB_CELL_COUNT]; /* 功率选择器输出，W。 */
    float i_grid_d_ref_raw;             /* 限幅前有功电流给定，A 峰值。 */
    float i_grid_d_ref; /* 限幅后有功电流给定，A 峰值。 */
    float pwr_cell_delta[CHB_CELL_COUNT]; /* 相对平均值的零和差模功率请求，W。 */
} chb_ctrl_power_output_t;

typedef struct chb_ctrl_reactive_input
{
    const float              *p_v_grid_rms;     /* 电网基波有效值，V。 */
    const float              *p_i_grid_d_ref;   /* 限幅后有功电流给定，A 峰值。 */
    const float              *p_pwr_cell_delta; /* 相对平均值的零和差模功率请求，W。 */
    const float              *p_v_bus_filtered; /* 各桥滤波母线电压，V。 */
    const float              *p_omega_grid;     /* 本拍电网角频率，rad/s。 */
    const chb_voltage_wave_t *p_voltage_wave;   /* 完整周期谐波波形和节点间曲率误差界。 */
} chb_ctrl_reactive_input_t;

typedef struct chb_ctrl_reactive_output
{
    float i_grid_q_ref;         /* 带符号的 q 轴电流给定，A 峰值。 */
    float i_grid_ref_magnitude; /* 参考电流矢量模长，A。 */
    float i_direction_d;        /* 沿参考电流方向的 d 轴单位分量。 */
    float i_direction_q;        /* 沿参考电流方向的 q 轴单位分量。 */
    float i_grid_q_min;         /* 周期可行域目标无功幅值，A。 */
    bool  zero_feasible;        /* 本次搜索的零无功可行性。 */
} chb_ctrl_reactive_output_t;

typedef struct chb_ctrl_current_input
{
    const float *p_i_grid_alpha_raw;     /* 网侧电流同相分量，A。 */
    const float *p_i_grid_beta_observed; /* 网侧电流正交分量，A。 */
    const float *p_v_grid_raw;           /* 本拍电网瞬时电压，V。 */
    const float *p_v_grid_fundamental_beta;  /* 电网基波正交分量，V。 */
    const float *p_harmonic_feedback_weight; /* 稳频观测可信度，0..1。 */
    const float *p_v_grid_harmonic_alpha;    /* 电网谐波同相分量，V。 */
    const float *p_cosine; /* 本拍电网相角余弦。 */
    const float *p_sine;   /* 本拍电网相角正弦。 */
    const float *p_current_phase_cos;    /* 电流采样延迟的相位余弦。 */
    const float *p_current_phase_sin;    /* 电流采样延迟的相位正弦。 */
    const float *p_omega_grid;           /* 本拍电网角频率，rad/s。 */
    const float *p_i_grid_d_ref;         /* 限幅后有功电流给定，A 峰值。 */
    const float *p_i_grid_q_ref;         /* 带符号的 q 轴电流给定，A 峰值。 */
    const float *p_v_current_d_integral; /* 反馈给 d 轴电流环的积分量，V。 */
    const float *p_v_current_q_integral; /* 反馈给 q 轴电流环的积分量，V。 */
} chb_ctrl_current_input_t;

typedef struct chb_ctrl_current_output
{
    float i_grid_d;       /* 电流 d 轴反馈，A 峰值。 */
    float i_grid_q;       /* 电流 q 轴反馈，A 峰值。 */
    float i_grid_d_error; /* d 轴电流误差，A。 */
    float i_grid_q_error; /* q 轴电流误差，A。 */
    float v_bridge_d;     /* 电流环原始 d 轴总电压请求，V。 */
    float v_bridge_q;     /* 电流环原始 q 轴总电压请求，V。 */
} chb_ctrl_current_output_t;

typedef struct chb_ctrl_voltage_limit_output
{
    float v_bridge_d;    /* 完整波形限幅后的 d 轴总电压，V。 */
    float v_bridge_q;    /* 完整波形限幅后的 q 轴总电压，V。 */
    bool  total_limited; /* 总桥基波电压受到限幅。 */
} chb_ctrl_voltage_limit_output_t;

typedef struct chb_ctrl_balance_output
{
    float v_anchor_parallel[CHB_CELL_COUNT];  /* 可行锚点沿电流的投影，V。 */
    float v_balance_parallel[CHB_CELL_COUNT]; /* 差模功率请求对应的平行电压，V。 */
    float balance_scale; /* 差模有功电压的可行缩放，0..1。 */
    float v_orthogonal_lower[CHB_CELL_COUNT]; /* 最后一次可行候选的逐桥正交下界，V。 */
    float v_orthogonal_upper[CHB_CELL_COUNT]; /* 同一候选的逐桥正交上界，V。 */
    bool  ranges_valid; /* 本拍可行候选已完成所有桥及总和检查，分配模块可直接复用区间。 */
} chb_ctrl_balance_output_t;

typedef struct chb_ctrl_voltage_output
{
    float v_cell_d[CHB_CELL_COUNT]; /* 各桥分配后的 d 轴电压，V。 */
    float v_cell_q[CHB_CELL_COUNT]; /* 各桥分配后的 q 轴电压，V。 */
    float balance_utilization;      /* 本拍正交电压分配容量利用率。 */
    bool  cell_limited;             /* 差模分配受到容量约束。 */
    bool  total_limited;            /* 周期约束限制了总桥基波请求；供电流积分门控。 */
    float balance_scale;            /* 差模均衡可行缩放，0..1；供诊断使用。 */
} chb_ctrl_voltage_output_t;

/** @brief 电压分配输入：总桥请求、各桥容量和有功均衡需求。 */
typedef struct chb_ctrl_voltage_input
{
    const float              *p_v_current_d_raw;      /* 电流环原始总桥 d 轴电压，V。 */
    const float              *p_v_current_q_raw;      /* 电流环原始总桥 q 轴电压，V。 */
    const float              *p_v_bus_raw;            /* 各桥同拍实时母线电压，V。 */
    const float              *p_v_bus_total;          /* 同拍实时母线电压和，V。 */
    const float              *p_i_direction_d;        /* 参考电流方向的 d 轴单位分量。 */
    const float              *p_i_direction_q;        /* 参考电流方向的 q 轴单位分量。 */
    const float              *p_i_grid_ref_magnitude; /* 参考电流模长，A 峰值。 */
    const float              *p_pwr_cell_delta;       /* 各桥零和差模有功需求，W。 */
    const chb_voltage_wave_t *p_voltage_wave;         /* 谐波补偿提供的完整波形容量约束。 */
} chb_ctrl_voltage_input_t;

/** @brief 电压模块私有工作区；中间求解结果不再作为跨模块端口发布。 */
typedef struct chb_ctrl_voltage_inter
{
    chb_ctrl_voltage_limit_output_t limit;   /* 本拍完整波形限幅结果。 */
    chb_ctrl_balance_output_t       balance; /* 本拍均衡锚点及可行缩放。 */
} chb_ctrl_voltage_inter_t;

typedef struct chb_ctrl_pwm_input
{
    const float *p_theta_grid;   /* 本拍电网相角，rad。 */
    const float *p_omega_grid;   /* 本拍电网角频率，rad/s。 */
    const float *p_v_bus_raw;    /* 各桥实时母线电压，V。 */
    const float *p_cosine;       /* 本拍电网相角余弦。 */
    const float *p_sine;         /* 本拍电网相角正弦。 */
    const float *p_i_grid_d_ref; /* 限幅后有功电流给定，A 峰值。 */
    const float *p_i_grid_q_ref; /* 带符号的 q 轴电流给定，A 峰值。 */
    const float *p_v_comp_harmonic_alpha; /* 谐波补偿电压同相分量，V。 */
    const float *p_v_comp_harmonic_beta;  /* 谐波补偿电压正交分量，V。 */
    const float *p_v_cell_d; /* 各桥分配后的 d 轴电压，V。 */
    const float *p_v_cell_q; /* 各桥分配后的 q 轴电压，V。 */
} chb_ctrl_pwm_input_t;

typedef struct chb_ctrl_pwm_output
{
    float                   v_pwm_ref[CHB_CELL_COUNT];     /* 最终逐桥发波电压，V。 */
    chb_pwm_deadtime_flag_t deadtime_flag[CHB_CELL_COUNT]; /* 作用时刻预测电流的方向。 */
    float                   v_pwm_total; /* 最终逐桥发波电压之和，V。 */
} chb_ctrl_pwm_output_t;

typedef struct chb_ctrl_integrator_input
{
    const float *p_e_bus_error;        /* 母线电容能量误差，J。 */
    const float *p_pwr_energy_ref;     /* 能量环总功率请求，W。 */
    const float *p_pwr_cell_ref;       /* 功率选择器输出，W。 */
    const float *p_pwr_load_error;     /* 输出功率上限与负载功率之差，W。 */
    const float *p_pwr_cell_upper_raw; /* 功率限制环未限幅的上界，W。 */
    const float *p_pwr_cell_upper;     /* 受物理容量约束的功率上界，W。 */
    const float *p_pwr_cell_capacity;  /* 各桥电流及调制能力允许的功率幅值，W。 */
    const float *p_i_grid_d_ref_raw;   /* 限幅前有功电流给定，A 峰值。 */
    const float *p_i_grid_d_ref;       /* 限幅后有功电流给定，A 峰值。 */
    const float *p_i_grid_q_ref;       /* 带符号的 q 轴电流给定，A 峰值。 */
    const float *p_i_grid_d_error;     /* d 轴电流误差，A。 */
    const float *p_i_grid_q_error;     /* q 轴电流误差，A。 */
    const bool  *p_total_limited;      /* 总桥基波电压受到限幅。 */
    const bool  *p_cell_limited;       /* 差模分配受到容量约束。 */
    const float *p_v_cell_d;           /* 各桥分配后的 d 轴电压，V。 */
    const float *p_v_cell_q;           /* 各桥分配后的 q 轴电压，V。 */
} chb_ctrl_integrator_input_t;

typedef struct chb_ctrl_integrator_inter
{
    float pwr_energy_integral[CHB_CELL_COUNT]; /* 反馈给各桥能量环的积分量，W。 */
    float pwr_limit_integral[CHB_CELL_COUNT];  /* 反馈给各桥功率上限环的积分量，W。 */
    float v_current_d_integral; /* 反馈给 d 轴电流环的积分量，V。 */
    float v_current_q_integral; /* 反馈给 q 轴电流环的积分量，V。 */
} chb_ctrl_integrator_inter_t;

typedef struct chb_ctrl_integrator_output
{
    float pwr_energy_integral[CHB_CELL_COUNT]; /* 反馈给各桥能量环的积分量，W。 */
    float pwr_limit_integral[CHB_CELL_COUNT];  /* 反馈给各桥功率上限环的积分量，W。 */
    float v_current_d_integral; /* 反馈给 d 轴电流环的积分量，V。 */
    float v_current_q_integral; /* 反馈给 q 轴电流环的积分量，V。 */
} chb_ctrl_integrator_output_t;

/** @brief 谐波模块本拍内部计算量；预算每拍重新计算。 */
typedef struct chb_ctrl_harmonic_inter
{
    float v_feedback_peak; /* 校正后电流谐波反馈的合成峰值预算，V。 */
    float harmonic_phase_cos[CHB_HARMONIC_COUNT]; /* 本拍各次谐波相角余弦，前馈与周期波形共用。 */
    float harmonic_phase_sin[CHB_HARMONIC_COUNT]; /* 本拍各次谐波相角正弦，每拍覆盖。 */
} chb_ctrl_harmonic_inter_t;

/** @brief 功率给定模块本拍内部计算量；求和每拍从零开始。 */
typedef struct chb_ctrl_power_inter
{
    float pwr_cell_total; /* 当前选定各桥功率请求之和，W。 */
} chb_ctrl_power_inter_t;

/** @brief 电流模块本拍内部计算量，不包含跨拍积分历史。 */
typedef struct chb_ctrl_current_inter
{
    float v_grid_base_alpha; /* 扣除独立补偿谐波后的电网同相前馈，V。 */
} chb_ctrl_current_inter_t;

/*
 * Entity: 每个控制模块分别拥有 input、inter、output 三个成员。
 * Prior: input 成员是 const 连线；模块只写自身 inter/output，不更换输入绑定。
 * Time: 模块对象静态存在；初始化/停机只清 inter/output，积分反馈下一拍生效。
 */
/**
 * @brief 采样与反馈模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_feedback
{
    const chb_ctrl_feedback_input_t input;
    chb_ctrl_feedback_inter_t       inter;
    chb_ctrl_feedback_output_t      output;
} chb_ctrl_feedback_t;

/**
 * @brief 谐波补偿模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_harmonic
{
    const chb_ctrl_harmonic_input_t input;
    chb_ctrl_harmonic_inter_t       inter;
    chb_ctrl_harmonic_output_t      output;
} chb_ctrl_harmonic_t;

/**
 * @brief 母线控制模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_bus
{
    const chb_ctrl_bus_input_t input;
    chb_ctrl_bus_inter_t       inter;
    chb_ctrl_bus_output_t      output;
} chb_ctrl_bus_t;

/**
 * @brief 功率给定模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_power
{
    const chb_ctrl_power_input_t input;
    chb_ctrl_power_inter_t       inter;
    chb_ctrl_power_output_t      output;
} chb_ctrl_power_t;

/**
 * @brief 无功规划模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_reactive
{
    const chb_ctrl_reactive_input_t input;
    chb_ctrl_reactive_inter_t       inter;
    chb_ctrl_reactive_output_t      output;
} chb_ctrl_reactive_t;

/**
 * @brief 电流控制模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_current
{
    const chb_ctrl_current_input_t input;
    chb_ctrl_current_inter_t       inter;
    chb_ctrl_current_output_t      output;
} chb_ctrl_current_t;

/**
 * @brief 电压分配模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_voltage
{
    const chb_ctrl_voltage_input_t input;
    chb_ctrl_voltage_inter_t       inter;
    chb_ctrl_voltage_output_t      output;
} chb_ctrl_voltage_t;

/**
 * @brief PWM模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_pwm
{
    const chb_ctrl_pwm_input_t input;
    chb_pwm_plan_t             inter;
    chb_ctrl_pwm_output_t      output;
} chb_ctrl_pwm_t;

/**
 * @brief 抗饱和积分反馈模块的完整封装。
 * @details input 保存该模块的固定只读连线；inter 保存实际状态或本拍工作量；
 *          output 由该模块发布，供其他模块及诊断只读消费；模块函数接收自身实例指针。
 */
typedef struct chb_ctrl_integrator
{
    const chb_ctrl_integrator_input_t input;
    chb_ctrl_integrator_inter_t       inter;
    chb_ctrl_integrator_output_t      output;
} chb_ctrl_integrator_t;

static chb_ctrl_cfg_t cfg = {0}; /* INIT 锁定的控制系数。 */

/* 积分反馈指向后面定义的模块输出；前置声明使这些固定地址可用于静态初始化。 */
static chb_ctrl_integrator_t integrator_ctrl;

/* 简图与函数对应（物理功率级由平台/模型提供）：
 * 采样与反馈 -> chb_ctrl_update_sample / chb_ctrl_feedback
 * 母线控制   -> chb_ctrl_bus_control
 * 功率给定   -> chb_ctrl_power_reference
 * 无功规划   -> chb_ctrl_reactive_reference
 * 电流控制   -> chb_ctrl_current_loop
 * 电压分配   -> chb_ctrl_voltage_allocation
 * 谐波补偿   -> chb_ctrl_harmonic_compensation
 * PWM        -> chb_ctrl_publish_pwm；最终由 run 同步调用 HAL/BSP 下发。
 * 虚线反馈   -> chb_ctrl_update_integrators；本拍末更新，下一拍读取。
 *
 * 各模块的 input 就是连线表；固定系数只读 cfg，各模块只写自身 inter/output。
 * 母线控制内部包含参考斜坡、能量环和功率限制；电压模块内部包含限幅、均衡和分配。
 */
static chb_ctrl_feedback_t feedback_ctrl = {
    .input =
        {
            .p_theta_grid = &theta_grid,
            .p_omega_grid = &omega_grid_sampled,
            .p_v_bus_raw  = v_bus_raw,
            .p_i_load_raw = i_load_raw,
        },
};

static chb_ctrl_harmonic_t harmonic_ctrl = {
    .input =
        {
            .p_theta_grid               = &theta_grid,
            .p_v_grid_rms               = &v_grid_rms,
            .p_harmonic_feedback_weight = &harmonic_feedback_weight,
            .p_v_grid_harmonic_alpha    = v_grid_harmonic_alpha,
            .p_v_grid_harmonic_beta     = v_grid_harmonic_beta,
            .p_i_grid_harmonic_alpha    = i_grid_harmonic_alpha,
            .p_i_grid_harmonic_beta     = i_grid_harmonic_beta,
            .p_omega_grid               = &feedback_ctrl.output.omega_grid,
            .p_v_bus_total              = &feedback_ctrl.output.v_bus_total,
        },
};

static chb_ctrl_bus_t bus_ctrl = {
    .input =
        {
            .p_v_bus_filtered      = feedback_ctrl.output.v_bus_filtered,
            .p_pwr_load_filtered   = feedback_ctrl.output.pwr_load_filtered,
            .p_pwr_energy_integral = integrator_ctrl.output.pwr_energy_integral,
            .p_pwr_limit_integral  = integrator_ctrl.output.pwr_limit_integral,
        },
};

static chb_ctrl_power_t power_ctrl = {
    .input =
        {
            .p_v_grid_rms        = &v_grid_rms,
            .p_pwr_energy_ref    = bus_ctrl.output.pwr_energy_ref,
            .p_pwr_cell_upper    = bus_ctrl.output.pwr_cell_upper,
            .p_pwr_cell_capacity = bus_ctrl.output.pwr_cell_capacity,
        },
};

static chb_ctrl_reactive_t reactive_ctrl = {
    .input =
        {
            .p_v_grid_rms     = &v_grid_rms,
            .p_i_grid_d_ref   = &power_ctrl.output.i_grid_d_ref,
            .p_pwr_cell_delta = power_ctrl.output.pwr_cell_delta,
            .p_v_bus_filtered = feedback_ctrl.output.v_bus_filtered,
            .p_omega_grid     = &feedback_ctrl.output.omega_grid,
            .p_voltage_wave   = &harmonic_ctrl.output.voltage_wave,
        },
};

static chb_ctrl_current_t current_ctrl = {
    .input =
        {
            .p_i_grid_alpha_raw         = &i_grid_alpha_raw,
            .p_i_grid_beta_observed     = &i_grid_beta_observed,
            .p_v_grid_raw               = &v_grid_raw,
            .p_v_grid_fundamental_beta  = &v_grid_fundamental_beta,
            .p_harmonic_feedback_weight = &harmonic_feedback_weight,
            .p_v_grid_harmonic_alpha    = v_grid_harmonic_alpha,
            .p_cosine                   = &feedback_ctrl.output.cosine,
            .p_sine                     = &feedback_ctrl.output.sine,
            .p_current_phase_cos        = &feedback_ctrl.output.current_phase_cos,
            .p_current_phase_sin        = &feedback_ctrl.output.current_phase_sin,
            .p_omega_grid               = &feedback_ctrl.output.omega_grid,
            .p_i_grid_d_ref             = &power_ctrl.output.i_grid_d_ref,
            .p_i_grid_q_ref             = &reactive_ctrl.output.i_grid_q_ref,
            .p_v_current_d_integral     = &integrator_ctrl.output.v_current_d_integral,
            .p_v_current_q_integral     = &integrator_ctrl.output.v_current_q_integral,
        },
};

static chb_ctrl_voltage_t voltage_ctrl = {
    .input =
        {
            .p_v_current_d_raw      = &current_ctrl.output.v_bridge_d,
            .p_v_current_q_raw      = &current_ctrl.output.v_bridge_q,
            .p_v_bus_raw            = v_bus_raw,
            .p_v_bus_total          = &feedback_ctrl.output.v_bus_total,
            .p_i_direction_d        = &reactive_ctrl.output.i_direction_d,
            .p_i_direction_q        = &reactive_ctrl.output.i_direction_q,
            .p_i_grid_ref_magnitude = &reactive_ctrl.output.i_grid_ref_magnitude,
            .p_pwr_cell_delta       = power_ctrl.output.pwr_cell_delta,
            .p_voltage_wave         = &harmonic_ctrl.output.voltage_wave,
        },
};

static chb_ctrl_pwm_t pwm_ctrl = {
    .input =
        {
            .p_theta_grid            = &theta_grid,
            .p_omega_grid            = &omega_grid_sampled,
            .p_v_bus_raw             = v_bus_raw,
            .p_cosine                = &feedback_ctrl.output.cosine,
            .p_sine                  = &feedback_ctrl.output.sine,
            .p_i_grid_d_ref          = &power_ctrl.output.i_grid_d_ref,
            .p_i_grid_q_ref          = &reactive_ctrl.output.i_grid_q_ref,
            .p_v_comp_harmonic_alpha = harmonic_ctrl.output.v_comp_harmonic_alpha,
            .p_v_comp_harmonic_beta  = harmonic_ctrl.output.v_comp_harmonic_beta,
            .p_v_cell_d              = voltage_ctrl.output.v_cell_d,
            .p_v_cell_q              = voltage_ctrl.output.v_cell_q,
        },
};

static chb_ctrl_integrator_t integrator_ctrl = {
    .input =
        {
            .p_e_bus_error        = bus_ctrl.output.e_bus_error,
            .p_pwr_energy_ref     = bus_ctrl.output.pwr_energy_ref,
            .p_pwr_cell_ref       = power_ctrl.output.pwr_cell_ref,
            .p_pwr_load_error     = bus_ctrl.output.pwr_load_error,
            .p_pwr_cell_upper_raw = bus_ctrl.output.pwr_cell_upper_raw,
            .p_pwr_cell_upper     = bus_ctrl.output.pwr_cell_upper,
            .p_pwr_cell_capacity  = bus_ctrl.output.pwr_cell_capacity,
            .p_i_grid_d_ref_raw   = &power_ctrl.output.i_grid_d_ref_raw,
            .p_i_grid_d_ref       = &power_ctrl.output.i_grid_d_ref,
            .p_i_grid_q_ref       = &reactive_ctrl.output.i_grid_q_ref,
            .p_i_grid_d_error     = &current_ctrl.output.i_grid_d_error,
            .p_i_grid_q_error     = &current_ctrl.output.i_grid_q_error,
            .p_total_limited      = &voltage_ctrl.output.total_limited,
            .p_cell_limited       = &voltage_ctrl.output.cell_limited,
            .p_v_cell_d           = voltage_ctrl.output.v_cell_d,
            .p_v_cell_q           = voltage_ctrl.output.v_cell_q,
        },
};

static bool  sample_allowed = false; /* 本拍采样已完成且未被应用保护禁止。 */
static float phase_cosine[CHB_VOLTAGE_PHASE_POINTS]; /* INIT 生成的基波约束节点。 */
static float phase_sine[CHB_VOLTAGE_PHASE_POINTS];
static float harmonic_cosine[CHB_HARMONIC_COUNT][CHB_VOLTAGE_PHASE_POINTS];
static float harmonic_sine[CHB_HARMONIC_COUNT][CHB_VOLTAGE_PHASE_POINTS];

#if defined(PLATFORM_PLECS)
typedef struct chb_ctrl_diag
{
    float   v_bus_total;          /* 同拍各路母线原始电压和，V。 */
    float   v_bus_filtered_total; /* 各路陷波及低通后的母线电压和，V。 */
    float   v_bus_error;          /* 总母线外环误差，V。 */
    float   v_bus_ref_ramped;     /* 每级母线当前斜坡给定，V。 */
    float   i_grid_d_ref;         /* 限幅后的 d 轴电流参考，A 峰值。 */
    float   i_grid_q_ref;         /* 轻载均衡电流的 q 轴参考，A 峰值。 */
    float   balance_scale;        /* 差模电压共用限幅系数，0..1。 */
    float   balance_utilization;  /* 调节无功电流使用的容量利用率。 */
    float   i_grid_q_min;         /* 规划器要求的最小无功电流幅值，A。 */
    uint8_t pf_zero_feasible;     /* 零无功电流的静态可行性。 */
    float   i_grid_d;             /* d 轴电流反馈，A 峰值。 */
    float   i_grid_q;             /* q 轴电流反馈，A 峰值。 */
    float   v_pwm_d;     /* 总桥 d 轴电压指令，V。 */
    float   v_pwm_q;     /* 总桥 q 轴电压指令，V。 */
    float   v_pwm_total; /* 逆 Park 后的总桥瞬时电压指令，V。 */
    float   v_bus_balance_error[CHB_CELL_COUNT]; /* 各桥相对平均母线的误差，V。 */
    float   pwr_cell_ref[CHB_CELL_COUNT];        /* 每桥限功率后的有功请求，W。 */
    uint8_t power_limited[CHB_CELL_COUNT];       /* 功率选择器接管该桥稳压。 */
    uint8_t total_limited; /* 总桥矢量限幅标志。 */
    uint8_t cell_limited;  /* 至少一桥系数限幅标志。 */
} chb_ctrl_diag_t;

static chb_ctrl_diag_t diag = {0};   /* FRAME 只观察副本，不写控制积分状态。 */
static float           i_grid_fault; /* 首次保护闭锁时的电感电流，A。 */
static float           v_bus_fault[CHB_CELL_COUNT]; /* 首次保护闭锁时的各桥母线电压，V。 */
static float           i_grid_d_ref_fault;          /* 首次保护闭锁前一拍的 d 轴电流给定，A。 */
static float           i_grid_d_fault;     /* 首次保护闭锁前一拍的 d 轴电流反馈，A。 */
static float           v_pwm_fault;        /* 首次保护闭锁前一拍的总调制电压，V。 */
static uint8_t         fault_sample_valid; /* 首次保护采样已经锁存。 */

/* 观测诊断直接连接静态变量，不经平台复制；Shell 名称保持兼容。 */
REG_SHELL_VAR(GRID_RMS_V, v_grid_rms, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H3_PEAK_V, v_grid_harmonic_peak[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H5_PEAK_V, v_grid_harmonic_peak[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H7_PEAK_V, v_grid_harmonic_peak[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_H9_PEAK_V, v_grid_harmonic_peak[3], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H3_PEAK_A, i_grid_harmonic_peak[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H5_PEAK_A, i_grid_harmonic_peak[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H7_PEAK_A, i_grid_harmonic_peak[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_H9_PEAK_A, i_grid_harmonic_peak[3], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_HZ, f_grid, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(GRID_FREQ_RATE_HZ_S, f_grid_rate, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(HARM_FEEDBACK_WEIGHT, harmonic_feedback_weight, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(THETA_RAD, theta_grid, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(I_BETA_A, i_grid_beta_observed, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_SUM_V, diag.v_bus_total, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_FILT_V, diag.v_bus_filtered_total, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_ERR_V, diag.v_bus_error, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BUS_REF_RAMP_V, diag.v_bus_ref_ramped, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_ID_REF_A, diag.i_grid_d_ref, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_REF_A, diag.i_grid_q_ref, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_SCALE, diag.balance_scale, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_UTIL, diag.balance_utilization, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_MIN_A, diag.i_grid_q_min, SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_PF_ZERO_OK, diag.pf_zero_feasible, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_ID_A, diag.i_grid_d, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_IQ_A, diag.i_grid_q, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VD_PWM_V, diag.v_pwm_d, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VQ_PWM_V, diag.v_pwm_q, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_VPWM_V, diag.v_pwm_total, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_1, diag.v_bus_balance_error[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_2, diag.v_bus_balance_error[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_BAL_ERR_3, diag.v_bus_balance_error[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_1_W, feedback_ctrl.output.pwr_load_filtered[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_2_W, feedback_ctrl.output.pwr_load_filtered[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(CHB_LOAD_P_3_W, feedback_ctrl.output.pwr_load_filtered[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_1_W, diag.pwr_cell_ref[0], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_2_W, diag.pwr_cell_ref[1], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_REQ_3_W, diag.pwr_cell_ref[2], SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_1, diag.power_limited[0], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_2, diag.power_limited[1], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_P_LIM_3, diag.power_limited[2], SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_TOTAL_LIM, diag.total_limited, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_CELL_LIM, diag.cell_limited, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_RESERVE_PU, harmonic_ctrl.output.v_harmonic_reserve_normalized, SHELL_FP32, 1.0f, 0.0f, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_SCALE, harmonic_ctrl.output.harmonic_scale, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_HARM_FB_SCALE, harmonic_ctrl.output.harmonic_feedback_scale, SHELL_FP32, 1.0f, 0.0f, NULL,
              SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_I_A, i_grid_fault, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_1_V, v_bus_fault[0], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_2_V, v_bus_fault[1], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_BUS_3_V, v_bus_fault[2], SHELL_FP32, FLT_MAX, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_ID_REF_A, i_grid_d_ref_fault, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_ID_A, i_grid_d_fault, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_VPWM_V, v_pwm_fault, SHELL_FP32, FLT_MAX, -FLT_MAX, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(CHB_FAULT_VALID, fault_sample_valid, SHELL_UINT8, 1u, 0u, NULL, SHELL_STA_NULL)
#endif

/**
 * @brief 清除控制模块状态、输出及本拍执行许可。
 * @details 复位反馈滤波、母线斜坡、无功规划、积分历史和本拍工作区；不读取硬件或调用 PWM。
 *          仅在初始化、运行准备或停机的串行边界调用，避免与控制中断同时修改状态。
 */
static void reset_states(void)
{
    /* const input 连线保持原地址；模块历史和输出逐一复位。 */
    (void)memset(&feedback_ctrl.inter, 0, sizeof(feedback_ctrl.inter));
    (void)memset(&feedback_ctrl.output, 0, sizeof(feedback_ctrl.output));
    (void)memset(&harmonic_ctrl.inter, 0, sizeof(harmonic_ctrl.inter));
    (void)memset(&harmonic_ctrl.output, 0, sizeof(harmonic_ctrl.output));
    (void)memset(&bus_ctrl.inter, 0, sizeof(bus_ctrl.inter));
    (void)memset(&bus_ctrl.output, 0, sizeof(bus_ctrl.output));
    (void)memset(&power_ctrl.inter, 0, sizeof(power_ctrl.inter));
    (void)memset(&power_ctrl.output, 0, sizeof(power_ctrl.output));
    (void)memset(&reactive_ctrl.inter, 0, sizeof(reactive_ctrl.inter));
    (void)memset(&reactive_ctrl.output, 0, sizeof(reactive_ctrl.output));
    (void)memset(&current_ctrl.inter, 0, sizeof(current_ctrl.inter));
    (void)memset(&current_ctrl.output, 0, sizeof(current_ctrl.output));
    (void)memset(&voltage_ctrl.inter, 0, sizeof(voltage_ctrl.inter));
    (void)memset(&voltage_ctrl.output, 0, sizeof(voltage_ctrl.output));
    (void)memset(&pwm_ctrl.inter, 0, sizeof(pwm_ctrl.inter));
    (void)memset(&pwm_ctrl.output, 0, sizeof(pwm_ctrl.output));
    (void)memset(&integrator_ctrl.inter, 0, sizeof(integrator_ctrl.inter));
    (void)memset(&integrator_ctrl.output, 0, sizeof(integrator_ctrl.output));
#if defined(PLATFORM_PLECS)
    (void)memset(&diag, 0, sizeof(diag));
#endif
    sample_allowed = false;
}

/**
 * @brief 建立固定控制系数、周期约束表及反馈滤波器绑定。
 * @details 配置只复制一次；相位/谐波表由 INIT 建立，运行时只读。
 *          先清状态，再把陷波器绑定到反馈模块自身的静态工作区，不读取未绑定 ADC。
 */
static void chb_ctrl_init(void)
{
    cfg = *chb_cfg_get_ctrl_cfg();
    reset_states();

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float theta_grid_point = M_2PI * (float)point / (float)CHB_VOLTAGE_PHASE_POINTS;
        phase_cosine[point] = cosf(theta_grid_point);
        phase_sine[point]   = sinf(theta_grid_point);

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            const float order = (float)(2u * harmonic + 3u);
            harmonic_cosine[harmonic][point] = cosf(order * theta_grid_point);
            harmonic_sine[harmonic][point]   = sinf(order * theta_grid_point);
        }
    }
#if defined(PLATFORM_PLECS)
    i_grid_fault = 0.0f;
    (void)memset(v_bus_fault, 0, sizeof(v_bus_fault));
    i_grid_d_ref_fault = 0.0f;
    i_grid_d_fault     = 0.0f;
    v_pwm_fault        = 0.0f;
    fault_sample_valid = 0u;
#endif
    feedback_ctrl.output.current_phase_cos = cosf(M_2PI * cfg.f_grid * cfg.t_current_sample_delay);
    feedback_ctrl.output.current_phase_sin = sinf(M_2PI * cfg.f_grid * cfg.t_current_sample_delay);

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        notch_init(&feedback_ctrl.inter.bus_notch[cell],
                   M_2PI * 2.0f * cfg.f_grid,
                   M_2PI * CHB_BUS_NOTCH_BANDWIDTH_HZ,
                   cfg.t_ctrl_period,
                   &feedback_ctrl.inter.v_bus_notch_input[cell]);
        notch_init(&feedback_ctrl.inter.power_notch[cell],
                   M_2PI * 2.0f * cfg.f_grid,
                   M_2PI * CHB_BUS_NOTCH_BANDWIDTH_HZ,
                   cfg.t_ctrl_period,
                   &feedback_ctrl.inter.pwr_load_notch_input[cell]);
    }
}
REG_INIT(3, chb_ctrl_init) /* 采样及观测信号建立后初始化各控制模块。 */

/**
 * @brief 在进入 RUN 前准备无扰启动所需的控制历史。
 * @details 先关闭 PWM，再清积分；用最新预充母线/负载快照填充滤波历史。
 *          母线参考斜坡从各桥平均预充电压开始，不直接跳到目标值。
 *          由 FSM 在尚未授予运行许可的串行边界调用。
 */
void chb_ctrl_prepare_run(void)
{
    chb_hal_get_ctrl()->p_pwm_disable(); /* 重启前先确保全部桥臂关闭。 */
    chb_ctrl_init(); /* 新一轮运行不继承上次积分。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        feedback_ctrl.inter.v_bus_notch_input[cell]    = v_bus_raw[cell];
        feedback_ctrl.inter.pwr_load_notch_input[cell] = v_bus_raw[cell] * i_load_raw[cell];

        for (uint32_t history = 0u; history < 3u; ++history)
        {
            feedback_ctrl.inter.bus_notch[cell].inter.x[history]   = v_bus_raw[cell];
            feedback_ctrl.inter.bus_notch[cell].inter.y[history]   = v_bus_raw[cell];
            feedback_ctrl.inter.power_notch[cell].inter.x[history] = feedback_ctrl.inter.pwr_load_notch_input[cell];
            feedback_ctrl.inter.power_notch[cell].inter.y[history] = feedback_ctrl.inter.pwr_load_notch_input[cell];
        }
        feedback_ctrl.inter.bus_notch[cell].output.val   = v_bus_raw[cell]; /* 预充电压无扰进入滤波。 */
        feedback_ctrl.inter.v_bus_lpf_input_last[cell]   = v_bus_raw[cell]; /* LPF 输入/输出历史取同一稳态起点。 */
        feedback_ctrl.inter.v_bus_filtered[cell]         = v_bus_raw[cell];
        feedback_ctrl.inter.power_notch[cell].output.val = feedback_ctrl.inter.pwr_load_notch_input[cell];
        feedback_ctrl.inter.pwr_load_lpf_input_last[cell] = feedback_ctrl.inter.pwr_load_notch_input[cell];
        feedback_ctrl.inter.pwr_load_filtered[cell]      = feedback_ctrl.inter.pwr_load_notch_input[cell];
        feedback_ctrl.output.v_bus_filtered[cell]        = feedback_ctrl.inter.v_bus_filtered[cell];
        feedback_ctrl.output.pwr_load_filtered[cell]     = feedback_ctrl.inter.pwr_load_filtered[cell];
        bus_ctrl.inter.v_bus_ref_ramped += v_bus_raw[cell] / (float)CHB_CELL_COUNT;
    }
    bus_ctrl.output.v_bus_ref_ramped = bus_ctrl.inter.v_bus_ref_ramped;
}

/**
 * @brief 停止 PWM 并清除控制历史和本拍执行许可。
 * @details 使用 INIT 已验证的停波回调同步关闭桥臂，再清控制状态。
 *          运行许可与应用开停机请求仍由 FSM/cfg 管理，本函数不修改它们。
 */
void chb_ctrl_stop(void)
{
    chb_hal_get_ctrl()->p_pwm_disable();
    reset_states();
}

/**
 * @brief 应用保护禁止本拍控制，并保留首次故障观测证据。
 * @details 在采样之后、控制之前调用；清本拍许可，后续 run 因此不积分、不发波。
 *          应用保护负责保护判据及紧急停波；本函数不拥有阈值，不在此重新分类故障。
 *          PLECS 构建只在首次抑制时保存故障快照，避免后续调用覆盖首因。
 */
void chb_ctrl_inhibit(void)
{
#if defined(PLATFORM_PLECS)

    if (fault_sample_valid == 0u)
    {
        i_grid_fault         = i_grid_alpha_raw;
        i_grid_d_ref_fault   = diag.i_grid_d_ref;
        i_grid_d_fault       = diag.i_grid_d;
        v_pwm_fault          = diag.v_pwm_total;

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            v_bus_fault[cell] = v_bus_raw[cell];
        }
        fault_sample_valid = 1u;
    }
#endif
    sample_allowed = false; /* 应用保护已下发停波命令，本拍不再执行控制。 */
}

/**
 * @brief 执行第一阶段采样并发布本拍执行资格。
 * @details 每拍先撤销旧资格；FSM INIT 期间不解引用未完成绑定的 HAL。
 *          采样完成后发布资格，第二阶段应用保护可撤销，第三阶段控制仅消费一次。
 */
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

/**
 * @brief 生成谐波补偿的完整周期容量约束。
 * @details 把本拍谐波分量旋转到电网角度坐标，并按母线电压和归一化。
 *          同时计算曲率上界，供无功规划及电压模块检查离散节点之间的波峰。
 * @param[in,out] p_module 谐波模块实例；读取本拍缓存相位与补偿电压，发布周期波形。
 * @note inter 的谐波相位已由本拍模块入口刷新，不能复用上一拍缓存。
 */
static void FUNC_RAM voltage_wave_prepare(chb_ctrl_harmonic_t *p_module)
{
    chb_voltage_wave_t *p_wave = &p_module->output.voltage_wave;
    const float v_bus_total    = *p_module->input.p_v_bus_total;

    (void)memset(p_wave->v_harmonic_normalized, 0, sizeof(p_wave->v_harmonic_normalized));
    p_wave->v_curvature_normalized = 0.0f;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float order = (float)(2u * harmonic + 3u);
        const float cosine                      = p_module->inter.harmonic_phase_cos[harmonic];
        const float sine                        = p_module->inter.harmonic_phase_sin[harmonic];
        const float v_harmonic_alpha_normalized = p_module->output.v_comp_harmonic_alpha[harmonic] / v_bus_total;
        const float v_harmonic_beta_normalized  = p_module->output.v_comp_harmonic_beta[harmonic] / v_bus_total;
        const float v_harmonic_d_normalized = v_harmonic_alpha_normalized * cosine + v_harmonic_beta_normalized * sine;
        const float v_harmonic_q_normalized = -v_harmonic_alpha_normalized * sine + v_harmonic_beta_normalized * cosine;
        p_wave->v_curvature_normalized += order * order * hypotf(v_harmonic_d_normalized, v_harmonic_q_normalized);

        for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
        {
            p_wave->v_harmonic_normalized[point] += v_harmonic_d_normalized * harmonic_cosine[harmonic][point]
                                                  - v_harmonic_q_normalized * harmonic_sine[harmonic][point];
        }
    }
}

/**
 * @brief 把周期节点间的插值误差转换为实际电压裕量。
 * @details 由曲率上界和固定相位网格估计漏峰裕量；纯计算，不修改周期波形。
 * @param[in] p_wave 谐波模块发布的只读周期波形。
 * @param[in] v_bus_sample 待约束的母线电压，V。
 * @return 节点间需要预留的电压幅值，V。
 */
static float FUNC_RAM voltage_wave_guard(const chb_voltage_wave_t *p_wave, float v_bus_sample)
{
    const float theta_phase_step = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;
    return (3.0f * cfg.modulation_limit + p_wave->v_curvature_normalized) * v_bus_sample * theta_phase_step
         * theta_phase_step * 0.125f;
}

/**
 * @brief 估计完整电网前馈周期的峰值容量需求。
 * @details 合成基波与本拍可信谐波，并加入离散网格曲率裕量。
 *          谐波电流反馈只能使用扣除该前馈需求后剩余的调制容量。
 * @param[in] p_module 谐波模块实例；读取本拍电网端口及 inter 中已刷新的谐波相位。
 * @return 完整前馈电压的保守峰值，V。
 */
static float FUNC_RAM grid_feedforward_peak(const chb_ctrl_harmonic_t *p_module)
{
    const chb_ctrl_harmonic_input_t *p_input = &p_module->input;
    float v_harmonic_d[CHB_HARMONIC_COUNT]   = {0};
    float v_harmonic_q[CHB_HARMONIC_COUNT]   = {0};
    const float v_grid_fundamental_peak = M_SQRT2 * (*p_input->p_v_grid_rms);
    float v_wave_curvature       = v_grid_fundamental_peak;
    float v_wave_peak            = 0.0f;
    const float theta_phase_step = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float order = (float)(2u * harmonic + 3u);
        const float cosine = p_module->inter.harmonic_phase_cos[harmonic];
        const float sine   = p_module->inter.harmonic_phase_sin[harmonic];
        const float v_harmonic_alpha = (*p_input->p_harmonic_feedback_weight)
                                     * p_input->p_v_grid_harmonic_alpha[harmonic];
        const float v_harmonic_beta = (*p_input->p_harmonic_feedback_weight)
                                    * p_input->p_v_grid_harmonic_beta[harmonic];
        v_harmonic_d[harmonic] = v_harmonic_alpha * cosine + v_harmonic_beta * sine;
        v_harmonic_q[harmonic] = -v_harmonic_alpha * sine + v_harmonic_beta * cosine;
        v_wave_curvature += order * order * hypotf(v_harmonic_alpha, v_harmonic_beta);
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        float v_grid_instantaneous = v_grid_fundamental_peak * phase_cosine[point];

        for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
        {
            v_grid_instantaneous += v_harmonic_d[harmonic] * harmonic_cosine[harmonic][point]
                                  - v_harmonic_q[harmonic] * harmonic_sine[harmonic][point];
        }
        v_wave_peak = fmaxf(v_wave_peak, fabsf(v_grid_instantaneous));
    }
    return v_wave_peak + v_wave_curvature * theta_phase_step * theta_phase_step * 0.125f;
}

/**
 * @brief 求固定平行电压下允许的正交电压闭区间。
 * @details 逐相位节点结合谐波、母线调制上限与额外裕量收缩区间。
 *          正交电压方向由总正交请求约束；失败时下游必须使用不可行分支。
 * @param[in] p_wave 完整周期谐波约束。
 * @param[in] v_cell_parallel 沿参考电流方向的电压，V。
 * @param[in] unit_d 参考电流方向的 d 轴单位分量。
 * @param[in] unit_q 参考电流方向的 q 轴单位分量。
 * @param[in] v_bus_sample 该桥母线电压，V。
 * @param[in] v_capacity_reserve 附加预留电压，V。
 * @param[in] v_orthogonal_total 总正交电压请求，V；约束区间方向。
 * @param[out] p_v_orthogonal_lower 可行正交电压下界，V；仅返回 true 时可消费。
 * @param[out] p_v_orthogonal_upper 可行正交电压上界，V；仅返回 true 时可消费。
 * @return true：周期容量约束有解；false：平行请求或区间交集不可行。
 */
static bool FUNC_RAM voltage_orth_range(const chb_voltage_wave_t *p_wave,
                                        float                     v_cell_parallel,
                                        float                     unit_d,
                                        float                     unit_q,
                                        float                     v_bus_sample,
                                        float                     v_capacity_reserve,
                                        float                     v_orthogonal_total,
                                        float                    *p_v_orthogonal_lower,
                                        float                    *p_v_orthogonal_upper)
{
    const float v_cell_bound = 2.0f * cfg.modulation_limit * v_bus_sample;
    const float v_cell_limit = cfg.modulation_limit * v_bus_sample - v_capacity_reserve
                             - voltage_wave_guard(p_wave, v_bus_sample);
    *p_v_orthogonal_lower = (v_orthogonal_total >= 0.0f) ? 0.0f : -v_cell_bound;
    *p_v_orthogonal_upper = (v_orthogonal_total >= 0.0f) ? v_cell_bound : 0.0f;

    if (fabsf(v_cell_parallel) > v_cell_bound)
    {
        return false;
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float along = unit_d * phase_cosine[point] - unit_q * phase_sine[point];
        const float across = -unit_q * phase_cosine[point] - unit_d * phase_sine[point];
        const float v_wave_offset = v_cell_parallel * along + v_bus_sample * p_wave->v_harmonic_normalized[point];

        if (fabsf(across) < 1.0e-6f)
        {
            if (fabsf(v_wave_offset) > v_cell_limit)
            {
                return false;
            }
        }
        else
        {
            const float v_orthogonal_first = (-v_cell_limit - v_wave_offset) / across;
            const float v_orthogonal_second = (v_cell_limit - v_wave_offset) / across;
            *p_v_orthogonal_lower = fmaxf(*p_v_orthogonal_lower, fminf(v_orthogonal_first, v_orthogonal_second));
            *p_v_orthogonal_upper = fminf(*p_v_orthogonal_upper, fmaxf(v_orthogonal_first, v_orthogonal_second));

            if (*p_v_orthogonal_lower > *p_v_orthogonal_upper)
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 判断一个固定正交电压是否属于完整波形允许的闭区间。
 * @details 使用与区间求解相同的节点上下界，但只检查给定值是否属于每个区间。
 *          不累计无功规划用不到的区间交集；第一个不满足的节点立即返回。
 *          规划调用使用总正交请求的逐桥平均值，方向约束由该输入关系保证。
 * @param[in] p_wave 本拍完整谐波波形。
 * @param[in] v_cell_parallel 沿参考电流方向的电压，V。
 * @param[in] v_cell_orthogonal 已确定的正交电压，V；各桥平均无功给定。
 * @param[in] unit_d 参考电流的 d 轴单位方向。
 * @param[in] unit_q 参考电流的 q 轴单位方向。
 * @param[in] v_bus_sample 待验证母线电压，V；规划阶段采用滤波反馈。
 * @param[in] v_capacity_reserve 额外规划裕量，V。
 * @return true：给定值满足所有节点及初始幅值约束；false：至少一个约束失败。
 */
static bool FUNC_RAM voltage_orth_contains(const chb_voltage_wave_t *p_wave,
                                           float                     v_cell_parallel,
                                           float                     v_cell_orthogonal,
                                           float                     unit_d,
                                           float                     unit_q,
                                           float                     v_bus_sample,
                                           float                     v_capacity_reserve)
{
    const float v_cell_bound = 2.0f * cfg.modulation_limit * v_bus_sample;
    const float v_cell_limit = cfg.modulation_limit * v_bus_sample - v_capacity_reserve
                             - voltage_wave_guard(p_wave, v_bus_sample);

    if (    (fabsf(v_cell_parallel) > v_cell_bound)
         || (fabsf(v_cell_orthogonal) > v_cell_bound))
    {
        return false;
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float along = unit_d * phase_cosine[point] - unit_q * phase_sine[point];
        const float across = -unit_q * phase_cosine[point] - unit_d * phase_sine[point];
        const float v_wave_offset = v_cell_parallel * along + v_bus_sample * p_wave->v_harmonic_normalized[point];

        if (fabsf(across) < 1.0e-6f)
        {
            if (fabsf(v_wave_offset) > v_cell_limit)
            {
                return false;
            }
        }
        else
        {
            /* 保留原区间端点的除法顺序，避免把不等式变形后改变边界舍入。 */
            const float v_first = (-v_cell_limit - v_wave_offset) / across;
            const float v_second = (v_cell_limit - v_wave_offset) / across;

            if (    (v_cell_orthogonal < fminf(v_first, v_second))
                 || (v_cell_orthogonal > fmaxf(v_first, v_second)))
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief 检查负 q 候选电流能否满足各桥完整周期电压容量。
 * @details 按候选参考电流建立功率投影，检查每桥正交容量与平均承担无功的可行性。
 *          采用滤波母线进行规划；最终电压模块仍按本拍实时母线约束。
 * @param[in] p_input 无功规划输入：有功给定、差模功率、电网及母线反馈、谐波约束。
 * @param[in] i_grid_q_abs 候选无功电流幅值，A 峰值；实际采用负 q 方向。
 * @return true：候选可行；false：至少一个桥的容量或等无功要求不可行。
 */
static bool FUNC_RAM q_candidate_feasible(const chb_ctrl_reactive_input_t *p_input, float i_grid_q_abs)
{
    const float i_grid_d_ref = *p_input->p_i_grid_d_ref;        /* 有功电流峰值，A。 */
    const float v_grid_peak = M_SQRT2 * *p_input->p_v_grid_rms; /* 电网基波峰值，V。 */
    float i_grid_candidate_magnitude = hypotf(i_grid_d_ref, i_grid_q_abs);         /* 候选电流峰值，A。 */
    float i_grid_projection_divisor = i_grid_candidate_magnitude;
    DN_LMT(i_grid_projection_divisor, 1.0e-6f); /* 零电流点只用于可行性判断，分母保留正下限。 */
    float v_parallel_total = v_grid_peak * i_grid_d_ref / i_grid_projection_divisor
                           - cfg.r_grid * i_grid_candidate_magnitude;
    float v_orthogonal_total = v_grid_peak * i_grid_q_abs / i_grid_projection_divisor
                             - *p_input->p_omega_grid * cfg.l_grid * i_grid_candidate_magnitude;
    const float v_orthogonal_equal = v_orthogonal_total / (float)CHB_CELL_COUNT; /* 等无功对应的每桥正交电压。 */
    const float unit_d             = i_grid_d_ref / i_grid_projection_divisor;
    const float unit_q = -i_grid_q_abs / i_grid_projection_divisor;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float v_cell_parallel = v_parallel_total / (float)CHB_CELL_COUNT
                              + 2.0f * p_input->p_pwr_cell_delta[cell] / i_grid_projection_divisor;

        if (!voltage_orth_contains(p_input->p_voltage_wave,
                                   v_cell_parallel,
                                   v_orthogonal_equal,
                                   unit_d,
                                   unit_q,
                                   p_input->p_v_bus_filtered[cell],
                                   CHB_PF_VOLTAGE_RESERVE_V))
        {
            return false;
        }
    }
    return true;
}

/**
 * @brief 有界搜索并平滑更新最小负 q 电流给定。
 * @details 每 CHB_PF_PLAN_TICKS 拍重新扫描，并用固定次数二分细化首个可行边界。
 *          搜索未找到可行点时目标保留额定圆允许的 q 上界；不把该回退宣称为可行。
 *          每拍推进参考斜坡，再按额定电流圆和最小均衡电流约束返回给定。
 * @param[in] p_input 本拍无功规划端口，电压 V、功率 W、电流 A 峰值。
 * @param[in,out] p_inter 本模块搜索目标、分频计数、参考斜坡及零无功可行性历史。
 * @return 本拍带符号的 q 轴电流给定，A 峰值，非正方向。
 */
static float FUNC_RAM chb_q_plan_run(const chb_ctrl_reactive_input_t *p_input,
                                     chb_ctrl_reactive_inter_t       *p_inter)
{
    const float i_grid_d_ref = *p_input->p_i_grid_d_ref; /* 有功电流峰值，A。 */
    float i_grid_q_max_squared = cfg.i_grid_peak_limit * cfg.i_grid_peak_limit - i_grid_d_ref * i_grid_d_ref;
    float i_grid_q_floor_squared = CHB_BALANCE_CURRENT_MIN_PK_A * CHB_BALANCE_CURRENT_MIN_PK_A
                                   - i_grid_d_ref * i_grid_d_ref;
    DN_LMT(i_grid_q_max_squared, 0.0f);
    DN_LMT(i_grid_q_floor_squared, 0.0f);
    const float i_grid_q_max = sqrtf(i_grid_q_max_squared);
    const float i_grid_q_floor = sqrtf(i_grid_q_floor_squared);

    if (p_inter->ticks == 0u)
    {
        p_inter->zero_feasible   = q_candidate_feasible(p_input, 0.0f);
        p_inter->i_grid_q_target = i_grid_q_floor;

        /* 下界为零时直接复用同拍的零无功判定，避免重复扫描完整周期。 */
        const bool floor_feasible = (i_grid_q_floor == 0.0f)
                                      ? p_inter->zero_feasible
                                      : q_candidate_feasible(p_input, i_grid_q_floor);

        if (!floor_feasible)
        {
            float i_grid_q_lower     = i_grid_q_floor; /* 已知不可行的无功幅值下界，A。 */
            p_inter->i_grid_q_target = i_grid_q_max;

            for (uint32_t scan = 1u; scan <= CHB_PF_SCAN_STEPS; ++scan)
            {
                float i_grid_q_upper = i_grid_q_floor
                                     + (i_grid_q_max - i_grid_q_floor) * (float)scan / (float)CHB_PF_SCAN_STEPS;

                if (q_candidate_feasible(p_input, i_grid_q_upper))
                {
                    for (uint32_t iteration = 0u; iteration < CHB_PF_REFINE_STEPS; ++iteration)
                    {
                        float i_grid_q_candidate = 0.5f * (i_grid_q_lower + i_grid_q_upper); /* 二分候选无功幅值。 */

                        if (q_candidate_feasible(p_input, i_grid_q_candidate))
                        {
                            i_grid_q_upper = i_grid_q_candidate;
                        }
                        else
                        {
                            i_grid_q_lower = i_grid_q_candidate;
                        }
                    }
                    p_inter->i_grid_q_target = i_grid_q_upper;
                    break;
                }
                i_grid_q_lower = i_grid_q_upper;
            }
        }
    }
    p_inter->ticks = (p_inter->ticks + 1u) % CHB_PF_PLAN_TICKS;
    const float i_grid_q_step_limit = CHB_PF_Q_SLEW_A_PER_S * cfg.t_ctrl_period;
    float i_grid_q_step = p_inter->i_grid_q_target - p_inter->i_grid_q_ramped;
    UP_DN_LMT(i_grid_q_step, i_grid_q_step_limit, -i_grid_q_step_limit);
    p_inter->i_grid_q_ramped += i_grid_q_step;
    float i_grid_q_ref = p_inter->i_grid_q_ramped;
    DN_LMT(i_grid_q_ref, i_grid_q_floor);
    UP_LMT(i_grid_q_ref, i_grid_q_max); /* 额定电流圆上限最终优先于均衡所需下限。 */
    return -i_grid_q_ref;
}

/**
 * @brief 复用同采样周期、同带宽陷波器的本拍系数。
 * @details 反馈模块在 INIT 中给所有陷波器配置相同周期和带宽。
 *          每拍只更新一个实例的频率系数，其余实例复用；各自 x/y 历史及输入绑定独立。
 * @param[out] p_target 本模块另一个陷波器，只更新中心频率及五个系数字段。
 * @param[in] p_source 本拍已调频的基准陷波器。
 */
static inline void chb_ctrl_copy_notch_coefficients(notch_t *p_target, const notch_t *p_source)
{
    p_target->cfg.w0   = p_source->cfg.w0;
    p_target->inter.c0 = p_source->inter.c0;
    p_target->inter.c1 = p_source->inter.c1;
    p_target->inter.c2 = p_source->inter.c2;
    p_target->inter.d1 = p_source->inter.d1;
    p_target->inter.d2 = p_source->inter.d2;
}

/**
 * @brief 采样与反馈模块的控制入口：发布相位与平滑母线/负载反馈。
 * @details 读取同拍观测端口，各桥先二倍频陷波，再调用 my_math.h 的 LPF，避免反馈历史互串。
 *          LPF 按双线性离散的一阶低通处理，独立保存上一拍输入与输出；截止频率转换为 rad/s。
 *          滤波值供母线控制和无功规划；实时母线单独求和供谐波预算和逐拍电压容量约束。
 *          在保护通过后的控制阶段执行，不移动前置采样/保护的注册顺序。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：同拍母线电压 V、负载电流 A、电网角度 rad 与频率 Hz。
 * @note inter：反馈模块独占的陷波绑定源、陷波状态及各路 LPF 输入/输出历史。
 * @note output：平滑母线 V、负载功率 W、实时母线和 V、角频率及变换系数。
 */
static void FUNC_RAM chb_ctrl_feedback(chb_ctrl_feedback_t *p_module)
{
    const float theta_grid_observed = *p_module->input.p_theta_grid;
    const float t_ctrl_period = cfg.t_ctrl_period;
    const float omega_bus_filter = M_2PI * cfg.f_bus_filter; /* LPF 的 wc 参数使用角频率，rad/s。 */

    p_module->output.v_bus_total = 0.0f;
    /* 电压控制采用本拍电网相角；电流反馈另按已知采样延迟旋转到对应时刻。 */
    p_module->output.cosine = cosf(theta_grid_observed);
    p_module->output.sine   = sinf(theta_grid_observed);
    p_module->output.omega_grid = *p_module->input.p_omega_grid;
    p_module->output.current_phase_cos = cosf(p_module->output.omega_grid * cfg.t_current_sample_delay);
    p_module->output.current_phase_sin = sinf(p_module->output.omega_grid * cfg.t_current_sample_delay);
    notch_update_freq(&p_module->inter.bus_notch[0], 2.0f * p_module->output.omega_grid);

    /* 各桥独立保留陷波和低通状态，避免某一级的母线或负载扰动串入其他级。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 电压和功率陷波器均跟踪 2 倍电网频率，抑制单相二倍频纹波。 */
        p_module->inter.v_bus_notch_input[cell] = p_module->input.p_v_bus_raw[cell];
        chb_ctrl_copy_notch_coefficients(&p_module->inter.bus_notch[cell], &p_module->inter.bus_notch[0]);
        chb_ctrl_copy_notch_coefficients(&p_module->inter.power_notch[cell], &p_module->inter.bus_notch[0]);
        notch_cal(&p_module->inter.bus_notch[cell]);
        /* 陷波后的电压再低通，作为各桥能量环和功率容量的平滑反馈。 */
        LPF(p_module->inter.bus_notch[cell].output.val,
            p_module->inter.v_bus_lpf_input_last[cell],
            p_module->inter.v_bus_filtered[cell], t_ctrl_period, omega_bus_filter);
        /* 同拍电压与负载电流相乘得到瞬时负载功率，再提取其慢变化部分。 */
        p_module->inter.pwr_load_notch_input[cell] = p_module->input.p_v_bus_raw[cell]
                                                   * p_module->input.p_i_load_raw[cell];
        notch_cal(&p_module->inter.power_notch[cell]);
        LPF(p_module->inter.power_notch[cell].output.val,
            p_module->inter.pwr_load_lpf_input_last[cell],
            p_module->inter.pwr_load_filtered[cell], t_ctrl_period, omega_bus_filter);
        /* 总桥电压约束使用实时母线电压，不使用已滤波的能量环反馈。 */
        p_module->output.v_bus_total += p_module->input.p_v_bus_raw[cell];
        p_module->output.v_bus_filtered[cell]    = p_module->inter.v_bus_filtered[cell];
        p_module->output.pwr_load_filtered[cell] = p_module->inter.pwr_load_filtered[cell];
    }
}

/**
 * @brief 谐波补偿模块：发布补偿电压和下游容量约束。
 * @details 把电流谐波相位校正到电压采样时刻，叠加电网谐波前馈与选频电流反馈。
 *          先保证完整电网前馈容量，再限制反馈强度及统一谐波幅值。
 *          补偿电压直接送 PWM；周期波形送无功规划/电压分配，保持同一谐波预算。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：只读电网/电流谐波、稳频可信度、相角、角频率和实时母线和。
 * @note inter：本拍谐波相位缓存及电流反馈峰值预算；入口刷新相位，预算从零累计。
 * @note output：谐波 αβ 电压 V、归一化完整波形/曲率和容量缩放诊断。
 */
static void FUNC_RAM chb_ctrl_harmonic_compensation(chb_ctrl_harmonic_t *p_module)
{
    const float v_bus_total              = *p_module->input.p_v_bus_total;
    const float feedback_weight = *p_module->input.p_harmonic_feedback_weight;

    float v_harmonic_peak_normalized = 0.0f; /* 含相位的谐波合成峰值上界，每伏母线。 */
    const float theta_phase_step     = M_2PI / (float)CHB_VOLTAGE_PHASE_POINTS;
    const float interpolation_weight = theta_phase_step * theta_phase_step * 0.125f;

    /* 同一拍的同一谐波相角只求一次三角函数，两个容量计算直接消费相同结果。 */

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float order = (float)(2u * harmonic + 3u);
        const float theta_harmonic = order * (*p_module->input.p_theta_grid);
        p_module->inter.harmonic_phase_cos[harmonic] = cosf(theta_harmonic);
        p_module->inter.harmonic_phase_sin[harmonic] = sinf(theta_harmonic);
    }
    const float v_feedforward_peak  = grid_feedforward_peak(p_module); /* 完整电网前馈周期峰值，V。 */
    p_module->inter.v_feedback_peak = 0.0f; /* 电流反馈附加电压预算，V。 */

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        const float theta_harmonic_delay = (float)(2u * harmonic + 3u) * (*p_module->input.p_omega_grid)
                                         * cfg.t_current_sample_delay; /* 恢复到电压采样时刻。 */
        const float cos_delay = cosf(theta_harmonic_delay);            /* 该次谐波的采样相位余弦。 */
        const float sin_delay = sinf(theta_harmonic_delay);            /* 该次谐波的采样相位正弦。 */
        const float i_grid_harmonic_alpha_corrected = p_module->input.p_i_grid_harmonic_alpha[harmonic] * cos_delay
                                                    - p_module->input.p_i_grid_harmonic_beta[harmonic] * sin_delay;
        const float i_grid_harmonic_beta_corrected = p_module->input.p_i_grid_harmonic_alpha[harmonic] * sin_delay
                                                   + p_module->input.p_i_grid_harmonic_beta[harmonic] * cos_delay;
        /* 整流方向下，提高同相桥电压使对应谐波电流下降。 */
        p_module->output.v_comp_harmonic_alpha[harmonic] = CHB_HARMONIC_CURRENT_GAIN * i_grid_harmonic_alpha_corrected;
        p_module->output.v_comp_harmonic_beta[harmonic]  = CHB_HARMONIC_CURRENT_GAIN * i_grid_harmonic_beta_corrected;
        p_module->inter.v_feedback_peak += hypotf(p_module->output.v_comp_harmonic_alpha[harmonic],
                                                  p_module->output.v_comp_harmonic_beta[harmonic]);
    }
    /* 启机低母线时优先建立基波电压，附加反馈只能使用其余容量。 */
    p_module->output.harmonic_feedback_scale = feedback_weight;

    if (p_module->inter.v_feedback_peak > 0.0f)
    {
        float v_feedback_available = cfg.modulation_limit * v_bus_total - v_feedforward_peak;
        DN_LMT(v_feedback_available, 0.0f);
        p_module->output.harmonic_feedback_scale = v_feedback_available / p_module->inter.v_feedback_peak;
        UP_LMT(p_module->output.harmonic_feedback_scale, feedback_weight);
    }

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        p_module->output.v_comp_harmonic_alpha[harmonic] = feedback_weight
                                                             * p_module->input.p_v_grid_harmonic_alpha[harmonic]
                                                         + p_module->output.harmonic_feedback_scale
                                                               * p_module->output.v_comp_harmonic_alpha[harmonic];
        p_module->output.v_comp_harmonic_beta[harmonic] = feedback_weight
                                                            * p_module->input.p_v_grid_harmonic_beta[harmonic]
                                                        + p_module->output.harmonic_feedback_scale
                                                              * p_module->output.v_comp_harmonic_beta[harmonic];
    }
    voltage_wave_prepare(p_module);

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        v_harmonic_peak_normalized = fmaxf(v_harmonic_peak_normalized,
                                           fabsf(p_module->output.voltage_wave.v_harmonic_normalized[point]));
    }
    v_harmonic_peak_normalized += p_module->output.voltage_wave.v_curvature_normalized * interpolation_weight;
    /* 零基波须为可行锚点；其余容量由完整波形约束分配，不按谐波幅值之和截断。 */
    p_module->output.harmonic_scale = 1.0f;

    if (v_harmonic_peak_normalized > 0.0f)
    {
        const float v_harmonic_available_normalized = cfg.modulation_limit * (1.0f - 3.0f * interpolation_weight);
        p_module->output.harmonic_scale = v_harmonic_available_normalized / v_harmonic_peak_normalized;
        UP_LMT(p_module->output.harmonic_scale, 1.0f);
    }
    p_module->output.v_harmonic_reserve_normalized = p_module->output.harmonic_scale * v_harmonic_peak_normalized;

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        p_module->output.v_comp_harmonic_alpha[harmonic] *= p_module->output.harmonic_scale;
        p_module->output.v_comp_harmonic_beta[harmonic] *= p_module->output.harmonic_scale;
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        p_module->output.voltage_wave.v_harmonic_normalized[point] *= p_module->output.harmonic_scale;
    }
    p_module->output.voltage_wave.v_curvature_normalized *= p_module->output.harmonic_scale;
}

/**
 * @brief 母线控制模块：稳定各桥母线并限制负载功率请求。
 * @details 推进母线目标斜坡，再分别计算电容能量环请求及负载功率限制上界。
 *          输入积分取上一拍抗饱和输出；本函数不提前积分，也不直接发波。
 *          请求/上界/容量送功率给定，误差和未限幅上界送本拍末抗饱和判断。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：滤波母线 V、负载功率 W、能量环及功率限制环积分 W。
 * @note inter：本模块参考斜坡历史，V；RUN 起点为平均预充母线电压。
 * @note output：本拍参考 V、能量误差 J、功率请求/限制上界/容量 W。
 */
static void FUNC_RAM chb_ctrl_bus_control(chb_ctrl_bus_t *p_module)
{
    /* 参考斜坡属于母线模块；RUN 起点由 prepare_run 按预充电压初始化。
     * 一拍最多推进 cfg.v_bus_ref_slew * cfg.t_ctrl_period，避免突然施加电压阶跃。 */
    const float v_bus_ramp_step  = cfg.v_bus_ref_slew * cfg.t_ctrl_period;
    float v_bus_ramp_error = cfg.v_bus_ref - p_module->inter.v_bus_ref_ramped;
    UP_DN_LMT(v_bus_ramp_error, v_bus_ramp_step, -v_bus_ramp_step);
    p_module->inter.v_bus_ref_ramped += v_bus_ramp_error;
    p_module->output.v_bus_ref_ramped = p_module->inter.v_bus_ref_ramped;
    const float v_bus_ref             = p_module->output.v_bus_ref_ramped;

    /* 稳压支路：把母线偏差换成电容能量误差，叠加负载功率前馈和上一拍积分。
     * 积分先只读，等后级完成限幅/分配后再由抗饱和函数决定本拍是否更新。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        p_module->output.e_bus_error[cell] = 0.5f * cfg.c_bus
                                           * (v_bus_ref * v_bus_ref
                                              - p_module->input.p_v_bus_filtered[cell]
                                                    * p_module->input.p_v_bus_filtered[cell]);
        p_module->output.pwr_energy_ref[cell] = p_module->input.p_pwr_load_filtered[cell]
                                              + cfg.k_energy_p * p_module->output.e_bus_error[cell]
                                              + p_module->input.p_pwr_energy_integral[cell];
        /* 同一桥的稳压请求和功率上界在同一次遍历完成；积分仍只读上一拍值。 */
        const float pwr_load_limit = cfg.pwr_load_limit;
        p_module->output.pwr_cell_capacity[cell] = 0.5f * cfg.modulation_limit * p_module->input.p_v_bus_filtered[cell]
                                                 * cfg.i_grid_peak_limit;
        p_module->output.pwr_load_error[cell] = pwr_load_limit - p_module->input.p_pwr_load_filtered[cell];
        p_module->output.pwr_cell_upper_raw[cell] = pwr_load_limit
                                                  + cfg.k_power_limit_p * p_module->output.pwr_load_error[cell]
                                                  + p_module->input.p_pwr_limit_integral[cell];
        p_module->output.pwr_cell_upper[cell] = p_module->output.pwr_cell_upper_raw[cell];
        DN_LMT(p_module->output.pwr_cell_upper[cell], -p_module->output.pwr_cell_capacity[cell]);
        UP_LMT(p_module->output.pwr_cell_upper[cell], p_module->output.pwr_cell_capacity[cell]);
    }
}

/**
 * @brief 功率给定模块：发布有功电流和桥间差模功率需求。
 * @details 逐桥选择母线稳压请求与功率上界中的较小者，并保留正/负物理容量限幅。
 *          功率总和换算为 d 轴峰值电流并限流；差模需求与总有功按相同比例缩放。
 *          有功给定送电流控制/无功规划，零和差模功率送无功规划/电压分配。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：母线模块请求、上界和容量 W，以及电网基波有效值 V。
 * @note inter：本拍功率求和工作量，W；每拍从零重新累计。
 * @note output：逐桥功率 W、有功电流原始/限幅给定 A 峰值、零和差模功率 W。
 */
static void FUNC_RAM chb_ctrl_power_reference(chb_ctrl_power_t *p_module)
{
    p_module->inter.pwr_cell_total = 0.0f;

    /* 逐桥选择稳压请求与负载限制上界中的较小值，并保持正/负功率容量限幅。
     * 总和决定从电网取得的有功电流；单桥偏离平均值的部分交给后级均衡。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        MIN(p_module->output.pwr_cell_ref[cell],
            p_module->input.p_pwr_energy_ref[cell], p_module->input.p_pwr_cell_upper[cell]);
        DN_LMT(p_module->output.pwr_cell_ref[cell], -p_module->input.p_pwr_cell_capacity[cell]);
        UP_LMT(p_module->output.pwr_cell_ref[cell], p_module->input.p_pwr_cell_capacity[cell]);
        p_module->inter.pwr_cell_total += p_module->output.pwr_cell_ref[cell];
    }
    /* 单相平均功率换算到 d 轴峰值电流；限幅前值保留给外环抗饱和判断。 */
    p_module->output.i_grid_d_ref_raw = 2.0f * p_module->inter.pwr_cell_total
                                      / (M_SQRT2 * (*p_module->input.p_v_grid_rms));
    p_module->output.i_grid_d_ref = p_module->output.i_grid_d_ref_raw;
    UP_DN_LMT(p_module->output.i_grid_d_ref, cfg.i_grid_peak_limit, -cfg.i_grid_peak_limit);
    const float current_scale = (fabsf(p_module->output.i_grid_d_ref_raw) > cfg.i_grid_peak_limit)
                                  ? p_module->output.i_grid_d_ref / p_module->output.i_grid_d_ref_raw
                                  : 1.0f;
    const float pwr_cell_mean = p_module->inter.pwr_cell_total / (float)CHB_CELL_COUNT;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 总有功被限流时，差模请求按同一比例缩放，且各桥差模功率之和为零。 */
        p_module->output.pwr_cell_delta[cell] = current_scale
                                              * (p_module->output.pwr_cell_ref[cell]
                                                 - pwr_cell_mean);
    }
}

/**
 * @brief 无功规划模块：发布 q 电流给定及参考电流方向。
 * @details 结合有功电流、桥间差模功率、滤波母线和谐波预算规划负 q 给定。
 *          输出 q 电流供电流控制/PWM，模长和单位方向供电压分配中的有功/无功投影。
 *          搜索分频、限斜率与额定圆约束由内部规划器实现；零无功可行性单独供诊断。
 *          搜索未找到可行点时目标回退到额定圆边界，不能把该输出当作可行性证明。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：功率模块给定及同拍电网、母线、谐波容量端口。
 * @note inter：本模块有界搜索、分频计数及无功参考斜坡历史。
 * @note output：q 电流 A 峰值、参考电流模长 A、单位方向及规划诊断。
 */
static void FUNC_RAM chb_ctrl_reactive_reference(chb_ctrl_reactive_t *p_module)
{
    const float i_grid_d_ref = *p_module->input.p_i_grid_d_ref;

    p_module->output.i_grid_q_ref = chb_q_plan_run(&p_module->input, &p_module->inter);
    const float i_grid_ref_squared = i_grid_d_ref * i_grid_d_ref
                                   + p_module->output.i_grid_q_ref * p_module->output.i_grid_q_ref;
    p_module->output.i_grid_ref_magnitude = sqrtf(i_grid_ref_squared);
    p_module->output.i_direction_d        = i_grid_d_ref / p_module->output.i_grid_ref_magnitude;
    p_module->output.i_direction_q        = p_module->output.i_grid_q_ref / p_module->output.i_grid_ref_magnitude;
    p_module->output.i_grid_q_min         = p_module->inter.i_grid_q_target;
    p_module->output.zero_feasible        = p_module->inter.zero_feasible;
}

/**
 * @brief 电流控制模块：跟踪有功/无功给定并发布原始总桥电压。
 * @details 按电流采样延迟校正 Park 角度，得到 d/q 反馈与误差。
 *          从电网 α 分量扣除独立补偿的谐波，叠加阻尼及 dq 解耦，并减去电流 PI 校正。
 *          本函数不限幅、不积分；原始请求送电压分配，误差送本拍末抗饱和积分。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：同拍电压/电流、相位变换系数、d/q 给定和上一拍积分反馈。
 * @note inter：本拍电网同相前馈工作量，V；不含跨拍积分历史。
 * @note output：d/q 电流反馈及误差 A 峰值、限幅前总桥 d/q 电压请求 V。
 */
static void FUNC_RAM chb_ctrl_current_loop(chb_ctrl_current_t *p_module)
{
    const float cosine                  = *p_module->input.p_cosine;
    const float current_phase_cos       = *p_module->input.p_current_phase_cos;
    const float current_phase_sin       = *p_module->input.p_current_phase_sin;
    const float v_grid_beta       = *p_module->input.p_v_grid_fundamental_beta;
    const float omega_grid              = *p_module->input.p_omega_grid;
    const float i_grid_alpha      = *p_module->input.p_i_grid_alpha_raw;
    const float i_grid_beta       = *p_module->input.p_i_grid_beta_observed;
    const float sine                    = *p_module->input.p_sine;

    float current_cosine = cosine * current_phase_cos + sine * current_phase_sin;
    float current_sine = sine * current_phase_cos - cosine * current_phase_sin;
    /* 旋转相角回到电流采样时刻，把延迟反馈还原为本拍工频矢量。 */
    p_module->output.i_grid_d = i_grid_alpha * current_cosine + i_grid_beta * current_sine;
    p_module->output.i_grid_q = -i_grid_alpha * current_sine + i_grid_beta * current_cosine;
    p_module->output.i_grid_d_error = (*p_module->input.p_i_grid_d_ref) - p_module->output.i_grid_d;
    p_module->output.i_grid_q_error = (*p_module->input.p_i_grid_q_ref) - p_module->output.i_grid_q;
    /* 原始电压减去已单独前馈的谐波，保留 MSOGI 的变频残差，避免 PLL 滞后成为电压扰动。 */
    p_module->inter.v_grid_base_alpha = (*p_module->input.p_v_grid_raw);

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        p_module->inter.v_grid_base_alpha -= (*p_module->input.p_harmonic_feedback_weight)
                                           * p_module->input.p_v_grid_harmonic_alpha[harmonic];
    }
    const float v_grid_d = p_module->inter.v_grid_base_alpha * cosine + v_grid_beta * sine;
    const float v_grid_q = -p_module->inter.v_grid_base_alpha * sine + v_grid_beta * cosine;
    /* 整流器网侧电流由电网与桥电压之差驱动，因此比例/积分校正从桥电压
     * 前馈中减去。电阻补偿和 dq 交叉耦合分别保留现有符号，不能照搬逆变符号。 */
    p_module->output.v_bridge_d = v_grid_d - cfg.r_grid * p_module->output.i_grid_d
                                + omega_grid * cfg.l_grid * p_module->output.i_grid_q
                                - cfg.k_current_p * p_module->output.i_grid_d_error
                                - (*p_module->input.p_v_current_d_integral);
    p_module->output.v_bridge_q = v_grid_q - cfg.r_grid * p_module->output.i_grid_q
                                - omega_grid * cfg.l_grid * p_module->output.i_grid_d
                                - cfg.k_current_p * p_module->output.i_grid_q_error
                                - (*p_module->input.p_v_current_q_integral);
}

/**
 * @brief 电压模块内部：限制总桥基波使完整波形满足容量。
 * @details 把基波缩放至实时母线允许范围，逐相位节点计入谐波与曲率漏峰裕量。
 *          原始请求由外层电压模块另外保留，本结果只作为均衡求解的可行基底。
 * @param[in,out] p_module 电压模块实例；读取原始总桥请求，发布 inter.limit。
 */
static void FUNC_RAM chb_ctrl_total_voltage_limit(chb_ctrl_voltage_t *p_module)
{
    const chb_ctrl_voltage_input_t *p_input   = &p_module->input;
    chb_ctrl_voltage_limit_output_t *p_output = &p_module->inter.limit;
    const float v_bus_total                   = *p_input->p_v_bus_total;
    const float v_bridge_d                    = *p_input->p_v_current_d_raw;
    const float v_bridge_q                    = *p_input->p_v_current_q_raw;

    const float v_bridge_limit = cfg.modulation_limit * v_bus_total
                               - voltage_wave_guard(p_input->p_voltage_wave, v_bus_total);
    const float v_bridge_magnitude = hypotf(v_bridge_d, v_bridge_q);
    float scale = 1.0f;

    if (v_bridge_magnitude > 0.0f)
    {
        scale = 2.0f * cfg.modulation_limit * v_bus_total / v_bridge_magnitude;
        UP_LMT(scale, 1.0f);
    }

    for (uint32_t point = 0u; point < CHB_VOLTAGE_PHASE_POINTS; ++point)
    {
        const float v_bridge_fundamental = v_bridge_d * phase_cosine[point] - v_bridge_q * phase_sine[point];
        const float v_bridge_harmonic        = v_bus_total * p_input->p_voltage_wave->v_harmonic_normalized[point];
        const float v_bridge_fundamental_abs = fabsf(v_bridge_fundamental);

        if (v_bridge_fundamental_abs > 1.0e-6f)
        {
            const float v_fundamental_available = v_bridge_limit
                                                - ((v_bridge_fundamental > 0.0f) ? v_bridge_harmonic
                                                                                 : -v_bridge_harmonic);
            float scale_candidate = v_fundamental_available / v_bridge_fundamental_abs;
            DN_LMT(scale_candidate, 0.0f);
            UP_LMT(scale, scale_candidate);
        }
    }
    p_output->v_bridge_d    = v_bridge_d * scale;
    p_output->v_bridge_q    = v_bridge_q * scale;
    p_output->total_limited = scale < 0.99999f;
}

/**
 * @brief 电压模块内部：求差模有功的可行缩放与分配锚点。
 * @details 按各桥实时母线容量分摊可行基波，形成各桥沿/垂直参考电流的锚点。
 *          对零和差模功率对应的平行电压做固定次数搜索，同时要求各桥正交区间总和可行。
 * @param[in,out] p_module 电压模块实例；读取可行基波及输入端口，发布 inter.balance。
 */
static void FUNC_RAM chb_ctrl_balance_capacity(chb_ctrl_voltage_t *p_module)
{
    const chb_ctrl_voltage_input_t *p_input = &p_module->input;
    chb_ctrl_balance_output_t *p_output     = &p_module->inter.balance;
    const float i_direction_d               = *p_input->p_i_direction_d;
    const float i_direction_q               = *p_input->p_i_direction_q;
    const float v_bridge_d                  = p_module->inter.limit.v_bridge_d;
    const float v_bridge_q                  = p_module->inter.limit.v_bridge_q;

    float v_anchor_orthogonal[CHB_CELL_COUNT] = {0}; /* 本模块搜索的锚点工作区，V。 */
    float v_candidate_lower[CHB_CELL_COUNT];         /* 当前候选区间；只有全部通过后才覆盖可复用结果。 */
    float v_candidate_upper[CHB_CELL_COUNT];
    const float v_parallel_total = v_bridge_d * i_direction_d + v_bridge_q * i_direction_q;
    const float v_orthogonal_total = -v_bridge_d * i_direction_q + v_bridge_q * i_direction_d;
    p_output->ranges_valid = false; /* 新一拍先撤销缓存资格，禁止复用上一拍的区间。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float capacity_fraction = p_input->p_v_bus_raw[cell] / (*p_input->p_v_bus_total);
        float v_anchor_d = capacity_fraction * v_bridge_d; /* 母线不等时也可行的限幅锚点。 */
        float v_anchor_q = capacity_fraction * v_bridge_q;

        p_output->v_anchor_parallel[cell] = v_anchor_d * i_direction_d + v_anchor_q * i_direction_q;
        v_anchor_orthogonal[cell] = -v_anchor_d * i_direction_q + v_anchor_q * i_direction_d;
        /* 单相平均功率为 0.5 * (vd * i_grid_d + vq * i_grid_q)。 */
        p_output->v_balance_parallel[cell] = v_parallel_total / (float)CHB_CELL_COUNT
                                           + 2.0f * p_input->p_pwr_cell_delta[cell] / (*p_input->p_i_grid_ref_magnitude)
                                           - p_output->v_anchor_parallel[cell];
    }
    float scale_lower = 0.0f;
    float scale_upper = 1.0f;

    /* 平行分量决定有功；正交分量在各桥间零和流转，不改变任何一级有功。 */

    for (uint32_t iteration = 0u; iteration < 24u; ++iteration)
    {
        float candidate = (iteration == 0u) ? 1.0f
                                            : 0.5f * (scale_lower + scale_upper);
        float v_orthogonal_lower_total = 0.0f;
        float v_orthogonal_upper_total = 0.0f;
        bool feasible                  = true;

        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            float v_cell_parallel = p_output->v_anchor_parallel[cell] + candidate * p_output->v_balance_parallel[cell];

            if (!voltage_orth_range(p_input->p_voltage_wave,
                                    v_cell_parallel,
                                    i_direction_d,
                                    i_direction_q,
                                    p_input->p_v_bus_raw[cell],
                                    0.0f,
                                    v_orthogonal_total,
                                    &v_candidate_lower[cell],
                                    &v_candidate_upper[cell]))
            {
                feasible = false;
                break;
            }
            v_orthogonal_lower_total += v_candidate_lower[cell] - v_anchor_orthogonal[cell];
            v_orthogonal_upper_total += v_candidate_upper[cell] - v_anchor_orthogonal[cell];
        }

        if (    feasible
             && (v_orthogonal_lower_total <= 0.0f)
             && (v_orthogonal_upper_total >= 0.0f))
        {
            scale_lower = candidate;
            (void)memcpy(p_output->v_orthogonal_lower, v_candidate_lower, sizeof(v_candidate_lower));
            (void)memcpy(p_output->v_orthogonal_upper, v_candidate_upper, sizeof(v_candidate_upper));
            p_output->ranges_valid = true; /* 与 scale_lower 对应的全部逐桥区间及总和均已通过。 */

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
    p_output->balance_scale = scale_lower;
}

/**
 * @brief 电压模块内部：分配逐桥 dq 电压并保留动态调节旁路。
 * @details 先分配缩放后的差模有功；可行时平均承担正交电压，否则在各桥可行区间内分配。
 *          再加入原始总桥请求减去可行基波的动态余量，并恢复未缩放差模请求。
 *          最终瞬时容量由 PWM 联合投影守住；本函数只写逐桥电压、容量利用率和差模受限标志。
 * @param[in,out] p_module 电压模块实例；消费自身 limit/balance 工作区并发布 output。
 */
static void FUNC_RAM chb_ctrl_cell_voltage_allocation(chb_ctrl_voltage_t *p_module)
{
    const chb_ctrl_voltage_input_t *p_input = &p_module->input;
    chb_ctrl_voltage_output_t *p_output     = &p_module->output;
    const float balance_scale               = p_module->inter.balance.balance_scale;
    const float i_direction_d               = *p_input->p_i_direction_d;
    const float i_direction_q               = *p_input->p_i_direction_q;
    const float v_bridge_d                  = p_module->inter.limit.v_bridge_d;
    const float v_bridge_q                  = p_module->inter.limit.v_bridge_q;

    p_output->balance_utilization  = 0.0f;
    float *p_v_orthogonal_lower    = p_module->inter.balance.v_orthogonal_lower;
    float *p_v_orthogonal_upper    = p_module->inter.balance.v_orthogonal_upper;
    float v_orthogonal_lower_total = 0.0f;
    float v_orthogonal_upper_total = 0.0f;
    float v_orthogonal_total = -v_bridge_d * i_direction_q
                             + v_bridge_q * i_direction_d;
    const float v_orthogonal_equal = v_orthogonal_total / (float)CHB_CELL_COUNT; /* 等无功的每桥正交电压。 */
    float orth_fraction            = 0.0f;
    bool equal_q_feasible          = true; /* 完整波形约束下是否可由每桥平均承担基波无功。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float v_cell_parallel = p_module->inter.balance.v_anchor_parallel[cell]
                              + balance_scale * p_module->inter.balance.v_balance_parallel[cell];

        /* 已接受的候选与本拍分配使用完全相同的平行请求，直接复用区间。
         * 没有候选通过时仍按原路径求解，保留零缩放回退的处理方式。 */

        if (    !p_module->inter.balance.ranges_valid
             &&!voltage_orth_range(p_input->p_voltage_wave,
                                   v_cell_parallel,
                                   i_direction_d,
                                   i_direction_q,
                                   p_input->p_v_bus_raw[cell],
                                   0.0f,
                                   v_orthogonal_total,
                                   &p_v_orthogonal_lower[cell],
                                   &p_v_orthogonal_upper[cell]))
        {
            equal_q_feasible = false;
        }
        v_orthogonal_lower_total += p_v_orthogonal_lower[cell];
        v_orthogonal_upper_total += p_v_orthogonal_upper[cell];

        if (    (v_orthogonal_equal < p_v_orthogonal_lower[cell])  /* 某桥所需无功低于电压可行区间。 */
             || (v_orthogonal_equal > p_v_orthogonal_upper[cell])) /* 某桥所需无功高于电压可行区间。 */
        {
            equal_q_feasible = false;
        }
    }
    /* 等正交电压即等无功；受各桥电压容量限制时退回可行区间分配。 */

    if (v_orthogonal_upper_total > v_orthogonal_lower_total)
    {
        orth_fraction = (v_orthogonal_total - v_orthogonal_lower_total)
                        / (v_orthogonal_upper_total - v_orthogonal_lower_total);
        UP_DN_LMT(orth_fraction, 1.0f, 0.0f);
    }

    const float v_bridge_d_delta = (*p_input->p_v_current_d_raw) - v_bridge_d;
    const float v_bridge_q_delta = (*p_input->p_v_current_q_raw) - v_bridge_q;
    const float v_parallel_delta = v_bridge_d_delta * i_direction_d + v_bridge_q_delta * i_direction_q;
    const float v_orthogonal_delta = -v_bridge_d_delta * i_direction_q + v_bridge_q_delta * i_direction_d;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        float v_cell_parallel = p_module->inter.balance.v_anchor_parallel[cell]
                              + balance_scale * p_module->inter.balance.v_balance_parallel[cell];
        float v_cell_orthogonal = p_v_orthogonal_lower[cell]
                                + orth_fraction * (p_v_orthogonal_upper[cell] - p_v_orthogonal_lower[cell]);

        if (equal_q_feasible)
        {
            v_cell_orthogonal = v_orthogonal_equal;
        }

        if (p_v_orthogonal_upper[cell] > p_v_orthogonal_lower[cell])
        {
            p_output->balance_utilization = fmaxf(
                p_output->balance_utilization,
                fabsf((v_cell_orthogonal - p_v_orthogonal_lower[cell])
                      / (p_v_orthogonal_upper[cell] - p_v_orthogonal_lower[cell])));
        }

        p_output->v_cell_d[cell] = v_cell_parallel * i_direction_d - v_cell_orthogonal * i_direction_q;
        p_output->v_cell_q[cell] = v_cell_parallel * i_direction_q + v_cell_orthogonal * i_direction_d;
        /* 同一桥的基础分配和动态余量在一个循环完成，保持每桥原来的加法顺序。 */
        const float share = p_input->p_v_bus_raw[cell] / (*p_input->p_v_bus_total);
        /* 电流调节余量的无功分量平均给各桥，有功分量沿母线容量分配。 */
        p_output->v_cell_d[cell] += share * v_parallel_delta * i_direction_d
                                  - v_orthogonal_delta * i_direction_q / (float)CHB_CELL_COUNT;
        p_output->v_cell_q[cell] += share * v_parallel_delta * i_direction_q
                                  + v_orthogonal_delta * i_direction_d / (float)CHB_CELL_COUNT;
        /* 保留差模有功请求，逐桥波形容量由发波时的各桥联合投影约束。 */
        p_output->v_cell_d[cell] += (1.0f - balance_scale) * p_module->inter.balance.v_balance_parallel[cell]
                                  * i_direction_d;
        p_output->v_cell_q[cell] += (1.0f - balance_scale) * p_module->inter.balance.v_balance_parallel[cell]
                                  * i_direction_q;
    }
    p_output->cell_limited = balance_scale < 0.99999f;
}

/**
 * @brief 电压分配模块：完成总电压限幅、桥间均衡与逐桥分配。
 * @details 内部依次执行完整波形限幅、差模容量求解和逐桥 dq 分配，外部只看统一端口。
 *          模块保留原始请求到最终分配的动态旁路；最终逐桥瞬时削顶由 PWM 模块处理。
 *          逐桥 dq 电压送 PWM 与外环可达功率估计；总桥/差模限幅标志送抗饱和反馈。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：原始总桥请求 V、实时母线 V、参考电流方向/模长、差模功率 W、谐波约束。
 * @note inter：本模块静态求解工作区，持有本拍限幅及均衡中间结果，不对外发布。
 * @note output：逐桥 d/q 电压 V、总桥/差模限幅标志及均衡缩放/利用率诊断。
 */
static void FUNC_RAM chb_ctrl_voltage_allocation(chb_ctrl_voltage_t *p_module)
{
    chb_ctrl_total_voltage_limit(p_module);     /* 按完整周期波形限制基波总请求。 */
    chb_ctrl_balance_capacity(p_module);        /* 求差模均衡可行缩放及各桥电压区间。 */
    chb_ctrl_cell_voltage_allocation(p_module); /* 分配逐桥 dq 电压并保留原有动态旁路。 */
    p_module->output.total_limited = p_module->inter.limit.total_limited;
    p_module->output.balance_scale = p_module->inter.balance.balance_scale;
}

/**
 * @brief PWM 内部：在同一作用时刻联合约束各桥瞬时电压。
 * @details 预测作用相角并合成各桥基波/谐波；先逐桥限幅，再按剩余调制裕量分配总电压残差。
 *          同一作用时刻的基波 sin/cos 在桥循环前各计算一次；只读取冻结的本拍计划。
 *          不改变分配模块 dq 输出，也不直接调用硬件。
 * @param[in] p_plan 本拍各桥母线、dq 电压、谐波及电网相角计划。
 * @param[in] theta_pwm_advance 从采样时刻到作用时刻的电角度增量，rad。
 * @param[out] p_v_pwm_projected 该作用时刻所有桥的可发波电压，V，长度 CHB_CELL_COUNT。
 */
static void FUNC_RAM chb_ctrl_project_pwm_voltage(const chb_pwm_plan_t *p_plan,
                                                  float                 theta_pwm_advance,
                                                  float                 p_v_pwm_projected[CHB_CELL_COUNT])
{
    const float theta_pwm_action = p_plan->theta_grid + theta_pwm_advance;
    const float pwm_action_cos   = cosf(theta_pwm_action); /* 所有桥投影到同一个作用时刻。 */
    const float pwm_action_sin   = sinf(theta_pwm_action);
    float v_comp_harmonic        = 0.0f;
    float v_pwm_requested_total  = 0.0f;
    float v_pwm_applied_total    = 0.0f;
    float v_pwm_headroom_total   = 0.0f;

    for (uint32_t harmonic_index = 0u; harmonic_index < CHB_HARMONIC_COUNT; ++harmonic_index)
    {
        const float theta_harmonic_advance = (float)(2u * harmonic_index + 3u) * theta_pwm_advance;
        v_comp_harmonic += p_plan->v_comp_harmonic_alpha[harmonic_index] * cosf(theta_harmonic_advance)
                         - p_plan->v_comp_harmonic_beta[harmonic_index] * sinf(theta_harmonic_advance);
    }

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float v_cell_bound = p_plan->v_cell_limit[cell];
        const float v_cell_requested = p_plan->v_cell_d[cell] * pwm_action_cos
                                     - p_plan->v_cell_q[cell] * pwm_action_sin
                                     + p_plan->v_bus_raw[cell] * v_comp_harmonic / p_plan->v_bus_total;
        v_pwm_requested_total += v_cell_requested;
        p_v_pwm_projected[cell] = v_cell_requested;
        UP_DN_LMT(p_v_pwm_projected[cell], v_cell_bound, -v_cell_bound);
        v_pwm_applied_total += p_v_pwm_projected[cell];
    }

    UP_DN_LMT(v_pwm_requested_total, p_plan->v_pwm_capacity_total, -p_plan->v_pwm_capacity_total);
    const float v_pwm_residual = v_pwm_requested_total - v_pwm_applied_total;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        v_pwm_headroom_total += p_plan->v_cell_limit[cell]
                              - ((v_pwm_residual >= 0.0f) ? p_v_pwm_projected[cell] : -p_v_pwm_projected[cell]);
    }

    if (v_pwm_headroom_total > 0.0f)
    {
        for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
        {
            const float v_cell_headroom = p_plan->v_cell_limit[cell]
                                        - ((v_pwm_residual >= 0.0f) ? p_v_pwm_projected[cell]
                                                                    : -p_v_pwm_projected[cell]);
            p_v_pwm_projected[cell] += v_pwm_residual * v_cell_headroom / v_pwm_headroom_total;
        }
    }
}

/**
 * @brief PWM 内部：按各桥实际更新延迟选择投影电压及死区方向。
 * @details 延迟包括既有 BSP/PWM 基础延迟及各桥载波相移，避免给所有桥套用同一作用角。
 *          每个作用时刻先联合投影所有桥，再取当前桥的电压；方向由预测参考电流决定。
 * @param[in] p_plan 同拍静态发波计划；电压 V、电流 A、角度 rad、频率 Hz。
 * @param[out] p_v_pwm_ref 逐桥最终发波电压数组，V，长度 CHB_CELL_COUNT。
 * @param[out] p_deadtime_flag 逐桥预测电流正/负/零方向，长度 CHB_CELL_COUNT。
 */
static void FUNC_RAM chb_ctrl_prepare_pwm_voltage(const chb_pwm_plan_t   *p_plan,
                                                  float                   p_v_pwm_ref[CHB_CELL_COUNT],
                                                  chb_pwm_deadtime_flag_t p_deadtime_flag[CHB_CELL_COUNT])
{
    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        const float delay_periods = CHB_PWM_BASE_DELAY_PERIODS
                                  + (float)cell / (2.0f * (float)CHB_CELL_COUNT);
        const float theta_pwm_advance = p_plan->omega_grid * cfg.t_ctrl_period * delay_periods;
        float v_pwm_projected[CHB_CELL_COUNT] = {0.0f};
        const float i_comp_predicted = p_plan->i_comp_alpha_ref * cosf(theta_pwm_advance)
                                     - p_plan->i_comp_beta_ref * sinf(theta_pwm_advance);

        chb_ctrl_project_pwm_voltage(p_plan, theta_pwm_advance, v_pwm_projected);
        p_v_pwm_ref[cell] = v_pwm_projected[cell];

        if (i_comp_predicted > 0.0f)
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_POSITIVE;
        }
        else if (i_comp_predicted < 0.0f)
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_NEGATIVE;
        }
        else
        {
            p_deadtime_flag[cell] = CHB_PWM_DEADTIME_OFF;
        }
    }
}

/**
 * @brief PWM 模块：合成逐桥电压并发布同步发波命令。
 * @details 冻结分配结果、谐波、实时母线和相角，预测桥电压作用时刻并联合投影。
 *          输出由 run 使用同拍母线交给 HAL/BSP；占空比及载波寄存器属于接口/平台层。
 *          本模块不再次采样，不修改任何外环或电流积分历史。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：电压分配/谐波输出、d/q 电流给定及同拍母线/相位信息。
 * @note inter：本模块静态工作计划，每拍覆盖；母线和、逐桥上限及总容量只累计一次。
 * @note output：各桥最终电压 V、死区方向及总瞬时电压 V。
 */
static void FUNC_RAM chb_ctrl_publish_pwm(chb_ctrl_pwm_t *p_module)
{
    const float cosine       = *p_module->input.p_cosine;
    const float i_grid_d_ref = *p_module->input.p_i_grid_d_ref;
    const float i_grid_q_ref = *p_module->input.p_i_grid_q_ref;
    const float sine         = *p_module->input.p_sine;

    p_module->inter.i_comp_alpha_ref = i_grid_d_ref * cosine
                                     - i_grid_q_ref * sine;
    p_module->inter.i_comp_beta_ref = i_grid_d_ref * sine
                                    + i_grid_q_ref * cosine;
    p_module->inter.theta_grid           = *p_module->input.p_theta_grid;
    p_module->inter.omega_grid           = *p_module->input.p_omega_grid;
    p_module->inter.v_bus_total          = 0.0f;
    p_module->inter.v_pwm_capacity_total = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        p_module->inter.v_bus_raw[cell] = p_module->input.p_v_bus_raw[cell];
        p_module->inter.v_cell_d[cell]  = p_module->input.p_v_cell_d[cell];
        p_module->inter.v_cell_q[cell]  = p_module->input.p_v_cell_q[cell];
        /* 和原逐次投影采用同一桥顺序，复用结果时保留浮点累计顺序。 */
        p_module->inter.v_bus_total += p_module->inter.v_bus_raw[cell];
        p_module->inter.v_cell_limit[cell] = cfg.modulation_limit * p_module->inter.v_bus_raw[cell];
        p_module->inter.v_pwm_capacity_total += p_module->inter.v_cell_limit[cell];
    }

    for (uint32_t harmonic = 0u; harmonic < CHB_HARMONIC_COUNT; ++harmonic)
    {
        p_module->inter.v_comp_harmonic_alpha[harmonic] = p_module->input.p_v_comp_harmonic_alpha[harmonic];
        p_module->inter.v_comp_harmonic_beta[harmonic]  = p_module->input.p_v_comp_harmonic_beta[harmonic];
    }
    /* 每桥按实际作用时刻合成基波与谐波；联合投影避免某桥削顶破坏总电压。
     * 这里只发布电压和方向，HAL/BSP 在 run 中使用同拍母线同步转换并发波。 */
    chb_ctrl_prepare_pwm_voltage(&p_module->inter, p_module->output.v_pwm_ref, p_module->output.deadtime_flag);
    p_module->output.v_pwm_total = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        p_module->output.v_pwm_total += p_module->output.v_pwm_ref[cell];
    }
}

/**
 * @brief 限幅反馈支路：条件更新积分并发布下一拍反馈。
 * @details 母线能量积分同时检查功率选择器与后级可达功率；未选中的功率限制环保持积分。
 *          电流积分仅由总桥周期限幅门控，积分幅值另有上下界，保持原有控制语义。
 *          在本拍 PWM 命令计算和下发之后调用；本拍控制已结束，新积分只用于下一拍。
 * @param[in,out] p_module 本模块实例；只读 input，维护自身 inter 并发布 output。
 * @note input：各环误差、选择器/容量信号、限幅标志、逐桥 dq 电压和参考电流。
 * @note inter：该支路独占的能量/功率积分 W 及 d/q 电流积分 V。
 * @note output：本拍更新或保持后的积分反馈；下一拍母线/电流模块只读。
 */
static void FUNC_RAM chb_ctrl_update_integrators(chb_ctrl_integrator_t *p_module)
{
    const bool total_limited = *p_module->input.p_total_limited;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        /* 这是分配后 dq 电压与参考电流的功率估计，不是最终 PWM 或实测功率。
         * 用选择器差额与后级可达差额分别判断积分是否会继续推向饱和方向。 */
        float pwr_cell_achievable = 0.5f
                                  * (p_module->input.p_v_cell_d[cell] * (*p_module->input.p_i_grid_d_ref)
                                     + p_module->input.p_v_cell_q[cell] * (*p_module->input.p_i_grid_q_ref));
        float pwr_selector_gap = p_module->input.p_pwr_energy_ref[cell] - p_module->input.p_pwr_cell_ref[cell];
        float pwr_actuator_gap = p_module->input.p_pwr_cell_ref[cell] - pwr_cell_achievable;
        bool selector_allows = (fabsf(pwr_selector_gap) <= 1.0f)
                            || (pwr_selector_gap * p_module->input.p_e_bus_error[cell] < 0.0f);
        bool actuator_allows = !(    total_limited
                                  || (*p_module->input.p_cell_limited)
                                  || (fabsf((*p_module->input.p_i_grid_d_ref_raw)) > cfg.i_grid_peak_limit))
                            || (fabsf(pwr_actuator_gap) <= 1.0f)
                            || (pwr_actuator_gap * p_module->input.p_e_bus_error[cell] < 0.0f);
        /* 选择器及后级调制/电流约束均参与能量积分抗饱和。 */

        if (    selector_allows
             && actuator_allows)
        {
            p_module->inter.pwr_energy_integral[cell] += cfg.k_energy_i * cfg.t_ctrl_period
                                                       * p_module->input.p_e_bus_error[cell];
        }
        /* 未被选择的功率环保持积分，避免轻载时长期积累正功率误差。 */

        if (    (p_module->input.p_pwr_energy_ref[cell] >= p_module->input.p_pwr_cell_upper[cell])
             && (    (    (p_module->input.p_pwr_cell_upper_raw[cell] > -p_module->input.p_pwr_cell_capacity[cell])
                       && (p_module->input.p_pwr_cell_upper_raw[cell] < p_module->input.p_pwr_cell_capacity[cell]))
                  || (    (p_module->input.p_pwr_cell_upper_raw[cell] >= p_module->input.p_pwr_cell_capacity[cell])
                       && (p_module->input.p_pwr_load_error[cell] < 0.0f))
                  || (    (p_module->input.p_pwr_cell_upper_raw[cell] <= -p_module->input.p_pwr_cell_capacity[cell])
                       && (p_module->input.p_pwr_load_error[cell] > 0.0f))))
        {
            p_module->inter.pwr_limit_integral[cell] += cfg.k_power_limit_i * cfg.t_ctrl_period
                                                      * p_module->input.p_pwr_load_error[cell];
        }
    }

    /* 保持原有电流积分门控：只看总桥周期限幅，不把差模受限等同于电流受限。
     * 即使允许积分，d/q 积分量本身仍各自受 cfg.v_current_integral_limit 约束。 */

    if (!total_limited)
    {
        p_module->inter.v_current_d_integral += cfg.k_current_i * cfg.t_ctrl_period
                                               * (*p_module->input.p_i_grid_d_error);
        UP_DN_LMT(p_module->inter.v_current_d_integral, cfg.v_current_integral_limit, -cfg.v_current_integral_limit);
        p_module->inter.v_current_q_integral += cfg.k_current_i * cfg.t_ctrl_period
                                               * (*p_module->input.p_i_grid_q_error);
        UP_DN_LMT(p_module->inter.v_current_q_integral, cfg.v_current_integral_limit, -cfg.v_current_integral_limit);
    }
    /* 输出端口发布本拍积分结果，功率环和电流环在下一拍读取。 */

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        p_module->output.pwr_energy_integral[cell] = p_module->inter.pwr_energy_integral[cell];
        p_module->output.pwr_limit_integral[cell]  = p_module->inter.pwr_limit_integral[cell];
    }
    p_module->output.v_current_d_integral = p_module->inter.v_current_d_integral;
    p_module->output.v_current_q_integral = p_module->inter.v_current_q_integral;
}

#if defined(PLATFORM_PLECS)
/**
 * @brief 只读各模块端口，更新 PLECS 诊断副本。
 * @details 诊断不参与控制，不回写模块输出/积分；在本拍计算完成、HAL 下发前复制。
 *          总桥 d/q 字段保留原始电流请求含义，瞬时总电压字段来自最终 PWM 输出。
 */
static void chb_ctrl_update_diag(void)
{
    float v_bus_filtered_total = 0.0f;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        v_bus_filtered_total += feedback_ctrl.output.v_bus_filtered[cell];
    }
    diag.v_bus_total          = feedback_ctrl.output.v_bus_total;
    diag.v_bus_filtered_total = v_bus_filtered_total;
    diag.v_bus_error = (float)CHB_CELL_COUNT * bus_ctrl.output.v_bus_ref_ramped - v_bus_filtered_total;
    diag.v_bus_ref_ramped    = bus_ctrl.output.v_bus_ref_ramped;
    diag.i_grid_d_ref        = power_ctrl.output.i_grid_d_ref;
    diag.i_grid_q_ref        = reactive_ctrl.output.i_grid_q_ref;
    diag.balance_scale       = voltage_ctrl.output.balance_scale;
    diag.balance_utilization = voltage_ctrl.output.balance_utilization;
    diag.i_grid_q_min        = reactive_ctrl.output.i_grid_q_min;
    diag.pf_zero_feasible    = reactive_ctrl.output.zero_feasible ? 1u : 0u;
    diag.i_grid_d            = current_ctrl.output.i_grid_d;
    diag.i_grid_q            = current_ctrl.output.i_grid_q;
    diag.v_pwm_d             = current_ctrl.output.v_bridge_d;
    diag.v_pwm_q             = current_ctrl.output.v_bridge_q;
    diag.v_pwm_total         = pwm_ctrl.output.v_pwm_total;
    diag.total_limited       = voltage_ctrl.output.total_limited ? 1u : 0u;
    diag.cell_limited        = voltage_ctrl.output.cell_limited ? 1u : 0u;

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        diag.v_bus_balance_error[cell] = v_bus_filtered_total / (float)CHB_CELL_COUNT
                                       - feedback_ctrl.output.v_bus_filtered[cell];
        diag.pwr_cell_ref[cell] = power_ctrl.output.pwr_cell_ref[cell];
        diag.power_limited[cell] = (bus_ctrl.output.pwr_energy_ref[cell] > bus_ctrl.output.pwr_cell_upper[cell]) ? 1u
                                                                                                                 : 0u;
    }
}
#endif

/**
 * @brief 按简图的信号依赖执行本拍控制并同步下发 PWM。
 * @details 先消费本拍资格并检查运行许可/停机请求；故障或停机时不执行后续模块。
 *          反馈与谐波先准备输入，主链依次执行母线控制、功率给定、无功规划、电流控制、
 *          电压分配和 PWM。计算模块只消费显式端口，最终 HAL 使用同拍母线。
 *          下发后再更新抗饱和积分，保持“前级读取上一拍积分、后级生成下一拍积分”的因果顺序。
 */
static void FUNC_RAM chb_ctrl_run(void)
{
    if (!sample_allowed)
    {
        return;
    }
    sample_allowed = false; /* 本拍不能重复积分或发波。 */

    if (    (chb_fsm_run_allowed() == 0u)      /* 读取 FSM 授予的运行许可。 */
         || (chb_cfg_get_run_request() == 0u)) /* 读取应用开停机请求。 */
    {
        chb_ctrl_stop(); /* 停止发波并清除控制历史。 */
        return;
    }

    /* 反馈和谐波支路先准备同拍信号，随后按简图执行主控制链。 */
    chb_ctrl_feedback(&feedback_ctrl); /* 更新相位系数、母线电压及负载功率反馈。 */
    chb_ctrl_harmonic_compensation(&harmonic_ctrl); /* 生成谐波补偿电压及完整波形容量约束。 */
    chb_ctrl_bus_control(&bus_ctrl);             /* 推进母线给定，计算稳压功率请求及负载功率上界。 */
    chb_ctrl_power_reference(&power_ctrl);       /* 选择各桥功率，生成有功电流及差模功率给定。 */
    chb_ctrl_reactive_reference(&reactive_ctrl); /* 规划无功电流，输出参考电流的模长与方向。 */
    chb_ctrl_current_loop(&current_ctrl);        /* 计算 dq 电流误差及原始总桥电压请求。 */
    chb_ctrl_voltage_allocation(&voltage_ctrl);  /* 完成总电压限幅、桥间均衡及各桥 dq 电压分配。 */
    chb_ctrl_publish_pwm(&pwm_ctrl);             /* 合成并联合限幅各桥发波电压，确定死区补偿方向。 */
#if defined(PLATFORM_PLECS)
    chb_ctrl_update_diag(); /* 将各模块输出复制到诊断端口。 */
#endif

    for (uint32_t cell = 0u; cell < CHB_CELL_COUNT; ++cell)
    {
        chb_hal_get_ctrl()->p_set_pwm_func[cell](pwm_ctrl.output.v_pwm_ref[cell],
                                                 pwm_ctrl.input.p_v_bus_raw[cell],
                                                 pwm_ctrl.output.deadtime_flag[cell]); /* 经已绑定 HAL/BSP 同步下发该桥 PWM 命令。 */
    }
    /* 后级限幅已经完成：此处只更新下一拍积分，不回写本拍电压/PWM。 */
    chb_ctrl_update_integrators(&integrator_ctrl); /* 按限幅及功率选择状态更新下一拍积分反馈。 */
}
REG_INTERRUPT(3, chb_ctrl_run)
