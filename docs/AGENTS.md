# AI Operation Manual

## Documentation Placement

- Keep `README.md` in the repository root.
- Put new Markdown documentation files under `docs/`.
- Engineering design notes, module descriptions, protocol documents, operation manuals, and AI-facing instructions should live in `docs/`.

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
