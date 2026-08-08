// SPDX-License-Identifier: MIT
/**
 * @file    shell_core.h
 * @brief   Shell core public interface.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Define shell variable types, command descriptors, variable descriptors, and runtime context
 *          - Define transport-neutral command and variable descriptors
 *          - Expose parser, explicit registry, lookup, print, and metadata APIs
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
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
#ifndef __SHELL_CORE_H__
#define __SHELL_CORE_H__

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    void (*my_printf)(const char *p_format, ...);
    void (*tx_by_dma)(char *p_data, int length);
} shell_core_io_t;

typedef void (*shell_core_func_t)(shell_core_io_t *p_io);

typedef void *(*shell_core_list_first_f)(void *p_context, void **pp_cursor);
typedef void *(*shell_core_list_next_f)(void *p_context, void **pp_cursor);

typedef struct
{
    void *p_context;
    shell_core_list_first_f p_first;
    shell_core_list_next_f p_next;
} shell_core_list_t;

typedef struct
{
    const shell_core_list_t *p_list;
    void *p_cursor;
} shell_core_list_iterator_t;

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
#define SHELL_STRING_ENABLE 1u

#if ((SHELL_STRING_ENABLE != 0u) && (SHELL_STRING_ENABLE != 1u))
#error "SHELL_STRING_ENABLE must be 0 or 1."
#endif

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
#define SHELL_STA_READ_ONLY (1u << 3)

/**
 * @brief Shell registration entry (command or variable).
 *
 * - shell_register() inserts descriptors supplied by the integration layer.
 * - shell_run() traverses this list to match and dispatch commands, and to
 *   read / write variables.
 *
 * The output-interface field is optional: implementations may use it to cache
 * or forward the transport output interface, but it is not mandatory.
 */
typedef struct shell_core_item
{
    const char *p_name;
    uint32_t p_name_size;

    void *p_var;                 ///< Variable address (NULL for commands).
    uint32_t type;               ///< SHELL_TYPE_E.
    void *p_max;                 ///< Upper-limit pointer (optional).
    void *p_min;                 ///< Lower-limit pointer (optional).
    shell_core_func_t func;      ///< Callback (command exec, variable-changed notification, etc.).
    uint32_t status;

    shell_core_io_t *my_printf; ///< Optional: cached / forwarded output interface at runtime.
} shell_core_item_t;

#define SHELL_CORE_LIMIT_DEFINE(suffix, type)                                      \
    static inline void shell_core_limit_##suffix(type *p_value,                    \
                                                   const void *p_upper,              \
                                                   const void *p_lower)              \
    {                                                                               \
        const type upper = *(const type *)p_upper;                                  \
        const type lower = *(const type *)p_lower;                                  \
        if (*p_value > upper)                                                       \
        {                                                                           \
            *p_value = upper;                                                       \
        }                                                                           \
        else if (*p_value < lower)                                                  \
        {                                                                           \
            *p_value = lower;                                                       \
        }                                                                           \
    }

SHELL_CORE_LIMIT_DEFINE(i8, int8_t)
SHELL_CORE_LIMIT_DEFINE(u8, uint8_t)
SHELL_CORE_LIMIT_DEFINE(i16, int16_t)
SHELL_CORE_LIMIT_DEFINE(u16, uint16_t)
SHELL_CORE_LIMIT_DEFINE(i32, int32_t)
SHELL_CORE_LIMIT_DEFINE(u32, uint32_t)
SHELL_CORE_LIMIT_DEFINE(f32, float)

#undef SHELL_CORE_LIMIT_DEFINE

/**
 * @brief Clamp a supported scalar variable using its typed limit objects.
 */
#define SHELL_UP_DN_LMT(var, p_up_lmt, p_dn_lmt)                   \
    _Generic(&(var),                                               \
        int8_t *: shell_core_limit_i8,                             \
        uint8_t *: shell_core_limit_u8,                            \
        int16_t *: shell_core_limit_i16,                           \
        uint16_t *: shell_core_limit_u16,                          \
        int32_t *: shell_core_limit_i32,                           \
        uint32_t *: shell_core_limit_u32,                          \
        float *: shell_core_limit_f32)(&(var), (p_up_lmt), (p_dn_lmt))

/* Shell: handler interface (for LINK dispatch)
 *
 * ctx convention: ctx points to a shell_ctx_t created by DECLARE_SHELL_CTX.
 */
void shell_core_run(const shell_core_list_t *p_list, uint8_t data, shell_core_io_t *my_printf, void *p_ctx);
void shell_core_item_print(shell_core_item_t *p_item, shell_core_io_t *my_printf);
uint32_t shell_core_count_get(const shell_core_list_t *p_list);
shell_core_item_t *shell_core_find(const shell_core_list_t *p_list, const char *p_name, uint8_t len);

#endif /* __SHELL_CORE_H__ */
