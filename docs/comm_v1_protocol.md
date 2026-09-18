# COMM v1 协议实现方案

## 1. 范围

- 适用链路：UART、RS485、CAN 适配层、TCP。
- 单帧 `DATA` 为 1~256 B；压缩只作用于 `DATA`。
- 控制、参数提交、启停等关键命令默认使用 RAW；压缩主要用于 Scope、Trace、日志和批量数据。
- 本文为实现约定。新 `0xE9` 协议与现有 `0xE8` 协议共存，不替换旧协议；以下新增接口和字段仍需在代码中实现。

### 1.1 与现有 comm 的集成

- 在 `comm.c/.h` 中增加独立的 `comm_v1_run()` 和 `comm_v1_ctx_t`；旧协议保留 `comm_run()` 和 `comm_ctx_t`。两套协议统一使用 `comm_send_data(void *p_pack, DEC_MY_PRINTF)`，在函数内部根据输入对象的第一个字节 SOP 选择编码方式。
- 两套 ctx 可以完全不同，各自管理解析状态、接收缓冲区与校验过程，通过 `comm_link` 的 handler 数组分别挂接。每条链路按需挂接一种或两种协议。
- 两套协议共用现有 `REG_COMM(cmd_set, cmd_word, handler)`、命令注册表和回调签名。v1 完成校验、解压后，填写 `section_packform_t`，再按命令集和命令字派发。
- 同一个注册项在两套协议中具有相同业务含义和原始 DATA 布局。v1 仅支持其字段位宽及长度范围内的命令、地址和数据，发送时必须检查范围，不能截断旧协议的超范围值。
- `comm_addr.h` 属于 demo 地址配置，放在 `code/data_source/demo/`，不与通用 `comm.c/.h` 放在一起。

```c
static const section_link_handler_item_t handlers[] = {
    {.func = comm_run,    .ctx = &comm_ctx},
    {.func = comm_v1_run, .ctx = &comm_v1_ctx},
};
```

上述为挂接示意。若同一字节流同时交给两个 run，某种协议的 DATA 可能包含另一种协议的有效帧；独立 ctx 不能消除交叉识别。需要严格隔离时，应分别使用链路或增加帧接收协调，不能仅靠帧头不同保证隔离。

### 1.2 业务数据包约定

`section_packform_t` 是两套协议共用的已解码业务包，不是线上裸包的内存映射。v1 接收时填写如下：

| 字段 | v1 含义 |
|---|---|
| `sop` | `0xE9` |
| `version` | `0x01`，本地协议标识，不占线上字节 |
| `src/d_src/dst/d_dst` | 从 MSG 提取的地址 |
| `cmd_set/cmd_word/is_ack` | 从 MSG 提取的命令与 ACK |
| `seq` | 在结构体尾部新增的字段，从 MSG 提取；旧协议置零 |
| `p_data` | RAW 数据或解压后的原始业务数据；纯命令可为 NULL |
| `len` | 原始业务数据长度，数据帧为 1~256 B，纯命令为 0 |
| `crc/eop` | v1 不使用，置零；SUM 保留在 v1 私有解析上下文中 |

CODEC、线上 LEN 和压缩 Token 由协议层处理，业务函数不承担解压。回调结束后不能继续持有接收缓冲区中的帧或 DATA 指针；延迟应答需要保存必要的消息信息和数据。

业务发送时同样填写 `section_packform_t`，两套协议都调用同一个发送入口：

```c
void comm_send_data(void *p_pack, DEC_MY_PRINTF);
```

`sop` 必须保持为 `section_packform_t` 的首字段，位于对象偏移 0。发送函数先检查指针非空，再读取输入对象的第一个字节：

- `0xE8`：按现有协议编码发送。
- `0xE9`：按 v1 协议压缩 DATA、填写 MSG 并编码发送。
- 其他值：拒绝发送，不能把任意非 `0xE8` 值都解释成 v1。

`void *` 是统一入口的参数类型，不表示可以传入任意缓冲区或已编码裸包；当前两套协议均要求它指向有效的 `section_packform_t` 对象。协议识别后按该结构读取地址、命令和原始 `p_data/len`。

旧协议包显式设置 `sop=0xE8`；v1 包设置 `sop=0xE9`、`version=0x01`。直接应答继承请求的协议标识、命令和 SEQ，业务函数无需选择不同发送接口，也不依赖全局“最后收到的协议”。迁移时应补齐现有零初始化回包和主动上报中的 SOP 赋值，不能继续依赖旧发送实现自动写入 `0xE8`。

