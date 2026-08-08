# PLECS 与 FRAME 调试通信

## 1. 工程位置

PLECS-FRAME 调试工程位于：

```text
platform/plecs/frame_bridge/
```

工程内容：

| 路径 | 内容 |
| --- | --- |
| `frame_bridge.plecs` | 包含 DLL Block、输入常量和输出显示的 PLECS 模型 |
| `CMakeLists.txt` | MinGW-w64 DLL 构建配置 |
| `compile.bat` | Windows 编译入口 |
| `../common/comm.c`、`../common/comm.h` | PLECS 专用协议解析、Windows section 扫描和 TCP 墙钟超时 |
| `../common/dbg/` | PLECS 专用完整调试模块副本，包含 Scope、SFRA、Perf、Shell、Trace 和 Section 列表服务 |
| `app/` | 仿真输入输出、Scope、SFRA、Trace 和 Perf 计数器接入 |
| `comm/` | FRAME TCP 服务 |

## 2. 通信结构

PLECS 加载 `build/bin/libplecs.dll`。DLL 在仿真启动时监听本机 TCP 端口 `5000`，接收 FRAME Ethernet Transport 发送的 `0xE8` 协议帧。

TCP 服务绑定 `0.0.0.0:5000`，允许 FRAME 使用 `127.0.0.1` 或运行 PLECS 的计算机局域网 IPv4 地址连接。协议本机地址为 `0x02`，动态地址为 `0x00`。

工程使用 `platform/plecs/common/dbg/` 中的完整调试模块副本，并在该副本中适配 Windows linker section。
PLECS 仿真变量通过 `REG_SHELL_VAR` 注册，参数列表、单参数读写和实时波形统一由 Shell 服务处理：

PLECS 构建使用 `platform/plecs/common/comm.c`，不编译 `code/comm/comm.c`。MCU 通信实现及其超时行为保持独立。
PLECS 不引用或编译 `code/dbg/` 中的源文件和头文件，MCU 调试实现保持不变。

| 命令集 | 命令字 | 功能 |
| --- | --- | --- |
| `0x01` | `0x01` | 返回参数数量并连续发送参数列表 |
| `0x01` | `0x02` | 读取单个参数 |
| `0x01` | `0x03` | 写入单个参数 |
| `0x01` | `0x04` | 参数列表项上报 |
| `0x01` | `0x05` | 选择实时波形参数 |
| `0x01` | `0x06` | 设置实时波形周期 |
| `0x01` | `0x07` | 实时波形数据上报 |
| `0x01` | `0x0C` | 启动或停止实时波形 |

## 3. 编译与运行

在工程目录运行：

```bat
cd /d D:\OneDrive\LWX\GD32\base\platform\plecs\frame_bridge
compile.bat
```

输出文件：

```text
platform/plecs/frame_bridge/build/bin/libplecs.dll
platform/plecs/frame_bridge/build/bin/plecs.map
```

使用 PLECS 打开 `platform/plecs/frame_bridge/frame_bridge.plecs` 并启动仿真。模型使用较长的仿真时长，便于 FRAME 建立连接；联调结束后在 PLECS 中手动停止。模型的 DLL Block 已配置相对路径：

```text
build\bin\libplecs.dll
```

## 4. FRAME 连接配置

FRAME 顶部通信配置填写：

| 字段 | 本机运行 PLECS 时的值 |
| --- | --- |
| Transport | `Ethernet` |
| Host | `127.0.0.1` |
| TCP Port | `5000` |

参数页目标地址使用：

| 字段 | 值 |
| --- | --- |
| Target Address | `0x02` |
| Dynamic Address | `0x00` |

点击连接后进入“参数读写”页面，点击“读取列表”。FRAME 接收数量 ACK 后显示全部仿真参数。

## 5. 参数定义

| 参数 | 类型 | 访问 | 含义 |
| --- | --- | --- | --- |
| `SIM_TIME_S` | FP32 | 只读 | 当前仿真时间，单位 s |
| `SIM_INPUT` | FP32 | 只读 | DLL 输入信号 |
| `SIM_GAIN` | FP32 | 读写 | 输出增益，默认范围 -10 到 10，FRAME 写入时可同步修改范围 |
| `SIM_OFFSET` | FP32 | 读写 | 输出偏置，默认范围 -100 到 100，FRAME 写入时可同步修改范围 |
| `SIM_OUTPUT` | FP32 | 只读 | DLL 输出信号 |
| `SIM_STEP_COUNT` | UINT32 | 只读 | DLL 仿真回调次数 |
| `FRAME_TCP_PORT` | UINT32 | 只读 | TCP 服务端口 |
| `DEMO_SHELL_COUNTER` | UINT32 | 读写 | Shell Demo 计数器 |
| `DEMO_SHELL_GAIN` | FP32 | 读写 | Shell Demo 增益，范围 0.1 到 10 |

