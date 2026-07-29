# AXI 3P3Z IIR 使用说明

## 1. 文件

rtl/iir_3p3z_core.v
    单周期 3P3Z IIR 运算 core、并行乘法器、上下限幅和历史状态。

rtl/axi_iir_3p3z.v
    AXI4-Lite 从设备、32 位寄存器组和 IIR core 封装。

sim/tb_iir_3p3z_core.sv
    不经过 AXI 的独立参考模型逐样点比对。

sim/tb_axi_iir_3p3z.sv
    AXI AW/W 独立握手、WSTRB、寄存器和数值测试。

## 2. 数学格式

y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
       - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]

所有系数为有符号 Q2.30。输入和输出为有符号 32 位整数，工程量
比例由调用方选择。内部使用 70 位累加器，结果算术右移 30 位后限幅到
LIMIT_LOWER..LIMIT_UPPER。反馈历史保存最终限幅后的输出。

## 3. core 接口

ready 始终为 1，busy 始终为 0。start 在上升沿被接受时，sample_out、
saturated、sample_count 和 X/Y 历史在该上升沿同步更新，固定完成延迟为
1 个 clk。连续保持 start 有效时，每个 clk 完成一个样点。

clear_state 的优先级高于 start，并清除输入/输出历史、计数和输出。

## 4. AXI4-Lite 寄存器

基地址由 SoC 地址编辑器决定；Zynq-7020 平台使用 0x43C00000。

偏移  名称          访问  内容
0x00  CONTROL       WO    bit0=start, bit1=reset_state, bit2=clear_done
0x04  STATUS        RO    bit0=busy, bit1=done, bit2=saturated, bit3=ready
0x08  INPUT         RW    有符号 32 位输入样点
0x0C  OUTPUT        RO    有符号 32 位输出样点
0x10  B0            RW    Q2.30 b0
0x14  B1            RW    Q2.30 b1
0x18  B2            RW    Q2.30 b2
0x1C  B3            RW    Q2.30 b3
0x20  A1            RW    Q2.30 a1
0x24  A2            RW    Q2.30 a2
0x28  A3            RW    Q2.30 a3
0x2C  SAMPLE_COUNT  RO    完成样点数
0x30  VERSION       RO    0x00020000
0x34  FORMAT        RO    0x0000201E
0x38  X1            RO    x[n-1]
0x3C  X2            RO    x[n-2]
0x40  X3            RO    x[n-3]
0x44  Y1            RO    y[n-1]
0x48  Y2            RO    y[n-2]
0x4C  Y3            RO    y[n-3]
0x50  LIMIT_LOWER   RW    有符号输出下限，复位值0x80000000
0x54  LIMIT_UPPER   RW    有符号输出上限，复位值0x7FFFFFFF

AW 和 W 通道可以独立先到，写数据支持 WSTRB。AXI done 为粘滞位，下一次
start、reset_state 或 clear_done 会清除。

## 5. 软件操作顺序

1) 写 B0/B1/B2/B3/A1/A2/A3。
2) 写 LIMIT_LOWER 和 LIMIT_UPPER，保证 lower <= upper。
3) 写 CONTROL.reset_state。
4) 写 INPUT。
5) 写 CONTROL.start。
6) 读取 OUTPUT 和 STATUS.saturated。
7) 写 CONTROL.clear_done，或直接启动下一样点。

PS 可以像访问 MCU 外设寄存器一样直接操作任意已定义偏移：

    bsp_iir_write_register(BSP_IIR_B0_OFFSET, 0x20000000U);
    bsp_iir_write_register(BSP_IIR_INPUT_OFFSET, (uint32_t)input);
    bsp_iir_write_register(BSP_IIR_CONTROL_OFFSET,
                           BSP_IIR_CONTROL_START);
    status = bsp_iir_read_register(BSP_IIR_STATUS_OFFSET);

`bsp_iir_configure()`、`bsp_iir_limit_configure()`、
`bsp_iir_process_sample()` 和 `bsp_iir_self_test()` 是在上述 32 位 MMIO
接口之上的便捷封装。`bsp_iir_limit_configure()` 会拒绝上下限倒置。

## 6. 验证

从仓库根目录执行：

    .\verilog\iir\sim\run_sim.ps1
    .\verilog\iir\sim\run_synth.ps1

run_sim.ps1 会生成：

    sim/iir_3p3z_core_numeric_results.csv
    sim/iir_3p3z_numeric_results.csv

run_synth.ps1 使用 Vivado 2018.3 对 XC7Z020 做 50MHz OOC 综合，任何 IIR
逻辑 DRC 或负时序裕量都会使脚本失败。ZPS7-1 是独立 PL core 不含 PS7 的
唯一允许项，完整 PS+PL 构建中该项必须为零。

## 7. 增加自定义 32 位寄存器

增加寄存器时按同一个字节偏移同步修改：

1) rtl/axi_iir_3p3z.v 的 AXI 读译码；可写寄存器还要加入写译码和 WSTRB。
2) platform/zynq7020/pl/package_axi_iir_ip.tcl 的 IP-XACT 寄存器描述。
3) platform/zynq7020/ps/bsp/bsp_iir.h 的偏移宏和需要的驱动接口。
4) sim/tb_axi_iir_3p3z.sv 的读写、复位值和访问属性测试。

地址必须 4 字节对齐。当前 AXI 地址宽度为 8 位，寄存器窗口为 256 字节；
IP-XACT 为该外设保留 4KB 地址段。只读状态由 PL 产生，可写配置由 PS 通过
AXI 写入，PL 在 start 被接受时锁存当前输入和系数。

## 8. 关联导航

- 源码：[IIR Core](../../rtl/iir_3p3z_core.v) · [AXI IIR 顶层](../../rtl/axi_iir_3p3z.v) · [Zynq IIR BSP](../../../../platform/zynq7020/ps/bsp/bsp_iir.c)
- 设计：[AXI 3P3Z IIR 设计](../design/axi_iir_3p3z_design.md)
