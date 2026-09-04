# GD32E507 双 Flash 与数据池 Demo

本 Demo 按 BSP、Interface、fal_cfg、fal_core、使用层组织，通用职责见 [FAL 分层工程方案](../../design/storage/fal_architecture.md)。本文说明当前双 Flash 配置、运行入口和操作方式。

## 文件与职责

| 文件 | 职责 |
| --- | --- |
| `code/business/demo/fal_cfg.c/.h` | Flash 操作适配与注册、设备与分区表、两个独立 FAL 实例及配置绑定 |
| `code/lib/fal/fal_core.c/.h` | 通用分区访问、请求状态机、实例初始化与周期推进 |
| `code/data_source/demo/demo_storage_service.c` | 使用已注册 FAL 实例进行启动扫描、参数双副本、测试与日志事务 |
| `code/data_source/demo/demo_storage_record.c` | 显式小端记录、CRC32、独立提交页、序号比较 |
| `code/data_source/demo/demo_storage_section.c` | 注册初始化和 1 ms 任务，依次调用 Core 与存储服务 |
| `code/data_pool/demo/demo_storage_pool.c` | RAM 参数、命令邮箱、设备状态与完成结果 |
| `code/business/demo/demo_fal.c` | Shell 操作、候选参数发布、10 ms 参数计算演示 |
| `code/interface/demo/common/flash_port.c` | 提供初始化、状态、读、编程、擦除、几何与诊断函数 |
| `platform/gd32e507/bsp/src/bsp_flash.c` | 板级设备分发与几何查询 |
| `platform/gd32e507/bsp/src/bsp_nor_flash.c` | GD25Q16 单线 SQPI 驱动 |
| `platform/gd32e507/bsp/src/bsp_nand_flash.c` | GD9FU1G8F2A EXMC 驱动、BCH4、固定地址坏块拒绝 |
| `code/lib/bch4.c`、`code/lib/flash_integrity.c` | BCH4、CRC32、小端字段读写 |

业务通过 `demo_storage_business.h` 访问数据池；存储任务通过 `demo_storage_exchange.h` 领取冻结请求、发布结果和恢复参数。FAL 经 Interface 调用 BSP。硬件轮询、Flash 擦写、编码和校验在任务上下文执行。

## 分区配置

分区配置入口是 `code/business/demo/fal_cfg.c` 中的 `nor_zones[]`、`nand_zones[]`，大小常量在配套 `fal_cfg.h` 中定义。设备表 `devices[]` 使用 `.ops` 注册 Interface 函数的 FAL 适配回调。每颗设备从地址 0 开始，依次分配保护区、参数 A、参数 B、测试区、日志区；起点由前序分区大小累加计算。可写区合计占设备最后 8 个擦除块。

| 分区 | NOR ID | NOR 起点 / 大小 | NAND ID | NAND 起点 / 大小 |
| --- | ---: | --- | ---: | --- |
| protected | 1 | `0x00000000` / 2016 KiB | 101 | `0x00000000` / 127 MiB |
| param_a | 2 | `0x001F8000` / 4 KiB | 102 | `0x07F00000` / 128 KiB |
| param_b | 3 | `0x001F9000` / 4 KiB | 103 | `0x07F20000` / 128 KiB |
| test | 4 | `0x001FA000` / 8 KiB | 104 | `0x07F40000` / 256 KiB |
| log | 5 | `0x001FC000` / 16 KiB | 105 | `0x07F80000` / 512 KiB |

NOR 容量为 2 MiB、页 256 字节、擦除扇区 4096 字节；NAND 主区容量为 128 MiB、页 2048 字节、擦除块 128 KiB。NAND 的 OOB 由 BSP 管理，FAL 地址仅对应主区。

项目分区大小、顺序和权限统一在 demo `fal_cfg` 中配置。BSP 校验物理容量、编程页、擦除块、忙状态及 NAND 坏块/ECC/编程顺序。NAND 编程状态按完整 1024 个物理块维护。

