# Bootloader 平台移植方法

## 1. 文档目的

本文档说明如何将当前 Bootloader 与 FAL 接入新的硬件平台。移植时，共享 Core 代码直接加入新平台工程编译，不复制、不修改；平台目录负责提供 Flash 驱动、FAL 配置、Bootloader-FAL 适配、启动跳转、通信链路、运行入口和链接配置。

当前软件依赖关系如下：

```text
平台 main / Section 调度
          │
          ├─ 平台 FAL cfg ──> 平台 Flash Driver
          │        │
          │      FAL Core
          │        │
          └─ Bootloader-FAL Adapter
                   │
             Bootloader Core
                   │
             FRAME 升级协议
```

FAL 由平台持有并初始化。Bootloader 通过 Flash 虚函数使用 FAL，不管理 FAL 配置、设备表和生命周期。

## 2. 原封不动复用的软件层

### 2.1 FAL Core

新平台工程直接加入：

```text
code/interface/fal/fal_core.c
code/interface/fal/fal_core.h
```

FAL Core 负责：

- 遍历平台 Flash 设备表。
- 根据设备内有序分区表累加换算物理地址。
- 推进异步读、写和擦除状态机。
- 根据最大读取长度、program page 和 erase block 拆分操作。
- 检查配置、权限、边界、容量和整数溢出。
- 等待底层 Flash 从 busy 返回 idle。

FAL Core 不包含平台寄存器、平台分区枚举和 Section 注册。

### 2.2 Bootloader Core

新平台工程直接加入：

```text
code/app/bootloader/core/bootloader_core.c
code/app/bootloader/core/bootloader_core.h
```

Bootloader Core 负责：

- 判断正常启动、IAP 升级请求和恢复启动。
- 在无有效 IAP、升级失败和恢复失败时驻留 Bootloader。
- 执行直接升级和暂存升级。
- 将暂存镜像复制到 IAP 区并读回校验。
- 校验分包 CRC、整包 CRC 和固件 footer。
- 管理升级状态和是否允许跳转 IAP。

Bootloader Core 只使用 `bootloader_flash_ops_t` 和 `bootloader_platform_ops_t`，不包含 `fal_core.h`，也不访问具体硬件。

### 2.3 升级元数据

新平台工程直接加入：

```text
code/app/bootloader/core/bootloader_metadata.c
code/app/bootloader/core/bootloader_metadata.h
```

元数据模块负责双份升级记录的编码、校验、新旧记录选择和掉电恢复信息维护。

### 2.4 FRAME 升级协议

继续使用 FRAME 协议和 Section 服务时，新平台工程直接加入：

```text
code/app/bootloader/protocol/bootloader_protocol.c
code/app/bootloader/protocol/bootloader_protocol.h
code/comm/comm.c
```

协议层处理：

| 命令 | 功能 |
|---|---|
| `0x08` | 提交升级信息 |
| `0x09` | Bootloader 就绪 |
| `0x0A` | 提交 1024 字节固件数据块 |
| `0x0B` | 结束升级并提交整包 CRC |

协议层负责请求长度、模块 ID、包顺序、包 CRC 和直接 ACK，不负责物理 Flash 操作。

### 2.5 Section 运行时

裸机 Bootloader 工程直接加入：

```text
code/section/baremetal/section.c
```

平台提供 Section 使用的时间基准或周期中断，并在前台循环中持续调用 `run_task()`。

### 2.6 IAP 升级切换服务

需要从 IAP 响应 FRAME 升级命令时，工程直接加入：

```text
code/app/bootloader/iap/iap_update_service.c
code/app/bootloader/iap/iap_update_service.h
```

该服务负责接收 `0x08`、轮询应用准备回调、保存复位间升级请求并调用 Section 提供的 `SYSTEM_RESET`。服务不访问平台寄存器，也不需要平台提供专用复位或串口驱动接口。

## 3. 新平台目录

新平台 Bootloader 目录包含以下平台文件：

```text
platform/<new_platform>/bootloader/
├─ main.c
├─ fal_cfg.c
├─ fal_cfg.h
├─ bootloader_fal_adapter.c
├─ boot_comm_link.c
├─ boot_uart.c
├─ boot_uart.h
├─ <platform>_boot_platform.c
├─ <platform>_boot_platform.h
├─ Makefile
└─ linker_script.ld
```

Keil 工程使用对应的 scatter 文件和工程配置。平台已有通用 BSP 时，Bootloader 目录直接引用经过裁剪的 BSP 源文件。

## 4. 实现物理 Flash 驱动

### 4.1 FAL 底层操作

