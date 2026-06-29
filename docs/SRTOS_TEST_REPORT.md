# SRTOS 测试报告

## 1. 测试信息

| 项目 | 内容 |
| --- | --- |
| 测试日期 | 2026-06-29 |
| 测试工程 | `gd32g553c` |
| 编译器 | GCC，`mingw32-make.exe` |
| 下载工具 | J-Link |
| 目标串口 | COM8 |
| 默认调试串口波特率 | 921600 |
| 默认 SRTOS | 开启 |
| 版本 | `1.2.9.6` |

测试方法：

- 每个用例重新编译固件。
- 编译成功后通过 J-Link 下载运行。
- 运行指定时间后读取 `g_section_fault_debug`。
- 串口类用例通过 COM8 采集输出行数。
- 故障注入类用例以期望 fault reason 作为通过条件。
- 测试结束后重新编译并下载默认固件。

## 2. 判定标准

正常运行类用例通过条件：

```text
task_fault_reason == 0
task_context_save_fail_count == 0
task_context_release_fail_count == 0
```

故障注入类用例通过条件：

```text
task_fault_reason == 期望故障码
```

公共现场池 keep running 策略通过条件：

```text
task_fault_reason == 0
task_context_save_fail_count > 0
task_context_release_fail_count == 0
```

## 3. 故障码对照

| fault reason | 含义 |
| --- | --- |
| `0` | 无 fault |
| `1` | 公共现场池满 |
| `2` | PSP 越界 |
| `3` | 恢复现场过大 |
| `4` | 现场释放顺序错误 |
| `5` | PSP 运行栈配置过小 |

## 4. 测试结果汇总

| 用例 | 结果 | fault | save fail | release fail | 说明 |
| --- | --- | ---: | ---: | ---: | --- |
| SRTOS-BASIC-001 默认长跑 20s | 通过 | 0 | 0 | 0 | 默认配置运行正常 |
| SRTOS-BASIC-002 100ms/123ms 长任务串口 | 通过 | 0 | 0 | 0 | 串口 6s 内采集到有效任务输出 |
| SRTOS-SLICE-001 1 tick 时间片 | 通过 | 0 | 0 | 0 | 高频切换无保存释放失败 |
| SRTOS-SLICE-003 50 tick 时间片 | 通过 | 0 | 0 | 0 | 低频切换正常 |
| SRTOS-CTX-006 任务时长时短 | 通过 | 0 | 0 | 0 | 5 拍短运行、1 拍长运行约 10ms，周期 3ms |
| SRTOS-POOL-003 16 words 现场池 fault 策略 | 通过 | 1 | 1 | 0 | 正确触发公共现场池满保护 |
| SRTOS-POOL-004 16 words 现场池 keep running 策略 | 通过 | 0 | 51091 | 0 | 正确拒绝切换并继续当前任务 |
| SRTOS-STACK-002 64 words 运行栈 | 通过 | 0 | 0 | 0 | 小运行栈压力下正常 |
| SRTOS-STACK-003 32 words 运行栈过小 | 通过 | 5 | 0 | 0 | 正确触发运行栈过小保护 |
| SRTOS-UART-006 9600 低速串口 | 阻塞 | - | - | - | 当前波特率宏不能通过 `DEFINES` 覆盖，构建被 `-Werror` 拦截 |
| SRTOS-CTX-003 调度探针浮点长任务 | 阻塞 | - | - | - | 调度探针测试代码存在 strict-aliasing 编译错误 |

## 5. 关键用例详情

### 5.1 默认长跑

配置：

```text
默认配置
运行 20s
```

结果：

