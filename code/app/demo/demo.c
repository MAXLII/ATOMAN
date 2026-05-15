#include "demo.h"

#include "comm.h"
#include "comm_addr.h"
#include "gpio.h"
#include "perf.h"
#include "pi_tustin.h"
#include "pr.h"
#include "scope.h"
#include "section.h"
#include "shell.h"
#include "sfra.h"
#include "sogi.h"
#include "trace.h"
#include "usart.h"

#include <math.h>
#include <string.h>

/*
 * This file is a runnable section-framework demo.
 *
 * Read this file with SECTION_E in mind:
 * - SECTION_INIT       -> REG_INIT
 * - SECTION_TASK       -> REG_TASK_MS
 * - SECTION_SHELL      -> REG_SHELL_VAR / REG_SHELL_CMD
 * - SECTION_COMM       -> REG_COMM
 * - SECTION_LINK       -> provided by interface/usart.c
 * - SECTION_INTERRUPT  -> REG_INTERRUPT + section_interrupt()
 * - SECTION_PERF       -> REG_PERF_RECORD / PERF_START / PERF_END
 * - SECTION_SCOPE      -> REG_SCOPE_EX / SCOPE_RUN
 * - SECTION_TRACE      -> DBG_TRACE_BIND_TIME / DBG_TRACE_MARK
 * - SECTION_COMM_ROUTE -> extension point, not used in this demo
 *
 * Runtime path:
 * main() -> section_init() -> REG_INIT callbacks
 * main() loop -> run_task() -> registered tasks
 * USART RX -> section link task -> shell_run()/comm_run()
 *
 * File layout:
 * - Configuration / Runtime State
 * - Section Objects / SFRA Model Contexts
 * - Local Helpers / Initialization
 * - Shell, Perf, Task, Interrupt, Trace, Scope, SFRA, Comm handlers
 * - Section Registration Table
 *
 * How to read this demo:
 * This file is intentionally written like an executable manual.  The goal is
 * not to hide the section framework behind compact abstractions, but to show
 * where each type of framework object is declared, initialized, updated, and
 * finally registered into its section.
 *
 * The demo contains several independent feature slices:
 * - shell variables and shell commands
 * - command-frame receive and reply examples
 * - task scheduling and task-period measurement
 * - software performance records
 * - software scope capture
 * - debug trace marks
 * - software interrupt dispatch
 * - SFRA examples for PI, PR, SOGI, PI+L open loop, and PI+L closed loop
 *
 * Most examples keep their state in file-scope static variables because this
 * is a board-level integration demo.  Real product modules should usually move
 * their state into their own module files and expose only the required public
 * interface.
 */

EXT_LINK(USART0_LINK); /* Imports the USART0 link object registered by the board/interface layer. */

/* -------------------------------------------------------------------------- */
/* Configuration                                                              */
/* -------------------------------------------------------------------------- */

/*
 * This block collects the compile-time parameters used by the demo.
 *
 * Keep constants here when they describe the demonstration scenario itself:
 * loop count, SFRA sampling frequency, default sweep range, controller gains,
 * and synthetic plant parameters.
 *
 * In an actual product project, constants like current-loop bandwidth,
 * inductance, PWM frequency, and controller limits normally belong to the
 * power-stage or control-loop module rather than to a demo file.
 */
#define DEMO_MATH_LOOP_COUNT (960u) /* Number of inner iterations used by Flash/RAM math workload demos. */

/*
 * SFRA sampling settings.
 *
 * REG_TASK(1, ...) in this project runs once per 100 us scheduler tick, so the
 * corresponding SFRA ISR-model task is treated as a 10 kHz sampling path.
 * The 1 ms SFRA task drains the sample FIFO and calculates magnitude/phase in
 * the background.
 */
#define DEMO_SFRA_ISR_FREQ_HZ (10000.0f)                         /* SFRA fast-path sampling frequency, in Hz. */
#define DEMO_SFRA_SAMPLE_PERIOD_S (1.0f / DEMO_SFRA_ISR_FREQ_HZ) /* SFRA fast-path sample period, in seconds. */
#define DEMO_SFRA_INJECT_AMPLITUDE (1.0f)                        /* Default injected sine amplitude for every demo loop. */
#define DEMO_SFRA_FREQ_START_HZ (10.0f)                          /* Default first sweep frequency shown in the SFRA UI. */
#define DEMO_SFRA_FREQ_END_HZ (5000.0f)                          /* Default last sweep frequency shown in the SFRA UI. */

/*
 * Plain PI demonstration parameters.
 *
 * demo_sfra measures only the PI controller body.  It is useful for checking
 * whether the SFRA service and plotting path are alive before connecting a
 * plant model.
 */
#define DEMO_SFRA_PI_KP (1.0f)        /* Proportional gain for the plain PI-only scan. */
#define DEMO_SFRA_PI_KI (400.0f)      /* Integral gain for the plain PI-only scan. */
#define DEMO_SFRA_PI_UP_LMT (100.0f)  /* Upper output saturation for the plain PI controller. */
#define DEMO_SFRA_PI_DN_LMT (-100.0f) /* Lower output saturation for the plain PI controller. */

/*
 * PI + inductor demonstration parameters.
 *
 * The PI gains are designed from an ideal current-loop plant 1/(sL):
 *   kp = sin(PM) * wc * L
 *   ki = kp * wc / tan(PM)
 *
 * These values are used by both:
 * - demo_pi_l_sfra: open-loop PI * 1/(sL) steady-state response
 * - demo_pi_l_closed_sfra: closed-loop current response simulation
 */
#define DEMO_SFRA_PI_L_INDUCTANCE_H (5.0e-6f)                                                                   /* Simulated current-loop inductor value, in henry. */
#define DEMO_SFRA_PI_L_BW_HZ (800.0f)                                                                           /* Target current-loop bandwidth used for PI design. */
#define DEMO_SFRA_PI_L_PM_RAD (3.14159265358979323846f / 3.0f)                                                  /* Target phase margin, 60 degrees expressed in radians. */
#define DEMO_SFRA_PI_L_WC_RAD_S (2.0f * 3.14159265358979323846f * DEMO_SFRA_PI_L_BW_HZ)                         /* Target crossover angular frequency. */
#define DEMO_SFRA_PI_L_KP (sinf(DEMO_SFRA_PI_L_PM_RAD) * DEMO_SFRA_PI_L_WC_RAD_S * DEMO_SFRA_PI_L_INDUCTANCE_H) /* Designed PI proportional gain for PI+L demos. */
#define DEMO_SFRA_PI_L_KI (DEMO_SFRA_PI_L_KP * DEMO_SFRA_PI_L_WC_RAD_S / tanf(DEMO_SFRA_PI_L_PM_RAD))           /* Designed PI integral gain for PI+L demos. */
#define DEMO_SFRA_PI_L_UP_LMT (100.0f)                                                                          /* Upper voltage command limit for PI+L demos. */
#define DEMO_SFRA_PI_L_DN_LMT (-100.0f)                                                                         /* Lower voltage command limit for PI+L demos. */

/*
 * PI + capacitor closed-loop demonstration parameters.
 *
 * The PI gains follow the same ideal integrator-plant design used by PI+L,
 * but the plant is 1/(sC) and the PI output is capacitor current:
 *   kp = sin(PM) * wc * C
 *   ki = kp * wc / tan(PM)
 */
#define DEMO_SFRA_PI_C_ISR_FREQ_HZ (30000.0f)                                                                    /* Hardware interrupt SFRA sampling frequency, in Hz. */
#define DEMO_SFRA_PI_C_SAMPLE_PERIOD_S (1.0f / DEMO_SFRA_PI_C_ISR_FREQ_HZ)                                       /* Hardware interrupt SFRA sample period, in seconds. */
#define DEMO_SFRA_PI_C_CAPACITANCE_F (4.0f * 470.0e-6f)                                                          /* Simulated DC-link capacitor value, in farad. */
#define DEMO_SFRA_PI_C_BW_HZ (DEMO_SFRA_PI_C_ISR_FREQ_HZ / 7.0f / 5.0f)                                          /* Target voltage-loop bandwidth. */
#define DEMO_SFRA_PI_C_PM_RAD (3.14159265358979323846f / 3.0f)                                                   /* Target phase margin, 60 degrees expressed in radians. */
#define DEMO_SFRA_PI_C_WC_RAD_S (2.0f * 3.14159265358979323846f * DEMO_SFRA_PI_C_BW_HZ)                          /* Target crossover angular frequency. */
#define DEMO_SFRA_PI_C_KP (sinf(DEMO_SFRA_PI_C_PM_RAD) * DEMO_SFRA_PI_C_WC_RAD_S * DEMO_SFRA_PI_C_CAPACITANCE_F) /* Designed PI proportional gain for PI+C demo. */
#define DEMO_SFRA_PI_C_KI (DEMO_SFRA_PI_C_KP * DEMO_SFRA_PI_C_WC_RAD_S / tanf(DEMO_SFRA_PI_C_PM_RAD))            /* Designed PI integral gain for PI+C demo. */
#define DEMO_SFRA_PI_C_UP_LMT (100.0f)                                                                           /* Upper current command limit for PI+C demo. */
#define DEMO_SFRA_PI_C_DN_LMT (-100.0f)                                                                          /* Lower current command limit for PI+C demo. */

/*
 * PR and SOGI demonstration parameters.
 *
 * These examples are present so that SFRA can scan reusable algorithm modules,
 * not only PI loops.  The demo keeps them small and fixed so that the focus
 * stays on the framework usage.
 */
#define DEMO_SFRA_PR_W0 (2.0f * 3.14159265358979323846f * 50.0f)  /* PR resonant angular frequency, 50 Hz. */
#define DEMO_SFRA_PR_WC (2.0f * 3.14159265358979323846f * 1.0f)   /* PR resonant bandwidth angular frequency. */
#define DEMO_SFRA_PR_KP (1.0f)                                    /* PR proportional gain. */
#define DEMO_SFRA_PR_KR (20.0f)                                   /* PR resonant gain. */
#define DEMO_SFRA_PR_UP_LMT (100.0f)                              /* Upper PR output saturation. */
#define DEMO_SFRA_PR_DN_LMT (-100.0f)                             /* Lower PR output saturation. */
#define DEMO_SFRA_SOGI_W (2.0f * 3.14159265358979323846f * 50.0f) /* SOGI nominal angular frequency, 50 Hz. */
#define DEMO_SFRA_SOGI_K (1.41421356237f)                         /* SOGI damping/coefficient setting, sqrt(2). */

/* -------------------------------------------------------------------------- */
/* Runtime State                                                              */
/* -------------------------------------------------------------------------- */

/*
 * General demo state.
 *
 * These variables back shell commands, command-frame examples, periodic tasks,
 * and simple board-visible behavior such as relay/LED state.  They are kept
 * file-local so the demo remains self-contained.
 */
static uint8_t s_demo_led_mask = 0x01u;                 /* Bit mask mirrored to the demo relay/GPIO outputs. */
static uint32_t s_demo_counter = 0u;                    /* 10 ms heartbeat counter exposed through shell and COMM examples. */
static int32_t s_demo_last_cmd = 0;                     /* Last accepted demo command encoded as cmd_set << 8 | cmd_word. */
static float s_demo_gain = 1.0f;                        /* Editable shell float used as a simple variable-registration example. */
static uint8_t s_demo_init_count = 0u;                  /* Number of demo init callbacks that have completed. */
static uint8_t s_demo_boot_ready = 0u;                  /* Flag set by the late init banner to show boot sequencing. */
static uint32_t s_demo_perf_spin = 2000u;               /* Loop count used by DEMO_PERF manual timing command. */
static uint32_t s_demo_last_tick = 0u;                  /* Last raw performance-counter value sampled by demo tasks/commands. */
static uint32_t s_demo_perf_acc = 0u;                   /* Accumulator result from the manual performance workload. */
static volatile uint32_t s_demo_flash_math_result = 0u; /* Flash workload result kept volatile so the compiler keeps the work. */
static volatile uint32_t s_demo_ram_math_result = 0u;   /* RAM workload result kept volatile so the compiler keeps the work. */
static uint32_t s_demo_period_counter = 0u;             /* Number of times the 20 ms period-measurement task has run. */
static uint32_t s_demo_scope_basic_tick = 0u;           /* Sample index for the 10 ms two-channel scope demo. */
static uint32_t s_demo_scope_fast_tick = 0u;            /* Sample index for the 100 us multi-channel scope demo. */
static uint32_t s_demo_scope_slow_tick = 0u;            /* Sample index for the 1 ms three-channel scope demo. */
static uint32_t s_demo_aux_counter = 0u;                /* Shared counter touched by perf-load tasks to create visible activity. */
static uint32_t s_demo_interrupt_counter = 0u;          /* Counter incremented by registered interrupt callbacks. */

