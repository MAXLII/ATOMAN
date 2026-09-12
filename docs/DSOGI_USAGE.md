# DSOGI 独立库与 PLL 联调

## 资料与范围

依据本地资料目录 `D:/OneDrive/LWX/Book/DSOGI/`：

- `01_DSOGI_PLL_Review_2021.pdf`，第 3.1、3.2 节，印刷页 247–248，式 (3)、(5)、(6)：SOGI 传递函数及正负序计算。
- `02_Imperix_SOGI_PLL_TN104.pdf`：SOGI 原理、离散实现及三相扩展。
- `05_PLL_SOGI_Study_Notes_Chinese.md`：同步信号链及模块边界。

`code/lib/dsogi.c/.h` 复用现有 `sogi.c/.h` 的两个 Tustin SOGI，仅负责 αβ 滤波和正负序分离。DSOGI 不依赖 PLL；PLL、Clarke 变换、频率反馈滤波、锁定检测由调用方独立组织。不修改 NPC 或 PLECS 模型。

## 符号与算法

输入为幅值不变 Clarke 坐标，正序约定为：

```text
alpha = A*cos(theta)
beta  = A*sin(theta)
```

两个 SOGI 分别产生同相输出 a、b 和滞后 90° 的正交输出 qa、qb：

```text
D(s) = k*omega*s / (s*s + k*omega*s + omega*omega)
Q(s) = k*omega*omega / (s*s + k*omega*s + omega*omega)

alpha_pos = (a - qb)/2
beta_pos  = (b + qa)/2
alpha_neg = (a + qb)/2
beta_neg  = (b - qa)/2
```

复用 SOGI 的双线性离散系数，不进行频率预畸变；因此数字中心频率与连续域目标存在小偏差。频率变化时保留历史，仅更新两轴系数。典型参数为采样 5 kHz、中心频率 50 Hz、k=√2。

传统 Q 通道不能完全抑制直流偏置；DSOGI 也不是理想基波滤波器。零序无法从两轴输入恢复，纯负序输入没有可供正序 PLL 锁定的参考。实际并网系统仍需独立的幅值/失压、相序、锁定与保护判断。

## 输入、配置与输出

- `dsogi_input_t`：绑定 `p_alpha`、`p_beta`、`p_omega`；电压单位 V，角频率单位 rad/s。
- `dsogi_cfg_t`：`ts`、`k`、`omega_min`、`omega_max`。
- `dsogi_inter_t`：两个 SOGI 的运行状态及初始化标记。
- `dsogi_output_t`：`alpha_pos`、`beta_pos`、`alpha_neg`、`beta_neg`，均为 V。

配置数值边界：ts∈[1e-6,0.01] s，k∈[0.1,4]，1≤omega_min≤omega_max≤100000 rad/s，且 0.001≤omega_min×ts≤omega_max×ts≤1。这些是当前 float/Tustin 实现的保守数值边界，不是所有参数组合的闭环性能保证。

`dsogi_init()` 绑定并初始化；`dsogi_reset()` 清历史和输出，保留配置与绑定；`dsogi_cal()` 每周期运行一次，返回 bool。初始化失败会将对象置为无效；运行期间无效电压、频率越界或算术溢出会返回 false 并清历史和输出，恢复有效输入后可继续运行。

外部输入必须在一次调用期间保持稳定，生命周期覆盖实例使用期间；不得指向 DSOGI 实例内部。实例初始化后不要复制为另一个实例，不要直接修改运行状态或配置；重配置使用 init。PLL 内部亦存在自引用绑定，初始化后不要复制 PLL 实例。

## 与现有 PLL 组合

```c
float alpha = 0.0f;                    /* Measured alpha voltage, V. */
float beta = 0.0f;                     /* Measured beta voltage, V. */
float omega = 314.159265f;             /* External SOGI tuning source, rad/s. */
dsogi_t dsogi = {0};                   /* Caller-owned sequence extractor. */
pll_t pll = {0};                       /* Independent PLL. */
const dsogi_cfg_t cfg = {              /* 5 kHz sample, approximately 40..63 Hz tuning. */
    .ts = 0.0002f,
    .k = 1.41421356f,
    .omega_min = 250.0f,
    .omega_max = 400.0f
};

/* Check both return values before entering periodic execution. */
bool ready = dsogi_init(&dsogi, &cfg, &alpha, &beta, &omega);
if (ready == true)
{
    ready = pll_init(&pll, cfg.ts, omega, 400.0f, 250.0f,
                     300.0f, 0.70710678f, 125.663706f,
                     85.0f, -64.0f, 0.0f,
                     &dsogi.output.alpha_pos, &dsogi.output.beta_pos);
}

/* Once per 200 us, after refreshing alpha and beta: */
if (ready == true)
{
    omega = pll.output.omega; /* Previous sample: avoids an algebraic loop. */
    if (dsogi_cal(&dsogi) == true)
    {
        ready = pll_cal(&pll);
    }
    else
    {
        /* Caller handles invalid feedback and decides when to resume synchronization. */
        ready = false;
    }
}
```

其中 PLL 的 vm 是标称正序电压峰值，示例为 300 V；应按实际系统调整调谐参数。该组合使用直接频率反馈，资料中的频率反馈低通滤波器可由外部按需求增加，并非 DSOGI 内部职责。

当前 PLL 的 `output.theta` 是本次频率积分后的下一采样点相角，联调测试按该时间语义比较。联调发现旧 `pll.c` 将 vq 作为 PI 的 act，而 PI 使用 ref−act，导致正序锁到约 180°；本次改为 act=−vq，使正 vq 增加估计角度。已有调用方若曾在外部补偿该错误符号，需取消相应补偿。

## Testbench

目录：`platform/testbench/dsogi/`。使用公共注册式 testbench，每例 10000 点、5 kHz、2 s，共 10 例和 100000 个周期；链接真实 dsogi、sogi、pll、pi_tustin，不替换算法。

```powershell
$env:PATH = 'C:\mingw64\bin;' + $env:PATH
Set-Location D:\OneDrive\LWX\GD32\base\platform\testbench\dsogi
mingw32-make test
```

10 个用例：纯正序、纯负序、混合序、平衡 PLL、不平衡 PLL、50→55 Hz、0.4 rad 相位跳变、幅值跌落到 40%、5/7 次谐波、非法输入及恢复。另验证空指针、无效配置、数值溢出和实例隔离。动态扰动发生在 0.5 s，稳态误差统计窗口为最后 0.3 s；不将此统计解释为暂态峰值或严格锁定时间。

当前 GCC 严格警告构建下全部通过：

- 无谐波用例最大序分量误差：约 0.062%，以 300 V 为基准。
- 无谐波 PLL 最大稳态相角误差：约 0.000584 rad（0.034°）。
- 无谐波 PLL 最大稳态频率误差：约 0.00123 Hz。
- 5% 五次 + 3% 七次谐波输入：最大序分量误差约 1.10%，相角误差约 0.000730 rad，频率误差约 0.0644 Hz。

`build/case_0.csv` 至 `build/case_9.csv` 按上述顺序输出各例波形，每 5 个控制点记录一次。公共运行器另修正了 linker 数组边界的显式地址比较，以通过当前 GCC 的 -Werror 构建。

验证范围为主机算法和注册式 testbench，不包含 MCU 耗时、RAM 放置或实际并网硬件验证。