```text
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

结论：默认配置下 RTOS 正常运行。

### 5.2 两个长任务串口输出

配置：

```text
默认配置
COM8 = 921600
采集 6s
```

结果：

```text
有效输出行数 = 108
TASK_DEAD_LOOP_100MS = 60
TASK_DEAD_LOOP_123MS = 48
bad_lines = 1
task_fault_reason = 0
save_fail = 0
release_fail = 0
```

结论：两个长任务均持续运行，未阻塞 RTOS 调度。

### 5.3 高频时间片

配置：

```text
SECTION_TASK_SLICE_TICKS = 1
运行 10s
```

结果：

```text
task_fault_reason = 0
save_fail = 0
release_fail = 0
```

结论：高频切换压力下上下文保存恢复正常。

### 5.4 公共现场池满 fault 策略

配置：

```text
SECTION_TASK_CONTEXT_POOL_WORDS = 16
SECTION_TASK_CONTEXT_POOL_FULL_POLICY = SECTION_TASK_CONTEXT_POOL_FAULT
```

结果：

```text
task_fault_reason = 1
task_context_save_fail_count = 1
task_context_release_fail_count = 0
```

结论：公共现场池不足时正确进入 fault。

### 5.5 公共现场池满 keep running 策略

配置：

```text
SECTION_TASK_CONTEXT_POOL_WORDS = 16
SECTION_TASK_CONTEXT_POOL_FULL_POLICY = SECTION_TASK_CONTEXT_POOL_KEEP_RUNNING
运行 5s
```

结果：

```text
task_fault_reason = 0
task_context_save_fail_count = 51091
task_context_release_fail_count = 0
```

结论：keep running 策略生效，现场池不足时不切换、不进入 fault，保存失败计数持续累加。

### 5.6 PSP 运行栈过小保护

配置：

```text
SECTION_TASK_RUNTIME_STACK_WORDS = 32
```

结果：

```text
task_fault_reason = 5
task_context_required_words = 33
task_context_save_fail_count = 0
task_context_release_fail_count = 0
```

结论：运行栈小于最小启动现场需求时，启动阶段正确进入保护。

### 5.7 任务时长时短

配置：

```text
DEMO_TASK_DEAD_LOOP_ENABLE = 0
DEMO_TASK_VARIABLE_LENGTH_ENABLE = 1
任务周期 = 3ms
运行模式 = 5 拍短运行，1 拍长运行
短运行 = 十几个 us 级别
长运行 = 约 10ms
```

测试任务行为：

```text
phase 1..5: 短路径快速完成
phase 0: 长路径等待 100 个 100us tick，约 10ms
```

20s 采样结果：

```text
enter_count = 15944
short_count = 13287
long_count = 2657
long_enter_count = 2657
max_run_ticks = 106
local_error_count = 0
task_fault_reason = 0
task_context_save_fail_count = 0
task_context_release_fail_count = 0
task_context_pool_used = 69
task_stack_free_words = 495
```

结论：

```text
同一个任务在短路径一次完成、长路径跨多个时间片保存恢复之间切换正常。
局部变量 guard 没有损坏，现场池没有泄漏，未出现保存释放失败。
```

该用例首次运行时触发了 `fault_reason=4`。根因是公共现场池环形回绕后，head 指向尾部 gap，恢复任务的有效快照位于池首地址 0，但释放逻辑先检查 `snapshot_offset == head`，没有先跳过 gap，导致误判为释放顺序错误。修复后，释放现场前后都会先识别并跳过 head 所在 gap，再执行快照释放。

## 6. 阻塞项

### 6.1 9600 波特率自动回归阻塞

本轮测试尝试通过编译宏覆盖：

```text
-DBSP_USART_DBG_BAUDRATE=9600UL
```

构建失败：

```text
gd32g553c/bsp/src/bsp_usart.c:20:9:
error: "BSP_USART_DBG_BAUDRATE" redefined [-Werror]
```

原因：`BSP_USART_DBG_BAUDRATE` 在 `bsp_usart.c` 内直接定义，当前不能通过 `DEFINES` 覆盖。此前临时修改源码后已经测过 921600 到 9600，RTOS 无 fault、无保存释放失败。本轮报告中不把该项计为通过，因为本轮自动回归没有成功构建。

### 6.2 调度探针用例阻塞

启用：

```text
DEMO_TASK_SCHED_PROBE_ENABLE = 1
```

构建失败：

```text
demo_task.c:168:
error: dereferencing type-punned pointer will break strict-aliasing rules [-Werror=strict-aliasing]
error: 'acc' is used uninitialized [-Werror=uninitialized]
```

原因：测试代码使用 `*((uint32_t *)&acc)` 读取浮点 bit，触发 strict-aliasing 风险。该问题属于测试代码问题，不是本轮 RTOS 运行时 fault。

## 7. 未覆盖项

| 用例 | 状态 | 原因 |
| --- | --- | --- |
| SRTOS-POOL-005 环形 head/tail 长时间回绕 | 部分覆盖 | 本轮有现场池保存恢复，但没有单独长时间统计回绕次数 |
| SRTOS-FAULT-002 PSP 越界 | 未执行 | 需要增加大局部数组或异常 SP 注入测试 |
| SRTOS-FAULT-003 恢复现场过大 | 未执行 | 需要专门注入异常快照大小 |
| SRTOS-FAULT-004 现场释放顺序错误 | 未执行 | 需要专门注入 release 顺序错误 |

## 8. 测试结论

本轮已通过的测试覆盖了：

- 默认 RTOS 长跑。
- 两个死循环长任务并发运行。
- 同一任务 5 拍短运行、1 拍长运行的上下文保存恢复。
- 高频和低频时间片配置。
- 公共现场池满的 fault 策略。
- 公共现场池满的 keep running 策略。
- PSP 运行栈过小保护。
- 64 words 小运行栈压力。

结论：

```text
RTOS 核心调度、上下文保存恢复、公共现场池策略和 PSP 运行栈保护在本轮已执行用例中表现正常。
```

后续建议优先补齐：

- 让 `BSP_USART_DBG_BAUDRATE` 支持编译宏覆盖，便于低波特率自动回归。
- 修复 `DEMO_TASK_SCHED_PROBE_ENABLE` 下的 strict-aliasing 编译问题。
- 增加现场池 head/tail 回绕次数统计，便于长期回归时直接观察环形池状态。
