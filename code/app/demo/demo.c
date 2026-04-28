#include "demo.h"

#include "comm.h"
#include "comm_addr.h"
#include "gpio.h"
#include "perf.h"
#include "scope.h"
#include "section.h"
#include "shell.h"
#include "usart.h"

#include <math.h>
#include <string.h>

/*
 * This file is a runnable section-framework demo.
 *
 * Read this file with SECTION_E in mind:
 * - SECTION_INIT       -> REG_INIT
 * - SECTION_TASK       -> REG_TASK_MS
 * - SECTION_SHELL      -> REG_SHELL_VAR / REG_SHELL_CMD
 * - SECTION_COMM       -> REG_COMM
 * - SECTION_LINK       -> provided by interface/usart.c
 * - SECTION_INTERRUPT  -> extension point, not used in this demo
 * - SECTION_PERF       -> REG_PERF_RECORD / PERF_START / PERF_END
 * - SECTION_COMM_ROUTE -> extension point, not used in this demo
 *
 * Runtime path:
 * main() -> section_init() -> REG_INIT callbacks
 * main() loop -> run_task() -> registered tasks
 * USART RX -> section link task -> shell_run()/comm_run()
 */

EXT_LINK(USART0_LINK);

static uint8_t s_demo_led_mask = 0x01u;
static uint32_t s_demo_counter = 0u;
static int32_t s_demo_last_cmd = 0;
static float s_demo_gain = 1.0f;
static uint8_t s_demo_init_count = 0u;
static uint8_t s_demo_boot_ready = 0u;
static uint32_t s_demo_perf_spin = 2000u;
static uint32_t s_demo_last_tick = 0u;
static uint32_t s_demo_perf_acc = 0u;
static uint32_t s_demo_period_counter = 0u;
static uint32_t s_demo_scope_fast_tick = 0u;
static uint32_t s_demo_scope_slow_tick = 0u;
extern section_perf_record_t section_perf_record_demo_task;
static demo_comm_frame_t s_demo_last_frame = {
    .counter = 0u,
    .led_mask = 0x01u,
    .temperature_x10 = 250,
    .reserved = 0u,
};

REG_PERF_RECORD(demo_manual_perf);
REG_PERF_RECORD(demo_period_perf);

#if defined(IS_HC32)
#define DEMO_SCOPE_FAST_BUF_SIZE 64
#define DEMO_SCOPE_FAST_TRIG_POST_CNT 32
#define DEMO_SCOPE_SLOW_BUF_SIZE 64
#define DEMO_SCOPE_SLOW_TRIG_POST_CNT 32
#else
#define DEMO_SCOPE_FAST_BUF_SIZE 500
#define DEMO_SCOPE_FAST_TRIG_POST_CNT 250
#define DEMO_SCOPE_SLOW_BUF_SIZE 200
#define DEMO_SCOPE_SLOW_TRIG_POST_CNT 100
#endif

/* 100 us scope example: 500 samples and 10 signal channels. */
REG_SCOPE_EX(demo_scope_fast, DEMO_SCOPE_FAST_BUF_SIZE, DEMO_SCOPE_FAST_TRIG_POST_CNT, 100u,
          scope_sin,
          scope_cos,
          scope_sin2,
          scope_cos2,
          scope_ramp,
          scope_triangle,
          scope_square,
          scope_mix,
          scope_saw,
          scope_index)

/* 1 ms scope example: 200 samples and 3 signal channels. */
REG_SCOPE_EX(demo_scope_slow, DEMO_SCOPE_SLOW_BUF_SIZE, DEMO_SCOPE_SLOW_TRIG_POST_CNT, 1000u,
             scope_slow_sin,
             scope_slow_cos,
             scope_slow_mix)

static void demo_apply_led_mask(uint8_t led_mask)
{
    gpio_set_main_rly_sta((uint8_t)((led_mask >> 0) & 0x01u));
    gpio_set_ss_rly_sta((uint8_t)((led_mask >> 1) & 0x01u));
    gpio_set_ac_in_rly_sta((uint8_t)((led_mask >> 2) & 0x01u));
    gpio_set_ac_out_rly_sta((uint8_t)((led_mask >> 3) & 0x01u));
}

/*
 * SECTION_INIT / REG_INIT example 1:
 * Register an early boot callback. Smaller priority runs earlier.
 */
