#ifndef __PLECS_H
#define __PLECS_H

#include "stdint.h"
#include "plecs_port.h"

#define PLECS_INPUT_NUM PLECS_INPUT_MAX
#define PLECS_OUTPUT_NUM PLECS_OUTPUT_MAX

float plecs_get_input(PLECS_INPUT_E num);
void plecs_set_output(PLECS_OUTPUT_E num, float val);

void plecs_printf(const char *file, int line, const char *format, ...);

#define PLECS_LOG(...) plecs_printf(__FILE__, __LINE__, __VA_ARGS__)

extern uint32_t plecs_time_100us;

#endif