模型计算关系：

```text
SIM_OUTPUT = SIM_INPUT * SIM_GAIN + SIM_OFFSET
```

FRAME 修改 `SIM_GAIN` 或 `SIM_OFFSET` 后，PLECS 模型输出从下一个采样点开始使用新值。

工程直接编译 `code/app/demo/demo_shell.c`，该文件保持原样。`DEMO_SHELL_PING` 命令也会注册到 Shell
链表，并以 `SHELL_CMD` 类型随完整 Shell 列表上报；命令节点的数据和范围字段为 0。

## 6. 实时波形

在“参数读写”页面勾选需要观察的参数，然后进入“波形”页面：

1. 设置上报周期，单位 ms。
2. 点击启动。
3. FRAME 按批次接收已勾选参数。
4. 点击停止结束上报。

实时波形由 Shell 服务任务按照 PLECS 仿真时基调度。

## 7. Scope

工程注册的 Scope 名称为：

```text
frame_simulation
```

Scope 配置：

| 项目 | 值 |
| --- | --- |
| 采样周期 | `100 us` |
| 样本数 | `512` |
| 触发后样本数 | `128` |

包含变量：

- `scope_sim_input`
- `scope_sim_output`
- `scope_sim_gain`
- `scope_sim_offset`

在 FRAME 的 Scope 页面依次刷新 Scope 列表、读取变量、启动、触发并拉取数据。

## 8. SFRA

工程注册的 SFRA 实例名称为：

```text
frame_gain
```

SFRA 将小信号注入 DLL 输入，采集 DLL 输出，测量当前 `SIM_GAIN` 对应的数学增益通道。默认配置：

| 项目 | 值 |
| --- | --- |
| 采样频率 | `10 kHz` |
| 注入幅值 | `0.05` |
| 扫频范围 | `10 Hz` 到 `1000 Hz` |
| 注入延迟 | `0` |

在 FRAME 的 SFRA 页面刷新实例列表后，可以修改扫频范围和注入幅值并启动扫频。

## 9. Perf

Perf 使用 Windows `QueryPerformanceCounter` 生成 `100 ns` 计数基准，测量以下运行项：

- PLECS 仿真采样回调
- 参数实时波形任务
- Scope 服务任务
- SFRA 服务和计算任务
- Perf 服务任务
- Trace 服务任务

在 FRAME 的 Perf 页面拉取字典后即可读取任务和中断执行时间。Perf 测量的是运行 PLECS 的计算机上的实际执行耗时，不是仿真时间跨度。

## 10. Trace

Trace 使用 PLECS 的 `100 us` 仿真时基。当前记录点包括：

- 仿真实例初始化
- `SIM_GAIN` 修改
- `SIM_OFFSET` 修改

在 FRAME 的 Trace 页面启动上报，然后修改 `SIM_GAIN` 或 `SIM_OFFSET`，即可收到对应记录。

## 11. Section 链表

工程接入 `0x01/0x38` 和 `0x01/0x39` 链表协议。FRAME 的 Section 链表页面可以刷新当前 DLL 中的注册目录和节点，包含 init、task、interrupt、comm、scope、shell、sfra 和 perf 等运行链表。

PLECS DLL 是 64 位进程，协议节点地址字段仍保持现有 32 位格式。链表数量和节点可以读取；使用 MAP 文件做符号匹配时，应考虑 Windows DLL 装载地址和 32 位协议地址字段的差异。

## 12. 未接入的硬件功能

以下 FRAME 页面依赖真实设备状态或非易失存储，不在该数学仿真工程中构造虚假响应：

- 固件升级
- 黑匣子
- 工厂模式
- PCS 设备主页和故障状态

## 13. 运行日志

运行日志位于 DLL 输出目录：

```text
platform/plecs/frame_bridge/build/bin/plecs_log.txt
```

日志记录 TCP 监听、客户端连接、断开和 Winsock 错误。端口被占用时，日志包含 `bind port 5000 failed`。
