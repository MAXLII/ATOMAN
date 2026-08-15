// SPDX-License-Identifier: MIT
/**
 * @file    testbench.h
 * @brief   GCC host testbench registration interface.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Define test module and test case registration descriptors
 *          - Register multiple DUT modules in a dedicated GCC linker section
 *          - Register each module's test cases in a module-specific linker section
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Independent from the base Section framework
 *          - Linker section boundaries are consumed only by the testbench runner
 *
 * @author  Max.Li
 * @date    2026-08-16
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#ifndef TESTBENCH_H
#define TESTBENCH_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief State returned by a test case after one completed DUT run.
 */
typedef enum
{
    TESTBENCH_CASE_RUNNING_E = 0, /**< Continue running the current test case. */
    TESTBENCH_CASE_PASS_E,        /**< Stop because all final assertions passed. */
    TESTBENCH_CASE_FAIL_E         /**< Stop because a final assertion failed. */
} TESTBENCH_CASE_STATE_E;

/**
 * @brief Test case descriptor registered in a module-specific linker section.
 */
typedef struct testbench_case
{
    const char *p_test_name;                       /**< Owning test module name. */
    const char *p_case_name;                       /**< Test case name within the module. */
    void (*p_init)(void);              /**< Case environment and recording initialization. */
    /** Advance delayed physics, apply scheduled changes, prepare inputs, and perform pre-DUT recording. */
    void (*p_before_dut)(double time_s);
    /** Observe the current DUT output, apply immediate effects, record results, and evaluate completion. */
    TESTBENCH_CASE_STATE_E (*p_after_dut)(double time_s);
    struct testbench_case *p_next;     /**< Runtime link to the next case in this module. */
} testbench_case_t;

/**
 * @brief DUT module descriptor registered in the common module linker section.
 */
typedef struct testbench_module
{
    const char *p_name;                         /**< Test module name. */
    double run_period_s;                        /**< Interval between adjacent DUT calls, in seconds. */
    void (*p_dut_init)(void);                   /**< DUT initialization callback. */
    void (*p_dut_run)(void);                    /**< DUT processing callback. */
    testbench_case_t *p_case_section_begin;     /**< First case descriptor in the module section. */
    testbench_case_t *p_case_section_end;       /**< One-past-last descriptor in the module section. */
    testbench_case_t *p_case_head;              /**< Runtime head of the module case list. */
    struct testbench_module *p_next;            /**< Runtime link to the next test module. */
} testbench_module_t;

/*
 * GNU ld synthesizes these boundaries for the common module section when the
 * testbench runner references them.
 */
#ifdef __cplusplus
extern "C"
{
#endif

extern testbench_module_t __start_testbench_module[];
extern testbench_module_t __stop_testbench_module[];

#ifdef __cplusplus
}
#endif

#define TESTBENCH_PRIVATE_STRING_IMPL(value) #value
#define TESTBENCH_PRIVATE_STRING(value) TESTBENCH_PRIVATE_STRING_IMPL(value)

#define TESTBENCH_PRIVATE_MODULE_VARIABLE_IMPL(test_name) \
    testbench_module_##test_name
#define TESTBENCH_PRIVATE_MODULE_VARIABLE(test_name) \
    TESTBENCH_PRIVATE_MODULE_VARIABLE_IMPL(test_name)

#define TESTBENCH_PRIVATE_CASE_VARIABLE_IMPL(test_name, case_name) \
    testbench_case_##test_name##_##case_name
#define TESTBENCH_PRIVATE_CASE_VARIABLE(test_name, case_name) \
    TESTBENCH_PRIVATE_CASE_VARIABLE_IMPL(test_name, case_name)

#define TESTBENCH_PRIVATE_CASE_SECTION_TOKEN_IMPL(test_name) \
    testbench_case_##test_name
#define TESTBENCH_PRIVATE_CASE_SECTION_TOKEN(test_name) \
    TESTBENCH_PRIVATE_CASE_SECTION_TOKEN_IMPL(test_name)
#define TESTBENCH_PRIVATE_CASE_SECTION(test_name) \
    TESTBENCH_PRIVATE_STRING(TESTBENCH_PRIVATE_CASE_SECTION_TOKEN(test_name))

#define TESTBENCH_PRIVATE_CASE_BEGIN_IMPL(test_name) \
    __start_testbench_case_##test_name
#define TESTBENCH_PRIVATE_CASE_BEGIN(test_name) \
    TESTBENCH_PRIVATE_CASE_BEGIN_IMPL(test_name)

#define TESTBENCH_PRIVATE_CASE_END_IMPL(test_name) \
    __stop_testbench_case_##test_name
#define TESTBENCH_PRIVATE_CASE_END(test_name) \
    TESTBENCH_PRIVATE_CASE_END_IMPL(test_name)

/**
 * @brief Register one DUT module in the common testbench module section.
 * @param test_name C identifier used for the descriptor symbol, display name, and case section.
 * @param period_s Interval between adjacent DUT calls, in seconds.
 * @param dut_init DUT initialization function called before each test case.
 * @param dut_run DUT body function called once per registered run period.
 */
#define TESTBENCH_REGISTER(test_name, period_s, dut_init, dut_run)                            \
    extern testbench_case_t TESTBENCH_PRIVATE_CASE_BEGIN(test_name)[];                       \
    extern testbench_case_t TESTBENCH_PRIVATE_CASE_END(test_name)[];                         \
    static testbench_module_t TESTBENCH_PRIVATE_MODULE_VARIABLE(test_name)                   \
        __attribute__((__used__, __section__("testbench_module"), __aligned__(sizeof(void *)))) = { \
            .p_name = TESTBENCH_PRIVATE_STRING(test_name),                                   \
            .run_period_s = (period_s),                                                       \
            .p_dut_init = (dut_init),                                                        \
            .p_dut_run = (dut_run),                                                          \
            .p_case_section_begin = TESTBENCH_PRIVATE_CASE_BEGIN(test_name),                 \
            .p_case_section_end = TESTBENCH_PRIVATE_CASE_END(test_name),                     \
            .p_case_head = NULL,                                                             \
            .p_next = NULL,                                                                  \
        };

/**
 * @brief Register one test case in its DUT module-specific testbench section.
 * @param test_name C identifier matching the owning TESTBENCH_REGISTER invocation.
 * @param case_name C identifier used for the descriptor symbol and display name.
 * @param init Case environment and recording initialization function.
 * @param before_dut Environment generation function called before every DUT run.
 * @param after_dut Environment feedback, recording, completion, and assertion function.
 */
#define TESTBENCH_CASE(test_name, case_name, init, before_dut, after_dut)                        \
    static testbench_case_t TESTBENCH_PRIVATE_CASE_VARIABLE(test_name, case_name)                 \
        __attribute__((__used__,                                                                \
                       __section__(TESTBENCH_PRIVATE_CASE_SECTION(test_name)),                   \
                       __aligned__(sizeof(void *)))) = {                                         \
            .p_test_name = TESTBENCH_PRIVATE_STRING(test_name),                                 \
            .p_case_name = TESTBENCH_PRIVATE_STRING(case_name),                                 \
            .p_init = (init),                                                                   \
            .p_before_dut = (before_dut),                                                       \
            .p_after_dut = (after_dut),                                                         \
            .p_next = NULL,                                                                     \
        };

#endif /* TESTBENCH_H */
