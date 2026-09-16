# MATLAB FRAME 双节点 S-Function

工程位于 `platform/matlab/frame_route_bridge`，提供两个独立的 Level-2 C MEX S-Function，供用户放入自己的 Simulink 模型。工程不提供模型文件，只保留两个独立编译 `.m` 脚本及共享 C 适配源码。

## 编译

需要 Windows x64 MATLAB/Simulink、CMake，以及 `C:/mingw64/bin` 下的 64 位 GCC/Make。

```matlab
cd('D:/OneDrive/LWX/GD32/base/platform/matlab/frame_route_bridge')
route_bridge_build_node02
route_bridge_build_node03
```

两个脚本用 MinGW64 将共用库直接链接进各自的 S-Function，输出为：

- `build/route_bridge_node02.mexw64`
- `build/route_bridge_node03.mexw64`

编译前须停止使用对应节点的仿真。两个脚本共用 CMake 构建目录，应顺序运行。编译后自动把 `build` 加入当前 MATLAB 搜索路径；新会话中执行：

```matlab
addpath('D:/OneDrive/LWX/GD32/base/platform/matlab/frame_route_bridge/build')
```

## S-Function 模块填写

| 字段 | 节点 02 模块 | 节点 03 模块 |
|---|---|---|
| S-Function 名称 | `route_bridge_node02` | `route_bridge_node03` |
| S-Function 参数 | 留空 | 留空 |
| S-Function 模块 | 留空 | 留空 |

名称不包含 `.mexw64` 后缀。参数中不填 `a`，模块中不填引号。

每个块有一个标量输入，接 Constant `0`，与 PLECS 示例的未使用输入对齐。输出为宽度 2 的向量 `[NODE_VALUE, PEER_CONNECTED]`，可接 Demux 或 Scope。

采样周期固定为 `1e-4 s`。建议使用 Normal 仿真模式、Fixed-step / discrete 求解器，步长 `1e-4 s`。开始仿真时 SECTION 自动初始化节点，通过非阻塞任务维护网络；结束或仿真出错时 `mdlTerminate` 调用 `sim_comm_stop()` 释放 Socket。没有后台通信线程，也没有普通 MATLAB 的 `start/step/stop` 命令接口。

每个节点 MEX 只允许一个活动块实例；重复放置同一节点会报错，避免共享 TCP 端口和内部状态。此适配层用于本机 Normal 模式，不支持代码生成、操作点保存恢复或 Fast Restart。

MEX 不再加载 PLECS 节点 DLL，移动产物时无需保留 PLECS 的 DLL 目录。CMake 中间文件位于 `build/cmake/`。

## 与 PLECS 对齐

两平台分别编译同一份 `code/sim/comm`、`code/sim/protocol` 和 `code/sim/debug` 源码。MATLAB 的 BSP 分为 `bsp/node02`、`bsp/node03`，分别在 `bsp_tcp.h` 中定义 IP 和端口，在 `bsp_tcp.c` 中注册 TCP。对应 interface 位于 `code/interface/frame_route_bridge/sim/node02` 和 `node03`，各自的 `comm_link.h` 定义设备、对端和 PC 地址，`comm_link.c` 负责协议上下文、链路、路由与发现注册；应用负责参数服务。CMake 只选择节点源码目录，不传入网络和设备地址配置。详细边界见 [统一仿真通信组件](SIM_COMM_USAGE.md)。

| 项目 | 行为 |
|---|---|
| 节点 0x02 | FRAME TCP 5000，内部 TCP 客户端 |
| 节点 0x03 | FRAME TCP 5002，内部 TCP 5001 服务端 |
| 发现 | UDP 5000，名称为 `MATLAB-SIM-02/03` |
| 回环 | `0x30/0x01`，支持本地与双向路由 ACK |
| 参数 | 原节点参数服务，包括 `NODE_VALUE` 和连接状态 |

进行 FRAME 交互时应使用仿真节奏控制，给外部客户端留出墙钟时间；很短的快速仿真可能在 TCP 建连前结束，暂停仿真也会暂停通信任务。不要与原 PLECS 示例同时占用相同端口。连接状态及最近的初始化/绑定错误可查看 `sim_tcp_get_status("dbg")` / `sim_tcp_get_status("iso")`；启动回调返回本身不代表监听成功。

这是通信仿真，不包含电力电路模型。参考工程的协议回归可在节点停止后从仓库根目录运行：

```powershell
python platform/plecs/frame_route_bridge/test/test_plecs_route_bridge.py
```
