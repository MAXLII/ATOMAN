// SPDX-License-Identifier: MIT
/**
 * @file    npc_platform.c
 * @brief   NPC simulation log adapter.
 * @details
 *          This file is part of the base digital power framework project.
 *          Provide the shared Shell/TCP stack with the PLECS logging facade.
 *          C11 compatible; no dynamic allocation; single simulation instance.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "plecs.h"
#include "npc_platform.h"
#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

static FILE *p_runtime_log; /* Append-only runtime log beside the loaded NPC DLL. */
static FILE *p_trace_log; /* Buffered, bounded waveform capture; simulation thread only. */
static char trace_buffer[65536]; /* Avoid a flush/system call for every control sample. */
static unsigned int trace_sequence; /* Distinguish captures started in the same millisecond. */
static unsigned int trace_flush_count; /* Flush once per second of captured simulation time. */
static const char module_anchor = 0; /* Object address used to locate this module without function-pointer casts. */

void npc_log_stop(void)
{
    npc_trace_stop();
    if (p_runtime_log != NULL)
    {
        (void)fclose(p_runtime_log);
        p_runtime_log = NULL;
    }
}

void npc_trace_stop(void)
{
    if (p_trace_log != NULL)
    {
        (void)fclose(p_trace_log);
        p_trace_log = NULL;
    }
}

int npc_trace_start(const char *p_header)
{
    HMODULE module = NULL; /* Loaded DLL owns the output directory. */
    wchar_t path[MAX_PATH] = {0}; /* Full unique CSV path. */
    wchar_t *p_separator = NULL;
    SYSTEMTIME stamp = {0}; /* UTC host time is only used for naming the file. */
    DWORD length = 0u;
    int written = 0;
    npc_trace_stop();
    if ((p_header == NULL) ||
        (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           &module_anchor, &module) == 0))
    {
        return 0;
    }
    length = GetModuleFileNameW(module, path, MAX_PATH);
    if ((length == 0u) || (length >= MAX_PATH)) { return 0; }
    p_separator = wcsrchr(path, L'\\');
    if (p_separator == NULL) { return 0; }
    ++p_separator;
    GetSystemTime(&stamp);
    ++trace_sequence;
    written = swprintf(p_separator, MAX_PATH - (size_t)(p_separator - path),
                       L"npc_trace_%04u%02u%02u_%02u%02u%02u_%03u_%lu_%u.csv",
                       (unsigned int)stamp.wYear, (unsigned int)stamp.wMonth, (unsigned int)stamp.wDay,
                       (unsigned int)stamp.wHour, (unsigned int)stamp.wMinute, (unsigned int)stamp.wSecond,
                       (unsigned int)stamp.wMilliseconds, (unsigned long)GetCurrentProcessId(), trace_sequence);
    if (written < 0) { return 0; }
    p_trace_log = _wfopen(path, L"wx"); /* Never replace an earlier trace. */
    if (p_trace_log == NULL) { return 0; }
    (void)setvbuf(p_trace_log, trace_buffer, _IOFBF, sizeof(trace_buffer));
    trace_flush_count = 0u;
    if ((fprintf(p_trace_log, "%s\n", p_header) < 0) || (fflush(p_trace_log) != 0))
    {
        npc_trace_stop();
        return 0;
    }
    return 1;
}

int npc_trace_write(double time_s, const float *p_values, size_t count)
{
    if ((p_trace_log == NULL) || (p_values == NULL)) { return 0; }
    (void)fprintf(p_trace_log, "%.9f", time_s);
    for (size_t i = 0u; i < count; ++i)
    {
        (void)fprintf(p_trace_log, ",%.9g", (double)p_values[i]);
    }
    (void)fputc('\n', p_trace_log);
    ++trace_flush_count;
    if (trace_flush_count >= 5000u)
    {
        trace_flush_count = 0u;
        if (fflush(p_trace_log) != 0) { npc_trace_stop(); return 0; }
    }
    if (ferror(p_trace_log) != 0) { npc_trace_stop(); return 0; }
    return 1;
}

int npc_log_start(void)
{
    HMODULE module = NULL; /* Module containing this logging adapter. */
    wchar_t path[MAX_PATH] = {0}; /* Unicode DLL path, then sibling log path. */
    wchar_t *p_separator = NULL; /* Final path component delimiter. */
    DWORD length = 0u; /* Windows-reported path length. */
    npc_log_stop();
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           &module_anchor, &module) == 0)
    {
        return 0;
    }
    length = GetModuleFileNameW(module, path, MAX_PATH);
    if ((length == 0u) || /* API failure. */
        (length >= MAX_PATH)) /* Reject a truncated path. */
    {
        return 0;
    }
    p_separator = wcsrchr(path, L'\\');
    if (p_separator == NULL)
    {
        return 0;
    }
    ++p_separator;
    if ((size_t)(p_separator - path) + wcslen(L"npc_runtime.log") >= MAX_PATH)
    {
        return 0;
    }
    (void)wcscpy(p_separator, L"npc_runtime.log");
    p_runtime_log = _wfopen(path, L"a");
    if (p_runtime_log == NULL)
    {
        return 0;
    }
    (void)fprintf(p_runtime_log, "\nNPC LOG OPEN build=%s %s pid=%lu\n", __DATE__, __TIME__,
                  (unsigned long)GetCurrentProcessId());
    (void)fflush(p_runtime_log);
    return 1;
}

/** @param file Source file. @param line Source line. @param format Printf format and following arguments. */
void plecs_printf(const char *file, int line, const char *format, ...)
{
    va_list args; /* Variable argument list used only by this log call. */
    (void)file;
    (void)line;
    va_start(args, format);
    (void)vfprintf(stderr, format, args);
    va_end(args);
    if (p_runtime_log != NULL)
    {
        va_start(args, format);
        (void)vfprintf(p_runtime_log, format, args);
        va_end(args);
        (void)fflush(p_runtime_log);
    }
}
