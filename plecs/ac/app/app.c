#include "app.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"
#include "inv_cfg.h"
#include "pfc_fsm.h"
#include "inv_fsm.h"
#include "pfc_cfg.h"
#include "pfc_hal.h"
#include "inv_hal.h"
#include "plecs.h"
#include "section.h"
#include "timing.h"
#include "my_math.h"

#include <math.h>

typedef enum
{
    APP_MODE_IDLE = 0,
    APP_MODE_AC_CHARGE = 1,
    APP_MODE_AC_DISCHARGE = 2,
} app_mode_e;

#define APP_VBUS_PRECHARGE_ENTER_V (M_SQRT2 * 200.0f)
#define APP_VBUS_NOM_ENTER_V (380.0f)
#define APP_GRID_RMS_MIN_V (180.0f)
#define APP_GRID_RMS_MAX_V (265.0f)
#define APP_RMS_FILTER_ALPHA (0.001f)
#define APP_INV_START_VBUS_MIN_V APP_VBUS_NOM_ENTER_V

static uint8_t app_pfc_hal_bound = 0U;
static uint8_t app_inv_hal_bound = 0U;
static uint8_t app_pfc_timing_bound = 0U;
static uint8_t app_inv_timing_bound = 0U;

static float app_v_g = 0.0f;
static float app_v_cap = 0.0f;
static float app_v_bus = 0.0f;
static float app_i_l = 0.0f;
static float app_v_g_rms = 0.0f;
static float app_v_g_rms_sq = 0.0f;
static pfc_vbus_sta_e app_vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
static uint8_t app_main_rly_is_closed = 0U;

static void app_inv_rly_on(void)
{
    plecs_set_output(PLECS_OUTPUT_AC_OUT_RLY_EN, 1.0f);
}

static void app_inv_rly_off(void)
{
    plecs_set_output(PLECS_OUTPUT_AC_OUT_RLY_EN, 0.0f);
}

static void app_main_rly_on(void)
{
    app_main_rly_is_closed = 1U;
    plecs_set_output(PLECS_OUTPUT_MAIN_RLY_EN, 1.0f);
}

static void app_main_rly_off(void)
{
    app_main_rly_is_closed = 0U;
    plecs_set_output(PLECS_OUTPUT_MAIN_RLY_EN, 0.0f);
}

static void app_pwm_enable(void)
{
    bsp_pwm_enable();
}

static void app_pwm_disable(void)
{
    bsp_pwm_disable();
}

static float app_calc_duty(float v_pwm, float v_bus, float *p_offset)
{
    float vbus = v_bus;

    if ((vbus < 1.0e-6f) && (vbus > -1.0e-6f))
    {
        vbus = 1.0e-6f;
    }

    if (v_pwm > 0.0f)
    {
        *p_offset = 0.0f;
        return v_pwm / vbus;
    }

    *p_offset = 1.0f;
    return (v_pwm / vbus) + 1.0f;
}

static void app_pwm_set_bridge(float v_pwm, float v_bus)
{
    float offset = 0.0f;
    float duty = app_calc_duty(v_pwm, v_bus, &offset);

    bsp_pwm_set_duty(duty, offset, 1U, 1U, 1U, 1U);
}

static void app_update_feedback(void)
{
    app_v_g = BSP_ADC_V_G;
    app_v_cap = BSP_ADC_V_CAP;
    app_v_bus = BSP_ADC_V_BUS;
    app_i_l = BSP_ADC_I_L;

    app_v_g_rms_sq += ((app_v_g * app_v_g) - app_v_g_rms_sq) * APP_RMS_FILTER_ALPHA;
    if (app_v_g_rms_sq < 0.0f)
    {
        app_v_g_rms_sq = 0.0f;
    }
    app_v_g_rms = sqrtf(app_v_g_rms_sq);

    if (app_v_bus >= APP_VBUS_NOM_ENTER_V)
    {
        app_vbus_sta = PFC_VBUS_STA_IN_REGULATION;
    }
    else if (app_v_bus >= APP_VBUS_PRECHARGE_ENTER_V)
    {
        app_vbus_sta = PFC_VBUS_STA_AT_INPUT_PEAK;
    }
    else
    {
        app_vbus_sta = PFC_VBUS_STA_BELOW_INPUT_PEAK;
    }
}

static uint8_t app_grid_is_ok(void)
{
    return (uint8_t)((app_v_g_rms >= APP_GRID_RMS_MIN_V) &&
                     (app_v_g_rms <= APP_GRID_RMS_MAX_V));
}

static void app_bind_pfc_timing(void)
{
    pfc_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
    };

    if (app_pfc_timing_bound != 0U)
    {
        return;
    }

    pfc_cfg_set_timing(&timing);
    app_pfc_timing_bound = pfc_cfg_is_ready();
}

static void app_bind_inv_timing(void)
{
    inv_ctrl_timing_t timing = {
        .ctrl_ts = CTRL_TS,
        .ctrl_freq = CTRL_FREQ,
    };

    if (app_inv_timing_bound != 0U)
    {
        return;
    }

    inv_cfg_set_timing(&timing);
    app_inv_timing_bound = inv_cfg_is_ready();
}

