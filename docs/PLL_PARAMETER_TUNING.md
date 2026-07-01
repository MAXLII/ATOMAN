# PLL 参数整定推导

## 1. 当前 PLL 结构

当前 `code/lib/pll.c` 实现的是三相 SRF-PLL。输入为三相电压经过等幅 Clarke 变换后的 $\alpha / \beta$：

$$
v_\alpha,\quad v_\beta
$$

PLL 内部计算流程为：

$$
v_\alpha, v_\beta
\xrightarrow{\text{Park}(\hat{\theta})}
v_d, v_q
\xrightarrow{e = 0 - v_q}
\text{PI}
\xrightarrow{}
v_f
$$

$$
\hat{\omega} = \omega_0 + v_f
$$

$$
\hat{\theta} = \int \hat{\omega}\,dt
$$

其中：

$$
\omega_0 = \omega_{\text{center}}
$$

代码中的 PI 输入方向为：

```c
p_pll->inter.pi_ref = 0.0f;
p_pll->inter.pi_act = p_pll->inter.vq;
```

因此 PI 误差为：

$$
e = \text{ref} - \text{act} = 0 - v_q = -v_q
$$

## 2. Park 变换约定

`my_math.h` 中的 `DQ_CAL` 宏为：

```c
d = costheta * a + sintheta * b;
q = sintheta * a - costheta * b;
```

令：

$$
a = v_\alpha,\quad b = v_\beta,\quad \theta = \hat{\theta}
$$

则：

$$
v_d = \cos\hat{\theta}\cdot v_\alpha + \sin\hat{\theta}\cdot v_\beta
$$

$$
v_q = \sin\hat{\theta}\cdot v_\alpha - \cos\hat{\theta}\cdot v_\beta
$$

三相平衡电压经过等幅 Clarke 变换后，可写成：

$$
v_\alpha = V_m\cos\theta_g
$$

$$
v_\beta = V_m\sin\theta_g
$$

其中：

$$
V_m
$$

为 $\alpha / \beta$ 平面中的电压峰值。

$$
\theta_g
$$

为电网真实相位。

$$
\hat{\theta}
$$

为 PLL 估计相位。

代入 $v_q$：

$$
v_q
= \sin\hat{\theta}\cdot V_m\cos\theta_g
- \cos\hat{\theta}\cdot V_m\sin\theta_g
$$

提取 $V_m$：

$$
v_q
= V_m\left(
\sin\hat{\theta}\cos\theta_g
- \cos\hat{\theta}\sin\theta_g
\right)
$$

利用三角恒等式：

$$
\sin(A-B)=\sin A\cos B-\cos A\sin B
$$

得到：

$$
v_q = V_m\sin(\hat{\theta}-\theta_g)
$$

## 3. 相位误差定义

定义 PLL 相位误差：

$$
\delta = \theta_g - \hat{\theta}
$$

因此：

$$
\hat{\theta}-\theta_g = -\delta
$$

代入 $v_q$：

$$
v_q = V_m\sin(-\delta)
$$

因为：

$$
\sin(-x) = -\sin x
$$

所以：

$$
v_q = -V_m\sin\delta
$$

锁相附近 $\delta$ 很小，可做小信号线性化：

$$
\sin\delta \approx \delta
$$

因此：

$$
v_q \approx -V_m\delta
$$

PI 误差为：

$$
e = -v_q
$$

所以：

$$
e \approx V_m\delta
$$

这一步很关键：当前代码中，PI 看到的误差信号与相位误差 $\delta$ 同号，并且被电压幅值 $V_m$ 放大。

## 4. 从相位误差到频率误差

相位误差定义为：

$$
\delta = \theta_g - \hat{\theta}
$$

对时间求导：

$$
\frac{d\delta}{dt}
= \frac{d\theta_g}{dt} - \frac{d\hat{\theta}}{dt}
$$

记：

$$
\dot{\delta} = \frac{d\delta}{dt}
$$

角度对时间的导数就是角频率：

$$
\frac{d\theta_g}{dt} = \omega_g
$$

$$
\frac{d\hat{\theta}}{dt} = \hat{\omega}
$$