## 2. 帧格式

### 2.1 数据帧

```text
| SOP | MSG_L0 | MSG_L1 | MSG_L2 | MSG_L3 | LEN | DATA... | SUM |
```

```text
SOP = 0xE9
总长度 = 7 + DATA_LEN
DATA_LEN = 1~256 B
```

### 2.2 纯命令帧

```text
| SOP | MSG_L0 | MSG_L1 | MSG_L2 | MSG_L3 | SUM |
```

纯命令帧固定为 6 B，并满足：`CMD=1`、`CODEC=000`，不包含 `LEN` 和 `DATA`。

### 2.3 LEN

```text
0x01~0xFF：DATA 长度为 1~255 B
0x00       ：DATA 长度为 256 B
```

## 3. MSG 定义

`MSG` 为 32 bit，在线上按小端顺序发送：

```text
MSG_L0 = MSG[7:0]
MSG_L1 = MSG[15:8]
MSG_L2 = MSG[23:16]
MSG_L3 = MSG[31:24]
```

```text
bit2:0    SEQ       序号，0~7
bit6:3    DST       静态目的地址，0~15
bit9:7    D_DST     动态目的地址，0~7
bit13:10  SRC       静态源地址，0~15
bit16:14  D_SRC     动态源地址，0~7
bit20:17  CMD_SET   指令集，0~15
bit26:21  CMD_WORD  子命令，0~63
bit29:27  CODEC     数据编码类型
bit30      CMD       1=纯命令帧；0=数据帧
bit31      ACK       1=直接响应；0=请求或主动上报
```

SEQ 排在 DST 前面。`MSG_L0` 已包含完整 SEQ 和静态目的地址 DST；动态目的地址 D_DST 跨越前两个字节：

```text
bit2:0  SEQ
bit6:3  DST
bit7    D_DST[0]
```

接收端收到 `SOP` 和首个 MSG 字节后立即判断，以下为逻辑示意：

```c
seq = msg_l0 & 0x07u;
dst = (msg_l0 >> 3u) & 0x0Fu;

is_duplicate = last_seq_valid && (seq == last_seq);
is_local_static = (dst == 0u) || (dst == local_static_addr);
accept_header = !is_duplicate && (is_local_static || is_route_target);
```

`is_route_target` 由接收链路 ID 和现有路由表中的目的地址判断。重复 SEQ，或 DST 既不匹配本机/广播也不匹配路由目的地址时，立即返回空闲，等待下一个 `0xE9`。

地址值 `0` 表示广播或任意匹配。继续接收后才检查完整 D_DST；本地投递要求静态、动态目的地址均匹配。需要路由的帧按路由规则处理，不能因为它不是发给本机就提前丢弃。

## 4. SUM 校验

`SUM` 覆盖 `SOP` 至 `DATA` 的最后一个字节，不包含 `SUM` 自身。

```c
uint8_t comm_sum_encode(const uint8_t *p_data, uint16_t length)
{
    uint8_t sum = 0u;

    for (uint16_t i = 0u; i < length; ++i)
    {
        sum = (uint8_t)(sum + p_data[i]);
    }

    if (sum == 0x00u) { return 0x55u; }
    if (sum == 0xFFu) { return 0xAAu; }
    return sum;
}
```

接收端必须对收到的 `SOP...DATA` 重新计算并与线上 `SUM` 比较；不相等则丢弃整帧。

## 5. CODEC 定义

| CODEC | 名称 | 定义 |
|---:|---|---|
| `000` | RAW | DATA 为原始字节流 |
| `001` | DICT | 静态字典压缩 |
| `010` | RLE | PackBits / 重复字节压缩 |
| `011` | LZSS | 块内 LZSS-256 压缩 |
| `100` | ZERO | 数零法变长编码（nibble 分段一元编码） |
| `101~111` | Reserved | 当前版本必须拒绝 |

规则：

- `CMD=1` 时 `CODEC` 必须为 `000`。
- `CODEC!=000` 时，先校验 SUM，再解压 DATA。
- 解压输出为 0 B、超过 256 B、压缩输入未恰好消费完或遇到非法 Token 时，丢弃整帧。

### 5.1 算法目录与职责