static void demo_init_defaults(void)
{
    s_demo_counter = 0u;
    s_demo_last_cmd = 0;
    s_demo_gain = 1.0f;
    s_demo_led_mask = 0x01u;
    s_demo_boot_ready = 0u;
    s_demo_perf_spin = 2000u;
    s_demo_last_tick = 0u;
    s_demo_perf_acc = 0u;
    s_demo_scope_fast_tick = 0u;
    s_demo_scope_slow_tick = 0u;
    s_demo_last_frame.counter = 0u;
    s_demo_last_frame.led_mask = s_demo_led_mask;
    s_demo_last_frame.temperature_x10 = 250;
    s_demo_last_frame.reserved = 0u;
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_init_count++;
}

/*
 * SECTION_INIT / REG_INIT example 2:
 * A later init item can depend on earlier state and print a boot banner.
 */
static void demo_init_banner(void)
{
    section_link_tx_func_t *p_link_printf = LINK_PRINTF(USART0_LINK);

    s_demo_boot_ready = 1u;
    s_demo_init_count++;

    if ((p_link_printf != NULL) && (p_link_printf->my_printf != NULL))
    {
        p_link_printf->my_printf("demo init done: init_count=%u led_mask=0x%02X\r\n",
                                 (unsigned)s_demo_init_count,
                                 s_demo_led_mask);
    }
}

static void demo_led_mask_changed(DEC_MY_PRINTF)
{
    demo_apply_led_mask(s_demo_led_mask);

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo led mask updated: 0x%02X\r\n", s_demo_led_mask);
    }
}

static void demo_ping_cmd(DEC_MY_PRINTF)
{
    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo ping ok\r\n");
        my_printf->my_printf("counter=%lu led_mask=0x%02X gain=%0.2f temp_x10=%d init_count=%u boot_ready=%u tick=%lu\r\n",
                             (unsigned long)s_demo_counter,
                             s_demo_led_mask,
                             (double)s_demo_gain,
                             s_demo_last_frame.temperature_x10,
                             (unsigned)s_demo_init_count,
                             (unsigned)s_demo_boot_ready,
                             (unsigned long)s_demo_last_tick);
    }
}

/*
 * SECTION_SHELL / REG_SHELL_CMD example:
 * Print the section framework map instead of business-only demo content.
 */
static void demo_help_cmd(DEC_MY_PRINTF)
{
    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("SECTION_E guide:\r\n");
        my_printf->my_printf("INIT       -> REG_INIT(prio, func)\r\n");
        my_printf->my_printf("TASK       -> REG_TASK/REG_TASK_MS\r\n");
        my_printf->my_printf("INTERRUPT  -> REG_INTERRUPT(priority, func)\r\n");
        my_printf->my_printf("SHELL      -> REG_SHELL_VAR / REG_SHELL_CMD\r\n");
        my_printf->my_printf("LINK       -> REG_LINK(rx, handlers, tx)\r\n");
        my_printf->my_printf("PERF       -> REG_PERF_BASE_CNT / REG_PERF_RECORD\r\n");
        my_printf->my_printf("COMM       -> REG_COMM(cmd_set, cmd_word, func)\r\n");
        my_printf->my_printf("COMM_ROUTE -> REG_COMM_ROUTE(src_link, dst_link, dst_addr)\r\n");
        my_printf->my_printf("demo uses  -> INIT TASK SHELL PERF COMM; LINK in interface/usart.c\r\n");
        my_printf->my_printf("PERF use 1 -> REG_PERF_RECORD(demo_manual_perf) + PERF_START/END\r\n");
        my_printf->my_printf("PERF use 2 -> REG_TASK_MS(10, demo_task) auto record when TASK_RECORD_PERF_ENABLE=1\r\n");
        my_printf->my_printf("PERF use 3 -> demo_period_task ends perf at entry, starts perf at exit\r\n");
        my_printf->my_printf("SCOPE fast -> REG_SCOPE_EX(demo_scope_fast, 500, 250, 100, ...10 vars)\r\n");
        my_printf->my_printf("SCOPE slow -> REG_SCOPE_EX(demo_scope_slow, 200, 100, 1000, ...3 vars)\r\n");
        my_printf->my_printf("SCOPE run  -> REG_TASK(1, demo_scope_fast_task) and REG_TASK_MS(1, demo_scope_slow_task)\r\n");
        my_printf->my_printf("shell vars -> DEMO_LED_MASK DEMO_COUNTER DEMO_LAST_CMD DEMO_GAIN DEMO_PERF_SPIN\r\n");
        my_printf->my_printf("shell cmds -> DEMO_PING DEMO_HELP DEMO_SEND DEMO_PERF DEMO_TICK\r\n");
        my_printf->my_printf("examples   -> DEMO_PERF | DEMO_TICK | DEMO_SEND\r\n");
    }
}

