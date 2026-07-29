# 控制算法库使用方法

## 1. 模块选择

| 需求 | 模块 |
|---|---|
| PI 闭环 | `pi_tustin` / `pi_tustin_i32` |
| 完整 PID | `pid` |
| 增量式 PID | `pid_inc` |
| 谐振控制 | `pr` |
| 两零两极补偿 | `z2p2` |
| 两个约束环竞争 | `pi_dual_compete` |

浮点与整数实现不能只按 MCU 是否有 FPU 选择，还应检查信号量程、饱和范围、中间乘积位宽和整条控制链的缩放边界。

## 2. 通用生命周期

1. 调用方静态分配模块对象、参考量和反馈量。
2. 用真实控制周期、系数、输出限幅和输入指针初始化。
3. 每个控制周期调用一次计算函数。
4. 从对象的 `output` 字段读取结果。
5. 停机、模式切换或重新使能前调用 reset 接口清除历史状态。
6. 在线更新系数时，明确决定保留还是清除历史状态。

```c
static float s_ref;
static float s_feedback;
static pi_tustin_t s_pi;

void loop_init(void)
{
    (void)pi_tustin_init(&s_pi, 0.2f, 10.0f, 0.0001f,
                         1.0f, -1.0f, &s_ref, &s_feedback);
}

float loop_run(void)
{
    (void)pi_tustin_cal(&s_pi);
    return s_pi.output.val;
}
```

## 3. 采样周期与系数

初始化参数中的 `ts` 必须等于实际调用间隔。若中断频率改变，应重新计算控制器系数；只修改调度周期而保留旧离散系数会改变闭环特性。

`pi_tustin_update()` 等在线更新接口只负责系数，不代表任何切换过程都无扰。大幅改变参数、限幅或控制对象时，应由上层安排停机、状态重置或输出跟踪。

## 4. 指针与对象所有权

库对象和传入的参考、反馈指针均由调用方持有，必须在模块使用期间持续有效。同一个对象只应由一个实时执行上下文更新；监控任务可以读取输出快照，但不应并发修改内部历史量。

## 5. 限幅与抗饱和

限幅应同时满足控制算法的数值范围和执行器物理范围。级联环路中，外环限幅通常对应内环可接受参考值；不能仅依赖最后一级 PWM 限幅来掩盖上游积分饱和。

## 6. 关联导航

- 源码：[PI Tustin](../../../code/lib/pi_tustin.h) · [整数 PI Tustin](../../../code/lib/pi_tustin_i32.h) · [PID](../../../code/lib/pid.h) · [PR](../../../code/lib/pr.h) · [2P2Z](../../../code/lib/z2p2.h)
- 设计：[控制模块总设计](../../design/control/ctrl_design.md)
