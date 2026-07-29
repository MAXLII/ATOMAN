# PLECS 编译环境安装和编译方法

## 1. 环境说明

本仓库的 PLECS 工程通过 CMake 调用 MinGW-w64 编译器生成 Windows DLL，供 PLECS 模型中的 DLL Block 加载。

当前 PLECS 工程位于：

| 目录 | 模型文件 | 编译脚本 |
| --- | --- | --- |
| `platform/plecs/ac/` | `ac.plecs` | `compile.bat` |
| `platform/plecs/buck/` | `buck.plecs` | `compile.bat` |
| `platform/plecs/boost/` | `boost.plecs` | `compile.bat` |
| `platform/plecs/cllc/` | `cllc.plecs` | `compile.bat` |
| `platform/plecs/inv/` | `inv.plecs` | `compile.bat` |
| `platform/plecs/llc/` | `llc.plecs` | `compile.bat` |
| `platform/plecs/pfc/` | `pfc.plecs` | `compile.bat` |
| `platform/plecs/pfc_i32/` | `pfc.plecs` | `compile.bat` |

公共 DLL 适配代码位于 `platform/plecs/common/`，控制算法代码复用 `code/` 目录下的模块。

## 2. 必需软件

### 2.1 PLECS

安装 PLECS Standalone。

PLECS 版本需要支持当前模型文件和 64 位 DLL Block。DLL Block 接口和加载行为以实际安装版本为准。

### 2.2 CMake

安装 CMake，并确保 `cmake.exe` 可以在 Windows 命令行中直接调用。

检查命令：

```bat
cmake --version
```

如果命令无法识别，将 CMake 安装目录下的 `bin` 目录加入 Windows `Path` 环境变量。

### 2.3 MinGW-w64

安装 64 位 MinGW-w64，并使编译器路径与当前工程脚本一致：

```text
C:/mingw64/bin
```

当前 `compile.bat` 和 `CMakeLists.txt` 使用的工具链路径为：

```text
C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe
C:/mingw64/bin/x86_64-w64-mingw32-g++.exe
C:/mingw64/bin/mingw32-make.exe
```

检查命令：

```bat
C:\mingw64\bin\x86_64-w64-mingw32-gcc.exe --version
C:\mingw64\bin\mingw32-make.exe --version
```

如果 MinGW-w64 安装在其它目录，需要同步修改对应工程中的 `compile.bat` 和 `CMakeLists.txt` 里的 `C:/mingw64/bin`。

## 3. 编译方法

在 Windows 命令提示符或 PowerShell 中进入具体 PLECS 工程目录，然后执行 `compile.bat`。

仓库位于 WSL UNC 路径时，不要直接把命令行当前目录停在 `\\wsl.localhost\...` 后执行 `.bat`。`cmd.exe` 不支持 UNC 当前工作目录，会退回到 Windows 系统目录，导致 CMake 找不到工程目录。

推荐使用 `pushd` 进入工程目录。`pushd` 会临时映射 UNC 路径到盘符，`compile.bat` 可以在该盘符目录下正常运行。

### 3.1 编译 AC 工程

AC 工程使用 `compile.bat` 执行 CMake 配置和 MinGW 编译：

```bat
pushd \\wsl.localhost\Ubuntu\home\zeus\work\base\platform\plecs\ac
compile.bat
popd
```

所有 PLECS 工程共用 `platform/plecs/common/plecs.c`。日志路径在 DLL 加载后根据 DLL 自身位置生成，不需要工程专用的绝对路径源文件。

生成文件：

```text
platform/plecs/ac/build/bin/libplecs.dll
platform/plecs/ac/build/bin/plecs.map
```

`ac.plecs` 当前保存的 DLL 路径为：

```text
build/libplecs.dll
```

当前 CMake 实际生成路径为 `build/bin/libplecs.dll`。如 PLECS 打开模型后提示 DLL 找不到，需要在 DLL Block 中把路径改为当前编译输出文件。

### 3.2 编译 Buck 工程

```bat
pushd \\wsl.localhost\Ubuntu\home\zeus\work\base\platform\plecs\buck
compile.bat
popd
```

生成文件：

```text
platform/plecs/buck/build/bin/libplecs.dll
platform/plecs/buck/build/bin/plecs.map
```

`buck.plecs` 当前加载的 DLL 路径为：

```text
build\bin\libplecs.dll
```

### 3.3 编译 Boost 工程

```bat
pushd \\wsl.localhost\Ubuntu\home\zeus\work\base\platform\plecs\boost
compile.bat
popd
```

生成文件：

```text
platform/plecs/boost/build/bin/libplecs.dll
platform/plecs/boost/build/bin/plecs.map
```

`boost.plecs` 当前加载的 DLL 路径为：

```text
build\bin\libplecs.dll
```

## 4. 编译脚本行为

各 PLECS 工程的 `compile.bat` 会执行以下步骤：

