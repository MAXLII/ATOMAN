#include "pid.h"
#include "string.h"

void pid_reset(pid_param_t *pid_param)
{
    memset(pid_param, 0, sizeof(pid_param_t));
}
float pid_cal(pid_param_t *pid_param, float ref, float act) {
    // 1. 计算误差项（减少重复访问内存）
    const float err = ref - act;
    const float err_diff = err - pid_param->inter.err_last;

    // 2. 预计算 P 和 D 项（减少重复计算）
    const float kp = pid_param->cfg.kp;
    const float ki = pid_param->cfg.ki;
    const float kd = pid_param->cfg.kd;
    const float ki_inv = pid_param->cfg.ki_inv;
    const float output_lmt_max = pid_param->cfg.output_lmt_max;
    const float output_lmt_min = pid_param->cfg.output_lmt_min;

    const float p_term = kp * err;
    const float d_term = kd * err_diff;

    // 3. 更新积分项（先不加到输出，避免后续限幅影响）
    pid_param->inter.i_err += err;

    // 4. 计算无积分限制的输出（减少分支预测影响）
    float output = p_term + (ki * pid_param->inter.i_err) + d_term;

    // 5. 输出限幅（优化分支逻辑）
    if (output > output_lmt_max) {
        output = output_lmt_max;
        // 抗饱和处理（仅在必要时计算）
        if (ki != 0.0f) {
            // 计算最大允许的积分项（提前计算常量）
            const float max_i_term = output_lmt_max - p_term - d_term;
            pid_param->inter.i_err = max_i_term * ki_inv;  // ki_inv = 1/ki
        }
    } 
    else if (output < output_lmt_min) {
        output = output_lmt_min;
        // 抗饱和处理（仅在必要时计算）
        if (ki != 0.0f) {
            // 计算最小允许的积分项（提前计算常量）
            const float min_i_term = output_lmt_min - p_term - d_term;
            pid_param->inter.i_err = min_i_term * ki_inv;  // ki_inv = 1/ki
        }
    }

    // 6. 更新历史误差（减少内存访问次数）
    pid_param->inter.err_last = err;
    pid_param->output.output = output;

    return output;
}