/*
 * SFRA signal state.
 *
 * Each REG_SFRA object creates two global float ports:
 *   <name>_inject
 *   <name>_collect
 *
 * The static variables below are the local model/controller signals that feed
 * those ports.  Keeping them explicit is useful during debugging: they can be
 * watched in a debugger or temporarily connected to Scope/Trace without
 * unpacking a large context object.
 */
static float s_demo_sfra_pi_ref = 0.0f;              /* Plain PI reference input driven by demo_sfra_inject. */
static float s_demo_sfra_pi_act = 0.0f;              /* Plain PI feedback input, held at zero for open-loop controller scan. */
static float s_demo_sfra_pi_out = 0.0f;              /* Plain PI output copied into demo_sfra_collect. */
static float s_demo_sfra_pi_l_ref = 0.0f;            /* Open-loop PI+L reference signal used for visibility/debugging. */
static float s_demo_sfra_pi_l_act = 0.0f;            /* Open-loop PI+L feedback signal, held at zero. */
static float s_demo_sfra_pi_l_voltage = 0.0f;        /* Open-loop PI voltage command generated from analytic PI response. */
static float s_demo_sfra_pi_l_closed_ref = 0.0f;     /* Closed-loop PI+L reference driven by demo_pi_l_closed_sfra_inject. */
static float s_demo_sfra_pi_l_closed_act = 0.0f;     /* Closed-loop PI+L feedback current sampled from the plant model. */
static float s_demo_sfra_pi_l_closed_voltage = 0.0f; /* Closed-loop PI voltage command applied to the delayed inductor model. */
static float s_demo_sfra_pi_c_closed_ref = 0.0f;     /* Closed-loop PI+C voltage reference driven by demo_pi_c_closed_sfra_inject. */
static float s_demo_sfra_pi_c_closed_act = 0.0f;     /* Closed-loop PI+C feedback voltage sampled from the capacitor model. */
static float s_demo_sfra_pi_c_closed_current = 0.0f; /* Closed-loop PI current command applied to the delayed capacitor model. */
static float s_demo_sfra_pr_ref = 0.0f;              /* PR reference input driven by demo_pr_sfra_inject. */
static float s_demo_sfra_pr_act = 0.0f;              /* PR feedback input, held at zero for controller-only scan. */
static float s_demo_sfra_pr_out = 0.0f;              /* PR output copied into demo_pr_sfra_collect. */
static float s_demo_sfra_sogi_d_in = 0.0f;           /* SOGI-D scan input driven by demo_sogi_d_sfra_inject. */
static float s_demo_sfra_sogi_d_out = 0.0f;          /* SOGI D-path output copied into demo_sogi_d_sfra_collect. */
static float s_demo_sfra_sogi_q_in = 0.0f;           /* SOGI-Q scan input driven by demo_sogi_q_sfra_inject. */
static float s_demo_sfra_sogi_q_out = 0.0f;          /* SOGI Q-path output copied into demo_sogi_q_sfra_collect. */

/*
 * Symbols owned by other modules.
 *
 * section_perf_record_demo_task is generated by REG_TASK_MS when task
 * performance recording is enabled.  sys_tick_100us is the time base used by
 * the trace demo.
 */
extern section_perf_record_t section_perf_record_demo_task; /* Auto-generated perf record for demo_task when task perf is enabled. */
extern volatile uint32_t sys_tick_100us;                    /* System 100 us tick used as the debug trace time base. */

/*
 * Last received or transmitted loopback frame.
 *
 * The loopback command handler copies inbound payload into this object and
 * sends it back as an ACK payload.  The shell send command also reuses it as a
 * convenient frame template.
 */
static demo_comm_frame_t s_demo_last_frame = {
    .counter = 0u,          /* Last frame counter value accepted or transmitted by loopback demo. */
    .led_mask = 0x01u,      /* Last frame output mask used by shell send and COMM loopback. */
    .temperature_x10 = 250, /* Example temperature payload, stored in 0.1 degree units. */
    .reserved = 0u,         /* Compatibility filler for the demo frame layout. */
};

/* -------------------------------------------------------------------------- */
/* SFRA Model Contexts                                                        */
/* -------------------------------------------------------------------------- */

/*
 * Context objects bind reusable algorithms to one SFRA loop.
 *
 * The SFRA core itself only knows about two float ports: inject and collect.
 * The demo context records which controller instance, plant state, and local
 * signal variables belong to a particular loop.  Frequency-prepare callbacks
 * receive these contexts so each sweep point can start from a clean state.
 */

/*
 * Plain PI context:
 * inject -> PI reference, feedback forced to zero, collect <- PI output.
 */
typedef struct
{
    pi_tustin_t *p_pi;     /* PI instance reset and executed by this SFRA loop. */
    float *p_sfra_collect; /* Address of the REG_SFRA collect port to clear per frequency. */
    float *p_pi_out;       /* Visible PI output variable to clear per frequency. */
} demo_sfra_ctx_t;

/*
 * Inductor model state.
 *
 * current_a is the simulated plant output.
 * voltage_cmd_z1 is the previous tick's voltage command.  It models the common
 * one-sample delay between controller calculation and plant response.
 */
typedef struct
{
    float current_a;      /* Simulated inductor current, in ampere. */
    float voltage_cmd_z1; /* Previous-sample voltage command used to model one-tick delay. */
} demo_sfra_inductor_t;

/*
 * Capacitor model state.
 *
 * voltage_v is the simulated plant output.
 * current_cmd_z1 is the previous tick's current command.  It mirrors the same
 * one-sample command-to-plant delay used by the PI+L closed-loop model.
 */
typedef struct
{
    float voltage_v;      /* Simulated capacitor voltage, in volt. */
    float current_cmd_z1; /* Previous-sample current command used to model one-tick delay. */
} demo_sfra_capacitor_t;

/* Tiny complex type used only by the analytic open-loop PI+L example. */
typedef struct
{
    float real; /* Real component of a small local complex value. */
    float imag; /* Imaginary component of a small local complex value. */
} demo_sfra_complex_t;

/*
 * PI + inductor context.
 *
 * The same context type is used by open-loop and closed-loop PI+L examples:
 * - open-loop uses analytic steady-state response for collect
 * - closed-loop uses the time-domain inductor state and PI feedback
 */
typedef struct
{
    pi_tustin_t *p_pi;                /* PI controller object bound to this PI+L scan. */
    demo_sfra_inductor_t *p_inductor; /* Simulated inductor state bound to this scan. */
    float *p_sfra_collect;            /* Address of the REG_SFRA collect port. */
    float *p_voltage;                 /* Visible PI voltage command signal. */
    float *p_ref;                     /* Visible PI reference signal. */
    float *p_act;                     /* Visible PI feedback/current signal. */
} demo_pi_l_sfra_ctx_t;

/* PI + capacitor closed-loop context. */
typedef struct
{
    pi_tustin_t *p_pi;                /* PI controller object bound to this PI+C scan. */
    demo_sfra_capacitor_t *p_cap;     /* Simulated capacitor state bound to this scan. */
    float *p_sfra_collect;            /* Address of the REG_SFRA collect port. */
    float *p_current;                 /* Visible PI current command signal. */
    float *p_ref;                     /* Visible PI voltage reference signal. */
    float *p_act;                     /* Visible PI voltage feedback signal. */
} demo_pi_c_sfra_ctx_t;

/* PR context: inject -> PR reference, feedback forced to zero, collect <- PR output. */
typedef struct
{
    pr_t *p_pr;            /* PR controller instance reset and executed by this scan. */
    float *p_sfra_collect; /* Address of the REG_SFRA collect port for the PR scan. */
    float *p_pr_out;       /* Visible PR output variable to clear per frequency. */
} demo_pr_sfra_ctx_t;

typedef enum
{
    DEMO_SOGI_OUTPUT_D = 0, /* Select the in-phase SOGI output osg_u[0]. */
    DEMO_SOGI_OUTPUT_Q = 1, /* Select the quadrature SOGI output osg_qu[0]. */
} demo_sogi_output_e;

typedef struct
{
    sogi_t *p_sogi;                /* SOGI filter instance bound to this scan. */
    demo_sogi_output_e output_sel; /* Which SOGI output is routed to SFRA collect. */
    float *p_input;                /* Visible input signal driven by the SFRA inject port. */
    float *p_sfra_collect;         /* Address of the REG_SFRA collect port. */
    float *p_output;               /* Visible output signal for debugger/scope inspection. */
} demo_sogi_sfra_ctx_t;

static pi_tustin_t s_demo_sfra_pi;                            /* Plain PI controller object measured by demo_sfra. */
static pi_tustin_t s_demo_sfra_pi_l;                          /* PI controller object used by the open-loop PI+L scan. */
static pi_tustin_t s_demo_sfra_pi_l_closed;                   /* PI controller object used by the closed-loop PI+L scan. */
static pi_tustin_t s_demo_sfra_pi_c_closed;                   /* PI controller object used by the closed-loop PI+C scan. */
static pr_t s_demo_sfra_pr;                                   /* PR controller object measured by demo_pr_sfra. */
static sogi_t s_demo_sogi_d;                                  /* SOGI instance used when scanning the D/in-phase output. */
static sogi_t s_demo_sogi_q;                                  /* SOGI instance used when scanning the Q/quadrature output. */
static demo_sfra_inductor_t s_demo_sfra_pi_l_inductor;        /* Open-loop plant state. */
static demo_sfra_inductor_t s_demo_sfra_pi_l_closed_inductor; /* Closed-loop plant state. */
static demo_sfra_capacitor_t s_demo_sfra_pi_c_closed_cap;     /* Closed-loop PI+C plant state. */
static demo_sfra_ctx_t s_demo_sfra_ctx;                       /* Plain PI SFRA context. */
static demo_pi_l_sfra_ctx_t s_demo_pi_l_sfra_ctx;             /* Open-loop PI+L SFRA context. */
static demo_pi_l_sfra_ctx_t s_demo_pi_l_closed_sfra_ctx;      /* Closed-loop PI+L SFRA context. */
static demo_pi_c_sfra_ctx_t s_demo_pi_c_closed_sfra_ctx;      /* Closed-loop PI+C SFRA context. */
static demo_pr_sfra_ctx_t s_demo_pr_sfra_ctx;                 /* PR SFRA context. */
static demo_sogi_sfra_ctx_t s_demo_sogi_d_sfra_ctx;           /* SOGI D-path SFRA context. */
static demo_sogi_sfra_ctx_t s_demo_sogi_q_sfra_ctx;           /* SOGI Q-path SFRA context. */

/*
 * Reset the plain PI scan before each frequency point.
 *
 * This guarantees every point starts from the same controller state and avoids
 * history leaking from the previous frequency into the next one.
 */
static void demo_sfra_prepare_freq(void *p_ctx)
{
    demo_sfra_ctx_t *p_ctx_sfra = (demo_sfra_ctx_t *)p_ctx; /* Typed PI-only context passed from REG_SFRA(). */

    if (p_ctx_sfra == NULL)
    {
        return;
    }

    if (p_ctx_sfra->p_pi != NULL)
    {
        pi_tustin_reset(p_ctx_sfra->p_pi);
    }

    if (p_ctx_sfra->p_sfra_collect != NULL)
    {
        *(p_ctx_sfra->p_sfra_collect) = 0.0f;
    }

    if (p_ctx_sfra->p_pi_out != NULL)
    {
        *(p_ctx_sfra->p_pi_out) = 0.0f;
    }
}

/*
 * Reset PI+L open-loop or closed-loop state before each frequency point.
 *
 * The same callback is used for both loops because both need the same reset:
 * - PI internal history is cleared
 * - inductor current is set to zero
 * - previous voltage command is set to zero
 * - SFRA collect and visible model signals are cleared
 */
