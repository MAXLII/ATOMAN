// SPDX-License-Identifier: MIT
/**
 * @file    demo_jitter.c
 * @brief   SRTOS interrupt jitter test module.
 * @details
 *          This file is part of the base project.
 *
 *          Module responsibilities:
 *          - Capture TIMER2 ISR entry timestamps into RAM
 *          - Register many non-uniform periodic stress tasks
 *          - Provide online jitter summary counters for debugger inspection
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR path stores fixed-size RAM records and updates simple counters
 *          - TIMER2 counter value is recorded at ISR entry on GD32G553
 *
 * @author  Max.Li
 * @date    2026-06-30
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

#include "demo_jitter.h"

#include "gd32g5x3.h"
#include "section.h"

#include <stdint.h>

#define DEMO_JITTER_TIMER2_HZ 30000u

#if (DEMO_JITTER_CAPTURE_ENABLE == 1u)
volatile demo_jitter_debug_t g_demo_jitter_debug;
volatile uint32_t g_demo_jitter_timer2_count[DEMO_JITTER_SAMPLE_COUNT];

static volatile uint32_t s_demo_jitter_sink;

static void demo_jitter_work_short(uint32_t seed)
{
    uint32_t acc = seed ^ 0x13579BDFu;

    for (uint32_t i = 0u; i < 32u; ++i)
    {
        acc = (acc << 3) ^ (acc >> 5) ^ i;
    }

    s_demo_jitter_sink ^= acc;
}

static void demo_jitter_work_medium(uint32_t seed)
{
    uint32_t acc = seed ^ 0x2468ACE0u;

    for (uint32_t i = 0u; i < 1500u; ++i)
    {
        acc += (i ^ seed);
        acc = (acc << 1) | (acc >> 31);
    }

    s_demo_jitter_sink ^= acc;
}

static void demo_jitter_work_long(uint32_t ticks, uint32_t seed)
{
    const uint32_t start_tick = SECTION_SYS_TICK;
    uint32_t acc = seed ^ 0xA5A5A5A5u;

    while ((uint32_t)(SECTION_SYS_TICK - start_tick) < ticks)
    {
        acc ^= 0x55AA55AAu;
        acc ^= 0x55AA55AAu;
        acc += 1u;
        acc -= 1u;
    }

    s_demo_jitter_sink ^= acc;
}

static void demo_jitter_task_run(uint32_t id, uint32_t mode)
{
    static uint32_t s_task_count[240];
    uint32_t count = 0u;

    if (id >= 240u)
    {
        return;
    }

    s_task_count[id]++;
    count = s_task_count[id];

    switch (mode)
    {
    case 0u:
        demo_jitter_work_short(id + count);
        break;
    case 1u:
        demo_jitter_work_medium(id + count);
        break;
    case 2u:
        demo_jitter_work_long(15u + (id % 7u), id + count);
        break;
    default:
        if ((count % 5u) == 0u)
        {
            demo_jitter_work_long(20u + (id % 11u), id + count);
        }
        else
        {
            demo_jitter_work_short(id + count);
        }
        break;
    }
}

#define DEMO_JITTER_TASK_DEFINE(id, period, mode)        \
    static void demo_jitter_task_##id(void)              \
    {                                                    \
        demo_jitter_task_run((uint32_t)(id), (mode));    \
    }                                                    \
    REG_TASK((period), demo_jitter_task_##id)

#define DEMO_JITTER_TASK_DEFINE4(id0, p0, id1, p1, id2, p2, id3, p3) \
    DEMO_JITTER_TASK_DEFINE(id0, p0, 0u)                             \
    DEMO_JITTER_TASK_DEFINE(id1, p1, 1u)                             \
    DEMO_JITTER_TASK_DEFINE(id2, p2, 2u)                             \
    DEMO_JITTER_TASK_DEFINE(id3, p3, 3u)

DEMO_JITTER_TASK_DEFINE4(0, 1u, 1, 2u, 2, 3u, 3, 5u)
DEMO_JITTER_TASK_DEFINE4(4, 7u, 5, 11u, 6, 13u, 7, 17u)
DEMO_JITTER_TASK_DEFINE4(8, 19u, 9, 23u, 10, 29u, 11, 31u)
DEMO_JITTER_TASK_DEFINE4(12, 37u, 13, 41u, 14, 43u, 15, 47u)
DEMO_JITTER_TASK_DEFINE4(16, 53u, 17, 59u, 18, 61u, 19, 67u)
DEMO_JITTER_TASK_DEFINE4(20, 71u, 21, 73u, 22, 79u, 23, 83u)
DEMO_JITTER_TASK_DEFINE4(24, 89u, 25, 97u, 26, 101u, 27, 103u)
DEMO_JITTER_TASK_DEFINE4(28, 107u, 29, 109u, 30, 113u, 31, 127u)
DEMO_JITTER_TASK_DEFINE4(32, 131u, 33, 137u, 34, 139u, 35, 149u)
DEMO_JITTER_TASK_DEFINE4(36, 151u, 37, 157u, 38, 163u, 39, 167u)
DEMO_JITTER_TASK_DEFINE4(40, 173u, 41, 179u, 42, 181u, 43, 191u)
DEMO_JITTER_TASK_DEFINE4(44, 193u, 45, 197u, 46, 199u, 47, 211u)
DEMO_JITTER_TASK_DEFINE4(48, 223u, 49, 227u, 50, 229u, 51, 233u)
DEMO_JITTER_TASK_DEFINE4(52, 239u, 53, 241u, 54, 251u, 55, 257u)
DEMO_JITTER_TASK_DEFINE4(56, 263u, 57, 269u, 58, 271u, 59, 277u)
DEMO_JITTER_TASK_DEFINE4(60, 281u, 61, 283u, 62, 293u, 63, 307u)
DEMO_JITTER_TASK_DEFINE4(64, 311u, 65, 313u, 66, 317u, 67, 331u)
DEMO_JITTER_TASK_DEFINE4(68, 337u, 69, 347u, 70, 349u, 71, 353u)
DEMO_JITTER_TASK_DEFINE4(72, 359u, 73, 367u, 74, 373u, 75, 379u)
DEMO_JITTER_TASK_DEFINE4(76, 383u, 77, 389u, 78, 397u, 79, 401u)
DEMO_JITTER_TASK_DEFINE4(80, 409u, 81, 419u, 82, 421u, 83, 431u)
DEMO_JITTER_TASK_DEFINE4(84, 433u, 85, 439u, 86, 443u, 87, 449u)
DEMO_JITTER_TASK_DEFINE4(88, 457u, 89, 461u, 90, 463u, 91, 467u)
DEMO_JITTER_TASK_DEFINE4(92, 479u, 93, 487u, 94, 491u, 95, 499u)
DEMO_JITTER_TASK_DEFINE4(96, 503u, 97, 509u, 98, 521u, 99, 523u)
DEMO_JITTER_TASK_DEFINE4(100, 541u, 101, 547u, 102, 557u, 103, 563u)
DEMO_JITTER_TASK_DEFINE4(104, 569u, 105, 571u, 106, 577u, 107, 587u)
DEMO_JITTER_TASK_DEFINE4(108, 593u, 109, 599u, 110, 601u, 111, 607u)
DEMO_JITTER_TASK_DEFINE4(112, 613u, 113, 617u, 114, 619u, 115, 631u)
DEMO_JITTER_TASK_DEFINE4(116, 641u, 117, 643u, 118, 647u, 119, 653u)
DEMO_JITTER_TASK_DEFINE4(120, 659u, 121, 661u, 122, 673u, 123, 677u)
DEMO_JITTER_TASK_DEFINE4(124, 683u, 125, 691u, 126, 701u, 127, 709u)
DEMO_JITTER_TASK_DEFINE4(128, 719u, 129, 727u, 130, 733u, 131, 739u)
DEMO_JITTER_TASK_DEFINE4(132, 743u, 133, 751u, 134, 757u, 135, 761u)
DEMO_JITTER_TASK_DEFINE4(136, 769u, 137, 773u, 138, 787u, 139, 797u)
DEMO_JITTER_TASK_DEFINE4(140, 809u, 141, 811u, 142, 821u, 143, 823u)
DEMO_JITTER_TASK_DEFINE4(144, 827u, 145, 829u, 146, 839u, 147, 853u)
DEMO_JITTER_TASK_DEFINE4(148, 857u, 149, 859u, 150, 863u, 151, 877u)
DEMO_JITTER_TASK_DEFINE4(152, 881u, 153, 883u, 154, 887u, 155, 907u)
DEMO_JITTER_TASK_DEFINE4(156, 911u, 157, 919u, 158, 929u, 159, 937u)
DEMO_JITTER_TASK_DEFINE4(160, 941u, 161, 947u, 162, 953u, 163, 967u)
DEMO_JITTER_TASK_DEFINE4(164, 971u, 165, 977u, 166, 983u, 167, 991u)
DEMO_JITTER_TASK_DEFINE4(168, 997u, 169, 1009u, 170, 1013u, 171, 1019u)
DEMO_JITTER_TASK_DEFINE4(172, 1021u, 173, 1031u, 174, 1033u, 175, 1039u)
DEMO_JITTER_TASK_DEFINE4(176, 1049u, 177, 1051u, 178, 1061u, 179, 1063u)
DEMO_JITTER_TASK_DEFINE4(180, 1069u, 181, 1087u, 182, 1091u, 183, 1093u)
DEMO_JITTER_TASK_DEFINE4(184, 1097u, 185, 1103u, 186, 1109u, 187, 1117u)
DEMO_JITTER_TASK_DEFINE4(188, 1123u, 189, 1129u, 190, 1151u, 191, 1153u)
DEMO_JITTER_TASK_DEFINE4(192, 1163u, 193, 1171u, 194, 1181u, 195, 1187u)
DEMO_JITTER_TASK_DEFINE4(196, 1193u, 197, 1201u, 198, 1213u, 199, 1217u)
DEMO_JITTER_TASK_DEFINE4(200, 1223u, 201, 1229u, 202, 1231u, 203, 1237u)
DEMO_JITTER_TASK_DEFINE4(204, 1249u, 205, 1259u, 206, 1277u, 207, 1279u)
DEMO_JITTER_TASK_DEFINE4(208, 1283u, 209, 1289u, 210, 1291u, 211, 1297u)
DEMO_JITTER_TASK_DEFINE4(212, 1301u, 213, 1303u, 214, 1307u, 215, 1319u)
DEMO_JITTER_TASK_DEFINE4(216, 1321u, 217, 1327u, 218, 1361u, 219, 1367u)
DEMO_JITTER_TASK_DEFINE4(220, 1373u, 221, 1381u, 222, 1399u, 223, 1409u)
DEMO_JITTER_TASK_DEFINE4(224, 1423u, 225, 1427u, 226, 1429u, 227, 1433u)
DEMO_JITTER_TASK_DEFINE4(228, 1439u, 229, 1447u, 230, 1451u, 231, 1453u)
DEMO_JITTER_TASK_DEFINE4(232, 1459u, 233, 1471u, 234, 1481u, 235, 1483u)
DEMO_JITTER_TASK_DEFINE4(236, 1487u, 237, 1489u, 238, 1493u, 239, 1499u)
#endif /* DEMO_JITTER_CAPTURE_ENABLE */