DICT、RLE、LZSS、ZERO 的压缩和解压实现统一放在 `code/lib/`，各自使用配对的 `.c/.h` 文件，例如 `codec_dict.c/.h`、`codec_rle.c/.h`、`codec_lzss.c/.h`、`codec_zero.c/.h`。算法只处理输入、输出字节流，不依赖 `comm.h`、`section_packform_t`、链路或硬件接口，不负责 SOP、MSG、SUM 和协议应答。

comm 层负责码本协商状态、候选算法选择、压缩收益比较、CODEC 填写以及收发组包；算法层负责按对应 Token 规则编码、解码和检查缓冲区边界。RAW 由 comm 层直接使用原始数据，不必新增压缩算法。

### 5.2 统一压缩与解压接口

所有算法的编码和解码函数统一返回 `int8_t`：成功返回 1，失败或达到限制长度时返回 0。参数顺序固定为：输入长度、输入 byte 指针、输出长度指针、输出 byte 指针、限制长度 `limit_len`。第三个参数 `p_output_len` 为输入/输出参数：调用前传入输出缓冲区容量，调用后写回实际输出长度，失败置为 0。

```c
int8_t codec_dict_encode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);
int8_t codec_dict_decode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);

int8_t codec_rle_encode(uint16_t input_len, const uint8_t *p_input,
                        uint16_t *p_output_len, uint8_t *p_output,
                        uint16_t limit_len);
int8_t codec_rle_decode(uint16_t input_len, const uint8_t *p_input,
                        uint16_t *p_output_len, uint8_t *p_output,
                        uint16_t limit_len);

int8_t codec_lzss_encode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);
int8_t codec_lzss_decode(uint16_t input_len, const uint8_t *p_input,
                         uint16_t *p_output_len, uint8_t *p_output,
                         uint16_t limit_len);
```

统一约定：

- 长度使用 `uint16_t`，能够直接表示 256；算法接口中的 0 不表示 256，线上 LEN 的特殊编码由 comm 层处理。
- 算法先保存 `*p_output_len` 中的输出容量，再将其置为 0；只有完整成功后才写回实际输出长度并 `return 1`。成功长度必须大于 0、不超过原容量且严格小于 `limit_len`。调用方每次调用前均重新填写容量。
- `limit_len` 是择优时的排他上限，不是缓冲区容量。压缩过程中已生成的编码字节数达到或超过它时，立即停止并 `return 0`，不再继续扫描、匹配或压缩后续输入；Token 和字面量均计入编码长度。写入下一段前若已确定其使长度达到上限，可直接提前返回；判断使用已确定且不会因后续处理缩短的输出长度，不能按未经确认的估算误停。
- 输入非法、容量不足、非法 Token、解压越界、输入未完整消费或触及限制长度时，保持 `*p_output_len=0` 并 `return 0`；缓冲区可能含部分输出，调用方必须忽略。
- `p_output_len` 为 NULL 时直接 `return 0`；其余必要指针为空、输入长度为 0、输出容量为 0、`limit_len=0` 或不符合对应算法限制时，在输出长度指针有效的前提下置零并 `return 0`。纯命令不调用压缩或解压函数。
- 每次读写均检查输入剩余长度和输出容量。输入、输出缓冲区由调用方提供且不得重叠，不分配动态内存；LZSS 解码内部的重叠回溯复制仍按第 8 节规则执行。
- comm 层只在编码返回 1 时使用写回长度比较并更新最优候选；选中后以该长度填写线上 LEN。返回 0 时放弃该候选，保留已有最优结果，最终仍可选择 RAW。
- 解码接口保持同样的参数和返回约定，以解码输出长度检查上限；返回 0 时丢弃整帧，返回 1 时用写回长度填写 `section_packform_t.len`。v1 解码传入 `limit_len=257`，允许完整输出 256 B；同时将输出容量限制为不超过 256 B，并确保实际缓冲区容量足够。不能传 256 作为限制长度，否则合法的 256 B 输出会被拒绝。

### 5.3 算法初始化

算法若有额外的一次性初始化，例如建立字典查找表，在各自 `.c` 中包含 `section.h`，定义私有初始化函数，并使用 `REG_INIT` 注册。对外 `.h` 不暴露初始化调用要求，comm 层不逐个手动调用算法初始化。

```c
/* Inside codec_dict.c, after the private codec_dict_init definition. */
#include "section.h"

REG_INIT(0, codec_dict_init)
```

`codec_dict_init` 应定义为 `static void codec_dict_init(void)`；初始化必须在通信任务使用算法前完成，若存在初始化依赖则按工程实际顺序安排注册级别。无需一次性初始化的算法不增加空初始化函数。

