# Shell 设计文档

## 1. 模块定位

Shell 模块用于把工程中的调试变量和调试命令统一注册成可发现、可查询、可读写的服务对象。

当前实现分为两层：

| 层级 | 文件 | 职责 |
| --- | --- | --- |
| Shell 内核 | `code/dbg/shell.c`、`code/dbg/shell.h` | 注册项定义、链接段扫描、链表维护、名称查找、可选字符串命令解析 |
| Shell 服务 | `code/dbg/shell_service.c`、`code/dbg/shell_service.h` | 二进制通信协议、变量列表上报、变量读写、波形自动上报 |

Shell 内核只维护“有哪些变量和命令”，不直接绑定具体串口、CAN 或其他物理链路。链路输入由 `interface/` 或平台侧把字节送入 `shell_run()`，二进制服务通过 `comm` 协议回包。

## 2. 注册模型

Shell 注册项统一使用 `section_shell_t` 描述：

| 字段 | 含义 |
| --- | --- |
| `p_name` | Shell 名称 |
| `p_name_size` | 名称长度 |
| `p_var` | 变量地址，命令项为 `NULL` |
| `type` | 变量或命令类型 |
| `p_max` / `p_min` | 变量上下限地址 |
| `func` | 命令回调或变量写入后的通知回调 |
| `status` | 状态位，当前 `SHELL_STA_AUTO` 用于波形自动上报选择 |
| `my_printf` | 可缓存当前输出链路 |
| `p_next` | 运行时链表指针 |

注册宏有两个：

```c
REG_SHELL_VAR(name, var, type, max, min, func, status)
REG_SHELL_CMD(name, func)
```

`REG_SHELL_VAR()` 会为变量生成上下限静态对象，并把 `section_shell_t` 放入 `SECTION_SHELL` 链接段。`REG_SHELL_CMD()` 只注册命令名和回调函数，不绑定变量地址。

名称最大长度由 `SHELL_STR_SIZE_MAX` 限制，当前为 40 字节。注册宏通过 `_Static_assert` 在编译期检查名称长度。

## 3. 初始化流程

`shell_init()` 通过 `REG_INIT(0, shell_init)` 注册为初始化函数。

初始化时，`shell_init()` 扫描 `SECTION_START` 到 `SECTION_STOP` 之间的所有注册项，只处理 `SECTION_SHELL` 类型，把对应的 `section_shell_t` 插入到 `p_shell_first` 链表。

插入时会检查同一个注册项是否已经存在，避免重复初始化导致链表重复挂载。

运行时可通过以下接口访问注册表：

```c
section_shell_t *shell_first_get(void);
uint32_t shell_count_get(void);
section_shell_t *shell_find(const char *p_name, uint8_t len);
```

## 4. 字符串 Shell

字符串 Shell 由 `SHELL_STRING_PARSE` 控制：

```c
#define SHELL_STRING_PARSE 0
```

当前默认值为 `0`，此时 `shell_run()` 是空实现，不解析输入字节。

当 `SHELL_STRING_PARSE == 1` 时，`shell_run()` 使用 `shell_ctx_t` 保存每条链路自己的输入缓冲：

```c
typedef struct
{
    uint8_t shell_buffer[128];
    uint8_t shell_index;
} shell_ctx_t;
```

平台或 interface 侧可以用 `DECLARE_SHELL_CTX(name)` 创建上下文，再把 `shell_run()` 作为链路 handler 接入。这样不同链路有独立输入缓冲，互不污染。

字符串输入以 `\n` 作为一行结束，兼容 `\r\n`。一行解析完成后会清空当前上下文缓冲。

支持的内置命令：

| 命令 | 作用 |
| --- | --- |
| `time` | 打印 `SECTION_SYS_TICK` 换算出的运行时间 |
| `reset` | 调用 `SYSTEM_RESET` |
| `help` | 打印已注册的 Shell 名称和类型 |

对注册项的匹配规则是名称精确匹配，名称后可以直接结束，也可以跟 `:` 携带参数。

## 5. 字符串参数解析

当 `SHELL_STRING_PARSE == 1` 时，变量写入支持以下形式：

```text
name:value
name:value -s status
```

整数类型支持：

- 十进制数字
- `0x` 十六进制
- `0b` 二进制
- 简单表达式

浮点类型支持数字和简单表达式。写入后会通过 `SHELL_UP_DN_LMT` 按注册时的 `max/min` 做限幅，然后调用注册项的 `func`。

`-s` 用于设置 `status`，并缓存当前输出链路到注册项的 `my_printf`。在字符串 Shell 服务中，`status` 的低位含义为：