void demo_jitter_timer2_isr_entry(void)
{
#if (DEMO_JITTER_CAPTURE_ENABLE == 1u)
    const uint32_t entry_count = TIMER_CNT(TIMER2) & 0x0000FFFFu;
    const uint32_t index = g_demo_jitter_debug.write_index % DEMO_JITTER_SAMPLE_COUNT;

    g_demo_jitter_timer2_count[index] = entry_count;
    g_demo_jitter_debug.write_index++;
    g_demo_jitter_debug.sample_count = DEMO_JITTER_SAMPLE_COUNT;
    g_demo_jitter_debug.timer2_counter_hz = SystemCoreClock;
    g_demo_jitter_debug.timer2_hz = DEMO_JITTER_TIMER2_HZ;
    g_demo_jitter_debug.timer2_period_ticks = (TIMER_CAR(TIMER2) & 0x0000FFFFu) + 1u;
    g_demo_jitter_debug.last_entry_count = entry_count;

    if ((g_demo_jitter_debug.min_entry_count == 0u) || (entry_count < g_demo_jitter_debug.min_entry_count))
    {
        g_demo_jitter_debug.min_entry_count = entry_count;
    }
    if (entry_count > g_demo_jitter_debug.max_entry_count)
    {
        g_demo_jitter_debug.max_entry_count = entry_count;
        g_demo_jitter_debug.max_entry_timestamp = g_demo_jitter_debug.write_index;
    }

    if ((g_demo_jitter_debug.write_index % DEMO_JITTER_SAMPLE_COUNT) == 0u)
    {
        g_demo_jitter_debug.wrapped_count++;
    }
#endif
}
