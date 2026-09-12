// SPDX-License-Identifier: MIT
/**
 * @file    test_svpwm_3level.c
 * @brief   Independent voltage and waveform checks for the production SVPWM library.
 * @details
 *          This file is part of the digital power framework project.
 *          - Check physical voltage feasibility and reconstruction in double precision
 *          - Integrate the documented switching sequence and check error invalidation
 *          - Exercise the actual library without MCU dependencies or algorithm stubs
 *          C11 compatible; host-only test; no hardware access.
 * @author  Max.Li
 * @date    2026-09-12
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li.
 * All rights reserved.
 * This file is licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */
#include "svpwm_3level.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) check((condition), #condition, __LINE__) /* Keep checks active at every optimization level. */

static uint32_t check_count = 0u;  /* Number of assertions executed. */
static uint32_t sample_count = 0u; /* Number of independent reference points checked. */
static uint32_t ti_count = 0u;     /* Equal-bus cases compared to a geometric mapped-vector oracle. */

/** @param passed Assertion result. @param p_text Failed expression. @param line Source line. */
static void check(bool passed, const char *p_text, int line)
{
    ++check_count;
    if (passed == false)
    {
        (void)fprintf(stderr, "FAIL line %d: %s (sample %lu)\n", line, p_text, (unsigned long)sample_count);
        exit(EXIT_FAILURE);
    }
}

/** @param p_phase One phase result. */
static void check_phase(const svpwm_3level_phase_output_t *p_phase)
{
    CHECK(isfinite(p_phase->duty_p) != 0);
    CHECK(isfinite(p_phase->duty_o) != 0);
    CHECK(isfinite(p_phase->duty_n) != 0);
    CHECK(p_phase->duty_p >= 0.0f);
    CHECK(p_phase->duty_p <= 1.0f);
    CHECK(p_phase->duty_o >= 0.0f);
    CHECK(p_phase->duty_o <= 1.0f);
    CHECK(p_phase->duty_n >= 0.0f);
    CHECK(p_phase->duty_n <= 1.0f);
    CHECK(fabs((double)p_phase->duty_p + p_phase->duty_o + p_phase->duty_n - 1.0) < 2.0e-7);
    CHECK((p_phase->duty_p == 0.0f) || /* A phase may use O/N. */
          (p_phase->duty_n == 0.0f)); /* Or O/P, but never P/N within a period. */
}

/** @param p_phase Phase result. @param t Normalized sample time. @return P=1, O=0, N=-1. */
static int level_at(const svpwm_3level_phase_output_t *p_phase, double t)
{
    if (fabs(t - 0.5) < (0.5 * (double)p_phase->duty_p))
    {
        return 1;
    }
    if ((t < (0.5 * (double)p_phase->duty_n)) ||         /* Beginning N segment. */
        (t > (1.0 - (0.5 * (double)p_phase->duty_n))))   /* Ending N segment. */
    {
        return -1;
    }
    return 0;
}

