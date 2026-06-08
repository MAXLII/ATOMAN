# 工程设计文档

## 1. 工程概述

本工程为数字电源框架项目，当前包含四个目标平台工程：

| 目录 | 说明 |
|------|------|
| `hc32f334/` | HDSC HC32F334 AC/DC 平台 |
| `gd32g553c/` | GigaDevice GD32G553C 平台 |
| `apm32/` | Geehy APM32F402/403 平台 |
| `plecs/` | PLECS 仿真模型 |

共享代码位于 `code/` 目录，各平台工程通过相对路径引用。

## 2. 目录结构

```
code/
├── app/           应用层（PFC、LLC、逆变器、AC 等）
├── comm/          通信模块
├── ctrl/          控制算法（PFC、逆变器、BB 等）
├── dbg/           调试与监控模块
├── interface/     硬件接口层
├── lib/           控制算法库（PID、SOGI、锁相环等）
└── section/       模块注册与链接框架
```

## 3. 模块注册与链接框架（`code/section/`）

### 3.1 设计目标

`code/section/` 框架解决嵌入式系统中模块自动注册和生命周期管理问题。各模块通过编译时链接器 section 机制将自身注册到框架中，运行时框架遍历注册链表完成初始化和调度，全程不使用动态内存分配。

### 3.2 注册机制

框架定义了 `reg_section_t` 统一描述符，包含 `section_type` 和 `p_str` 两个字段。每个模块使用 `REG_SECTION_FUNC` 宏将描述符放入 `.section` 段。链接脚本在 `.section` 段首尾放置 `SECTION_START` / `SECTION_STOP` 符号：

```
[ SECTION_START | reg_section_t | reg_section_t | ... | reg_section_t | SECTION_STOP ]
```

`AUTO_REG_SECTION` 属性将描述符放置在 `.section` 段。`section_init()` 在 `main()` 中调用，遍历 `SECTION_START` 到 `SECTION_STOP` 之间的所有描述符，按 `section_type` 分派到对应的有序链表：

| 分段类型 | 运行时结构 | 插入方式 |
|---------|----------|---------|
| `SECTION_INIT` | `reg_init_t` 链表 | 按 `priority` 升序，同优先级保持编译顺序 |
| `SECTION_TASK` | `reg_task_t` 链表 | 按编译顺序追加到尾部 |
| `SECTION_INTERRUPT` | `reg_interrupt_t` 链表 | 按 `priority` 升序 |
| `SECTION_LINK` | `section_link_t` 链表 | 按编译顺序追加 |
| `SECTION_SHELL` | `section_shell_t` 链表 | 由 `shell_init` 独立扫描 |
| `SECTION_SCOPE` | `scope_t` 链表 | 由 `scope_service_init` 独立扫描 |
| `SECTION_PERF` | 由 `perf_insert` 处理 | perf 模块内部分派 |
| `SECTION_COMM` | 由 `comm` 模块处理 | 通信协议注册 |
| `SECTION_COMM_ROUTE` | 由 `comm` 模块处理 | 通信路由注册 |

### 3.3 任务调度 (`run_task`)

`run_task()` 在 `main()` 的 `while(1)` 循环中持续调用。遍历 `p_task_first` 链表，对每个到达周期的任务执行回调。调度特性：

- **周期驱动**：每个任务有 `t_period`（以 `SECTION_SYS_TICK` 为单位），`REG_TASK_MS` 宏将毫秒转为 tick。任务到期后 `time_last` 增加 `k * period`（`k = elapsed / period`），确保不积累延迟偏差。
- **性能测量**：如启用 `TASK_RECORD_PERF_ENABLE`，调度器在调用前后读取硬件计数器，计算任务执行时间并更新 `section_perf_record_t` 中的 `time`、`max_time`、`run_time`。ISR 在下一次任务调用之间打断的时间会被扣除。
- **无阻塞**：每个周期只执行一次任务回调，不循环追赶。

### 3.4 中断调度 (`section_interrupt`)

`section_interrupt()` 在 HRPWM 溢出中断中调用。按优先级顺序遍历 `p_interrupt_first` 链表执行所有注册的中断回调。同样支持性能测量：如检测到当前正在执行一个 `run_task` 回调，ISR 执行时间会被累加到 `s_running_task_interrupt_time` 并在任务结束时从任务耗时中扣除。

