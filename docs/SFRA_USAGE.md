# SFRA 使用说明

## 采样对齐

SFRA 每个采样周期分为两个调用点：

- `sfra_isr_pre_sample()` 生成当前注入量 `inject`
- 用户代码运行被测对象，并更新 `collect`
- `sfra_isr_post_sample()` 采集本周期用于 DFT 的 `inject` 与 `collect`

`REG_SFRA(name, delay_tick, ts, inject_amp, freq_start, freq_end)` 中的
`delay_tick` 用于对齐 SFRA 参与计算的注入量与响应量。

## 不需要延迟一拍

当 `collect` 对应的是当前周期 `inject` 直接产生的响应时，使用
`delay_tick = 0U`。

典型情况：

- 纯算法模块没有额外计算延迟
- 被测对象在同一个 ISR 周期内完成输入到输出计算
- 当前 tick 写入输入，当前 tick 就能读取对应输出
- 扫描对象是解析稳态模型，输出按当前注入相位直接生成

此时 SFRA 使用当前 `*p_inject` 作为 DFT 分母：

```c
REG_SFRA(demo_sfra,
         0U,
         DEMO_SFRA_SAMPLE_PERIOD_S,
         DEMO_SFRA_INJECT_AMPLITUDE,
         DEMO_SFRA_FREQ_START_HZ,
         DEMO_SFRA_FREQ_END_HZ)
```

## 需要延迟一拍

当 `collect` 对应的是上一周期 `inject` 经过控制器、PWM 更新、功率级或采样链路后产生的响应时，使用
`delay_tick = 1U`。

典型情况：

- 当前 tick 计算出的控制量要到下一 tick 才作用到对象
- 控制器输出经 PWM 更新寄存器后一拍生效
- ADC 采样值反映的是上一周期控制量造成的电流或电压
- 仿真模型中显式使用 `z1`、`last`、`prev` 等上一拍状态
- 代码结构为先采集当前输出，再根据当前注入计算下一拍控制量

此时 SFRA 使用上一拍 `inject` 作为 DFT 分母，使注入量与当前采到的响应量对齐：

```c
REG_SFRA(demo_pi_l_closed_sfra,
         1U,
         DEMO_SFRA_SAMPLE_PERIOD_S,
         DEMO_SFRA_INJECT_AMPLITUDE,
         DEMO_SFRA_FREQ_START_HZ,
         DEMO_SFRA_FREQ_END_HZ)
```

闭环电流对象的一拍延迟时序为：

```c
sfra_isr_pre_sample(&demo_pi_l_closed_sfra);

demo_pi_l_closed_sfra_collect = inductor.current_a;
sfra_isr_post_sample(&demo_pi_l_closed_sfra);

pi_ref = demo_pi_l_closed_sfra_inject;
pi_act = demo_pi_l_closed_sfra_collect;
pi_tustin_cal(&pi);

inductor.current_a += inductor.voltage_cmd_z1 / L * Ts;
inductor.voltage_cmd_z1 = pi.output.val;
```

## 判断方法

判断 `delay_tick` 的核心是看 `collect` 与哪个时刻的 `inject` 对应。

- `collect[n]` 是 `inject[n]` 的响应，使用 `delay_tick = 0U`
- `collect[n]` 是 `inject[n-1]` 的响应，使用 `delay_tick = 1U`
- `collect[n]` 是 `inject[n-2]` 的响应，使用 `delay_tick = 2U`

当前 SFRA 模块支持的最大延迟为 `SFRA_MAX_INJECT_DELAY_TICK`。

## 实际工程建议

实际工程中优先按采样与控制时序确定延迟，而不是按模块名称确定延迟。

电流环、功率级、PWM 更新、ADC 采样链路通常存在一拍延迟。纯软件滤波器、控制器本体、无状态数学模型通常不需要延迟。

如果扫频结果相位整体多出接近一个采样周期的相位滞后，检查 `delay_tick` 是否过小。
如果扫频结果相位整体少了一个采样周期的滞后，检查 `delay_tick` 是否过大。