/** @param p_svpwm Valid instance whose exact switching intervals are integrated. */
static void check_waveform(const svpwm_3level_t *p_svpwm)
{
    const svpwm_3level_phase_output_t *p_phases[3] = { /* Public phase outputs in A/B/C order. */
        &p_svpwm->output.phase_a, &p_svpwm->output.phase_b, &p_svpwm->output.phase_c
    };
    double edges[8] = {0.0, 1.0}; /* Period boundaries plus 2 transitions per phase. */
    double mean[3] = {0.0};       /* Pole-voltage integrals in volts. */
    int previous[3] = {0};        /* Previous segment states for transition checks. */
    uint32_t segments = 0u;       /* Number of nonzero-duration segments. */

    for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
    {
        double high = (p_phases[i]->duty_p > 0.0f) ? p_phases[i]->duty_p : p_phases[i]->duty_o;
        /* High virtual-state duty: P in O/P, or O in N/O. */
        edges[2u + (2u * i)] = (1.0 - high) * 0.5;
        edges[3u + (2u * i)] = (1.0 + high) * 0.5;
    }
    for (uint32_t i = 1u; i < 8u; ++i) /* Insertion-sort source index. */
    {
        double edge = edges[i]; /* Edge being inserted. */
        uint32_t j = i;         /* Destination position. */
        while (j > 0u)
        {
            if (edges[j - 1u] <= edge)
            {
                break;
            }
            edges[j] = edges[j - 1u];
            --j;
        }
        edges[j] = edge;
    }
    for (uint32_t i = 0u; i < 7u; ++i) /* Candidate segment index. */
    {
        double duration = edges[i + 1u] - edges[i]; /* Segment fraction of the full period. */
        double t = 0.5 * (edges[i + 1u] + edges[i]); /* Interior sample avoids edge conventions. */
        if (duration <= 1.0e-12)
        {
            continue;
        }
        for (uint32_t j = 0u; j < 3u; ++j) /* Phase index. */
        {
            int level = level_at(p_phases[j], t); /* Current pole level. */
            CHECK(level == level_at(p_phases[j], 1.0 - t));
            if (segments > 0u)
            {
                CHECK(abs(level - previous[j]) <= 1);
            }
            previous[j] = level;
            if (level > 0)
            {
                mean[j] += duration * p_svpwm->input.v_dc_p;
            }
            if (level < 0)
            {
                mean[j] -= duration * p_svpwm->input.v_dc_n;
            }
        }
        ++segments;
    }
    CHECK(segments <= 7u);
    for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
    {
        double expected = ((double)p_phases[i]->duty_p * p_svpwm->input.v_dc_p) -
                          ((double)p_phases[i]->duty_n * p_svpwm->input.v_dc_n); /* Dwell-derived pole voltage. */
        CHECK(fabs(mean[i] - expected) <= (2.0e-7 * fmax(p_svpwm->input.v_dc_p, p_svpwm->input.v_dc_n)));
    }
}

/** @param p_svpwm Valid equal-bus result to compare against geometric vector dwell times. */
static void check_ti_balanced(const svpwm_3level_t *p_svpwm)
{
    const uint32_t states[6] = {1u, 3u, 2u, 6u, 4u, 5u}; /* Counterclockwise 2-level active vectors. */
    const double pi = acos(-1.0);                  /* Geometric angle unit. */
    const double alpha = (double)p_svpwm->input.v_alpha / p_svpwm->input.v_dc_p; /* Half-bus-normalized alpha. */
    const double beta = (double)p_svpwm->input.v_beta / p_svpwm->input.v_dc_p;   /* Half-bus-normalized beta. */
    const double phases[3] = {alpha, (-0.5 * alpha) + (sqrt(3.0) * beta * 0.5),
                             (-0.5 * alpha) - (sqrt(3.0) * beta * 0.5)}; /* Independent sign-sector references. */
    const svpwm_3level_phase_output_t *p_phases[3] = { /* Actual phase results. */
        &p_svpwm->output.phase_a, &p_svpwm->output.phase_b, &p_svpwm->output.phase_c
    };
    uint32_t mask = 0u;    /* Main-sector O/P bit mask. */
    uint32_t main_sector = 0u; /* Zero-based main sector. */
    uint32_t sub;          /* Zero-based sector after subtracting the mapping vector. */
    double mapped_alpha;  /* Reference relative to the selected subhexagon center. */
    double mapped_beta;   /* Reference relative to the selected subhexagon center. */
    double theta;         /* Angle of the mapped reference. */
    double radius;        /* Magnitude of the mapped reference. */
    double t1;            /* Lower-angle active-vector dwell fraction. */
    double t2;            /* Upper-angle active-vector dwell fraction. */
    double t0;            /* Combined virtual zero-vector fraction. */
    for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
    {
        if (fabs(phases[i]) < 1.0e-5)
        {
            return; /* Tied sign sectors may legitimately choose different redundant states. */
        }
        if (phases[i] > 0.0)
        {
            mask |= 1u << i;
        }
    }
    while (states[main_sector] != mask)
    {
        ++main_sector;
        CHECK(main_sector < 6u);
    }
    mapped_alpha = alpha - ((2.0 / 3.0) * cos((double)main_sector * pi / 3.0));
    mapped_beta = beta - ((2.0 / 3.0) * sin((double)main_sector * pi / 3.0));
    theta = atan2(mapped_beta, mapped_alpha);
    if (theta < 0.0)
    {
        theta += 2.0 * pi;
    }
    sub = (uint32_t)fmin(5.0, floor(theta / (pi / 3.0)));
    radius = hypot(mapped_alpha, mapped_beta);
    t1 = sqrt(3.0) * radius * sin(((double)sub + 1.0) * pi / 3.0 - theta);
    t2 = sqrt(3.0) * radius * sin(theta - (double)sub * pi / 3.0);
    t0 = 1.0 - t1 - t2;
    for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
    {
        double expected = 0.5 * t0; /* Equal allocation to virtual 000 and 111. */
        double actual = ((mask & (1u << i)) != 0u) ? p_phases[i]->duty_p : p_phases[i]->duty_o;
        /* Actual virtual high-state duty, selected by the independent sector oracle. */
        if ((states[sub] & (1u << i)) != 0u)
        {
            expected += t1;
        }
        if ((states[(sub + 1u) % 6u] & (1u << i)) != 0u)
        {
            expected += t2;
        }
        CHECK(fabs(actual - expected) < 2.0e-6);
    }
    ++ti_count;
}

