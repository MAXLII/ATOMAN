#ifndef __DEMO_H__
#define __DEMO_H__

#include <stdint.h>

/*
 * section framework guide
 *
 * The most important thing in this project is SECTION_E in section.h.
 * Every REG_xxx macro eventually registers one object into a section type,
 * then section_init()/run_task() wire them together at runtime.
 *
 * SECTION_INIT
 *   Macro: REG_INIT(prio, func)
 *   Purpose: one-shot startup initialization.
 *   Parameters:
 *     prio : execution order in section_init(); smaller runs earlier.
 *     func : void func(void), the init callback to execute once.
 *
 * SECTION_TASK
 *   Macro: REG_TASK(...) / REG_TASK_MS(...)
 *   Purpose: periodic background scheduling.
 *   Parameters:
 *     period    : scheduler period in section tick units.
 *     period_ms : scheduler period in milliseconds for REG_TASK_MS.
 *     func      : void func(void), the periodic task body.
 *
 * SECTION_INTERRUPT
 *   Macro: REG_INTERRUPT(priority, func)
 *   Purpose: software-managed interrupt callback chain dispatched by
 *            section_interrupt().
 *   Parameters:
 *     priority : smaller value runs earlier in section_interrupt().
 *     func     : void func(void), interrupt-side callback.
 *
 * SECTION_SHELL
 *   Macro: REG_SHELL_VAR(...) / REG_SHELL_CMD(...)
 *   Purpose: export variables and commands to shell.
 *   REG_SHELL_VAR parameters:
 *     _name   : shell-visible name, for example DEMO_GAIN.
 *     _var    : backing C variable.
 *     _type   : SHELL_UINT8 / SHELL_UINT32 / SHELL_FP32 ...
 *     _max    : upper limit applied on write.
 *     _min    : lower limit applied on write.
 *     _func   : optional callback after write, NULL if not needed.
 *     _status : shell flags, usually SHELL_STA_NULL.
 *   REG_SHELL_CMD parameters:
 *     _name : shell-visible command name.
 *     _func : void func(DEC_MY_PRINTF), command handler.
 *
 * SECTION_LINK
 *   Macro: REG_LINK(...)
 *   Purpose: register one byte-stream link and its handler chain.
 *   Parameters:
 *     link         : logical link id, for example USART0_LINK.
 *     print        : section_link_tx_func_t instance for print/tx.
 *     _rx_get_byte : uint8_t func(uint8_t *p_data), byte source.
 *     _handler_arr : handler table, each item is {func, ctx}.
 *     _handler_num : number of handlers in the table.
 *
 * SECTION_PERF
 *   Macro: REG_PERF_BASE_CNT(...) / REG_PERF_RECORD(...)
 *   Purpose: performance counter and timing records.
 *   Parameters:
 *     timer_cnt : uint32_t * counter base used by PERF_START/END.
 *     name      : record symbol name used by P_RECORD_PERF(name).
 *
 * SECTION_SCOPE
 *   Macro: REG_SCOPE_EX(...)
 *   Purpose: register variables that can be sampled and uploaded as waveforms.
 *
 * SECTION_TRACE
 *   Macro: DBG_TRACE_BIND_TIME(...) / DBG_TRACE_MARK()
 *   Purpose: bind a time base, then mark source lines for binary Trace upload.
 *
 * SECTION_COMM
 *   Macro: REG_COMM(...)
 *   Purpose: dispatch a decoded protocol frame to a command handler.
 *   Parameters:
 *     _cmd_set  : high-level command group.
 *     _cmd_word : command inside that group.
 *     _func     : handler function, signature
 *                 void func(section_packform_t *p_pack, DEC_MY_PRINTF).
 *
 * SECTION_COMM_ROUTE
 *   Macro: REG_COMM_ROUTE(...)
 *   Purpose: forward frames between links by route table.
 *   Parameters:
 *     _src_link_id : source link id.
 *     _dst_link_id : destination link id.
 *     _dst_addr    : destination device address.
 *
 * This demo mainly exercises INIT/TASK/SHELL/COMM/PERF/SCOPE/TRACE/INTERRUPT.
 * LINK is provided by interface/usart.c.
 * COMM_ROUTE is described here as an extension point.
 */

typedef struct
{
    uint32_t counter;
    uint8_t led_mask;
    int16_t temperature_x10;
    uint8_t reserved;
} demo_comm_frame_t;

#define DEMO_CMD_SET_LOOPBACK 0x30u
#define DEMO_CMD_WORD_LOOPBACK 0x01u

#define DEMO_CMD_SET_CONTROL 0x30u
#define DEMO_CMD_WORD_CONTROL 0x02u

#endif
