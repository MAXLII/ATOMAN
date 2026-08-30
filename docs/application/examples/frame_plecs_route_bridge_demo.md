# PLECS 双节点 TCP 路由 Demo

## 1. 工程结构

工程位于：

```text
platform/plecs/frame_route_bridge/
```

PLECS 模型加载两个相互独立的 DLL：

```text
                    TCP 127.0.0.1:5000
FRAME ───────────────────────────────► 节点 0x02 DLL
  │                                      │
  │ TCP 127.0.0.1:5002                   │ 内部 TCP 127.0.0.1:5001
  └───────────────────────────────► 节点 0x03 DLL
```

节点 `0x03` 监听内部 TCP 端口 `5001`，节点 `0x02` 主动连接并在断线后持续重连。TCP 连接建立后
为全双工通道。两个节点分别提供 Frame TCP 入口：节点 `0x02` 使用 `5000`，节点 `0x03` 使用
`5002`。FRAME 连接任一入口后，通过报文目的地址选择本地处理或路由至另一个节点。

## 2. 路由表

两个节点使用对称的 Section 路由：

| 来源链路 | 目的地址 | 输出链路 | 数据方向 |
|---|---:|---|---|
| Frame TCP，链路 `1` | `0x03` | 节点间 TCP，链路 `2` | Frame → 0x02 → 0x03 |
| 节点间 TCP，链路 `2` | `0x01` | Frame TCP，链路 `1` | 0x03 → 0x02 → Frame |
| Frame TCP，链路 `1` | `0x02` | 节点间 TCP，链路 `2` | Frame → 0x03 → 0x02 |
| 节点间 TCP，链路 `2` | `0x01` | Frame TCP，链路 `1` | 0x02 → 0x03 → Frame |

转发过程保留协议帧的源地址、目的地址、动态地址、命令集、命令字和 ACK 属性。

## 3. 构建

在工程目录执行：

```powershell
Set-Location .\platform\plecs\frame_route_bridge
.\compile.bat
```

生成文件：

```text
build/bin/plecs_node02/plecs_node02.dll
build/bin/plecs_node02/plecs_node02.map
build/bin/plecs_node03/plecs_node03.dll
build/bin/plecs_node03/plecs_node03.map
```

两个 DLL 分目录保存，各自的运行日志为同目录下的 `plecs_log.txt`。

## 4. 自动 TCP 测试

从仓库根目录执行：

```powershell
.\platform\plecs\frame_route_bridge\test\run_test.ps1
```

脚本重新构建两个 DLL，并通过真实的 `5000`、`5001`、`5002` 端口验证：

- UDP 搜索返回 `PLECS-SIM-02` 和 `PLECS-SIM-03` 两条独立设备记录；
- 无效 CRC 帧丢弃和后续解析恢复；
- 分片请求与连续请求；
- Frame 对节点 `0x02` 的直接回环；
- Frame 经节点 `0x02` 访问节点 `0x03` 的路由回环；
- Frame 对节点 `0x03` 的直接回环；
- Frame 经节点 `0x03` 访问节点 `0x02` 的路由回环；
- 两个节点的参数数量 ACK、参数列表上报和 `NODE_VALUE` 写入；
- 任一节点重启后的自动重连与双向通信恢复。

测试运行时端口 `5000`、`5001` 和 `5002` 需要处于空闲状态。

## 5. PLECS 运行

打开并运行：

```text
platform/plecs/frame_route_bridge/frame_route_bridge.plecs
```

模型包含两个 DLL Block。每个节点显示：

- `NODE_VALUE`：当前节点可由 FRAME 修改的数值；
- `PEER_CONNECTED`：内部 TCP 连接状态，连接成功时为 `1`。

模型启动后，节点 `0x03` 建立监听，节点 `0x02` 自动连接。两个节点的
`Peer Connected` 显示均为 `1` 表示内部链路已建立。

## 6. FRAME 配置

FRAME 搜索时通过 UDP `5000` 返回两个名称带节点地址的设备：

| 设备名称 | 节点地址 | TCP 地址 |
|---|---:|---|
| `PLECS-SIM-02` | `0x02` | `127.0.0.1:5000` |
| `PLECS-SIM-03` | `0x03` | `127.0.0.1:5002` |

直接连接节点 `0x02` 时使用：

| 字段 | 值 |
|---|---|
| Transport | `Ethernet` |
| Host | `127.0.0.1` |
| TCP Port | `5000` |

访问节点 `0x02`：

| 字段 | 值 |
|---|---|
| Target Address | `0x02` |
| Dynamic Address | `0x00` |

访问节点 `0x03` 时保持 TCP 连接，将目标地址改为：

| 字段 | 值 |
|---|---|
| Target Address | `0x03` |
| Dynamic Address | `0x00` |

直接连接节点 `0x03` 时将 TCP Port 改为 `5002`。此时目标地址 `0x03/0x00` 由本地节点处理，目标
地址 `0x02/0x00` 经节点间链路转发。FRAME 的现有 Ethernet 搜索和目标地址功能可直接完成联调。

## 7. 节点参数与回环命令

两个节点均注册：

| 参数 | 访问 | 含义 |
|---|---|---|
| `NODE_ADDR` | 只读 | 当前 DLL 的节点地址 `0x02` 或 `0x03` |
| `NODE_VALUE` | 读写 | 模型中显示的节点数值，范围 `0`～`1000000` |
| `LOOPBACK_COUNT` | 只读 | 当前节点处理的有效回环请求数 |
| `PEER_CONNECTED` | 只读 | 节点间 TCP 连接状态 |
| `FRAME_CONNECTED` | 只读 | 当前节点的 Frame TCP 客户端连接状态 |

回环命令为 `cmd_set=0x30`、`cmd_word=0x01`。节点使用相同命令直接返回 ACK，原样回送 payload，
并在 ACK 的 `src` 字段中标识实际处理请求的节点地址。

## 8. 验收步骤

1. 启动 PLECS 模型并确认两个 `Peer Connected` 显示为 `1`。
2. 在 FRAME 搜索结果中确认存在 `PLECS-SIM-02` 和 `PLECS-SIM-03`。
3. FRAME 连接 `PLECS-SIM-02`，使用目标地址 `0x02/0x00` 读取参数列表。
4. 修改节点 `0x02` 的 `NODE_VALUE`，确认对应 PLECS 显示同步更新。
5. 保持 TCP 连接，把目标地址切换为 `0x03/0x00` 并重新读取参数列表。
6. 断开后连接 `PLECS-SIM-03`，分别使用目标地址 `0x03/0x00` 和 `0x02/0x00` 验证直接与路由访问。
7. 停止并重新启动任一节点，确认内部连接与双向路由恢复。