1. 删除当前工程目录下已有的 `build/`。
2. 重新创建 `build/`。
3. 使用 CMake 生成 MinGW Makefiles。
4. 调用 `mingw32-make.exe -j8` 编译。
5. 生成 `libplecs.dll` 和链接 map 文件。

等价的手动编译流程如下：

```bat
cd platform\plecs\buck
rmdir /s /q build
mkdir build
cd build
cmake -G "MinGW Makefiles" -DCMAKE_C_COMPILER=C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe ..
C:/mingw64/bin/mingw32-make.exe -j8
```

AC、INV、PFC 和 PFC_I32 工程还会向 CMake 传入 C++ 编译器路径：

```bat
cmake -G "MinGW Makefiles" ^
  -DCMAKE_C_COMPILER=C:/mingw64/bin/x86_64-w64-mingw32-gcc.exe ^
  -DCMAKE_CXX_COMPILER=C:/mingw64/bin/x86_64-w64-mingw32-g++.exe ^
  ..
```

## 5. 在 PLECS 中运行

1. 先完成对应工程的 DLL 编译。
2. 打开对应模型文件：
   - `platform/plecs/ac/ac.plecs`
   - `platform/plecs/buck/buck.plecs`
   - `platform/plecs/boost/boost.plecs`
   - `platform/plecs/cllc/cllc.plecs`
   - `platform/plecs/inv/inv.plecs`
   - `platform/plecs/llc/llc.plecs`
   - `platform/plecs/pfc/pfc.plecs`
   - `platform/plecs/pfc_i32/pfc.plecs`
3. 检查模型中 DLL Block 的 DLL 路径是否指向当前工程生成的 `libplecs.dll`。
4. 运行仿真。

DLL 运行时日志文件与当前加载的 `libplecs.dll` 位于同一目录：

| 工程 | 日志文件 |
| --- | --- |
| AC | `platform/plecs/ac/build/bin/plecs_log.txt` |
| Buck | `platform/plecs/buck/build/bin/plecs_log.txt` |
| Boost | `platform/plecs/boost/build/bin/plecs_log.txt` |
| CLLC | `platform/plecs/cllc/build/bin/plecs_log.txt` |
| INV | `platform/plecs/inv/build/bin/plecs_log.txt` |
| LLC | `platform/plecs/llc/build/bin/plecs_log.txt` |
| PFC | `platform/plecs/pfc/build/bin/plecs_log.txt` |
| PFC_I32 | `platform/plecs/pfc_i32/build/bin/plecs_log.txt` |

Windows 构建使用宽字符系统接口读取当前 DLL 的完整路径，再在其目录下创建 `plecs_log.txt`。仓库路径包含中文时也不依赖本地 ANSI 代码页。日志路径解析失败时，仿真继续运行，但日志文件不会创建。

## 6. 常见问题

### 6.1 `cmake` 不是内部或外部命令

CMake 没有安装，或 CMake 的 `bin` 目录没有加入 Windows `Path`。

处理方法：安装 CMake 后重新打开命令行窗口，再执行：

```bat
cmake --version
```

### 6.2 找不到 `x86_64-w64-mingw32-gcc.exe`

MinGW-w64 安装路径与工程配置不一致。

处理方法：

1. 确认本机是否存在 `C:\mingw64\bin\x86_64-w64-mingw32-gcc.exe`。
2. 如果安装在其它目录，修改对应工程的 `compile.bat` 和 `CMakeLists.txt`。

### 6.3 PLECS 提示 DLL 加载失败

按以下顺序检查：

1. DLL 是否已经成功编译生成。
2. PLECS DLL Block 中配置的路径是否与生成路径一致。
3. PLECS 与 DLL 是否同为 64 位环境。
4. DLL 是否正在被其它 PLECS 仿真实例占用。

### 6.4 修改代码后仿真结果没有变化

重新执行对应工程的 `compile.bat`，然后在 PLECS 中重新加载模型或重新启动仿真。

如果 DLL 被 PLECS 占用导致编译失败，关闭正在运行的仿真或退出 PLECS 后重新编译。

### 6.5 从 `\\wsl.localhost` 路径执行脚本失败

`cmd.exe` 不支持 UNC 路径作为当前工作目录。使用 `pushd` 进入工程目录后再执行脚本：

```bat
pushd \\wsl.localhost\Ubuntu\home\zeus\work\base\platform\plecs\buck
compile.bat
popd
```

### 6.6 没有生成日志文件

确认 DLL 已从当前工程的 `build/bin/` 加载，并检查该目录是否具有写入权限。日志路径解析或文件打开失败不会中止仿真。

## 7. 关联导航

- 源码：[PLECS 公共运行时](../../../platform/plecs/common/plecs.c) · [Buck 构建配置](../../../platform/plecs/buck/CMakeLists.txt) · [CLLC 构建配置](../../../platform/plecs/cllc/CMakeLists.txt)
- 设计：[工程设计](../../engineering_design.md) · [控制模块总设计](../../design/control/ctrl_design.md)
