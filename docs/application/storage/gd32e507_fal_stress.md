# GD32E507 FAL 板上压力测试

## 测试计划与通过标准

测试对象为 GD32E507 的 NOR、NAND 和当前 FAL Demo。擦写限定在设备 test 分区，参数与日志分区不作为写入目标。使用匹配固件的 ELF 定位调试符号，串口不可用时经 SWD 调用或发布请求，分别记录所覆盖的入口。

| 编号 | 用例 | 数量与通过标准 |
|---|---|---|
| S01 | 下载及设备识别 | 固件校验通过，双设备 ID 正确，初始化与驱动结果正常 |
| S02 | NOR 连续擦写读回 | 100 次 test 事务，每次请求号与完成号相等、busy=0、result=0 |
| S03 | NAND 连续擦写读回 | 100 次 test 事务，结果同 S02，记录 ECC 与坏块诊断 |
| S04 | 双设备交替访问 | 50 轮 NOR→NAND，共 100 次，逐次完成且无错误 |
| S05 | 忙时拒绝新命令 | 对活动 Demo 事务提交第二个请求，拒绝且原事务完成 |
| S06 | NAND 底层重复请求 | 实际读/写活动期间重复提交，原指针与状态保持且原请求完成 |
| S07 | Core 分区边界与权限 | 越界、保护区擦写及 NAND 不对齐写入被拒绝，不发起介质操作 |
| S08 | 独立实例交错推进 | 两个实例同时持有请求，一片忙时另一片仍有进展并最终成功 |
| S09 | 软件复位后复测 | 5 轮复位，每轮初始化正常并各执行 1 次双设备 test 事务 |
| S10 | 回归与交付检查 | 主机回归、相关构建、变更边界和版本检查通过后分批提交并推送 |

单次事务设置有限等待时间，任何错误、超时、诊断异常均记为失败并停止该批，不通过无限重试掩盖失败。发现问题后保留失败证据，修复后重新执行受影响用例。

本计划不将软件复位等同于物理断电，不将无错读回等同于 ECC 位错误注入，不包含耐久寿命认证。真实断电、外部故障注入和全芯片磨损需要独立条件。

## 复现入口

板上测试任务位于 `platform/gd32e507/tests/fal_board_test.c/.h`，使用独立缓冲区和真实业务/Core/Interface API。默认构建不包含测试任务。测试构建中也必须显式写入 `g_fal_board_report.command` 才会启动。

在仓库根目录构建专用固件：

```powershell
mingw32-make -C platform/gd32e507 -s -j8 FAL_BOARD_TEST=1 BUILD_DIR=build/fal_stress
```

通过 J-Link 下载 `platform/gd32e507/build/fal_stress/gd32e507_demo.hex` 并复位运行，下载目标设为 GD32E507ZE、SWD、1000 kHz。确认没有其他调试器持有连接，然后执行：

```powershell
python platform/gd32e507/tests/run_fal_stress.py --elf platform/gd32e507/build/fal_stress/gd32e507_demo.elf --output tests/host/demo_storage/board_stress.jsonl --confirm-test-partition-erase
```

观察脚本使用 ELF 中的报告符号，不硬编码 RAM 地址；默认 J-Link 序列号为本次实测的 69409716，可通过 `--serial` 显式指定。脚本依次执行完整批次和 5 轮复位复测，失败即停止，保存进度和最终计数。使用的 ELF 必须与刚下载的测试固件匹配。

任务每个事务设置 30 秒上限，主机单批设置 15 分钟上限。`status=2` 且各用例计数满足计划才判定通过。`status=3` 时保留第一个失败编号、预期值和实际值；没有自动重试或自动清错。

主机回归入口：

```powershell
mingw32-make -C platform/testbench/fal_core -s -B test
mingw32-make -C platform/testbench/fal_demo -s -B test
powershell -NoProfile -ExecutionPolicy Bypass -File platform/testbench/fal_demo/test_nand_request.ps1
```

上述 Makefile 默认使用本机 `C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe`，可通过 make 的 `CC=` 参数指定兼容编译器。NAND 请求入口脚本同样使用该本机编译器。

测试结束后，通过默认 `platform/gd32e507/download.bat` 恢复正常固件。压力测试是显式擦写操作，test 分区原内容会被覆盖，脚本不提供数据备份。

## 执行结果

2026-09-05，J-Link 69409716，SWD 1000 kHz。正式批次 `board_stress_target_02` 用时约 507 秒，完整批次和 5 轮软件复位均通过。

| 用例 | 实测结果 |
|---|---|
| S01 | 下载校验通过；NOR ID `0x00C84015`，NAND ID `0xC8F1801D` |
| S02 / S03 | NOR 100/100，NAND 100/100 |
| S04 | 50 轮双设备交替，100/100 事务 |
| S05 | 两设备各 1 次忙时拒绝，原事务均成功 |
| S06 | 重复编程、重复读取各 1 次拒绝；原数据读回一致，被拒读缓冲区保持哨兵值 |
| S07 | 7 项权限、范围与对齐拒绝检查通过 |
| S08 | NAND 在 NOR 仍忙时完成读取，随后两实例均成功且数据一致 |
| S09 | 5/5 轮软件复位，每轮 NOR、NAND 各 1 次事务，共 10 次 |
| S10 | 主机 Core 61、配置 63、Demo/BCH 13917、NAND 请求入口 40 项检查全部通过；GD32E507 默认与测试构建、HC32F334 GCC Bootloader、Zynq SDK 2018.3 Bootloader 构建通过 |

累计 312 次 Demo test 事务通过，另有底层重复请求和 Core 边界/独立推进检查。正式批次 `failure=0`、`corrected=0`、`bad_blocks=0`；最长 Demo 事务为 3024100 μs（含任务调度与 SWD 观察影响，不作为实时性能上限）。5 轮复测各自最大事务耗时为 2412400、2294300、2437300、2638200、2381500 μs。

固件与证据 SHA-256：

- 测试 ELF：`e5dd075ad09a274da894ed5d219ce7442dd539f7a03c7dc0fbbe55f7a50bd0e1`
- 测试 HEX：`252913d955c54b2293e1bbb3483e130e38676168164d0be1308b7dedd5b416bd`
- 本地完整 JSONL：`258ed674af1558750c5e057256288b2eab71af1cc923a57b83cbad6e7231c80d`

正式结果摘要见 [机器可读结果](gd32e507_fal_stress_result.json)。预检曾遇到 GDB 函数调用方式不适用，以及测试把 Core 接受请求的返回值误写为 SUCCESS；改用正常任务执行并将期望修正为 IN_PROGRESS 后，从零重跑正式批次，预检计数不计入结果。

额外构建检查中，PLECS Buck 当前缺少 `plecs_dispatch_lock.c` 导致链接失败，未改动该工程；包含公共版本信息的 PLECS frame_route_bridge 双节点构建通过。Keil 工程仅检查 XML 与迁移路径，未执行 Keil 编译。

已通过 `download.bat` 恢复默认固件并校验通过，复查双设备 ID 正确、驱动错误为 0、两个 FAL 实例均空闲。默认 HEX SHA-256 为 `dfdf9a904e4420084f00f1a57d4ac91e606956237b67eece2ba40e7f55720a3b`。

串口命令链路、真实断电、真实坏块、硬件 ECC 位翻转注入及寿命测试不在本次通过范围。板上仅擦写 test 分区，原测试内容已被覆盖且未备份。