### 3.5 状态机 (`REG_FSM`)

框架提供 `REG_FSM` 宏定义有限状态机。每个状态机有 `fsm_sta`（当前状态）、状态转移表（`in` / `exe` / `chk` / `out` 函数指针），以及事件变量。`section_fsm_func()` 在 1ms 周期任务中执行：进入状态时调用 `func_in`，每周期调用 `func_exe`，检测到事件时通过 `func_chk` 判断转移目标并执行 `func_out`。

### 3.6 通信链路 (`section_link_t`)

`section_link_t` 抽象串行通信通道。每个 link 包含 `rx_get_byte`（接收回调）、`handler_arr[]`（接收字节到回调的映射）、以及 `my_printf`（输出接口）。`section_link_task` 以 100us 周期轮询所有 link 的接收缓冲区，按字节分派到 handler。

### 3.7 FLASH 与 RAM 用量

| 模块 | Code | RO Data | FLASH | RW Data | ZI Data | RAM |
|------|-----:|--------:|------:|--------:|--------:|----:|
| `section.o` | 592 | 10 | 602 | 34 | 72 | 106 |

## 4. Shell 命令行模块（`code/dbg/shell.{c,h}` + `shell_service.{c,h}`）

### 4.1 分层架构

| 层 | 文件 | 职责 |
|----|------|------|
| 内核 | `shell.c/h` | 文本解析、命令匹配、变量读写、注册表维护 |
| 服务 | `shell_service.c/h` | 二进制协议上报、list 异步打印、wave 波形流上报、状态周期执行 |

内核层只依赖 `section.h`，不依赖通信协议或打印设施细节。服务层依赖 `comm.h` 并引入 `REG_COMM` 注册二进制协议处理函数。

### 4.2 变量与命令注册

`section_shell_t` 统一表示 shell 变量和命令。通过 `REG_SHELL_VAR` 和 `REG_SHELL_CMD` 宏注册，内部使用 `REG_SECTION_FUNC(SECTION_SHELL, ...)` 放入链接器 section。

`section_shell_t` 关键字段：

| 字段 | 说明 |
|------|------|
| `p_name` / `p_name_size` | 变量名和长度 |
| `p_var` | 变量地址（命令为 NULL） |
| `type` | `SHELL_TYPE_E`：支持 `INT8/UINT8/INT16/UINT16/INT32/UINT32/FP32/CMD` |
| `p_max` / `p_min` | 上下限指针，写入时通过 `SHELL_UP_DN_LMT` 钳位 |
| `func` | 命令回调或变量变更通知回调 |
| `status` | 状态位（`SHELL_STA_AUTO = 1<<2` 表示周期自动上报） |
| `my_printf` | 运行时缓存的输出接口 |

`shell_init()` 在 `REG_INIT(0, shell_init)` 阶段扫描 `SECTION_SHELL` 构建 `p_shell_first` 链表。

### 4.3 文本命令解析

`shell_run()` 作为 link handler 接收逐字节输入。行缓冲器 128 字节，以 `\n` 触发解析：

1. 先匹配内置命令：`time`（系统时间）、`reset`（复位）、`help`（列出所有注册项）
2. 遍历 `p_shell_first` 链表，用 `strncmp` 按名称匹配
3. 匹配后通过 `:` 分隔命令名和参数值
4. 参数值支持 `-s N` 后缀设置 `status` 位（周期执行）

### 4.4 表达式求值（SHELL_STRING_PARSE）

当 `SHELL_STRING_PARSE == 1` 时，shell 支持变量写入和表达式求值：

- **`parse_integer`**：支持十进制、`0x` 十六进制、`0b` 二进制，以及简单算术表达式如 `1+2*3`
- **`eval_expr`**：递归下降表达式求值器，支持 `+` `-` `*` `/` 四则运算和括号嵌套，使用 `strtof` 解析浮点数
- **`shell_write_item_if_needed`**：按变量类型解析写入值，写入后钳位到 `[p_min, p_max]` 范围，然后调用 `func` 通知上层

当 `SHELL_STRING_PARSE == 0` 时，`shell_run` 为空桩函数，变量只读且通过二进制协议访问。

### 4.5 服务层

`shell_service.c` 提供三种服务：

