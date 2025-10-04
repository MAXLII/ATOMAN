#include "fll.h"
#include "my_math.h"
#include "stddef.h"

// 默认参数配置
static const fll_params_t default_params = {
    .gamma = 150,
    .omega_init = 2.0f * M_PI * 50.0f,
    .ts = CTRL_TS,
};

// 初始化FLL状态和参数
void fll_init(fll_state_t *fll,
              const fll_params_t *params,
              float *p_v,
              float *p_qv,
              float *p_epsilon)
{
    fll->p_v = p_v;
    fll->p_qv = p_qv;
    fll->p_epsilon = p_epsilon;
    // 使用默认参数或用户自定义参数
    fll->params = (params != NULL) ? *params : default_params;

    // 初始化状态
    fll->omega = fll->params.omega_init;
}

// 更新FLL频率估计
void fll_cal(fll_state_t *fll)
{
    if ((fll->p_v == NULL) ||
        (fll->p_qv == NULL) ||
        (fll->p_epsilon == NULL))
    {
        return;
    }

    float qvv = *fll->p_v * *fll->p_v + *fll->p_qv * *fll->p_qv;
    DN_LMT(qvv, 0.001f);
    fll->i_err = fll->i_err -
                 fll->params.gamma *
                     1.414f *
                     fll->params.ts *
                     fll->omega *
                     *fll->p_epsilon *
                     *fll->p_qv *
                     (1.0f / qvv);
    UP_DN_LMT(fll->i_err, 2 * M_PI * 30, -2 * M_PI * 30);
    fll->omega = fll->i_err + fll->params.omega_init;
}
