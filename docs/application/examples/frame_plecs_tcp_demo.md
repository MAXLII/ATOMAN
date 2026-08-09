# PLECS 与 FRAME TCP 联调 Demo

本文使用 ATOMAN 的 `platform/plecs/frame_bridge/` 工程，在没有目标板和串口线的情况下，通过本机 Ethernet TCP 验证 FRAME 参数、波形和调试页面。

## 1. Demo 数据路径

```text
FRAME GUI
    │ Ethernet TCP 127.0.0.1:5000
    ▼
PLECS frame_bridge.plecs
    │ DLL Block
    ▼
build/bin/libplecs.dll
    ├─ 参数与实时波形
    ├─ Scope / SFRA / Perf / Trace
    └─ Section 链表
```

协议地址为 `Target Address = 0x02`、`Dynamic Address = 0x00`。

## 2. 前置条件

- Windows 10/11；
- PLECS；
- MinGW-w64 和 CMake，可由 `compile.bat` 正常调用；
- [FRAME Release](https://github.com/MAXLII/FRAME/releases/latest) 或 FRAME 源码环境。

## 3. 编译 DLL

在 ATOMAN 仓库根目录执行：

```powershell
Set-Location .\platform\plecs\frame_bridge
.\compile.bat
```

成功后生成：

```text
platform/plecs/frame_bridge/build/bin/libplecs.dll
platform/plecs/frame_bridge/build/bin/plecs.map
```

## 4. 启动 PLECS 仿真

打开并运行：

```text
platform/plecs/frame_bridge/frame_bridge.plecs
```

必须启动仿真，DLL 才会监听 TCP 端口。服务绑定 `0.0.0.0:5000`，本机 FRAME 使用 `127.0.0.1:5000`。

## 5. 连接 FRAME

FRAME 顶部通信配置：

| 字段 | 值 |
|---|---|
| Transport | `Ethernet` |
| Host | `127.0.0.1` |
| TCP Port | `5000` |

功能页面协议地址：

| 字段 | 值 |
|---|---|
| Target Address | `0x02` |
| Dynamic Address | `0x00` |

点击连接。若连接立即断开，先确认 PLECS 仿真正在运行。

## 6. Demo 一：参数读写

在参数页点击“读取列表”，应看到：

- `SIM_TIME_S`
- `SIM_INPUT`
- `SIM_GAIN`
- `SIM_OFFSET`
- `SIM_OUTPUT`
- `SIM_STEP_COUNT`
- `FRAME_TCP_PORT`
- `DEMO_SHELL_COUNTER`
- `DEMO_SHELL_GAIN`

修改 `SIM_GAIN` 或 `SIM_OFFSET` 后，模型从下一个采样点开始使用新值：

```text
SIM_OUTPUT = SIM_INPUT * SIM_GAIN + SIM_OFFSET
```

## 7. Demo 二：实时波形

1. 在参数页勾选 `SIM_TIME_S`、`SIM_INPUT` 和 `SIM_OUTPUT`。
2. 进入波形页设置上报周期。
3. 启动波形。
4. 修改 `SIM_GAIN`，观察输出幅值变化。
5. 停止波形。

批量波形使用 PLECS 提供的仿真时钟，因此横轴反映仿真时间，不是 Windows 墙钟时间。

## 8. Demo 三：Scope

Scope 名称为 `frame_simulation`：

| 项目 | 值 |
|---|---|
| 采样周期 | `100 us` |
| 样本数 | `512` |
| 触发后样本数 | `128` |
| 通道 | input、output、gain、offset |

在 Scope 页面依次执行刷新列表、读取变量、启动、触发和拉取数据。

## 9. Demo 四：SFRA、Perf、Trace

### SFRA

刷新实例列表并选择 `frame_gain`。默认扫频范围为 `10 Hz` 到 `1000 Hz`，注入幅值为 `0.05`。启动后等待扫频完成并查看幅频、相频结果。

### Perf

读取 Perf 信息和字典，再刷新采样。数据反映 PLECS DLL 在 Windows 上的实际执行耗时。

### Trace

启动 Trace 上报，然后修改 `SIM_GAIN` 或 `SIM_OFFSET`，应收到对应执行记录。

## 10. Demo 五：Section 链表

进入 Section 链表页面并刷新，应看到 init、task、interrupt、comm、scope、shell、sfra 和 perf 等注册目录及节点。这一步可以确认各调试服务确实进入 DLL。

## 11. 结果判定

| 检查项 | 通过条件 |
|---|---|
| TCP 连接 | FRAME 成功连接 `127.0.0.1:5000` |
| 参数 | 能读取并修改 `SIM_GAIN`、`SIM_OFFSET` |
| 波形 | `SIM_OUTPUT` 随参数变化 |
| Scope | 能拉取 `frame_simulation` 的 512 点数据 |
| SFRA | 能完成 `frame_gain` 扫频 |
| Perf | 能读取字典和采样 |
| Trace | 修改参数后收到记录 |
| Section | 能读取注册目录和节点 |

## 12. 常见问题

- `Connection refused`：PLECS 模型尚未运行或端口不是 `5000`。
- 端口占用：检查其他 PLECS 实例或进程是否占用 5000。
- 参数页超时：检查目标地址是否为 `0x02/0x00`。
- 功能列表为空：确认使用的是最新编译生成的 `build/bin/libplecs.dll`。
- 查看通信日志：`platform/plecs/frame_bridge/build/bin/plecs_log.txt`。

## 13. 关联导航

- [ATOMAN 与 FRAME 配合使用](../communication/frame_atoman_integration.md)
- [PLECS 与 FRAME 详细说明](../communication/plecs_frame_bridge.md)
- [FRAME 仓库](https://github.com/MAXLII/FRAME)