/*
 * SECTION_PERF / REG_PERF_RECORD example 1:
 * Manually wrap one code section with PERF_START/PERF_END to measure its cost.
 */
static void demo_perf_cmd(DEC_MY_PRINTF)
{
    section_perf_record_t *manual_rec = P_RECORD_PERF(demo_manual_perf);
    section_perf_record_t *task_rec = P_RECORD_PERF(demo_task);
    section_perf_record_t *period_rec = P_RECORD_PERF(demo_period_perf);
    volatile uint32_t acc = 0u;
    uint32_t i;

    PERF_START(demo_manual_perf);
    for (i = 0u; i < s_demo_perf_spin; ++i)
    {
        acc += (i ^ 0x5A5A5A5Au) + (acc >> 1);
    }
    PERF_END(demo_manual_perf);

    s_demo_perf_acc = acc;

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("perf manual: spin=%lu last=%u max=%lu start=%u end=%u acc=%lu\r\n",
                             (unsigned long)s_demo_perf_spin,
                             (unsigned)((manual_rec != NULL) ? manual_rec->time : 0u),
                             (unsigned long)((manual_rec != NULL) ? manual_rec->max_time : 0u),
                             (unsigned)((manual_rec != NULL) ? manual_rec->start : 0u),
                             (unsigned)((manual_rec != NULL) ? manual_rec->end : 0u),
                             (unsigned long)s_demo_perf_acc);
        my_printf->my_printf("perf task  : last=%u max=%lu name=%s\r\n",
                             (unsigned)((task_rec != NULL) ? task_rec->time : 0u),
                             (unsigned long)((task_rec != NULL) ? task_rec->max_time : 0u),
                             (task_rec != NULL) ? task_rec->p_name : "null");
        my_printf->my_printf("perf period: last=%u max=%lu name=%s count=%lu\r\n",
                             (unsigned)((period_rec != NULL) ? period_rec->time : 0u),
                             (unsigned long)((period_rec != NULL) ? period_rec->max_time : 0u),
                             (period_rec != NULL) ? period_rec->p_name : "null",
                             (unsigned long)s_demo_period_counter);
    }
}

/*
 * SECTION_PERF / REG_PERF_BASE_CNT example 2:
 * Show the raw free-running timer used as the performance counter base.
 */
static void demo_tick_cmd(DEC_MY_PRINTF)
{
    s_demo_last_tick = perf_base_cnt_get();

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("perf base tick=%lu\r\n", (unsigned long)s_demo_last_tick);
    }
}

/*
 * SECTION_SHELL / REG_SHELL_CMD example:
 * A shell command can also be the trigger that sends one COMM frame.
 */
static void demo_send_loopback_cmd(DEC_MY_PRINTF)
{
    section_packform_t pack = {0};
    demo_comm_frame_t frame = s_demo_last_frame;

    frame.counter = s_demo_counter;
    frame.led_mask = s_demo_led_mask;

    pack.src = LOCAL_ADDR;
    pack.dst = PC_ADDR;
    pack.cmd_set = DEMO_CMD_SET_LOOPBACK;
    pack.cmd_word = DEMO_CMD_WORD_LOOPBACK;
    pack.is_ack = 0u;
    pack.len = sizeof(frame);
    pack.p_data = (uint8_t *)&frame;

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo send loopback frame\r\n");
    }

    comm_send_data(&pack, LINK_PRINTF(USART0_LINK));
}

/*
 * SECTION_TASK / REG_TASK_MS example:
 * run_task() dispatches this periodic task in the background.
 */
