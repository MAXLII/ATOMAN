# COMM v1 协议实现与自测报告

本文记录 `docs/comm_v1_protocol.md` 的代码实现范围、自测环境、用例与结果。协议文档描述实现约定，本文保存已验证的结果。

## 1. 实现范围

| 位置 | 内容 |
|---|---|
| `code/lib/codec_rle.c/.h` | CODEC=010 RLE / PackBits 编解码 |
| `code/lib/codec_dict.c/.h` | CODEC=001 静态字典编解码、内置码本、CRC32、`REG_INIT` 查找表 |
| `code/lib/codec_lzss.c/.h` | CODEC=011 LZSS-256 编解码（重叠回溯复制） |
| `code/data_source/comm/comm.h` | `section_packform_t` 尾部新增 `seq`；`comm_msg_t`、`COMM_V1_STA_E`、`comm_v1_ctx_t`、`DECLARE_COMM_V1_CTX`、`comm_sum_encode`、`comm_v1_run` 等接口与常量 |
| `code/data_source/comm/comm_v1.c` | v1 接收状态机（去重、地址过滤、SUM、解压、命令派发、路由转发）、CODEC_SELECT 协商、发送组包与压缩择优 |
| `code/data_source/comm/comm.c` | `comm_send_data` 按首字节 SOP 分派 0xE8 / 0xE9，其他值拒绝 |
| `code/interface/demo/common/comm_link.c` | demo 链路 handler 数组追加 `comm_v1_run` |
| `code/section/platform.h` | `PLATFORM_TESTBENCH` 补充 `__LDREXB/__STREXB/__DMB` 原子原语 |
| `code/dbg/*_service.c`、`code/data_source/demo/demo_comm.c`、`code/app/bootloader/*` | 补齐回包 SOP 显式赋值：直接 ACK 继承请求协议与 SEQ，主动上报固定 0xE8 |
| `platform/gd32e507/Makefile` | 接入 3 个 codec 与 `comm_v1.c` |
| `platform/testbench/comm_v1/` | GCC host 自测工程（MinGW，`PLATFORM_TESTBENCH`） |

## 2. 自测环境

| 项 | 值 |
|---|---|
| Host 测试编译器 | `C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe`，`-std=c11 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wundef` |
| 固件构建 | `platform/gd32e507/Makefile`，Arm GNU Toolchain 14.2（`-Werror` 严格警告） |
| 上位机 | FRAME C++ 后端，MSVC `/W4`，ctest |

## 3. Host 测试用例（platform/testbench/comm_v1）

运行：`mingw32-make test`。用例覆盖：

- SUM 编码：普通和值、0x00→0x55、0xFF→0xAA、和值回绕；
- RLE：文档示例 `12 34 00 00 00 00 00 AB → 01 12 34 82 00 00 AB` 字节级 golden、长字面量/长重复段 roundtrip；
- DICT：码本命中压缩、`0xC0..0xFF` 非法 token 拒绝、空输入/NULL 长度指针拒绝、码本 CRC32 golden `0xB6DD009D`；
- LZSS：周期数据压缩与重叠回溯 roundtrip、offset 越界拒绝；
- limit_len 提前停止、容量不足、零 limit 拒绝；
- 接收：纯命令帧、RAW 数据帧、RLE 压缩帧解压派发、SUM 错误丢弃、重复 SEQ 丢弃、非本机 DST 丢弃、保留 CODEC 拒绝、256 B 帧（LEN=0x00）、任意分块喂入；
- 发送：纯命令帧 6 B、256 B 帧 263 B（LEN=0x00）、100 个相同字节自动选择压缩、超范围字段（cmd_word=0x40、src=0x10）拒绝发送、CODEC_SELECT 请求强制 RAW 且响应 result/codec/CRC32 正确；
- 纯命令帧跨端 golden：`E9 0E 08 2A 44 6D`（与 FRAME 侧断言一致）。

结果：`PASS all comm_v1 checks`。

## 4. gd32e507 固件构建

```
Memory region         Used Size  Region Size  %age Used
           FLASH:      137552 B       512 KB     26.24%
             RAM:       96720 B       128 KB     73.79%
```

构建通过 `-Werror` 全量严格警告，`comm_v1.c`、3 个 codec、`comm_link.c` 挂接与各服务 SOP 修改均参与编译链接。

## 5. 上下位机互通验证

使用临时工具做了双向跨端验证（验证后已移除）：