因此：

$$
\dot{\delta} = \omega_g - \hat{\omega}
$$

这一步只是相位误差定义的直接结果。

## 5. 写出 $\omega_g$ 和 $\hat{\omega}$

PLL 使用中心角频率：

$$
\omega_0 = \omega_{\text{center}}
$$

真实电网角频率可写成：

$$
\omega_g = \omega_0 + \Delta\omega
$$

其中 $\Delta\omega$ 表示电网真实角频率相对中心角频率的偏差。

例如中心频率是 $50\,\text{Hz}$，真实频率是 $50.2\,\text{Hz}$：

$$
\omega_0 = 2\pi\cdot 50
$$

$$
\omega_g = 2\pi\cdot 50.2
$$

$$
\Delta\omega = 2\pi\cdot 0.2
$$

PLL 估计角频率由代码给出：

```c
p_pll->output.omega = p_pll->cfg.omega_center + p_pll->inter.vf;
```

数学形式为：

$$
\hat{\omega} = \omega_0 + v_f
$$

代入：

$$
\dot{\delta} = \omega_g - \hat{\omega}
$$

得到：

$$
\dot{\delta}
= (\omega_0 + \Delta\omega) - (\omega_0 + v_f)
$$

消去相同的 $\omega_0$：

$$
\dot{\delta} = \Delta\omega - v_f
$$

这句话的物理含义是：

$$
\text{相位误差变化速度}
=
\text{电网频率偏差}
-
\text{PLL 输出的频率修正量}
$$

如果电网变快，$\Delta\omega$ 会让相位误差增长。如果 PLL 的 PI 输出 $v_f$ 也变大，PLL 估计频率 $\hat{\omega}$ 被拉高，相位误差会被压回去。

## 6. PI 输出与相位误差的关系

PI 控制器为：

$$
G_{\text{PI}}(s) = K_p + \frac{K_i}{s}
$$

PI 输出为：

$$
v_f = G_{\text{PI}}(s)\cdot e
$$

前面已经得到：

$$
e \approx V_m\delta
$$

所以：

$$
v_f
= \left(K_p + \frac{K_i}{s}\right)V_m\delta
$$

整理：

$$
v_f
= V_m\left(K_p + \frac{K_i}{s}\right)\delta
$$

代回：

$$
\dot{\delta} = \Delta\omega - v_f
$$

得到：

$$
\dot{\delta}
= \Delta\omega
- V_m\left(K_p + \frac{K_i}{s}\right)\delta
$$

这就是 SRF-PLL 的小信号误差动态。

## 7. 闭环特征方程

为了整定 $K_p / K_i$，看电网频率偏差为零时的自然动态。

令：

$$
\Delta\omega = 0
$$

则：

$$
\dot{\delta}
= -V_m\left(K_p + \frac{K_i}{s}\right)\delta
$$

在拉普拉斯域中：

$$
\dot{\delta} \rightarrow s\delta
$$

所以：

$$
s\delta
= -V_m\left(K_p + \frac{K_i}{s}\right)\delta
$$

移到等式左边：

$$
s\delta
+ V_m\left(K_p + \frac{K_i}{s}\right)\delta
= 0
$$

提取 $\delta$：

$$
\left[
s
+ V_m\left(K_p + \frac{K_i}{s}\right)
\right]\delta
= 0
$$

闭环特征方程为：

$$
s
+ V_m\left(K_p + \frac{K_i}{s}\right)
= 0
$$

两边乘以 $s$：

$$
s^2 + V_mK_p s + V_mK_i = 0
$$

这就是 PLL 的二阶闭环特征方程。

## 8. 与标准二阶系统匹配

标准二阶系统特征方程：

$$
s^2 + 2\zeta\omega_n s + \omega_n^2 = 0
$$

PLL 特征方程：

$$
s^2 + V_mK_p s + V_mK_i = 0
$$

逐项对比：

$$
V_mK_p = 2\zeta\omega_n
$$

$$
V_mK_i = \omega_n^2
$$

所以：

$$
K_p = \frac{2\zeta\omega_n}{V_m}
$$

