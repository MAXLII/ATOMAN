# BB Mode 设计文档

## 1. 模块定位

`bb_mode` 位于 `code/lib/`，用于 Buck-Boost CCM 路径的工作模式选择和 duty 计算。

该模块是硬件无关算法模块，不访问 MCU、BSP、HAL、PWM 或 ADC。调用方提供输入电压、输出电压和电感电压命令指针，模块根据当前模式计算 buck duty、boost duty 和半频运行标志。

## 2. 文件

| 文件 | 职责 |
| --- | --- |
| `bb_mode.h` | 定义模式枚举、输入结构、内部状态、输出结构和公开接口 |
| `bb_mode.c` | 实现模式切换、duty 计算、duty 限幅和空指针保护 |

## 3. 数据结构

`bb_mode_t` 由三部分组成：

| 成员 | 说明 |
| --- | --- |
| `input` | 绑定 `p_v_l`、`p_v_in`、`p_v_out` 三个实时输入指针 |
| `inter` | 保存当前工作模式 |
| `output` | 输出 `buck_duty`、`boost_duty` 和 `is_half_freq` |

工作模式由 `bb_mode_e` 表示：

| 模式 | 说明 |
| --- | --- |
| `BB_MODE_BUCK` | Buck 区域 |
| `BB_MODE_BOOST` | Boost 区域 |
| `BB_MODE_BUCK_BOOST` | Buck-Boost 联合调制区域 |

## 4. 计算关系

基础 duty 关系：

| 路径 | 公式 |
| --- | --- |
| Buck | `buck_duty = (v_l + v_out) / v_in` |
| Boost | `boost_duty = (v_in - v_l) / v_out` |
| 固定 boost 求 buck | `buck_duty = (v_l + v_out * boost_duty) / v_in` |
| 固定 buck 求 boost | `boost_duty = (v_in * buck_duty - v_l) / v_out` |

输入电压和输出电压在计算前会做下限保护，最小值为 `0.001f`，避免除 0。

## 5. 模式切换

模块根据当前模式和 duty 阈值更新 `inter.mode`：

| 常量 | 作用 |
| --- | --- |
| `BB_MODE_SSW_TO_BB_MODE_THR` | 从单边软开关区域进入 Buck-Boost 区域的阈值 |
| `BB_MODE_BB_MODE_TO_SSW_THR` | 从 Buck-Boost 区域退回单边软开关区域的阈值 |
| `BB_MODE_DUTY_MAX` | 单路 duty 上限 |
| `BB_MODE_DUTY_SUM` | buck duty 与 boost duty 的合计上限 |
| `BB_MODE_BUCK_BOOST_FIXED_BOOST_DUTY` | Buck-Boost 区域中优先使用的固定 boost duty |

在 Buck-Boost 模式中，模块先按基础 buck duty 判断当前工作点：buck duty 超过单路上限时固定 buck duty 并反算 boost duty；否则固定 boost duty 并反算 buck duty。之后再限制 buck/boost duty 之和。

## 6. 输出

`bb_mode_func()` 更新：

| 输出 | 说明 |
| --- | --- |
| `output.buck_duty` | buck 侧 duty，最终限幅到 `[0.0f, 1.0f]` |
| `output.boost_duty` | boost 侧 duty，最终限幅到 `[0.0f, 1.0f]` |
| `output.is_half_freq` | 当前模式为 `BB_MODE_BUCK_BOOST` 时置 1 |

## 7. 约束

- 不使用动态内存。
- 输入指针由调用方持有并保持有效。
- `bb_mode_func()` 对 `NULL` 指针做保护；输入无效时直接返回。
- 模块只生成 duty，不负责 PWM 使能、死区、互锁和硬件寄存器写入。