static void demo_pi_l_sfra_prepare_freq(void *p_ctx)
{
    demo_pi_l_sfra_ctx_t *p_ctx_sfra = (demo_pi_l_sfra_ctx_t *)p_ctx; /* Typed PI+L context for open-loop or closed-loop reset. */

    if (p_ctx_sfra == NULL)
    {
        return;
    }

    if (p_ctx_sfra->p_pi != NULL)
    {
        pi_tustin_reset(p_ctx_sfra->p_pi);
    }

    if (p_ctx_sfra->p_inductor != NULL)
    {
        p_ctx_sfra->p_inductor->current_a = 0.0f;
        p_ctx_sfra->p_inductor->voltage_cmd_z1 = 0.0f;
    }

    if (p_ctx_sfra->p_sfra_collect != NULL)
    {
        *(p_ctx_sfra->p_sfra_collect) = 0.0f;
    }

    if (p_ctx_sfra->p_voltage != NULL)
    {
        *(p_ctx_sfra->p_voltage) = 0.0f;
    }

    if (p_ctx_sfra->p_ref != NULL)
    {
        *(p_ctx_sfra->p_ref) = 0.0f;
    }

    if (p_ctx_sfra->p_act != NULL)
    {
        *(p_ctx_sfra->p_act) = 0.0f;
    }
}

/*
 * Reset PI+C closed-loop state before each frequency point.
 */
static void demo_pi_c_sfra_prepare_freq(void *p_ctx)
{
    demo_pi_c_sfra_ctx_t *p_ctx_sfra = (demo_pi_c_sfra_ctx_t *)p_ctx; /* Typed PI+C context passed from REG_SFRA(). */

    if (p_ctx_sfra == NULL)
    {
        return;
    }

    if (p_ctx_sfra->p_pi != NULL)
    {
        pi_tustin_reset(p_ctx_sfra->p_pi);
    }

    if (p_ctx_sfra->p_cap != NULL)
    {
        p_ctx_sfra->p_cap->voltage_v = 0.0f;
        p_ctx_sfra->p_cap->current_cmd_z1 = 0.0f;
    }

    if (p_ctx_sfra->p_sfra_collect != NULL)
    {
        *(p_ctx_sfra->p_sfra_collect) = 0.0f;
    }

    if (p_ctx_sfra->p_current != NULL)
    {
        *(p_ctx_sfra->p_current) = 0.0f;
    }

    if (p_ctx_sfra->p_ref != NULL)
    {
        *(p_ctx_sfra->p_ref) = 0.0f;
    }

    if (p_ctx_sfra->p_act != NULL)
    {
        *(p_ctx_sfra->p_act) = 0.0f;
    }
}

/* Reset the PR scan before each frequency point. */
static void demo_pr_sfra_prepare_freq(void *p_ctx)
{
    demo_pr_sfra_ctx_t *p_ctx_sfra = (demo_pr_sfra_ctx_t *)p_ctx; /* Typed PR context passed from REG_SFRA(). */

    if (p_ctx_sfra == NULL)
    {
        return;
    }

    if (p_ctx_sfra->p_pr != NULL)
    {
        pr_reset(p_ctx_sfra->p_pr);
    }

    if (p_ctx_sfra->p_sfra_collect != NULL)
    {
        *(p_ctx_sfra->p_sfra_collect) = 0.0f;
    }

    if (p_ctx_sfra->p_pr_out != NULL)
    {
        *(p_ctx_sfra->p_pr_out) = 0.0f;
    }
}

/*
 * Reset the SOGI scan before each frequency point.
 *
 * SOGI has internal delay states, so reinitializing it per point keeps each
 * measured point independent.
 */
static void demo_sogi_sfra_prepare_freq(void *p_ctx)
{
    demo_sogi_sfra_ctx_t *p_ctx_sfra = (demo_sogi_sfra_ctx_t *)p_ctx; /* Typed SOGI context passed from REG_SFRA(). */

    if (p_ctx_sfra == NULL)
    {
        return;
    }

    if (p_ctx_sfra->p_sogi != NULL)
    {
        sogi_init(p_ctx_sfra->p_sogi,
                  DEMO_SFRA_SAMPLE_PERIOD_S,
                  DEMO_SFRA_SOGI_W,
                  DEMO_SFRA_SOGI_K,
                  p_ctx_sfra->p_input);
    }

    if (p_ctx_sfra->p_input != NULL)
    {
        *(p_ctx_sfra->p_input) = 0.0f;
    }

    if (p_ctx_sfra->p_sfra_collect != NULL)
    {
        *(p_ctx_sfra->p_sfra_collect) = 0.0f;
    }

    if (p_ctx_sfra->p_output != NULL)
    {
        *(p_ctx_sfra->p_output) = 0.0f;
    }
}

/* -------------------------------------------------------------------------- */
/* Section Objects                                                            */
/* -------------------------------------------------------------------------- */

/*
 * Section objects are declared near the top so the file reads like a map of
 * all demo resources.
 *
 * The REG_* macros do two things:
 * - allocate or bind the object used at runtime
 * - place a pointer/function into a linker section
 *
 * At boot, section_init() walks the linker sections and calls the registered
 * init functions.  During runtime, run_task(), shell_run(), comm_run(), and
 * other services use their own sections to discover registered objects.
 */

/*
 * PERF example step 1:
 * Create named records for code sections that are not automatically registered
 * by REG_TASK/REG_TASK_MS.
 */
REG_PERF_RECORD(demo_manual_perf);       /* Perf record for DEMO_PERF command's manual workload. */
REG_PERF_RECORD(demo_task_period_perf);  /* Perf record for entry-to-entry timing of demo_task_period_20ms(). */
REG_PERF_RECORD(demo_code_section_perf); /* Perf record for a normal code section inside a task. */
REG_PERF_RECORD(demo_flash_math_perf);   /* Perf record for the flash-resident math workload. */
REG_PERF_RECORD(demo_ram_math_perf);     /* Perf record for the RAM-resident math workload. */

/*
 * Scope buffer sizing is split by target.
 *
 * HC32 uses smaller buffers here, while the default configuration keeps larger
 * buffers for richer desktop visualization.  These are demonstration buffers;
 * product code should size scope buffers from RAM budget and required capture
 * depth.
 */
#if defined(IS_HC32)
#define DEMO_SCOPE_BASIC_BUF_SIZE 64      /* Basic scope sample buffer depth for smaller HC32 RAM targets. */
#define DEMO_SCOPE_BASIC_TRIG_POST_CNT 32 /* Basic scope samples retained after trigger on HC32 targets. */
#define DEMO_SCOPE_FAST_BUF_SIZE 64       /* Fast scope sample buffer depth for smaller HC32 RAM targets. */
#define DEMO_SCOPE_FAST_TRIG_POST_CNT 32  /* Fast scope samples retained after trigger on HC32 targets. */
#define DEMO_SCOPE_SLOW_BUF_SIZE 64       /* Slow scope sample buffer depth for smaller HC32 RAM targets. */
#define DEMO_SCOPE_SLOW_TRIG_POST_CNT 32  /* Slow scope samples retained after trigger on HC32 targets. */
#else
#define DEMO_SCOPE_BASIC_BUF_SIZE 100     /* Basic scope sample buffer depth for default demo builds. */
#define DEMO_SCOPE_BASIC_TRIG_POST_CNT 50 /* Basic scope samples retained after trigger in default builds. */
#define DEMO_SCOPE_FAST_BUF_SIZE 500      /* Fast scope sample buffer depth for default demo builds. */
#define DEMO_SCOPE_FAST_TRIG_POST_CNT 250 /* Fast scope samples retained after trigger in default builds. */
#define DEMO_SCOPE_SLOW_BUF_SIZE 200      /* Slow scope sample buffer depth for default demo builds. */
#define DEMO_SCOPE_SLOW_TRIG_POST_CNT 100 /* Slow scope samples retained after trigger in default builds. */
#endif

/*
 * SCOPE example step 1:
 * Register the variables that will be sampled by SCOPE_RUN().
 *
 * Minimal example:
 * - REG_SCOPE_EX creates float variables: scope_basic_ramp/scope_basic_toggle
 * - The service registers this scope with name "demo_scope_basic"
 * - demo_scope_basic_task updates those variables and calls SCOPE_RUN()
 */
REG_SCOPE_EX(demo_scope_basic, DEMO_SCOPE_BASIC_BUF_SIZE, DEMO_SCOPE_BASIC_TRIG_POST_CNT, 10000u,
             scope_basic_ramp,   /* Generated float: slow ramp sampled by the basic scope. */
             scope_basic_toggle) /* Generated float: binary toggle sampled by the basic scope. */

/* 100 us scope example: 500 samples and 10 signal channels. */
REG_SCOPE_EX(demo_scope_fast, DEMO_SCOPE_FAST_BUF_SIZE, DEMO_SCOPE_FAST_TRIG_POST_CNT, 100u,
             scope_sin,      /* Generated float: main sine waveform. */
             scope_cos,      /* Generated float: main cosine waveform. */
             scope_sin2,     /* Generated float: faster secondary sine waveform. */
             scope_cos2,     /* Generated float: faster secondary cosine waveform. */
             scope_ramp,     /* Generated float: normalized ramp waveform. */
             scope_triangle, /* Generated float: normalized triangle waveform. */
             scope_square,   /* Generated float: sign waveform derived from scope_sin. */
             scope_mix,      /* Generated float: weighted mixture of several channels. */
             scope_saw,      /* Generated float: sawtooth waveform. */
             scope_index)    /* Generated float: monotonically wrapping sample index. */

/* 1 ms scope example: 200 samples and 3 signal channels. */
REG_SCOPE_EX(demo_scope_slow, DEMO_SCOPE_SLOW_BUF_SIZE, DEMO_SCOPE_SLOW_TRIG_POST_CNT, 1000u,
             scope_slow_sin, /* Generated float: slow sine waveform. */
             scope_slow_cos, /* Generated float: slow cosine waveform. */
             scope_slow_mix) /* Generated float: slow mixed waveform. */

/*
 * SFRA delay_tick aligns the DFT denominator with the collected response:
 * - 0U: collect[n] is the response to inject[n].
 * - 1U: collect[n] is the response to inject[n - 1], such as PWM/ADC or z1 plant paths.
 *
 * Every REG_SFRA below creates:
 * - an sfra_t object named by the first argument
 * - a float injection port named <name>_inject
 * - a float collection port named <name>_collect
 * - a section entry so sfra_service can list and control it from the host UI
 */
REG_SFRA(demo_sfra,
         0U,                         /* delay_tick: PI-only collect[n] responds to inject[n]. */
         DEMO_SFRA_SAMPLE_PERIOD_S,  /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE, /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,    /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,      /* freq_end_hz: default sweep end. */
         demo_sfra_prepare_freq,     /* freq_prepare: reset PI state before each sweep point. */
         &s_demo_sfra_ctx)           /* p_ctx: plain PI SFRA context. */

REG_SFRA(demo_pi_l_sfra,
         1U,                          /* delay_tick: open-loop PI+L collect is generated one sample behind inject. */
         DEMO_SFRA_SAMPLE_PERIOD_S,   /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE,  /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,     /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,       /* freq_end_hz: default sweep end. */
         demo_pi_l_sfra_prepare_freq, /* freq_prepare: reset PI+L state before each sweep point. */
         &s_demo_pi_l_sfra_ctx)       /* p_ctx: open-loop PI+L SFRA context. */

REG_SFRA(demo_pi_l_closed_sfra,
         1U,                           /* delay_tick: closed-loop current plant has one-tick command/sample delay. */
         DEMO_SFRA_SAMPLE_PERIOD_S,    /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE,   /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,      /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,        /* freq_end_hz: default sweep end. */
         demo_pi_l_sfra_prepare_freq,  /* freq_prepare: reset PI+L state before each sweep point. */
         &s_demo_pi_l_closed_sfra_ctx) /* p_ctx: closed-loop PI+L SFRA context. */

REG_SFRA(demo_pi_c_closed_sfra,
         1U,                            /* delay_tick: closed-loop capacitor plant has one-tick command/sample delay. */
         DEMO_SFRA_PI_C_SAMPLE_PERIOD_S, /* sample_period_s: 30 kHz hardware interrupt period. */
         DEMO_SFRA_INJECT_AMPLITUDE,    /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,       /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,         /* freq_end_hz: default sweep end. */
         demo_pi_c_sfra_prepare_freq,   /* freq_prepare: reset PI+C state before each sweep point. */
         &s_demo_pi_c_closed_sfra_ctx)  /* p_ctx: closed-loop PI+C SFRA context. */

