# Zynq-7020 3P3Z IIR PL 数字验证

## 1. 验证结论

2026-07-17 完成 3P3Z IIR 的纯 PL 数字验证。RTL 数值、流水线握手、AXI4-Lite 访问、DSP 映射、DRC 和 50MHz 时序均通过，可以接入 PS 软件和板上测试。

| 项目 | 结果 |
|------|------|
| 独立 core 参考模型比对 | 322 个样点，0 mismatch |
| AXI4-Lite 数值与协议测试 | 10 个样点，0 mismatch |
| 固定完成延迟 | 13 个 PL 时钟 |
| 独立 core 综合 | 0 warning，0 error |
| IIR 可执行 DRC | 0 |
| 50MHz OOC WNS/TNS | +15.599ns / 0ns |
| IIR 资源 | 373 LUT、634 寄存器、4 DSP48E1 |
| 完整 PS+PL 实现 DRC | 0 |
| 完整 PS+PL 实现日志 | 0 warning，0 critical warning，0 error |
| 完整 PS+PL WNS/TNS | +13.019ns / 0ns |

OOC DRC 报告保留 `ZPS7-1`，原因是独立 IIR core 顶层不包含 Zynq PS7。验证脚本只豁免这一条；完整 PS+PL 实现中 DRC 为 0。

## 2. 数值模型

RTL 与 testbench 使用相同的定点边界、不同的实现路径逐样点比较：

```text
y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
       - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
```

- 系数：有符号 Q2.30
- 样点：有符号 32 位整数
- 累加：有符号 70 位
- 缩放：算术右移 30 位
- 输出：饱和到 `INT32_MIN..INT32_MAX`
- 反馈：使用饱和后的输出历史

独立参考模型直接计算 7 个 64 位乘积和 70 位和式；RTL 使用 4 个 DSP48E1 组成的流水线 32×32 位乘法器，连续发出 7 个乘积后顺序累加。

## 3. 向量覆盖

### 3.1 独立 core：322 个样点

| 用例 | 样点数 | 覆盖内容 |
|------|------:|----------|
| `multiplier_corner` | 64 | 8 个输入边界 × 8 个系数边界，覆盖正负高/低部分积 |
| `continuous_prbs` | 256 | 混合正负 3P3Z 系数、连续状态反馈、历史寄存器 |
| 正/负饱和 | 2 | `INT32_MAX`、`INT32_MIN` 与饱和状态 |

测试同时检查 `ready/busy/done`、完成延迟、`sample_count` 和 X/Y 历史。结果中 8 个样点触发饱和，全部与参考模型一致。

### 3.2 AXI：10 个样点

AXI testbench 验证：

- AW 与 W 同时到达
- AW 先于 W 到达
- W 先于 AW 到达
- `WSTRB` 字节写
- VERSION、FORMAT、系数、状态、样点计数和历史寄存器
- 8 点已知脉冲响应
- 正、负饱和

脉冲响应为：

```text
524288, 131072, 32768, 8192, -14336, 512, 1152, 544
```

## 4. 结果文件

- 独立 core 全量数据：`verilog/iir/sim/iir_3p3z_core_numeric_results.csv`
- AXI 数值数据：`verilog/iir/sim/iir_3p3z_numeric_results.csv`
- 独立 core testbench：`verilog/iir/sim/tb_iir_3p3z_core.sv`
- AXI testbench：`verilog/iir/sim/tb_axi_iir_3p3z.sv`
- OOC 报告：`build/verilog_synth/`
- 完整实现报告：`platform/zynq7020/pl/build/output/`

## 5. 复现命令

```powershell
cd D:\OneDrive\LWX\GD32\base
.\verilog\iir\sim\run_sim.ps1
.\verilog\iir\sim\run_synth.ps1
.\platform\zynq7020\pl\build_pl.ps1
```

通过标志：

```text
CORE_SIM_RESULT PASS vectors=322 latency=13 failures=0
SIM_RESULT PASS vectors=10 failures=0
PL_CORE_SYNTH_RESULT status=PASS actionable_drc=0 ... wns_ns=15.599 dsp=4
PL_BUILD_RESULT status=PASS ...
```