static void demo_task(void)
{
    static uint8_t led_phase = 0u;

    s_demo_counter++;
    s_demo_last_tick = perf_base_cnt_get();

    if ((s_demo_counter % 500u) == 0u)
    {
        led_phase = (uint8_t)((led_phase + 1u) & 0x03u);
        s_demo_led_mask = (uint8_t)(1u << led_phase);
        s_demo_last_frame.counter = s_demo_counter;
        s_demo_last_frame.led_mask = s_demo_led_mask;
        demo_apply_led_mask(s_demo_led_mask);
    }
}

/*
 * SECTION_PERF / REG_TASK_MS period example:
 * End the performance record at task entry and start it again at task exit.
 * This measures the interval between the end of the previous execution and the
 * beginning of the current execution, which is useful for observing scheduler
 * cadence.
 */
static void demo_period_task(void)
{
    static uint8_t s_period_started = 0u;
    volatile uint32_t delay = 0u;

    if (s_period_started != 0u)
    {
        PERF_END(demo_period_perf);
    }

    s_demo_period_counter++;

    for (delay = 0u; delay < 64u; ++delay)
    {
        __NOP();
    }

    PERF_START(demo_period_perf);
    s_period_started = 1u;
}

/*
 * Fast scope example:
 * - REG_TASK(1, ...) means one scheduler tick, which is 100 us in this project.
 * - Ten float channels are updated every 100 us.
 * - The scope is auto-triggered on a rising zero crossing of scope_sin.
 */
static void demo_scope_fast_task(void)
{
    float theta;
    float theta_fast;
    uint32_t phase500;

    theta = 2.0f * 3.1415926f * (float)(s_demo_scope_fast_tick % 200u) / 200.0f;
    theta_fast = 2.0f * 3.1415926f * (float)(s_demo_scope_fast_tick % 50u) / 50.0f;
    phase500 = s_demo_scope_fast_tick % 500u;

    scope_sin = sinf(theta);
    scope_cos = cosf(theta);
    scope_sin2 = 0.6f * sinf(theta_fast);
    scope_cos2 = 0.6f * cosf(theta_fast);
    scope_ramp = (float)phase500 / 499.0f;
    scope_triangle = (phase500 < 250u)
                         ? ((float)phase500 / 249.0f)
                         : ((float)(499u - phase500) / 249.0f);
    scope_square = (scope_sin >= 0.0f) ? 1.0f : -1.0f;
    scope_mix = 0.7f * scope_sin + 0.2f * scope_cos2 + 0.1f * scope_triangle;
    scope_saw = ((float)(s_demo_scope_fast_tick % 100u) / 99.0f) * 2.0f - 1.0f;
    scope_index = (float)(s_demo_scope_fast_tick % 1000u);

    SCOPE_RUN(demo_scope_fast);
    s_demo_scope_fast_tick++;
}

/*
 * Slow scope example:
 * - Three float channels are updated every 1 ms.
 * - The scope uses a 200-sample buffer.
 * - Trigger condition is the rising zero crossing of scope_slow_sin.
 */
static void demo_scope_slow_task(void)
{
    float theta;

    theta = 2.0f * 3.1415926f * (float)(s_demo_scope_slow_tick % 100u) / 100.0f;

    scope_slow_sin = sinf(theta);
    scope_slow_cos = cosf(theta);
    scope_slow_mix = 0.8f * scope_slow_sin + 0.2f * cosf(theta * 0.25f);

    SCOPE_RUN(demo_scope_slow);
    s_demo_scope_slow_tick++;
}

/*
 * SECTION_COMM / REG_COMM example 1:
 * Handle one decoded loopback frame and send an ACK.
 */
static void demo_loopback_comm(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    section_packform_t ack = {0};

    if ((p_pack == NULL) || (p_pack->len != sizeof(demo_comm_frame_t)))
    {
        if (my_printf && my_printf->my_printf)
        {
            my_printf->my_printf("demo loopback invalid len=%u\r\n",
                                 (unsigned)((p_pack != NULL) ? p_pack->len : 0u));
        }
        return;
    }

    memcpy(&s_demo_last_frame, p_pack->p_data, sizeof(s_demo_last_frame));
    s_demo_led_mask = s_demo_last_frame.led_mask;
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_last_cmd = (int32_t)((DEMO_CMD_SET_LOOPBACK << 8) | DEMO_CMD_WORD_LOOPBACK);

    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = sizeof(s_demo_last_frame);
    ack.p_data = (uint8_t *)&s_demo_last_frame;
    comm_send_data(&ack, my_printf);
}