REG_SFRA(demo_pr_sfra,
         0U,                         /* delay_tick: PR-only collect[n] responds to inject[n]. */
         DEMO_SFRA_SAMPLE_PERIOD_S,  /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE, /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,    /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,      /* freq_end_hz: default sweep end. */
         demo_pr_sfra_prepare_freq,  /* freq_prepare: reset PR state before each sweep point. */
         &s_demo_pr_sfra_ctx)        /* p_ctx: PR SFRA context. */

REG_SFRA(demo_sogi_d_sfra,
         0U,                          /* delay_tick: SOGI D output is collected in the same sample slot. */
         DEMO_SFRA_SAMPLE_PERIOD_S,   /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE,  /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,     /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,       /* freq_end_hz: default sweep end. */
         demo_sogi_sfra_prepare_freq, /* freq_prepare: reset SOGI state before each sweep point. */
         &s_demo_sogi_d_sfra_ctx)     /* p_ctx: SOGI D-path SFRA context. */

REG_SFRA(demo_sogi_q_sfra,
         0U,                          /* delay_tick: SOGI Q output is collected in the same sample slot. */
         DEMO_SFRA_SAMPLE_PERIOD_S,   /* sample_period_s: 10 kHz software ISR period. */
         DEMO_SFRA_INJECT_AMPLITUDE,  /* inject_amplitude: default host-visible sine amplitude. */
         DEMO_SFRA_FREQ_START_HZ,     /* freq_start_hz: default sweep start. */
         DEMO_SFRA_FREQ_END_HZ,       /* freq_end_hz: default sweep end. */
         demo_sogi_sfra_prepare_freq, /* freq_prepare: reset SOGI state before each sweep point. */
         &s_demo_sogi_q_sfra_ctx)     /* p_ctx: SOGI Q-path SFRA context. */

/* -------------------------------------------------------------------------- */
/* Local Helpers                                                              */
/* -------------------------------------------------------------------------- */

/*
 * Apply the shell/comm controlled bit mask to board output pins.
 *
 * This helper deliberately keeps the mapping visible:
 * bit0 -> main relay
 * bit1 -> soft-start relay
 * bit2 -> AC input relay
 * bit3 -> AC output relay
 *
 * The demo uses this as a simple observable output for shell and comm tests.
 */
static void demo_apply_led_mask(uint8_t led_mask)
{
    gpio_set_main_rly_sta((uint8_t)((led_mask >> 0) & 0x01u));
    gpio_set_ss_rly_sta((uint8_t)((led_mask >> 1) & 0x01u));
    gpio_set_ac_in_rly_sta((uint8_t)((led_mask >> 2) & 0x01u));
    gpio_set_ac_out_rly_sta((uint8_t)((led_mask >> 3) & 0x01u));
}

/* -------------------------------------------------------------------------- */
/* SFRA Model Helpers                                                         */
/* -------------------------------------------------------------------------- */

/*
 * One-tick delayed inductor update.
 *
 * This mirrors the style used in the standalone fft/main.c reference:
 * current[n+1] = current[n] + voltage_cmd_z1 / L * Ts
 * voltage_cmd_z1 = voltage_cmd[n]
 *
 * The current tick's PI voltage command is stored and affects the inductor on
 * the next tick.  This is the reason closed-loop PI+L SFRA uses delay_tick=1.
 */
static void demo_pi_l_sfra_inductor_step(demo_sfra_inductor_t *p_inductor,
                                         float voltage_cmd)
{
    if (p_inductor == NULL)
    {
        return;
    }

    p_inductor->current_a +=
        (p_inductor->voltage_cmd_z1 / DEMO_SFRA_PI_L_INDUCTANCE_H) *
        DEMO_SFRA_SAMPLE_PERIOD_S;
    p_inductor->voltage_cmd_z1 = voltage_cmd;
}

/*
 * One-tick delayed capacitor update.
 *
 * voltage[n+1] = voltage[n] + current_cmd_z1 / C * Ts
 * current_cmd_z1 = current_cmd[n]
 */
static void demo_pi_c_sfra_capacitor_step(demo_sfra_capacitor_t *p_cap,
                                          float current_cmd)
{
    if (p_cap == NULL)
    {
        return;
    }

    p_cap->voltage_v +=
        (p_cap->current_cmd_z1 / DEMO_SFRA_PI_C_CAPACITANCE_F) *
        DEMO_SFRA_PI_C_SAMPLE_PERIOD_S;
    p_cap->current_cmd_z1 = current_cmd;
}

/* Complex divide helper for analytic transfer-function calculation. */
static demo_sfra_complex_t demo_sfra_complex_div(demo_sfra_complex_t numerator,
                                                 demo_sfra_complex_t denominator)
{
    demo_sfra_complex_t result = {0.0f, 0.0f}; /* Division result initialized to zero for divide-by-zero protection. */
    float den;                                 /* Squared magnitude of the complex denominator. */

    den = (denominator.real * denominator.real) + (denominator.imag * denominator.imag);
    if (den <= 0.0f)
    {
        return result;
    }

    result.real = ((numerator.real * denominator.real) +
                   (numerator.imag * denominator.imag)) /
                  den;
    result.imag = ((numerator.imag * denominator.real) -
                   (numerator.real * denominator.imag)) /
                  den;

    return result;
}

/* Complex multiply helper for PI(s/z) times plant response. */
static demo_sfra_complex_t demo_sfra_complex_mul(demo_sfra_complex_t a,
                                                 demo_sfra_complex_t b)
{
    demo_sfra_complex_t result; /* Product of the two local complex operands. */

    result.real = (a.real * b.real) - (a.imag * b.imag);
    result.imag = (a.real * b.imag) + (a.imag * b.real);

    return result;
}

/*
 * Calculate the discrete PI frequency response at the current SFRA point.
 *
 * pi_tustin_cal() implements:
 *   U(z) / E(z) = (b0 + b1*z^-1) / (1 + a1*z^-1)
 *
 * The open-loop SFRA example uses this helper to generate the steady-state
 * output directly instead of running the PI integrator from zero for each
 * point.
 */
static demo_sfra_complex_t demo_pi_l_sfra_pi_response(const pi_tustin_t *p_pi,
                                                      float phase_step_rad)
{
    demo_sfra_complex_t numerator;   /* PI transfer-function numerator evaluated on the unit circle. */
    demo_sfra_complex_t denominator; /* PI transfer-function denominator evaluated on the unit circle. */
    demo_sfra_complex_t z_inv;       /* z^-1 value for the current frequency point. */

    z_inv.real = cosf(phase_step_rad);
    z_inv.imag = -sinf(phase_step_rad);

    numerator.real = p_pi->inter.b0 + (p_pi->inter.b1 * z_inv.real);
    numerator.imag = p_pi->inter.b1 * z_inv.imag;
    denominator.real = 1.0f + (p_pi->inter.a1 * z_inv.real);
    denominator.imag = p_pi->inter.a1 * z_inv.imag;

    return demo_sfra_complex_div(numerator, denominator);
}

/*
 * Calculate open-loop PI * 1/(sL) frequency response.
 *
 * The inductor is treated as the continuous plant 1/(sL) for this open-loop
 * curve.  The PI part is discrete because the controller under test is the
 * actual pi_tustin implementation.
 */
static demo_sfra_complex_t demo_pi_l_sfra_open_loop_response(const pi_tustin_t *p_pi,
                                                             float freq_hz,
                                                             float phase_step_rad)
{
    demo_sfra_complex_t pi_response;       /* Discrete PI response at the current frequency. */
    demo_sfra_complex_t inductor_response; /* Continuous 1/(sL) response at the current frequency. */

    pi_response = demo_pi_l_sfra_pi_response(p_pi, phase_step_rad);

    inductor_response.real = 0.0f;
    inductor_response.imag =
        -1.0f / (2.0f * 3.14159265358979323846f * freq_hz * DEMO_SFRA_PI_L_INDUCTANCE_H);

    return demo_sfra_complex_mul(pi_response, inductor_response);
}

/* -------------------------------------------------------------------------- */
/* Initialization                                                             */
/* -------------------------------------------------------------------------- */

/*
 * SECTION_INIT / REG_INIT example 1:
 * Register an early boot callback. Smaller priority runs earlier.
 *
 * DBG_TRACE example step 1:
 * Bind the trace time base once before calling DBG_TRACE_MARK().
 * The demo uses sys_tick_100us, so one trace tick is 100 us.
 */
static void demo_init_defaults(void)
{
    /*
     * This init item runs early.  It prepares predictable default values before
     * shell variables, tasks, or communication handlers observe the demo state.
     */
    DBG_TRACE_BIND_TIME(&sys_tick_100us);

    s_demo_counter = 0u;
    s_demo_last_cmd = 0;
    s_demo_gain = 1.0f;
    s_demo_led_mask = 0x01u;
    s_demo_boot_ready = 0u;
    s_demo_perf_spin = 2000u;
    s_demo_last_tick = 0u;
    s_demo_perf_acc = 0u;
    s_demo_scope_basic_tick = 0u;
    s_demo_scope_fast_tick = 0u;
    s_demo_scope_slow_tick = 0u;
    s_demo_last_frame.counter = 0u;
    s_demo_last_frame.led_mask = s_demo_led_mask;
    s_demo_last_frame.temperature_x10 = 250;
    s_demo_last_frame.reserved = 0u;
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_init_count++;
}

/*
 * SECTION_INIT / REG_INIT example 2:
 * A later init item can depend on earlier state and print a boot banner.
 */
static void demo_init_banner(void)
{
    /*
     * This init item runs after defaults.  It demonstrates that later init
     * callbacks can use state initialized by earlier callbacks and can also use
     * the registered link printing function.
     */
    section_link_tx_func_t *p_link_printf = LINK_PRINTF(USART0_LINK); /* USART0 printf route discovered from the link section. */

    s_demo_boot_ready = 1u;
    s_demo_init_count++;

    if ((p_link_printf != NULL) && (p_link_printf->my_printf != NULL))
    {
        p_link_printf->my_printf("demo init done: init_count=%u led_mask=0x%02X\r\n",
                                 (unsigned)s_demo_init_count,
                                 s_demo_led_mask);
    }
}

