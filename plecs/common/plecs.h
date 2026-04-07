#ifndef __PLECS_H
#define __PLECS_H

#include "stdint.h"

typedef enum
{
    PLECS_INPUT_I_L,
    PLECS_INPUT_V_CAP,
    PLECS_INPUT_V_BUS,
    PLECS_INPUT_V_G,
    PLECS_INPUT_I_OUT,
    PLECS_INPUT_V_OUT,
    PLECS_INPUT_MODE,
    PLECS_INPUT_MAX,
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_PWM_FAST_DUTY,
    PLECS_OUTPUT_PWM_FAST_UP_EN,
    PLECS_OUTPUT_PWM_FAST_DN_EN,
    PLECS_OUTPUT_PWM_SLOW_DUTY,
    PLECS_OUTPUT_PWM_SLOW_UP_EN,
    PLECS_OUTPUT_PWM_SLOW_DN_EN,
    PLECS_OUTPUT_AC_OUT_RLY_EN,
    PLECS_OUTPUT_MAIN_RLY_EN,
    PLECS_OUTPUT_DBG,
    PLECS_OUTPUT_MAX = 30,
} PLECS_OUTPUT_E;

#define PLECS_INPUT_NUM PLECS_INPUT_MAX
#define PLECS_OUTPUT_NUM PLECS_OUTPUT_MAX

float plecs_get_input(PLECS_INPUT_E num);
void plecs_set_output(PLECS_OUTPUT_E num, float val);

void plecs_printf(const char *file, int line, const char *format, ...);

#define PLECS_LOG(...) plecs_printf(__FILE__, __LINE__, __VA_ARGS__)

extern uint32_t plecs_time_100us;

#endif
