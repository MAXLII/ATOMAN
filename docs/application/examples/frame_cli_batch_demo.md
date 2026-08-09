# FRAME CLI 批处理 Demo

本文使用 FRAME 源码仓库中的 `frame.ps1` 对 ATOMAN 目标进行一次性命令检查，并把参数、Scope 和 Perf 结果导出为 JSON 或 CSV。该方式适合重复联调、回归检查和保存现场数据。

## 1. 适用范围

本 Demo 以 GD32G553 串口工程为例：

| 项目 | 示例值 |
|---|---|
| Port | `COM8` |
| Baud | `921600` |
| Target Address | `0x02` |
| Dynamic Address | `0x00` |

运行前先完成 [GD32G553 串口与 FRAME 联调](frame_gd32g553_serial_demo.md)，并关闭占用同一 COM 口的 FRAME GUI 或其他串口工具。

FRAME Release 适合 GUI 操作；本 Demo 需要克隆 [FRAME 源码仓库](https://github.com/MAXLII/FRAME)，使用其中的 `frame.ps1`。

## 2. 准备 FRAME 源码环境

```powershell
git clone https://github.com/MAXLII/FRAME.git FRAME
Set-Location .\FRAME
python -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
.\frame.ps1 --help
```

## 3. 设置本次检查参数

在 PowerShell 中设置任务专用变量：

```powershell
$frameCli = (Resolve-Path .\frame.ps1).Path
$targetPort = 'COM8'
$targetBaud = 921600
$targetAddr = '0x02'
$targetDynamicAddr = '0x00'
$resultDir = Join-Path (Get-Location) 'demo-results'
New-Item -ItemType Directory -Force -Path $resultDir | Out-Null
```

后续命令都复用这些变量。把 `COM8` 替换成实际端口。

## 4. 检查串口与参数

枚举串口：

```powershell
& $frameCli serial ports
```

导出完整参数列表：

```powershell
& $frameCli param list `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format json --output (Join-Path $resultDir 'parameters.json')
```

读写 Demo 参数：

```powershell
& $frameCli param read DEMO_SHELL_COUNTER `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr

& $frameCli param write DEMO_SHELL_COUNTER 5 123 `
  --min 0 --max 4294967295 `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr

& $frameCli param read DEMO_SHELL_COUNTER `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr
```

最后一次读取应返回 `123`。

## 5. 检查自定义协议回环

```powershell
& $frameCli proto `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --cmd-set 0x30 --cmd-word 0x01 `
  --payload '78 56 34 12 A5 85 FF' `
  --format json --output (Join-Path $resultDir 'loopback.json')
```

打开 `loopback.json`，应满足：

```text
cmd_set     = 0x30
cmd_word    = 0x01
is_ack      = 1
payload_hex = 78 56 34 12 A5 85 FF
```

## 6. 导出 Scope 与 Perf 信息

导出 Scope 列表：

```powershell
& $frameCli scope list `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format json --output (Join-Path $resultDir 'scope-list.json')
```

导出 Perf 汇总：

```powershell
& $frameCli perf summary `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format csv --output (Join-Path $resultDir 'perf-summary.csv')
```

需要完整 Perf 字典和采样时执行：

```powershell
& $frameCli perf dict `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format json --output (Join-Path $resultDir 'perf-dict.json')

& $frameCli perf sample `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format csv --output (Join-Path $resultDir 'perf-sample.csv')
```

## 7. 组合成最小回归检查

PowerShell 的 `$LASTEXITCODE` 保存每条 CLI 命令的退出码。可以在关键命令后立即判断：

```powershell
& $frameCli param list `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format json --output (Join-Path $resultDir 'parameters.json')
if ($LASTEXITCODE -ne 0) { throw 'parameter list failed' }

& $frameCli proto `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --cmd-set 0x30 --cmd-word 0x01 `
  --payload '78 56 34 12 A5 85 FF' `
  --format json --output (Join-Path $resultDir 'loopback.json')
if ($LASTEXITCODE -ne 0) { throw 'protocol loopback failed' }

& $frameCli perf summary `
  --port $targetPort --baud $targetBaud `
  --dst $targetAddr --d-dst $targetDynamicAddr `
  --format csv --output (Join-Path $resultDir 'perf-summary.csv')
if ($LASTEXITCODE -ne 0) { throw 'perf summary failed' }
```

## 8. 输出文件

成功运行后，`demo-results/` 至少包含：

```text
demo-results/
├─ parameters.json
├─ loopback.json
├─ scope-list.json
├─ perf-summary.csv
├─ perf-dict.json
└─ perf-sample.csv
```

这些文件适合作为一次联调记录。它们是本地运行结果，不应提交到 ATOMAN 或 FRAME 源码仓库。

## 9. 常见问题

- `Access denied`：FRAME GUI 或其他程序仍占用 COM 口。
- `timeout`：检查端口、波特率、目标地址和目标板运行状态。
- `param write` 类型错误：`DEMO_SHELL_COUNTER` 使用类型 `5`，`DEMO_SHELL_GAIN` 使用类型 `6`。
- Scope 或 Perf 为空：对应服务没有参与目标固件构建，或 Section 注册段未保留。
- 输出目录中的旧文件可能混淆结果，运行新一轮检查前可换一个新的结果目录。

## 10. 关联导航

- [ATOMAN 与 FRAME 配合使用](../communication/frame_atoman_integration.md)
- [GD32G553 串口与 FRAME 联调](frame_gd32g553_serial_demo.md)
- [FRAME CLI 命令](https://github.com/MAXLII/FRAME/blob/master/docs/CLI_COMMANDS.md)
