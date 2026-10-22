// SPDX-License-Identifier: MIT
/**
 * @file demo_scope.c
 * @brief Own the demo scope sample storage and binding retry lifecycle.
 * @details Entity: bounded sample storage. Prior: platform Interface is available.
 *          Time: initialization and 1 ms retry task; sampling stays in the shared demo ISR.
 * @author Max.Li
 * @date 2026-10-23
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#include "realtime_scope_bindings.h"
#include "section.h"
static uint8_t sample_queue[BASE_SCOPE_QUEUE_BYTES];
static bool bound;
static void bind_service(void)
{
    if (!bound)
        bound = realtime_scope_bindings_bind(sample_queue, sizeof(sample_queue));
}
REG_INIT(2, bind_service)
REG_TASK_MS(1u, bind_service)
