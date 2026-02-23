#ifndef __PLECS_H
#define __PLECS_H

#include "stdint.h"

typedef enum
{
    PLECS_INPUT_SIM_TIME,
    PLECS_INPUT_MAX = 2,
} PLECS_INPUT_E;

typedef enum
{
    PLECS_OUTPUT_LED1,
    PLECS_OUTPUT_LED2,
    PLECS_OUTPUT_LED3,
    PLECS_OUTPUT_LED4,
    PLECS_OUTPUT_MAX,
} PLECS_OUTPUT_E;

#define PLECS_INPUT_NUM PLECS_INPUT_MAX
#define PLECS_OUTPUT_NUM PLECS_OUTPUT_MAX

float plecs_get_input(PLECS_INPUT_E num);
void plecs_set_output(PLECS_OUTPUT_E num, float val);

void plecs_printf(const char *file, int line, const char *format, ...);

#define PLECS_LOG(...) plecs_printf(__FILE__, __LINE__, __VA_ARGS__)

extern uint32_t plecs_time_1ms;

#endif
