# Shell 使用文档

## 1. 适用范围

本文档说明如何在当前工程中使用 Shell 注册调试变量和调试命令，以及如何通过通信协议访问这些变量。

Shell 的内部实现见 [SHELL_DESIGN.md](SHELL_DESIGN.md)。

## 2. 头文件

业务模块需要使用 Shell 时，引入：

```c
#include "shell.h"
```

如果需要直接使用 Shell 二进制服务相关结构，再引入：

```c
#include "shell_service.h"
```

普通业务模块通常只需要 `shell.h`。

## 3. 注册变量

使用 `REG_SHELL_VAR()` 注册变量：

```c
REG_SHELL_VAR(name, var, type, max, min, func, status)
```

参数含义：

| 参数 | 说明 |
| --- | --- |
| `name` | Shell 中显示和访问的名称 |
| `var` | 被注册的变量 |
| `type` | 变量类型 |
| `max` | 上限 |
| `min` | 下限 |
| `func` | 写入变量后的回调函数，可填 `NULL` |
| `status` | 状态位，常用 `SHELL_STA_NULL` |

支持的类型：

| 类型 | 说明 |
| --- | --- |
| `SHELL_INT8` | `int8_t` |
| `SHELL_UINT8` | `uint8_t` |
| `SHELL_INT16` | `int16_t` |
| `SHELL_UINT16` | `uint16_t` |
| `SHELL_INT32` | `int32_t` |
| `SHELL_UINT32` | `uint32_t` |
| `SHELL_FP32` | `float` |

示例：

```c
static uint32_t s_counter = 0u;
static float s_gain = 1.0f;

REG_SHELL_VAR(DEMO_COUNTER, s_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_GAIN, s_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
```

变量写入后需要通知业务模块时，可以提供回调：

```c
static uint8_t s_enable = 0u;

static void demo_enable_changed(DEC_MY_PRINTF)
{
    (void)my_printf;
    /* 在这里同步业务状态 */
}

REG_SHELL_VAR(DEMO_ENABLE, s_enable, SHELL_UINT8, 1u, 0u, demo_enable_changed, SHELL_STA_NULL)
```

## 4. 注册命令

使用 `REG_SHELL_CMD()` 注册命令：

```c
REG_SHELL_CMD(name, func)
```

命令回调原型：

```c
static void demo_cmd(DEC_MY_PRINTF)
{
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("ok\r\n");
    }
}
```

示例：

```c
static void demo_ping_cmd(DEC_MY_PRINTF)
{
    if ((my_printf != NULL) && (my_printf->my_printf != NULL))
    {
        my_printf->my_printf("pong\r\n");
    }
}

REG_SHELL_CMD(DEMO_PING, demo_ping_cmd)
```

## 5. 自动上报变量

变量注册时可以把 `status` 设置为 `SHELL_STA_AUTO`：

```c
REG_SHELL_VAR(DEMO_STATE, s_state, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_AUTO)
```

`SHELL_STA_AUTO` 表示该变量默认进入波形自动上报选择。也可以通过上位机协议动态打开或关闭某个变量的自动上报。

## 6. 字符串 Shell 接入

当前工程中 `SHELL_STRING_PARSE` 默认是 `0`，字符串 Shell 不解析输入。

如果打开字符串解析，需要为每条链路准备独立上下文：

```c
DECLARE_SHELL_CTX(s_shell_ctx);
```

然后把 `shell_run()` 接到链路 handler 中：

```c
static section_link_handler_t s_handler[] = {
    {.func = shell_run, .ctx = (void *)&s_shell_ctx},
};
```

每收到一个字节，链路层把字节送入 `shell_run()`。字符串命令以 `\n` 结束。

常用字符串命令格式：

```text
help
time
reset
DEMO_COUNTER
DEMO_COUNTER:100
DEMO_GAIN:1.5
DEMO_PING
```

如果字符串解析关闭，变量和命令仍然可以通过二进制通信协议访问。

## 7. 上位机二进制访问

Shell 服务通过 `comm` 协议提供二进制访问能力。业务模块不需要额外注册这些命令，只要 `shell_service.c` 被编译进工程即可。

常用能力：

| 功能 | 说明 |
| --- | --- |
| 查询变量数量 | 获取当前注册的 Shell 项数量 |
| 查询变量列表 | 分帧上报名称、类型、当前值、上下限和自动上报状态 |
| 读取变量 | 按名称读取变量当前值 |
| 写入变量 | 按名称写入变量值和上下限 |
| 选择波形变量 | 选择某个变量是否参与自动上报 |
| 启停波形 | 开启或停止 Shell 波形流 |
| 设置波形周期 | 设置两帧波形之间的等待周期 |

上位机访问变量时使用 Shell 名称匹配。名称长度由协议中的 `name_len` 指定。

## 8. 在 demo 中参考

当前 demo 里可以参考：

```text
code/app/demo/demo_shell.c
```

该文件演示了：

- 注册 `uint32_t` 变量
- 注册 `float` 变量
- 注册 Shell 命令
- 在变量写入后执行回调

## 9. 使用注意事项

- 注册变量的 `type` 必须和变量真实类型一致。
- Shell 名称最长 40 字节。
- `REG_SHELL_VAR()` 和 `REG_SHELL_CMD()` 通常放在模块 `.c` 文件中。
- 命令回调中使用 `my_printf` 前需要判空。
- 写变量时会按注册的 `max/min` 做限幅。
- `SHELL_FP32` 通过 4 字节浮点数据读写。
- 字符串 Shell 使用每链路 128 字节输入缓冲。
- 二进制协议访问不依赖 `SHELL_STRING_PARSE` 是否开启。
