## Architecture boundary closure - 2026-10-24

`SECTION_WEAK` is now an explicit required platform.h capability, alongside the
registration/time contract. Scheduler core contains no GNU weak attribute syntax;
the selected platform defines the compiler spelling. A missing provider fails at
compile time, in both standalone and managed scheduler inclusion. No silent weak
fallback is installed. The E507 provider retains GNU weak semantics; all allocated
non-BSS core/CPU-port object sections equal the previous passed runtime. CPU port
and no-task-priority fair preemption semantics remain unchanged.

46 positive/negative/real-core/CPU contracts passed. The current platform B fixture
passed 89 hardware assertions, including five fairness/lifecycle/recovery windows
with task counts 3724/3724, 3723/3723, 3722/3722, 3720/3720 and 3721/3721. Historical
A/C/M results remain applicable runtime evidence via the exact runtime-section
equivalence check; they were not rerun merely for this attribute spelling change.

Shuttle image lifecycle/CRC/network policy stays outside base SECTION. The final
validated board image is retained by user choice; physical power tests remain
user-deferred/unverified. See the current delivery index for actual identities.

## Current verification snapshot - 2026-10-24

This section supersedes earlier pending/CRC-gap/SWD-only acceptance notes below;
those notes retain historical failure and recovery evidence.

The default task quantum is still 10 platform ticks (1 ms). Current formal-source
hardware results: A, two 600 us tasks in one firmware, 3726/3726 completions;
B, one 600 us task per firmware, initial 3724/3724 and all 5 lifecycle windows
passed; C, three equal tasks versus one, 1863/1863/1863/1864; M, 600/150/300 us,
4218 each. Each window spans at least 5 target-clock seconds. The ready-task FIFO
provides task fairness, not equal CPU shares per firmware. Fixed-instruction
minimum-calibrated CPU demand is reported separately from preempted wall time;
it is not a context-switch CPU accounting trace. 120%/240% overload does not
promise 1 ms callback deadlines. Counts, calibrated demand, maximum waits and
group totals are in the external current_acm_results/B hardware evidence.

Actual core/port/positive-negative build contracts: 44 assertions passed. This
includes rejecting missing/zero owner hooks and native candidate-read capability;
standalone base remains a legitimate mode without Shuttle dependencies. CPU
switching remains in the M port; directory/lifecycle/native cadence policy remains
in Shuttle services. No ordinary-task timeout, group weight or release-gate
relaxation was introduced.

Shuttle owns image integrity, catalog lifecycle and transport policy; these are not
implemented in base SECTION. Full integration evidence is documented in Shuttle
Docs/gd32e507_srtos_platform.md and ShuttleCfg docs/E507_SRTOS_PLATFORM.md. The
existing original board image was restored; physical power removal remains untested.

# SRTOS_STL — Shuttle 托管的 Cortex-M section 运行时

## 2026-10-24 一衍架构边界收敛（已验证）

本轮只修改本组件 core/section.c、core/section.h 和本设计文档；m/port.c、port.h
内容未变。注册段属性和 tick 微秒单位由显式框架 platform.h 契约给出，缺项报错；
毫秒注册校验精确整数表示、溢出和有效 tick 范围，不再兜底GNU注册属性或固定乘10。
所选Platform提供CPU port声明，M port继续拥有实际寄存器/FP/SVC/PendSV/BASEPRI，
core不依赖Shuttle类型、SDK、固件目录、Flash或业务策略。

base独立模式仍合法：平台显式提供契约即可，不引入Shuttle宿主头。宿主weak默认行为
仅用于无宿主情景；Shuttle集成在自己的managed_contract.h强制ENABLE/OWNER_HOOKS/
LOADER均为1，缺失或关闭在编译失败，遗漏桥接源在链接失败。组代次、撤销、引用及
快照回收仍是调度器机制；固件身份、退出READY、升级预算及资源/IRQ策略留在Shuttle。
目录解析、每1ms返回常驻服务策略属于Shuttle的native_service，不在CPU port或BSP。

本轮架构/正负构建42项、真实core旧用例18项（包含在42项内）、M3/M4/M33独立编译
及实际include/强符号检查通过；100/250/400us独立时间契约覆盖精确换算及非法值。
Shuttle 250us错误时间ABI另有真实桥接负向编译。宿主策略调用顺序、活跃期1ms机会
与目录转接经单元验证。实机平台51项、不返回INIT29项和全新使用者工程40项通过。
非目标10us IRQ擦写窗口推算漏期0；实际ADC/公共RAM/1msUDP严格连续性复验通过。
首轮BUSY碰撞的失败保留；修复只在业务任务做最多2次有界读取、不在ISR重试，
没有修改受保护公共池或放宽READY/释放门禁。

测试/产物/日志留在仓库外。既有原512KiB镜像每轮完整恢复，原固件运行、探针关闭；
无新增备份/提交/推送/发布。物理断电、生产UART、目标端整镜像CRC、绝对IRQ延迟
及完整WCET不属于本轮新增保证。当前CPU端口仍为GNU兼容工具链，非GNU未验证。
普通任务无超时；现场池不足时，native请求仍可能延后，不宣称无条件硬时限。


## E507 独立生成平台集成补充（2026-10-23）

当前 base SRTOS 源码已被 Shuttle 的 gd32e507_srtos 独立端口及 ShuttleCfg 源码载荷
采用；本轮未增加另一套调度器或普通任务超时。Shuttle 现有 binding 持有可撤销
entry/INIT 与退出维护组；任务/快照回收、owner commit、BASEPRI 边界保持本组件契约。
第一层新生成平台已做 Flash 复位启动、真实 SUP 下载/同名覆盖、IRQ 所有者卸载与
不返回 INIT 隔离；所有实机轮次均用既有原镜像恢复全片后启动原固件、关闭探针。
详细集成/范围见 Shuttle/Docs/gd32e507_srtos_platform.md。此项不等于后续真实 ADC、
公共 RAM、1 ms 以太网业务流程通过，也不宣称物理断电恢复或普通任务硬延迟保证。
测试/产品/日志仍在仓库外；不提交、不推送、不发布、不新增备份。


## 2026-10-23 托管桥接与退出修复

本次实际安装 core SHA256 `08b17855ed5aefd8619212428709fb06a5afecbfd99a8c153d206aa0ee6600f0`。
M port 源码保持既有 BASEPRI 版本。新增可信 host owner commit/switch hold 钩子和
sticky native-return 请求，分别覆盖新任务、恢复、inline 及 native 返回；
独立使用时 weak hook 有确定的无宿主默认行为。现场池耗尽仍可能延迟 native 返回。

Shuttle 沿用常驻 bindings，将旧映像 V3 回调放入同一 binding 持有的维护组，
在共享 PSP、未持 service/调度锁时执行。仅维护单次/总期限由 100 us tick 管理，
不增加普通任务超时。READY 仍须依次实际静默 IRQ/资源、归还外部 lease 和回收 FIFO；
既有模块释放/commit 门禁不放宽。失败或超时保持隔离并拒绝覆盖。

实际 E507 SRAM 组合验证 32 条断言通过，包括同名 SUP 活动覆盖、PENDING 回调、
旧代次拒绝、所有者 IRQ 卸载、组/ready/pool/record/fault 回归零，以及不返回退出回调被撤销。
非目标业务导出 RAM ISR 的两次下载窗口入口 239921/307058，推算漏周期与完整性错误均 0。
180 MHz 最大/最小相邻 C 入口间隔 1810/1790 cycles；绝对硬件响应与完整异常 WCET 未测。

本次软件回归：ARM 4/4、独立主机 18/18、桥接 9/9、维护生命周期 6/6。
16 个改动页使用已有原恢复源还原，512 KiB SHA256 回到 `1940e094...`，原固件运行、探针关闭。
源码、测试方案、证据分别可核对 Shuttle `Docs/shuttle_srtos_managed_lifecycle.md` 和仓库外
`C:\Users\76475\Documents\Codex\2026-10-23\task-2\repair\managed_scaffold\runtime\host_protocol\evidence\repair-20261023T181904Z`。

没有把 SRAM 组合、原固件复位或历史通过项宣传为托管 Flash 常驻冷启动。
任意不返回 INIT、旧镜像/头 CRC、断电 commit 恢复、外部绝对延迟仍不在本次已验证范围。
测试、产物、日志与已有备份全部留在原仓库外，不新增或删除备份，不推送发布。

## 前阶段状态与范围（以本次更新为准）