1. 设备侧（C）产帧 → FRAME 主机（C++）解析：纯命令、RAW、DICT、LZSS 帧全部解码正确，`PASS device-to-host frames`；
2. 主机侧（C++）产帧 → 设备（C）解析：同上反向通过，`PASS host-to-device frames`；
3. 纯命令帧、RAW 帧、DICT 帧、LZSS 帧两端编码字节完全一致；码本 CRC32 两端同为 `0xB6DD009D`。

## 6. 实机验证（GD32E507 目标板）

验证日期：2026-09-19。固件通过 SEGGER J-Link（SWD，GD32E507ZE）烧录 `platform/gd32e507/build/gd32e507_demo.hex`（Flash 139264 字节，Program & Verify 通过）。上位机为 FRAME CLI（`frame.exe param list`），通信日志由 `FRAME_COMM_LOG_DIR` 开启。

### 6.1 串口链路（COM6，CH340，115200，wire=e9）

参数列表成功（code=0）。抓取的通信帧（前缀，完整帧见 FRAME communication log）：

```text
TX 探测帧（CODEC_SELECT，14 B）: E910040000070100019D00DDB636
TX 请求帧（param list 纯命令，6 B）: E91104224060
RX 协商响应（result=0，codebook CRC32=9D00DDB6，与内置码本一致）
RX 参数条目上报（全部 0xE9 帧，SEQ 循环递增，示例）:
  E90A0882001708070000022000
  E90B0882001B0C070000022000
  E90D0882001708070000022000
  E90808820018090700000220
```

结论：串口链路 E9 请求、直接响应、参数列表主动上报全部使用 0xE9，协商后压缩可用。

### 6.2 以太网链路（TCP 169.254.2.7:5000，wire=e9）

参数列表成功（code=0）。抓取的通信帧：

```text
TX 探测帧: E910040000070100019D00DDB636
TX 请求帧: E91104224060
RX 协商响应: E90808808008000100019D00DDB6B3（result=0，CRC32 匹配）
RX 参数条目（0xE9 帧，含参数名）:
  E90A0882001708070000022000000220000002200046414C5F544553547B  → "FAL_TEST"
  E90B0882001B0C070000022000000220000002200046414C5F44454641554C54539C  → "FAL_DEFAULTS"
```

结论：以太网 TCP 链路（`enet_comm.c` 挂接 `comm_v1_run` 后）E9 全链路正常。

### 6.3 会话协议继承

E9 会话中主动上报（shell 参数条目、wave 流、scope/sfra/perf/trace 上报）继承请求协议：
- 0xE8 会话：上报仍为 0xE8；
- 0xE9 会话：上报为 0xE9，version=0x01，SEQ 从请求序号起 0~7 循环递增（不固定为 0）。

## 7. CODEC=100 ZERO 数零法（2026-09-19 新增）

按 `docs/comm_v1_protocol.md` 第 9 节实现 nibble 数零法变长编码，并接入上下位机择优策略（所有长度候选）。

- 实现：`code/lib/codec_zero.c/.h`；FRAME `protocol.cpp`（zero_encode/zero_decode、codec=zero、择优候选）。
- Host 测试：0~F 码表 golden（80 bit → `48 86 CC 61 DC E3 87 BC F1 E1`）、全零 16 B→8 B、高 nibble roundtrip、padding/未终止拒绝、limit 提前停止。`PASS all comm_v1 checks`。
- FRAME 回归：同 golden 与拒绝路径；ctest 6/6、UI 全套通过。
- 实机（串口 COM6 与 TCP 169.254.2.7:5000，wire=e9）：参数列表成功。条目帧 CODEC 统计：
  - 串口：RX 23 帧中 ZERO 17、RAW 4、DICT 1、RLE 1；
  - TCP：RX 36 帧中 ZERO 22、RAW 10、DICT 2、RLE 2。
  - ZERO 帧示例（前缀）：`E90C0882201978B0AAA22AAA22`（MSG CODEC=4，LEN=0x19）。

## 8. 限制与说明

- 码本协商为初版约定：设备回 `result=0` 表示接受；不支持下发音典，双方内置同一码本（16 条目，CRC32 `0xB6DD009D`）。
- 同一链路同时挂 0xE8 与 0xE9 解析存在文档 1.1 所述交叉识别限制；需要严格隔离时应分别使用链路。
- 实机测试覆盖串口与以太网 TCP 的参数列表全流程；Scope/SFRA/Perf/Trace 的 E9 会话上报按同一继承机制实现，未逐一实测。
