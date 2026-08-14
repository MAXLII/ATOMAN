// SPDX-License-Identifier: MIT
/**
 * @file    cllc_fsm.c
 * @brief   Bidirectional CLLC common FSM module.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Run init, idle, startup, run, and fault states for both directions
 *          - Validate and latch the requested direction at the idle-to-start boundary
 *          - Route lifecycle actions through CLLC HAL callbacks
 *
 *          Design notes:
 *          - C11 compatible
 *          - No dynamic memory allocation
 *          - Scheduled by REG_FSM at the common 1 ms FSM cadence
 *          - Emergency shutdown is immediate; state transition follows asynchronously
 *
 * @author  Max.Li
 * @date    2026-07-26
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "cllc_fsm.h"

#include "cllc_cfg_fsm.h"
#include "cllc_hal.h"
#include "section.h"

#include <stddef.h>

/** Transition events consumed by the section FSM implementation. */
typedef enum
{
    CLLC_FSM_EVENT_NULL = 0, /* No transition pending. */
    CLLC_FSM_EVENT_IDLE,     /* Move to idle. */
    CLLC_FSM_EVENT_STARTUP,  /* Move to startup. */
    CLLC_FSM_EVENT_RUN,      /* Move to run. */
    CLLC_FSM_EVENT_FAULT     /* Move to fault. */
} CLLC_FSM_EVENT_E;

static uint32_t fsm_event = CLLC_FSM_EVENT_NULL;                /* Pending transition event. */
static volatile CLLC_FSM_CMD_E fsm_command = CLLC_FSM_CMD_NULL; /* Command shared with protection context. */
static uint32_t startup_count = 0u;                             /* Remaining bridge-settling FSM ticks. */
static CLLC_DIRECTION_E run_direction = CLLC_DIRECTION_FORWARD; /* Direction latched at start. */
static uint8_t init_wait_logged = 0u;                           /* Prevent repeated dependency-wait log messages. */

#define CLLC_FSM_HAL (cllc_hal_get_fsm())

void cllc_fsm_set_cmd(CLLC_FSM_CMD_E command)
{
    if ((command >= CLLC_FSM_CMD_NULL) && /* Reject negative enum values from external casts. */
        (command <= CLLC_FSM_CMD_RESET))  /* Accept only public command values. */
    {
        fsm_command = command;
        if (command != CLLC_FSM_CMD_NULL)
        {
            PLECS_LOG("cllc_fsm command posted: %u\n", (unsigned int)command);
        }
    }
    else
    {
        PLECS_LOG("cllc_fsm rejected invalid command: %d\n", (int)command);
    }
}

void cllc_fsm_emergency_stop(void)
{
    PLECS_LOG("cllc_fsm emergency stop requested\n");
    cllc_hal_hard_protect_trip();
}

/** Fetch and clear one posted command. */
static CLLC_FSM_CMD_E get_command(void)
{
    CLLC_FSM_CMD_E command = fsm_command; /* One-shot command snapshot. */

    fsm_command = CLLC_FSM_CMD_NULL;
    return command;
}

/** Return whether the externally bound protection latch is active. */
static uint8_t hard_fault_is_active(void)
{
    cllc_fsm_hal_t *p_hal = CLLC_FSM_HAL; /* FSM HAL binding object. */

    if ((p_hal == NULL) ||
        (p_hal->p_latched == NULL)) /* Missing latch binding is treated as a hard fault. */
    {
        return 1u;
    }
    return (*p_hal->p_latched == 0u) ? (uint8_t)0u : (uint8_t)1u;
}

/** Init-state entry action. */
static void init_in(void)
{
    init_wait_logged = 0u;
    PLECS_LOG("cllc_fsm enter init\n");
}

/** Wait until the internal lifecycle callbacks are available. */
static void init_execute(void)
{
    cllc_fsm_hal_t *p_hal = CLLC_FSM_HAL; /* FSM HAL binding object. */

    if ((p_hal != NULL) &&
        (p_hal->p_enter_run != NULL) && /* Run preparation and direction-aware PWM enable are available. */
        (p_hal->p_exit_run != NULL))    /* A safe stop callback is available. */
    {
        PLECS_LOG("cllc_fsm init ready, goto idle\n");
        fsm_event = CLLC_FSM_EVENT_IDLE;
        return;
    }
    if (init_wait_logged == 0u)
    {
        PLECS_LOG("cllc_fsm init waiting for lifecycle HAL callbacks\n");
        init_wait_logged = 1u;
    }
}

/** Accept the init-to-idle transition. */
static uint32_t init_check(uint32_t event)
{
    return (event == CLLC_FSM_EVENT_IDLE) ? (uint32_t)CLLC_FSM_STATE_IDLE : 0u;
}

/** Init-state exit action. */
static void init_out(void)
{
    PLECS_LOG("cllc_fsm leave init\n");
}

/** Unlock measurement and callback binding while stopped. */
static void idle_in(void)
{
    cllc_cfg_unlock_direction();
    cllc_hal_unlock_binding();
    PLECS_LOG("cllc_fsm enter idle, waiting for start command\n");
}

