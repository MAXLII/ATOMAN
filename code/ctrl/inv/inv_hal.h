#ifndef __INV_HAL_H
#define __INV_HAL_H

#include <stdint.h>

typedef struct
{
    float *p_v_cap;
    float *p_i_l;
    float *p_v_bus;
    void (*p_set_pwm_func)(float v_pwm, float v_bus);
    void (*p_pwm_enable)(void);
    void (*p_pwm_disable)(void);
} inv_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void);
    void (*p_exit_run_func)(void);
    void (*p_inv_rly_on_func)(void);
    void (*p_inv_rly_off_func)(void);
    uint8_t *p_latched;
} inv_fsm_hal_t;

void inv_hal_hard_protect_trip(void);
void inv_hal_hard_protect_clear(void);

#endif