每个物理 Flash 设备通过 `fal_flash_ops_t` 挂载：

```c
typedef struct
{
    void *p_context;
    fal_result_t (*p_init)(void *p_context);
    fal_device_state_t (*p_get_state)(void *p_context);
    fal_result_t (*p_read)(void *p_context,
                           uint32_t address,
                           uint32_t length,
                           uint8_t *p_data);
    fal_result_t (*p_program)(void *p_context,
                              uint32_t address,
                              uint32_t length,
                              const uint8_t *p_data);
    fal_result_t (*p_erase)(void *p_context,
                            uint32_t address,
                            uint32_t length);
    fal_result_t (*p_sync)(void *p_context);
} fal_flash_ops_t;
```

实际声明以 `fal_core.h` 为准。底层函数接收物理 Flash 地址，不接收 Bootloader 区域或 FAL 分区 ID。

驱动必须满足：

- `p_init` 初始化设备并验证设备可用性。
- `p_get_state` 返回 idle、busy 或 error。
- `p_read` 支持 FAL 拆分后的读取长度。
- `p_program` 支持不跨 program page 的单次写入。
- `p_erase` 支持按设备基本擦除块擦除。
- `p_sync` 为可选接口。
- 异步擦写启动后立即返回，由 `p_get_state` 报告后续状态。
- Cache、DMA、SPI、QSPI、EFM 和寄存器操作全部保留在平台 BSP 中。

### 4.2 Flash geometry

每个 `fal_device_cfg_t` 填写：

- 唯一设备 ID。
- 设备总容量。
- program page 大小。
- erase block 大小。
- 单次最大读取长度；`0` 表示不限制。
- 该设备的有序分区表。
- 对应 Flash 驱动函数表。

容量、页大小和擦除块大小使用芯片真实参数，不能使用 Bootloader Core 中的固定值。

## 5. 配置 FAL 分区

### 5.1 平台分区枚举

平台在 `fal_cfg.h` 中定义自己的设备和分区枚举：

```c
typedef enum
{
    FAL_DEVICE_NEW_INTERNAL_E = 1u,
    FAL_DEVICE_NEW_EXTERNAL_E
} new_platform_fal_device_id_t;

typedef enum
{
    FAL_ZONE_NEW_BOOT_E = 1u,
    FAL_ZONE_NEW_IAP_E,
    FAL_ZONE_NEW_STAGING_E,
    FAL_ZONE_NEW_META_A_E,
    FAL_ZONE_NEW_META_B_E,
    FAL_ZONE_NEW_LAYOUT_E
} new_platform_fal_zone_id_t;
```

不同物理 Flash 使用不同的 `fal_zone_cfg_t` 分区表。每张表中的区域按照物理地址顺序排列，FAL Core 通过累加前序区域大小得到当前区域的物理起始地址。

### 5.2 必需区域

当前 Bootloader 使用以下逻辑区域：

| 区域 | 用途 | 权限 |
|---|---|---|
| Bootloader | Bootloader 自身镜像 | 只读 |
| IAP | 最终应用镜像 | 读、写、擦 |
| STAGING | 完整暂存镜像 | 读、写、擦 |
| META_A | 第 1 份升级元数据 | 读、写、擦 |
| META_B | 第 2 份升级元数据 | 读、写、擦 |
| LAYOUT | Flash 布局描述 | 只读 |

Bootloader 区域只存在于 FAL cfg，不映射到 `bootloader_flash_zone_t`。在线升级因此无法取得 Bootloader 自身区域的写擦入口。

META_A 和 META_B 分别占用独立擦除块。分区起点和大小均按所属设备的 erase block 对齐。

### 5.3 FAL 实例和调度

平台持有唯一 FAL 实例：

```c
const fal_cfg_t g_new_platform_fal_cfg = {
    .p_devices = devices,
    .device_count = DEVICE_COUNT,
};

fal_t g_new_platform_fal = {0};

static void fal_service_init(void)
{
    (void)fal_init(&g_new_platform_fal, &g_new_platform_fal_cfg);
}

static void fal_service_process(void)
{
    fal_process(&g_new_platform_fal);
}

REG_INIT(0, fal_service_init)
REG_TASK_MS(1u, fal_service_process)
```

FAL 的初始化和周期调度属于平台。Bootloader-FAL Adapter 只检查 FAL 是否已经可用，不调用 `fal_init()`。

## 6. 实现 Bootloader-FAL Adapter

`bootloader_fal_adapter.c` 是 Bootloader Core 与平台 FAL 之间唯一的 Flash 适配层。

### 6.1 映射逻辑区域

