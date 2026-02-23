#include "DllHeader.h"
#include "plecs.h"
#include "section.h"
#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"
#include "plecs_log_file_path.c"

FILE *fp_plecs;

static struct SimulationState *plecs_astate;
uint32_t plecs_time_100us = 0;

float plecs_get_input(PLECS_INPUT_E num)
{
    if (num < PLECS_INPUT_MAX)
    {
        return plecs_astate->inputs[num];
    }
    else
    {
        return 0.0f;
    }
}

void plecs_set_output(PLECS_OUTPUT_E num, float val)
{
    if (num < PLECS_OUTPUT_MAX)
    {
        plecs_astate->outputs[num] = val;
    }
}

void plecs_printf(const char *file, int line, const char *format, ...)
{
    if (fp_plecs)
    {
        double time = plecs_astate->time; // unit: seconds
        fprintf(fp_plecs, "[%8.4f] [%s:%d] ", time, file, line);
        va_list args;
        va_start(args, format);
        vfprintf(fp_plecs, format, args);
        va_end(args);
        fflush(fp_plecs);
    }
}

DLLEXPORT void plecsSetSizes(struct SimulationSizes *aSizes)
{
    aSizes->numInputs = PLECS_INPUT_NUM;
    aSizes->numOutputs = PLECS_OUTPUT_NUM;
    aSizes->numParameters = 0;
    aSizes->numStates = 0;
}

DLLEXPORT void plecsStart(struct SimulationState *aState)
{
    plecs_astate = aState;
    if (fp_plecs)
    {
        fclose(fp_plecs);
        fp_plecs = NULL;
    }
    fp_plecs = fopen(PLECS_LOG_FILE_PATH, "w"); // 以写入模式打开
    section_init();
}

DLLEXPORT void plecsOutput(struct SimulationState *aState)
{
    plecs_astate = aState;
    static float time = 0.0f;
    static float time_last = 0.0f;
    time = plecs_astate->time;
    if ((time - time_last) > 0.0001f)
    {
        plecs_time_100us += (uint32_t)((time - time_last) * 10000.0f);
        run_task();
        time_last += 0.0001f;
    }
    section_interrupt();
}