/** Validate configuration, protection, and direction before startup. */
static void idle_execute(void)
{
    CLLC_FSM_CMD_E command = get_command();  /* Command consumed by the idle state. */
    cllc_ctrl_setpoint_t *p_setpoint = NULL; /* Staged start configuration. */

    if (command != CLLC_FSM_CMD_START)
    {
        return;
    }
    PLECS_LOG("cllc_fsm idle received start command\n");
    if (cllc_hal_is_ready() == 0u)
    {
        PLECS_LOG("cllc_fsm start rejected: HAL binding is not ready\n");
        return;
    }
    if (cllc_cfg_is_ready() == 0u)
    {
        PLECS_LOG("cllc_fsm start rejected: timing or setpoint buffer is not ready\n");
        return;
    }
    if (hard_fault_is_active() != 0u)
    {
        PLECS_LOG("cllc_fsm start rejected: hard fault is active\n");
        fsm_event = CLLC_FSM_EVENT_FAULT;
        return;
    }
    p_setpoint = cllc_cfg_get_p_building();
    if ((p_setpoint == NULL) ||                             /* Start requires a complete staged configuration. */
        (p_setpoint->direction < CLLC_DIRECTION_FORWARD) || /* Reject negative enum values. */
        (p_setpoint->direction >= CLLC_DIRECTION_MAX))      /* Start requires one defined direction. */
    {
        PLECS_LOG("cllc_fsm start rejected: direction is invalid\n");
        return;
    }

    cllc_cfg_set_run_allowed(1u);
    cllc_cfg_publish_building();
    cllc_cfg_sync_building_to_active();
    p_setpoint = cllc_cfg_get_p_active();
    run_direction = p_setpoint->direction;
    PLECS_LOG("cllc_fsm start accepted: direction=%u, goto startup\n", (unsigned int)run_direction);
    fsm_event = CLLC_FSM_EVENT_STARTUP;
}

/** Accept idle transitions to startup or fault. */
static uint32_t idle_check(uint32_t event)
{
    if (event == CLLC_FSM_EVENT_STARTUP)
    {
        return (uint32_t)CLLC_FSM_STATE_STARTUP;
    }
    if (event == CLLC_FSM_EVENT_FAULT)
    {
        return (uint32_t)CLLC_FSM_STATE_FAULT;
    }
    return 0u;
}

/** Freeze HAL bindings before bridge startup. */
static void idle_out(void)
{
    cllc_cfg_lock_direction();
    cllc_hal_lock_binding();
    PLECS_LOG("cllc_fsm leave idle\n");
}

/** Prepare algorithms and enable only the bridge direction latched in idle. */
static void startup_in(void)
{
    cllc_fsm_hal_t *p_hal = CLLC_FSM_HAL; /* FSM lifecycle callback owner. */

    startup_count = cllc_cfg_get_startup_delay_ticks();
    PLECS_LOG("cllc_fsm enter startup: direction=%u, delay_ticks=%lu\n",
              (unsigned int)run_direction,
              (unsigned long)startup_count);
    if ((p_hal != NULL) &&
        (p_hal->p_enter_run != NULL)) /* Callback was validated before leaving idle. */
    {
        p_hal->p_enter_run(run_direction);
    }
}

/** Enforce stop/fault commands and wait the configured bridge-settling delay. */
static void startup_execute(void)
{
    CLLC_FSM_CMD_E command = get_command(); /* Command consumed during startup. */
    cllc_fsm_hal_t *p_hal = CLLC_FSM_HAL;   /* FSM lifecycle callback owner. */

    if (hard_fault_is_active() != 0u)
    {
        PLECS_LOG("cllc_fsm startup interrupted by hard fault\n");
        if ((p_hal != NULL) &&
            (p_hal->p_exit_run != NULL)) /* Stop power transfer before entering fault. */
        {
            p_hal->p_exit_run();
        }
        cllc_cfg_set_run_allowed(0u);
        cllc_cfg_publish_building();
        fsm_event = CLLC_FSM_EVENT_FAULT;
        return;
    }
    if (command == CLLC_FSM_CMD_STOP)
    {
        PLECS_LOG("cllc_fsm startup received stop, goto idle\n");
        if ((p_hal != NULL) &&
            (p_hal->p_exit_run != NULL)) /* Cancel an in-progress startup safely. */
        {
            p_hal->p_exit_run();
        }
        cllc_cfg_set_run_allowed(0u);
        cllc_cfg_publish_building();
        fsm_event = CLLC_FSM_EVENT_IDLE;
        return;
    }

    if (startup_count > 0u)
    {
        startup_count--;
    }
    if (startup_count == 0u)
    {
        PLECS_LOG("cllc_fsm startup delay complete, goto run\n");
        fsm_event = CLLC_FSM_EVENT_RUN;
    }
}

