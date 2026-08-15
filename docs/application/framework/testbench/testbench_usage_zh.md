# GCC Host Testbench 使用指南

## 1. 文档定位

本文说明 GCC Host Testbench 的设计约定、工程结构、注册接口、运行时序、构建方法和测试用例开发流程。

首次使用时，可按下面的顺序了解并操作这套环境：

1. 确认 Testbench 的适用范围。
2. 查看目录和公共运行框架。
3. 掌握模块注册、用例注册和逐拍执行过程。
4. 确定 `p_before_dut()`、DUT 和 `p_after_dut()` 的时序关系。
5. 通过 PI 控制电感案例理解 `before_dut()` 和 `after_dut()` 的用法。
6. 建立一个新的测试工程和 Makefile。
7. 编译、运行并查看 CSV 波形。

## 2. Testbench 是什么

Testbench 是一套运行在 PC 上的 GCC 软件测试平台，用来脱离 MCU、MATLAB 和 PLECS，单独验证纯软件 DUT。

DUT 是 Design Under Test 的缩写，统一表示 Testbench 中的被测对象。DUT 可以是函数、模块、算法，也可以是完整的软件子系统。

Testbench 将需要验证的软件统一抽象为 DUT，在 PC 上建立输入、环境和时间，周期性运行 DUT、记录过程，并根据测试用例给出 COMPLETE、PASS 或 FAIL。

Testbench 当前具备以下能力：

- 使用 GCC 编译真实工程代码；
- 静态注册多个 DUT 模块；
- 每个 DUT 模块可以静态注册多个测试用例；
- 一个可执行文件自动发现并运行所有已经链接的模块和用例；
- 每个 DUT 模块拥有自己的运行周期；
- 支持状态变化、物理对象、闭环控制和 SFRA 等测试环境；
- 测试用例可以生成 CSV 波形或波特图数据；
- 通过进程退出码向自动化脚本返回整体测试结果。

## 3. 当前目录结构

```text
platform/testbench/
├── common/
│   ├── testbench.h
│   └── testbench.cpp
├── ac_loss_det/
│   ├── Makefile
│   ├── ac_loss_det_test.c
│   └── ac_loss_det_test.h
└── pi/
    ├── Makefile
    ├── pi_test.c
    └── pi_test.h
```

各目录职责如下：

| 目录 | 职责 |
|---|---|
| `common/` | 公共注册接口、链表初始化、测试调度、结果汇总和 `main()`。 |
| `ac_loss_det/` | 交流掉电检测模块的输入生成、状态检查和 CSV 记录。 |
| `pi/` | PI、电感对象、阶跃响应、闭环 SFRA 和开环 SFRA 测试。 |

新增测试工程时，在 `platform/testbench/` 下建立一个与 `common/` 同级的目录。生产代码继续保留在原目录，测试工程直接编译原来的源代码文件，不需要复制或者迁移 DUT 源码和头文件。

测试产生的 `.exe`、`.o`、日志和 CSV 统一放到测试工程自己的 `build/` 目录。

## 4. 编译环境

Windows 环境需要安装 MinGW GCC、G++ 和 `mingw32-make`。可以先检查：

```powershell
gcc --version
g++ --version
mingw32-make --version
```

公共 runner 使用 C++ 实现，是为了让同一个测试可执行文件能够同时容纳 C 或 C++ 编写的 DUT。DUT、依赖和测试辅助代码可以按照各自的源文件类型，选择合适的编译器和语言标准。

当前参考 Makefile 将 `common/testbench.cpp` 按 C++14 编译，并使用 G++ 完成最终链接，因为 runner 使用了 C++ 标准库。其他构建系统也可以采用等效的 C++ 编译和链接配置。参考 Makefile 中的严格警告选项属于示例工程的质量策略，不是 Testbench 的强制要求，可以根据被测工程调整。

当前注册方案依赖 GNU ld 自动生成的 `__start_<section>` 和 `__stop_<section>` 符号，因此需要使用兼容的 GNU 链接器。