2026-10-23，ONWAY；来源基线 base `680f1a6` 的 `code/section/srtos_m`。
本文件是唯一主方案。初稿写于新增源码之前；当前D版本core74de3e0b32b8/portce5807189228
已通过BASEPRI修复后的实机公平性36窗、10us固定ISR四窗及native/FP回归，原版仅审核未改。
无实测绝对入口延迟/完整异常WCET/外部示波器结论；真实Flash升级尚未执行，证据见文末。
只新增 `srtos_stl/core/section.c`、`section.h` 和 `srtos_stl/m/port.c`、`port.h`，不建立其他架构目录。
现有 baremetal、srtos_m、srtos_a9、srtos_riscv 和公共 section 文件保持不变。
历史验证工程在 base 外的 `D:\OneDrive\LWX\GD32\srtos_stl_e507_validation`；新增测试导出被自动审批拒绝后，当前工程与全部新产物留在 `C:\Users\76475\Documents\Codex\2026-10-23\task`，不再写入OneDrive验证目录。
之前创建的 `base/tests/host/srtos_stl` 及首轮产物迁出保留；后续测试、构建和日志不写入 base，不改生成 Makefile。
后续最小桥接和owner/native hooks只在C staging验证，真实48KiB全SRAM组合ELF已链接，
尚未上板。普通任务超时需求已由用户取消，原初始化/下载等超时保留；新managed生命周期
硬超时尚未实现，不绕过覆盖门禁。控制ISR已明确不调用调度/任务组接口。物理板子当前
原固件运行、probe关闭，Flash仍是可信原哈希1940e094...；独立SRAM恢复工具先于真实写入。

## 首次实机失败及端口修正设计

`runs/20261023T032634Z_dbfe27dd` 保留首次 SRAM 执行失败：phase 1、HardFault、CFSR=0x8200，
SVC 在 `PC-2` 读取指令时使用了错误的 FP 帧偏移。Flash 全量读回与备份相同，目标暂停。
实测 native SVC SP=0x2001FEA0，核心硬件帧 PC 在 SP+24（0x20002ADE），SP+96 为零。
扩展硬件帧仍先放 R0–R3/R12/LR/PC/xPSR，再放 S0–S15/FPSCR/保留字；不能把 18-word FP 区当作前缀。
修正前明确布局：软件 R4–R11/EXC_RETURN 共 9 words，可选 S16–S31 共 16 words，随后核心硬件帧 8 words，
最后可选低 FP 硬件区 18 words。故 basic xPSR 索引为 16，extended 为 32，总长度仍为 17/51。
SVC 固定从原 MSP+24 取 PC；测试故障解码也固定从原硬件 SP+24/+20 取 PC/LR。
此次只修正 M 端口及外部故障取证代码，不修改公共调度算法。修正后重跑 4 种 ARM 编译和 17 个主机用例，
再以新 nonce 完整运行相同 E507 测试；失败记录不覆盖，硬件通过只能依据新运行证据。

## 目标与信念

- 实体：常驻内核拥有组表、任务控制块、ready FIFO、现场分配记录和共享运行栈；业务仅提供注册描述符和回调。
- 安全先验：代码地址或节点地址相同不代表同一次加载；未经资源静默确认不能宣称可覆盖。
- 时间模型：单核 Cortex-M，100 us 原生 tick；每个公开状态更新由端口临界区序列化，取消在安全切换点生效。

沿用 M 的周期激活、单 ready FIFO、固定时间片、共享运行栈复制快照和完成后的新任务快路径。
不将此组件宣传为跨 BIN 二进制 ABI 兼容、通用 MPU 隔离、无停机 Flash 擦写或 10 us 周期保证。

## 目录与职责

| 文件 | 职责 |
|---|---|
| core/section.h | 源码级 REG_TASK / REG_TASK_MS / REG_TASK_STEP / REG_INIT 注册约定，组句柄、状态、常驻 API |
| core/section.c | 注册复制、初始化管理、全局组链表、任务状态机、周期与 FIFO、快照池和有序回收 |
| m/port.h | 栈帧与执行边界契约，不包含另一套 section 实现 |
| m/port.c | GNU Cortex-M3/M4 及显式 M33 无安全扩展的帧、SVC/PendSV、PSP/MSP、FP、PRIMASK/PSPLIM |

## 核心与端口接口（已实现）

- `section_stl_port_lock/unlock`：保存/恢复 BASEPRI 0x80；调度侧 IRQ/Thread 序列化，控制快 IRQ 保持可抢占且不得调用组 API。
- `section_stl_port_prepare(low, top, entry)`、`frame_valid(sp, words)`：M 帧布局仅由端口解释。
- `section_stl_port_run()`：只接受特权 Thread/MSP、无屏蔽的 native 调用，SVC 进入；没有任务时返回 native 调用者。
- `section_stl_port_svc_handler/pendsv_handler`：独立名字，由板级显式绑定向量；不能与 Shuttle 旧 PSP 端口并用同一向量。
- `section_stl_port_request_switch()`、`in_handler()`、`active()`、`inline_allowed()`：切换请求及上下文资格。
- 核心 `section_stl_switch(saved_sp)` 在常驻 MSP、屏蔽事务内保存旧现场并选择下一任务；NULL 返回代表返回 native。
- 核心 `section_stl_entry()` 调用已复制的函数，完成后可以在当前 slice 内执行 FIFO 队首 READY_NEW，不能跨越旧现场。
- 浮点端口必须保存高 FP 寄存器和原 EXC_RETURN；lazy stacking 与 host 返回帧由端口负责。无能力则明确拒绝编译。
- 默认 GNU ARMv7-M/ARMv7E-M；M33 须显式选择下述无安全扩展模式。run 拒绝 Thread/PSP、非特权、PRIMASK/BASEPRI/FAULTMASK 非零和未启用 STKALIGN。
  FP 构建还检查 CP10/CP11 已使能；进入期间关闭 lazy stacking，返回恢复原 PSP 与相关 FP 配置。
  保存帧为 R4-R11、EXC_RETURN、可选 S16-S31 和硬件帧：basic 17 words，extended 51 words，另计 STKALIGN padding。
  PendSV 必须是最低优先级；port 不自动接管现有 Shuttle/RTOS 向量。inline_allowed 在进入核心临界区前采样。

## 数据与所有权（已实现）

固定容量组槽和任务槽，不使用堆。全局组链表链接常驻组槽，不链接业务提供的节点。
句柄包含非零 group_id 和进程内单调 generation；generation 到上限拒绝注册，不回绕。
注册从原有形状的 reg_section_t / section_item_t 收集任务，将 period、函数指针、step 参数复制进内核任务槽。
之后调度、取消、回收都不再解引用业务 reg_task_t 或 wrapper；业务函数和 ctx 在组有效期间仍属于业务内存。
REG 宏兼容源码调用形式，不承诺和旧 reg_task_t 二进制布局相同；其他 section 服务由业务原有机制处理。
全局启动初始化只允许一次，业务组初始化不清空全局调度器。

每个快照分配记录自含 `{group_id,generation,task_index,offset,words,charge,consumed}`。
record FIFO 和数据 pool 均为常驻；charge 包含回绕尾部 gap。任务槽复用不改变旧 record 的身份。
记录失效时先 consumed，仅从 record FIFO 头连续物理释放；回收不读取业务内存。

## 生命周期（已实现）

- REGISTERED/DISABLED：复制任务完成但不调度；显式 managed init 执行初始化回调并返回，期间用在途引用阻止覆盖。
- ENABLED：允许新周期任务；ENABLE 对重启相位重新建立基线，不积压历史周期。
- DISABLED：停止新激活和队列中的新任务，已有旧现场允许继续完成；不是卸载，也不是暂停。
- PAUSED：保留注册和业务数据，但本版仅接受无当前执行、无快照、无外部引用的组，否则 BUSY，原状态不变。enable 时重建相位。
- REVOKING：立即禁止激活、enable 和新引用。保存的旧现场标记失效；当前代码先到达安全端口边界，不能在其仍执行时允许覆盖。
- REVOKED：无当前执行或初始化；旧记录仅按池头消耗，外部 lease 仍需显式归还。
- `group_quiescent` 是 native 管理者的明确资源静默确认；必须确认 IRQ/回调注销、DMA 停止完成、锁/外部资源清理、无迟到入口。
- `group_forget` 仅在 native、端口未运行、引用为零、记录已回收且静默已确认时移除组、释放任务槽；返回成功才允许复用所属内存。
- 重复 revoke 幂等。旧 generation 的 enable、引用和查询不得作用于同地址新组。

撤销允许丢弃暂停的业务调用链，故业务任务必须支持取消：任务持有的外部锁、DMA 或回调要用常驻 lease 标识并由关闭流程清理。
组件不能发现绕过 lease 的引用，静默确认是平台责任。若没有可取消契约，平台应先 disable 并等待任务自然完成，再 revoke。

## FIFO 和并发不变量（已实现，CPU 边界待实机验证）