static void demo_sfra_model_init(void)
{
    /*
     * This init item binds controller/plant objects to SFRA loops.
     *
     * REG_SFRA creates the sfra_t objects and their inject/collect ports.  This
     * function initializes the actual algorithm objects and fills the context
     * objects passed to the per-frequency reset callbacks.
     */
    s_demo_sfra_ctx.p_pi = &s_demo_sfra_pi;
    s_demo_sfra_ctx.p_sfra_collect = &demo_sfra_collect;
    s_demo_sfra_ctx.p_pi_out = &s_demo_sfra_pi_out;

    s_demo_sfra_pi_l_inductor.current_a = 0.0f;
    s_demo_sfra_pi_l_inductor.voltage_cmd_z1 = 0.0f;
    s_demo_pi_l_sfra_ctx.p_pi = &s_demo_sfra_pi_l;
    s_demo_pi_l_sfra_ctx.p_inductor = &s_demo_sfra_pi_l_inductor;
    s_demo_pi_l_sfra_ctx.p_sfra_collect = &demo_pi_l_sfra_collect;
    s_demo_pi_l_sfra_ctx.p_voltage = &s_demo_sfra_pi_l_voltage;
    s_demo_pi_l_sfra_ctx.p_ref = &s_demo_sfra_pi_l_ref;
    s_demo_pi_l_sfra_ctx.p_act = &s_demo_sfra_pi_l_act;

    s_demo_sfra_pi_l_closed_inductor.current_a = 0.0f;
    s_demo_sfra_pi_l_closed_inductor.voltage_cmd_z1 = 0.0f;
    s_demo_pi_l_closed_sfra_ctx.p_pi = &s_demo_sfra_pi_l_closed;
    s_demo_pi_l_closed_sfra_ctx.p_inductor = &s_demo_sfra_pi_l_closed_inductor;
    s_demo_pi_l_closed_sfra_ctx.p_sfra_collect = &demo_pi_l_closed_sfra_collect;
    s_demo_pi_l_closed_sfra_ctx.p_voltage = &s_demo_sfra_pi_l_closed_voltage;
    s_demo_pi_l_closed_sfra_ctx.p_ref = &s_demo_sfra_pi_l_closed_ref;
    s_demo_pi_l_closed_sfra_ctx.p_act = &s_demo_sfra_pi_l_closed_act;

    s_demo_sfra_pi_c_closed_cap.voltage_v = 0.0f;
    s_demo_sfra_pi_c_closed_cap.current_cmd_z1 = 0.0f;
    s_demo_pi_c_closed_sfra_ctx.p_pi = &s_demo_sfra_pi_c_closed;
    s_demo_pi_c_closed_sfra_ctx.p_cap = &s_demo_sfra_pi_c_closed_cap;
    s_demo_pi_c_closed_sfra_ctx.p_sfra_collect = &demo_pi_c_closed_sfra_collect;
    s_demo_pi_c_closed_sfra_ctx.p_current = &s_demo_sfra_pi_c_closed_current;
    s_demo_pi_c_closed_sfra_ctx.p_ref = &s_demo_sfra_pi_c_closed_ref;
    s_demo_pi_c_closed_sfra_ctx.p_act = &s_demo_sfra_pi_c_closed_act;

    s_demo_pr_sfra_ctx.p_pr = &s_demo_sfra_pr;
    s_demo_pr_sfra_ctx.p_sfra_collect = &demo_pr_sfra_collect;
    s_demo_pr_sfra_ctx.p_pr_out = &s_demo_sfra_pr_out;

    s_demo_sogi_d_sfra_ctx.p_sogi = &s_demo_sogi_d;
    s_demo_sogi_d_sfra_ctx.output_sel = DEMO_SOGI_OUTPUT_D;
    s_demo_sogi_d_sfra_ctx.p_input = &s_demo_sfra_sogi_d_in;
    s_demo_sogi_d_sfra_ctx.p_sfra_collect = &demo_sogi_d_sfra_collect;
    s_demo_sogi_d_sfra_ctx.p_output = &s_demo_sfra_sogi_d_out;

    s_demo_sogi_q_sfra_ctx.p_sogi = &s_demo_sogi_q;
    s_demo_sogi_q_sfra_ctx.output_sel = DEMO_SOGI_OUTPUT_Q;
    s_demo_sogi_q_sfra_ctx.p_input = &s_demo_sfra_sogi_q_in;
    s_demo_sogi_q_sfra_ctx.p_sfra_collect = &demo_sogi_q_sfra_collect;
    s_demo_sogi_q_sfra_ctx.p_output = &s_demo_sfra_sogi_q_out;

    (void)pi_tustin_init(&s_demo_sfra_pi,
                         DEMO_SFRA_PI_KP,
                         DEMO_SFRA_PI_KI,
                         DEMO_SFRA_SAMPLE_PERIOD_S,
                         DEMO_SFRA_PI_UP_LMT,
                         DEMO_SFRA_PI_DN_LMT,
                         &s_demo_sfra_pi_ref,
                         &s_demo_sfra_pi_act);

    (void)pi_tustin_init(&s_demo_sfra_pi_l,
                         DEMO_SFRA_PI_L_KP,
                         DEMO_SFRA_PI_L_KI,
                         DEMO_SFRA_SAMPLE_PERIOD_S,
                         DEMO_SFRA_PI_L_UP_LMT,
                         DEMO_SFRA_PI_L_DN_LMT,
                         &s_demo_sfra_pi_l_ref,
                         &s_demo_sfra_pi_l_act);

    (void)pi_tustin_init(&s_demo_sfra_pi_l_closed,
                         DEMO_SFRA_PI_L_KP,
                         DEMO_SFRA_PI_L_KI,
                         DEMO_SFRA_SAMPLE_PERIOD_S,
                         DEMO_SFRA_PI_L_UP_LMT,
                         DEMO_SFRA_PI_L_DN_LMT,
                         &s_demo_sfra_pi_l_closed_ref,
                         &s_demo_sfra_pi_l_closed_act);

    (void)pi_tustin_init(&s_demo_sfra_pi_c_closed,
                         DEMO_SFRA_PI_C_KP,
                         DEMO_SFRA_PI_C_KI,
                         DEMO_SFRA_PI_C_SAMPLE_PERIOD_S,
                         DEMO_SFRA_PI_C_UP_LMT,
                         DEMO_SFRA_PI_C_DN_LMT,
                         &s_demo_sfra_pi_c_closed_ref,
                         &s_demo_sfra_pi_c_closed_act);

    (void)pr_init(&s_demo_sfra_pr,
                  DEMO_SFRA_PR_KP,
                  DEMO_SFRA_PR_KR,
                  DEMO_SFRA_PR_W0,
                  DEMO_SFRA_PR_WC,
                  DEMO_SFRA_SAMPLE_PERIOD_S,
                  DEMO_SFRA_PR_UP_LMT,
                  DEMO_SFRA_PR_DN_LMT,
                  &s_demo_sfra_pr_ref,
                  &s_demo_sfra_pr_act);

    sogi_init(&s_demo_sogi_d,
              DEMO_SFRA_SAMPLE_PERIOD_S,
              DEMO_SFRA_SOGI_W,
              DEMO_SFRA_SOGI_K,
              &s_demo_sfra_sogi_d_in);
    sogi_init(&s_demo_sogi_q,
              DEMO_SFRA_SAMPLE_PERIOD_S,
              DEMO_SFRA_SOGI_W,
              DEMO_SFRA_SOGI_K,
              &s_demo_sfra_sogi_q_in);
}

/* -------------------------------------------------------------------------- */
/* Shell Command Handlers                                                     */
/* -------------------------------------------------------------------------- */

static void demo_led_mask_changed(DEC_MY_PRINTF)
{
    /*
     * Shell variable callback.
     *
     * REG_SHELL_VAR can attach a function that runs after the variable changes.
     * Here the backing variable is s_demo_led_mask, and the callback pushes the
     * new mask to board outputs.
     */

    demo_apply_led_mask(s_demo_led_mask);

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo led mask updated: 0x%02X\r\n", s_demo_led_mask);
    }
}

static void demo_ping_cmd(DEC_MY_PRINTF)
{
    /*
     * Minimal shell command.
     *
     * It prints enough state to confirm that the shell link, printf routing,
     * scheduler counter, and demo defaults are all alive.
     */

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo ping ok\r\n");
        my_printf->my_printf("counter=%lu led_mask=0x%02X gain=%0.2f temp_x10=%d init_count=%u boot_ready=%u tick=%lu\r\n",
                             (unsigned long)s_demo_counter,
                             s_demo_led_mask,
                             (double)s_demo_gain,
                             s_demo_last_frame.temperature_x10,
                             (unsigned)s_demo_init_count,
                             (unsigned)s_demo_boot_ready,
                             (unsigned long)s_demo_last_tick);
    }
}

/*
 * SECTION_SHELL / REG_SHELL_CMD example:
 * Print the section framework map instead of business-only demo content.
 */
static void demo_help_cmd(DEC_MY_PRINTF)
{
    /*
     * Built-in map of this file.
     *
     * This command intentionally prints framework concepts rather than product
     * help text.  It is a quick reminder of which REG_* macro corresponds to
     * which linker section and runtime service.
     */

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("SECTION_E guide:\r\n");
        my_printf->my_printf("INIT       -> REG_INIT(prio, func)\r\n");
        my_printf->my_printf("TASK       -> REG_TASK/REG_TASK_MS\r\n");
        my_printf->my_printf("INTERRUPT  -> REG_INTERRUPT(priority, func)\r\n");
        my_printf->my_printf("SHELL      -> REG_SHELL_VAR / REG_SHELL_CMD\r\n");
        my_printf->my_printf("LINK       -> REG_LINK(rx, handlers, tx)\r\n");
        my_printf->my_printf("PERF       -> REG_PERF_BASE_CNT / REG_PERF_RECORD\r\n");
        my_printf->my_printf("COMM       -> REG_COMM(cmd_set, cmd_word, func)\r\n");
        my_printf->my_printf("COMM_ROUTE -> REG_COMM_ROUTE(src_link, dst_link, dst_addr)\r\n");
        my_printf->my_printf("demo uses  -> INIT TASK SHELL COMM PERF SCOPE TRACE INTERRUPT\r\n");
        my_printf->my_printf("LINK       -> interface/usart.c provides USART0_LINK\r\n");
        my_printf->my_printf("PERF use 1 -> REG_PERF_RECORD(demo_manual_perf) + PERF_START/END\r\n");
        my_printf->my_printf("PERF use 2 -> REG_TASK_MS(10, demo_task) auto record when TASK_RECORD_PERF_ENABLE=1\r\n");
        my_printf->my_printf("PERF use 3 -> demo_task_period_20ms measures entry-to-entry task period\r\n");
        my_printf->my_printf("TRACE use  -> bind sys_tick_100us, then mark in 100/500/1000 ms tasks\r\n");
        my_printf->my_printf("INT use    -> REG_TASK_MS(5, demo_interrupt_trigger_5ms_task) calls section_interrupt\r\n");
        my_printf->my_printf("RAM func   -> compare demo_flash_math_calc and demo_ram_math_calc perf records\r\n");
        my_printf->my_printf("SCOPE basic-> REG_SCOPE_EX(demo_scope_basic, ...2 vars) + REG_TASK_MS(10, demo_scope_basic_task)\r\n");
        my_printf->my_printf("SCOPE fast -> REG_SCOPE_EX(demo_scope_fast, 500, 250, 100, ...10 vars)\r\n");
        my_printf->my_printf("SCOPE slow -> REG_SCOPE_EX(demo_scope_slow, 200, 100, 1000, ...3 vars)\r\n");
        my_printf->my_printf("SCOPE run  -> REG_TASK(1, demo_scope_fast_task) and REG_TASK_MS(1, demo_scope_slow_task)\r\n");
        my_printf->my_printf("shell vars -> DEMO_LED_MASK DEMO_COUNTER DEMO_LAST_CMD DEMO_GAIN DEMO_PERF_SPIN\r\n");
        my_printf->my_printf("shell cmds -> DEMO_PING DEMO_HELP DEMO_SEND DEMO_PERF DEMO_TICK\r\n");
        my_printf->my_printf("examples   -> DEMO_PERF | DEMO_TICK | DEMO_SEND\r\n");
    }
}

/* -------------------------------------------------------------------------- */
/* Perf Shell Handlers                                                        */
/* -------------------------------------------------------------------------- */

/*
 * SECTION_PERF / REG_PERF_RECORD example 1:
 * Manually wrap one code section with PERF_START/PERF_END to measure its cost.
 */
static void demo_perf_cmd(DEC_MY_PRINTF)
{
    /*
     * Manual performance measurement example.
     *
     * PERF_START/PERF_END can wrap any code section, even if that code is not a
     * scheduled task.  The result is stored in the named record registered near
     * the top of this file.
     */
    section_perf_record_t *manual_rec = P_RECORD_PERF(demo_manual_perf);      /* Record storing the manual PERF_START/PERF_END timing. */
    section_perf_record_t *task_rec = P_RECORD_PERF(demo_task);               /* Auto-created task record for the 10 ms demo task. */
    section_perf_record_t *period_rec = P_RECORD_PERF(demo_task_period_perf); /* Record measuring 20 ms task entry-to-entry period. */
    volatile uint32_t acc = 0u;                                               /* Volatile workload accumulator kept visible to the compiler. */
    uint32_t i;                                                               /* Loop index for the manual spin workload. */

    PERF_START(demo_manual_perf);
    for (i = 0u; i < s_demo_perf_spin; ++i)
    {
        acc += (i ^ 0x5A5A5A5Au) + (acc >> 1);
    }
    PERF_END(demo_manual_perf);

    s_demo_perf_acc = acc;

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("perf manual: spin=%lu last=%u max=%lu start=%u end=%u acc=%lu\r\n",
                             (unsigned long)s_demo_perf_spin,
                             (unsigned)((manual_rec != NULL) ? manual_rec->time : 0u),
                             (unsigned long)((manual_rec != NULL) ? manual_rec->max_time : 0u),
                             (unsigned)((manual_rec != NULL) ? manual_rec->start : 0u),
                             (unsigned)((manual_rec != NULL) ? manual_rec->end : 0u),
                             (unsigned long)s_demo_perf_acc);
        my_printf->my_printf("perf task  : last=%u max=%lu name=%s\r\n",
                             (unsigned)((task_rec != NULL) ? task_rec->time : 0u),
                             (unsigned long)((task_rec != NULL) ? task_rec->max_time : 0u),
                             (task_rec != NULL) ? task_rec->p_name : "null");
        my_printf->my_printf("perf period: last=%u max=%lu name=%s count=%lu\r\n",
                             (unsigned)((period_rec != NULL) ? period_rec->time : 0u),
                             (unsigned long)((period_rec != NULL) ? period_rec->max_time : 0u),
                             (period_rec != NULL) ? period_rec->p_name : "null",
                             (unsigned long)s_demo_period_counter);
    }
}

