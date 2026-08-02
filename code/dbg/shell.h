// SPDX-License-Identifier: MIT
/**
 * @file    shell.h
 * @brief   Shell Section adapter public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Preserve the existing Shell registration and runtime APIs
 *          - Register Shell descriptors in SECTION_SHELL
 *          - Adapt the Section-owned list to the pure shell_core interface
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Linker-section access is implemented only by shell.c
 *          - Protocol handling belongs to shell_service.c
 *
 * @author  Max.Li
 * @date    2026-08-02
 * @version 2.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SHELL_H__
#define __SHELL_H__

#include "section.h"
#include "shell_core.h"

typedef shell_core_item_t section_shell_t;

#define REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)                       \
    static __typeof__(_var) _name##_##max = (__typeof__(_var))(_max);                       \
    static __typeof__(_var) _name##_##min = (__typeof__(_var))(_min);                       \
    section_shell_t section_shell_##_name = {                                               \
        .p_name = #_name,                                                                   \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1u),                                     \
        .p_var = (void *)&(_var),                                                           \
        .type = (uint32_t)(_type),                                                          \
        .p_max = (void *)&_name##_##max,                                                    \
        .p_min = (void *)&_name##_##min,                                                    \
        .func = (shell_core_func_t)(_func),                                                 \
        .status = (uint32_t)(_status),                                                      \
        .my_printf = NULL,                                                                  \
    };                                                                                      \
    _Static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1u), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

#define REG_SHELL_CMD(_name, _func)                                                         \
    section_shell_t section_shell_##_name = {                                               \
        .p_name = #_name,                                                                   \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1u),                                     \
        .p_var = NULL,                                                                      \
        .type = (uint32_t)SHELL_CMD,                                                        \
        .p_max = NULL,                                                                      \
        .p_min = NULL,                                                                      \
        .func = (shell_core_func_t)(_func),                                                 \
        .status = 0u,                                                                       \
        .my_printf = NULL,                                                                  \
    };                                                                                      \
    _Static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1u), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

extern section_item_t *p_shell_first;

void shell_init(void);
void shell_run(uint8_t data, DEC_MY_PRINTF, void *p_ctx);
void shell_item_print(section_shell_t *p_item, DEC_MY_PRINTF);
uint32_t shell_count_get(void);
section_shell_t *shell_find(const char *p_name, uint8_t len);

#endif /* __SHELL_H__ */