适配层使用 `switch case` 将 Bootloader 区域映射到平台 FAL 分区：

```c
static bootloader_result_t fal_zone_get(bootloader_flash_zone_t zone,
                                        fal_zone_id_t *p_fal_zone)
{
    if (p_fal_zone == NULL)
    {
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }

    switch (zone)
    {
    case BOOTLOADER_FLASH_ZONE_IAP_E:
        *p_fal_zone = FAL_ZONE_NEW_IAP_E;
        break;
    case BOOTLOADER_FLASH_ZONE_STAGING_E:
        *p_fal_zone = FAL_ZONE_NEW_STAGING_E;
        break;
    case BOOTLOADER_FLASH_ZONE_META_A_E:
        *p_fal_zone = FAL_ZONE_NEW_META_A_E;
        break;
    case BOOTLOADER_FLASH_ZONE_META_B_E:
        *p_fal_zone = FAL_ZONE_NEW_META_B_E;
        break;
    case BOOTLOADER_FLASH_ZONE_LAYOUT_E:
        *p_fal_zone = FAL_ZONE_NEW_LAYOUT_E;
        break;
    case BOOTLOADER_FLASH_ZONE_COUNT_E:
    default:
        return BOOTLOADER_RESULT_INVALID_ARGUMENT_E;
    }

    return BOOTLOADER_RESULT_SUCCESS_E;
}
```

该映射不包含 Bootloader 自身区域。

### 6.2 挂载 Flash 虚函数

适配层实现并挂载：

```c
static const bootloader_flash_ops_t flash_ops = {
    .p_zone_info_get = zone_info_get,
    .p_read = read,
    .p_write = write,
    .p_erase = erase,
    .p_is_busy = is_busy,
    .p_result_get = result_get,
};
```

这些函数直接使用平台唯一 FAL 实例：

```c
fal_zone_info_get(&g_new_platform_fal, fal_zone, p_info);
fal_read(&g_new_platform_fal, fal_zone, offset, length, p_data);
fal_write(&g_new_platform_fal, fal_zone, offset, length, p_data);
fal_erase(&g_new_platform_fal, fal_zone, offset, length);
fal_is_busy(&g_new_platform_fal);
fal_result_get(&g_new_platform_fal);
```

适配层同时完成 `fal_result_t` 到 `bootloader_result_t` 的转换。它不计算物理地址，不访问寄存器，也不初始化 FAL。

### 6.3 Bootloader 配置和缓冲区

适配层静态持有：

- `bootloader_t` Core 实例。
- `bootloader_protocol_t` 协议实例。
- 1024 字节升级包缓冲区。
- 暂存复制缓冲区。
- `bootloader_config_t` 配置。
- `bootloader_flash_ops_t` 操作表。

复制缓冲区容量至少能够容纳一次复制分块，当前平台通常按照目标 Flash 的 erase block 大小分配。

配置需要指定：

- 目标模块 ID。
- 默认直接升级或暂存升级模式。
- 交给平台检查的镜像首部长度。
- 包缓冲区和复制缓冲区。
- FRAME CRC16 初始化和增量计算函数。

完成配置后依次调用：

```c
bootloader_flash_ops_init(&bootloader, &flash_ops);
bootloader_init(&bootloader, &config, &platform_ops);
bootloader_protocol_init(&protocol,
                         &bootloader,
                         packet_crc,
                         config.default_mode);
bootloader_protocol_mount(&protocol);
```

Adapter 使用 `REG_INIT(1, ...)` 注册，确保优先级 `0` 的 FAL 已经完成初始化。

## 7. 实现平台启动接口

平台通过 `bootloader_platform_ops_t` 提供：

```c
typedef struct
{
    void *p_context;
    bootloader_boot_reason_t (*p_boot_reason_get)(void *p_context);
    bootloader_result_t (*p_boot_reason_clear)(void *p_context);
    bootloader_result_t (*p_image_header_is_valid)(void *p_context,
                                                    const uint8_t *p_header,
                                                    uint32_t header_length,
                                                    uint32_t image_size,
                                                    uint8_t *p_valid);
    bootloader_result_t (*p_jump_to_iap)(void *p_context);
    void (*p_watchdog_kick)(void *p_context);
} bootloader_platform_ops_t;
```

### 7.1 启动原因

平台需要保存并读取：

- 正常上电启动。
- IAP 请求进入 Bootloader。
- 升级恢复启动。

启动记录可以保存在保留 SRAM、备份寄存器或其他复位后仍有效的位置。记录应包含有效性校验，避免随机 RAM 内容被识别为升级请求。

