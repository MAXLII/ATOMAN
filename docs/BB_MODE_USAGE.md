# BB Mode 使用文档

## 1. 适用范围

本文档说明如何使用 `code/lib/bb_mode.c`。

设计说明见 [BB_MODE_DESIGN.md](BB_MODE_DESIGN.md)。

## 2. 准备输入和对象

调用方需要准备三个实时输入变量：

```c
static float v_l_cmd = 0.0f;
static float v_in = 0.0f;
static float v_out = 0.0f;
static bb_mode_t bb_mode = {0};
```

含义：

| 变量 | 说明 |
| --- | --- |
| `v_l_cmd` | 电感电压命令 |
| `v_in` | 输入电压 |
| `v_out` | 输出电压 |

## 3. 初始化

```c
bb_mode_init(&bb_mode,
             &v_l_cmd,
             &v_in,
             &v_out,
             BB_MODE_BUCK);
```

初始模式由调用方根据当前输入/输出电压关系选择。

## 4. 周期计算

在控制 ISR 或控制任务中，先更新输入变量，再调用：

```c
bb_mode_func(&bb_mode);
```

调用后读取输出：

```c
float buck_duty = bb_mode.output.buck_duty;
float boost_duty = bb_mode.output.boost_duty;
uint8_t half_freq = bb_mode.output.is_half_freq;
```

## 5. 典型接入

在 BB 控制模块中，`bb_mode` 用在 CCM 路径：

```c
pi_tustin_cal(&ind_curr_loop);
bb_mode_func(&bb_mode);

pwm_setter(bb_mode.output.buck_duty,
           1U,
           1U,
           bb_mode.output.boost_duty,
           1U,
           1U);
```

`bb_mode` 只负责 duty 和模式选择，PWM 上下管使能、保护、死区和寄存器写入由上层控制模块或平台 BSP 处理。

## 6. 使用注意事项

- `p_v_l`、`p_v_in`、`p_v_out` 必须在 `bb_mode_func()` 调用期间有效。
- 输入电压和输出电压可以为较小值，模块内部会把分母限制到至少 `0.001f`。
- 输出 duty 会被限制在 `[0.0f, 1.0f]`。
- `inter.mode` 会在 `bb_mode_func()` 内根据 duty 阈值自动切换。
- `output.is_half_freq` 为 1 表示当前处于 Buck-Boost 联合调制区域。
