# Trace 设计文档

## 1. 模块定位

Trace 模块用于记录代码执行路径中的时间戳和源码行号，适合定位流程顺序、异常路径和偶发事件。

当前实现分为两层：

| 层级 | 文件 | 职责 |
| --- | --- | --- |
| Trace 内核 | `code/dbg/trace.c`、`code/dbg/trace.h` | 固定长度环形缓冲、时间源绑定、记录写入、记录读取 |
| Trace 服务 | `code/dbg/trace_service.c`、`code/dbg/trace_service.h` | Shell 打印、清空命令、二进制上报 |

Trace 只记录 `line` 和 `time`，不记录文件名和自定义文本。

## 2. 时间源

Trace 使用外部时间源：

```c
DBG_TRACE_BIND_TIME(p_system_time)
```

`p_system_time` 是一个 `volatile uint32_t *`。每次记录时，Trace 读取该指针当前值作为时间戳。

服务层默认把时间单位定义为：

```c
#define TRACE_SERVICE_TIME_UNIT_US 100u
```

## 3. 缓冲模型

Trace 使用固定环形缓冲：

```c
#define DBG_TRACE_BUFFER_SIZE 64u
```

每条记录为：

| 字段 | 含义 |
| --- | --- |
| `line` | 调用 `DBG_TRACE_MARK()` 的源码行号 |
| `time` | 绑定时间源的快照 |

内部维护 `write_count` 和 `read_count`。写满后，新记录覆盖最旧记录，并把读指针推进到仍然有效的位置。

## 4. 记录流程

业务代码调用：

```c
DBG_TRACE_MARK();
```

宏展开后调用 `dbg_trace_record(__LINE__)`。

记录流程：

1. 检查时间源是否已绑定。
2. 使用 `write_count & (DBG_TRACE_BUFFER_SIZE - 1)` 计算写入位置。
3. 写入时间戳和行号。
4. 推进 `write_count`。
5. 如缓冲溢出，推进 `read_count`。

## 5. 读取流程

Trace 内核提供：

```c
uint8_t dbg_trace_read(uint32_t *p_time, uint32_t *p_line);
```

该接口从最旧记录开始读取，并推进 `read_count`。Shell 打印和二进制上报都使用该接口，因此读出后记录会被消费。

## 6. 服务层

Shell 命令：

| 命令 | 说明 |
| --- | --- |
| `dbg_trace_print` | 启动分步打印 |
| `dbg_trace_clear` | 清空 trace 缓冲 |

二进制命令使用 `cmd_set = 0x01`：

| 功能 | cmd_word | 说明 |
| --- | --- | --- |
| 控制 | `0x2C` | 开启或停止二进制上报 |
| 记录上报 | `0x2D` | 周期上报 trace 记录 |

`dbg_trace_service_binary_task()` 每 1 ms 运行一次，每次最多上报 `DBG_TRACE_BINARY_MAX_REPORT_PER_TASK` 条记录。

## 7. 当前约束

- 不使用动态内存。
- 缓冲大小固定为 64 条。
- 缓冲大小依赖按位与取模，当前值应保持为 2 的幂。
- 只记录源码行号，不记录文件名。
- Shell 打印和二进制上报都会消费记录。
- 未绑定时间源时，`DBG_TRACE_MARK()` 不写入记录。
