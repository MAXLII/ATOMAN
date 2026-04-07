#include "pwm.h"

static uint8_t pwm_step = 0;
static float v_pwm_last = 0.0f;
static float v_pwm_tag = 0.0f;
static float v_pwm_act = 0.0f;
static uint8_t v_pwm_i = 0;

void pwm_enable(void)
{
    pwm_step = 0;
    v_pwm_last = 0.0f;
    v_pwm_tag = 0.0f;
    v_pwm_act = 0.0f;
    v_pwm_i = 0U;
}

static inline float pfc_calc_duty(float v_pwm, float v_bus, float *p_offset)
{
    /* 保护：防止除0；你也可以改成直接关PWM并返回 */
    const float vbus = (v_bus > 1e-6f || v_bus < -1e-6f) ? v_bus : 1e-6f;

    if (v_pwm > 0.0f)
    {
        *p_offset = 0.0f;
        return v_pwm / vbus;
    }
    else
    {
        *p_offset = 1.0f;
        return v_pwm / vbus + 1.0f;
    }
}

/* 全开 1,1,1,1 */
static inline void pfc_pwm_apply_full(float v_pwm, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm, v_bus, &offset);
    bsp_pwm_set_duty(duty, offset, 1, 1, 1, 1);
}

/* 过零斜坡阶段：只开一条支路（按你原来的配置） */
static inline void pfc_pwm_apply_ramp(float v_pwm_act, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm_act, v_bus, &offset);

    if (v_pwm_act > 0.0f)
    {
        bsp_pwm_set_duty(duty, offset, 0, 1, 0, 0);
    }
    else
    {
        bsp_pwm_set_duty(duty, offset, 1, 0, 0, 0);
    }
}

static inline uint8_t pfc_is_zero_cross(float v_now, float v_last)
{
    /* 与你原来逻辑一致：乘积<=0 判定过零（包含等于0） */
    return (v_now * v_last) <= 0.0f;
}

void pwm_set_inv(float v_pwm, float v_bus)
{
    float offset;
    float duty = pfc_calc_duty(v_pwm, v_bus, &offset);

    bsp_pwm_set_duty(duty, offset, 1, 1, 1, 1);
}

void pwm_set_pfc(float v_pwm, float v_bus)
{
    switch (pwm_step)
    {
    default:
    case 0:
        // pwm_step = 1;
        pfc_pwm_apply_full(v_pwm, v_bus);
        break;

    case 1:
        if (pfc_is_zero_cross(v_pwm, v_pwm_last))
        {
            pwm_step = 2;
            bsp_pwm_set_duty(0.0f, 0.0f, 0, 0, 0, 0);

            v_pwm_tag = v_pwm;
            v_pwm_act = 0.0f;
            v_pwm_i = 0; /* 建议：显式复位计数器，避免继承旧值 */
        }
        else
        {
            pfc_pwm_apply_full(v_pwm, v_bus);
        }
        break;

    case 2:
        if (v_pwm_i < 5)
        {
            v_pwm_i++;

            /* 线性斜坡：i/5 */
            v_pwm_act = v_pwm_tag * ((float)v_pwm_i / 5.0f);
            pfc_pwm_apply_ramp(v_pwm_act, v_bus);
        }
        else
        {
            pwm_step = 1;
            pfc_pwm_apply_full(v_pwm, v_bus);
        }
        break;
    }
    v_pwm_last = v_pwm;
}
void pwm_disable(void)
{
    bsp_pwm_disable();
}

// test
#include "section.h"
#include "shell.h"
#include "bsp_gpio.h"