/** Accept startup transitions to run, idle, or fault. */
static uint32_t startup_check(uint32_t event)
{
    if (event == CLLC_FSM_EVENT_RUN)
    {
        return (uint32_t)CLLC_FSM_STATE_RUN;
    }
    if (event == CLLC_FSM_EVENT_IDLE)
    {
        return (uint32_t)CLLC_FSM_STATE_IDLE;
    }
    if (event == CLLC_FSM_EVENT_FAULT)
    {
        return (uint32_t)CLLC_FSM_STATE_FAULT;
    }
    return 0u;
}

/** Startup-state exit action. */
static void startup_out(void)
{
    PLECS_LOG("cllc_fsm leave startup\n");
}

/** Run-state entry action. */
static void run_in(void)
{
    PLECS_LOG("cllc_fsm enter run: direction=%u\n", (unsigned int)run_direction);
}

/** Monitor stop commands and the asynchronous protection latch. */
static void run_execute(void)
{
    CLLC_FSM_CMD_E command = get_command(); /* Command consumed by the run state. */

    if (hard_fault_is_active() != 0u)
    {
        PLECS_LOG("cllc_fsm run interrupted by hard fault\n");
        fsm_event = CLLC_FSM_EVENT_FAULT;
        return;
    }
    if (command == CLLC_FSM_CMD_STOP)
    {
        PLECS_LOG("cllc_fsm run received stop, goto idle\n");
        fsm_event = CLLC_FSM_EVENT_IDLE;
    }
}

/** Accept run transitions to idle or fault. */
static uint32_t run_check(uint32_t event)
{
    if (event == CLLC_FSM_EVENT_IDLE)
    {
        return (uint32_t)CLLC_FSM_STATE_IDLE;
    }
    if (event == CLLC_FSM_EVENT_FAULT)
    {
        return (uint32_t)CLLC_FSM_STATE_FAULT;
    }
    return 0u;
}

/** Disable power transfer whenever run ends. */
static void run_out(void)
{
    cllc_fsm_hal_t *p_hal = CLLC_FSM_HAL; /* FSM lifecycle callback owner. */

    PLECS_LOG("cllc_fsm leave run\n");
    if ((p_hal != NULL) &&
        (p_hal->p_exit_run != NULL)) /* Stop either direction through one common path. */
    {
        p_hal->p_exit_run();
    }

    cllc_cfg_set_run_allowed(0u);
    cllc_cfg_publish_building();
}

/** Keep PWM disabled while faulted. */
static void fault_in(void)
{
    cllc_cfg_lock_direction();
    cllc_hal_pwm_disable();
    cllc_cfg_set_run_allowed(0u);
    cllc_cfg_publish_building();
    PLECS_LOG("cllc_fsm enter fault, PWM disabled\n");
}

/** Clear the latch only after an explicit reset command. */
static void fault_execute(void)
{
    if (get_command() == CLLC_FSM_CMD_RESET)
    {
        PLECS_LOG("cllc_fsm fault reset accepted, goto idle\n");
        cllc_hal_hard_protect_clear();
        fsm_event = CLLC_FSM_EVENT_IDLE;
    }
}

/** Accept fault reset back to idle. */
static uint32_t fault_check(uint32_t event)
{
    return (event == CLLC_FSM_EVENT_IDLE) ? (uint32_t)CLLC_FSM_STATE_IDLE : 0u;
}

/** Fault-state exit action. */
static void fault_out(void)
{
    PLECS_LOG("cllc_fsm leave fault\n");
}

REG_FSM(CLLC_FSM, CLLC_FSM_STATE_INIT, fsm_event,
        FSM_ENTRY(CLLC_FSM_STATE_INIT, init_in, init_execute, init_check, init_out),
        FSM_ENTRY(CLLC_FSM_STATE_IDLE, idle_in, idle_execute, idle_check, idle_out),
        FSM_ENTRY(CLLC_FSM_STATE_STARTUP, startup_in, startup_execute, startup_check, startup_out),
        FSM_ENTRY(CLLC_FSM_STATE_RUN, run_in, run_execute, run_check, run_out),
        FSM_ENTRY(CLLC_FSM_STATE_FAULT, fault_in, fault_execute, fault_check, fault_out), )

CLLC_RUN_STATE_E cllc_fsm_get_run_state(void)
{
    CLLC_FSM_STATE_E state = (CLLC_FSM_STATE_E)FSM_GET_STATE(CLLC_FSM); /* Internal section FSM state. */

    switch (state)
    {
    case CLLC_FSM_STATE_INIT:
        return CLLC_RUN_STATE_INIT;
    case CLLC_FSM_STATE_IDLE:
        return CLLC_RUN_STATE_IDLE;
    case CLLC_FSM_STATE_STARTUP:
        return CLLC_RUN_STATE_STARTUP;
    case CLLC_FSM_STATE_RUN:
        return CLLC_RUN_STATE_RUN;
    case CLLC_FSM_STATE_FAULT:
        return CLLC_RUN_STATE_FAULT;
    default:
        return CLLC_RUN_STATE_INIT;
    }
}

CLLC_DIRECTION_E cllc_fsm_get_direction(void)
{
    return run_direction;
}
