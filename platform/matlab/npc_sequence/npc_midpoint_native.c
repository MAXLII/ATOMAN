// SPDX-License-Identifier: MIT
/**
 * @file npc_midpoint_native.c
 * @brief Native switched-model runner for npc_midpoint_switching.m.
 * @details Calls production NPC control and SVPWM directly. Only the dispatcher
 *          boundaries are trapped; no hardware/transport or live PLECS calls.
 *          Double precision is confined to this offline electrical test model.
 * @author Max.Li
 * @date 2026-09-13
 * @version 1.0.0
 *          Copyright (c) 2026 Max.Li. All rights reserved.
 *          Licensed under the MIT License. See LICENSE in the project root.
 */
#include "npc_ctrl.c"
#include "svpwm_3level.h"
#include "pwm.h"
#include "pr.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "plant.h"
#include "npc_modulation_candidate.h"
#include "npc_predictor_candidate.h"
#include "npc_learning_candidate.h"
#include "npc_average_balance_candidate.h"
/* Unused scheduler boundaries. The harness calls the real calculation directly. */
static void production_gate_duties(const svpwm_3level_t *p_mod, double duty[3][2])
{
    const svpwm_3level_phase_output_t *phases[3] = {&p_mod->output.phase_a,
                                                    &p_mod->output.phase_b,
                                                    &p_mod->output.phase_c};
    float currents[3]       = {p_mod->input.i_a, p_mod->input.i_b, p_mod->input.i_c};
    double reconstructed[3] = {0};

    for (int phase = 0; phase < 3; ++phase)
    {
        float positive = 0.0f, negative = 0.0f;
        pwm_correct_phase_duty(phases[phase], currents[phase], p_mod->inter.current_filtered, &positive, &negative);
        duty[phase][0] = positive;
        duty[phase][1] = negative;
        assert(    positive >= 0.0f
                && positive <= 1.0f
                && negative >= 0.0f
                && negative <= 1.0f);
        assert(phases[phase]->duty_p * phases[phase]->duty_n == 0.0f);
        assert(fabs(phases[phase]->duty_p + phases[phase]->duty_o + phases[phase]->duty_n - 1.0) < 2e-6);
        reconstructed[phase] =
            (double)phases[phase]->duty_p * p_mod->input.v_dc_p - (double)phases[phase]->duty_n * p_mod->input.v_dc_n;
    }
    assert(fabs((2.0 * reconstructed[0] - reconstructed[1] - reconstructed[2]) / 3.0 - p_mod->input.v_alpha) < .002);
    assert(fabs((reconstructed[1] - reconstructed[2]) * M_1_SQRT3 - p_mod->input.v_beta) < .002);
}
const npc_ctrl_hal_t *npc_hal_get_ctrl(void)
{
    abort();
}
NPC_RUN_STA_E npc_fsm_get_run_sta(void)
{
    abort();
}
uint8_t npc_fsm_read_published(npc_ctrl_setpoint_t *p_out)
{
    (void)p_out;
    abort();
}

