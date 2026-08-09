# GD32G553 串口与 FRAME 联调 Demo

本文使用 ATOMAN 的 `platform/gd32g553c/` 工程和 `code/app/demo/` 公共示例，通过 FRAME 验证串口协议回环、参数读写、实时波形和 Scope。

## 1. Demo 配置

当前 GD32G553 工程已经编译通信与调试 Demo：

```text
code/app/demo/demo_comm.c
code/app/demo/demo_shell.c
code/app/demo/demo_scope.c
code/app/demo/demo_perf.c
code/app/demo/demo_sfra.c
code/app/demo/demo_trace.c
```

固定通信配置为：

| 项目 | 值 |
|---|---|
| MCU 外设 | `USART0` |
| TX | `PA9` |
| RX | `PA10` |
| 串口格式 | `921600 8N1` |
| FRAME Target Address | `0x02` |
| FRAME Dynamic Address | `0x00` |

`HOST_ADDR` 默认等于 `LLC_ADDR`，即 `0x02`。USART0 收到的字节同时送入字符串 Shell 和 FRAME 二进制协议解析器，两套解析器使用独立上下文。

## 2. 准备硬件

需要：

- GD32G553 目标板；
- 支持 3.3 V TTL 电平的 USB 转串口模块；
- 调试器或现有固件下载工具。

串口连接：

```text
GD32 PA9  (TX) ── USB-UART RX
GD32 PA10 (RX) ── USB-UART TX
GD32 GND       ── USB-UART GND
```

不要把 RS-232 电平接口直接连接到 MCU TTL 引脚。

## 3. 编译并下载固件

在 ATOMAN 仓库根目录执行：

```powershell
.\platform\gd32g553c\compile.bat
```

构建目标为 `demo`。编译脚本生成 `platform/gd32g553c/build/demo.elf`，并在带时间戳的 `platform/gd32g553c/builds/` 子目录保存追加固件信息后的 `demo.bin`。

使用项目现有下载方式把固件写入目标板，复位后确认目标板持续运行 `section_init()` 和 `run_task()`。

## 4. 启动 FRAME

普通用户从 [FRAME 最新 Release](https://github.com/MAXLII/FRAME/releases/latest) 安装并启动 GUI。使用源码时执行：

```powershell
Set-Location ..\FRAME
.\frame.ps1 gui
```

在顶部通信区域填写：

| 字段 | 值 |
|---|---|
| Transport | `Serial` |
| Port | Windows 识别到的实际 COM 口 |
| Baud | `921600` |
| Data bits | `8` |
| Parity | `None` |
| Stop bits | `1` |

连接后，在需要目标地址的页面填写 `Target Address = 0x02`、`Dynamic Address = 0x00`。

## 5. 验证参数读写

进入 FRAME 参数页并点击“读取列表”，应至少看到：

| 名称 | 类型 | 范围 |
|---|---|---|
| `DEMO_SHELL_COUNTER` | `UINT32` | `0` 到 `0xFFFFFFFF` |
| `DEMO_SHELL_GAIN` | `FP32` | `0.1` 到 `10.0` |
| `DEMO_SHELL_PING` | 命令 | 无数值范围 |

操作步骤：

1. 读取 `DEMO_SHELL_COUNTER`。
2. 写入一个新值，例如 `123`。
3. 再次读取并确认值为 `123`。
4. 写入 `DEMO_SHELL_GAIN = 2.5`。
5. 执行 `DEMO_SHELL_PING`，该命令会把计数器加一。

使用 FRAME 源码 CLI 可以执行同样的检查：

```powershell
.\frame.ps1 param list --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
.\frame.ps1 param read DEMO_SHELL_COUNTER --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
.\frame.ps1 param write DEMO_SHELL_COUNTER 5 123 --min 0 --max 4294967295 --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
.\frame.ps1 param write DEMO_SHELL_GAIN 6 2.5 --min 0.1 --max 10 --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
```

其中类型编号 `5` 表示 `SHELL_UINT32`，`6` 表示 `SHELL_FP32`。把示例中的 `COM8` 替换为实际端口。

## 6. 验证协议回环

`demo_comm.c` 注册了 `cmd_set = 0x30`、`cmd_word = 0x01` 的结构体回环命令。发送以下 7 字节 payload：

```text
78 56 34 12 A5 85 FF
```

字段解释：

| 字段 | 值 | 线格式 |
|---|---|---|
| `counter` | `0x12345678` | `78 56 34 12` |
| `led_mask` | `0xA5` | `A5` |
| `temperature_x10` | `-123` | `85 FF` |

FRAME CLI 命令：

```powershell
.\frame.ps1 proto --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00 --cmd-set 0x30 --cmd-word 0x01 --payload "78 56 34 12 A5 85 FF"
```

正确结果是收到相同 `cmd_set/cmd_word`、`is_ack = 1` 且 payload 仍为 `78 56 34 12 A5 85 FF` 的响应。

## 7. 验证实时波形

1. 在参数页勾选 `DEMO_SHELL_COUNTER` 和 `DEMO_SHELL_GAIN`。
2. 进入波形页设置上报周期。
3. 点击启动。
4. 修改两个参数，确认曲线随上报批次更新。
5. 点击停止，避免离开页面后仍持续占用链路带宽。

实时参数波形来自 Shell 服务，不是 Scope 缓冲录波。

## 8. 验证 Scope

`demo_scope.c` 注册了：

| 项目 | 值 |
|---|---|
| Scope 名称 | `demo_scope_wave` |
| 采样周期 | `10 ms` |
| 样本数 | `100` |
| 触发后样本数 | `50` |
| 通道 | `demo_scope_sin`、`demo_scope_cos` |

在 FRAME Scope 页面执行：

1. 刷新 Scope 列表。
2. 选择 `demo_scope_wave` 并读取变量。
3. 点击启动。
4. 点击触发。
5. 等待触发后样本采集完成。
6. 拉取数据，应看到一组正弦和余弦波形。

CLI 可先检查列表：

```powershell
.\frame.ps1 scope list --port COM8 --baud 921600 --dst 0x02 --d-dst 0x00
```

## 9. 结果判定

| 检查项 | 通过条件 |
|---|---|
| 连接 | FRAME 不报告串口打开失败 |
| 参数列表 | 能看到 `DEMO_SHELL_COUNTER` 和 `DEMO_SHELL_GAIN` |
| 参数写入 | 写入后重新读取数值一致 |
| 协议回环 | ACK payload 与发送 payload 一致 |
| 实时波形 | 参数变化能连续显示 |
| Scope | 能拉取 100 点双通道正弦/余弦数据 |

## 10. 常见问题

- 列表读取超时：检查 COM 口、`921600 8N1`、TX/RX 交叉连接和目标地址 `0x02`。
- 能看到字符串日志但二进制页面超时：检查 `comm.c` 与对应 `*_service.c` 是否参与当前构建。
- Scope 列表为空：检查 `demo_scope.c`、`scope.c`、`scope_service.c` 和 Section 链接段。
- GUI 与 CLI 不能同时打开同一个 COM 口；运行 CLI 前先断开 GUI。

## 11. 关联导航

- [ATOMAN 与 FRAME 配合使用](../communication/frame_atoman_integration.md)
- [公共 Demo 总览](demo.md)
- [Demo 通信源码](../../../code/app/demo/demo_comm.c)
- [Demo 参数源码](../../../code/app/demo/demo_shell.c)
- [Demo Scope 源码](../../../code/app/demo/demo_scope.c)