/*
 * SECTION_COMM / REG_COMM example 2:
 * Accept a short control payload, update local state, and return an ACK.
 */
static void demo_control_comm(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    uint8_t ack_data[2];
    section_packform_t ack = {0};

    if ((p_pack == NULL) || (p_pack->len < 2u))
    {
        return;
    }

    s_demo_led_mask = p_pack->p_data[0];
    s_demo_last_frame.temperature_x10 = (int16_t)((int16_t)p_pack->p_data[1] * 10);
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_last_cmd = (int32_t)((DEMO_CMD_SET_CONTROL << 8) | DEMO_CMD_WORD_CONTROL);

    ack_data[0] = s_demo_led_mask;
    ack_data[1] = (uint8_t)(s_demo_last_frame.temperature_x10 / 10);

    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = sizeof(ack_data);
    ack.p_data = ack_data;
    comm_send_data(&ack, my_printf);
}

/*
 * REG_INIT(prio, func)
 *   prio : smaller executes earlier during section_init()
 *   func : init function, signature is void func(void)
 *
 * Example below:
 *   REG_INIT(-10, demo_init_defaults) -> early default state
 *   REG_INIT(10, demo_init_banner)    -> later banner print
 */
REG_INIT(-10, demo_init_defaults)
REG_INIT(10, demo_init_banner)

/*
 * REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)
 *   _name   : shell command name string generated from token
 *   _var    : backing variable address
 *   _type   : shell data type, such as SHELL_UINT8/SHELL_UINT32/SHELL_FP32
 *   _max    : upper limit for writes from shell/comm
 *   _min    : lower limit for writes from shell/comm
 *   _func   : optional callback after value changes
 *   _status : flag bits, commonly SHELL_STA_NULL
 *
 * Typical usage:
 *   REG_SHELL_VAR(DEMO_GAIN, s_demo_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
 */
REG_SHELL_VAR(DEMO_LED_MASK, s_demo_led_mask, SHELL_UINT8, 0x0F, 0, demo_led_mask_changed, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_COUNTER, s_demo_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_LAST_CMD, s_demo_last_cmd, SHELL_INT32, 0x7FFFFFFF, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_GAIN, s_demo_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_PERF_SPIN, s_demo_perf_spin, SHELL_UINT32, 200000u, 1u, NULL, SHELL_STA_NULL)

/*
 * REG_SHELL_CMD(_name, _func)
 *   _name : shell command name string generated from token
 *   _func : command handler, signature is void func(DEC_MY_PRINTF)
 *
 * Typical usage:
 *   REG_SHELL_CMD(DEMO_PERF, demo_perf_cmd)
 */
REG_SHELL_CMD(DEMO_PING, demo_ping_cmd)
REG_SHELL_CMD(DEMO_HELP, demo_help_cmd)
REG_SHELL_CMD(DEMO_PERF, demo_perf_cmd)
REG_SHELL_CMD(DEMO_SEND, demo_send_loopback_cmd)
REG_SHELL_CMD(DEMO_TICK, demo_tick_cmd)

/*
 * REG_COMM(_cmd_set, _cmd_word, _func)
 *   _cmd_set  : protocol command set
 *   _cmd_word : protocol command word inside that set
 *   _func     : handler, signature is
 *               void func(section_packform_t *p_pack, DEC_MY_PRINTF)
 *
 * Typical usage:
 *   REG_COMM(0x30, 0x01, demo_loopback_comm)
 */
REG_COMM(DEMO_CMD_SET_LOOPBACK, DEMO_CMD_WORD_LOOPBACK, demo_loopback_comm)
REG_COMM(DEMO_CMD_SET_CONTROL, DEMO_CMD_WORD_CONTROL, demo_control_comm)

/*
 * REG_TASK_MS(period_ms, func)
 *   period_ms : execution period in milliseconds
 *   func      : task body, signature is void func(void)
 *
 * When TASK_RECORD_PERF_ENABLE == 1, this macro also auto-creates
 * section_perf_record_<func>, so the task execution time can be queried by
 * P_RECORD_PERF(func).
 */
REG_TASK_MS(10, demo_task)
REG_TASK_MS(20, demo_period_task)
REG_TASK(1, demo_scope_fast_task)
REG_TASK_MS(1, demo_scope_slow_task)