### 7.2 镜像首部检查

Cortex-M 平台检查：

- 初始 MSP 位于合法 SRAM。
- Reset Handler 的 Thumb 位有效。
- Reset Handler 位于 IAP 可执行地址范围。
- 镜像长度不超过 IAP 分区。

Cortex-A 或其他架构按照其启动格式检查入口指令、加载地址和目标内存范围。

### 7.3 跳转 IAP

跳转前按平台需要完成：

- 禁止全局中断。
- 停止 SysTick 和 Bootloader 定时器。
- 停止 UART、DMA、SPI、QSPI 等活动外设。
- 清除中断 pending 状态。
- 更新中断向量地址。
- Cortex-M 设置 MSP。
- 需要时刷新或关闭 Cache。
- 跳转到 IAP 入口。

寄存器、Cache 和指针地址转换只出现在平台启动文件中。

`p_watchdog_kick` 可为空；启用看门狗的平台通过该回调在 Bootloader 驻留和长时间 Flash 操作期间喂狗。

## 8. 实现 FRAME 通信链路

### 8.1 串口接口

平台提供 Bootloader 使用的最小串口能力：

- 串口初始化。
- 接收一个字节。
- 发送数据。
- 判断发送是否完成。
- 使用 DMA 时提供 DMA 完成和发送移位寄存器空闲判断。

### 8.2 Bootloader 通信上下文

`boot_comm_link.c` 分配独立的 Bootloader FRAME 上下文。RX payload 至少为 1033 字节，以容纳完整 `0x0A` 数据命令。

```c
#define NEW_BOOTLOADER_RX_PAYLOAD_SIZE 1033u

DECLARE_COMM_CTX(bootloader_comm,
                 NEW_BOOTLOADER_RX_PAYLOAD_SIZE,
                 HOST_ADDR,
                 NEW_BOOTLOADER_LINK_ID);
```

通过 `REG_LINK` 将平台收发函数与 `comm_run` 挂载。Bootloader 链路只处理 FRAME 到本机目标的升级，不加入转发升级链路。

## 9. 保持 main.c 最小化

`main.c` 只负责建立运行环境：

```c
int main(void)
{
    platform_clock_init();
    platform_tick_init();
    section_init();

    for (;;)
    {
        run_task();
    }
}
```

`main.c` 中不放置：

- FAL 分区映射。
- Flash 虚函数。
- Bootloader Core 和协议实例。
- CRC 实现。
- 升级模式和模块 ID 配置。
- 启动原因处理。
- IAP 有效性检查和跳转逻辑。

## 10. IAP 侧切换服务

IAP 不编译 Bootloader Core、Bootloader 元数据和 `0x09` 至 `0x0B` 协议处理。IAP 编译共用的 `iap_update_service.c/.h`：

1. 响应 `0x08` 升级信息并保存请求内容。
2. 回复 `0x08` ACK，表示升级切换请求已经接收。
3. 在周期任务中调用用户实现的弱符号 `iap_update_prepare()`。
4. 回调返回 `IAP_UPDATE_PREPARE_IN_PROGRESS_E` 时继续驻留 IAP 并在后续周期再次调用。
5. 回调返回 `IAP_UPDATE_PREPARE_FAILED_E` 时取消本次切换，不写启动原因，也不复位。
6. 回调返回 `IAP_UPDATE_PREPARE_READY_E` 后停止重复调用，并等待可通过 `IAP_UPDATE_RESET_DELAY_MS` 配置的软件发送余量时间。
7. 向 `.noinit.boot_request` 写入完整 `0x08` 信息、校验值和最后提交的有效标记。
8. 使用 `section.h` 提供的 `SYSTEM_RESET` 复位进入 Bootloader。

用户准备回调用于封锁 PWM 输出、保存运行参数和完成异步 Flash 写入等操作。回调不能阻塞等待硬件，应通过返回状态由周期任务持续推进。

IAP 构建统一使用 `IS_IAP` 宏启用该服务的通信注册、准备任务和复位流程，并使用同一宏设置向量表。Bootloader 和 ISP 均不编译 `iap_update_service.c`。IAP 与 Bootloader 通过 `bootloader_update_request.h` 共享保留请求的数据格式，平台启动适配读取并校验请求，再通过 `p_upgrade_info_get` 把模块号、版本、文件大小和升级类型交给 Bootloader Core。Core 完成元数据检查后直接建立下载会话，因此 FRAME 进入 Bootloader 后不需要重复发送 `0x08`。IAP 与 Bootloader 的链接配置必须把 `.noinit.boot_request` 放在相同且复位时不初始化的 SRAM 地址。