/** @param p_svpwm Instance to calculate and check against a physical double-precision oracle. */
static void verify_sample(svpwm_3level_t *p_svpwm)
{
    const double alpha = p_svpwm->input.v_alpha; /* Exact float input promoted for the oracle. */
    const double beta = p_svpwm->input.v_beta;   /* Exact float input promoted for the oracle. */
    const double vb = (-0.5 * alpha) + (sqrt(3.0) * 0.5 * beta); /* Independent phase B reference. */
    const double vc = (-0.5 * alpha) - (sqrt(3.0) * 0.5 * beta); /* Independent phase C reference. */
    const double spread = fmax(alpha, fmax(vb, vc)) - fmin(alpha, fmin(vb, vc)); /* Required bus span. */
    const double bus = (double)p_svpwm->input.v_dc_p + p_svpwm->input.v_dc_n; /* Available physical span. */
    const double tolerance = 4.0e-6 * fmax(p_svpwm->input.v_dc_p, p_svpwm->input.v_dc_n); /* Numeric boundary margin. */
    SVPWM_3LEVEL_STATUS_E status = svpwm_3level_cal(p_svpwm); /* Production result. */
    ++sample_count;
    CHECK(status == p_svpwm->output.status);
    if (spread < (bus - tolerance))
    {
        CHECK(status == SVPWM_3LEVEL_OK);
    }
    if (spread > (bus + tolerance))
    {
        CHECK(status == SVPWM_3LEVEL_OUT_OF_RANGE);
    }
    if (status == SVPWM_3LEVEL_OK)
    {
        const double va_out = ((double)p_svpwm->output.phase_a.duty_p * p_svpwm->input.v_dc_p) -
                              ((double)p_svpwm->output.phase_a.duty_n * p_svpwm->input.v_dc_n); /* Actual phase A pole. */
        const double vb_out = ((double)p_svpwm->output.phase_b.duty_p * p_svpwm->input.v_dc_p) -
                              ((double)p_svpwm->output.phase_b.duty_n * p_svpwm->input.v_dc_n); /* Actual phase B pole. */
        const double vc_out = ((double)p_svpwm->output.phase_c.duty_p * p_svpwm->input.v_dc_p) -
                              ((double)p_svpwm->output.phase_c.duty_n * p_svpwm->input.v_dc_n); /* Actual phase C pole. */
        check_phase(&p_svpwm->output.phase_a);
        check_phase(&p_svpwm->output.phase_b);
        check_phase(&p_svpwm->output.phase_c);
        CHECK(fabs(((2.0 * va_out - vb_out - vc_out) / 3.0) - alpha) <= tolerance);
        CHECK(fabs(((vb_out - vc_out) / sqrt(3.0)) - beta) <= tolerance);
        check_waveform(p_svpwm);
        if (p_svpwm->input.v_dc_p == p_svpwm->input.v_dc_n)
        {
            check_ti_balanced(p_svpwm);
        }
    }
    else
    {
        const svpwm_3level_phase_output_t *p_phases[3] = { /* Invalid output must not retain any old dwell. */
            &p_svpwm->output.phase_a, &p_svpwm->output.phase_b, &p_svpwm->output.phase_c
        };
        for (uint32_t i = 0u; i < 3u; ++i) /* Phase index. */
        {
            CHECK(p_phases[i]->duty_p == 0.0f);
            CHECK(p_phases[i]->duty_o == 0.0f);
            CHECK(p_phases[i]->duty_n == 0.0f);
        }
    }
}

