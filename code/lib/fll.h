#ifndef FLL_H
#define FLL_H

// FLL 参数配置结构体
typedef struct
{
    float gamma;      // 自适应增益 (默认: 5e-6)
    float omega_init; // 初始频率 (默认: 314 rad/s, 对应50Hz)
    float ts;         // 采样周期 (默认: 1/30kHz)
} fll_params_t;

// FLL 状态结构体
typedef struct
{
    float *p_v;
    float *p_qv;
    float *p_epsilon;
    float omega; // 当前频率估计值
    float i_err;
    fll_params_t params; // 参数配置
} fll_state_t;

// 初始化FLL模块
void fll_init(fll_state_t *fll,
              const fll_params_t *params,
              float *p_v,
              float *p_qv,
              float *p_epsilon);

// 更新FLL频率估计
void fll_cal(fll_state_t *fll);

#endif // FLL_H
