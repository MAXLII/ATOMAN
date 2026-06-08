# Scope 设计文档

## 1. 模块定位

Scope 模块用于捕获一组浮点变量的时间序列，支持启动、触发、停止、复位和按样本读取。

当前实现分为两层：

| 层级 | 文件 | 职责 |
| --- | --- | --- |
| Scope 内核 | `code/dbg/scope.c`、`code/dbg/scope.h` | scope 对象定义、采样缓冲、触发后采样、状态切换 |
| Scope 服务 | `code/dbg/scope_service.c`、`code/dbg/scope_service.h` | scope 列表、信息查询、变量名查询、样本读取、可选 Shell 打印 |

Scope 不直接绑定 ADC、PWM 或控制环。业务代码在合适的周期调用 `SCOPE_RUN()`，Scope 读取注册变量当前值并写入内部缓冲。

## 2. Scope 对象

`scope_t` 表示一个采样实例。

| 字段 | 含义 |
| --- | --- |
| `buffer` | 采样缓冲，按变量数量和样本数展开 |
| `var_ptrs` | 被采样变量指针数组 |
| `var_names` | 变量名称数组 |
| `buffer_size` | 每个变量的样本数 |
| `var_count` | 变量数量 |
| `write_index` | 当前写入位置 |
| `trigger_index` | 触发发生时的写入位置 |
| `trigger_post_cnt` | 触发后继续采样数量 |
| `state` | `IDLE` / `RUNNING` / `TRIGGERED` |
| `data_ready` | 一帧数据是否可读 |
| `sample_period_us` | 采样周期，用于上位机显示 |
| `capture_tag` | 捕获批次标识 |
| `scope_id` | 服务层分配的 ID |
| `p_name` | scope 名称 |
| `p_next` | 运行时链表指针 |

当前宏最多支持一次注册 10 个变量。

## 3. 注册模型

注册宏：

```c
REG_SCOPE(name, buf_size, trig_post_cnt, var1, var2, ...)
REG_SCOPE_EX(name, buf_size, trig_post_cnt, sample_period_us, var1, var2, ...)
```

`REG_SCOPE()` 默认采样周期为 1000 us。`REG_SCOPE_EX()` 允许显式指定采样周期。

注册宏会生成：

- 每个变量的 `float` 存储对象
- 变量指针数组
- 变量名称数组
- 采样缓冲
- `scope_t` 实例
- `SECTION_SCOPE` 注册项

## 4. 运行状态

状态定义：

| 状态 | 含义 |
| --- | --- |
| `SCOPE_STATE_IDLE` | 未运行 |
| `SCOPE_STATE_RUNNING` | 正在循环采样 |
| `SCOPE_STATE_TRIGGERED` | 已触发，正在完成触发后采样 |

核心接口：

```c
void scope_start(scope_t *scope);
void scope_stop(scope_t *scope);
void scope_trigger(scope_t *scope);
void scope_reset(scope_t *scope);
void scope_run(scope_t *scope);
```

`scope_run()` 在业务调用周期中执行。未触发时持续循环写入；触发后继续记录 `trigger_post_cnt` 个样本，然后置 `data_ready` 并停止。

## 5. 服务初始化

`scope_service_init()` 通过 `REG_INIT(0, scope_service_init)` 注册。

初始化时扫描 `SECTION_SCOPE`，构建 `g_scope_first` 链表，并给每个 scope 分配 `scope_id`。

## 6. 二进制服务

Scope 服务使用 `cmd_set = 0x01`。

| 功能 | cmd_word | 说明 |
| --- | --- | --- |
| 列表查询 | `0x18` | 获取已注册 scope 列表 |
| 信息查询 | `0x19` | 获取指定 scope 状态、样本数、触发位置等 |
| 变量查询 | `0x1A` | 获取指定变量名 |
| 启动 | `0x1B` | 调用 `scope_start()` |
| 触发 | `0x1C` | 调用 `scope_trigger()` |
| 停止 | `0x1D` | 调用 `scope_stop()` |
| 复位 | `0x1E` | 调用 `scope_reset()` |
| 样本查询 | `0x1F` | 按样本索引读取所有变量值 |

服务层使用 `capture_tag` 区分不同批次的采样数据。上位机读取样本时可带上期望的 `capture_tag`，避免读到新旧混合数据。

## 7. 可选 Shell 打印

`SCOPE_ENABLE_PRINTF == 1` 时，`scope_service.h` 提供 Shell 命令注册宏：

| 宏 | 说明 |
| --- | --- |
| `REG_SCOPE_STATUS_CMD(name)` | 注册状态打印命令 |
| `REG_SCOPE_START_CMD(name)` | 注册启动命令 |
| `REG_SCOPE_DATA_STEP_CMD(name)` | 注册分步打印数据命令 |

默认 `SCOPE_ENABLE_PRINTF` 为 `0`，这些宏为空。

## 8. 当前约束

- 不使用动态内存。
- 当前采样变量按 `float` 存储。
- 注册宏最多支持 10 个变量。
- Scope 只在调用 `scope_run()` 时采样。
- 样本缓冲大小由注册宏静态决定。
- 二进制样本读取按 `scope_id` 和 `sample_index` 查询。