/*
 * SECTION_PERF / REG_PERF_BASE_CNT example 2:
 * Show the raw free-running timer used as the performance counter base.
 */
static void demo_tick_cmd(DEC_MY_PRINTF)
{
    /*
     * Read the raw performance counter base.
     *
     * This is useful when bringing up a board because it verifies the timer
     * backing perf_base_cnt_get() is running.
     */

    s_demo_last_tick = perf_base_cnt_get();

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("perf base tick=%lu\r\n", (unsigned long)s_demo_last_tick);
    }
}

/*
 * SECTION_SHELL / REG_SHELL_CMD example:
 * A shell command can also be the trigger that sends one COMM frame.
 */
static void demo_send_loopback_cmd(DEC_MY_PRINTF)
{
    /*
     * Send one demo protocol frame from a shell command.
     *
     * The frame is routed through the same comm_send_data() path used by normal
     * protocol handlers, so this checks packing, link binding, and host receive
     * behavior without needing an external command first.
     */
    section_packform_t pack = {0};               /* Outbound protocol envelope for the shell-triggered loopback frame. */
    demo_comm_frame_t frame = s_demo_last_frame; /* Payload snapshot copied from the latest loopback frame. */

    frame.counter = s_demo_counter;
    frame.led_mask = s_demo_led_mask;

    pack.src = LOCAL_ADDR;
    pack.dst = PC_ADDR;
    pack.cmd_set = DEMO_CMD_SET_LOOPBACK;
    pack.cmd_word = DEMO_CMD_WORD_LOOPBACK;
    pack.is_ack = 0u;
    pack.len = sizeof(frame);
    pack.p_data = (uint8_t *)&frame;

    if (my_printf && my_printf->my_printf)
    {
        my_printf->my_printf("demo send loopback frame\r\n");
    }

    comm_send_data(&pack, LINK_PRINTF(USART0_LINK));
}

/* -------------------------------------------------------------------------- */
/* Base Demo Tasks                                                            */
/* -------------------------------------------------------------------------- */

/*
 * SECTION_TASK / REG_TASK_MS example:
 * run_task() dispatches this periodic task in the background.
 */
static void demo_task(void)
{
    /*
     * Basic scheduled task.
     *
     * It increments a counter every 10 ms and periodically changes the relay
     * mask.  This gives the task scheduler an easy visible heartbeat.
     */
    static uint8_t led_phase = 0u; /* Four-state rotating output bit used by the scheduler heartbeat. */

    s_demo_counter++;
    s_demo_last_tick = perf_base_cnt_get();

    if ((s_demo_counter % 500u) == 0u)
    {

        led_phase = (uint8_t)((led_phase + 1u) & 0x03u);
        s_demo_led_mask = (uint8_t)(1u << led_phase);
        s_demo_last_frame.counter = s_demo_counter;
        s_demo_last_frame.led_mask = s_demo_led_mask;
        demo_apply_led_mask(s_demo_led_mask);
    }
}

/* -------------------------------------------------------------------------- */
/* Perf Tasks And Workloads                                                   */
/* -------------------------------------------------------------------------- */

/*
 * SECTION_PERF / REG_TASK_MS period example:
 * End and restart the performance record at task entry.
 * This measures the interval between two adjacent entries of this 20 ms task,
 * which is useful for observing scheduler cadence.
 */
static void demo_task_period_20ms(void)
{
    /*
     * Task-period measurement example.
     *
     * The perf record is ended at the beginning of the next call, so it measures
     * entry-to-entry period rather than only the body execution time.
     */
    static uint8_t s_period_started = 0u; /* Tracks whether the period perf record has a previous start point. */
    volatile uint32_t delay = 0u;         /* Small volatile delay loop so the period task has measurable work. */

    if (s_period_started != 0u)
    {
        PERF_END(demo_task_period_perf);
    }

    PERF_START(demo_task_period_perf);
    s_period_started = 1u;

    s_demo_period_counter++;

    for (delay = 0u; delay < 64u; ++delay)
    {
        __NOP();
    }
}

static void demo_busy_delay(uint32_t loop_cnt)
{
    /* Small deterministic workload used by perf and interrupt examples. */
    volatile uint32_t delay; /* Volatile loop index used to create deterministic busy-wait work. */

    for (delay = 0u; delay < loop_cnt; ++delay)
    {
        __NOP();
    }
}

/*
 * FUNC_RAM example:
 * Keep two functions with the same math body:
 * - demo_flash_math_calc() runs from normal flash.
 * - demo_ram_math_calc() is placed in .func_ram by FUNC_RAM.
 *
 * The 10 ms task below wraps each call with a different perf record so the
 * Perf viewer can compare Flash and RAM execution time directly.
 */
static __attribute__((noinline, used)) uint32_t demo_flash_math_calc(uint32_t seed)
{
    /*
     * Flash-resident workload.
     *
     * The noinline/used attributes keep the compiler from optimizing this demo
     * away too aggressively, making flash-vs-RAM measurements easier to see.
     */
    uint32_t x = seed | 1u;          /* First pseudo-random state value for the flash workload. */
    uint32_t y = seed ^ 0x9E3779B9u; /* Second pseudo-random state value for the flash workload. */
    uint32_t acc = 0xA5A5A5A5u;      /* Accumulator mixed each iteration to keep arithmetic nontrivial. */
    uint32_t i;                      /* Loop index for the fixed-size flash workload. */

    for (i = 0u; i < DEMO_MATH_LOOP_COUNT; ++i)
    {
        x = (x * 1664525u) + 1013904223u + i;
        y ^= (x << 5) | (x >> 27);
        acc += ((x ^ y) * 33u) + ((acc >> 3) ^ (y << 2));
        acc = (acc << 7) | (acc >> 25);
        x += (acc / 17u) ^ (y >> 11);
    }

    return acc ^ x ^ y;
}

static FUNC_RAM uint32_t demo_ram_math_calc(uint32_t seed)
{
    /*
     * RAM-resident copy of the same workload.
     *
     * Keeping the algorithm body identical lets the Perf viewer compare memory
     * placement rather than algorithm differences.
     */
    uint32_t x = seed | 1u;          /* First pseudo-random state value for the RAM workload. */
    uint32_t y = seed ^ 0x9E3779B9u; /* Second pseudo-random state value for the RAM workload. */
    uint32_t acc = 0xA5A5A5A5u;      /* Accumulator mixed each iteration to keep arithmetic nontrivial. */
    uint32_t i;                      /* Loop index for the fixed-size RAM workload. */

    for (i = 0u; i < DEMO_MATH_LOOP_COUNT; ++i)
    {
        x = (x * 1664525u) + 1013904223u + i;
        y ^= (x << 5) | (x >> 27);
        acc += ((x ^ y) * 33u) + ((acc >> 3) ^ (y << 2));
        acc = (acc << 7) | (acc >> 25);
        x += (acc / 17u) ^ (y >> 11);
    }

    return acc ^ x ^ y;
}

/*
 * PERF task-load example:
 * These tasks do a small amount of work at different periods. When
 * TASK_RECORD_PERF_ENABLE is enabled, REG_TASK_MS automatically creates one
 * perf record for each task, so the Perf viewer can show task time and load.
 */
static void demo_perf_load_2ms_task(void)
{
    /*
     * Light periodic load.  It creates a small measurable task body so the Perf
     * page can show load distribution across multiple task periods.
     */

    s_demo_aux_counter++;
    demo_busy_delay(16u);
}

static void demo_perf_load_5ms_task(void)
{
    /*
     * Heavier periodic load than demo_perf_load_2ms_task.  The different
     * periods and loop counts make the Perf UI more interesting to inspect.
     */

    s_demo_aux_counter += 3u;
    demo_busy_delay(48u);
}

/*
 * PERF code-section example:
 * Use PERF_START/PERF_END around any normal code block that is not a task or
 * interrupt callback.
 */
static void demo_perf_code_section_10ms_task(void)
{
    /*
     * Demonstrate nested/manual code-section measurements inside a normal task.
     *
     * The task has its own task-level perf record from REG_TASK_MS, while the
     * three PERF_START/PERF_END pairs below expose finer-grained sections.
     */
    const uint32_t seed = s_demo_aux_counter + s_demo_counter; /* Changing seed shared by flash and RAM math comparisons. */

    PERF_START(demo_code_section_perf);
    s_demo_aux_counter += 7u;
    demo_busy_delay(96u);
    PERF_END(demo_code_section_perf);

    PERF_START(demo_flash_math_perf);
    s_demo_flash_math_result = demo_flash_math_calc(seed);
    PERF_END(demo_flash_math_perf);

    PERF_START(demo_ram_math_perf);
    s_demo_ram_math_result = demo_ram_math_calc(seed);
    PERF_END(demo_ram_math_perf);
}

/* -------------------------------------------------------------------------- */
/* Interrupt Demo                                                             */
/* -------------------------------------------------------------------------- */

/*
 * INTERRUPT example step 2:
 * Register callbacks with REG_INTERRUPT() near the bottom of this file.
 */
static void demo_fast_interrupt(void)
{
    /* Higher-priority interrupt callback registered near the bottom. */
    s_demo_interrupt_counter++;
    demo_busy_delay(24u);
}

static void demo_slow_interrupt(void)
{
    /* Lower-priority interrupt callback with a heavier workload. */
    s_demo_interrupt_counter += 2u;
    demo_busy_delay(80u);
}

/* -------------------------------------------------------------------------- */
/* Trace Tasks                                                                */
/* -------------------------------------------------------------------------- */

/*
 * DBG_TRACE example step 2:
 * Put DBG_TRACE_MARK() directly at the point you want to observe.
 * The three tasks below intentionally contain only DBG_TRACE_MARK(), so the
 * line number shown by the Trace viewer maps directly to this demo usage.
 */
static void demo_trace_mark_100ms_task(void)
{
    /*
     * Trace marks are intentionally one-line tasks.  That keeps the source line
     * shown by the Trace viewer directly tied to the call site below.
     */
    DBG_TRACE_MARK();
}

static void demo_trace_mark_500ms_task(void)
{
    /* Same trace marker pattern, different period. */
    DBG_TRACE_MARK();
}

static void demo_trace_mark_1000ms_task(void)
{
    /* Same trace marker pattern, different period. */
    DBG_TRACE_MARK();
}

/* -------------------------------------------------------------------------- */
/* Scope Tasks                                                                */
/* -------------------------------------------------------------------------- */

/*
 * Basic scope example:
 * - Update the registered float variables.
 * - Call SCOPE_RUN(demo_scope_basic) once after the variables are updated.
 * - REG_TASK_MS(10, demo_scope_basic_task) below makes this a 10 ms scope.
 */
static void demo_scope_basic_task(void)
{
    /*
     * Basic scope signals:
     * - a slow ramp
     * - a binary toggle
     *
     * This is the smallest useful Scope example and is good for checking host
     * plotting before moving to many-channel captures.
     */
    scope_basic_ramp = (float)(s_demo_scope_basic_tick % 100u);
    scope_basic_toggle = ((s_demo_scope_basic_tick / 10u) & 0x01u) ? 1.0f : 0.0f;

    SCOPE_RUN(demo_scope_basic);
    s_demo_scope_basic_tick++;
}

/*
 * Fast scope example:
 * - REG_TASK(1, ...) means one scheduler tick, which is 100 us in this project.
 * - Ten float channels are updated every 100 us.
 * - The scope is auto-triggered on a rising zero crossing of scope_sin.
 */