static void app_update_setpoint(void)
{
    pfc_cfg_set_vbus_ref_v(PFC_CFG_DEFAULT_VBUS_REF_V);
    pfc_cfg_set_vbus_slew_vps(PFC_CFG_DEFAULT_VBUS_SLEW_VPS);
    pfc_cfg_publish_building();

    inv_cfg_set_freq_hz(INV_CFG_DEFAULT_FREQ_HZ);
    inv_cfg_set_freq_slew_hzps(INV_CFG_DEFAULT_FREQ_SLEW_HZPS);
    inv_cfg_set_rms_ref_v(INV_CFG_DEFAULT_RMS_REF_V);
    inv_cfg_set_rms_slew_vps(INV_CFG_DEFAULT_RMS_SLEW_VPS);
    inv_cfg_publish_building();
}

static void app_feedback_isr(void)
{
    app_update_feedback();
}

REG_INTERRUPT(0, app_feedback_isr)

static void app_bind_pfc_hal(void)
{
    if (app_pfc_hal_bound != 0U)
    {
        return;
    }

    pfc_hal_unlock_binding();
    pfc_hal_set_v_g_ptr(&app_v_g);
    pfc_hal_set_v_cap_ptr(&app_v_cap);
    pfc_hal_set_i_l_ptr(&app_i_l);
    pfc_hal_set_v_bus_ptr(&app_v_bus);
    pfc_hal_set_v_rms_ptr(&app_v_g_rms);
    pfc_hal_set_vbus_sta_ptr(&app_vbus_sta);
    pfc_hal_set_main_rly_is_closed_ptr(&app_main_rly_is_closed);
    pfc_hal_set_pwm_setter(app_pwm_set_bridge);
    pfc_hal_set_pwm_enable(app_pwm_enable);
    pfc_hal_set_pwm_disable(app_pwm_disable);
    pfc_hal_set_main_rly_on_func(app_main_rly_on);
    pfc_hal_set_main_rly_off_func(app_main_rly_off);

    app_pfc_hal_bound = pfc_hal_is_ready();
    if (app_pfc_hal_bound != 0U)
    {
        pfc_hal_lock_binding();
    }
}

static void app_bind_inv_hal(void)
{
    if (app_inv_hal_bound != 0U)
    {
        return;
    }

    inv_hal_unlock_binding();
    inv_hal_set_v_cap_ptr(&app_v_cap);
    inv_hal_set_i_l_ptr(&app_i_l);
    inv_hal_set_v_bus_ptr(&app_v_bus);
    inv_hal_set_pwm_setter(app_pwm_set_bridge);
    inv_hal_set_pwm_enable(app_pwm_enable);
    inv_hal_set_pwm_disable(app_pwm_disable);
    inv_hal_set_inv_rly_on_func(app_inv_rly_on);
    inv_hal_set_inv_rly_off_func(app_inv_rly_off);

    app_inv_hal_bound = inv_hal_is_ready();
    if (app_inv_hal_bound != 0U)
    {
        inv_hal_lock_binding();
    }
}

static void app_task(void)
{
    app_mode_e mode = (app_mode_e)((uint8_t)plecs_get_input(PLECS_INPUT_MODE));
    pfc_run_sta_e pfc_run_sta = pfc_fsm_get_run_sta();
    inv_run_sta_e inv_run_sta = inv_fsm_get_run_sta();

    app_update_feedback();
    app_bind_pfc_timing();
    app_bind_inv_timing();
    app_update_setpoint();

    switch (mode)
    {
    case APP_MODE_AC_CHARGE:
        app_inv_hal_bound = 0U;
        if ((app_grid_is_ok() != 0U) &&
            (pfc_run_sta == pfc_run_sta_idle))
        {
            app_bind_pfc_hal();
            pfc_fsm_set_cmd(pfc_fsm_cmd_start);
        }

        inv_fsm_set_cmd(inv_fsm_cmd_stop);
        if ((app_grid_is_ok() == 0U) &&
            (pfc_run_sta != pfc_run_sta_idle))
        {
            pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
            app_pfc_hal_bound = 0U;
        }
        break;

    case APP_MODE_AC_DISCHARGE:
        app_pfc_hal_bound = 0U;
        pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        if ((app_v_bus >= APP_INV_START_VBUS_MIN_V) &&
            (inv_run_sta == inv_run_sta_idle))
        {
            app_bind_inv_hal();
            inv_fsm_set_cmd(inv_fsm_cmd_start);
        }
        else if ((app_v_bus < APP_INV_START_VBUS_MIN_V) &&
                 (inv_run_sta != inv_run_sta_idle))
        {
            inv_fsm_set_cmd(inv_fsm_cmd_stop);
            app_inv_hal_bound = 0U;
        }
        break;

    case APP_MODE_IDLE:
    default:
        app_pfc_hal_bound = 0U;
        app_inv_hal_bound = 0U;
        pfc_fsm_set_cmd(pfc_fsm_cmd_stop);
        inv_fsm_set_cmd(inv_fsm_cmd_stop);
        break;
    }

    plecs_set_output(PLECS_OUTPUT_DBG, (float)mode);
}

REG_TASK_MS(1, app_task)
