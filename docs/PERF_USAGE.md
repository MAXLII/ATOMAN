# Perf 使用文档

## 1. 适用范围

本文档说明如何在当前工程中使用 Perf 测量任务、代码段和中断耗时。

内部实现见 [PERF_DESIGN.md](PERF_DESIGN.md)。

## 2. 平台时间基准

每个 MCU 平台需要先提供硬件计数器：

```c
#include "perf.h"

REG_PERF_BASE_CNT(timer_cnt, period_s)
```

参数说明：

| 参数 | 说明 |
| --- | --- |
| `timer_cnt` | 硬件计数寄存器地址 |
| `period_s` | 一个计数 tick 的实际秒数 |

例子：

```c
REG_PERF_BASE_CNT(&TIMER6->CNT, 0.1e-6f)
```

`period_s` 必须按实际定时器时钟和分频计算，不按期望值填写。

## 3. 手动测量代码段

在 `.c` 文件中注册 record：

```c
REG_PERF_RECORD(my_code)
```

在需要测量的地方成对调用：

```c
PERF_START(my_code);
do_something();
PERF_END(my_code);
```

record 名称会显示为 `my_code`。

## 4. 任务和中断测量

任务 record 通常由 `section` 任务注册宏自动创建或绑定。需要手动声明时使用：

```c
REG_TASK_PERF_RECORD(my_task)
```

中断阶段使用：

```c
REG_INTERRUPT_PERF_RECORD(my_interrupt)
```

调度层会在任务或中断执行前后调用 Perf begin/end 接口。

## 5. Shell 查看

如果 Shell 字符串解析可用，可以使用以下命令：

```text
perf_info
perf_summary
CPU_Utilization
perf_print_record
perf_print_task
perf_print_interrupt
perf_print_code
perf_reset_peak
```

常用 Shell 变量：

```text
TASK_METRIC
TASK_METRIC_MAX
INTERRUPT_METRIC
INTERRUPT_METRIC_MAX
```

## 6. 上位机查看

Perf Viewer 通过二进制协议访问：

1. 查询 `perf_info`，获取协议版本、计时单位和 record 数量。
2. 拉取字典，建立 `record_id` 与名称/类型的对应关系。
3. 周期拉取采样数据。
4. 需要时发送峰值清零。

二进制访问不依赖字符串 Shell 是否开启。

## 7. 使用注意事项

- `PERF_START()` 和 `PERF_END()` 必须成对出现。
- 同一个手动 record 不适合嵌套使用。
- 被测代码越短，测量开销占比越高。
- 平台必须注册正确的 `period_s`，否则显示时间会整体成比例错误。
- 中断测量会从正在运行的任务累计时间中扣除中断耗时。
- `perf_reset_peak` 只清峰值，不删除 record。
