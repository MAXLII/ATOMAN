# Zynq-7020 3P3Z IIR PL 数字验证

## 1. 验证结论

2026-07-25 完成单周期 3P3Z IIR 的数字验证、OOC 综合、完整平台实现及板级验证。RTL 版本为 `0x00020000`。

| 项目 | 结果 |
|---|---|
| 独立 core 参考模型比对 | 325 个样点，0 mismatch |
| AXI4-Lite 数值与协议测试 | 13 个样点，0 mismatch |
| 固定完成延迟 | 1 个 PL 时钟 |
| 连续吞吐量 | 每个 PL 时钟 1 个样点 |
| 上下限及限幅历史 | 上限、下限、`y[n-1]` 反馈全部 PASS |
| IIR 可操作 DRC | 0 |
| 50 MHz OOC WNS/TNS | +4.290 ns / 0 ns |
| IIR OOC 资源 | 7625 LUT、226 寄存器、0 DSP |
| 完整 PS+PL 可操作 DRC | 0 |
| 完整 PS+PL WNS/TNS | +0.272 ns / 0 ns |
| 完整 PS+PL 资源 | 11317 LUT、4075 寄存器、0 DSP |
| baremetal 板测 | PASS |
| A9 SRTOS 板测 | PASS，fault/save_fail/release_fail 均为 0 |

OOC 报告仅保留不含 PS7 顶层产生的 `ZPS7-1`。完整平台报告保留 AXI Interconnect 内部无可路由负载的厂商 `RTSTAT-10` 豁免；两者的可操作 DRC 均为 0。

## 2. 数值模型

RTL 与 testbench 使用相同的定点边界、不同的实现路径逐样点比较：

```text
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
       - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
```

- 系数：有符号 Q2.30。
- 样点：有符号 32 位整数。
- 乘法：7 个并行 32×32 位有符号组合乘法器。
- 累加：有符号 70 位平衡加法树。
- 缩放：算术右移 30 位。
- 输出：限幅到 `LIMIT_LOWER..LIMIT_UPPER`。
- 反馈：使用最终限幅后的输出历史。

## 3. 向量覆盖

### 3.1 独立 core：325 个样点

| 用例 | 样点数 | 覆盖内容 |
|---|---:|---|
| `multiplier_corner` | 64 | 8 个输入边界 × 8 个系数边界 |
| `continuous_prbs` | 256 | 混合正负系数、连续状态反馈、历史寄存器 |
| 32 位正/负饱和 | 2 | 默认上下限下的 `INT32_MAX`、`INT32_MIN` |
| 配置限幅与反馈 | 3 | 上限、下限和解除限幅后的 `y[n-1]` 反馈 |

另有 8 个连续时钟启动样点，验证每周期接受和完成一个样点。测试同步检查 `ready/busy/done`、固定单周期延迟、`sample_count` 和 X/Y 历史。

### 3.2 AXI：13 个样点

AXI testbench 验证：

- AW 与 W 同时到达、AW 先到、W 先到。
- `WSTRB` 字节写。
- VERSION、FORMAT、系数、上下限、状态、计数和历史寄存器。
- 8 点已知脉冲响应。
- 正、负 32 位饱和。
- 配置上限、配置下限以及限幅值进入 `y[n-1]`。

脉冲响应为：

```text
524288, 131072, 32768, 8192, -14336, 512, 1152, 544
```

## 4. 板级结果

PL JTAG 自检和 COM6 `IIR_TEST` 均验证：

```text
iir_limits=0x00000064/0xFFFFFFB5/0x00000064
iir_version=0x00020000
```

三个数值分别表示上限结果 `100`、下限结果 `-75` 和使用限幅 `y[n-1]` 得到的反馈结果 `100`。baremetal 与 A9 SRTOS 均返回 `BOARD_IIR_SELFTEST result=PASS`。

## 5. 复现命令

```powershell
cd D:\OneDrive\LWX\GD32\base
.\verilog\iir\sim\run_sim.ps1
.\verilog\iir\sim\run_synth.ps1
.\platform\zynq7020\pl\build_pl.ps1
.\platform\zynq7020\ps\compile.ps1 -Srtos 0
.\platform\zynq7020\ps\compile.ps1 -Srtos 1
.\platform\zynq7020\ps\board_iir_selftest.ps1 -Srtos 0
.\platform\zynq7020\ps\board_iir_selftest.ps1 -Srtos 1
```

通过标志：

```text
CORE_SIM_RESULT PASS vectors=325 latency=1 failures=0
SIM_RESULT PASS vectors=13 failures=0
PL_CORE_SYNTH_RESULT status=PASS actionable_drc=0 period_ns=20.000 wns_ns=4.290 dsp=0
PL_BUILD_RESULT status=PASS
BOARD_IIR_SELFTEST result=PASS port=COM6 baud=921600 mode=baremetal
BOARD_IIR_SELFTEST result=PASS port=COM6 baud=921600 mode=srtos-a9
```
