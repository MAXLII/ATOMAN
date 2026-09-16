// SPDX-License-Identifier: MIT
/**
 * @file    npc_protect.h
 * @brief   Application-owned NPC sampled-protection policy.
 * @details Current trip is derived from the controller current budget; no overvoltage trip.
 * @author  Max.Li
 * @date    2026-09-13
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. All rights reserved.
 * Licensed under the MIT License. See LICENSE in the project root.
 */
#ifndef NPC_PROTECT_H
#define NPC_PROTECT_H

/* 应用过流门限相对于控制器电流给定上限的倍数。 */
#define NPC_PROTECT_CURRENT_FACTOR (1.2f)

/* 应用过流闭锁码，保留已有故障锁存编号。 */
#define NPC_PROTECT_OVERCURRENT (12u)

#endif /* NPC_PROTECT_H */
