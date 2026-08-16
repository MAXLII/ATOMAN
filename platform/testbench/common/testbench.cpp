// SPDX-License-Identifier: MIT
/**
 * @file    testbench.cpp
 * @brief   Common DUT testbench runtime module.
 * @details
 *          This file is part of the base digital power framework project.
 *
 *          Module responsibilities:
 *          - Build module and test case lists from GCC linker section boundaries
 *          - Execute every registered DUT module and its registered test cases
 *          - Report case results and return a process status suitable for automation
 *
 *          Design notes:
 *          - C++14 compatible
 *          - No dynamic memory allocation
 *          - Independent from the base Section framework
 *          - Intended for host execution and never called from an ISR
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

#include "testbench.h"

#include <cstdlib>
#include <iostream>

/**
 * @brief Aggregate execution counts for the complete testbench run.
 */
typedef struct
{
    uint32_t module_count;         /**< Number of registered modules executed. */
    uint32_t case_count;           /**< Number of registered cases executed. */
    uint32_t passed_case_count;    /**< Number of cases that passed. */
    uint32_t completed_case_count; /**< Number of cases completed without a verdict. */
    uint32_t failed_case_count;    /**< Number of cases that failed. */
} testbench_summary_t;

/**
 * @brief Build the runtime case list for one module from its linker section.
 * @param p_module Registered DUT module owning the case section.
 * @return First case in the module list, or NULL when the section is empty.
 */
static testbench_case_t *link_module_cases(testbench_module_t *p_module)
{
    testbench_case_t *p_case = nullptr; /**< Case currently being linked. */
    testbench_case_t *p_next = nullptr; /**< Next case address derived from section order. */

    p_module->p_case_head = nullptr;
    if (p_module->p_case_section_begin >= p_module->p_case_section_end)
    {
        return nullptr;
    }

    p_module->p_case_head = p_module->p_case_section_begin;
    p_case = p_module->p_case_section_begin;
    while (p_case < p_module->p_case_section_end)
    {
        p_next = p_case + 1;
        if (p_next < p_module->p_case_section_end)
        {
            p_case->p_next = p_next;
        }
        else
        {
            p_case->p_next = nullptr;
        }
        p_case = p_next;
    }

    return p_module->p_case_head;
}

/**
 * @brief Build the runtime module list and every nested test case list.
 * @return First registered test module, or NULL when the module section is empty.
 */
static testbench_module_t *link_testbench_modules(void)
{
    testbench_module_t *p_module = nullptr; /**< Module currently being linked. */
    testbench_module_t *p_next = nullptr;   /**< Next module address derived from section order. */

    if (__start_testbench_module >= __stop_testbench_module)
    {
        return nullptr;
    }

    p_module = __start_testbench_module;
    while (p_module < __stop_testbench_module)
    {
        (void)link_module_cases(p_module);

        p_next = p_module + 1;
        if (p_next < __stop_testbench_module)
        {
            p_module->p_next = p_next;
        }
        else
        {
            p_module->p_next = nullptr;
        }
        p_module = p_next;
    }

    return __start_testbench_module;
}

/**
 * @brief Verify the module timing and all callbacks required to execute a case.
 * @param p_module DUT module associated with the case.
 * @param p_case Test case being validated.
 * @return
 *         0: the module period is invalid or one or more callbacks are missing.
 *         1: the module period and all required callbacks are valid.
 */
static uint8_t case_configuration_valid(const testbench_module_t *p_module, const testbench_case_t *p_case)
{
    if ((p_module->run_period_s <= 0.0) ||   /* Elapsed time requires a positive module period. */
        (p_module->p_dut_init == nullptr) || /* Every case requires DUT initialization. */
        (p_module->p_dut_run == nullptr) ||  /* Every period requires the DUT body. */
        (p_case->p_init == nullptr) ||       /* The case must initialize its isolated environment. */
        (p_case->p_before_dut == nullptr) || /* The case must prepare each DUT input period. */
        (p_case->p_after_dut == nullptr))    /* The case must apply feedback and return its state. */
    {
        return 0u;
    }
    return 1u;
}

/**
 * @brief Execute one test case against its owning DUT module.
 * @param p_module DUT module associated with the case.
 * @param p_case Registered test case to execute.
 * @return COMPLETE, PASS, or FAIL result reported by the test case.
 */