| 位 | 含义 |
| --- | --- |
| bit0 | 周期打印变量 |
| bit1 | 周期执行回调 |
| bit2 | 波形自动上报选择，对应 `SHELL_STA_AUTO` |

## 6. 二进制 Shell 服务

`shell_service.c` 通过 `REG_COMM()` 注册二进制协议处理函数。当前 Shell 服务使用的命令如下：

| 功能 | cmd_set | cmd_word | 说明 |
| --- | --- | --- | --- |
| 查询数量 | `0x01` | `0x01` | 返回当前 Shell 注册项数量，并启动列表上报上下文 |
| 读取变量 | `0x01` | `0x02` | 按名称读取变量值 |
| 写入变量 | `0x01` | `0x03` | 按名称写入变量值和上下限 |
| 列表上报 | `0x01` | `0x04` | 周期任务分帧上报注册项信息 |
| 选择波形变量 | `0x01` | `0x05` | 设置或清除目标变量的 `SHELL_STA_AUTO` |
| 设置波形周期 | `0x01` | `0x06` | 设置波形帧间隔 |
| 波形数据上报 | `0x01` | `0x07` | 上报自动选择变量的数据 |
| 启停波形上报 | `0x01` | `0x0C` | 开启或停止波形流 |

二进制协议使用变长名称字段。接收侧会用 `name_len` 计算真实 payload 长度，避免固定拷贝完整 `SHELL_STR_SIZE_MAX`。

## 7. 列表上报

上位机发送 Shell 数量查询后，`shell_data_num_act()` 返回注册项数量，并缓存本次请求的源地址、目标地址和输出链路。

随后 `shell_data_report_act()` 通过 `REG_TASK_MS(50, shell_data_report_act)` 周期执行，每次上报一个注册项，直到链表遍历完成。

列表上报结构为 `shell_report_list_t`，包含：

- 名称长度
- 类型
- 当前值
- 上限
- 下限
- 自动上报标志
- 名称

## 8. 变量读写

读变量流程：

1. 按 `name/name_len` 调用 `shell_find()` 查找注册项。
2. 如果注册项存在且带 `func`，先调用 `func` 刷新变量。
3. 回包 `type/data/name`。

写变量流程：

1. 按名称找到注册项。
2. 根据注册项类型，把 `data/data_max/data_min` 拷贝到变量和上下限对象。
3. 重新执行限幅。
4. 回包写入后的值和上下限。
5. 如果注册项带 `func`，写入成功后调用回调。

当前二进制读写的数据字段为 `uint32_t` 承载。小于 32 位的整数按真实类型拷贝，`SHELL_FP32` 按 4 字节浮点数据拷贝。

## 9. 波形自动上报

波形服务使用 `SHELL_STA_AUTO` 标记需要上报的变量。

控制流程：

1. 上位机通过 `CMD_WORD_SHELL_WAVE_ENABLE_PARAM` 选择变量。
2. 上位机通过 `CMD_WORD_SHELL_WAVE_PERIOD` 设置上报周期。
3. 上位机通过 `CMD_WORD_SHELL_WAVE_START` 启动波形流。
4. `shell_wave_report_task()` 每 1 ms 运行一次，通过状态机输出一帧波形数据。

波形状态机：

| 状态 | 行为 |
| --- | --- |
| `IDLE` | 等待启动标志 |
| `START` | 发送 `0x55555555` 帧起始标记 |
| `DATA` | 遍历链表，上报所有 `SHELL_STA_AUTO` 变量 |
| `END` | 发送 `0xAAAAAAAA` 帧结束标记 |
| `WAIT` | 等待配置的帧间隔后进入下一帧 |

## 10. 使用示例

变量注册：

```c
static uint32_t demo_counter;
static float demo_gain = 1.0f;

REG_SHELL_VAR(DEMO_COUNTER, demo_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_GAIN, demo_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
```

命令注册：

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

链路接入：

```c
DECLARE_SHELL_CTX(s_shell_ctx);

static section_link_handler_t s_handler[] = {
    {.func = shell_run, .ctx = (void *)&s_shell_ctx},
};
```

## 11. 当前约束

- 不使用动态内存。
- Shell 名称最大 40 字节。
- 字符串 Shell 默认关闭。
- 字符串解析缓冲为每链路 128 字节。
- 二进制服务按名称查找注册项。
- 波形上报只上报设置了 `SHELL_STA_AUTO` 的变量。
- 变量读写需要保证注册类型和变量真实类型一致。