初始化只准备模块的固定资源，不建立跨帧压缩历史。每帧编码、解码的临时状态在调用内独立管理；码本协商仍属于通信会话，不通过切换模块全局可变状态使不同链路互相影响。

## 6. CODEC=001：DICT

双方预置相同字典；每个字典最多有 64 个条目。每个条目长度为 1~256 B。

```c
typedef struct
{
    uint16_t offset;
    uint8_t length;
} comm_dict_entry_t;
```

压缩 DATA Token：

```text
0x00~0x7F：字面量段，length = token + 1，后跟 length 个原始字节
0x80~0xBF：字典引用，index = token & 0x3F
0xC0~0xFF：非法
```

编码器扫描原始数据时优先选择最长字典匹配；匹配长度相同则选择索引更小的条目。未命中的字节收集为字面量段。

### 6.1 码本协商

保留 `CMD_SET=0x0`、`CMD_WORD=0x00` 作为 `CODEC_SELECT`。

请求 DATA：

```text
| codec(1 B) | codebook_id(1 B) | version(1 B) | codebook_crc32(4 B, little-endian) |
```

响应 DATA：

```text
| result(1 B) | codec(1 B) | codebook_id(1 B) | version(1 B) | codebook_crc32(4 B) |
```

`result=0` 表示成功。初版不支持在线下发字典；不一致、设备重启或上位机重连后使用 RAW，直至重新协商成功。

## 7. CODEC=010：RLE / PackBits

压缩 DATA Token：

```text
0x00~0x7F：字面量段，length = token + 1，后跟 length 个原始字节
0x80~0xFF：重复段，length = (token & 0x7F) + 3，后跟 1 B repeat_value
```

连续相同字节长度大于等于 3 时使用重复段；其余字节合并进字面量段。

例子：

```text
原始：12 34 00 00 00 00 00 AB
压缩：01 12 34 82 00 00 AB
```

其中 `01` 表示后跟 2 B 字面量，`82 00` 表示 5 个 `00`，`00 AB` 表示后跟 1 B 字面量。

## 8. CODEC=011：LZSS-256

只使用当前 DATA 块已解压字节作为历史窗口，不跨帧继承状态。

压缩 DATA Token：

```text
0x00~0x7F：字面量段，length = token + 1，后跟 length 个原始字节
0x80~0xFF：回溯引用，length = (token & 0x7F) + 3，后跟 1 B offset_minus_1
```

```text
offset = offset_minus_1 + 1
```

解码时从已经输出的数据末尾向前 `offset` 字节处开始复制 `length` 字节，允许重叠复制。

有效条件：

```text
offset >= 1
offset <= 当前输出长度
输出总长度 <= 256
```

## 9. CODEC=100：ZERO 数零法

将输入字节流按 nibble（半字节）逐段编码为变长 bit 流，适合小值密集的数据。

### 9.1 nibble 编码

每个 nibble n（0~F）编码为：

```text
1^k + 0^(m+1) + 1
```

其中 `k = n >> 2`（所在段 0~3/4~7/8~B/C~F 的溢出次数）、`m = n & 3`（段内偏移）。

- 段前缀 `1^k`：k 个 1，表示"溢出选到下一段"；k=0 时无前缀。
- 段内偏移 `0^(m+1) 1`：m+1 个 0 后跟一个 1 收尾（0 后紧跟的 1 分隔）。

码表：

```text
0:01      4:101       8:1101      C:11101
1:001     5:1001      9:11001     D:111001
2:0001    6:10001     A:110001    E:1110001
3:00001   7:100001    B:1100001   F:11100001
```

0~F 依次编码恰好为（80 bit）：

```text
01001000100001101100110001100001110111001110001110000111101111001111000111100001
```

### 9.2 bit 序与字节填充

- 编码 bit 流按书写顺序填入输出字节：流的第 1 个 bit 放在输出首字节的最高位（MSB-first）。
- 最后一个字节剩余的空 bit 全部补 1（用 1 补空）。

### 9.3 解码

- 解码器按 bit 顺序读 nibble：先数段前缀的 1（到 0 为止），再数 0 串（到 1 为止），nibble = 4k + m。
- 每两个 nibble 按先高后低拼成一个字节。
- 段前缀的 1 一直读到输入结束仍未遇到 0：该未完成 nibble 视为字节填充的补空 bit，丢弃并停止解码。
- 段内 0 串读到输入结束仍未遇到收尾 1：数据损坏，丢弃整帧。
- 完整 nibble 总数必须为偶数（否则最后半个字节，丢弃整帧）；解压输出必须为 1~256 B。
- 编码输出必须严格短于 `limit_len`（达到即失败），其余参数与返回约定同 5.2 节。

