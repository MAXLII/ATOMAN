// SPDX-License-Identifier: MIT
/**
 * @file    svpwm_3level.h
 * @brief   Three-phase 3-level SVPWM interface contract.
 * @details
 *          This file is part of the digital power framework project.
 *
 *          Module responsibilities:
 *          - Accept stationary-frame voltage references and split DC-link voltages
 *          - Describe phase-level dwell ratios and output validity
 *          - Separate modulation from topology-specific PWM gate generation
 *
 *          Design notes:
 *          - Center-aligned modulation based on TI SPRABS6, with split-bus extension
 *          - C11 compatible; caller-owned storage; no dynamic allocation
 *          - Caller supplies a coherent input snapshot for each PWM period
 *          - Gate mapping, dead time and shutdown belong to HAL / BSP
 *
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 *
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 *
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#ifndef SVPWM_3LEVEL_H
#define SVPWM_3LEVEL_H

#include <stdbool.h>

typedef enum
{
    SVPWM_3LEVEL_NOT_READY = 0,    /* No usable modulation result, including after reset. */
    SVPWM_3LEVEL_OK,               /* Current phase dwell ratios may be consumed. */
    SVPWM_3LEVEL_INVALID_ARGUMENT, /* A required object pointer is NULL. */
    SVPWM_3LEVEL_INVALID_CONFIG,   /* The configured DC-link threshold is not finite or positive. */
    SVPWM_3LEVEL_INVALID_INPUT,    /* Non-finite input, low half bus or unrepresentable bus ratio. */
    SVPWM_3LEVEL_OUT_OF_RANGE      /* Requested vector cannot be synthesized in the linear region. */
} SVPWM_3LEVEL_STATUS_E;

/**
 * Voltage convention: amplitude-invariant Clarke transform, alpha along phase A,
 * positive beta toward phase B. For a zero-sequence-free reference:
 * va = v_alpha; vb = -v_alpha / 2 + sqrt(3) * v_beta / 2;
 * vc = -v_alpha / 2 - sqrt(3) * v_beta / 2.
 * All values use volts; v_dc_p and v_dc_n are positive magnitudes.
 * This interface targets fully controlled NPC / T-type bridges with P/O/N states.
 * It does not describe the current-dependent switching constraints of Vienna PFC.
 */
typedef struct svpwm_3level_input
{
    float v_alpha; /* Requested alpha-axis voltage in V. */
    float v_beta;  /* Requested beta-axis voltage in V. */
    float v_dc_p;  /* Positive rail minus DC midpoint voltage in V. */
    float v_dc_n;  /* DC midpoint minus negative rail voltage in V. */
} svpwm_3level_input_t;

typedef struct svpwm_3level_cfg
{
    float v_dc_half_min; /* Minimum permitted voltage of EACH half bus in V; finite and > 0. */
} svpwm_3level_cfg_t;

/**
 * Fractions of one COMPLETE PWM period, not gate duties or timer compare values.
 * With valid output, every value lies in [0, 1] and their sum is 1 within
 * floating-point tolerance. The mean pole voltage relative to the DC midpoint is
 * duty_p * v_dc_p - duty_n * v_dc_n.
 * At most 1 of duty_p and duty_n is nonzero in each phase. For normalized time
 * t in [0, 1), P occupies [(1-duty_p)/2, (1+duty_p)/2); N occupies
 * [0, duty_n/2) and [1-duty_n/2, 1); O occupies the remaining time.
 * All phases share the same period origin. This fixes a symmetric sequence with
 * at most 7 segments and no direct P/N transition within a period. Sector changes
 * at period boundaries still require topology-specific commutation handling.
 * PWM adapters must apply gate mapping, dead time and hardware shutdown separately.
 */
typedef struct svpwm_3level_phase_output
{
    float duty_p; /* Fraction at positive rail P: pole voltage +v_dc_p. */
    float duty_o; /* Fraction at midpoint O: pole voltage 0 V. */
    float duty_n; /* Fraction at negative rail N: pole voltage -v_dc_n. */
} svpwm_3level_phase_output_t;

typedef struct svpwm_3level_output
{
    svpwm_3level_phase_output_t phase_a; /* Phase A dwell ratios for the latest calculation. */
    svpwm_3level_phase_output_t phase_b; /* Phase B dwell ratios for the latest calculation. */
    svpwm_3level_phase_output_t phase_c; /* Phase C dwell ratios for the latest calculation. */
    SVPWM_3LEVEL_STATUS_E status;        /* Dwell ratios are usable only when equal to SVPWM_3LEVEL_OK. */
} svpwm_3level_output_t;

typedef struct svpwm_3level
{
    svpwm_3level_input_t input;   /* Caller-written coherent snapshot, unchanged during cal(). */
    svpwm_3level_cfg_t cfg;       /* Configuration copied by init(); reinitialize to change it. */
    svpwm_3level_output_t output; /* Library-written result; consume only after cal() completes. */
} svpwm_3level_t;

/**
 * @brief Initialize an instance and copy its configuration.
 * @param p_svpwm Caller-owned instance; p_cfg may point to its cfg member.
 * @param p_cfg Configuration source; copied, not retained.
 * @return true on successful initialization; false on NULL or invalid configuration.
 *
 * Clear input and all dwell ratios; set output.status to NOT_READY on success.
 * On failure with a non-NULL instance,
 * clear the instance and set INVALID_ARGUMENT or INVALID_CONFIG as appropriate.
 * Initialization does not produce a valid PWM command.
 */
bool svpwm_3level_init(svpwm_3level_t *p_svpwm, const svpwm_3level_cfg_t *p_cfg);

/**
 * @brief Calculate 1 complete PWM period from the current input snapshot.
 * @param p_svpwm Initialized instance; exclusive access is required for this call.
 * @return Current calculation status; INVALID_ARGUMENT for a NULL instance.
 *
 * Validate configuration and all inputs; reject non-finite values, either half
 * bus below v_dc_half_min and a half-bus ratio that underflows float. Synthesize
 * the reference inside the attainable hexagon (phase maximum minus minimum <=
 * total bus voltage), without reference clipping or overmodulation. Feasibility
 * allows 8 * FLT_EPSILON of boundary roundoff in max-half-bus units, with final
 * duty clipping only to remove that roundoff. A non-OK result clears all
 * dwell ratios and replaces the previous status, so stale output is not reused.
 *
 * This implementation provides no neutral-point balance regulator, minimum
 * pulse handling or dead-time compensation. Split-bus measurements describe
 * available voltage levels; they do not imply closed-loop midpoint balancing.
 * A single instance is not reentrant. The caller owns PWM timing and must arrange
 * coherent input capture and output transfer across ISR/task boundaries.
 */
SVPWM_3LEVEL_STATUS_E svpwm_3level_cal(svpwm_3level_t *p_svpwm);

/**
 * @brief Invalidate the output while retaining configuration and input values.
 * @param p_svpwm Instance to reset; NULL is a no-op.
 *
 * Clear all dwell ratios and set
 * NOT_READY. Cleared duties are an invalid sentinel, not a realizable waveform.
 * The caller must inhibit PWM when status is not OK. An O state is an active
 * midpoint connection and must never be treated as a hardware shutdown command.
 */
void svpwm_3level_reset(svpwm_3level_t *p_svpwm);

#endif /* SVPWM_3LEVEL_H */
