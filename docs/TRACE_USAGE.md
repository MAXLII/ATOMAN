# Trace 使用文档

## 1. 适用范围

本文档说明如何使用 Trace 记录执行路径。

内部实现见 [TRACE_DESIGN.md](TRACE_DESIGN.md)。

## 2. 绑定时间源

使用 Trace 前需要绑定时间源：

```c
#include "trace.h"

DBG_TRACE_BIND_TIME(&system_time);
```

`system_time` 应该是持续递增的 `uint32_t` 计数值。

## 3. 添加 Trace 点

在需要记录的位置调用：

```c
DBG_TRACE_MARK();
```

示例：

```c
void demo_state_run(void)
{
    DBG_TRACE_MARK();

    if (fault_active)
    {
        DBG_TRACE_MARK();
        return;
    }

    DBG_TRACE_MARK();
}
```

Trace 会记录每个 `DBG_TRACE_MARK()` 所在行号和当前时间戳。

## 4. 清空记录

代码中清空：

```c
dbg_trace_clear();
```

Shell 中清空：

```text
dbg_trace_clear
```

## 5. Shell 打印

如果 Shell 字符串解析可用，可以执行：

```text
dbg_trace_print
```

该命令会启动分步打印任务，周期性输出 trace 记录。记录输出后会被消费。

## 6. 上位机二进制读取

Trace 服务通过二进制协议提供启停和上报：

1. 发送控制命令启动二进制上报。
2. 后台任务周期读取 trace 缓冲。
3. 每条记录上报时间和行号。
4. 发送控制命令停止上报。

二进制上报同样会消费记录。

## 7. 使用注意事项

- 先绑定时间源，再调用 `DBG_TRACE_MARK()`。
- Trace 缓冲只有 64 条，事件太密会覆盖旧记录。
- `DBG_TRACE_MARK()` 只记录行号，不记录文件名。
- 同一行多次调用无法从记录中区分调用点。
- Shell 打印和二进制读取不能同时作为两个独立消费者使用。
- Trace 适合临时定位流程问题，验证完成后应减少无用 trace 点。