int main(int argc, char **p_argv)
{
    int mode = argc > 1 ? atoi(p_argv[1]) : 1;
    npc_ctrl_t control = {0};
    npc_ctrl_cfg_t cfg = npc_cfg_default();

    if (mode < 16) /* Historical cases do not include the production voltage-damping path. */
    {
        cfg.voltage_damping_gain = 0.0f;
    }

    if (mode == 16) /* Exercise the real implementation with candidate settings; release default remains disabled. */
    {
        cfg.voltage_damping_gain = 0.15f;
    }

    if (mode < 6) /* Preserve pre-bias-fix baselines and avoid applying the experimental compensation twice. */
    {
        cfg.current_bias_resistance = 0.0f;
    }
    npc_ctrl_sample_t sample = {0};
    svpwm_3level_t mod       = {0};
    svpwm_3level_cfg_t mod_cfg = {.v_dc_half_min = 20.0f, .midpoint_kp = mode == 0 ? 0.0f : 1.8849556f};

    if (    (mode == 80)
         || (mode == 82))
    {
        mod_cfg = pwm_make_modulator_cfg(20.0f, 0.06f * M_2PI * 2.0f);
    }
    double x[10]      = {0};
    double duty[3][2] = {
        {0}
    };
    double age[3][4] = {
        {0}
    };
    double delta = argc > 2 ? atof(p_argv[2]) : 0;
    double average[3]                   = {0};
    double peak_delta                   = 0;
    double residual_filtered[3]         = {0}; /* Offline low-frequency current-bias observer. */
    double previous_offset              = 0.0; /* Offline common-mode slew experiment, V. */
    double previous_voltage_residual[2] = {0}; /* Offline phase-shaped voltage damping history. */
    pr_t harmonics[6][2]                = {0}; /* Offline 2nd through 7th voltage-harmonic rejection. */
    float harmonic_residual[2]          = {0};
    float harmonic_reference            = 0.0f;
    int scenario = argc > 3 ? atoi(p_argv[3]) : 0;
    double damping = argc > 4 ? atof(p_argv[4]) : 0.1;     /* Offline residual-current damping candidate, ohm. */
    double dead_time = argc > 5 ? atof(p_argv[5]) : 10e-6; /* Diagnostic gate delay, seconds. */

    if (    (mode > 16)
         && (mode != 80)
         && (mode != 82) /* Production modes retain the actual C configuration. */
         && (argc > 4))  /* Otherwise retain the real production default. */
    {
        cfg.voltage_damping_gain = (float)damping;
    }
    int delay_periods = argc > 6 ? atoi(p_argv[6]) : 1; /* Compare immediate application with one-period buffering. */

    if (    (mode > 16)
         && (mode != 80)
         && (mode != 82) /* Filter-frequency sweeps are offline only. */
         && (argc > 7))  /* Production case 16 always retains its configured cutoff. */
    {
        cfg.voltage_damping_cutoff_hz = (float)atof(p_argv[7]);
    }

    if (mode == 7) /* Isolate neutral-point modulation from dead-time and LC dynamics. */
    {
        mod_cfg.midpoint_kp = 0.0f;
    }
    char filename[80] = {0};
    snprintf(filename, sizeof(filename), "mode_%d_delta_%g_scenario_%d.csv", mode, delta, scenario);
    FILE *p_file = fopen(filename, "w");

    if (p_file == NULL)
    {
        return 4;
    }
    fprintf(
        p_file,
        "time,delta,ref,estimate,actual,voltage_peak,va,vb,vc,ia,ib,ic,vd_pos,vq_pos,vd_neg,vq_neg,current_limited,voltage_limited\n");
    snprintf(filename, sizeof(filename), "mode_%d_delta_%g_scenario_%d_fast.csv", mode, delta, scenario);
    FILE *p_fast = fopen(filename, "w"); /* 50 kHz electrical-state record avoids 5 kHz sampling aliases. */

    if (p_fast == NULL)
    {
        fclose(p_file);
        return 4;
    }
    fprintf(p_fast, "time,va,vb,vc,ia,ib,ic\n");
    npc_ctrl_init_states(&control, &cfg);
    svpwm_3level_init(&mod, &mod_cfg);

    for (int harmonic = 0; harmonic < 6; ++harmonic)
    {
        float warped = 2.0f / cfg.ts * tanf((float)(harmonic + 2) * cfg.omega * cfg.ts * 0.5f);

        for (int axis = 0; axis < 2; ++axis)
        {
            (void)pr_init(&harmonics[harmonic][axis],
                          0.0f,
                          5.0f,
                          warped,
                          M_2PI,
                          cfg.ts,
                          40.0f,
                          -40.0f,
                          &harmonic_reference,
                          &harmonic_residual[axis]);
        }
    }

    for (int k = 0; k < 60000; ++k)
    {
        int stage = (    k >= 20000
                      && k < 40000)
                      ? scenario
                      : 0;

        if (scenario == 3) /* Constant extreme-unbalance case from the PLECS report. */
        {
            stage = 3;
        }

        if (scenario == 4) /* Constant balanced 3 MW load, including startup. */
        {
            stage = 2;
        }

        if (scenario == 5) /* Actual PLECS light-load resistor settings: 1 W per branch. */
        {
            stage = 4;
        }
        double t = k * .0002, vp = (1331 + delta) / 2, vn = (1331 - delta) / 2;

        for (int p = 0; p < 3; ++p)
        {
            sample.i_l[p] = (float)(inverse_clarke[p][0] * x[0] + inverse_clarke[p][1] * x[1]);
            double terminal_voltage = 0.0;

            for (int state = 0; state < 10; ++state)
            {
                terminal_voltage += (inverse_clarke[p][0] * voltage_output[0][0][state]
                                     + inverse_clarke[p][1] * voltage_output[0][1][state])
                                  * x[state];
            }
            sample.v_out[p] = (float)terminal_voltage;
        }
        sample.theta = (float)fmod(t * 2 * acos(-1) * 50, 2 * acos(-1));
        sample.vd_pos_ref = (float)fmin(t * 281.5, 563);
        sample.v_dc_p     = (float)vp;
        sample.v_dc_n     = (float)vn;

        if (npc_ctrl_cal(&control, &sample) == false)
        {
            return 2;
        }

        if (    (mode >= 50)
             && (mode < 60))
        {
            candidate_control(&control, &sample, delay_periods);
        }

        if (    (mode >= 60)
             && (mode < 70))
        {
            candidate_learning(&control, &sample, delay_periods, k);
        }

        if (    (mode >= 76)
             && (mode < 80))
        {
            candidate_resonance_damping(&control, mode);
        }
        mod.input = (svpwm_3level_input_t){.v_alpha = control.output.v_alpha,
                                           .v_beta  = control.output.v_beta,
                                           .v_dc_p  = (float)vp,
                                           .v_dc_n  = (float)vn};
        double currents[3] = {0};

        for (int p = 0; p < 3; ++p)
        {
            currents[p] = sample.i_l[p];

            if (    (mode == 2)  /* Released fundamental-current balancing path. */
                 || (mode >= 4)) /* Offline damping / common-mode slew experiments. */
            {
                currents[p] = control.output.i_fundamental[p];
            }

            if (mode == 3)
            {
                currents[p] = average[p];
            }
        }
        mod.input.i_a = (float)currents[0];
        mod.input.i_b = (float)currents[1];
        mod.input.i_c = (float)currents[2];

        if (    (mode >= 26) /* Candidate selective harmonic rejection outside the unchanged dq PI loops. */
             && (mode < 30)) /* Later cases isolate dead-time compensation without resonant controllers. */
        {
            float correction[2] = {0};
            harmonic_residual[0] =
                control.inter.v_ab[0] - control.inter.voltage.output.alpha_pos - control.inter.voltage.output.alpha_neg;
            harmonic_residual[1] =
                control.inter.v_ab[1] - control.inter.voltage.output.beta_pos - control.inter.voltage.output.beta_neg;

            for (int harmonic = 0; harmonic < 6; ++harmonic)
            {
                for (int axis = 0; axis < 2; ++axis)
                {
                    (void)pr_cal(&harmonics[harmonic][axis]);
                    correction[axis] += harmonics[harmonic][axis].output.val;
                }
            }
            mod.input.v_alpha += correction[0];
            mod.input.v_beta += correction[1];
        }

        if (    (mode >= 8)  /* Start of the offline voltage-damping candidates. */
             && (mode < 16)) /* Production mode already computes compensation before the voltage limiter. */
        {
            double residual[2] = {
                control.inter.v_ab[0] - control.inter.voltage.output.alpha_pos - control.inter.voltage.output.alpha_neg,
                control.inter.v_ab[1] - control.inter.voltage.output.beta_pos - control.inter.voltage.output.beta_neg};
            mod.input.v_alpha += (float)(damping * (residual[0] - previous_voltage_residual[0]));
            mod.input.v_beta += (float)(damping * (residual[1] - previous_voltage_residual[1]));
            previous_voltage_residual[0] = residual[0];
            previous_voltage_residual[1] = residual[1];
        }

        if (    (mode == 4)  /* Historical offline bias candidate. */
             || (mode == 5)) /* Historical bias + common-mode slew candidate. */
        {
            double residual[3] = {0};

            for (int phase = 0; phase < 3; ++phase)
            {
                residual_filtered[phase] +=
                    0.003762841 * (sample.i_l[phase] - control.output.i_fundamental[phase] - residual_filtered[phase]);
                residual[phase] = residual_filtered[phase];
            }
            mod.input.v_alpha -= (float)(damping * (2.0 * residual[0] - residual[1] - residual[2]) / 3.0);
            mod.input.v_beta -= (float)(damping * (residual[1] - residual[2]) / sqrt(3.0));
        }
        svpwm_3level_cal(&mod);

        if (    (    (mode >= 40)
                  && (mode < 50))
             || (mode == 51))
        {
            candidate_modulate(&mod);
        }

        if (    (    (mode >= 70)
                  && (mode < 80))
             || (mode == 81))
        {
            candidate_average_balance(&mod, mode);
        }

        if (    (mode == 5)   /* Historical common-mode slew candidate. */
             || (mode == 34)) /* Repeat with the production damping filter and a smaller slew. */
        {
            double phase_v[3] = {0};
            double minimum    = 1e9;
            double maximum    = -1e9;

            for (int phase = 0; phase < 3; ++phase)
            {
                phase_v[phase] =
                    inverse_clarke[phase][0] * mod.input.v_alpha + inverse_clarke[phase][1] * mod.input.v_beta;
                minimum = fmin(minimum, phase_v[phase]);
                maximum = fmax(maximum, phase_v[phase]);
            }
            double lower_offset = fmax(-vn - minimum, -maximum);
            double upper_offset = fmin(vp - maximum, -minimum);
            double slew = mode == 34 ? 5.0 : 25.0;
            double offset = fmax(previous_offset - slew, fmin(previous_offset + slew, mod.output.common_mode_v));
            offset          = fmax(lower_offset, fmin(upper_offset, offset));
            previous_offset = offset;
            svpwm_3level_phase_output_t *p_phases[3] = {&mod.output.phase_a, &mod.output.phase_b, &mod.output.phase_c};
            mod.output.midpoint_current = 0.0f;
            mod.output.common_mode_v    = (float)offset;

            for (int phase = 0; phase < 3; ++phase)
            {
                double pole_voltage     = phase_v[phase] + offset;
                p_phases[phase]->duty_p = (float)fmax(0.0, pole_voltage / vp);
                p_phases[phase]->duty_n = (float)fmax(0.0, -pole_voltage / vn);
                p_phases[phase]->duty_o = 1.0f - p_phases[phase]->duty_p - p_phases[phase]->duty_n;
                mod.output.midpoint_current += p_phases[phase]->duty_o * (float)currents[phase];
            }
        }
        double charge = 0;

        if (delay_periods == 0) /* The normal runner keeps the requested one-period delay. */
        {
            duty[0][0] = mod.output.phase_a.duty_p;
            duty[0][1] = mod.output.phase_a.duty_n;
            duty[1][0] = mod.output.phase_b.duty_p;
            duty[1][1] = mod.output.phase_b.duty_n;
            duty[2][0] = mod.output.phase_c.duty_p;
            duty[2][1] = mod.output.phase_c.duty_n;

            if (    (    (mode >= 30)
                      && (mode <= 33))
                 || (    (    (    mode >= 73
                                && mode < 80)
                           || mode == 81)
                      && (balance_current_filtered > 200.0)))
            {
                for (int p = 0; p < 3; ++p)
                {
                    double correction = dead_time / .0002 * fmax(-1.0, fmin(1.0, currents[p] / 100.0));

                    if (duty[p][0] > 0.0)
                    {
                        duty[p][0] = fmax(0.0, fmin(1.0, duty[p][0] + correction));
                    }
                    else if (duty[p][1] > 0.0)
                    {
                        duty[p][1] = fmax(0.0, fmin(1.0, duty[p][1] - correction));
                    }
                }
            }
        }

        if (    (    (mode == 80)
                  || (mode == 82))
             && (delay_periods == 0))
        {
            production_gate_duties(&mod, duty);
        }

        for (int p = 0; p < 3; ++p)
        {
            average[p] = 0;
        }

        for (int sub = 0; sub < SUBSTEPS; ++sub)
        {
            double carrier = fabs(2 * (sub + .5) / SUBSTEPS - 1);

            if (mode == 82) /* PLECS regular single update at carrier minimum. */
            {
                carrier = 1.0 - carrier;
            }
            double lower[3] = {0}, upper[3] = {0}, pole[3] = {0}, base[10] = {0};

            for (int p = 0; p < 3; ++p)
            {
                int q1 = duty[p][0] > carrier, q2 = 1 - duty[p][1] > carrier;
                int req[4] = {q1, q2, !q1, !q2}, gate[4] = {0};

                for (int q = 0; q < 4; ++q)
                {
                    age[p][q] = req[q] ? age[p][q] + DT : 0;
                    gate[q] = req[q]
                           && age[p][q] > dead_time;
                }
                lower[p] = gate[1] ? (gate[0] ? vp : 0) : -vn;
                upper[p] = gate[2] ? (gate[3] ? -vn : 0) : vp;

                if (lower[p] > upper[p] + 1e-6)
                {
                    puts("invalid gates");
                    return 3;
                }
                pole[p] = (lower[p] + upper[p]) / 2;
            }

            for (int r = 0; r < 10; ++r)
            {
                for (int c = 0; c < 10; ++c)
                {
                    base[r] += plant_a[stage][r][c] * x[c];
                }
            }
            /* Implicit diode complementarity: positive current selects lower path,
             * negative current upper path; a floating pole may hold current at zero. */

            for (int it = 0; it < 30; ++it)
            {
                for (int p = 0; p < 3; ++p)
                {
                    double cur = inverse_clarke[p][0] * base[0] + inverse_clarke[p][1] * base[1];

                    for (int q = 0; q < 3; ++q)
                    {
                        cur += current_gain[stage][p][q] * pole[q];
                    }
                    pole[p] = fmax(lower[p], fmin(upper[p], pole[p] - cur / current_gain[stage][p][p]));
                }
            }
            double old[3] = {0};

            for (int p = 0; p < 3; ++p)
            {
                old[p] = inverse_clarke[p][0] * x[0] + inverse_clarke[p][1] * x[1];
            }

            for (int r = 0; r < 10; ++r)
            {
                x[r] = base[r];

                for (int p = 0; p < 3; ++p)
                {
                    x[r] += plant_b[stage][r][p] * pole[p];
                }
            }

            for (int p = 0; p < 3; ++p)
            {
                double cur = .5 * (old[p] + inverse_clarke[p][0] * x[0] + inverse_clarke[p][1] * x[1]);
                average[p] += cur / SUBSTEPS;

                if (fabs(pole[p]) < 1e-6)
                {
                    charge += cur * DT;
                }
            }

            if (    (k >= 59000) /* Last 0.2 s contains ten complete fundamental cycles. */
                 && ((sub + 1) % (SUBSTEPS / 10) == 0)) /* Ten samples per 200 us control period. */
            {
                fprintf(p_fast, "%.8f", t + (sub + 1) * DT);

                for (int phase = 0; phase < 3; ++phase)
                {
                    double terminal_voltage = 0.0;

                    for (int state = 0; state < 10; ++state)
                    {
                        terminal_voltage += (inverse_clarke[phase][0] * voltage_output[0][0][state]
                                             + inverse_clarke[phase][1] * voltage_output[0][1][state])
                                          * x[state];
                    }
                    fprintf(p_fast, ",%.6f", terminal_voltage);
                }

                for (int phase = 0; phase < 3; ++phase)
                {
                    fprintf(p_fast, ",%.6f", inverse_clarke[phase][0] * x[0] + inverse_clarke[phase][1] * x[1]);
                }
                fprintf(p_fast, "\n");
            }
        }

        if (    (scenario == 3)
             && (mode != 82)) /* Historical snapshot bleeders; absent from the live model. */
        {
            charge += (-vp / 1000.0 + vn / 1100.0) * .0002;
        }
        delta += charge / ((mode == 82) ? .063 : .06);
        peak_delta                                  = fmax(peak_delta, fabs(delta));
        const svpwm_3level_phase_output_t phases[3] = {mod.output.phase_a, mod.output.phase_b, mod.output.phase_c};

        for (int p = 0; p < 3; ++p)
        {
            duty[p][0] = phases[p].duty_p;
            duty[p][1] = phases[p].duty_n;

            if (    (    (mode >= 13)
                      && (mode < 16)) /* Historical differential-damping cases. */
                 || (    (mode >= 30)
                      && (mode <= 33))
                 || (    (    (    mode >= 73
                                && mode < 80)
                           || mode == 81)
                      && (balance_current_filtered > 200.0))) /* Compensate only when fundamental current dominates ripple. */
            {
                double correction = dead_time / .0002 * fmax(-1.0, fmin(1.0, currents[p] / 100.0));

                if (duty[p][0] > 0.0)
                {
                    duty[p][0] = fmax(0.0, fmin(1.0, duty[p][0] + correction));
                }
                else if (duty[p][1] > 0.0)
                {
                    duty[p][1] = fmax(0.0, fmin(1.0, duty[p][1] - correction));
                }
            }
        }

        if (    (mode == 80)
             || (mode == 82))
        {
            production_gate_duties(&mod, duty);
        }

        if (    (k % 50 == 0) /* Decimated startup record. */
             || (k >= 55000)) /* Full-rate final second for sequence and harmonic analysis. */
        {
            fprintf(p_file,
                    "%.5f,%.6f,%.6f,%.6f,%.6f,%.6f",
                    t,
                    delta,
                    mod.output.midpoint_current_ref,
                    mod.output.midpoint_current,
                    charge / .0002,
                    hypot(x[2], x[3]));

            for (int phase = 0; phase < 3; ++phase)
            {
                fprintf(p_file, ",%.6f", (double)sample.v_out[phase]);
            }

            for (int phase = 0; phase < 3; ++phase)
            {
                fprintf(p_file, ",%.6f", (double)sample.i_l[phase]);
            }

            for (int axis = 0; axis < 4; ++axis)
            {
                fprintf(p_file, ",%.6f", (double)control.output.v_dq[axis]);
            }
            fprintf(p_file, ",%d,%d\n", control.output.current_limited, control.output.voltage_limited);
        }
    }
    printf("mode %d delta=%.3f peak=%.3f\n", mode, delta, peak_delta);
    fclose(p_file);
    fclose(p_fast);
    return 0;
}