保护区通过分区表的 `FAL_ZONE_PERMISSION_READ` 由 FAL Core 检查，在调用底层前拒绝擦写。启动只读取尾部记录，不自动擦除或保存。执行保存、日志追加及测试，会改写对应尾部分区中已有内容。

## 独立使用 FAL

调用关系为 `demo → fal_core → fal_cfg 中注册的适配回调 → flash_port 函数 → BSP`。BSP 和 Interface 使用各自的返回值与状态类型；`fal_flash_ops_t`、FAL 类型转换、设备及分区注册集中在 demo 的 `fal_cfg.c`。

`g_demo_fal_runtime` 描述实例数组 `g_demo_fal` 与配置数组 `g_demo_fal_cfg` 的绑定。`fal_runtime_init(&g_demo_fal_runtime)` 挂载两个实例，配置中的初始化回调核对设备几何与 Interface 查询结果；`fal_runtime_process(&g_demo_fal_runtime)` 周期推进各实例。重新挂载选中设备使用 `fal_runtime_mount(&g_demo_fal_runtime, device)`，活动请求存在时返回忙并保留请求。

初始化返回首个失败结果，其他独立实例仍尝试初始化；各实例的状态与结果分别查询。周期推进返回成功表示本轮遍历完成，异步操作结果以 `fal_result_get()` 为准。

以下读取示例用于独立 FAL 使用场景。`g_demo_fal[DEMO_FAL_NOR]`、`g_demo_fal[DEMO_FAL_NAND]` 可直接传给 FAL Core API：

```c
#include "fal_cfg.h"

static uint8_t read_buffer[256]; /* Retained until the asynchronous read completes. */

/* Call after fal_runtime_init() has successfully mounted the NOR instance.
 * The caller checks the returned fal_result_t before waiting for completion. */
static fal_result_t read_parameter(void)
{
    return fal_read(&g_demo_fal[DEMO_FAL_NOR], DEMO_FAL_NOR_PARAM_A,
                    0u, sizeof(read_buffer), read_buffer);
}
/* Keep calling fal_runtime_process(&g_demo_fal_runtime) in the owning task
 * until fal_is_busy(&g_demo_fal[DEMO_FAL_NOR]) is 0,
 * then inspect fal_result_get() before consuming read_buffer. */
```

独立 FAL 应用接入配置、`code/lib/fal/` 中的 Core、Interface 和 BSP，并提供初始化与周期调用入口。BSP 与 Interface 均不包含 FAL 头文件或操作表；类型转换和操作挂载由 `fal_cfg` 完成。

当前参数 Demo 的 `demo_storage_section.c` 用 `REG_INIT(0, storage_init)` 调用 Core 初始化，再用 `REG_INIT(1, demo_storage_service_init)` 初始化参数服务。`REG_TASK_MS(1, storage_process)` 在同一个任务中依次调用 `fal_runtime_process()` 和 `demo_storage_service_process()`；请求提交和推进保持串行。

参数服务运行期间，由该服务统一拥有实例事务，业务通过数据池提交保存、加载等命令，不同时执行上述独立读取示例。注册周期为 1 ms，实际 NAND ECC 运算耗时需板上测量。

## 参数与日志事务

每个参数副本使用一个擦除块中的前两页：第 1 页保存数据记录，第 2 页保存提交记录。数据记录包含版本标识、类型、序号、有效长度、三个参数和 CRC32；提交页包含类型、同一序号、数据 CRC32 和提交 CRC32。所有多字节字段显式小端编码。

保存先扫描两个副本，擦除非活动副本，写数据页并全页读回比较，再写提交页并全页读回比较。扫描只接受数据与提交记录都完整的副本，按支持回绕的序号选择最新记录。读不到某个槽位时，保存和日志追加返回 `-23`，保留未知内容；加载仍可返回另一个有效副本，并在设备诊断中保留读取错误。

