// SPDX-License-Identifier: MIT
/**
 * @file    shell.h
 * @brief   Shell core public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define shell variable types, command descriptors, variable descriptors, and runtime context
 *          - Provide REG_SHELL_CMD and REG_SHELL_VAR registration macros for section-based discovery
 *          - Expose shell parser, registry lookup, item print, and metadata access APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-05-01
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef __SHELL_H__
#define __SHELL_H__

#include <stdint.h>
#include <stddef.h>

#include "section.h"

/**
 * @brief Define this macro to enable string expression parsing and variable writing.
 *
 * When defined, the shell supports:
 * - Value assignment to variables (e.g., "gain:3.5")
 * - Arithmetic expressions (e.g., "1+2*3")
 * - Hex/binary literals (0x1A, 0b1010)
 * - Status flag parsing (-s N)
 *
 * When not defined, the shell is read-only: variables can be printed but not written.
 */
#define SHELL_STRING_PARSE 0

/* Shell: input parsing context (used by shell_run only)
 *
 * - shell_ctx_t is the private per-link, per-handler context for shell_run.
 * - The link layer passes ctx as void* transparently and does not inspect it.
 */
typedef struct
{
    uint8_t shell_buffer[128];
    uint8_t shell_index;
} shell_ctx_t;

/**
 * @brief Declare a shell ctx in one line inside a business module.
 * @note The ctx is typically bound as the handler_arr / shell_run ctx.
 */
#define DECLARE_SHELL_CTX(name) \
    static shell_ctx_t name = {0}

/* Shell: variable / command registration
 *
 * Items are placed in the SECTION_SHELL linker section.
 * At runtime section_init() scans the section and inserts each item
 * into the p_shell_first linked list.
 */

typedef enum
{
    SHELL_INT8 = 0,
    SHELL_UINT8,
    SHELL_INT16,
    SHELL_UINT16,
    SHELL_INT32,
    SHELL_UINT32,
    SHELL_FP32,
    SHELL_CMD,
} SHELL_TYPE_E;

#define SHELL_STR_SIZE_MAX 40u

#define SHELL_STA_NULL (0u)
#define SHELL_STA_AUTO (1u << 2)

/**
 * @brief Shell registration entry (command or variable).
 *
 * - section_init() inserts all SECTION_SHELL items into the p_shell_first
 *   linked list.
 * - shell_run() traverses this list to match and dispatch commands, and to
 *   read / write variables.
 *
 * The DEC_MY_PRINTF field is optional: implementations may use it to cache
 * or forward the transport output interface, but it is not mandatory.
 */
typedef struct section_shell_t
{
    const char *p_name;
    uint32_t p_name_size;

    void *p_var;                 ///< Variable address (NULL for commands).
    uint32_t type;               ///< SHELL_TYPE_E.
    void *p_max;                 ///< Upper-limit pointer (optional).
    void *p_min;                 ///< Lower-limit pointer (optional).
    void (*func)(DEC_MY_PRINTF); ///< Callback (command exec, variable-changed notification, etc.).
    uint32_t status;

    DEC_MY_PRINTF;               ///< Optional: cached / forwarded output interface at runtime.
    struct section_shell_t *p_next;
} section_shell_t;

/**
 * @brief Variable clamping macro.
 */
#define SHELL_UP_DN_LMT(var, p_up_lmt, p_dn_lmt)         \
    do                                                   \
    {                                                    \
        if ((var) > *(__typeof__(var) *)(p_up_lmt))      \
        {                                                \
            (var) = *(__typeof__(var) *)(p_up_lmt);      \
        }                                                \
        else if ((var) < *(__typeof__(var) *)(p_dn_lmt)) \
        {                                                \
            (var) = *(__typeof__(var) *)(p_dn_lmt);      \
        }                                                \
    } while (0)

/**
 * @brief Register a Shell variable.
 *
 * @param _name   Variable name (converted to a string as the shell command name).
 * @param _var    Variable instance.
 * @param _type   SHELL_TYPE_E.
 * @param _max    Maximum value (literal or same-type value).
 * @param _min    Minimum value (literal or same-type value).
 * @param _func   Optional callback (triggered after the variable is written).
 * @param _status Status bits (e.g. SHELL_STA_AUTO).
 */
#define REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)                      \
    static __typeof__(_var) _name##_##max = (__typeof__(_var))(_max);                      \
    static __typeof__(_var) _name##_##min = (__typeof__(_var))(_min);                      \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1),                                     \
        .p_var = (void *)&(_var),                                                          \
        .type = (uint32_t)(_type),                                                         \
        .p_max = (void *)&_name##_##max,                                                   \
        .p_min = (void *)&_name##_##min,                                                   \
        .func = (_func),                                                                   \
        .status = (uint32_t)(_status),                                                     \
        .p_next = NULL,                                                                    \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

/**
 * @brief Register a Shell command.
 */
#define REG_SHELL_CMD(_name, _func)                                                        \
    section_shell_t section_shell_##_name = {                                              \
        .p_name = #_name,                                                                  \
        .p_name_size = (uint32_t)(sizeof(#_name) - 1),                                     \
        .p_var = NULL,                                                                     \
        .type = (uint32_t)SHELL_CMD,                                                       \
        .func = (_func),                                                                   \
        .status = 0,                                                                       \
        .p_next = NULL,                                                                    \
    };                                                                                     \
    static_assert(sizeof(#_name) <= (SHELL_STR_SIZE_MAX + 1), #_name " String too long!"); \
    REG_SECTION_FUNC(SECTION_SHELL, section_shell_##_name)

/* Shell: handler interface (for LINK dispatch)
 *
 * ctx convention: ctx points to a shell_ctx_t created by DECLARE_SHELL_CTX.
 */
void shell_run(uint8_t data, DEC_MY_PRINTF, void *ctx);

void shell_item_print(section_shell_t *p, DEC_MY_PRINTF);
section_shell_t *shell_first_get(void);
uint32_t shell_count_get(void);
section_shell_t *shell_find(const char *p_name, uint8_t len);

#endif /* __SHELL_H__ */
