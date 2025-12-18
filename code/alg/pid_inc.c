#include "pid_inc.h"

void pid_inc_Init(pid_inc_t *pid, float kp, float ki, float kd,
              float out_max, float out_min, float delta_max, float delta_min)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;

    pid->error = 0.0f;
    pid->error_prev1 = 0.0f;
    pid->error_prev2 = 0.0f;

    pid->output = 0.0f;
    pid->output_prev = 0.0f;

    pid->output_max = out_max;
    pid->output_min = out_min;

    pid->delta_max = delta_max;
    pid->delta_min = delta_min;
}

float pid_inc_Calculate(pid_inc_t *pid, float target, float feedback) {
    // 计算当前误差
    pid->error_prev2 = pid->error_prev1;
    pid->error_prev1 = pid->error;
    pid->error = target - feedback;
    
    // 增量式 pid_inc 公式:
    // Δu(k) = Kp*[e(k)-e(k-1)] + Ki*e(k) + Kd*[e(k)-2e(k-1)+e(k-2)]
    float delta_out = pid->Kp * (pid->error - pid->error_prev1) +
                     pid->Ki * pid->error +
                     pid->Kd * (pid->error - 2.0f * pid->error_prev1 + pid->error_prev2);
    
    // 增量输出限幅
    if (delta_out > pid->delta_max) {
        delta_out = pid->delta_max;
    } else if (delta_out < pid->delta_min) {
        delta_out = pid->delta_min;
    }
    
    // 计算当前输出
    pid->output = pid->output_prev + delta_out;
    
    // 总输出限幅
    if (pid->output > pid->output_max) {
        pid->output = pid->output_max;
    } else if (pid->output < pid->output_min) {
        pid->output = pid->output_min;
    }
    
    // 更新上一次输出值
    pid->output_prev = pid->output;
    
    return pid->output;
}

void pid_inc_SetParameters(pid_inc_t *pid, float kp, float ki, float kd) {
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
}

void pid_inc_Reset(pid_inc_t *pid) {
    pid->error = 0.0f;
    pid->error_prev1 = 0.0f;
    pid->error_prev2 = 0.0f;
    pid->output = 0.0f;
    pid->output_prev = 0.0f;
}
