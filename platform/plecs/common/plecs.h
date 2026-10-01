#ifndef __PLECS_H
#define __PLECS_H

#include "stdint.h"
#include "stdbool.h"
#include "plecs_port.h"

/* Existing models have no DLL parameters unless their port definition opts in. */
#ifndef PLECS_PARAMETER_NUM
#define PLECS_PARAMETER_NUM 0
#endif

#define PLECS_INPUT_NUM  PLECS_INPUT_MAX
#define PLECS_OUTPUT_NUM PLECS_OUTPUT_MAX

float plecs_get_input(PLECS_INPUT_E num);
bool plecs_get_parameter(uint32_t num, double *value);
void plecs_set_error_message(const char *message);
void plecs_set_output(PLECS_OUTPUT_E num, float val);

void plecs_printf(const char *file,
                  int line,
                  const char *format,
                  ...);

#define PLECS_LOG(...) plecs_printf(__FILE__, __LINE__, __VA_ARGS__)

extern uint32_t plecs_time_100us;

void sim_comm_stop(void);
void plecs_platform_start(void);
void plecs_platform_terminate(void);
void plecs_perf_counter_refresh(void);

#endif
