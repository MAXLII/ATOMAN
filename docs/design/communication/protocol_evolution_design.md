# 二进制协议演进与兼容设计

二进制协议一旦被固件、上位机和现场设备共同使用，字段顺序和语义就成为跨版本契约。兼容设计的目标不是预留大量未知空间，而是让旧端与新端在已知长度内交换共同字段，对未知扩展保持可判定行为。

## 1. 三层版本

协议中存在不同层次的版本：

- FRAME版本：帧头、地址、长度、CRC等基础封装版本；
- 命令payload版本：某条命令字段集合的演进；
- 业务对象版本：固件、字典、参数布局等对象自己的版本。

三者不能共用一个数字。基础帧不变时，新增payload尾部字段不需要修改FRAME版本；固件版本变化也不表示命令格式变化。

## 2. 已发布字段不可移动

兼容结构遵循尾部追加：

```text
V1: field_a | field_b
V2: field_a | field_b | field_c
V3: field_a | field_b | field_c | field_d
```

已发布字段的偏移、宽度、字节序和含义保持稳定。不能在中间插字段，也不能把同一bit改成另一种业务含义。

不主动添加reserved字段。未知未来需求无法被可靠预测，预留位容易被不同端私自复用。真正扩展发生时，在尾部增加有定义字段，并通过实际`data_len`兼容。

## 3. 长度驱动兼容

接收端先把本地目标初始化为默认值，再拷贝共同前缀：

```c
local_request_t request = {0};
uint16_t copy_len = MIN(p_pack->len, (uint16_t)sizeof(request));

memcpy(&request, p_pack->p_data, copy_len);
```

行为是：

- 旧代码接收新payload：只读取自己认识的前缀；
- 新代码接收旧payload：新增尾部字段保持默认值；
- 长度小于必需前缀：拒绝请求；
- 长度大于本地结构：忽略未知尾部，而不是写越界。

“只接受`len == sizeof(local)`”适用于永远不会扩展的固定数据块，不适合需要跨版本兼容的控制命令。

## 4. 必需前缀与可选尾部

不是所有短payload都有效。每条命令应定义：

```text
minimum_length：完成基本动作所需字段末端
known_length：当前代码认识的完整结构大小
received_length：本次帧实际payload长度
```

只有`received_length >= minimum_length`才允许处理。新增字段必须有安全默认值；若新字段成为动作必需条件，应新增命令或明确提升最低协议版本，不能让旧端在缺失安全字段时继续执行危险操作。

## 5. 字节序和对齐

线上的多字节整数必须规定字节序。结构体`packed`只能消除填充，不能自动转换CPU大小端，也不能保证未对齐访问在所有处理器上安全。

稳妥的解析方法是按偏移显式读取：

```c
static uint32_t read_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) |
           ((uint32_t)p[3] << 24u);
}
```

`_Static_assert(sizeof(type) == expected)`和`offsetof`断言可以检查当前编译器生成的布局，但不能替代线序定义。

## 6. 位域与联合体

C位域的位序、分配单元和跨字节布局具有实现定义行为。位域适合作为本编译目标中的可读视图，不应成为唯一的线协议定义。

协议应同时定义数值mask：

```c
#define REASON_OVERSIZE_MASK (1u << 0u)
#define REASON_VERSION_MASK  (1u << 1u)
```

发送时构造raw整数，接收时用mask判断。联合体字段视图也不能代替明确的字节序列化。

## 7. ACK与异步完成

ACK只表示对同一个`cmd_set/cmd_word`请求的直接响应：

```text
request X → ACK X，is_ack = 1
```

请求启动异步操作后，通过命令Y主动报告完成：

```text
request X → ACK X
后台状态机运行
device → report Y，is_ack = 0
```

把Y标成ACK会破坏主机事务匹配。直接ACK也不应承诺尚未完成的Flash写入、扫频或复位已经结束，应返回“已接受/忙/拒绝”等准确状态。

## 8. 会话、序号与幂等

超过一帧的事务需要额外身份：

- session或capture tag区分两次操作；
- sequence或offset确定分片顺序；
- 重复上一分片可以幂等确认；
- 跳跃、回退到更早分片或跨会话分片应拒绝；
- 会话超时后释放上下文，不能让新请求覆盖旧事务。

CRC证明字节传输一致，不证明分片顺序、业务权限或会话身份正确。

## 9. 请求解析与响应构造

通信回调的责任顺序：

1. 检查帧、payload和最小长度。
2. 初始化本地结构并复制共同前缀。
3. 显式解析多字节字段。
4. 校验枚举、范围、模块和当前状态。
5. 只提交快速业务事件。
6. 构造同命令直接ACK。
7. 长操作由周期状态机推进并另行上报。

响应结构也只包含真实字段。未初始化的栈字节不能被序列化到线上，因此响应对象必须完整初始化，发送长度必须等于真实有效字段长度。

## 10. 发布检查

- 为所有线结构提供字段偏移和总长度断言。
- 使用两个不同版本结构互相做长短payload测试。
- 验证未对齐地址和不同CPU大小端的解析方法。
- 验证未知尾部不会写越界。
- 验证缺失必需前缀被拒绝。
- 验证同命令ACK与异步上报区分正确。
- 验证重包、错序、超时和会话重建。
- 归档命令号、字段语义和首次发布版本。

## 11. 关联导航

### 应用文档

- [FRAME命令开发方法](../../application/communication/command_development_usage.md)
- [FRAME通信接入](../../application/communication/frame_usage.md)

### 基础教材

- [二进制序列化、对齐与版本兼容基础](../../tutorial/binary_serialization_and_compatibility.md)
- [嵌入式通信分发、性能与可靠性基础](../../tutorial/communication_performance_reliability.md)