static void demo_scope_fast_task(void)
{
    /*
     * Fast 100 us scope.
     *
     * The signals are synthetic but deliberately varied: sine, cosine, ramp,
     * triangle, square, mixed waveform, sawtooth, and index.  This checks that
     * multi-channel capture, trigger, and plotting all work at the faster task
     * rate.
     */
    float theta;       /* Slow waveform phase used by the main sine/cosine channels. */
    float theta_fast;  /* Faster waveform phase used by secondary sine/cosine channels. */
    uint32_t phase500; /* 0..499 phase index used to build ramp and triangle signals. */

    theta = 2.0f * 3.1415926f * (float)(s_demo_scope_fast_tick % 200u) / 200.0f;
    theta_fast = 2.0f * 3.1415926f * (float)(s_demo_scope_fast_tick % 50u) / 50.0f;
    phase500 = s_demo_scope_fast_tick % 500u;

    scope_sin = sinf(theta);
    scope_cos = cosf(theta);
    scope_sin2 = 0.6f * sinf(theta_fast);
    scope_cos2 = 0.6f * cosf(theta_fast);
    scope_ramp = (float)phase500 / 499.0f;
    scope_triangle = (phase500 < 250u)
                         ? ((float)phase500 / 249.0f)
                         : ((float)(499u - phase500) / 249.0f);
    scope_square = (scope_sin >= 0.0f) ? 1.0f : -1.0f;
    scope_mix = 0.7f * scope_sin + 0.2f * scope_cos2 + 0.1f * scope_triangle;
    scope_saw = ((float)(s_demo_scope_fast_tick % 100u) / 99.0f) * 2.0f - 1.0f;
    scope_index = (float)(s_demo_scope_fast_tick % 1000u);

    SCOPE_RUN(demo_scope_fast);
    s_demo_scope_fast_tick++;
}

/*
 * Slow scope example:
 * - Three float channels are updated every 1 ms.
 * - The scope uses a 200-sample buffer.
 * - Trigger condition is the rising zero crossing of scope_slow_sin.
 */
static void demo_scope_slow_task(void)
{
    /*
     * Slow 1 ms scope.
     *
     * It uses fewer channels and a lower sampling rate to show the same Scope
     * service can host multiple independent capture objects.
     */
    float theta; /* Slow scope waveform phase for 1 ms sine/cosine channels. */

    theta = 2.0f * 3.1415926f * (float)(s_demo_scope_slow_tick % 100u) / 100.0f;

    scope_slow_sin = sinf(theta);
    scope_slow_cos = cosf(theta);
    scope_slow_mix = 0.8f * scope_slow_sin + 0.2f * cosf(theta * 0.25f);

    SCOPE_RUN(demo_scope_slow);
    s_demo_scope_slow_tick++;
}

/* -------------------------------------------------------------------------- */
/* SFRA Tasks                                                                 */
/* -------------------------------------------------------------------------- */

static void demo_sfra_isr_10k_task(void)
{
    /*
     * Plain PI SFRA fast path.
     *
     * This path scans the controller alone:
     *   inject -> PI reference
     *   feedback = 0
     *   collect <- PI output
     *
     * It is a quick sanity test for SFRA because the measured object is a small
     * pure software controller.
     */
    sfra_isr_pre_sample(&demo_sfra);

    s_demo_sfra_pi_ref = demo_sfra_inject;
    s_demo_sfra_pi_act = 0.0f;
    (void)pi_tustin_cal(&s_demo_sfra_pi);

    s_demo_sfra_pi_out = s_demo_sfra_pi.output.val;
    demo_sfra_collect = s_demo_sfra_pi_out;

    sfra_isr_post_sample(&demo_sfra);
}

static void demo_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the plain PI scan. */
    (void)sfra_task(&demo_sfra);
}

static void demo_pi_l_sfra_isr_10k_task(void)
{
    /*
     * Open-loop PI + 1/(sL) SFRA fast path.
     *
     * The requested open-loop scan should not close feedback around the
     * inductor.  Therefore act is forced to zero and collect is generated from
     * the intended open-loop transfer function.
     *
     * A direct zero-state simulation of PI integrator + ideal inductor
     * integrator creates large finite-window transients, especially at low
     * frequency.  For that reason this demo generates the steady-state response
     * for the current point analytically while still using the real PI
     * coefficients from pi_tustin.
     */
    demo_sfra_complex_t pi_response;        /* Discrete PI response at this sweep frequency. */
    demo_sfra_complex_t open_loop_response; /* Combined PI * 1/(sL) response at this sweep frequency. */
    float sample_phase_rad;                 /* Current injection phase adjusted for one-sample delayed collection. */
    float pi_mag;                           /* Magnitude of the discrete PI response. */
    float pi_phase_rad;                     /* Phase of the discrete PI response, in radians. */
    float open_loop_mag;                    /* Magnitude of the combined open-loop response. */
    float open_loop_phase_rad;              /* Phase of the combined open-loop response, in radians. */

    sfra_isr_pre_sample(&demo_pi_l_sfra);

    if ((demo_pi_l_sfra.task.state != SFRA_STATE_SETTLE) &&
        (demo_pi_l_sfra.task.state != SFRA_STATE_COLLECT))
    {
        s_demo_sfra_pi_l_ref = 0.0f;
        s_demo_sfra_pi_l_act = 0.0f;
        s_demo_sfra_pi_l_voltage = 0.0f;
        s_demo_sfra_pi_l_inductor.current_a = 0.0f;
        demo_pi_l_sfra_collect = 0.0f;
        sfra_isr_post_sample(&demo_pi_l_sfra);
        return;
    }

    s_demo_sfra_pi_l_ref = demo_pi_l_sfra_inject;
    s_demo_sfra_pi_l_act = 0.0f;

    /*
     * Open-loop PI + 1/(sL) is generated as a steady-state response.  The
     * SFRA denominator uses inject[n - 1], so collect is generated one sample
     * behind the current injection phase.
     */
    sample_phase_rad = demo_pi_l_sfra.isr.phase_rad - demo_pi_l_sfra.isr.phase_step_rad;
    pi_response = demo_pi_l_sfra_pi_response(&s_demo_sfra_pi_l,
                                             demo_pi_l_sfra.isr.phase_step_rad);
    open_loop_response =
        demo_pi_l_sfra_open_loop_response(&s_demo_sfra_pi_l,
                                          demo_pi_l_sfra.isr.current_freq_hz,
                                          demo_pi_l_sfra.isr.phase_step_rad);

    pi_mag = sqrtf((pi_response.real * pi_response.real) +
                   (pi_response.imag * pi_response.imag));
    pi_phase_rad = atan2f(pi_response.imag, pi_response.real);
    open_loop_mag = sqrtf((open_loop_response.real * open_loop_response.real) +
                          (open_loop_response.imag * open_loop_response.imag));
    open_loop_phase_rad = atan2f(open_loop_response.imag, open_loop_response.real);

    s_demo_sfra_pi_l_voltage =
        demo_pi_l_sfra.cfg.inject_amplitude * pi_mag * sinf(sample_phase_rad + pi_phase_rad);
    s_demo_sfra_pi_l_inductor.current_a =
        demo_pi_l_sfra.cfg.inject_amplitude * open_loop_mag *
        sinf(sample_phase_rad - demo_pi_l_sfra.isr.phase_step_rad + open_loop_phase_rad);
    demo_pi_l_sfra_collect = s_demo_sfra_pi_l_inductor.current_a;

    sfra_isr_post_sample(&demo_pi_l_sfra);
}

static void demo_pi_l_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the open-loop PI+L scan. */
    (void)sfra_task(&demo_pi_l_sfra);
}

static void demo_pi_l_closed_sfra_isr_10k_task(void)
{
    /*
     * Closed-loop PI + inductor SFRA fast path.
     *
     * This follows the practical sampled-control order:
     * 1. SFRA writes the current injection.
     * 2. The model exposes the present current as collect.
     * 3. SFRA captures inject/collect.
     * 4. PI calculates the next voltage command from ref and feedback current.
     * 5. The inductor integrates using the previous voltage command.
     *
     * Because collect[n] corresponds to inject[n - 1] through the z1 plant
     * path, the SFRA object is registered with delay_tick = 1U.
     */
    sfra_isr_pre_sample(&demo_pi_l_closed_sfra);

    if ((demo_pi_l_closed_sfra.task.state != SFRA_STATE_SETTLE) &&
        (demo_pi_l_closed_sfra.task.state != SFRA_STATE_COLLECT))
    {
        s_demo_sfra_pi_l_closed_ref = 0.0f;
        s_demo_sfra_pi_l_closed_act = 0.0f;
        s_demo_sfra_pi_l_closed_voltage = 0.0f;
        s_demo_sfra_pi_l_closed_inductor.current_a = 0.0f;
        s_demo_sfra_pi_l_closed_inductor.voltage_cmd_z1 = 0.0f;
        demo_pi_l_closed_sfra_collect = 0.0f;
        sfra_isr_post_sample(&demo_pi_l_closed_sfra);
        return;
    }

    demo_pi_l_closed_sfra_collect = s_demo_sfra_pi_l_closed_inductor.current_a;
    sfra_isr_post_sample(&demo_pi_l_closed_sfra);

    /*
     * Closed-loop current SFRA follows the real control timing: sample the
     * present current first, then calculate the PI command that updates the
     * inductor model on the next tick through voltage_cmd_z1.
     */
    s_demo_sfra_pi_l_closed_ref = demo_pi_l_closed_sfra_inject;
    s_demo_sfra_pi_l_closed_act = demo_pi_l_closed_sfra_collect;
    (void)pi_tustin_cal(&s_demo_sfra_pi_l_closed);

    s_demo_sfra_pi_l_closed_voltage = s_demo_sfra_pi_l_closed.output.val;
    demo_pi_l_sfra_inductor_step(&s_demo_sfra_pi_l_closed_inductor,
                                 s_demo_sfra_pi_l_closed_voltage);
}

static void demo_pi_l_closed_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the closed-loop PI+L scan. */
    (void)sfra_task(&demo_pi_l_closed_sfra);
}

static void demo_pi_c_closed_sfra_isr_30k_interrupt(void)
{
    /*
     * Closed-loop PI + capacitor SFRA fast path.
     *
     * The controller output is current.  The plant is the delayed capacitor
     * model 1/(sC), so the sampled current command integrates into voltage and
     * that voltage is fed back to the PI on the next interrupt tick.
     */
    sfra_isr_pre_sample(&demo_pi_c_closed_sfra);

    if ((demo_pi_c_closed_sfra.task.state != SFRA_STATE_SETTLE) &&
        (demo_pi_c_closed_sfra.task.state != SFRA_STATE_COLLECT))
    {
        s_demo_sfra_pi_c_closed_ref = 0.0f;
        s_demo_sfra_pi_c_closed_act = 0.0f;
        s_demo_sfra_pi_c_closed_current = 0.0f;
        s_demo_sfra_pi_c_closed_cap.voltage_v = 0.0f;
        s_demo_sfra_pi_c_closed_cap.current_cmd_z1 = 0.0f;
        demo_pi_c_closed_sfra_collect = 0.0f;
        sfra_isr_post_sample(&demo_pi_c_closed_sfra);
        return;
    }

    demo_pi_c_closed_sfra_collect = s_demo_sfra_pi_c_closed_cap.voltage_v;
    sfra_isr_post_sample(&demo_pi_c_closed_sfra);

    s_demo_sfra_pi_c_closed_ref = demo_pi_c_closed_sfra_inject;
    s_demo_sfra_pi_c_closed_act = demo_pi_c_closed_sfra_collect;
    (void)pi_tustin_cal(&s_demo_sfra_pi_c_closed);

    s_demo_sfra_pi_c_closed_current = s_demo_sfra_pi_c_closed.output.val;
    demo_pi_c_sfra_capacitor_step(&s_demo_sfra_pi_c_closed_cap,
                                  s_demo_sfra_pi_c_closed_current);
}

static void demo_pi_c_closed_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the 30 kHz closed-loop PI+C scan. */
    (void)sfra_task(&demo_pi_c_closed_sfra);
}

