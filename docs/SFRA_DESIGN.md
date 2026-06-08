# SFRA 设计文档

## 1. 模块定位

SFRA 用于在控制系统中注入扫频正弦信号，采集响应信号，并计算每个频点的幅值和相位。

当前实现分为两层：

| 层级 | 文件 | 职责 |
| --- | --- | --- |
| SFRA 内核 | `code/dbg/sfra.c`、`code/dbg/sfra.h` | 注入信号生成、采样缓冲、DFT 计算、扫频状态机 |
| SFRA 服务 | `code/dbg/sfra_service.c`、`code/dbg/sfra_service.h` | 实例列表、配置、启停控制、频点结果查询和完成上报 |

SFRA 内核不直接访问 ADC 或 PWM。业务代码负责把 `p_inject` 加入控制量，并把被测响应写入 `p_collect`。

## 2. 实例注册

注册宏：

```c
REG_SFRA(name, delay_tick, ts, inject_amp, freq_start, freq_end, prepare_cb, prepare_ctx)
```

注册宏会生成：

- 注入变量 `name_inject`
- 响应变量 `name_collect`
- `sfra_t` 实例
- `SECTION_SFRA` 注册项

`ts` 是 ISR 采样周期，`delay_tick` 用于对齐注入与采集响应。

## 3. 核心对象

`sfra_t` 包含以下主要部分：

| 成员 | 含义 |
| --- | --- |
| `port` | 外部注入和响应端口 |
| `cfg` | 扫频范围、采样周期、注入幅值、稳定周期、采集周期 |
| `isr` | ISR 快速路径状态、注入延迟线、采样缓冲 |
| `task` | 慢速扫频状态机 |
| `cb` | 频点准备回调 |
| `output` | 当前输出状态和最新频点结果 |
| `result_cache` | 完成频点缓存 |
| `sweep_tag` | 当前扫频批次标识 |
| `sfra_id` | 服务层分配 ID |
| `p_next` | 运行时链表 |

## 4. 状态机

SFRA 任务状态：

| 状态 | 含义 |
| --- | --- |
| `SFRA_STATE_IDLE` | 空闲 |
| `SFRA_STATE_PREPARE_FREQ` | 准备当前频点 |
| `SFRA_STATE_SETTLE` | 等待系统稳定 |
| `SFRA_STATE_COLLECT` | 采集 DFT 样本 |
| `SFRA_STATE_CALC` | 计算幅值和相位 |
| `SFRA_STATE_DONE` | 扫频完成 |

`sfra_task()` 推进状态机，`sfra_isr_pre_sample()` 和 `sfra_isr_post_sample()` 在控制 ISR 中调用。

## 5. ISR 与任务分工

ISR 前采样：

- 根据当前频点生成正弦注入值。
- 维护注入延迟线。
- 把注入值写入 `*p_inject`。

ISR 后采样：

- 读取 `*p_inject` 和 `*p_collect`。
- 把样本写入内部环形缓冲。

任务侧：

- 从环形缓冲取样本。
- 推进稳定和采集计数。
- 调用 DFT 计算注入和响应。
- 计算传递函数幅值和相位。
- 缓存每个频点结果。

## 6. 服务层

`sfra_service_init()` 通过 `REG_INIT(0, sfra_service_init)` 注册。初始化时扫描 `SECTION_SFRA`，构建 `g_sfra_first` 链表，并分配 `sfra_id`。

`sfra_service_poll_task()` 每 1 ms 运行一次，用于推进已注册实例的服务状态和完成上报。

二进制命令使用 `cmd_set = 0x01`：

| 功能 | cmd_word | 说明 |
| --- | --- | --- |
| 列表查询 | `0x2F` | 查询 SFRA 实例列表 |
| 信息查询 | `0x30` | 查询状态和配置 |
| 配置设置 | `0x31` | 设置扫频范围和注入幅值 |
| 启动 | `0x32` | 启动扫频 |
| 停止 | `0x33` | 停止扫频 |
| 复位 | `0x34` | 复位实例 |
| 频点查询 | `0x35` | 查询指定频点结果 |
| 频点上报 | `0x36` | 上报频点结果 |
| 完成上报 | `0x37` | 扫频完成通知 |

## 7. 当前约束

- 不使用动态内存。
- 频点表最大长度为 `SFRA_FREQ_TABLE_SIZE`，当前 300。
- ISR 采样缓冲为 `SFRA_SAMPLE_BUFFER_SIZE`，当前 32。
- 注入延迟最大 `SFRA_MAX_INJECT_DELAY_TICK`，当前 2。
- 业务代码必须在控制环中调用 pre/post sample。
- 注入幅值需要由用户保证不会破坏控制系统稳定性。