static TESTBENCH_CASE_STATE_E run_case(const testbench_module_t *p_module, const testbench_case_t *p_case)
{
    uint32_t run_count = 1u;     /**< Current execution beat; beat 0 is reserved for the initial condition. */
    double elapsed_time_s = 0.0; /**< Simulated time of the current execution beat, in seconds. */

    TESTBENCH_CASE_STATE_E case_state = TESTBENCH_CASE_RUNNING; /**< State returned after a DUT run. */

    std::cout << "  CASE " << p_case->p_case_name << '\n';
    if (case_configuration_valid(p_module, p_case) == 0u)
    {
        std::cout << "    RESULT FAIL | invalid module or case configuration\n";
        return TESTBENCH_CASE_FAIL;
    }

    p_case->p_init();
    p_module->p_dut_init();
    elapsed_time_s = static_cast<double>(run_count) * p_module->run_period_s;

    for (;;)
    {
        p_case->p_before_dut(elapsed_time_s);
        p_module->p_dut_run();
        case_state = p_case->p_after_dut(elapsed_time_s);
        if (case_state != TESTBENCH_CASE_RUNNING)
        {
            break;
        }
        run_count++;
        elapsed_time_s = static_cast<double>(run_count) * p_module->run_period_s;
    }

    if ((case_state != TESTBENCH_CASE_COMPLETE) && /* The case did not report neutral completion. */
        (case_state != TESTBENCH_CASE_PASS) &&     /* The case did not report successful completion. */
        (case_state != TESTBENCH_CASE_FAIL))       /* The case did not report an assertion failure. */
    {
        std::cout << "    RESULT FAIL | invalid case state="
                  << static_cast<unsigned int>(case_state) << '\n';
        return TESTBENCH_CASE_FAIL;
    }
    if (case_state == TESTBENCH_CASE_FAIL)
    {
        std::cout << "    RESULT FAIL | time_s=" << elapsed_time_s << '\n';
        return TESTBENCH_CASE_FAIL;
    }
    if (case_state == TESTBENCH_CASE_COMPLETE)
    {
        std::cout << "    RESULT COMPLETE | time_s=" << elapsed_time_s << '\n';
        return TESTBENCH_CASE_COMPLETE;
    }

    std::cout << "    RESULT PASS | time_s=" << elapsed_time_s << '\n';
    return TESTBENCH_CASE_PASS;
}

/**
 * @brief Execute all test cases registered below one DUT module.
 * @param p_module Registered DUT module to execute.
 * @param p_summary Aggregate counters updated for this module and its cases.
 */
static void run_module(const testbench_module_t *p_module, testbench_summary_t *p_summary)
{
    const testbench_case_t *p_case = nullptr;            /**< Case currently being executed. */
    TESTBENCH_CASE_STATE_E result = TESTBENCH_CASE_FAIL; /**< Result of the current case. */

    p_summary->module_count++;
    std::cout << "MODULE " << p_module->p_name << '\n';
    if (p_module->p_case_head == nullptr)
    {
        std::cout << "  RESULT FAIL | no registered test case\n";
        p_summary->failed_case_count++;
        return;
    }

    p_case = p_module->p_case_head;
    while (p_case != nullptr)
    {
        p_summary->case_count++;
        result = run_case(p_module, p_case);
        if (result == TESTBENCH_CASE_PASS)
        {
            p_summary->passed_case_count++;
        }
        else if (result == TESTBENCH_CASE_COMPLETE)
        {
            p_summary->completed_case_count++;
        }
        else
        {
            p_summary->failed_case_count++;
        }
        p_case = p_case->p_next;
    }
}

int main(void)
{
    testbench_module_t *p_module_head = nullptr; /**< Head of the runtime DUT module list. */
    testbench_module_t *p_module = nullptr;      /**< Module currently being executed. */
    testbench_summary_t summary = {};            /**< Aggregate result of the testbench run. */

    p_module_head = link_testbench_modules();
    if (p_module_head == nullptr)
    {
        std::cout << "TESTBENCH FAIL | no registered test module\n";
        return EXIT_FAILURE;
    }

    std::cout << "TESTBENCH START\n";
    p_module = p_module_head;
    while (p_module != nullptr)
    {
        run_module(p_module, &summary);
        p_module = p_module->p_next;
    }

    std::cout << "TESTBENCH SUMMARY | modules=" << summary.module_count
              << " cases=" << summary.case_count
              << " passed=" << summary.passed_case_count
              << " completed=" << summary.completed_case_count
              << " failed=" << summary.failed_case_count << '\n';

    if (summary.failed_case_count == 0u)
    {
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