| 功能 | 实现 | 说明 |
|------|------|------|
| 二进制列表上报 | `shell_data_num_act` + `shell_data_report_act` | 上位机查询全部 shell 项，分帧上报 |
| 二进制读写 | `shell_read_data_act` / `shell_write_data_act` | 远程读写变量值 |
| 波形流 | `shell_wave_start_act` + `shell_wave_report_task` | `SHELL_STA_AUTO` 选中的变量周期性流式上报 |
| list 分步打印 | `list_print_start` + `list_print_step` | 非阻塞分页打印所有注册项 |

list 分步打印和状态周期执行由 `SHELL_STRING_PARSE` 宏控制。

### 4.6 FLASH 与 RAM 用量

| 模块 | Code | RO Data | FLASH | RW Data | ZI Data | RAM |
|------|-----:|--------:|------:|--------:|--------:|----:|
| `shell.o` | 142 | 12 | 154 | 8 | 12 | 20 |
| `shell_service.o` | 1,246 | 26 | 1,272 | 125 | 220 | 345 |

## 5. Scope 录波模块（`code/dbg/scope.{c,h}` + `scope_service.{c,h}`）

### 5.1 分层架构

| 层 | 文件 | 职责 |
|----|------|------|
| 内核 | `scope.c/h` | 多通道环形缓冲区管理、采样触发、状态机 |
| 服务 | `scope_service.c/h` | 二进制协议（list/info/var/start/trigger/stop/reset/sample）、printf 数据输出、轮询检测 |

### 5.2 数据模型

`scope_t` 封装一个录波通道的全部状态。每个 scope 实例通过 `SCOPE_DEFINE` 宏一次声明：

- **环形缓冲区**：`float scope_<name>_buffer[var_count][buf_size]`，多维数组，每变量一行
- **变量指针数组**：`scope_<name>_var_ptrs[]`，指向各变量实例
- **变量名数组**：`scope_<name>_var_names[]`，字符串化变量名

`REG_SCOPE` 宏组合 `SCOPE_DEFINE` 和 `SECTION_SCOPE` 注册。`scope_service_init()` 在 `REG_INIT(0, ...)` 阶段遍历 `SECTION_SCOPE` 构建 `g_scope_first` 链表并分配 `scope_id`。

### 5.3 状态机与触发

scope 有三个状态：

```
IDLE ──start()──▶ RUNNING ──trigger()──▶ TRIGGERED ──缓冲填满──▶ IDLE
                     │                      │
                     └──stop()──▶ IDLE ◀──reset()──┘
```

- **`scope_start()`**：进入 RUNNING，清除缓冲区和索引
- **`scope_run()`**：ISR 中调用，将 `var_ptrs` 指向的变量值拷贝到当前 `write_index` 行，索引自增
- **`scope_trigger()`**：记录 `trigger_index`，持续采集 `trigger_post_cnt` 次后回到 IDLE
- **`scope_stop()`**：立即停止并回到 IDLE
- **`scope_reset()`**：重置所有索引和标志位

### 5.4 服务层协议

`scope_service.c` 提供完整二进制协议：

| 命令 | 说明 |
|------|------|
| `SCOPE_LIST_QUERY` (0x18) | 枚举所有 scope 实例，分帧上报 |
| `SCOPE_INFO_QUERY` (0x19) | 查询单个 scope 状态、缓冲区大小、触发位置 |
| `SCOPE_VAR_QUERY` (0x1A) | 查询 scope 变量名称列表 |
| `SCOPE_START` (0x1B) | 启动采样 |
| `SCOPE_TRIGGER` (0x1C) | 触发 |
| `SCOPE_STOP` (0x1D) | 停止 |
| `SCOPE_RESET` (0x1E) | 复位 |
| `SCOPE_SAMPLE_QUERY` (0x1F) | 按逻辑索引读取采样数据，支持 Normal 和 Force 两种模式 |

采样数据读出通过逻辑索引到物理索引的映射：在 Normal 模式下从 `trigger_index + trigger_post_cnt + 1` 开始环形读取；在 Force 模式下从当前 `write_index` 开始，允许在 RUNNING 期间强制读取。

`scope_service_poll_state()` 以 1ms 周期检测录波完成：当 `last_state == TRIGGERED` 且 `state == IDLE` 时，置 `data_ready = 1` 并递增 `capture_tag`，上位机通过轮询 `scope_info_query` 的 `data_ready` 位获知新数据。

### 5.5 FLASH 与 RAM 用量