#define PWM_TRIG_SET_PARAM (1U)
#define PWM_TRIG_ENABLE_OUTPUT (2U)
#define PWM_TRIG_DISABLE_OUTPUT (3U)
#define PWM_TRIG_AUTO_START (4U)
#define PWM_TRIG_CASE_BASE_DISABLE (5U)
#define PWM_TRIG_CASE_ENABLE_BLANK (6U)
#define PWM_TRIG_CASE_ALL_OFF_1 (7U)
#define PWM_TRIG_CASE_FAST_UP_LOW (8U)
#define PWM_TRIG_CASE_FAST_UP_HIGH (9U)
#define PWM_TRIG_CASE_FAST_DN_LOW (10U)
#define PWM_TRIG_CASE_FAST_BOTH_MID (11U)
#define PWM_TRIG_CASE_SLOW_UP_LOW (12U)
#define PWM_TRIG_CASE_SLOW_DN_HIGH (13U)
#define PWM_TRIG_CASE_SLOW_BOTH_MID (14U)
#define PWM_TRIG_CASE_BOTH_BOTH_MIX (15U)
#define PWM_TRIG_CASE_FAST_UP_SLOW_DN (16U)
#define PWM_TRIG_CASE_FAST_DN_SLOW_UP (17U)
#define PWM_TRIG_CASE_DISABLE_ACTIVE (18U)
#define PWM_TRIG_CASE_ENABLE_AGAIN (19U)
#define PWM_TRIG_CASE_BOTH_SWAP_DUTY (20U)
#define PWM_TRIG_CASE_ALL_OFF_2 (21U)
#define PWM_TRIG_CASE_DISABLE_FINAL (22U)
#define PWM_TRIG_CASE_PFC_START (23U)
#define PWM_TRIG_CASE_PFC_STOP (24U)
#define PWM_TRIG_CASE_FIRST (PWM_TRIG_CASE_BASE_DISABLE)
#define PWM_TRIG_CASE_AUTO_LAST (PWM_TRIG_CASE_DISABLE_FINAL)
#define PWM_TRIG_MAX (PWM_TRIG_CASE_PFC_STOP)
#define PWM_PFC_TEST_ISR_FREQ_HZ (30000.0f)

uint8_t pwm_trig = 0;
REG_SHELL_VAR(pwm_trig, pwm_trig, SHELL_UINT8, PWM_TRIG_MAX, 0, NULL, SHELL_STA_NULL)

struct
{
    float fast_duty;
    uint8_t fast_up_en;
    uint8_t fast_dn_en;
    float slow_duty;
    uint8_t slow_up_en;
    uint8_t slow_dn_en;
} pwm_param;