1. 恢复前验证常驻组身份、代次、任务关联和可恢复状态；失效时不复制 CPU 帧，不恢复旧 PC/SP。
2. 检查至异常返回处于同一 M 切换事务；NMI/HardFault 与高于 BASEPRI ceiling 的控制 IRQ 不得调用这些 API；调度侧 ISR 由 BASEPRI 序列化。
3. live 快照不可被覆盖；数据只从分配 FIFO 的 consumed 前缀释放，回绕 gap 一并计入 charge。
4. Pool/record 耗尽时保留当前 CPU 现场运行并报告状态，不能部分发布分配或覆盖旧记录；撤销现场不能以池满为由恢复。
5. pause 不允许钉住 live FIFO 现场；本版显式 BUSY，避免独立组暂停导致其他组永久无法回收。
6. 执行期间的 revoke 只能表示请求；仍有 CPU/初始化/外部 lease/分配记录时，不返回“可覆盖”。
7. native 初始化及注册和接口调用必须有界；初始化回调本身的超时/看门狗由 Shuttle 托管入口实现。

## 上下电、下载和 Shuttle 接入责任

本组件只维护易失 RAM。冷启动清空 BSS 后重新验证固件、建立组和新代次，不恢复持久化 CPU 快照。
持久化启用策略、commit 标志、断电下载恢复、Flash 擦写与 DMA 等待由已有 Shuttle catalog/download 管理。
Shuttle 必须导出经过 ABI 版本验证的组注册接口、把当前模块身份绑定到调用者、校验注册表与回调可执行范围。
托管入口先构造本地 section 注册表，再调用常驻注册/初始化接口并返回；不能运行旧版 run_task 或重置常驻状态。
覆盖前接入 disable/revoke → IRQ/回调/DMA 清理与 lease 归还 → native quiescent → forget → 既有下载门禁。
本轮不实现以上 Shuttle 对接，不把 native 资源确认函数等同于已经停掉硬件。

## 验证计划

先用本机 Arm GNU 编译 core 和 M port：Cortex-M3 soft-float、Cortex-M4 hard-float、M33 soft/FPv5 hard；执行无硬件的链接/反汇编检查。
之后主机测试编译真实 core，仅替换 port，覆盖：注册复制、managed init 返回、全局组多实例、代次失效、enable/disable/pause、
FIFO 首/中间取消、跨环回、重复 revoke、当前执行保护、lease 阻止覆盖、pool 耗尽、容量与无任务返回。
严格 C11 警告采用 GNU C skill 策略。主机模拟不能证明真实异常返回、FP lazy stacking、ISR 时延、DMA 停机和 Flash 安全。
独立复核发现 inline 快路径原来没有被 fake port 执行；补充可控 inline 模式，验证新任务直启、旧现场队首不可跨越、
到期/待切换阻止直启，以及 callback 内 revoke 后的安全退场。该补测设计记录于新增测试代码之前。

新增 E507 验证阶段：先只读识别探针、精确 MCU、活动调试会话和现有工程；不得通过猜测连接/复位目标。
确认固件可读备份和对应恢复方法，备份、工程、map、日志全部放 base 外。不修改读保护/安全配置，不做无必要整片擦除。
板上功率输出、外设连接和安全状态不明时，暂停依赖硬件的部分并提出具体问题。
目标确认后先构建只使用内核、时基和 RAM 结果区的最小程序，不驱动功率输出。
按能力依次验证真实 MSP/PSP 往返、周期调度、抢占栈复制/恢复、撤销/失效 FIFO 回收和重复注册代次。
主机合成帧 PASS 与实机 PASS 分开记录；不会把 E507 最小验证视为 Shuttle 下载接入或突然掉电验证。

## 证据与遗留

- 设计初稿先落盘到 base 后，新增四个源码；已完成软件构建、主机测试及独立审查。
- 已实现：常驻组链表、描述符复制、非回绕代次、managed init、enable/disable/保守 pause、两阶段 revoke/确认/forget、引用计数、独立 FIFO 分配记录。
- 已实现：M3/M4/M33（无安全扩展）的端口、host MSP 返回、basic/FP 帧、初始栈、PSP 写入前边界检查；失败进入 UDF，不能假装已返回 native。
- 交叉编译：M3 soft、M4 soft/softfp/hard 端口通过；真实 core+port 的 M3soft/M4hard/M33soft/M33FPv5hard 严格编译及 relocatable link 通过。
- 主机测试 17/17、ARM 编译 4/4 通过；E507 SRAM 真实 CPU/FP 切换及撤销回收已通过，覆盖边界见文末。
- M0、非 GNU 编译器、A9、RISC-V：不属于本轮实现范围。

## 实现细化与审查记录

- Cortex-M 清 PRIMASK 到异常返回之间存在可被高优先级 IRQ 抢占的窗口。revoke 若命中 current，会在锁内重新请求 PendSV；尾链应使撤销现场先到安全边界而非继续业务代码，此项需要硬件接受测试。
- group_acquire/release 是受信任调用方的引用计数，要求每次成功 acquire 恰好 release 一次。不是防重放 lease token；重复完成通知需由外部资源层去重。
- managed init 在常驻调用者栈上同步执行，不启动调度。init_busy 期间拒绝 section_stl_run；超时和恶意不返回不由此组件解决。
- 初始化回调列表使用单份常驻缓冲，不在小 MSP 上分配大数组；禁止任意组的嵌套 managed init，且 sticky fault 后拒绝新初始化。
- 初始化表必须由 loader 验证为该组同一有效映像，并一直保持可读；函数本身不验证代码签名、Flash 范围或跨 BIN ABI。
- REG_TASK_MS 等宏将越界周期转换成无效 0，由注册统一拒绝，避免乘法截断变成意外短周期。
- port.c 必须 -fno-lto；所有参与对象需要兼容的 CPU/FPU ABI。板级绑定 SVC/PendSV，PendSV 最低优先级，硬件自动入栈仍需 MSP/PSP 余量或 MPU guard。
- sticky CORRUPT 后不再保存/恢复任何业务 CPU 帧，也不提供覆盖许可；只允许诊断与受控复位。POOL_FULL 是可观测容量事件，保留仍有效的当前执行。

## 最小接入与使用（已实现的源码接口）

构建只选择本组件 `core/section.c` 和 `m/port.c`，头搜索路径添加对应两个目录。
不要同时链接另一套 section.c，也不要在同一编译单元同时包含旧 section.h 和本组件 section.h。
现有公共 platform.h 可提前定义 SECTION_REG_ATTR_PREFIX/SUFFIX；否则使用 GNU `section("section")`。
本版只实现任务和初始化收集；其他 SECTION 服务不会自动迁移进此运行时，注册中的非任务项由原有业务服务负责。
注册宏没有自动生成一个可安全调用的跨固件函数表；下列是常驻源码 API 的调用顺序。

```c
/* Native cold boot: exactly once, not once per business firmware. */
section_stl_init(now_100us);

/* Loader validates and pins this image and its registration range. */
section_stl_handle_t handle = {0};
section_stl_group_register(module_id, entries, entry_count, &handle);
section_stl_group_initialize(handle, entries, entry_count); /* Returns; never starts a private loop. */
section_stl_group_enable(handle);

/* One native timebase, including periodic ISR ticks during task execution. */
section_stl_tick(now_100us);
section_stl_run(); /* Native Thread/MSP only, returns when no work remains or a fault stops dispatch. */
```

上段省略错误分支以展示顺序；可编译并检查每次返回值的完整例子在 base 外验证工程的 `host/example.c`。
任务仍可使用 `REG_TASK_MS(2u, task)`、`REG_TASK_STEP_CTX(...)` 和 `REG_INIT(priority, init)`；init 必须返回。
`section_stl_tick` 单位固定为 100 us，原生控制中断可更快，但这不等同于任务调度的 10 us 保证。

取消流程必须由 native loader 实现：

1. `group_revoke(handle)` 关闭入口并使已有 snapshot 失效；BUSY 表示请求已生效但当前 CPU/init 尚未退出。
2. 在原有关闭流程中注销 IRQ/回调入口，等待 DMA 真正停止，处理丢弃栈持有的资源，并归还每个引用。
3. 原生循环继续服务其他组，让 FIFO 前方的有效记录正常消费；中间 tombstone 暂时占空间，不移动 live 现场。
4. native 下调用 `group_quiescent(handle)` 作为外部清理已完成的明确确认。
5. `group_forget(handle)==OK` 后才能把内存交给已有 Flash 下载门禁；旧 handle 此后返回 STALE。

单独 disable 不授权覆盖，保留旧 CPU continuation 供其完成；已完成的 step 不再继续排新 step。
如不具备任务可取消契约，应先 disable 并等待所有任务自然退出，不能直接取消持锁调用链。

端口入口为 `section_stl_port_svc_handler` 和 `section_stl_port_pendsv_handler`，须直接放入向量或无栈汇编跳转。
普通 C wrapper 不合法；现有 Shuttle 和 FreeRTOS 若占有这些向量，必须先完成一个统一的端口所有权设计。
本组件没有更改这些平台文件，因而不是已接入现有 Shuttle 镜像的完成版。

## 硬件验收范围（逐项结果见文末实机记录）

