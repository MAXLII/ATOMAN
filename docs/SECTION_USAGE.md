# Section 使用文档

## 1. 适用范围

本文档说明如何使用 Section 自动注册初始化函数、周期任务、中断回调、状态机和通信链路。

内部实现见 [SECTION_DESIGN.md](SECTION_DESIGN.md)。

## 2. 初始化调用

系统启动时需要调用：

```c
section_init();
```

主循环中需要持续调用：

```c
while (1)
{
    run_task();
}
```

硬件中断中可以按需要调用：

```c
void Timer_IRQHandler(void)
{
    section_interrupt();
}
```

`section_interrupt()` 使用 `FUNC_RAM` 修饰，平台工程需要保证 `.func_ram` 段按目标 MCU 的方式放到 RAM 中运行。

## 3. 注册初始化函数

使用：

```c
REG_INIT(priority, func)
```

示例：

```c
static void demo_init(void)
{
    /* module init */
}

REG_INIT(0, demo_init)
```

`priority` 越小越早执行。

## 4. 注册周期任务

使用系统 tick 为单位：

```c
REG_TASK(period_tick, func)
```

使用毫秒为单位：

```c
REG_TASK_MS(period_ms, func)
```

示例：

```c
static void demo_task_10ms(void)
{
    /* periodic work */
}

REG_TASK_MS(10, demo_task_10ms)
```

任务函数不能长时间阻塞，否则会影响其他任务和链路轮询。

## 5. 注册中断回调

使用：

```c
REG_INTERRUPT(priority, func)
```

示例：

```c
static void demo_fast_ctrl(void)
{
    /* fast control */
}

REG_INTERRUPT(0, demo_fast_ctrl)
```

硬件 ISR 中调用 `section_interrupt()` 后，所有注册中断回调会按优先级执行。

## 6. 注册状态机

事件变量必须是 `uint32_t` 大小：

```c
typedef enum
{
    DEMO_IDLE = 1,
    DEMO_RUN,
    DEMO_FAULT,
} demo_state_t;

static uint32_t s_demo_event;
```

状态处理函数：

```c
static void demo_idle_in(void) {}
static void demo_idle_exe(void) {}
static uint32_t demo_idle_chk(uint32_t ev)
{
    if (ev == 1u)
    {
        return DEMO_RUN;
    }
    return DEMO_IDLE;
}
static void demo_idle_out(void) {}
```

注册：

```c
REG_FSM(demo,
        DEMO_IDLE,
        s_demo_event,
        FSM_ENTRY(DEMO_IDLE, demo_idle_in, demo_idle_exe, demo_idle_chk, demo_idle_out),
        FSM_ENTRY(DEMO_RUN, NULL, demo_run_exe, demo_run_chk, NULL),
        FSM_ENTRY(DEMO_FAULT, NULL, demo_fault_exe, NULL, NULL))
```

触发事件：

```c
s_demo_event = 1u;
```

读取当前状态：

```c
uint32_t state = FSM_GET_STATE(demo);
```

## 7. 注册通信链路

链路接收函数原型：

```c
static uint8_t demo_rx_get_byte(uint8_t *p_data)
{
    /* 有数据返回 1，无数据返回 0 */
}
```

发送接口：

```c
static section_link_tx_func_t demo_print = {
    .my_printf = demo_printf,
    .tx_by_dma = demo_tx_by_dma,
};
```

handler 数组：

```c
static const section_link_handler_item_t demo_handlers[] = {
    {.func = shell_run, .ctx = (void *)&s_shell_ctx},
    {.func = comm_run, .ctx = NULL},
};
```

注册链路：

```c
REG_LINK(0, demo_print, demo_rx_get_byte, demo_handlers, ARRAY_SIZE(demo_handlers))
```

每个收到的字节会依次送入 handler 数组。handler 需要自己维护协议解析状态。

## 8. 使用 Perf 自动测量

默认情况下，`REG_TASK()` 和 `REG_INTERRUPT()` 会为任务和中断回调自动绑定 Perf record。

如需关闭：

```c
#define TASK_RECORD_PERF_ENABLE 0
#define INTERRUPT_RECORD_PERF_ENABLE 0
```

这些宏需要在包含 `section.h` 之前生效。

`INTERRUPT_RECORD_PERF_ENABLE` 会影响 `section_interrupt()` 的编译路径。关闭后，中断调度不会调用 Perf begin/end 函数，也不会增加运行时判断。

## 9. 使用注意事项

- 注册宏通常放在模块 `.c` 文件中。
- 注册对象为静态生命周期，不需要手动释放。
- `REG_TASK_MS()` 的周期单位是 ms。
- `REG_TASK()` 的周期单位是 `SECTION_SYS_TICK`。
- 任务函数应短小，避免阻塞。
- 中断回调应只做中断安全的操作。
- 链路 handler 按字节被调用，不能假设一次收到完整帧。
- 新平台需要保证链接段和 `SECTION_START/SECTION_STOP` 符号正确。