启动扫描两颗设备，自动加载 NOR 的最新有效参数。NOR 无有效参数时使用 RAM 默认值；NAND 参数通过显式 `FAL_LOAD` 加载。默认参数为输入 1000 mV、增益 1000（1 倍）、阈值 1500 mV。

日志区由 4 个擦除块构成循环队列，每次追加占用一个块的前两页，最多保留 4 条已提交参数快照。`FAL_SCAN` 刷新最近日志，`FAL_INFO` 显示其序号和三个参数。日志追加仅由命令触发。

测试擦除 test 区的两个块，在第 1 块最后一页写入测试数据、在第 2 块第一页写入提交页，再分别读回比较。测试不改写参数区或日志区。

数据池忙时拒绝新请求和参数覆盖。`queued=1` 表示已入队；以 `FAL_INFO` 的 `busy=0`、`completed=request` 和 `result=0` 判断成功。

## Shell 操作

通过工程现有 Shell 通道访问。字符串通道的赋值格式为 `名称:数值`；命令单独一行。每项存储命令完成后再发起下一项。

```text
FAL_INFO
FAL_DEVICE:0
FAL_EDIT_INPUT_MV:12000
FAL_EDIT_GAIN_MILLI:1500
FAL_EDIT_THRESHOLD_MV:15000
FAL_APPLY
FAL_SAVE
FAL_INFO
```

`FAL_DEVICE:0` 选择 NOR，`FAL_DEVICE:1` 选择 NAND。`FAL_APPLY` 将三个候选值一起写入 RAM 数据池；`FAL_SAVE` 保存当前 RAM 参数。成功应用以上参数后，10 ms 演示任务计算得到 `output_mv=18000`、`threshold_hit=1`，由 `FAL_INFO` 显示。该计算仅为软件演示。

| 命令 | 行为 |
| --- | --- |
| `FAL_INFO` | 显示 RAM 参数、演示输出、事务结果、分区和设备诊断、最近日志 |
| `FAL_SCAN` | 重新探测选中设备并只读扫描记录 |
| `FAL_SAVE` | 将当前 RAM 参数保存至选中设备的非活动参数副本 |
| `FAL_LOAD` | 将选中设备的最新有效参数恢复到 RAM 数据池 |
| `FAL_DEFAULTS` | 将 RAM 参数恢复默认值，保留 Flash 内容 |
| `FAL_LOG_APPEND` | 将当前 RAM 参数追加到选中设备日志环 |
| `FAL_TEST_ARM:1` 后执行 `FAL_TEST` | 一次性授权擦写选中设备的 test 分区 |

加载和恢复默认值修改 RAM 参数，不改动 `FAL_EDIT_*` 候选变量；再次 `FAL_APPLY` 会重新发布候选值。`FAL_TEST_ARM` 执行后清零；`FAL_SAVE`、`FAL_LOG_APPEND` 本身就是对应分区的写入命令。

结果码：0 成功，-20 无完整参数记录，-21 写后比较失败，-22 设备离线或扫描 I/O 失败，-23 扫描不完整而拒绝覆盖；其余使用 `fal_result_t`。设备 `error` 进一步区分 ID 不符(-1)、超时(-2)、写保护(-3)、坏块(-4)、ECC/页完整性错误(-5)、芯片读写失败(-6)、NAND 写入顺序错误(-7)。

## NAND 数据格式与约束

NAND 使用固定物理块地址，检查每块第 0、1 页 OOB 第 0 字节的坏块标记。坏块拒绝访问，不进行地址重编号或透明替换；运行时写入失败的块在本次启动内隔离，未实现持久化坏块表、磨损均衡或完整 FTL。

写入必须为 2048 字节整页。BSP 只接受本次初始化后明确擦除过的物理块，按递增页号每页编程一次；重启或重新探测会清除可编程记录，保存事务重新擦除非活动槽位后写入。