| 模块 | Code | RO Data | FLASH | RW Data | ZI Data | RAM |
|------|-----:|--------:|------:|--------:|--------:|----:|
| `scope.o` | 196 | 0 | 196 | 0 | 0 | 0 |
| `scope_service.o` | 1,710 | 12 | 1,722 | 112 | 180 | 292 |

## 6. Trace 追踪模块（`code/dbg/trace.{c,h}` + `trace_service.{c,h}`）

### 6.1 分层架构

| 层 | 文件 | 职责 |
|----|------|------|
| 内核 | `trace.c/h` | FIFO 追踪缓冲、记录写入、清除、顺序读回 |
| 服务 | `trace_service.c/h` | 二进制控制/上报、printf 打印输出 |

### 6.2 数据模型与记录

`dbg_trace_item_t` 包含两个字段：`line`（源码行号）和 `time`（绑定系统时间戳）。`DBG_TRACE_MARK()` 宏展开为 `dbg_trace_record(__LINE__)`，在 ISR 或任务中插入追踪点。

FIFO 缓冲区固定 64 条记录（`DBG_TRACE_BUFFER_SIZE = 64`）。`write_count` 和 `read_count` 为单调递增计数器，通过位与运算映射到缓冲区索引：`index = count & (64 - 1)`。

溢出处理：当 `write_count > read_count + 64` 时，`read_count` 被推到 `write_count - 64`，丢弃最旧记录。

### 6.3 时间绑定

`DBG_TRACE_BIND_TIME(p_system_time)` 绑定一个硬件计数器（如 HRPWM 时基）。每个追踪点使用 `DBG_TRACE_MARK()` 插入时自动采样该计数器。

### 6.4 服务层

服务层提供三种消费方式：

| 功能 | 实现 | 说明 |
|------|------|------|
| printf 打印 | `dbg_trace_service_print_task` | 以 50ms 周期逐条打印 FIFO 记录（由 `TRACE_SERVICE_PRINTF` 宏控制） |
| 二进制上报 | `dbg_trace_service_binary_task` | 上位机通过 control 命令开启后，以 1ms 周期批量上报记录（每次最多 3 条） |
| 清除 | `dbg_trace_clear_cmd` | shell 命令清除缓冲区 |

### 6.5 FLASH 与 RAM 用量

| 模块 | Code | RO Data | FLASH | RW Data | ZI Data | RAM |
|------|-----:|--------:|------:|--------:|--------:|----:|
| `trace.o` | 132 | 8 | 140 | 0 | 0 | 0 |
| `trace_service.o` | 232 | 8 | 240 | 54 | 84 | 138 |

## 7. Perf 性能监控模块（`code/dbg/perf.{c,h}` + `perf_service.{c,h}` + `record_dict`）

### 7.1 分层架构

| 层 | 文件 | 职责 |
|----|------|------|
| 内核 | `perf.c/h` | 硬件计数器绑定、记录插入、CPU 负载计算、度量接口 |
| 服务 | `perf_service.c/h` | 二进制协议上报（list/info/record/reset peak） |
| 辅助 | `record_dict.c/h` | 性能记录 ID 字典，轻量级 `malloc` 替代 |

### 7.2 测量点注册

perf 支持三类测量点，通过统一的 `REG_PERF_RECORD_EX` 宏注册：

| 类型 | 宏 | 用途 |
|------|-----|------|
| `SECTION_PERF_RECORD_CODE` | `REG_PERF_RECORD` | 代码段级手动测量（`PERF_START`/`PERF_END` 成对调用） |
| `SECTION_PERF_RECORD_TASK` | `REG_TASK_PERF_RECORD` | 任务执行时间自动测量 |
| `SECTION_PERF_RECORD_INTERRUPT` | `REG_INTERRUPT_PERF_RECORD` | 中断服务时间自动测量 |

任务和中断的 `RECORD` 由 `section` 框架在 `run_task()` 和 `section_interrupt()` 中自动完成测量（见 §3.3、§3.4），无需业务代码手动插桩。

每个 record 包含 `start`/`end`/`time`（单次执行时间）、`max_time`（历史最大）、`run_time`（累计运行时间）、`load`/`load_max`（CPU 占用率）。

### 7.3 CPU 负载计算

`perf_cpu_load_calculate()` 以 `PERF_CPU_LOAD_PERIOD_MS`（500ms）周期执行。计算方式：

