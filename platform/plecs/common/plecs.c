#include "DllHeader.h"
#include "plecs.h"
#include "section.h"
#include "stdint.h"
#include "stdio.h"
#include "stdarg.h"

#if defined(_WIN32)
#include "wchar.h"
#include "windows.h"
#define PLECS_LOG_FILE_NAME L"plecs_log.txt"
#define PLECS_LOG_PATH_CAPACITY 1024U
#else
#define PLECS_LOG_FILE_NAME "plecs_log.txt"
#endif

static FILE *fp_plecs = NULL; /* 当前 PLECS DLL 实例使用的日志文件句柄。 */
static double interrupt_time_last = 0.0; /* Simulation time of the latest committed control interrupt. */
static uint8_t interrupt_time_valid = 0u; /* Nonzero after one control interrupt has run in this simulation. */

/**
 * @return DLL 目录中的日志文件句柄；路径解析或文件打开失败时返回 NULL。
 */
static FILE *plecs_log_open(void)
{
#if defined(_WIN32)
    HMODULE module = NULL;        /* 当前 common/plecs.c 所属的已加载 DLL 模块句柄。 */
    DWORD path_length = 0U;       /* 不含结尾空字符的 DLL 绝对路径长度。 */
    wchar_t *separator = NULL;    /* DLL 路径中最后一个目录分隔符的位置。 */
    size_t directory_length = 0U; /* 包含末尾目录分隔符的 DLL 目录长度。 */
    const size_t file_name_length = sizeof(PLECS_LOG_FILE_NAME) / sizeof(PLECS_LOG_FILE_NAME[0]);
    static wchar_t log_path[PLECS_LOG_PATH_CAPACITY] = {L'\0'}; /* DLL 同目录日志文件的完整路径。 */

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)(const void *)&fp_plecs,
                           &module) == FALSE)
    {
        return NULL;
    }

    path_length = GetModuleFileNameW(module, log_path, (DWORD)PLECS_LOG_PATH_CAPACITY);
    if (path_length == 0U)
    {
        return NULL;
    }
    if (path_length >= (DWORD)PLECS_LOG_PATH_CAPACITY)
    {
        return NULL;
    }

    separator = wcsrchr(log_path, L'\\');
    if (separator == NULL)
    {
        separator = wcsrchr(log_path, L'/');
    }
    if (separator == NULL)
    {
        return NULL;
    }

    directory_length = (size_t)(separator - log_path) + 1U;
    if ((directory_length + file_name_length) > PLECS_LOG_PATH_CAPACITY)
    {
        return NULL;
    }

    (void)wmemcpy(&log_path[directory_length], PLECS_LOG_FILE_NAME, file_name_length);
    return _wfopen(log_path, L"w");
#else
    return fopen(PLECS_LOG_FILE_NAME, "w");
#endif
}

static struct SimulationState *plecs_astate;
uint32_t plecs_time_100us = 0;

float plecs_get_input(PLECS_INPUT_E num)
{
    if (num < PLECS_INPUT_MAX)
    {
        return (float)plecs_astate->inputs[num];
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
    (void)file;
    (void)line;
    if (fp_plecs != NULL)
    {
        double time = plecs_astate->time; // unit: seconds
        fprintf(fp_plecs, "[%8.4f] ", time);
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
    interrupt_time_last = 0.0;
    interrupt_time_valid = 0u;
    if (fp_plecs != NULL)
    {
        fclose(fp_plecs);
        fp_plecs = NULL;
    }
    fp_plecs = plecs_log_open(); /* 在当前 DLL 所在目录创建本次仿真的日志文件。 */
    section_init();
}

DLLEXPORT void plecsOutput(struct SimulationState *aState)
{
    plecs_astate = aState;
    static float time = 0.0f;
    static float time_last = 0.0f;
    time = (float)plecs_astate->time;
    if ((time - time_last) > 0.0001f)
    {
        plecs_time_100us += (uint32_t)((time - time_last) * 10000.0f);
        run_task();
        time_last += 0.0001f;
    }
    if ((interrupt_time_valid == 0u) ||             /* Execute the initial sample exactly once. */
        (plecs_astate->time > interrupt_time_last)) /* Reject repeated output evaluations at the same sample time. */
    {
        section_interrupt();
        interrupt_time_last = plecs_astate->time;
        interrupt_time_valid = 1u;
    }
}

/**
 * @brief Release resources owned by the current PLECS simulation instance.
 * @param aState Simulation state supplied by PLECS during termination.
 */
DLLEXPORT void plecsTerminate(struct SimulationState *aState)
{
    (void)aState;

    if (fp_plecs != NULL)
    {
        (void)fclose(fp_plecs);
        fp_plecs = NULL;
    }
    plecs_astate = NULL;
}
