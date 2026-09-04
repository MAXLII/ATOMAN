# AI Operation Manual

## Documentation Placement

- Keep `README.md` in the repository root.
- Put new Markdown documentation files under `docs/`.
- Engineering design notes, module descriptions, protocol documents, operation manuals, and AI-facing instructions should live in `docs/`.

## Platform Macro Boundary

- Centralize all platform-level macro recognition and capability mapping in `platform.h`. Platform-level inputs include compiler built-ins, normalized toolchain macros, MCU identity macros, host-environment macros, and simulation-platform macros.
- Build systems may define the minimum platform-selection inputs, but architecture and shared modules must not consume those identity macros directly. `platform.h` is the single boundary that translates platform identity into architecture-facing capability macros.
- Architecture code, including Section runtimes, may use only the semantic interfaces exported by `platform.h`, such as linker-section attributes, weak linkage, alignment, static assertions, system tick, reset, synchronization, and hardware capability values.
- Do not add compiler-, MCU-, host-, or simulation-specific `#if` branches outside `platform.h`. When supporting a new platform or toolchain, extend the `platform.h` contract instead of adding identity checks to architecture code.
- Business-level configuration macros are outside this rule. Product and topology selections such as `IS_BUCK`, `IS_BOOST`, and `IS_CLLC` remain in business or project configuration and must not be moved into `platform.h`.

## Git Commit Workflow

- When the user says to commit code, inspect the actual current code changes and split them into meaningful batches before committing.
- Each commit should group related changes by feature, module, or behavior instead of blindly committing every modified file together.
- Use Chinese commit messages unless the user explicitly requests another language.

## Workspace Boundaries

- Before editing code outside the current workspace/repository, ask the user for confirmation again.
- Treat requests that appear to target another repository or working directory as potentially caused by the user forgetting to switch process or context.
- Do not modify external workspaces just because a path is discoverable on disk; confirm the intended target first.

## C/H File Header Template

When creating a new `.c` or `.h` file, add the following header at the top of the file.
Replace `xxx`, `yyyy-mm-dd`, and `yyyy` with the actual file/module/project/date/year values.

The `@details` section must describe the real purpose and responsibility of the current file.
Do not copy the template mechanically. Fill in concrete module responsibilities based on the
code in that file, such as protocol parsing, control-loop calculation, HAL adaptation,
debug data capture, scheduling, or reusable algorithm support.

```c
// SPDX-License-Identifier: MIT
/**
 * @file    xxx.c
 * @brief   xxx module.
 * @details
 *          This file is part of the xxx project.
 *
 *          Module responsibilities:
 *          - xxx
 *          - xxx
 *          - xxx
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - ISR-safe path should be explicitly documented
 *          - Hardware access should be abstracted through HAL / BSP
 *
 * @author  Max.Li
 * @date    yyyy-mm-dd
 * @version 1.0.0
 *
 * Copyright (c) yyyy Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
```
