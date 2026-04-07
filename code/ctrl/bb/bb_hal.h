/* bb_hal.h
 * Buck-boost controller and FSM HAL binding definitions.
 */

#ifndef __BB_HAL_H
#define __BB_HAL_H

#include <stdint.h>

typedef struct
{
    float *p_v_in;  /* p_v_in: input-voltage sample */
    float *p_i_in;  /* p_i_in: input-current sample */
    float *p_v_out; /* p_v_out: output-voltage sample */
    float *p_i_out; /* p_i_out: output-current sample */
    float *p_i_l;   /* p_i_l: inductor-current sample */
    void (*p_set_pwm_func)(float buck_duty,
                           uint8_t buck_up_en,
                           uint8_t buck_dn_en,
                           float boost_duty,
                           uint8_t boost_up_en,
                           uint8_t boost_dn_en); /* p_set_pwm_func: buck-boost PWM update hook */
    void (*p_pwm_disable)(void); /* p_pwm_disable: fast PWM shutdown hook */
} bb_ctrl_hal_t;

typedef struct
{
    void (*p_enter_run_func)(void); /* p_enter_run_func: enter-run side effects */
    void (*p_exit_run_func)(void);  /* p_exit_run_func: exit-run side effects */
    uint8_t *p_latched;             /* p_latched: hard-protect latch shared with FSM */
} bb_fsm_hal_t;

void bb_hal_hard_protect_trip(void);
void bb_hal_hard_protect_clear(void);

#endif
