#include "DllHeader.h"
#include "plecs.h"
#include "section.h"
#include "stdint.h"

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

void plecs_set_output(PLECS_INPUT_E num, float val)
{
    if (num < PLECS_OUTPUT_MAX)
    {
        plecs_astate->outputs[num] = val;
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
    section_init();
}

DLLEXPORT void plecsOutput(struct SimulationState *aState)
{
    plecs_astate = aState;
    static float time = 0.0f;
    static float time_last = 0.0f;
    time = plecs_get_input(PLECS_INPUT_SIM_TIME);
    if ((time - time_last) > 0.0001f)
    {
        plecs_time_100us += (uint32_t)((time - time_last) * 10000.0f);
        run_task();
        time_last += 0.0001f;
    }
    section_interrupt();
}