```
load = run_time / elapsed_perf_cnt
```

其中 `elapsed_perf_cnt` 为硬件计数器在两个周期之间的增量。`load_max` 为历史最大负载。总负载 `perf_task_metric` 和 `perf_interrupt_metric` 分别聚合所有任务/中断的 `run_time` 占比。

### 7.4 时间基准

perf 需要一个硬件计数器作为时间基准，通过 `REG_PERF_BASE_CNT(timer_cnt, period_s)` 注册。`timer_cnt` 指向硬件计数寄存器，`period_s` 表示一个计数 tick 的实际秒数。各平台按真实时钟树和分频填写该周期，perf 服务通过注册值换算时间和 CPU 占用率。

### 7.5 服务层协议

`perf_service.c` 提供二进制协议：

| 命令 | 说明 |
|------|------|
| `PERF_LIST_QUERY` | 枚举所有 perf 记录，分帧上报 |
| `PERF_INFO_QUERY` | 查询 CPU 负载、字典版本、记录总数 |
| `PERF_RECORD_QUERY` | 查询单个记录的运行时间、负载、ID |
| `PERF_RECORD_RESET_PEAK` | 重置峰值统计数据 |

printf 输出（`PERF_SERVICE_PRINTF` 宏控制）提供 `perf_printf_status` 和 `perf_printf_record` 用于终端调试。

### 7.6 FLASH 与 RAM 用量

| 模块 | Code | RO Data | FLASH | RW Data | ZI Data | RAM |
|------|-----:|--------:|------:|--------:|--------:|----:|
| `perf.o` | 512 | 28 | 540 | 48 | 88 | 136 |
| `perf_service.o` | 1,782 | 72 | 1,854 | 91 | 156 | 247 |
| `record_dict.o` | 186 | 0 | 186 | 0 | 0 | 0 |

## 8. 内核层与服务层分离模式

dbg 的五个子模块（shell、scope、trace、perf）遵循统一的分层模式：

```
┌─────────────────────────────────────────┐
│  服务层 (_service.c/h)                   │
│  - 二进制协议解析 (REG_COMM)             │
│  - shell 命令注册 (REG_SHELL_CMD)        │
│  - 周期任务注册 (REG_TASK_MS)            │
│  - printf 格式化输出 (宏隔离)            │
├─────────────────────────────────────────┤
│  内核层 (.c/h)                           │
│  - 纯数据结构和算法                       │
│  - 不依赖 section_link_tx_func_t / comm   │
│  - ISR 可安全调用                         │
└─────────────────────────────────────────┘
```

这种分层的设计原则：

- **依赖方向单向**：服务层依赖内核层和 `comm.h`/`section.h`，内核层不依赖服务层或通信框架
- **printf 宏隔离**：每个服务层提供 `*_SERVICE_PRINTF` / `*_ENABLE_PRINTF` 宏，在资源紧张的场景中可关闭所有 printf 代码
- **二进制协议无依赖**：`REG_COMM` 注册的二进制协议处理函数始终编译，不随 printf 宏裁剪
- **内核 ISR 安全**：内核函数（`scope_run`、`dbg_trace_record`、`PERF_START`/`PERF_END` 等）不包含任何阻塞或重量级操作

## 9. 编译配置

HC32F334 AC 平台当前使用 ARM Compiler for Embedded 6.24，优化等级 `-Oz`（极致空间优化）。

### 9.1 裁剪宏汇总

| 宏 | 模块 | 默认值 | 关闭时行为 |
|----|------|--------|-----------|
| `SHELL_STRING_PARSE` | shell | 0 | 只读变量，不解析表达式 |
| `SCOPE_ENABLE_PRINTF` | scope_service | 0 | printf 输出为空桩 |
| `TRACE_SERVICE_PRINTF` | trace_service | 0 | printf 输出为空桩 |
| `PERF_SERVICE_PRINTF` | perf_service | 0 | printf 输出为空桩 |
| `PERF_RECORD_ENABLE` | perf | 1 | 关闭 `PERF_START`/`PERF_END` 插桩 |
| `TASK_RECORD_PERF_ENABLE` | section | 1 | 关闭任务自动测量 |
| `INTERRUPT_RECORD_PERF_ENABLE` | section | 1 | 关闭中断自动测量 |