## 5. 两个注册宏

测试源文件先引用：

```c
/* 引入注册宏和测试用例状态定义。 */
#include "testbench.h"
```

### 5.1 注册 DUT 模块

```c
/* 注册一个 DUT，并设置相邻两次执行之间的周期。 */
TESTBENCH_REGISTER(test_name, period_s, dut_init, dut_run)
```

参数含义：

| 参数 | 含义 |
|---|---|
| `test_name` | 可用于宏拼接的合法标识符，同时用作显示名称、结构体变量名和链接段名称。 |
| `period_s` | DUT 相邻两次运行之间的周期，单位是秒，例如 `0.001` 表示 1 ms。 |
| `dut_init` | DUT 初始化函数，每个测试用例开始前都会单独调用一次。 |
| `dut_run` | DUT 主体函数，每一拍调用一次。 |

“每个测试用例都会重新调用 DUT 初始化”是隔离测试状态的重要保证。不同测试用例不能依赖上一个用例留下的 DUT 状态。

### 5.2 注册测试用例

```c
/* 在指定 DUT 模块下注册一个独立初始化的测试场景。 */
TESTBENCH_CASE(test_name, case_name, init, before_dut, after_dut)
```

参数含义：

| 参数 | 含义 |
|---|---|
| `test_name` | 必须与所属 `TESTBENCH_REGISTER` 的名称完全一致。 |
| `case_name` | 可用于宏拼接的合法标识符，同时用作测试用例名称和结构体变量名的一部分。 |
| `init` | 初始化当前测试用例的环境、fixture、场景和记录资源。 |
| `before_dut` | 在本拍 DUT 执行前推进和准备测试环境。 |
| `after_dut` | 处理本拍 DUT 输出、立即生效的环境影响、记录和结束判断。 |

这两个宏都用于函数外，展开结果是结构体变量声明。宏定义内部已经带分号，因此调用处不加分号：

```c
/* DUT 模块只注册一次。 */
TESTBENCH_REGISTER(pi, PI_TEST_CONTROL_PERIOD_S, dut_init, dut_run)

/* 注册属于 PI DUT 模块的阶跃测试用例。 */
TESTBENCH_CASE(pi,
               current_reference_step,
               step_init,
               step_before_dut,
               step_after_dut)
```

当前注册宏使用 C 指定初始化器，因此参考工程把展开 `TESTBENCH_REGISTER` 和 `TESTBENCH_CASE` 的少量注册代码放在 C11 源文件中。这不限制 DUT 必须使用 C；C++ DUT 和辅助代码可以单独使用 G++ 编译，再链接到同一个测试可执行文件。

## 6. 一个测试用例如何运行

每个测试用例的调用顺序是：

```text
测试用例 init
DUT init

第 1 拍：
    p_before_dut(Ts)
    dut_run()
    p_after_dut(Ts)

第 2 拍：
    p_before_dut(2 × Ts)
    dut_run()
    p_after_dut(2 × Ts)

……
```

完整调用关系如下：

```text
环境初始化 → DUT 初始化
                  ↓
before(k) → DUT(k) → after(k) → before(k+1) → DUT(k+1) → after(k+1)
```

公共 runner 把拍 0 留给初始条件，正常 DUT 执行从拍 1 开始。测试用例初始化先于 DUT 初始化，因此可以在 `init` 中打开 CSV，并在 `dut_init` 完成 DUT 初始化后记录 `time_s = 0.0`：

```c
static void dut_init(void)
{
    production_module_init(&fixture.dut, &fixture.input, &fixture.output);
    process_record(0.0); /* 把初始化后的 DUT 状态记录为拍 0。 */
}
```

测试用例达到结束条件时，`p_after_dut()` 返回 `TESTBENCH_CASE_COMPLETE`、`TESTBENCH_CASE_PASS` 或 `TESTBENCH_CASE_FAIL`；返回 `TESTBENCH_CASE_RUNNING` 时进入下一拍。

