# 通信命令开发方法

## 1. 定义线格式

新增命令前先确定命令集、命令字、请求字段、直接应答字段和最小合法长度。已发布字段保持顺序和语义稳定；扩展字段只追加在结构体尾部，不插入保留字段。

对于固定二进制结构，应增加大小和关键偏移断言：

```c
_Static_assert(sizeof(example_request_t) == 8U, "request size mismatch");
_Static_assert(offsetof(example_request_t, value) == 4U, "value offset mismatch");
```

跨编译器协议优先显式逐字段编解码。若使用打包结构体，也必须显式规定字节序，并避免把 C 位域作为线格式。

## 2. 兼容解析

接收端先清零本地对象，再按收到长度与本地结构体大小的较小值复制：

```c
example_request_t request = {0};
uint16_t copy_len = p_pack->len;

if (copy_len > (uint16_t)sizeof(request))
{
    copy_len = (uint16_t)sizeof(request);
}

if (copy_len >= EXAMPLE_REQUEST_MIN_SIZE)
{
    (void)memcpy(&request, p_pack->p_data, copy_len);
}
```

`EXAMPLE_REQUEST_MIN_SIZE` 表示完成当前命令所需的字段前缀，而不是强制要求等于本地结构体完整大小。新接收端读取旧请求时，尾部新增字段保持清零后的默认值。

## 3. 注册回调

命令通过 `REG_COMM` 注册。回调只完成长度、地址、字段和状态检查，随后提交业务事件。耗时 Flash、文件、控制等待或网络操作应交给周期状态机推进。

直接应答必须使用与请求相同的 `cmd_set` 和 `cmd_word`，并设置 ACK。由该请求触发的后续进度或结果上报使用自己的命令字，按普通上报发送。

## 4. 幂等与重试

有丢包重试需求的命令应携带会话号、序号或偏移。接收端至少区分：

- 新请求：接受并执行；
- 上一请求的重复：返回相同结果，不重复产生副作用；
- 错序请求：明确拒绝；
- 已结束会话的迟到请求：拒绝或返回已完成状态。

## 5. 发送结构体

发送端不要直接发送含指针、自然对齐空洞、平台枚举宽度或位域的内存镜像。整数按协议字节序写入发送缓冲区，再计算长度和 CRC。发送缓冲区生命周期必须覆盖底层异步发送过程。

## 6. 验收清单

- 最小长度、完整长度和更长的新版本报文均有测试。
- 截断字段、非法枚举、越界数值和错误 CRC 被拒绝。
- 同命令直接响应才标记 ACK。
- 重复请求不会重复执行不可逆动作。
- 结构体大小、字段偏移和字节序有编译期或主机测试约束。
- 回调中没有阻塞设备操作。

## 7. 关联导航

- 源码：[FRAME 核心](../../../code/comm/comm.c) · [SFRA 服务](../../../code/dbg/sfra_service.c) · [Bootloader 协议类型](../../../code/app/bootloader/protocol/bootloader_protocol_types.h)
- 设计：[协议演进与兼容设计](../../design/communication/protocol_evolution_design.md) · [FRAME通信核心设计](../../design/communication/frame_design.md)