$$
K_i = \frac{\omega_n^2}{V_m}
$$

如果使用带宽频率 $f_n$ 表示：

$$
\omega_n = 2\pi f_n
$$

则：

$$
K_p = \frac{2\zeta\cdot 2\pi f_n}{V_m}
$$

$$
K_i = \frac{(2\pi f_n)^2}{V_m}
$$

## 9. 34.5 kV 电压下的参数

用户给定：

$$
V_m = 34.5\,\text{kV}\cdot\sqrt{2}
$$

即：

$$
V_m = 34500\cdot 1.414213562 = 48790.37\,\text{V}
$$

取常用阻尼比：

$$
\zeta = 0.707
$$

### 9.1 PLL 带宽 20 Hz

$$
f_n = 20\,\text{Hz}
$$

$$
\omega_n = 2\pi\cdot 20 = 125.6637\,\text{rad/s}
$$

计算 $K_p$：

$$
K_p
= \frac{2\cdot 0.707\cdot 125.6637}{48790.37}
= 0.00364
$$

计算 $K_i$：

$$
K_i
= \frac{125.6637^2}{48790.37}
= 0.3237
$$

代码参数：

```c
#define APP_PLL_PI_KP (0.00364f)
#define APP_PLL_PI_KI (0.3237f)
```

### 9.2 PLL 带宽 50 Hz

$$
f_n = 50\,\text{Hz}
$$

$$
\omega_n = 2\pi\cdot 50 = 314.1593\,\text{rad/s}
$$

计算 $K_p$：

$$
K_p
= \frac{2\cdot 0.707\cdot 314.1593}{48790.37}
= 0.00911
$$

计算 $K_i$：

$$
K_i
= \frac{314.1593^2}{48790.37}
= 2.0224
$$

代码参数：

```c
#define APP_PLL_PI_KP (0.00911f)
#define APP_PLL_PI_KI (2.0224f)
```

## 10. 推荐使用值

当前 MATLAB PLL 测试模型建议先使用 20 Hz 带宽：

```c
#define APP_PLL_PI_KP (0.00364f)
#define APP_PLL_PI_KI (0.3237f)
```

这组参数更稳，锁相速度不会太激进。

如果需要更快锁相，可尝试 50 Hz 带宽：

```c
#define APP_PLL_PI_KP (0.00911f)
#define APP_PLL_PI_KI (2.0224f)
```

这组参数响应更快，但对噪声和谐波更敏感。

## 11. Tustin PI 离散约束

`pi_tustin_update()` 中生成的离散 PI 系数为：

$$
b_0 = K_p + \frac{K_iT_s}{2}
$$

$$
b_1 = -K_p + \frac{K_iT_s}{2}
$$

代码中要求：

$$
T_sK_i < 2K_p
$$

当前 PLL 每 $200\,\mu s$ 计算一次：

$$
T_s = 0.0002\,s
$$

20 Hz 参数检查：

$$
T_sK_i = 0.0002\cdot 0.3237 = 0.0000647
$$

$$
2K_p = 2\cdot 0.00364 = 0.00728
$$

满足：

$$
T_sK_i < 2K_p
$$

50 Hz 参数检查：

$$
T_sK_i = 0.0002\cdot 2.0224 = 0.0004045
$$

$$
2K_p = 2\cdot 0.00911 = 0.01822
$$

也满足：

$$
T_sK_i < 2K_p
$$

## 12. 幅值归一化影响

当前 PLL 未对 $v_q$ 按电压幅值归一化。

因此小信号关系是：

$$
e \approx V_m\delta
$$

所以参数中需要除以 $V_m$：

$$
K_p = \frac{2\zeta\omega_n}{V_m}
$$

$$
K_i = \frac{\omega_n^2}{V_m}
$$

如果后续将误差改为幅值归一化形式：

$$
e = \frac{-v_q}{\sqrt{v_\alpha^2 + v_\beta^2}}
$$

则锁相附近：

$$
e \approx \delta
$$

此时整定公式变为：

$$
K_p = 2\zeta\omega_n
$$

$$
K_i = \omega_n^2
$$

当前代码未使用幅值归一化形式。