static void demo_pr_sfra_isr_10k_task(void)
{
    /*
     * PR SFRA fast path.
     *
     * This mirrors the plain PI scan: inject drives the PR reference, feedback
     * is zero, and the PR output is measured.
     */
    sfra_isr_pre_sample(&demo_pr_sfra);

    s_demo_sfra_pr_ref = demo_pr_sfra_inject;
    s_demo_sfra_pr_act = 0.0f;
    (void)pr_cal(&s_demo_sfra_pr);

    s_demo_sfra_pr_out = s_demo_sfra_pr.output.val;
    demo_pr_sfra_collect = s_demo_sfra_pr_out;

    sfra_isr_post_sample(&demo_pr_sfra);
}

static void demo_pr_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the PR scan. */
    (void)sfra_task(&demo_pr_sfra);
}

static void demo_sogi_sfra_isr_10k_task(sfra_t *p_sfra, demo_sogi_sfra_ctx_t *p_ctx)
{
    /*
     * Shared SOGI SFRA fast path.
     *
     * The D and Q scans use identical code; the context selects which SOGI
     * output is routed to collect.  This demonstrates how one implementation
     * can serve multiple registered SFRA loop objects.
     */
    if ((p_sfra == NULL) || (p_ctx == NULL) || (p_ctx->p_sogi == NULL) ||
        (p_ctx->p_input == NULL) || (p_ctx->p_output == NULL) ||
        (p_ctx->p_sfra_collect == NULL))
    {
        return;
    }

    sfra_isr_pre_sample(p_sfra);

    *(p_ctx->p_input) = *(p_sfra->port.p_inject);
    sogi_cal(p_ctx->p_sogi);
    if (p_ctx->output_sel == DEMO_SOGI_OUTPUT_Q)
    {
        *(p_ctx->p_output) = p_ctx->p_sogi->osg_qu[0];
    }
    else
    {
        *(p_ctx->p_output) = p_ctx->p_sogi->osg_u[0];
    }

    *(p_ctx->p_sfra_collect) = *(p_ctx->p_output);
    sfra_isr_post_sample(p_sfra);
}

static void demo_sogi_d_sfra_isr_10k_task(void)
{
    /* D-axis SOGI wrapper used by REG_TASK. */
    demo_sogi_sfra_isr_10k_task(&demo_sogi_d_sfra, &s_demo_sogi_d_sfra_ctx);
}

static void demo_sogi_d_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the SOGI D output scan. */
    (void)sfra_task(&demo_sogi_d_sfra);
}

static void demo_sogi_q_sfra_isr_10k_task(void)
{
    /* Q-axis SOGI wrapper used by REG_TASK. */
    demo_sogi_sfra_isr_10k_task(&demo_sogi_q_sfra, &s_demo_sogi_q_sfra_ctx);
}

static void demo_sogi_q_sfra_task_1ms(void)
{
    /* Background SFRA state machine for the SOGI Q output scan. */
    (void)sfra_task(&demo_sogi_q_sfra);
}

/* -------------------------------------------------------------------------- */
/* Communication Handlers                                                     */
/* -------------------------------------------------------------------------- */

/*
 * SECTION_COMM / REG_COMM example 1:
 * Handle one decoded loopback frame and send an ACK.
 */
static void demo_loopback_comm(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    /*
     * Protocol loopback handler.
     *
     * A valid payload is copied into s_demo_last_frame and then returned to the
     * sender with is_ack = 1.  This verifies receive parsing, ACK routing, and
     * payload serialization.
     */
    section_packform_t ack = {0}; /* ACK envelope that mirrors source/destination and command fields from the request. */

    if ((p_pack == NULL) || (p_pack->len != sizeof(demo_comm_frame_t)))
    {
        if (my_printf && my_printf->my_printf)
        {
            my_printf->my_printf("demo loopback invalid len=%u\r\n",
                                 (unsigned)((p_pack != NULL) ? p_pack->len : 0u));
        }
        return;
    }

    memcpy(&s_demo_last_frame, p_pack->p_data, sizeof(s_demo_last_frame));
    s_demo_led_mask = s_demo_last_frame.led_mask;
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_last_cmd = (int32_t)((DEMO_CMD_SET_LOOPBACK << 8) | DEMO_CMD_WORD_LOOPBACK);

    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = sizeof(s_demo_last_frame);
    ack.p_data = (uint8_t *)&s_demo_last_frame;
    comm_send_data(&ack, my_printf);
}

/*
 * SECTION_COMM / REG_COMM example 2:
 * Accept a short control payload, update local state, and return an ACK.
 */
static void demo_control_comm(section_packform_t *p_pack, DEC_MY_PRINTF)
{
    /*
     * Small command handler that mutates demo state.
     *
     * Byte 0 controls the relay/LED mask.
     * Byte 1 controls a simple temperature field in units of 10 degrees.
     *
     * The ACK returns the values that were accepted, which is the pattern most
     * small configuration commands should follow.
     */
    uint8_t ack_data[2];          /* Compact ACK payload: accepted LED mask and temperature byte. */
    section_packform_t ack = {0}; /* ACK envelope that mirrors source/destination and command fields from the request. */

    if ((p_pack == NULL) || (p_pack->len < 2u))
    {

        return;
    }

    s_demo_led_mask = p_pack->p_data[0];
    s_demo_last_frame.temperature_x10 = (int16_t)((int16_t)p_pack->p_data[1] * 10);
    demo_apply_led_mask(s_demo_led_mask);
    s_demo_last_cmd = (int32_t)((DEMO_CMD_SET_CONTROL << 8) | DEMO_CMD_WORD_CONTROL);

    ack_data[0] = s_demo_led_mask;
    ack_data[1] = (uint8_t)(s_demo_last_frame.temperature_x10 / 10);

    ack.src = p_pack->dst;
    ack.d_src = p_pack->d_dst;
    ack.dst = p_pack->src;
    ack.d_dst = p_pack->d_src;
    ack.cmd_set = p_pack->cmd_set;
    ack.cmd_word = p_pack->cmd_word;
    ack.is_ack = 1u;
    ack.len = sizeof(ack_data);
    ack.p_data = ack_data;
    comm_send_data(&ack, my_printf);
}

/* -------------------------------------------------------------------------- */
/* Section Registration Table                                                 */
/* -------------------------------------------------------------------------- */

/*
 * The registration table is intentionally kept at the end.
 *
 * This makes the file read in two passes:
 * 1. Define objects, state, and functions.
 * 2. Register those functions into framework sections.
 *
 * The order of declarations here is also useful documentation.  It shows which
 * functions are lifecycle callbacks, shell commands, protocol handlers,
 * periodic tasks, and interrupt callbacks.
 */

/*
 * REG_INIT(prio, func)
 *   prio : smaller executes earlier during section_init()
 *   func : init function, signature is void func(void)
 *
 * Example below:
 *   REG_INIT(-10, demo_init_defaults) -> early default state
 *   REG_INIT(10, demo_init_banner)    -> later banner print
 */
REG_INIT(-10, demo_init_defaults)
REG_INIT(10, demo_init_banner)
REG_INIT(5, demo_sfra_model_init)

/*
 * REG_SHELL_VAR(_name, _var, _type, _max, _min, _func, _status)
 *   _name   : shell command name string generated from token
 *   _var    : backing variable address
 *   _type   : shell data type, such as SHELL_UINT8/SHELL_UINT32/SHELL_FP32
 *   _max    : upper limit for writes from shell/comm
 *   _min    : lower limit for writes from shell/comm
 *   _func   : optional callback after value changes
 *   _status : flag bits, commonly SHELL_STA_NULL
 *
 * Example below:
 *   REG_SHELL_VAR(DEMO_GAIN, s_demo_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
 */
REG_SHELL_VAR(DEMO_LED_MASK, s_demo_led_mask, SHELL_UINT8, 0x0F, 0, demo_led_mask_changed, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_COUNTER, s_demo_counter, SHELL_UINT32, 0xFFFFFFFFu, 0u, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_LAST_CMD, s_demo_last_cmd, SHELL_INT32, 0x7FFFFFFF, 0, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_GAIN, s_demo_gain, SHELL_FP32, 10.0f, 0.1f, NULL, SHELL_STA_NULL)
REG_SHELL_VAR(DEMO_PERF_SPIN, s_demo_perf_spin, SHELL_UINT32, 200000u, 1u, NULL, SHELL_STA_NULL)

/*
 * REG_SHELL_CMD(_name, _func)
 *   _name : shell command name string generated from token
 *   _func : command handler, signature is void func(DEC_MY_PRINTF)
 *
 * Example below:
 *   REG_SHELL_CMD(DEMO_PERF, demo_perf_cmd)
 */
REG_SHELL_CMD(DEMO_PING, demo_ping_cmd)
REG_SHELL_CMD(DEMO_HELP, demo_help_cmd)
REG_SHELL_CMD(DEMO_PERF, demo_perf_cmd)
REG_SHELL_CMD(DEMO_SEND, demo_send_loopback_cmd)
REG_SHELL_CMD(DEMO_TICK, demo_tick_cmd)

/*
 * REG_COMM(_cmd_set, _cmd_word, _func)
 *   _cmd_set  : protocol command set
 *   _cmd_word : protocol command word inside that set
 *   _func     : handler, signature is
 *               void func(section_packform_t *p_pack, DEC_MY_PRINTF)
 *
 * Example below:
 *   REG_COMM(0x30, 0x01, demo_loopback_comm)
 */
REG_COMM(DEMO_CMD_SET_LOOPBACK, DEMO_CMD_WORD_LOOPBACK, demo_loopback_comm)
REG_COMM(DEMO_CMD_SET_CONTROL, DEMO_CMD_WORD_CONTROL, demo_control_comm)

/*
 * REG_TASK_MS(period_ms, func)
 *   period_ms : execution period in milliseconds
 *   func      : task body, signature is void func(void)
 *
 * When TASK_RECORD_PERF_ENABLE == 1, this macro also auto-creates
 * section_perf_record_<func>, so the task execution time can be queried by
 * P_RECORD_PERF(func).
 */
REG_TASK_MS(10, demo_task)
REG_TASK_MS(20, demo_task_period_20ms)
REG_TASK_MS(2, demo_perf_load_2ms_task)
REG_TASK_MS(5, demo_perf_load_5ms_task)
REG_TASK_MS(10, demo_perf_code_section_10ms_task)

/*
 * DBG_TRACE example step 3:
 * Register periodic tasks that call DBG_TRACE_MARK() directly.
 * The Trace viewer should show these three source lines at 100/500/1000 ms.
 */
REG_TASK_MS(100, demo_trace_mark_100ms_task)
REG_TASK_MS(500, demo_trace_mark_500ms_task)
REG_TASK_MS(1000, demo_trace_mark_1000ms_task)

/*
 * SCOPE example step 2:
 * Update the sampled variables first, then call SCOPE_RUN(scope_name).
 */
REG_TASK_MS(10, demo_scope_basic_task)
REG_TASK(1, demo_scope_fast_task)
REG_TASK_MS(1, demo_scope_slow_task)
REG_TASK(1, demo_sfra_isr_10k_task)
REG_TASK_MS(1, demo_sfra_task_1ms)
REG_TASK(1, demo_pi_l_sfra_isr_10k_task)
REG_TASK_MS(1, demo_pi_l_sfra_task_1ms)
REG_TASK(1, demo_pi_l_closed_sfra_isr_10k_task)
REG_TASK_MS(1, demo_pi_l_closed_sfra_task_1ms)
REG_TASK_MS(1, demo_pi_c_closed_sfra_task_1ms)
REG_TASK(1, demo_pr_sfra_isr_10k_task)
REG_TASK_MS(1, demo_pr_sfra_task_1ms)
REG_TASK(1, demo_sogi_d_sfra_isr_10k_task)
REG_TASK_MS(1, demo_sogi_d_sfra_task_1ms)
REG_TASK(1, demo_sogi_q_sfra_isr_10k_task)
REG_TASK_MS(1, demo_sogi_q_sfra_task_1ms)

/*
 * INTERRUPT example step 3:
 * Smaller priority number runs earlier when section_interrupt() is called.
 */
REG_INTERRUPT(5, demo_pi_c_closed_sfra_isr_30k_interrupt)
REG_INTERRUPT(6, demo_fast_interrupt)
REG_INTERRUPT(7, demo_slow_interrupt)