各结束状态的含义如下：

- `TESTBENCH_CASE_COMPLETE`：用例正常结束，但不作 PASS/FAIL 判定；
- `TESTBENCH_CASE_PASS`：用例结束，且断言通过；
- `TESTBENCH_CASE_FAIL`：用例结束，且至少一项必要检查失败。

## 7. before 和 after 的职责

固定时序是：

```text
p_before_dut(k) → dut_run(k) → p_after_dut(k) → 下一拍
```

### 7.1 `p_before_dut()`

`p_before_dut()` 表示“本拍 DUT 执行之前的完整测试环境阶段”，可以完成：

- 让上一拍 DUT 输出作用于有延迟的物理对象；
- 推进电感、电容、机械对象或其他离散模型；
- 生成正弦波、斜坡、阶跃、故障和状态切换；
- 准备本拍 DUT 的给定、反馈和输入；
- 记录本拍 DUT 执行前状态或其他过程数据；
- 执行 SFRA task 和本拍注入准备。

### 7.2 `p_after_dut()`

`p_after_dut()` 表示“本拍 DUT 输出产生之后的完整测试环境阶段”，可以完成：

- 让本拍 DUT 输出立即影响环境；
- 处理没有一拍延迟的理想开关或逻辑状态；
- 记录本拍 DUT 的输入和输出；
- 更新测试用例内部的统计和状态；
- 判断是否结束；
- 执行最终断言并关闭 CSV；
- 返回 RUNNING、COMPLETE、PASS 或 FAIL。

环境阶段的放置原则如下：

> 有延迟、要到当前拍开始时才看见的物理变化放在 before；本拍 DUT 输出立即产生的变化放在 after。记录放在哪里，取决于希望 CSV 表示哪个时刻。

## 8. before 和 after 使用案例

下面通过 PI 电流控制案例说明两个回调如何分工。对于被控对象 `1/(sL)`，第 `k` 拍 PI 算出的电压 `u[k]` 经过一个控制周期后，形成第 `k+1` 拍电流 `i[k+1]`：

```text
i[k+1] = i[k] + u[k] × Ts / L
```

`p_before_dut()` 负责推进电感对象，并按照时间改变电流给定：

```c
static void step_before_dut(double time_s)
{
    /* 在计算第 k 拍前，让 u[k - 1] 经过电感产生新的电流。 */
    fixture.current_a +=
        (fixture.voltage_v / PI_TEST_INDUCTANCE_H) * PI_TEST_CONTROL_PERIOD_S;

    /* 把更新后的电流反馈和本拍给定交给 PI。 */
    fixture.pi_feedback_a = fixture.current_a;
    if (time_s >= PI_TEST_STEP_TIME_S)
    {
        fixture.current_ref_a = PI_TEST_STEP_CURRENT_A;
    }
    else
    {
        fixture.current_ref_a = 0.0f;
    }
}
```

DUT 本拍计算新的电压，`p_after_dut()` 负责记录本拍结果并判断测试是否结束：

```c
static TESTBENCH_CASE_STATE_E step_after_dut(double time_s)
{
    /* 更新稳定性判断所需的响应状态。 */
    step_response_evaluate(time_s);
    step_record(time_s); /* 记录 i[k] 和新计算的 u[k]。 */

    /* 响应尚未稳定时继续执行下一拍。 */
    if (step_finished(time_s) == 0u)
    {
        return TESTBENCH_CASE_RUNNING;
    }
    /* 把最终检查结果转换成测试终止状态。 */
    return assertion_state_get(step_assert());
}
```

CSV 中应看到：

```text
第 20 拍：记录 i[20] 和 u[20]
第 21 拍：记录由 u[20] 产生的 i[21]，同时记录新的 u[21]
```

在这个案例中，`p_before_dut()` 推进测试环境并准备 DUT 输入，`p_after_dut()` 记录 DUT 输出并判断测试结果。

