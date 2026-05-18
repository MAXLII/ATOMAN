# Demo Guide

## Overview

This demo is a runnable example for the current GD32 workspace.
It is built as a fixed `ISP` + `IS_DC` project and uses the section-based framework already integrated into this repository.

Each `demo_xxxx.c` file is a single, clean example for one registration area.
The examples keep their own state as file-scope `static` variables and avoid depending on other demo files.

## Demo Files

- `demo.c`: `REG_INIT`
- `demo_task.c`: `REG_TASK`, `REG_TASK_MS`
- `demo_shell.c`: `REG_SHELL_VAR`, `REG_SHELL_CMD`
- `demo_comm.c`: `REG_COMM`
- `demo_perf.c`: `REG_PERF_RECORD`, `PERF_START`, `PERF_END`
- `demo_scope.c`: `REG_SCOPE_EX`, `SCOPE_RUN`
- `demo_trace.c`: `DBG_TRACE_MARK`
- `demo_interrupt.c`: `REG_INTERRUPT`
- `demo_sfra.c`: `REG_SFRA`
- `demo.h`: command IDs and shared communication payload type

## Communication Example

`demo_comm.c` registers one loopback command:

- `cmd_set = 0x30`
- `cmd_word = 0x01`

The handler initializes a local payload object, copies `MIN(data_len, sizeof(local_struct))` bytes from the received payload, and replies with an ACK using the same `cmd_set` and `cmd_word`.

## Shell Example

`demo_shell.c` registers:

- `DEMO_SHELL_COUNTER`
- `DEMO_SHELL_GAIN`
- `DEMO_SHELL_PING`

The shell example only demonstrates shell variable and command registration.

## Performance Example

`demo_perf.c` registers:

- `DEMO_PERF_LOOP`
- `DEMO_PERF_RUN`
- `demo_perf_shell`
- `demo_perf_task`

The shell command measures one local workload. The periodic task measures one local workload. It does not read task records from other demo files.

## Scope Example

`demo_scope.c` registers one scope:

- name: `demo_scope_wave`
- sample period: `10 ms`
- channel count: `2`

The task updates sine and cosine channels, then calls `SCOPE_RUN(demo_scope_wave)`.

## SFRA Example

`demo_sfra.c` registers one PI-controller SFRA object:

- name: `demo_sfra`
- sample period: `100 us`
- sweep range: `10 Hz` to `5000 Hz`

The fast task samples the PI controller. The 1 ms task runs the SFRA background state machine.

## Build

Build from `gd32g553c`:

```bat
compile.bat
```

The same demo source files are also listed in:

- `../../../gd32g553c/makefile`
- `../../../gd32g553c/mdk/gd32g553.uvprojx`
- `../../../hc32f334/ac/keil_mdk/hc32f334_ac.uvprojx`
