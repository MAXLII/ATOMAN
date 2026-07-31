/*
 * @file    pi_dual_compete.h
 * @brief   双通道 PI 竞争模块头文件.
 * @details
 *          This file is part of the PFC project.
 *
 *          Module responsibilities:
 *          - 定义双通道 PI 竞争模式和数据结构
 *          - 声明初始化、更新、计算、重置接口
 *
 * @author  Max.Li
 * @date    2026-05-20
 * @version 1.0.0
 */

#ifndef __PI_DUAL_COMPETE_H
#define __PI_DUAL_COMPETE_H

#include <stdbool.h>

typedef enum
{
    PI_DUAL_COMPETE_MIN = 0,
    PI_DUAL_COMPETE_MAX = 1
} PI_DUAL_COMPETE_MODE_E;

typedef enum
{
    PI_DUAL_CH_A = 0,
    PI_DUAL_CH_B = 1
} PI_DUAL_COMPETE_CH_E;

typedef struct
{
    float *p_ref_a;
    float *p_act_a;
    float *p_ref_b;
    float *p_act_b;
} pi_dual_compete_input_t;

typedef struct
{
    float kp;
    float ki;
} pi_dual_compete_loop_cfg_t;

typedef struct
{
    pi_dual_compete_loop_cfg_t loop_a;
    pi_dual_compete_loop_cfg_t loop_b;

    float up_lmt;
    float dn_lmt;

    PI_DUAL_COMPETE_MODE_E mode;
} pi_dual_compete_cfg_t;

typedef struct
{
    /* Shared integral term */
    float i_share;

    float err_a;
    float err_b;

    float p_a;
    float p_b;

    float out_a_raw;
    float out_b_raw;

    PI_DUAL_COMPETE_CH_E active_ch;
} pi_dual_compete_inter_t;

typedef struct
{
    float val;
    float val_a;
    float val_b;
} pi_dual_compete_output_t;

typedef struct
{
    pi_dual_compete_input_t input;
    pi_dual_compete_cfg_t cfg;
    pi_dual_compete_inter_t inter;
    pi_dual_compete_output_t output;
} pi_dual_compete_t;

bool pi_dual_compete_init(pi_dual_compete_t *p_str,
                          float kp_a,
                          float ki_a,
                          float kp_b,
                          float ki_b,
                          float up_lmt,
                          float dn_lmt,
                          PI_DUAL_COMPETE_MODE_E mode,
                          float *p_ref_a,
                          float *p_act_a,
                          float *p_ref_b,
                          float *p_act_b);

bool pi_dual_compete_update_a(pi_dual_compete_t *p_str,
                              float kp,
                              float ki);

bool pi_dual_compete_update_b(pi_dual_compete_t *p_str,
                              float kp,
                              float ki);

bool pi_dual_compete_set_mode(pi_dual_compete_t *p_str,
                              PI_DUAL_COMPETE_MODE_E mode);

bool pi_dual_compete_cal(pi_dual_compete_t *p_str);

void pi_dual_compete_reset(pi_dual_compete_t *p_str);

#endif
