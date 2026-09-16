# 仿真 TCP 组件与平台分层

## 三层职责

| 层 | 路径 | 职责 |
|---|---|---|
| 组件 | `code/sim/comm/sim_tcp.c/.h` | 定义 `REG_SIM_TCP`；扫描 SECTION 注册表形成链表，管理 TCP 连接、重连和字节缓冲 |
| BSP | `platform/<平台>/<项目>/bsp/bsp_tcp.c/.h` | 注册实际端口、角色、IP 和通道名称，提供 `bsp_tcp_*` 接口 |
| commlink | `code/interface/<项目>/sim/comm_link.c` | 使用 BSP 收发接口注册 `REG_LINK`，连接字节流与上层协议，维护协议解析上下文 |

MATLAB 的 `frame_route_bridge` 将 BSP 拆为 `bsp/node02`、`bsp/node03`，将 interface 拆为 `code/interface/frame_route_bridge/sim/node02`、`node03`。各节点分别注册 TCP 与协议上下文；PLECS 保留 `code/interface/frame_route_bridge/sim/comm_link.c`。其余项目为 `frame_bridge`、`buck`、`inv`、`inv_dq`、`npc`。这些源码显式加入 Windows 仿真构建，统一使用 MinGW64。

TCP 组件不包含平台判断、节点地址、FRAME 帧处理或 UDP 发现。`code/sim/protocol/sim_discovery.c` 单独处理 FRAME UDP 发现，`comm.c` 处理上层协议，`code/sim/debug` 保留仿真调试服务。

## 静态注册

在 BSP 中通过宏声明句柄并注册：

```c
REG_SIM_TCP(p_dbg_tcp, "dbg", SIM_TCP_SERVER, "0.0.0.0", 5000u)
REG_SIM_TCP(p_iso_tcp, "iso", SIM_TCP_CLIENT, "127.0.0.1", 5001u)
```

宏声明并初始化静态句柄，生成静态注册对象，并通过 `REG_SECTION_FUNC(SECTION_SIM_TCP, ...)` 放入 SECTION 段。参数必须能用于静态初始化；句柄参数必须是唯一的变量标识符，不需要提前声明。名称是组件查询通道的键，同一模块内必须唯一，字符串使用静态存储期。服务端 IP 是绑定地址，客户端 IP 是连接目标，当前支持数字 IPv4 地址。

`sim_tcp_init()` 调用 `section_collect(SECTION_SIM_TCP)` 收集静态链表，校验配置，为每项分配内部状态并回填句柄。不需要平台主动调用注册函数。当前固定池最多支持 8 条通道，无堆分配；无效配置和容量不足使对应句柄保持 NULL，并在注册项 `status.last_error` 中记录错误。端口绑定失败保留通道并按墙钟重试；注册成功不代表连接成功。

## BSP 与 commlink

每条链路提供三个收发接口，例如：

```c
void bsp_tcp_dbg_printf(const char *p_format, ...);
void bsp_tcp_dbg_tx(char *p_data, int len);
uint8_t bsp_tcp_dbg_rx_get_byte(uint8_t *p_data);
```

内部调用 `sim_tcp_vprintf`、`sim_tcp_tx` 和 `sim_tcp_rx_get_byte`，传入本 BSP 的句柄。路由项目另提供 `bsp_tcp_iso_*`。

`sim_tcp_get_status("dbg")`、`sim_tcp_get_status("iso")` 返回只读的连接状态、会话代次、完整写入丢弃计数和最近的初始化/绑定错误。commlink 通过 `DECLARE_COMM_CTX(..., "dbg")` 填写 TCP 注册名称，由协议组件直接查询 TCP 组件状态，处理器表直接填写 `.func = comm_run`。协议组件在 `comm_run()` 内检查会话代次并复位解析器，避免跨连接拼接半帧；每次仿真初始化也由协议组件重置已注册的解析上下文。真实毫秒时钟由仿真协议组件内部获取，commlink 不包含 Windows 头文件，也不直接调用 Windows API。上层协议使用 SECTION 现有发送函数结构，其字段名保持框架接口兼容。

每通道有 64 KiB 接收环和 1 MiB 发送环。发送立即复制字节，容量不足则拒绝本次完整写入；接收满时由 TCP 回压。断线清空队列。格式化输出小于 1024 字节，超长消息不发送。

## 初始化、任务和停止

- `REG_INIT(10, sim_tcp_init)`：收集注册链表，初始化 TCP。
- `REG_TASK(1, sim_tcp_task)`：遍历已建立的通道状态，进行有界非阻塞维护。
- `REG_INIT(11, sim_discovery_init)` 与对应任务：独立初始化和维护 UDP 发现。
- `sim_comm_stop()`：由 BSP 实现，依次停止发现服务和 TCP 组件；公共宿主仍提供弱空实现。

所有操作在仿真上下文串行执行，无后台线程。结束回调和 `mdlTerminate` 直接调用 `sim_comm_stop()`。停止清空通道句柄和 Socket，保留静态注册项；重复运行时重新收集和初始化。重试间隔 100 ms，挂起连接超时 1000 ms，均使用墙钟。暂停仿真也暂停通信任务。

## 项目配置与发现

MATLAB 的 IP、端口宏位于 `bsp/node02/bsp_tcp.h` 和 `bsp/node03/bsp_tcp.h`；设备地址宏 `COMM_LINK_DEVICE_ADDR`、对端地址宏 `COMM_LINK_PEER_ADDR` 位于对应 interface 节点目录的 `comm_link.h`。CMake 仅选择源码目录。其他项目保留 `bsp/bsp_tcp.h`；TCP 组件不消费 BSP 配置宏。

| 节点 | FRAME TCP | 内部 TCP | 发现 |
|---|---|---|---|
| 02 | 5000 | 客户端连接 127.0.0.1:5001 | UDP 5000 通告 02/03 |
| 03 | 5002 | 服务端监听 127.0.0.1:5001 | 不单独启动发现监听 |

`REG_SIM_DISCOVERY(id, name, node_addr, tcp_port, udp_port)` 注册发现内容；MATLAB 的注册位于 node02 commlink，使用 interface 的设备地址宏与 BSP 的网络配置宏。一个服务实例的所有通告使用相同 UDP 端口；名称前缀由 BSP 配置，PLECS 为 `PLECS-SIM`，MATLAB 为 `MATLAB-SIM`，协议服务追加节点地址后缀。端口与名称不再由 TCP 组件硬编码。

MATLAB 仍保留两个独立编译脚本，MEX 输出到项目 `build`。详见 [S-Function 说明](MATLAB_FRAME_ROUTE_BRIDGE_USAGE.md)。