/** @brief Cover full rotations inside, at and beyond the physical hexagon. */
static void test_sweep(void)
{
    const float buses[][2] = { /* Equal, unequal, extreme-scale and nearly one-sided buses. */
        {350.0f, 350.0f}, {250.0f, 450.0f}, {450.0f, 250.0f},
        {10.0f, 690.0f}, {690.0f, 10.0f}, {0.001f, 0.001f},
        {1.0e30f, 2.0e30f}, {1.0e-30f, 2.0e-30f}, {0.001f, 700.0f}
    };
    const double radii[] = {0.0, 0.05, 0.5, 0.9, 0.99999, 1.0, 1.00001, 1.05}; /* Fractions of the directional boundary. */
    for (size_t b = 0u; b < (sizeof(buses) / sizeof(buses[0])); ++b) /* Bus case index. */
    {
        svpwm_3level_t instance = {0}; /* Real production instance. */
        svpwm_3level_cfg_t cfg = {0.5f * fminf(buses[b][0], buses[b][1])}; /* Valid floor for this voltage scale. */
        CHECK(svpwm_3level_init(&instance, &cfg) == true);
        instance.input.v_dc_p = buses[b][0];
        instance.input.v_dc_n = buses[b][1];
        for (uint32_t angle = 0u; angle < 720u; ++angle) /* Half-degree angular step. */
        {
            double theta = (double)angle * (acos(-1.0) / 360.0); /* Angle in radians. */
            double a = cos(theta);                              /* Unit alpha reference. */
            double beta = sin(theta);                           /* Unit beta reference. */
            double vb = (-0.5 * a) + (sqrt(3.0) * 0.5 * beta);   /* Unit phase B. */
            double vc = (-0.5 * a) - (sqrt(3.0) * 0.5 * beta);   /* Unit phase C. */
            double limit = ((double)buses[b][0] + buses[b][1]) /
                           (fmax(a, fmax(vb, vc)) - fmin(a, fmin(vb, vc))); /* Physical hexagon radius. */
            for (size_t r = 0u; r < (sizeof(radii) / sizeof(radii[0])); ++r) /* Radial sample index. */
            {
                instance.input.v_alpha = (float)(a * limit * radii[r]);
                instance.input.v_beta = (float)(beta * limit * radii[r]);
                verify_sample(&instance);
            }
        }
    }
}