## 10. 上下位机压缩选择

RAW 永远作为候选。只有候选压缩结果明显更短时才启用压缩：

```text
raw_length <= 16：至少节省 2 B
raw_length > 16 ：至少节省 max(2 B, ceil(raw_length / 16))
```

推荐策略：

| 原始 DATA 长度 | 尝试算法 |
|---:|---|
| 1~16 B | DICT、ZERO；失败则 RAW |
| 17~63 B | DICT、RLE、ZERO；选择最短 |
| 64~256 B | DICT、RLE、LZSS、ZERO；选择最短 |

令 `best_length` 为当前最优 DATA 长度，初始为 `raw_length`；`min_saved_bytes` 为上面相对 RAW 的最低节省量。若 `raw_length <= min_saved_bytes`，不存在满足要求的非空压缩结果，直接使用 RAW，避免无符号减法下溢。

每次尝试候选前计算：

```text
limit_len = min(best_length, raw_length - min_saved_bytes + 1)
```

候选长度必须严格小于 `limit_len`：既比当前最优结果更短，又至少达到相对 RAW 的节省要求。加 1 用于允许“恰好节省 min_saved_bytes”的结果。最低节省量始终相对 RAW 计算，不要求每个后续算法再次比已有最优结果节省同样的字节数。

编码器在压缩过程中达到限制即返回 0，不必完成后续压缩。返回 1 时保存候选 DATA 并更新 `best_length`，再据此计算下一个候选的限制；尝试新算法不能覆盖已保存的最优数据。相同长度不替换已有结果。

择优过程额外使用一个 256 B 临时缓冲区，例如 `uint8_t candidate_data[256]`，由 comm 发送组包过程持有，依次复用于 DICT、RLE、LZSS 候选，不为每个算法单独分配缓冲区：

1. 初始最优编码为 RAW，`best_length=raw_length`，将原始 DATA 复制到发送缓冲区的 DATA 区。
2. 每次候选调用前，将候选输出长度变量重新设为 256，作为实际输出容量；另行计算并传入 `limit_len`。
3. 算法只向临时缓冲区写入。返回 0 时忽略其中的部分数据，发送缓冲区中的最优 DATA 保持不变。
4. 返回 1 且满足择优条件时，将临时缓冲区中的有效数据复制到发送缓冲区的 DATA 区，同时更新 `best_length` 和最优 CODEC。
5. 所有候选完成后，用最优 CODEC 和 `best_length` 填写 MSG、LEN，再计算 SUM。临时缓冲区可以释放或复用。

原始输入始终保留，所有候选都从同一份原始 DATA 开始压缩。临时缓冲区独立于输入和发送缓冲区；每个并发发送过程必须独占自己的临时空间，不能无保护地共用一个可写静态数组。纯命令帧不需要候选压缩。最大 v1 数据帧需要 263 B 的发送空间，额外的 256 B 临时缓冲区不替代该发送空间。

例如 `raw_length=100`、最低节省 7 B 时，首次限制为 94，允许 93 B 的候选；得到 80 B 的最优结果后，下一算法的限制为 80，输出达到 80 B 即停止，79 B 仍可成为更优结果。

### 9.1 统一发送入口与 v1 组包流程

```text
comm_send_data(void *p_pack, DEC_MY_PRINTF)
  → 检查指针非空，读取输入对象第一个字节 SOP
  → SOP=0xE8：进入现有协议编码发送流程
  → SOP 既非 0xE8 也非 0xE9：拒绝发送
  → SOP=0xE9：按 section_packform_t 读取原始业务数据
  → 检查 version=0x01、地址/命令/SEQ 范围和数据长度
  → len=0：CMD=1、CODEC=RAW，不写 LEN 和 DATA
  → len=1~256：CMD=0，根据数据和压缩策略选择 RAW 或压缩编码
  → 确定最终 DATA、编码后长度和 CODEC
  → 将 SEQ、地址、命令、CODEC、CMD、ACK 填入 MSG
  → 从 p_pack->sop 写入 SOP，按小端写入 MSG
  → 数据帧写入编码后的 LEN、DATA；256 B 的 LEN 编码为 0x00
  → 对最终裸包计算并追加 SUM
```

