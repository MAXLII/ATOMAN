#ifndef __PID_INC_H
#define __PID_INC_H

typedef struct
{
    float Kp; // 比例系数
    float Ki; // 积分系数
    float Kd; // 微分系数

    float error;       // 当前误差
    float error_prev1; // 前一次误差
    float error_prev2; // 前两次误差

    float output;      // 当前输出值
    float output_prev; // 上一次输出值

    float output_max; // 输出上限
    float output_min; // 输出下限

    float delta_max; // 增量输出上限
    float delta_min; // 增量输出下限
} pid_inc_t;

void pid_inc_Init(pid_inc_t *pid,
                  float kp,
                  float ki,
                  float kd,
                  float out_max,
                  float out_min,
                  float delta_max,
                  float delta_min);

float pid_inc_Calculate(pid_inc_t *pid, float target, float feedback);

void pid_inc_SetParameters(pid_inc_t *pid, float kp, float ki, float kd);

void pid_inc_Reset(pid_inc_t *pid);

#endif
