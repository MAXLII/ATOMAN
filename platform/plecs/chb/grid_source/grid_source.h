// SPDX-License-Identifier: MIT
/**
 * @file grid_source.h
 * @brief Independent harmonic grid-source simulation interface.
 * @details Owns the source phase and frequency ramp independently of CHB control.
 * @author Max.Li
 * @date 2026-09-27
 * @version 1.0.0
 * Copyright (c) 2026 Max.Li. Licensed under the MIT License.
 */
#ifndef CHB_GRID_SOURCE_H
#define CHB_GRID_SOURCE_H

/** @brief Reset the simulated source and its online parameters before a run. */
void grid_source_reset(void);
/** @param time_s Absolute PLECS simulation time, s. @return Source voltage, V. */
float grid_source_step(double time_s);
/** @param time_s Absolute PLECS simulation time, s; publish the source voltage. */
void grid_source_output_step(double time_s);

#endif /* CHB_GRID_SOURCE_H */
