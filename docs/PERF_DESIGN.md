# Perf 设计文档

## 1. 模块定位

Perf 模块用于记录任务、代码段和中断阶段的运行时间，并把结果通过 Shell 或二进制通信协议提供给上位机。

当前实现分为两层：

| 层级 | 文件 | 职责 |
| --- | --- | --- |
| Perf 内核 | `code/dbg/perf.c`、`code/dbg/perf.h` | 时间基准注册、record 注册、耗时累计、负载统计 |
| Perf 服务 | `code/dbg/perf_service.c`、`code/dbg/perf_service.h` | Shell 打印、字典上报、采样上报、峰值清零 |

Perf 不直接初始化硬件定时器。平台 BSP 负责配置一个自由运行计数器，并通过 `REG_PERF_BASE_CNT()` 注册计数寄存器地址和真实 tick 周期。

## 2. 时间基准

时间基准使用 `section_perf_base_t` 描述：

| 字段 | 含义 |
| --- | --- |
| `p_cnt` | 硬件计数寄存器指针 |
| `cnt_period_s` | 一个硬件计数 tick 的真实秒数 |

注册宏：

```c
REG_PERF_BASE_CNT(timer_cnt, period_s)
```

`period_s` 由平台实际时钟树和定时器分频计算得到。例如 0.1 us 写作 `0.1e-6f`。

如果没有注册硬件计数器，Perf record 仍会被扫描进链表，但 `PERF_START/PERF_END` 和任务/中断自动测量无法得到有效硬件时间。

## 3. Record 模型

Perf 使用 `section_perf_record_t` 表示一个测量对象。

| 字段 | 含义 |
| --- | --- |
| `p_name` | record 名称 |
| `start` / `end` | 最近一次开始和结束计数值 |
| `time` | 最近一次运行耗时，单位为硬件 tick |
| `max_time` | 峰值耗时，单位为硬件 tick |
| `run_time` | 当前统计窗口累计耗时 |
| `period_us` | 任务周期，任务 record 使用 |
| `load` / `load_max` | 当前负载和峰值负载 |
| `record_id` | 字典 ID |
| `record_type` | `CODE` / `TASK` / `INTERRUPT` |
| `p_cnt` | 指向当前 Perf 时间基准指针 |
| `p_next` | 运行时链表指针 |

record 类型：

| 类型 | 注册宏 | 用途 |
| --- | --- | --- |
| `SECTION_PERF_RECORD_CODE` | `REG_PERF_RECORD()` | 手动代码段测量 |
| `SECTION_PERF_RECORD_TASK` | `REG_TASK_PERF_RECORD()` | 任务执行时间测量 |
| `SECTION_PERF_RECORD_INTERRUPT` | `REG_INTERRUPT_PERF_RECORD()` | 中断阶段测量 |

## 4. 初始化流程

`perf_init()` 通过 `REG_INIT(0, perf_init)` 注册。

初始化时会：

1. 清空 record 链表和统计值。
2. 初始化 record 字典。
3. 扫描 `SECTION_PERF`。
4. 先记录时间基准，再把 record 插入链表。
5. 给每个 record 分配 `record_id`。
6. 把 record 的 `p_cnt` 指向全局时间基准指针。

`perf_dict_version` 用于上位机判断 record 字典是否变化。

## 5. 测量方式

手动代码段测量使用：

```c
REG_PERF_RECORD(my_code)

PERF_START(my_code);
/* measured code */
PERF_END(my_code);
```

`PERF_START()` 读取当前硬件计数到 `start`。`PERF_END()` 再次读取计数，使用无符号减法得到 delta，写入 `time`，更新 `max_time`，并累计到 `run_time`。

任务和中断测量由 `section` 调度层调用 `section_perf_task_begin/end()` 和 `section_perf_interrupt_begin/end()`。中断结束时，如果当前存在正在运行的任务 record，会把中断耗时从任务累计时间中扣除，避免任务负载包含中断时间。

## 6. 负载统计

`perf_cpu_load_calculate()` 通过 `REG_TASK_MS(PERF_CPU_LOAD_PERIOD_MS, ...)` 周期运行，默认统计周期为 500 ms。

统计流程：

1. 计算当前统计窗口内经过的系统 tick。
2. 使用 `perf_cnt_per_sys_tick_get()` 换算为硬件计数窗口。
3. 遍历所有 record，计算单个 record 的 `load` 和 `load_max`。
4. 汇总任务 record 得到任务总负载。
5. 汇总中断 record 得到中断总负载。
6. 清零每个 record 的 `run_time`，进入下一个统计窗口。

`perf_count_to_us()` 使用注册的硬件 tick 周期把计数换算为 us。

## 7. Shell 服务

Perf 服务注册了以下 Shell 命令：

| 命令 | 说明 |
| --- | --- |
| `perf_print_record` | 分步打印全部 record |
| `perf_print_task` | 打印任务 record |
| `perf_print_interrupt` | 打印中断 record |
| `perf_print_code` | 打印代码段 record |
| `CPU_Utilization` | 打印任务和中断总负载 |
| `perf_summary` | 打印汇总信息 |
| `perf_info` | 打印计时单位、系统 tick 换算、record 数量 |
| `perf_reset_peak` | 清除峰值 |

同时注册了以下 Shell 变量：

| 变量 | 说明 |
| --- | --- |
| `TASK_METRIC` | 当前任务总负载 |
| `TASK_METRIC_MAX` | 任务总负载峰值 |
| `INTERRUPT_METRIC` | 当前中断总负载 |
| `INTERRUPT_METRIC_MAX` | 中断总负载峰值 |

## 8. 二进制服务

Perf 二进制服务使用 `cmd_set = 0x01`，命令如下：

| 命令 | cmd_word | 说明 |
| --- | --- | --- |
| 信息查询 | `0x20` | 查询协议版本、record 数量、计时单位 |
| 汇总查询 | `0x21` | 查询任务/中断总负载 |
| 峰值清零 | `0x25` | 清除耗时和负载峰值 |
| 字典查询 | `0x26` | 启动 record 字典上报 |
| 字典项上报 | `0x27` | 分帧上报 record 名称和类型 |
| 字典结束 | `0x28` | 字典上报结束 |
| 采样查询 | `0x29` | 启动 record 当前值上报 |
| 采样批量上报 | `0x2A` | 分批上报 record 耗时/负载 |
| 采样结束 | `0x2B` | 采样上报结束 |
| 上报控制 | `0x2E` | 取消当前上报 |

`perf_opt_poll_task()` 每 10 ms 运行一次，负责推进字典和采样传输，并同步 Shell 变量中的负载值。

## 9. 当前约束

- 不使用动态内存。
- 计时精度由平台注册的硬件计数器决定。
- record 名称来自注册宏参数。
- 二进制字典和采样一次只允许一个活跃传输上下文。
- `PERF_RECORD_ENABLE == 0` 时，record 和测量宏会被裁剪为空。
- 平台需要保证硬件计数器自由运行，并允许无符号差值处理回绕。