/** @brief Check lifecycle, invalidation and a known SPRABS6 sector-1 sequence. */
static void test_lifecycle(void)
{
    svpwm_3level_t instance = {0};          /* Test instance repeatedly recovered from faults. */
    svpwm_3level_cfg_t cfg = {10.0f};       /* Each half bus must be at least 10 V. */
    const float invalid[] = {NAN, INFINITY, -INFINITY}; /* Non-finite controller and measurement values. */
    CHECK(svpwm_3level_init(NULL, &cfg) == false);
    CHECK(svpwm_3level_cal(NULL) == SVPWM_3LEVEL_INVALID_ARGUMENT);
    svpwm_3level_reset(NULL);
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_CONFIG);
    CHECK(svpwm_3level_init(&instance, NULL) == false);
    CHECK(instance.output.status == SVPWM_3LEVEL_INVALID_ARGUMENT);
    CHECK(svpwm_3level_init(&instance, &cfg) == true);
    CHECK(instance.output.status == SVPWM_3LEVEL_NOT_READY);
    instance.input = (svpwm_3level_input_t){300.0f, 100.0f, 350.0f, 350.0f};
    verify_sample(&instance);
    /* Sector 1, ordered virtual duties A > B > C: ONN, PNN, PON, POO, then mirror. */
    CHECK(fabs((double)instance.output.phase_a.duty_p - 0.766575057683492) < 2.0e-7);
    CHECK(fabs((double)instance.output.phase_b.duty_o - 0.728296601621902) < 2.0e-7);
    CHECK(fabs((double)instance.output.phase_c.duty_o - 0.233424942316508) < 2.0e-7);
    svpwm_3level_reset(&instance);
    CHECK(instance.output.status == SVPWM_3LEVEL_NOT_READY);
    CHECK(instance.output.phase_a.duty_p == 0.0f);
    CHECK(instance.input.v_alpha == 300.0f);
    CHECK(instance.cfg.v_dc_half_min == 10.0f);
    verify_sample(&instance);
    CHECK(svpwm_3level_init(&instance, &instance.cfg) == true);
    CHECK(instance.cfg.v_dc_half_min == 10.0f);
    CHECK(instance.input.v_alpha == 0.0f);
    for (size_t i = 0u; i < (sizeof(invalid) / sizeof(invalid[0])); ++i) /* Non-finite case index. */
    {
        cfg.v_dc_half_min = 10.0f;
        CHECK(svpwm_3level_init(&instance, &cfg) == true);
        for (uint32_t field = 0u; field < 4u; ++field) /* Input member being faulted. */
        {
            instance.input = (svpwm_3level_input_t){300.0f, 100.0f, 350.0f, 350.0f};
            verify_sample(&instance);
            switch (field)
            {
                case 0u: instance.input.v_alpha = invalid[i]; break;
                case 1u: instance.input.v_beta = invalid[i]; break;
                case 2u: instance.input.v_dc_p = invalid[i]; break;
                default: instance.input.v_dc_n = invalid[i]; break;
            }
            CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_INPUT);
            CHECK(instance.output.phase_a.duty_p == 0.0f);
            CHECK(instance.output.phase_b.duty_o == 0.0f);
            CHECK(instance.output.phase_c.duty_n == 0.0f);
        }
        cfg.v_dc_half_min = invalid[i];
        CHECK(svpwm_3level_init(&instance, &cfg) == false);
        CHECK(instance.output.status == SVPWM_3LEVEL_INVALID_CONFIG);
    }
    cfg.v_dc_half_min = 0.0f;
    CHECK(svpwm_3level_init(&instance, &cfg) == false);
    cfg.v_dc_half_min = -1.0f;
    CHECK(svpwm_3level_init(&instance, &cfg) == false);
    cfg.v_dc_half_min = 10.0f;
    CHECK(svpwm_3level_init(&instance, &cfg) == true);
    instance.input = (svpwm_3level_input_t){0.0f, 0.0f, 10.0f, 10.0f};
    verify_sample(&instance);
    CHECK(instance.output.phase_a.duty_o == 1.0f);
    instance.input.v_dc_p = nextafterf(10.0f, 0.0f);
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_INPUT);
    instance.input.v_dc_p = 10.0f;
    instance.input.v_dc_n = 0.0f;
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_INPUT);
    instance.input = (svpwm_3level_input_t){FLT_MAX, FLT_MAX, 350.0f, 350.0f};
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_OUT_OF_RANGE);
    instance.input = (svpwm_3level_input_t){0.5f * FLT_MAX, 0.0f, FLT_MAX, FLT_MAX};
    verify_sample(&instance);
    instance.cfg.v_dc_half_min = nanf("");
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_CONFIG);
    CHECK(instance.output.phase_a.duty_p == 0.0f);
    cfg.v_dc_half_min = nextafterf(0.0f, 1.0f);
    CHECK(svpwm_3level_init(&instance, &cfg) == true);
    instance.input = (svpwm_3level_input_t){0.0f, 0.0f, cfg.v_dc_half_min, FLT_MAX};
    CHECK(svpwm_3level_cal(&instance) == SVPWM_3LEVEL_INVALID_INPUT);
}

/** @return EXIT_SUCCESS when every invariant passes. */
int main(void)
{
    test_lifecycle(); /* Verify initialization and failure behavior before broad sweeps. */
    test_sweep();     /* Validate physical reconstruction across the full operating area. */
    (void)printf("PASS: %lu samples, %lu checks, %lu TI comparisons\n",
                 (unsigned long)sample_count, (unsigned long)check_count, (unsigned long)ti_count);
    return EXIT_SUCCESS;
}