每页 OOB 的 8..11 字节保存格式标识，12..15 字节保存主区 CRC32，16..43 字节保存四个 512 字节扇区的 BCH4 校验（每组 7 字节）。OOB 第 0 字节保留坏块标记用途。BCH 使用 GF(2^13) 本原多项式 `0x201B`、生成多项式 `0x14523043AB86AB`，每个扇区纠正最多 4 位错误，纠正后继续检查主区 CRC32。解码失败可能修改工作缓冲区，BSP 仅在整页检查成功后复制给调用者。

全 FF 页按空页处理；已有其他 OOB 格式或 OOB 元数据损坏的页报告页完整性错误。元数据本身不在 BCH 纠错范围内，CRC 用于辅助完整性判断。写保护不会被驱动自动解除。该 demo 的恢复与错误处理不等同于量产存储可靠性认证。

硬件依据为仓库 `references/GD32E50x_Demo_Suites_V1.8.0/GD32E507Z_EVAL_Demo_Suites/Docs/Schematic/GD32E507Z-EVAL-Rev1.2.pdf`，以及同套例程的 `13_SPI_SQPI_Flash`、`15_EXMC_NandFlash`。NAND 几何、ID、纠错要求参考 [GD9Fx1GxF2A Rev2.9 数据手册](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/8888/PdfFile663921.pdf)。

## 验证记录

2026-09-05，本地 GD32E507 构建通过：`platform/gd32e507` 内执行 `mingw32-make -s -j8`。Flash 使用 125764 B，RAM 使用 91376 B，输出为 `platform/gd32e507/build/gd32e507_demo.elf/.hex/.bin`。

主机验证只替换 BSP，实际编译 demo `fal_cfg`、FAL Core、Interface、数据池、记录、存储服务及 BCH/CRC 实现；没有编译 MCU 寄存器驱动。独立测试入口：

```powershell
mingw32-make -C platform/testbench/fal_demo -s -B test
mingw32-make -C platform/testbench/fal_core -s -B test
```

新测试 13917 项检查通过，原 FAL 测试 61 项通过。新测试包含双设备保存/加载、默认值、忙时冻结、提交失败与 CRC 损坏回退、扫描失败禁止覆盖、设备独立性、日志循环、跨块测试、NAND 写入粒度及保护区校验；BCH 包含独立多项式向量、4148 个位置的单比特遍历、400 组 1～4 位错误及 100 组五位错误配合 CRC 拒绝检查。

另一个独立可执行文件 `test_fal_cfg.exe` 只编译 `fal_cfg + fal_core + Interface + 模拟 BSP`，直接读写擦除、分区权限、范围校验、忙时挂载拒绝、运行绑定参数校验、整组忙状态预检和设备独立性共 63 项检查通过。

可复现测试源码位于 `platform/testbench/fal_demo/` 和 `platform/testbench/fal_core/`，日志在各测试目录的 `test_demo_storage.log`、`test_fal_cfg.log`、`test_fal_core.log`。NAND 请求入口回归由 `platform/testbench/fal_demo/test_nand_request.ps1` 编译当前驱动的请求函数，在模拟启动边界下检查忙时拒绝和缓冲区所有权，共 40 项检查通过。

连续板上压力测试的构建开关、观察脚本及用例定义见 [FAL 板上压力测试](gd32e507_fal_stress.md)。

板上已完成双 Flash ID 读取和 312 次 Demo test 事务，重复请求拒绝后原请求仍正确完成；Core 边界、双设备独立推进及 5 轮软件复位复测均通过。结果与固件身份见 [板上压力测试](gd32e507_fal_stress.md)，首次验证记录见 [板上验证记录](gd32e507_fal_board_validation.md)。测试结束已恢复默认固件。串口命令通道、物理断电恢复、真实坏块、硬件 ECC 注入和实时性能上限尚未验证，主机故障注入不代替这些硬件验证。