## 11. 链接与 Flash 布局

新平台需要同时确定：

- Bootloader 链接起始地址和最大长度。
- IAP 链接起始地址和最大长度。
- 中断向量表地址。
- STAGING、META_A、META_B 和 LAYOUT 的物理位置。
- Section 注册段及其起止符号。
- 栈、保留 SRAM、DMA 区和镜像加载区。
- IAP 与 Bootloader 共用的 `.noinit.boot_request` 保留地址。

Bootloader 分区大小按照最终可启动镜像实际大小向 erase block 对齐。链接脚本、FAL 分区和 IAP 向量表地址必须使用同一个 `IAP_BASE`。

构建系统分别生成 Bootloader 和 IAP 离散文件。IAP 产物使用 `_iap` 后缀，并生成带 34 字节 FRAME footer 的升级文件。

## 12. 初始化与运行顺序

初始化顺序为：

```text
main
  ├─ 平台时钟、时间基准和 Section 端口初始化
  └─ section_init
       ├─ REG_INIT(0): fal_init
       └─ REG_INIT(1):
            ├─ 初始化升级串口和必要中断
            ├─ 检查 FAL 状态
            ├─ 挂载 bootloader_flash_ops_t
            ├─ 挂载 bootloader_platform_ops_t
            └─ 初始化并挂载 FRAME 协议
```

周期运行顺序为：

```text
Section tick / foreground loop
  ├─ fal_process
  ├─ bootloader_process
  └─ FRAME link process
```

通信回调只校验和提交升级事件。Flash 读写擦和升级状态机通过周期任务推进。

## 13. 推荐移植顺序

1. 确定 Bootloader、IAP、STAGING 和元数据布局。
2. 完成启动文件、时钟、时间基准和最小 `main.c`。
3. 完成每种物理 Flash 的初始化、读、写、擦和状态驱动。
4. 建立 `fal_cfg.c/h` 并单独验证所有 FAL 分区。
5. 实现启动原因保存、镜像首部检查和 IAP 跳转。
6. 实现 Bootloader-FAL Adapter 和区域映射。
7. 接入 Bootloader 专用 FRAME 串口链路。
8. 接入 IAP 侧独立升级切换服务。
9. 生成 Bootloader、IAP 和带 footer 的升级文件。
10. 使用 FRAME 完成首次升级、二次升级和异常恢复验证。

## 14. 验收清单

### 14.1 Flash 与 FAL

- 每种 Flash 的 JEDEC ID 或设备标识正确。
- 单页写入、跨页写入、擦除和读回正确。
- FAL 分区首尾边界正确。
- Bootloader 区只有读权限。
- Bootloader 逻辑区域无法映射到 Bootloader 自身分区。
- Flash busy 和 error 能够传递到 FAL 状态机。

### 14.2 启动与跳转

- 空 IAP 时永久停留 Bootloader。
- 有效 IAP 且无升级请求时能够启动 IAP。
- 损坏栈指针或入口地址时禁止跳转。
- IAP 请求升级后 Bootloader 保持驻留。
- 跳转前中断、DMA、外设和 Cache 状态处理正确。

### 14.3 升级流程

- FRAME 能够完成首次暂存升级。
- IAP 能够通过 `0x08` 和用户回调请求二次升级。
- 直接升级模式能够写入目标 IAP 区。
- 重复上一数据包能够幂等 ACK。
- 错序、越界、错误长度、错误模块和错误 CRC 被拒绝。
- 下载、元数据提交和复制期间复位后能够恢复或重新升级。
- 升级失败后不跳转损坏的 IAP。
- 所有在线升级路径都不能擦写 Bootloader 区。

### 14.4 构建

- 平台工程使用 C11 和严格警告编译通过。
- Bootloader 和 IAP 链接区域不重叠。
- 栈、保留 SRAM 和 DMA 区域不重叠。
- Bootloader 镜像未超过对齐后的 Bootloader 分区。
- IAP 固件未超过 IAP 分区。
- Section 注册段未被链接器垃圾回收。
- Bootloader 和 `_iap` 离散文件均正确生成。

## 15. 当前平台参考

HC32F334 平台参考：

```text
platform/hc32f334/bootloader/
```

Zynq-7020 平台参考：

```text
platform/zynq7020/ps/bootloader/
```

两者使用相同的 FAL Core、Bootloader Core、升级元数据和 FRAME 协议层，只在 Flash 驱动、FAL cfg、区域映射、通信硬件、启动判断、IAP 跳转和构建链接方面提供平台实现。
