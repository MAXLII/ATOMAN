# Demo Guide

## Overview

This demo is a runnable example for the current GD32 workspace.
It is built as a fixed `ISP` + `IS_DC` project and uses the section-based framework already integrated into this repository.

The demo is intended to show how these parts work together:

- section registration and scheduling
- shell variable and command registration
- communication frame handling through `comm`
- performance measurement through `perf`
- scope data capture through `scope`
- GD32 USART-based debug link integration

## Device Address

The local communication address used by this demo is:

- `LOCAL_ADDR = DC_ADDR = LLC_ADDR = 0x02`

This comes from:

- `../../interface/common/comm_addr.h`
- `../../../gd32g553c/makefile`

Because this project is fixed as `IS_DC`, `LOCAL_ADDR` resolves to `DC_ADDR`, and `DC_ADDR` is currently mapped to `LLC_ADDR`, which is `0x02`.

## UART / Baud Rate

The demo debug link uses:

- UART peripheral: `USART0`
- baud rate: `115200`
- data format: `8-N-1`

This comes from:

- `../../../gd32g553c/bsp/bsp_usart.c`
- `../../interface/usart.c`

Relevant details:

- `usart_baudrate_set(USART0, 115200U);`
- shell and comm both run on `USART0_LINK`

## Demo Features

This demo currently includes the following function groups.

### 1. Section Framework Examples

The demo shows how to use:

- `REG_INIT`
- `REG_TASK_MS`
- `REG_TASK`
- `REG_SHELL_VAR`
- `REG_SHELL_CMD`
- `REG_COMM`
- `REG_PERF_RECORD`
- `REG_SCOPE_EX`

The implementation lives in:

- `demo.c`
- `demo.h`

### 2. Shell Examples

The demo registers shell variables:

- `DEMO_LED_MASK`
- `DEMO_COUNTER`
- `DEMO_LAST_CMD`
- `DEMO_GAIN`
- `DEMO_PERF_SPIN`

The demo registers shell commands:

- `DEMO_PING`
- `DEMO_HELP`
- `DEMO_PERF`
- `DEMO_SEND`
- `DEMO_TICK`

### 3. Communication Examples

The demo registers two communication handlers:

- loopback command
  - `cmd_set = 0x30`
  - `cmd_word = 0x01`
- control command
  - `cmd_set = 0x30`
  - `cmd_word = 0x02`

These are implemented with:

- `REG_COMM(DEMO_CMD_SET_LOOPBACK, DEMO_CMD_WORD_LOOPBACK, demo_loopback_comm)`
- `REG_COMM(DEMO_CMD_SET_CONTROL, DEMO_CMD_WORD_CONTROL, demo_control_comm)`

### 4. Performance Examples

The demo includes:

- manual performance measurement using `PERF_START` / `PERF_END`
- task execution performance records
- task period measurement example
- free-running timer readback example

Related commands:

- `DEMO_PERF`
- `DEMO_TICK`

### 5. Scope Examples

The demo includes two scope examples.

#### Fast scope

- name: `demo_scope_fast`
- sample period: `100 us`
- buffer size: `500`
- channel count: `10`

Signals include:

- `scope_sin`
- `scope_cos`
- `scope_sin2`
- `scope_cos2`
- `scope_ramp`
- `scope_triangle`
- `scope_square`
- `scope_mix`
- `scope_saw`
- `scope_index`

#### Slow scope

- name: `demo_scope_slow`
- sample period: `1 ms`
- buffer size: `200`
- channel count: `3`

Signals include:

- `scope_slow_sin`
- `scope_slow_cos`
- `scope_slow_mix`

## How To Use

### Build

Build from:

- `../../../gd32g553c`

Typical command:

```bat
compile.bat
```

The current workspace is fixed to:

- `ISP`
- `IS_DC`

### Connect Serial Port

Use a serial tool with:

- baud rate: `115200`
- data bits: `8`
- parity: `none`
- stop bits: `1`

### Try Shell Commands

Basic checks:

```text
DEMO_PING
DEMO_HELP
DEMO_TICK
DEMO_PERF
DEMO_SEND
```

Modify shell variables:

```text
DEMO_LED_MASK:3
DEMO_GAIN:2.5
DEMO_PERF_SPIN:5000
```

### Observe Communication

The debug link is wired through `USART0_LINK`.
Incoming bytes are dispatched to:

- `shell_run`
- `comm_run`

This path is implemented in:

- `../../interface/usart.c`

### Observe Scope

The scope service is registered through `scope`.
The demo only updates signals and runs the scope engine in periodic tasks.
Scope service support is implemented in:

- `../../dbg/scope.h`
- `../../dbg/scope.c`

## Files Supporting This Demo

### Application files

- `demo.c`
- `demo.h`
- `demo.md`

### Communication and shell

- `../../comm/comm.c`
- `../../comm/comm.h`
- `../../comm/shell.c`
- `../../comm/shell.h`

### Section / framework

- `../../section/section.c`
- `../../section/section.h`
- `../../section/perf.h`
- `../../section/platform.h`

### Debug tools

- `../../dbg/perf.c`
- `../../dbg/perf.h`
- `../../dbg/scope.c`
- `../../dbg/scope.h`

### Interface / link layer

- `../../interface/usart.c`
- `../../interface/usart.h`
- `../../interface/common/comm_addr.h`
- `../../interface/ac/gpio.c`

### GD32 BSP

- `../../../gd32g553c/bsp/bsp_usart.c`
- `../../../gd32g553c/bsp/bsp_usart.h`
- `../../../gd32g553c/bsp/bsp_timer.c`
- `../../../gd32g553c/bsp/bsp_timer.h`
- `../../../gd32g553c/bsp/bsp_gpio.c`
- `../../../gd32g553c/bsp/bsp_init.c`

### Build files

- `../../../gd32g553c/makefile`
- `../../../gd32g553c/compile.bat`
- `../../../gd32g553c/download.bat`
