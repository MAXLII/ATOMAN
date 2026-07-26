# 控制模块原理性时域仿真

## 1. 仿真入口

| 控制模块 | MATLAB 脚本 | 主要验证内容 |
| --- | --- | --- |
| BB | `docs/bb/bb_principle_time_domain.m` | Buck、Buck-Boost、Boost 模式切换，参考变化，负载变化，DCM 迟滞 |
| Boost | `docs/boost/boost_principle_time_domain.m` | 两相电压外环/电流内环，输入输出变化，CR/CC/CV 负载 |
| Buck | `docs/buck/buck_principle_time_domain.m` | 两相电压外环/电流内环，软启动，阶跃，斜坡，负载和功率限制 |
| CLLC | `docs/cllc/` 下的 `*_time_domain.m` | 正反向控制、双环竞争、PR 纹波抑制和异常工况 |
| INV | `docs/inv/inv_principle_time_domain.m` | dq 电压 PI、电流比例环、RMS 软启动、频率斜坡、负载阶跃 |
| LLC | `docs/llc/llc_principle_time_domain.m` | 单电压 PI、输出反馈 LPF、100 Hz 母线前馈 |
| PFC | `docs/pfc/pfc_principle_time_domain.m` | 母线 PI、正弦电流参考、PR 电流环、负载和频率变化 |
| PFC-I32 | `docs/pfc_i32/pfc_i32_principle_time_domain.m` | 母线 PI、整数电流 PI、ADC/PWM 量化及连续域对比 |

在 MATLAB 当前目录为仓库根目录时，可用以下方式运行任意脚本：

```matlab
run('docs/pfc/pfc_principle_time_domain.m')
```

## 2. 模型边界

这些脚本使用连续导通平均模型或低阶平均模型，目标是验证控制方向、PI/PR 参数、动态趋势、限幅和模式切换。它们不包含 MOSFET 开关瞬态、死区、磁性器件非线性、寄生参数、采样延迟和 ADC 噪声，最终硬件参数需要继续在 PLECS 开关模型和实机上验证。

BB 的 `HW_BUCK_BOOST_INPUT_CAP_VALUE`、`HW_BUCK_BOOST_OUTPUT_CAP_VALUE` 和 `HW_BUCK_BOOST_IND_VALUE` 当前均为 0。BB 脚本使用明确标注的原理仿真参数 `L = 200 uH`、`Cout = 2 mF`，运行时也会输出警告；硬件参数确定后应同步替换脚本参数并重新计算 PI。

PFC-I32 脚本按照工程代码域使用以下量化范围：交流电压有符号 12 bit、母线电压无符号 12 bit、电流有符号 14 bit、PWM reload 为 64000。该脚本用于观察量化引入的差异，不是 C 代码逐指令仿真。