- basic / extended FP 帧，Thread/MSP → task PSP → native MSP，包含中途抢占与 native FP-active 调用者。
- PendSV 最低优先级，清 PRIMASK 到 EXC_RETURN 窗口的 revoke/尾链，以及 pending 请求返回 native 后不重入旧组。
- MSP 最坏异常嵌套和保留 host 帧预算，PSP 硬件自动入栈边界、FP eager stacking 和溢出故障路径。
- 所有外部 callback/DMA 引用的恰好一次登记释放、迟到中断、停止失败保持 Flash 锁定。
- 真实 Flash 同 bank 擦写停顿、缓存/总线/中断驻留约束、掉电 commit 恢复与持久化使能策略。
- 长任务、禁中断代码及总线争用下的延迟；目前仅有软件证据，不承诺实时期限。

## 软件验证证据（2026-10-23，ONWAY）

实际源码验证入口（从 base 外验证工程执行）：

```powershell
python host/run.py --source-root D:\OneDrive\LWX\GD32\base\code\section\srtos_stl
```

脚本强制先编译真实 ARM core + port，再允许宿主测试；没有 ARM 工具链时会失败，而不是用 host PASS 代替。
当前实际仓库源码：ARM GCC 14.2.1，M3 soft、M4 hard、M33 soft、M33 FPv5 hard 严格 C11 -O2 -fno-lto 编译与可重定位链接 4/4 通过；
未解析符号仅 memcpy/memset，由最终板级 C runtime 提供。不是完整板级固件链接。
MinGW GCC 6.3.0，真实 core 配合 fake port，17/17 独立进程用例通过（含 3 项 inline 补测）。

结果文件在 base 外验证工程 `host/build/`：`test.log`、`results.json`、`m3_soft_disassembly.txt`、
`m4_hard_disassembly.txt` 和对应 `*_size.txt`。fake 使用合成帧标记与显式安全边界，不执行或仿真 ARM 指令。
新增 M33 也有同名形式的反汇编和 size 文件；results.json 保存四个生产源码/头文件的 SHA-256 以绑定验证版本。

默认配额（8 组、32 任务、32 记录、512-word 共享栈、1024-word 池、128 init 槽）的组合对象 BSS 为 10036 bytes；
还未包含板级 MSP、调用方栈、C runtime 和其他 Shuttle 数据。M3 组合 text 为 5764 bytes，M33 hard 为 6012 bytes（实际最终链接可变化）。
不能直接放入此前 Shuttle 的 8 KiB 私有 RAM 配置；集成时必须按配额调节并审查链接 map，本轮未改现有内存布局。

原来被 base `/tests/host/*` 忽略的本轮测试目录整体迁出，首轮证据保留在验证工程 `previous_host14/`。
未修改原有忽略规则或暂存任何文件。本轮没有 commit/push；E507 使用 SRAM 加载执行，未烧写或擦除 Flash。

## E507 验证扩展设计（2026-10-23，先于本块代码）

只读发现现有 `platform/gd32e507` 构建使用 Cortex-M33/FPv5，而非 M4；当前 M3/M4 端口不能直接宣称支持 E507。
端口扩展已实现，限定显式 `SECTION_STL_M33_NO_SECURITY=1` 的 Cortex-M33、无 Security Extension 配置；不扩展 TrustZone 切换。
进入前检查 CPUID 的 M33 part 及 `ID_PFR1.Security==0`，不符合即拒绝派发，不修改 SAU、读保护或安全配置。