REG_SHELL_VAR(fast_duty, pwm_param.fast_duty, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(fast_up_en, pwm_param.fast_up_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(fast_dn_en, pwm_param.fast_dn_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(slow_duty, pwm_param.slow_duty, SHELL_FP32, 1.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(slow_up_en, pwm_param.slow_up_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(slow_dn_en, pwm_param.slow_dn_en, SHELL_UINT8, 1, 0, NULL, SHELL_STA_NULL)

static struct
{
    float vbus;
    float vpwm_peak;
    float freq_hz;
} pwm_pfc_test_param = {
    .vbus = 400.0f,
    .vpwm_peak = 311.0f,
    .freq_hz = 50.0f,
};

REG_SHELL_VAR(pfc_test_vbus, pwm_pfc_test_param.vbus, SHELL_FP32, 1000.0f, 1.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(pfc_test_vpwm_peak, pwm_pfc_test_param.vpwm_peak, SHELL_FP32, 400.0f, 0.0f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(pfc_test_freq_hz, pwm_pfc_test_param.freq_hz, SHELL_FP32, 400.0f, 0.1f, NULL, SHELL_STA_NULL)

static uint8_t pwm_auto_running = 0U;
static uint8_t pwm_auto_next_case = PWM_TRIG_CASE_FIRST;
static uint8_t pwm_auto_pending_case = 0U;
static uint8_t pwm_pfc_test_running = 0U;
static float pwm_pfc_test_phase_rad = 0.0f;

typedef enum
{
    PWM_TEST_ACTION_SET = 0,
    PWM_TEST_ACTION_ENABLE,
    PWM_TEST_ACTION_DISABLE,
    PWM_TEST_ACTION_PFC_START,
    PWM_TEST_ACTION_PFC_STOP,
} pwm_test_action_t;

typedef struct
{
    uint8_t trig;
    uint8_t action;
    float fast_duty;
    float slow_duty;
    uint8_t fast_up_en;
    uint8_t fast_dn_en;
    uint8_t slow_up_en;
    uint8_t slow_dn_en;
} pwm_test_case_t;

static const pwm_test_case_t s_pwm_test_case_table[] = {
    {PWM_TRIG_CASE_BASE_DISABLE, PWM_TEST_ACTION_DISABLE, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_ENABLE_BLANK, PWM_TEST_ACTION_ENABLE, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_ALL_OFF_1, PWM_TEST_ACTION_SET, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_FAST_UP_LOW, PWM_TEST_ACTION_SET, 0.20f, 0.0f, 1U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_FAST_UP_HIGH, PWM_TEST_ACTION_SET, 0.70f, 0.0f, 1U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_FAST_DN_LOW, PWM_TEST_ACTION_SET, 0.20f, 0.0f, 0U, 1U, 0U, 0U},
    {PWM_TRIG_CASE_FAST_BOTH_MID, PWM_TEST_ACTION_SET, 0.50f, 0.0f, 1U, 1U, 0U, 0U},
    {PWM_TRIG_CASE_SLOW_UP_LOW, PWM_TEST_ACTION_SET, 0.0f, 0.25f, 0U, 0U, 1U, 0U},
    {PWM_TRIG_CASE_SLOW_DN_HIGH, PWM_TEST_ACTION_SET, 0.0f, 0.65f, 0U, 0U, 0U, 1U},
    {PWM_TRIG_CASE_SLOW_BOTH_MID, PWM_TEST_ACTION_SET, 0.0f, 0.45f, 0U, 0U, 1U, 1U},
    {PWM_TRIG_CASE_BOTH_BOTH_MIX, PWM_TEST_ACTION_SET, 0.30f, 0.60f, 1U, 1U, 1U, 1U},
    {PWM_TRIG_CASE_FAST_UP_SLOW_DN, PWM_TEST_ACTION_SET, 0.35f, 0.55f, 1U, 0U, 0U, 1U},
    {PWM_TRIG_CASE_FAST_DN_SLOW_UP, PWM_TEST_ACTION_SET, 0.35f, 0.55f, 0U, 1U, 1U, 0U},
    {PWM_TRIG_CASE_DISABLE_ACTIVE, PWM_TEST_ACTION_DISABLE, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_ENABLE_AGAIN, PWM_TEST_ACTION_ENABLE, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_BOTH_SWAP_DUTY, PWM_TEST_ACTION_SET, 0.65f, 0.25f, 1U, 1U, 1U, 1U},
    {PWM_TRIG_CASE_ALL_OFF_2, PWM_TEST_ACTION_SET, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_DISABLE_FINAL, PWM_TEST_ACTION_DISABLE, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_PFC_START, PWM_TEST_ACTION_PFC_START, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
    {PWM_TRIG_CASE_PFC_STOP, PWM_TEST_ACTION_PFC_STOP, 0.0f, 0.0f, 0U, 0U, 0U, 0U},
};

static const pwm_test_case_t *pwm_test_case_find(uint8_t trig)
{
    uint32_t i;

    for (i = 0U; i < (sizeof(s_pwm_test_case_table) / sizeof(s_pwm_test_case_table[0])); i++)
    {
        if (s_pwm_test_case_table[i].trig == trig)
        {
            return &s_pwm_test_case_table[i];
        }
    }

    return NULL;
}

static void pwm_pfc_test_stop(uint8_t disable_output)
{
    pwm_pfc_test_running = 0U;
    pwm_pfc_test_phase_rad = 0.0f;

    if (0U != disable_output)
    {
        pwm_disable();
    }
}

static void pwm_pfc_test_start(void)
{
    pwm_enable();
    bsp_pwm_enable();
    pwm_pfc_test_running = 1U;
    pwm_pfc_test_phase_rad = 0.0f;
}

static void pwm_pfc_test_step(void)
{
    float vpwm;
    float phase_step_rad;

    if (0U == pwm_pfc_test_running)
    {
        return;
    }

    vpwm = pwm_pfc_test_param.vpwm_peak * sinf(pwm_pfc_test_phase_rad);
    pwm_set_pfc(vpwm, pwm_pfc_test_param.vbus);
    phase_step_rad = M_2PI * pwm_pfc_test_param.freq_hz / PWM_PFC_TEST_ISR_FREQ_HZ;
    pwm_pfc_test_phase_rad += phase_step_rad;

    if (pwm_pfc_test_phase_rad >= M_2PI)
    {
        pwm_pfc_test_phase_rad -= M_2PI;
    }
}

static void pwm_test_apply_case(uint8_t trig)
{
    const pwm_test_case_t *p_case = pwm_test_case_find(trig);

    if (PWM_TRIG_SET_PARAM == trig)
    {
        bsp_pwm_set_duty(pwm_param.fast_duty,
                         pwm_param.slow_duty,
                         pwm_param.fast_up_en,
                         pwm_param.fast_dn_en,
                         pwm_param.slow_up_en,
                         pwm_param.slow_dn_en);
        return;
    }

    if (PWM_TRIG_ENABLE_OUTPUT == trig)
    {
        bsp_pwm_enable();
        return;
    }

    if (PWM_TRIG_DISABLE_OUTPUT == trig)
    {
        bsp_pwm_disable();
        return;
    }

    if (((PWM_TRIG_CASE_PFC_START != trig) && (PWM_TRIG_CASE_PFC_STOP != trig)) && (0U != pwm_pfc_test_running))
    {
        pwm_pfc_test_stop(0U);
    }

    if (NULL == p_case)
    {
        return;
    }

    switch (p_case->action)
    {
    case PWM_TEST_ACTION_SET:
        bsp_pwm_set_duty(p_case->fast_duty,
                         p_case->slow_duty,
                         p_case->fast_up_en,
                         p_case->fast_dn_en,
                         p_case->slow_up_en,
                         p_case->slow_dn_en);
        break;

    case PWM_TEST_ACTION_ENABLE:
        bsp_pwm_enable();
        break;

    case PWM_TEST_ACTION_DISABLE:
        bsp_pwm_disable();
        break;

    case PWM_TEST_ACTION_PFC_START:
        pwm_pfc_test_start();
        break;

    case PWM_TEST_ACTION_PFC_STOP:
        pwm_pfc_test_stop(1U);
        break;

    default:
        break;
    }
}

static void pwm_test_auto_start(void)
{
    if (0U != pwm_pfc_test_running)
    {
        pwm_pfc_test_stop(1U);
    }

    pwm_auto_running = 1U;
    pwm_auto_next_case = PWM_TRIG_CASE_FIRST;
    pwm_auto_pending_case = 0U;
}

static void pwm_test_auto_task(void)
{

    static uint32_t cnt = 0;
    if (cnt < 1000)
    {
        cnt++;
    }
    else if (cnt == 1000)
    {
        pwm_trig = 23;
        cnt++;
        PLECS_LOG("pwm_trig = 23;\n");
    }
    else
    {
    }

    if (0U == pwm_auto_running)
    {
        return;
    }

    if (0U != pwm_auto_pending_case)
    {
        return;
    }

    if (pwm_auto_next_case <= PWM_TRIG_CASE_AUTO_LAST)
    {
        pwm_auto_pending_case = pwm_auto_next_case;
        pwm_trig = pwm_auto_next_case;
        pwm_auto_next_case++;
    }
    else
    {
        pwm_auto_running = 0U;
        pwm_auto_pending_case = 0U;
        pwm_auto_next_case = PWM_TRIG_CASE_FIRST;
        pwm_trig = 0U;
    }
}

static void pwm_test(void)
{
    uint8_t trig_exec = 0U;

    if (0U != pwm_auto_running)
    {
        if (0U != pwm_auto_pending_case)
        {
            trig_exec = pwm_auto_pending_case;
            pwm_auto_pending_case = 0U;
        }
        else if (PWM_TRIG_AUTO_START == pwm_trig)
        {
            return;
        }
    }
    else if (0U != pwm_trig)
    {
        trig_exec = pwm_trig;
        if (PWM_TRIG_AUTO_START != trig_exec)
        {
            pwm_trig = 0U;
        }
    }

    if (0U != trig_exec)
    {
        bsp_gpio_set_bit(PWM_ACT, 1);
        if (PWM_TRIG_AUTO_START == trig_exec)
        {
            pwm_test_auto_start();
        }
        else
        {
            pwm_test_apply_case(trig_exec);
        }

        if (0U != pwm_pfc_test_running)
        {
            pwm_pfc_test_step();
        }

        bsp_gpio_set_bit(PWM_ACT, 0);
    }
    else if (0U != pwm_pfc_test_running)
    {
        bsp_gpio_set_bit(PWM_ACT, 1);
        pwm_pfc_test_step();
        bsp_gpio_set_bit(PWM_ACT, 0);
    }
}

REG_TASK_MS(1, pwm_test_auto_task)
REG_INTERRUPT(5, pwm_test)