## 9. 建立 Makefile

Makefile 把需要调整的目录和源码集中放在 `User modification section` 中。建立新测试工程时，主要修改这一区域，其余构建规则可以保持不变：

```makefile
CC := gcc
CXX := g++

BUILD_DIR := build
TARGET := $(BUILD_DIR)/example_test.exe

# =================== User modification section begin ===================

# Project directories.
COMMON := ../common
CODE := ../../../code
LIB := $(CODE)/lib

# C source files.
C_SOURCES := example_test.c \
             $(LIB)/production_module.c

# C++ source files.
CXX_SOURCES := $(COMMON)/testbench.cpp

# Header search paths.
INCLUDE_DIRS := $(COMMON) \
                $(LIB)

# ==================== User modification section end ====================

CPPFLAGS := $(addprefix -I,$(INCLUDE_DIRS)) -DIS_TESTBENCH
CFLAGS := -std=c11
CXXFLAGS := -std=c++14
LDLIBS := -lm

C_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
CXX_OBJECTS := $(addprefix $(BUILD_DIR)/,$(notdir $(CXX_SOURCES:.cpp=.o)))
OBJECTS := $(C_OBJECTS) $(CXX_OBJECTS)

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.cpp $(sort $(dir $(CXX_SOURCES)))

.PHONY: all test prepare

all: prepare $(TARGET)

prepare: | $(BUILD_DIR)
	powershell.exe -NoProfile -Command \
		"Remove-Item -Path '$(BUILD_DIR)/*.o','$(BUILD_DIR)/*.exe' -Force -ErrorAction Ignore"

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDLIBS) -o $@

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

test: prepare $(TARGET)
	$(TARGET)
```

`User modification section` 是使用者主要修改的区域：

- 工程目录发生变化时修改 `COMMON`、`CODE`、`LIB` 等目录变量；
- C 源码加入 `C_SOURCES`；
- C++ 源码加入 `CXX_SOURCES`；
- 头文件目录加入 `INCLUDE_DIRS`。

`BUILD_DIR` 决定编译输出目录，`TARGET` 决定可执行文件名称。`prepare` 在每次编译前清除 `.o` 和 `.exe`，保留测试程序生成的 CSV 和日志。其余规则根据源码列表完成编译和链接，最终生成一个包含全部已注册测试用例的可执行文件。

目标文件使用源码文件名生成。同名但位于不同目录的源码会得到相同的目标文件名，因此加入列表的源码文件名应保持唯一。

## 10. 编译与运行

只编译：

```powershell
mingw32-make -C platform/testbench/ac_loss_det
```

编译并执行所有已经链接的测试用例：

```powershell
mingw32-make -C platform/testbench/ac_loss_det test
mingw32-make -C platform/testbench/pi test
```

测试成功时会输出汇总：

```text
TESTBENCH SUMMARY | modules=1 cases=4 passed=4 completed=0 failed=0
```

全部通过时进程返回 0；任一用例失败时返回非 0。

## 11. CSV 波形记录

CSV 由测试用例管理，公共 runner 不规定列和采样策略。建议流程是：

1. 在 `init` 中重置 fixture、打开 `build/<case>.csv` 并写入表头。
2. 需要初始条件时，在第一次 `p_before_dut()` 中记录 `time_s = 0.0`。
3. 根据时序语义在 before 或 after 中记录。
4. 结束时关闭文件。
5. 打开、写入或关闭失败都应使最终断言失败。

时域波形第一列统一使用 `time_s`。不要求每一拍都写 CSV，可以采用：

- 每 N 拍记录一次；
- 数值变化超过阈值时记录；
- 状态改变时记录；
- SFRA 完成一个频点时记录。

对于 PI 阶跃，需要核对上一拍电压和下一拍电流是否满足：

```text
i[k+1] = i[k] + u[k] × Ts / L
```

这项检查可以直接证明物理环境的拍序没有错位。