[Arm DDI0553B.y](https://community.arm.com/cfs-file/__key/communityserver-discussions-components-files/471/DDI0553B_5F00_y_5F00_armv8m_5F00_arm.pdf)
的 D1.2.96 规定无安全扩展时 EXC_RETURN.S/ES 为 0、DCRS 为 1；D1.2.141 定义 ID_PFR1.Security。
因此 M33 无安全扩展的 PSP 返回值应为 BC/AC，原生 MSP 为 B8/A8（均带 FFFFFF 前缀），不能沿用 M3/M4 的 FD/ED、F9/E9。
其普通/FP 帧字数保持 17/51，本组件不跨安全域，不含额外安全状态帧。
M33 在每次恢复共享 PSP 前设置 PSPLIM 为常驻栈下界，保留 native 原 PSPLIM 并在返回后恢复；
native 清理在临界区先临时清 PSPLIM，再恢复原 PSP 和原 PSPLIM；MSPLIM 保持板级配置。
架构选择、返回值和栈限制只放入已有 `m/port.c/.h`，核心算法和目录组织保持不变。

已增加 M33 soft、FPv5 hard 的真实对象编译/链接与反汇编，17 项核心主机回归仍通过。
M33 未显式选择、M33 Secure CMSE、M4 误用 M33 选择三个错误配置均被编译拒绝；M33 运行时能力门禁尚待板级执行。
上述 4 种配置结果本身只代表构建；其中 E507 M33 hard 另有文末实机证据，不能外推到其他处理器或安全配置。
探针枚举发现一个 SEGGER USB 设备，序列标识 `000069409716`，CDC 为 COM4；没有发现活动的常见调试进程。
用户确认 GD32E507Z EVAL 板，已核对下述实际原理图。
用户进一步明确“没有，只是一个开发板”，确认没有外接功率级、负载或执行器；相同安全条件不再重复询问。
实机 CPUID=0x410FD214、ID_PFR1=0x00000200，确认 Cortex-M33 r0p4、无安全扩展；J-Link 69409716、VTref 3232 mV。完整 512 KiB Flash 两次独立读取相同，SHA-256 为 `1940e0940f4fb5f85f336271a094e8da24807b3f551340dbdfd7744d1600be3d`，备份在外部验证目录 `backup/20261023T030746Z_15b91be8/`。
历史日志曾识别同序列探针连接 GD32E507ZET6 且无安全扩展，但不据此跳过本次运行时检查。

实机已使用独立 SRAM ELF，向量、代码、数据、现场池和 MSP 全放 SRAM。
经安全确认和固件读回备份后，再受控 reset/halt、加载 RAM、设定 MSP/PC 并运行；不需要擦写 Flash。
测试只启用内核时基/FPU和 RAM 结果区，不启用 GPIO、PWM、ADC、DMA 或业务外设；普通复位回原 Flash 固件。
RAM 执行仍会停止并改变当前易失状态，因此不能绕过板上安全确认。Flash 读保护若阻止备份，不尝试解除。

### 独立 SRAM 程序设计（已实现；安全和备份已确认）

现有 `platform/gd32e507/ldscripts/gd32e507ze_flash.ld` 描述 ZE 的 Flash 512 KiB、SRAM 128 KiB。
该信息只作为候选链接配置，必须与当前连接芯片一致才可加载。SRAM 链接范围 0x20000000..0x2001FFFF，
顶端保留 8 KiB native MSP，链接 ASSERT 检查程序/池不与 MSP 重叠；向量 512-byte 对齐。
文件放外部验证目录 `firmware/`：`startup.S`、`main.c`、`evidence.h`、`ram.ld`、`build.py`，全部产物仍在该目录内。
不调用原工程 `SystemInit`（它会切 PLL 和配置 Flash）；先核对复位 RCU 确为 IRC8M、AHB /1，
才配置 SysTick LOAD=799。条件不满足时写 RAM 错误码并停止，不擅自改变时钟来源。

程序只含单一 RAM 结果结构：版本/magic、阶段、首个失败码、tick、回调计数、局部 canary 结果、
各阶段 stats、组身份/代次、native CONTROL/PSP/PSPLIM/MSPLIM 及 fault 寄存器。故障/超时为 FAIL，
只有所有断言完成才发布 PASS magic；没有 printf、串口、半主机或功率外设输出。

按下列顺序测试，每阶段释放组后再进行下一阶段，不重置全局调度器：

1. native MSP 基线、managed init 同步返回、无任务 run 返回；验证全局 init 只能一次。
2. 一个 5 tick 周期短任务执行 4 次后 self-disable，核对累计释放时刻、PSP/Thread、native 返回及随后 12 tick 静默。
3. 两个 self-disable 长任务保存 volatile 局部 canary，等待多个 slice；SysTick 触发真实抢占，
   要求 saves/restores 增加、canary 正确，完成后 ready/record/pool 为零。
4. 三个同时 ready 的任务按首次执行分配角色；前两者各被抢占留一份现场，第三者先撤销较新记录所属组。
   在有界受保护事务中核对中间 tombstone 不提前回收，再撤销最老记录组，验证连续 FIFO 前缀消费。
   被撤销两个任务的退出标记必须保持 0；不得恢复其 PC/SP，native 后才 quiescent/forget。
5. 当前长任务由 SysTick 撤销，首次返回 BUSY；安全切换后回 native，旧任务后续标记不执行。
6. 同组 ID、同描述符与函数地址重新注册，确认 generation 增加、旧句柄 STALE、新任务能完成。

每个长等待都有可观测 tick 上限，SysTick 失效时由外部调试超时停止；软件不能证明完全失去中断后的恢复。
FPU 上下文、高优先级窗口尾链、栈越界和突然掉电另列专项；未被本程序明确断言的能力不计为实机通过。

原理图实际路径：`references/GD32E50x_Demo_Suites_V1.8.0/GD32E507Z_EVAL_Demo_Suites/Docs/Schematic/GD32E507Z-EVAL-Rev1.2.pdf`。
第 9 页 U1 是 GD32E507ZET6；第 13 页 PD13 通过 R39/Q2 控制 CN3 OTG 的 +5V VBUS，低电平可使其导通。
因此验证程序完全不配置 GPIO，也不沿用可能按 USB_ID 处理 PD13 的其他示例；所有外接负载确认来自用户，而非原理图推测。

验证骨架已建立整数/FP 寄存器探针初稿；复核发现两任务相同种子可能漏掉跨任务污染。
在把这些探针接入主测试前补充验收设计：每任务使用不同 seed，对 r4–r11 和 s16–s31 设置不同位模式，
跨多个真实 slice 后逐项比较，分别累计整数和 FP 检查数；同时保留 volatile 局部栈 canary。
FP 探针只在 FPv5 hard 构建执行，包含 FPU 已激活的 native 调用者往返；不据此宣称覆盖所有 FP 异常组合。
native 每次 run 前后核对 CONTROL、PSP、PSPLIM、MSPLIM 和 MSP，保证 M33 栈限制保存恢复被实测断言。
该软件探针只为验证程序新增，不改变生产核心帧格式或组生命周期。

启动骨架复核还发现故障处理若把 MSP 重置到原栈顶，可能在 C 取证前覆盖原 native 异常帧。
实现前调整验证链接布局：单独保留 1 KiB fault 栈 0x2001DC00..0x2001DFFF，
原 native MSP 仍完整占用 0x2001E000..0x2001FFFF；普通代码/数据/池的最终链接上限为 0x2001D000，另预留 native PSP 哨兵区。
fault 入口先保留原 EXC_RETURN、SP 和 IPSR，再切换到独立 fault 栈及其 MSPLIM，
C 取证仅在原帧地址/长度通过边界检查后读取 PC/LR。故障后不会尝试回到业务 CPU 现场。

周期验收按第 n 次累计释放时间 `now - anchor >= n * period` 判断，不要求相邻回调严格相隔一个周期；
相位对齐调度允许上一次回调晚执行后下一次间隔缩短。本版用一个周期任务完成 4 次释放及禁能后 2 个周期静默检查。
M33 native 的不活动 PSP/PSPLIM 若一直为 0，会漏掉错误清零；验证将预置为 0x2001D800/0x2001D000，
不从该栈执行，链接断言普通映像结束不超过 0x2001D000，再检查每次 run 均保留这两个非零值。

FP 独立审查发现仅运行不改 FP 的短任务不足以验证原生高 FP 寄存器恢复；正常回调自行 vpop 也可能掩盖缺陷。
因此原生 S16 哨兵要跨越“任务写入不同 S16–S31 后、未执行 vpop 就被 ISR 撤销”的 32 次试验。
任务使用有界长 FP 探针，撤销必须先到来；原生包装器检查 S16 与 FPCA，确保取消调用链也恢复原生扩展帧。
正常 FP 抢占阶段另检查探针前后的 save/restore 计数增加，避免仅靠整数阶段的拷贝计数声称 FP 抢占已测。
最短撤销延迟可能抢在 FP 种子写完之前；FP 探针在完成全部高寄存器写入后发布 fp_ready，
SysTick 仅在该标记有效后撤销，并统计 32 次已就绪撤销。未就绪不能计为 FP 恢复测试通过。


## E507 最终实机记录（2026-10-23，已测试）

外部验证工程：`D:\OneDrive\LWX\GD32\srtos_stl_e507_validation`。
有效通过证据：`runs/20261023T033018Z_fc11fb37/` 的 `evidence.json`、`outcome.json`、
`registers_after_run.json`、`sram_after_run.bin`、`main_flash_after.bin`、`original_recovery.json`。
前两次调试器交接在 Reset/写 SRAM/Go 前停止；首次实际执行的 FP 帧偏移故障及原始 ELF/证据保留在
`runs/20261023T032634Z_dbfe27dd/`。该故障已按前述方案修正，未隐藏失败或复用 PASS。

最终测试 ELF SHA-256：`b1fcee736b14bbe873446ff24b8026573ce09e1b4a5a5822f502e4bde50b9f9c`，
nonce=`0x7F3101B1`，source marker=`0x9FF8639A`；run 目录保留 ELF、manifest 和精确 runner。
板上身份匹配，phase=6、PASS、fail_code=0、fault=0，667 个名义 100 us tick 后结束。

| 验证项 | 实测结果 |
|---|---|
| 托管初始化与原生返回 | init 次序 12；MSP/PSP/CONTROL/PSPLIM/MSPLIM 前后一致 |
| 周期与禁能 | 5 tick 周期共执行 4 次；禁能后 12 tick 无新回调 |
| 共享栈复制抢占 | 2 个长任务，48 个局部 canary 正确；整数及不同种子高 FP 寄存器检查通过 |
| 真实 FP 抢占 | 2 次完整 FP 探针通过；各探针窗口观察到保存/恢复（聚合计数 7/8，窗口可重叠） |
| 失效记录有序回收 | 中间撤销仍占 2 记录/146 words；队头撤销后变 0；旧任务返回计数 0 |
| 相同地址重新注册 | 同 group ID/描述符/函数地址，generation 4→7；旧句柄拒绝，新任务执行 |
| ISR 撤销当前任务 | 32 次返回 BUSY 后安全切回；旧任务出口计数 0；32 次均等 FP ready 后撤销 |
| 原生浮点恢复 | 32 次 native S16/FPCA 检查通过，被取消任务未先执行自己的 vpop |
| 最终资源 | saves=31、restores=29、discarded=2；pool/record/ready/fault 均为 0 |

计数是本次运行的观测值，不是实时性能承诺；`preempt_exits=3` 包括 2 个长任务和 1 个重新注册任务。
全量 Flash 前后 SHA-256 与双读备份一致；恢复原固件只需复位启动，不需要写 Flash。
原固件已复位恢复运行：原 Reset PC=0x08016354、MSP=0x20020000，关闭探针前 halted=0，连接已释放。
证据为同目录 `original_recovery.json`；不把 RAM 内容替换后的旧 CPU 现场当作可恢复状态。

复现软件检查（在外部验证工程中）：

```powershell
python host/run.py --source-root D:\OneDrive\LWX\GD32\base\code\section\srtos_stl
python firmware/build.py
```

修正后重新执行严格 C11、`-Werror -O2 -fno-lto`：M3 soft/M4 hard/M33 soft/M33 hard 4/4，
主机 17/17 全部通过，结果和最终生产源码哈希在 `host/build/results.json`。
主机 fake port 不执行 ARM 指令；M3/M4/M33 soft 只有编译证据，实机仅为本块 E507 M33 FPv5 hard。

尚未完成：Shuttle 常驻导出表/链接布局与向量接入、下载覆盖流程的 IRQ/回调/DMA 静默对接、
持久化使能状态及突然掉电恢复、Flash 同 bank 擦写可执行性、10 us 调度及最坏延时测量。
本次没有制造栈溢出、穷举所有异常尾链窗口或执行安全扩展切换；默认 BSS 10036 bytes 仍需按 Shuttle 预算裁剪。
用户已确认仅开发板、无外接负载；没有修改保护/安全配置、驱动业务外设、擦写 Flash、推送或发布。

## 2026-10-23 公平性回归设计（实施前记录）

外部主方案 `srtos_stl_e507_validation/integration/TEST_PLAN.md` 已冻结阈值。
E507 IRC 8 MHz 的首个 600 us + 600 us / 1 ms / 5 s 实测失败：启动
1368/1250，完成 1367/1249，校准均为 602.5 us；不放宽 2% 阈值。
原始证据 `fairness/runs/20261023T044501Z_43fd7c61/fairness.json` 保留。
这属于组件测试，不能作为 Shuttle 业务下载或升级通过的证据。

源码比较确认两处偏离原 `8c03698` 调度事务的迁移：

1. 新 `section_stl_tick` 先激活到期任务，之后 PendSV 才把当前未完成任务
   保存并追加 FIFO；原版先追加被抢占任务，再激活到期任务。
2. 新完成路径置 SLEEPING 后不扫描到期任务，原版完成事务会先处理到期
   再选取 inline 队头，保留完成顺序。

计划：用独立主机最小事件序列证明两个差异；然后恢复上述事务顺序。
托管任务执行期间 tick 只推进公共时间并请求周期切换；此时激活在切换或完成事务中进行。
原生空闲期间 tick 仍激活到期任务，保留既有 ready_count 可观察语义；无当前任务需要先追加。
完成时，step continuation 仍先追加，随后激活睡眠且到期的任务；不会
越过已有 OLD 队头。SVC/native run 已进入 switch，因此空闲时仍可释放到期任务。
不改变周期单位、全局 slice 相位、组生命周期、池布局或阈值。
周期 PendSV 重入/尾链另行调查，未证明前不在通用 request_switch 中忽略请求，
否则可能吞掉撤销所必需的切换。

验证顺序：原源码回归 FAIL → 最小修复后 PASS → 原有主机 17 项和
M3/M4/M33 soft/hard 交叉编译 → 同时钟同阈值实机重测；有限诊断 trace
仅定位，不混入最终无 trace 基准。

实施记录：原源码两项顺序回归均失败；第一候选完全移除 tick 激活后两项通过，
但原有主机用例发现空闲 tick 的 ready_count 可观察语义改变。第二候选因此仅在
p_current 非空时推迟激活，保留原生空闲行为；此修订先记录再修改代码。实机重测待进行。

第二候选 `e41ca2c9afd412a6e1bd7a15b2ad1385375fb3a84b6e603f1aa87ad33712cd07`
已通过新顺序回归 2/2。原 inline_old_head 用例的 ready_count 从 2 改为 3：
完成且已欠期的任务现在立即进入队尾；原有旧现场帧必须先恢复的断言继续保留。
完整主机回归 17/17、M3 soft/M4 hard/M33 soft/M33 hard 交叉编译 4/4 通过。
证据在本机 `C:/Users/76475/Documents/Codex/2026-10-23/task/` 下：
`fairness_regression/verify_e41ca2c9afd4_k3ds9h7o/results.json` 与
`srtos_stl_tests/build_fairness_final_local/results.json`。

新增诊断源码、二进制和日志写入 OneDrive 验证目录被自动审批拒绝，未执行、未重试；
后续证据改留上述本机 C 盘工作区，均在 base 外。既有 OneDrive 证据保持原样。
新公平性实机矩阵正在准备，尚不能宣称 2% 门槛通过。

### 周期请求与异常尾链（修复前设计）

e41ca2c9 实机首两窗同组 AB/BA 通过，第三窗跨组 AB 启动 1652/1612，
差 2.451%，仍未达到 2%。两任务校准均 602.5 us，仍用原 8 MHz/2% 判据；
保存 `task/fairness_final_local/runs/20261023T050419Z_5b08cb13/fairness.json`。
无故障、池最终为空；原固件恢复运行，探针已关闭。

另一个确定遗漏：原 `srtos_m/section.c:859` 在周期请求前检查
`SECTION_PORT_CONTEXT_SWITCH_ACTIVE()`，该宏读取 SCB.SHCSR.PENDSVACT；
已处于 PendSV 的 tick 不再次挂起同一切换。新 M 端口在解除 PRIMASK 到
异常返回之间可被 SysTick 抢占，此时旧周期请求可能造成任务零进度尾链。
有限旧源码轨迹已观测到该现象，但不把单条轨迹当作全部偏差的充分解释。

新增 M 端口只读查询 `section_stl_port_switch_active()`，由 core 的 tick
周期请求条件使用。通用 request_switch 保持无条件请求：运行中撤销、yield
或完成请求不能因 PendSV 正活跃而丢失。SHCSR.PENDSVACT 位只属于端口层。
先以 fake-port 活跃位模拟证明周期请求被抑制而 revoke 仍请求切换，再执行
4 种 ARM 编译、原 17 项、全部新回归与同条件无 trace 实机矩阵。此段先于代码。

该查询已实现；core `9b786676...` / port `67038b17...` 通过 ARM 4/4、原主机
17/17、新回归 4/4（包含 active 周期抑制与撤销请求保留）。但 8 MHz 最新首窗
仍失败，启动 1637/1544，校准 601.25/602.5 us；切换耗时占窗口 39.81%。
故目前只确认迁移顺序和重复请求修复，不宣称已达到 8 MHz 下 2% 公平性。
证据保留于本地 `fairness_final_local/runs/20261023T051425Z_4c8095fb/`。
原固件已恢复，Flash 未变。另设真实 Shuttle 时钟的 180 MHz profile：原理图
Y2 25 MHz，复用原 SystemInit/SystemCoreClockUpdate，运行前校验时钟寄存器与
RAM 向量；仍用同一 600 us/1 ms/5 s/2% 判据。该新频率测试不替代 8 MHz 失败。

### 保持过载后的周期相位（候选设计，尚未修改生产）

180 MHz 实际时钟校验通过后仍失败：启动 4264/3772，偏差 12.245%；
两项均校准为 601.833 us，切换仅占窗口 1.861%，无 delayed tick。
因此不能把公平性失败归因于 8 MHz 性能。原固件已恢复，Flash 未变。
证据：本地 `fairness_180mhz_local/runs/20261023T052351Z_95246f7d/fairness_detail.json`。

当前继承原版的过载规则在 elapsed >= 2*period 时令 last=now，会移动周期相位。
例 period=10、last=10、now=35：当前下一次45；保留原相位应下一次40。
这不是本次迁移遗漏，原 srtos_m 已有相同策略。候选是在合并过期释放时使用
`last += (elapsed / period) * period`；不补发重叠激活，不改变ready FIFO或CPU现场格式。
先做真实core公共API的35→40最小回归及有界工作量模型对照，再决定候选是否值得
实机验证。模型不能证明CPU切换或实机公平性；最终仍以原2%矩阵判据评估。

候选实施与验证：C盘候选 core `2ce3c9b37a8dbffa4f6afe0b2981be73c3c082d87962e05ca51544ad6714a370`
已通过真实ARM 4/4、原主机17/17、新回归5/5。真实180MHz相同校准和阈值下，
36个窗口终态 PASS（phase36/fail0/fault0/ticks1800000）；完整证据与原固件恢复
仍在收尾。采用此一行相位修复到组件，随后重跑实际生产路径验证。
原8MHz与180MHz失败全部保留，不宣称对所有精确相位有数学保证：工作量模型
仍有零开销、精确600us/tick优先的边界反例，尚无该边界的CPU实测结论。

### Shuttle 最小桥接当前范围

`D:/project/Shuttle/code/core/shuttle.c` 和 `code/app/managed/` 已增加可选的
原生预留句柄、resident组绑定、撤销/静默与覆盖门控、有限REG_LINK协议泵。
业务激活与升级执行仍明确 UNSUPPORTED，实际native/owner端口和预算口径未接通。
桥接主机6/6、默认核心11/11、协议14/14通过；ARM容量链接为RAM15664/32768、
Flash21072/65536，旧8KiB布局预期溢出。这是容量与接口验证，不是整体验证。
12个原ESP用户修改哈希未变。整镜像CRC缺失仅作为旧缺陷报告，未改不兼容协议。

### 最终相位修复验证记录

相位修复已落盘生产组件，core SHA256仍为 `2ce3c9b3...`。真实180MHz共36/36窗通过；
600us的20窗最大偏差：启动0.174499%、完成0.149589%、校准CPU工作估计0.164379%。
代表窗启动4014/4012，完成4014/4011。600us校准601.8333–601.8444us；
200/450/mixed各4窗无合并释放；500us四窗允许过载，合并释放合计747/745。
全36窗清理后pool/record/fault均0，最大启动间隔2251.45us，无延迟tick。
证据：本地 `fairness_180mhz_local/runs/20261023T055049Z_6228feac/fairness_summary.json`。
原固件resetGo后halted=0、probe关闭；Flash前后SHA256与可靠备份完全一致。

实际生产路径重测：ARM4/4+主机17/17、新回归5/5、桥接6/6均通过；
容量链接RAM15664/32768、Flash21068/65536，旧8KiB布局预期失败。
本机证据分别为 `srtos_stl_tests/build_phase_production/results.json`、
`fairness_regression/verify_2ce3c9b37a8d_qdml_ftp/results.json`、
`managed_scaffold/host/evidence/run-3uwtanhf/result.json`、
`managed_scaffold/evidence/CAPACITY_ONLY-jbb6dsbc/result.json`。
固定ISR的首轮instrumented测量有漏期，但其PRIMASK内统计包装构成额外开销；
正在进行仅去除该包装、直接链接生产core的同条件定向复核，不改变端口锁模型。

固定ISR最终复核完成：直接链接生产2ce3，ELF确认无统计切换包装；负载段仍漏期，
双任务9996次/最大入口间隔38.5333us，禁能使能9947次/40.0833us，撤销重新注册
9898次/38.5722us。空载零漏期；整数/浮点完整性错误0。旧IRQ用9b78，新复核用
2ce3且移除包装，不能把数值差单归因于包装。最终生产路径本身仍不满足10us用例。
PRIMASK保护完整切换事务；优先级更高的固定timer也无法在此期间进入。
绝对入口延迟、完整异常WCET、外部示波器、Flash擦写并发均未测，不作时序保证。
证据：本地 `fixed_isr_local/runs/20261023T055948Z_1c68d7be/ASSESSMENT.md`。

若后续设计高优先级快速ISR端口，必须先明确该ISR是否可禁止调用调度/任务组API；
不能在未约定数据并发边界时直接把PRIMASK改成BASEPRI。预算CPU/墙钟口径也仍待确认。
真实managed端口、owner切换及业务激活未完成，因此整体Flash下载升级、擦写中固定ISR
连续性、提交阶段实机故障注入、突然掉电与整合长测仍未执行。旧整镜像CRC缺口仅报告。

06:02:45已恢复原固件running并关闭probe，全Flash与双备份SHA256一致，未擦写Flash。
当前无占用的硬件会话；所有新测试/日志位于本地C盘，原OneDrive拒绝复制未重试。

### 中断优先修订（2026-10-23，先设计、后实现）

用户已取消普通托管任务超时需求；初始化、下载等原有超时保持。此前等待任务预算
口径的事项关闭。原版 srtos_m 仅审核，不修改。

原版 E507 PendSV（platform/gd32e507/src/gd32e50x_it.c:86）没有整体PRIMASK，
复制现场可被高优先级IRQ抢占；原版任务完成路径 section.c:1126 的PRIMASK区仍含
扫描，不能推定原版满足10us。新版 m/port.c:409 将整个切换包进PRIMASK是新增的
屏蔽扩大；priority0 TIMER1启停场景最大入口间隔40.0833us、漏期9947。这是入口
间隔，不是已测绝对延迟或完整ISR WCET。

计划：M端口 lock/unlock 与SVC/PendSV事务改用BASEPRI_MAX，默认编码阈值0x80，
保存/精确恢复原BASEPRI，不改变PRIMASK；长现场复制、扫描及FIFO回收仅屏蔽
内核相关优先级，priority0控制IRQ可抢占。致命栈错误关IRQ是不可返回故障路径。
运行前校验优先级分组及SVC/PendSV/SysTick抢占关系（即使未启用也配置SysTick，
避免读取CTRL清除COUNTFLAG）。运行期间优先级固定。测试控制IRQ0、SVC C0、
SysTick80、PendSV F0。

新增 port_api_allowed：Thread允许；阈值及以下紧迫度IRQ允许；更紧迫IRQ与
NMI/Fault拒绝。core全部公开状态API在访问共享元数据前检查，拒绝返回BAD_CONTEXT；
stats_get只填调用方输出（全0、fault=BAD_CONTEXT），不读写全局统计。注册/初始化/
覆盖许可仍额外要求native。此约束只针对非重入调度表/FIFO API，不禁止控制ISR
处理其专属数据。若实际需要控制ISR同步调度，应另设计有界延后通道，不能声称
任意高优先级ISR都可重入核心。

现场布局不改：native在MSP，任务在PSP，高IRQ在MSP嵌套。非lazy FP硬件帧及软件
S16-S31保存继续保留，实测切换中抢占时整数/浮点现场。组代次、取消标记、仅池头
有序回收及静默覆盖契约不变。固定IRQ所属固件与CPU优先级是不同约束。

验证计划：ARM四配置及主机API拒绝/无副作用回归，然后真实core四个5秒10us
TIMER1窗口（空闲/双任务/启停/撤销重注册），记录漏期、最大入口间隔及整数/浮点
错误，保留旧失败证据。重跑公平性矩阵，不放宽2%阈值。绝对延迟、完整异常WCET、
外部示波器、Flash擦写并发及突然掉电需分别验证。
状态：设计已落盘；BASEPRI代码、API守卫与新实机结果尚未完成。

准入校验补充：在确认调用方全部mask清零且尚未发布active后，短暂以
BASEPRI_MAX=0xFF读回实际可实现的优先级位，再精确恢复；此探测不写NVIC优先级，
不包含遍历/复制，priority0可抢占。用于判断PendSV最低值、阈值有效及PRIGROUP对齐，
避免假设每个M均有4个优先级位或在运行准入扫描全部IRQ。

审核证据（原版未修改）：

| 项目 | 原版 srtos_m / E507 | 新版修复前 / 已有实测 |
|---|---|---|
| PendSV屏蔽 | gd32e50x_it.c:86–141，无整体cpsid/BASEPRI | m/port.c:405–472，PRIMASK覆盖全部switch |
| 复制 | section.c:611保存、653恢复；PendSV期间可被更紧迫IRQ抢占 | core保存/恢复在上述全局屏蔽事务内 |
| 完成临界区 | section.c:1126–1137，完成+tick全表扫描+inline；1202/1222保存恢复PRIMASK | 新版completion同样经port_lock，改为优先级阈值 |
| 周期激活 | section.c:1434–1438每个due任务短锁；tick扫描1450–1453 | 新版tick调用共享队列API，使用同一阈值 |
| 优先级 | systick.c:46 SysTick0，47 PendSV最低；SVC未找到明确设置 | 旧测试SVC40/SysTick80/PendSV F0/控制0，修复后SVC C0 |
| 浮点 | gd32e50x_it.c:98–114条件保存/恢复S16–31；platform.h:331关闭LSPEN | 同样非lazy、软件高寄存器，native MSP/任务PSP；旧实测整数/FP错误0 |
| 时长 | 本轮未测原版，不给臆测数值 | 旧实测四窗口入口间隔10.0056/38.5333/40.0833/38.5722us |

没有因ISR固定归属便假定它不能与调度并发。原版无PendSV全局锁也不等于任意ISR
可重入它的任务/FIFO API；本审核没有证明这类重入安全。异常栈故障关IRQ与正常
调度屏蔽分开记录。旧实机测的是整段入口间隔，不能反推单次memcpy的准确屏蔽时长。

原版ISR与队列关系：section.c:859的section_task_irq_exit_request只检查PendSV active、
调度已启动及时间片到期，然后pend一次切换（873）；不在ISR中操作ready FIFO或复制
现场。E507 SysTick在gd32e50x_it.c:145调用它。原版并无新版组启停/撤销API，因此
不能把原版ISR钩子可用等同于新版所有元数据接口都可从最高优先级ISR同步调用。

本阶段实现/测试进展：BASEPRI端口与15个状态API、stats_get守卫已同步实际D路径。
core SHA256前缀74de3e0b32b8，port前缀ce5807189228；正常调度不再写PRIMASK，
仅不可返回栈故障保留cpsid。实际源ARM4/4、主机18/18（新增83条fast-ISR检查）、
公平性小回归5/5、Shuttle桥接6/6通过；硬件重测尚在执行，不沿用旧PRIMASK结果。
C盘证据：srtos_stl_tests/build_priority_guard_cf7d2b9d/results.json、
fairness_regression/verify_74de3e0b32b8_48d46rhj/results.json、
managed_scaffold/host/evidence/run-jfolu27y/result.json。

下一阶段接口草案（计划，尚未实现）：真实Shuttle接入需要在保存/取消/出队前检查
resident servicing，繁忙则保留当前现场；在resume、新任务、inline换任务及native
返回四类路径中原子提交精确group-owner或NONE。还需sticky native-service请求：
先保存/排队当前任务，再回到MSP执行有界协议泵，阻止inline绕过请求，防止连续ready
任务饿死下载。所有hook与元数据驻留常驻区，不能持有可覆盖业务对象作为权威。
初始化native同步回调仍要求返回；旧Reset_Handler不可直接当managed入口使用，
原legacy初始化/下载超时保留，不能声称新managed初始化已具有可强制中止的超时。

Flash接入新发现（已审计、尚未修复/实测）：Shuttle bsp_update.c事务BASEPRI E0
不会屏蔽SysTick80；直接改80会屏蔽DMA完成IRQ D0且被dma_context_ready拒绝。
独立测试profile拟复用RAM DMA单字写并轮询DMA_INTF，避免完成IRQ依赖；busy期间
完整代码/常量/控制ISR必须驻RAM。默认生产驱动和协议不因该草案改变，错误/超时
也不能在FMC仍busy时返回Flash取指。此项不计为升级或不停机擦写已通过。

后续最小hook接口详细设计（仅在本地staging准备，当前D实机版本保持冻结）：
两个常驻weak hook为 section_stl_host_switch_allowed（默认1）和
section_stl_host_owner_commit(handle)（默认1，全零handle为NONE）；强实现仅可链接
常驻Shuttle代码，不能执行业务回调。gate在switch任何状态改变前检查，不允许时
原样返回当前帧。owner commit在新建/恢复/inline和所有native返回路径调用；提交
失败锁存CORRUPT并尝试clear，clear也失败则保持隔离，不能执行协议泵或覆盖。
新增 section_stl_request_native 状态API：仅active调度中接受，受同一ISR守卫保护；
sticky请求在当前现场成功保存并排队后返回MSP，inline不得越过请求。池耗尽时
保留当前现场/owner和请求，不能伪称native响应在任意池压力下都有界。
验证需覆盖gate无副作用、正常/inline/native owner顺序、提交失败隔离、native请求
保存/FIFO/池耗尽保留，以及新增API的fastIRQ拒绝。此段是计划，尚未上线。

BASEPRI实机首轮已测试：fixed_isr_local/runs/20261023T064115Z_bc728510/ASSESSMENT.md。
4个5秒窗口各500000/500000，漏期上下界均0；最大入口间隔依次为10.011111、
10.094444、10.094444、10.100000us，内部相对C入口lateness最大0.355556us。
该lateness以timer启动附近DWT为内部参考，不是独立绝对入口延迟。C主体最大
2.761111us含一次真实priority0 API/canary检查，其他窗口<=0.961111us；不含完整
硬件/编译器入口出口，不当作完整WCET。整数/浮点错误0，49次撤销/静默/遗忘成功，
所有窗口末ready/pool/record/fault为0。真实高IRQ API拒绝且输出canary不变。
Flash前后完整SHA256与原双备份一致，测试后原固件reset/Go running已记录；仍无
Flash擦写。native FP及完整公平性重测另行记录，不沿用旧版本PASS冒充本版本结果。

用户现已明确确认“控制 ISR 不调用这些调度接口”。高优先级ISR的调度表API限制
不再是待确认事项；读取已证明有界的时间值、处理专属控制数据继续允许。

### 最小真实Shuttle验证profile（计划，base外本地C工程）

为避免将压测变为重写/烧录整个Shuttle平台，首次组合profile将真实Shuttle core、
SUP解析、组桥接、SRTOS和RAM Flash驱动整体驻SRAM；业务镜像才通过既有协议写入
真实Flash。它不是Flash常驻内核的启动/掉电验证。调试器重新装载SRAM的reset恢复
必须单列，不能称为产品从Flash自启动。原固件完整双备份及每次碰Flash页的原内容
均保留；结束恢复所有改写页并核对完整原Flash哈希，不改保护、不整片擦除。

profile内存与此前生产容量草案不同：kernel代码+数据合计48KiB
0x20000000..0x2000BFFF；native MSP8KiB 0x2000C000..0x2000DFFF；mailbox4KiB
0x2000E000..0x2000EFFF；业务RAM0x2000F000..0x2001FFFF（68KiB）。两个业务槽各32KiB
从0x2000F000和0x20017000起，其余4KiB留边界检查。链接须断言各段边界，越界不得启动。
该独立布局不改Shuttle默认生产布局。Flash仍保护原kernel前64KiB；两个测试业务槽
拟从0x08010000、0x08020000起，catalog0x0807C000..0x0807FFFF沿用既有协议规则。

M独立platform只在boot初始化时钟/向量/FPU/优先级，两核心init后才启100us tick。
共享BASEPRI80，native MSP、任务PSP，新M独占SVC/PendSV；kernel request_dispatch
在active时请求安全返回native，schedule映射yield。原legacy context_create/
upgrade_context_create拒绝，不能跳旧Reset_Handler。普通managed任务不设预算；
旧初始化/维护/下载超时保留；新的任意managed INIT硬超时仍未实现，不能据此放行
无限初始化用例或宣称覆盖。Flash busy期间时间基准/超时若停滞须记录，不掩盖成实时保证。

仅此profile接受ABI tag0x53544C01：保持V3 144字节header、entry位置和SUP格式，
entry解释为返回24字节descriptor的函数。先由native内部prepare复制已验证header/slot
并复用load_data，再逐层检查entry、descriptor、records、wrapper、task/init/ctx指针、
尺寸与Thumb执行范围；resident拷贝后注册并运行受控返回INIT。生产默认兼容规则不改。
失败须回滚绑定/组/资源，不能留下可调度半组。测试邮箱经过同一REG_LINK解析链，
明确其物理传输是SWD而非UART。控制IRQ固定唯一业务所有者，不跨固件接力；其完整
busy期间调用链与数据在RAM。Flash驱动只在该profile采用RAM DMA标志轮询。

本阶段先产出可执行ELF和主机loader/owner测试，再由单一probe所有者做有停止条件
的运行；Flash真实写入前核对精确擦写范围、备份和恢复程序。硬件结果另行记录。

BASEPRI冻结版本轮硬件回归已完成（core74de3e0b32b8/portce5807189228）：
native FP证据srtos_stl_firmware/runs/20261023T064533Z_afd0b8ce，48项canary、32次
native FP及32次FP-ready撤销通过，MSP/PSP/CONTROL/PSPLIM/MSPLIM前后一致。
公平性证据fairness_180mhz_local/runs/20261023T064818Z_1cf42c57，36/36通过，
20个600us窗口最大启动偏差0.274485%、完成/CPU估计偏差0.249563%，低于预设2%；
row0启动4013/4002、完成4012/4002。600us校准601.833333–601.838889us。
200/450/mixed各4窗coalesced0；500us4窗合并780/768、未完成2/2（允许超载）；
所有窗口cleanup和delayed-tick为0。06:53:26已恢复原固件running、关闭probe，
三轮前后完整Flash哈希均为原1940e094...，无擦写/保护修改。

后续C staging hooks版（08b17855ed5a）只通过ARM4、主机18、公平性5及新hook11项；
实际Shuttle strong hooks主机9/9，ARM容量RAM15672/32768、Flash22244/65536，
均未部署D也未上板，不能把上述冻结版硬件PASS算作hooks版PASS。NONE clear也失败
时外部旧owner仍可能保留；core CORRUPT隔离，桥接必须阻止覆盖。ABI样例与polling
Flash profile只编译/静态检查通过，真实组合profile正在实现。

恢复设计新增确切约束：原固件reset PC=0x08016355，位于拟slotA，保护前64KiB并
不能保护原固件全部可执行代码。真实改写后不得直接Reset/Go原固件。必须先准备不
依赖原Flash入口的独立SRAM恢复stub和原512KiB可信备份，按8KiB页对照/恢复所有
改写页，整Flash哈希恢复为1940e094...后才恢复原固件运行。意外测试复位只允许
halt并重新装载SRAM恢复程序，不能因原resetvector仍在便假定原代码可执行。
07:02前短暂exec-server断连已恢复；断连期间未执行Flash写入，硬件一直原固件运行。

组合验证准备优化（先设计后实现）：原catalog确实有REG2/REGC有效旧记录；实际SUP
REMOVE先经旧镜像ABI/维护准入，新profile会正确拒绝，不能把拒绝伪装成已删除。
独立SRAM recovery fixture已编译（无Shuttle/catalog/SRTOS），拟增加明确command2
ERASE_TEST_CATALOG：仅硬编码0x0807C000和0x0807E000两个8KiB页，擦除并全页FF读回，
不使用主机任意地址。冷启动该fixture本来没有任务/IRQ所有者，避免在普通runtime
另加可重入诊断服务。主机必须先验证原完整可信备份及目标Flash一致才执行；此为
明确记录的测试准备，不当作SUP删除测试。完成后ResetHalt重装正常SRAM ELF清BSS，
再走真实SUP下载、提交、激活。最终cmd1逐页恢复原内容，整Flash哈希匹配才原固件Go。
cmd2不改保护/安全、不整片擦除，正常生产协议和驱动默认不变。

模块负载设计修正：早期C编译样例误用REG_TASK(6)及DWT墙钟等待，尚未运行；必须
改为period10（1ms）及预校准固定工作迭代，校准目标600us±2%。被抢占的墙钟差只
用于诊断，不当作CPU执行量；保留早期compile-only产物，禁止计作公平性通过证据。


## Periodic task fairness correction (2026-10-24; verified in current snapshot)

The configured quantum remains10 ticks (1ms on E507). Remaining quantum is now
owned by each resident task, rather than sharing the residue of a global phase
across callback boundaries. Completed callbacks and voluntary yields forfeit
unused credit. A native-only preemption preserves remaining credit and resumes
that saved continuation before rotating competitors, unless its budget expired.
At least one tick is charged to a zero-tick native interruption; repeated early
requests therefore cannot renew a task forever. IRQ time is charged to the
interrupted task's wall-clock quantum; calibrated CPU work is a separate metric.

No group CPU weights, extra group table, ordinary execution timeout, CPU-port
change or relaxed release gate. Equal-work tasks are the fairness target; a
firmware with more ready tasks is not promised equal total CPU to a smaller one.
The actual-core model counterexample changed from1250/1250/2499/2499 to
1875/1874/1874/1875;30 phase/native-frequency/long-task scans,18 previous core cases,
M3/M4/M33 compilation and zero-tick80-request40/40 rotation passed. These initial software/model results are now supplemented by the current hardware
and fresh user-workflow evidence above.
