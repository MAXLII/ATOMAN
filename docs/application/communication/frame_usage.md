# FRAME通信接入

本文说明如何把FRAME解析器挂到Section Link、注册本机命令、发送直接ACK，以及在确有网络转发需求时配置路由。

## 1. 接入准备

目标工程需要提供：

- 一套Section运行时；
- 至少一条已注册的Section Link；
- 本机静态地址；
- 每条FRAME链路独立的`comm_ctx_t`和payload缓冲区；
- 可满足最大协议payload的静态内存。

公共源码为：

```text
code/comm/comm.c
code/comm/comm.h
```

## 2. 为每条链路声明上下文

```c
#include "comm.h"

#define DEVICE_ADDR 0x21u
#define DEBUG_LINK_ID 2u

DECLARE_COMM_CTX(s_debug_comm_ctx, 256u, DEVICE_ADDR, DEBUG_LINK_ID);
```

`256u`是本链路可接收的最大payload，不包含FRAME固定头尾。升级链路等大包协议应使用满足其最大请求的数据区；普通控制命令不应因为另一固件角色需要大包而盲目扩大。

同一物理链路同时运行Shell和FRAME时，分别声明协议上下文，由Link同步扇出字节。不要让两个协议共用解析状态。

## 3. 挂到Section Link

把`comm_run`和上下文作为一个handler加入链路handler表：

```c
static const section_link_handler_item_t s_handlers[] = {
    {
        .func = comm_run,
        .ctx = &s_debug_comm_ctx,
    },
};
```

具体链路描述符和收发驱动按照[Section使用文档](../framework/section/section_usage.md)注册。`comm_run()`按字节调用，发送函数由当前Link传入，因此命令ACK会沿请求来源返回。

## 4. 注册本机命令

本机命令通过`REG_COMM`注册。payload线格式、长度兼容、回调职责、直接ACK和异步上报统一按[通信命令开发方法](command_development_usage.md)实现。

FRAME接入层只保证有效帧能够沿当前Link到达命令回调，并让直接响应沿请求来源返回。需要Flash、参数保存或其他等待动作时，回调提交事件后立即返回，实际工作由周期状态机推进。

## 5. 配置路由

只有目标设备确实承担协议转发时才注册路由：

```c
REG_COMM_ROUTE(DEBUG_LINK_ID, FIELD_LINK_ID, FIELD_DEVICE_ADDR)
```

含义是：从`DEBUG_LINK_ID`收到、目的地址为`FIELD_DEVICE_ADDR`的有效帧，由`FIELD_LINK_ID`发出。

普通本机升级、调试和参数命令不需要路由项。业务回调主动发出另一条命令也不属于FRAME透明路由。

## 6. 容量与运行检查

- 确认payload容量覆盖本链路最大请求。
- 确认不同链路使用不同`link_id`和`comm_ctx_t`。
- 确认协议回调不会持有接收payload指针到回调结束之后。
- DMA发送链路必须在底层保证发送缓冲生命周期。
- 检查噪声、截断帧和CRC错误后解析器能够继续接收下一帧。
- 检查本机命令、未知命令和路由帧走入正确路径。
- 检查主动上报没有错误设置ACK。

## 7. 关联导航

### 源代码

- [FRAME公共接口](../../../code/comm/comm.h)
- [FRAME解析与路由实现](../../../code/comm/comm.c)
- [Section Link接口](../../../code/section/baremetal/section.h)

### 设计文档

- [FRAME通信核心设计](../../design/communication/frame_design.md)
- [Section Link设计](../../design/framework/section/link_design.md)

### 相关应用

- [通信命令开发方法](command_development_usage.md)