`comm_send_data()` 的 v1 分支使用与调用者原始 DATA 分离的发送缓冲区，不覆盖原始 DATA，也不把业务包的 `len` 改为压缩长度。`version=0x01` 只用于本地标识，不额外写入线上帧。DATA 非空时必须有有效指针；v1 长度超过 256 B 或字段超范围时拒绝发送。这些 v1 范围限制不改变旧协议的原有范围。

## 10. 请求、响应与上一包去重

请求帧：

```text
ACK=0，SEQ 为当前请求序号
```

响应帧：

```text
ACK=1
SEQ、CMD_SET、CMD_WORD 与请求保持一致
单播响应的 SRC/D_SRC 与请求 DST/D_DST 对调，DST/D_DST 取请求源地址
```

广播是否应答由业务约定；需要应答时使用本机实际源地址，不把广播地址当成本机源地址。

每个 v1 接收 ctx 只保存上一有效包的序号及其有效标志：

```text
last_seq
last_seq_valid
```

- 初始 `last_seq_valid=0`，第一包不因 SEQ 被拒绝。
- 收到 MSG_L0 后，若 `last_seq_valid` 有效且 SEQ 等于 `last_seq`，直接返回空闲，不执行业务、不转发、不缓存或重发上次响应。
- SEQ 不同只是继续接收的条件。整包通过校验和相应有效性检查，确认可本地派发或路由后，才更新 `last_seq` 并置有效；残帧、校验失败、非法编码或无效本地命令不更新它。
- 普通解析状态复位保留上一有效 SEQ；上下文初始化或明确的新会话复位清除有效标志。
- 新请求和主动上报的发送序号按 0~7 循环递增，主动上报不能固定为 0；直接响应仍回显请求 SEQ。
- 去重只比较同一 ctx 的上一有效包 SEQ，不按源地址、命令或 ACK 分组，不建立历史窗口。首字节尚无完整源地址，也没有 ACK，因此多个发送方或交错请求/响应若出现相同 SEQ，也会被丢弃；链路使用方必须接受或协调这一限制。
- 原请求已接收但响应丢失时，相同 SEQ 的重发会被丢弃，不保证可靠重试。换新 SEQ 会作为新包处理，非幂等操作可能再次执行。

## 11. 接收状态机

```text
等待 SOP(0xE9)
  → 接收 MSG_L0，提取 SEQ 和 DST
      → SEQ 等于上一有效包：返回空闲，等待 E9
      → DST 既不匹配本机/广播也不匹配路由：返回空闲，等待 E9
  → 接收 MSG_L1~MSG_L3，提取剩余字段，检查完整地址及编码合法性
  → CMD=1：接收 SUM；CMD=0：接收 LEN、DATA、SUM
  → SUM 校验，不通过则返回空闲
  → 本地包：按 CODEC 解压或使用 RAW，填写 section_packform_t
      → 确认本地命令有效，记录 SEQ，经原 REG_COMM 表派发业务
  → 路由包：确认可转发，记录 SEQ，按路由表转发
  → 返回空闲，等待 E9
```

SOP、地址、长度、SUM、CODEC、解压、命令任一项非法时，丢弃当前帧并重新搜索 `0xE9`。

路由转发必须在完整帧校验之后进行，并保留原始源/目的地址、命令、ACK、SEQ 和编码数据。透明路由无需依赖本机命令注册项或解压字典；不得把路由包重新当作本机新消息分配 SEQ。跨 `0xE8/0xE9` 协议转换不属于本方案。

首字节拒绝后立即进入搜索态，不继续按长度跳过余下帧。因此余下数据中的 `0xE9` 可能成为新候选帧头，仍须完整校验后才能执行或转发。接收半帧应有超时复位，超时不更新上一有效 SEQ。

## 12. 本地联合体

联合体仅用于本地访问字段；线上格式应始终由显式移位和字节序列化保证。

```c
typedef union
{
    uint32_t raw;
    uint8_t bytes[4];

    struct
    {
        uint32_t seq       : 3;
        uint32_t dst       : 4;
        uint32_t d_dst     : 3;
        uint32_t src       : 4;
        uint32_t d_src     : 3;
        uint32_t cmd_set   : 4;
        uint32_t cmd_word  : 6;
        uint32_t codec     : 3;
        uint32_t cmd       : 1;
        uint32_t ack       : 1;
    } bits_lsb;
} comm_msg_t;
```
