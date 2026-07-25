// SPDX-License-Identifier: MIT
/**
 * @file    iir_3p3z_core.v
 * @brief   Single-cycle fixed-point 3P3Z IIR filter core.
 * @details
 *          Seven signed products are evaluated in parallel and reduced by a
 *          balanced combinational adder tree. Coefficients use signed Q2.30
 *          format; input and output samples are signed 32-bit values in a
 *          software-selected engineering scale.
 *
 *          Difference equation:
 *          y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
 *                 - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
 *
 *          The configured lower and upper limits are applied before the
 *          result is registered. The limited value is both sample_out and
 *          the y[n-1] feedback history for the next accepted sample.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 2.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module iir_3p3z_core #(
    parameter integer COEFF_FRAC_BITS = 30
)(
    input  wire                     clk,
    input  wire                     resetn,
    input  wire                     start,
    input  wire                     clear_state,
    input  wire signed [31:0]       sample_in,
    input  wire signed [31:0]       coeff_b0,
    input  wire signed [31:0]       coeff_b1,
    input  wire signed [31:0]       coeff_b2,
    input  wire signed [31:0]       coeff_b3,
    input  wire signed [31:0]       coeff_a1,
    input  wire signed [31:0]       coeff_a2,
    input  wire signed [31:0]       coeff_a3,
    input  wire signed [31:0]       limit_lower,
    input  wire signed [31:0]       limit_upper,
    output wire                     ready,
    output wire                     busy,
    output reg                      done,
    output reg                      saturated,
    output reg signed [31:0]        sample_out,
    output reg [31:0]               sample_count,
    output wire signed [31:0]       history_x1,
    output wire signed [31:0]       history_x2,
    output wire signed [31:0]       history_x3,
    output wire signed [31:0]       history_y1,
    output wire signed [31:0]       history_y2,
    output wire signed [31:0]       history_y3
);

    reg signed [31:0] x1_reg;
    reg signed [31:0] x2_reg;
    reg signed [31:0] x3_reg;
    reg signed [31:0] y1_reg;
    reg signed [31:0] y2_reg;
    reg signed [31:0] y3_reg;

    (* use_dsp = "no" *) wire signed [63:0] product_b0;
    (* use_dsp = "no" *) wire signed [63:0] product_b1;
    (* use_dsp = "no" *) wire signed [63:0] product_b2;
    (* use_dsp = "no" *) wire signed [63:0] product_b3;
    (* use_dsp = "no" *) wire signed [63:0] product_a1;
    (* use_dsp = "no" *) wire signed [63:0] product_a2;
    (* use_dsp = "no" *) wire signed [63:0] product_a3;

    wire signed [69:0] product_b0_extended;
    wire signed [69:0] product_b1_extended;
    wire signed [69:0] product_b2_extended;
    wire signed [69:0] product_b3_extended;
    wire signed [69:0] product_a1_extended;
    wire signed [69:0] product_a2_extended;
    wire signed [69:0] product_a3_extended;
    wire signed [69:0] feedforward_sum_01;
    wire signed [69:0] feedforward_sum_23;
    wire signed [69:0] feedback_sum_12;
    wire signed [69:0] feedforward_sum;
    wire signed [69:0] feedback_sum;
    wire signed [69:0] accumulator;
    wire signed [69:0] scaled_result;
    wire signed [69:0] limit_lower_extended;
    wire signed [69:0] limit_upper_extended;
    wire               clamp_lower;
    wire               clamp_upper;
    wire signed [31:0] limited_sample;

    assign ready = 1'b1;
    assign busy = 1'b0;
    assign history_x1 = x1_reg;
    assign history_x2 = x2_reg;
    assign history_x3 = x3_reg;
    assign history_y1 = y1_reg;
    assign history_y2 = y2_reg;
    assign history_y3 = y3_reg;

    assign product_b0 = sample_in * coeff_b0;
    assign product_b1 = x1_reg * coeff_b1;
    assign product_b2 = x2_reg * coeff_b2;
    assign product_b3 = x3_reg * coeff_b3;
    assign product_a1 = y1_reg * coeff_a1;
    assign product_a2 = y2_reg * coeff_a2;
    assign product_a3 = y3_reg * coeff_a3;

    assign product_b0_extended = {{6{product_b0[63]}}, product_b0};
    assign product_b1_extended = {{6{product_b1[63]}}, product_b1};
    assign product_b2_extended = {{6{product_b2[63]}}, product_b2};
    assign product_b3_extended = {{6{product_b3[63]}}, product_b3};
    assign product_a1_extended = {{6{product_a1[63]}}, product_a1};
    assign product_a2_extended = {{6{product_a2[63]}}, product_a2};
    assign product_a3_extended = {{6{product_a3[63]}}, product_a3};

    assign feedforward_sum_01 = product_b0_extended + product_b1_extended;
    assign feedforward_sum_23 = product_b2_extended + product_b3_extended;
    assign feedback_sum_12 = product_a1_extended + product_a2_extended;
    assign feedforward_sum = feedforward_sum_01 + feedforward_sum_23;
    assign feedback_sum = feedback_sum_12 + product_a3_extended;
    assign accumulator = feedforward_sum - feedback_sum;
    assign scaled_result = accumulator >>> COEFF_FRAC_BITS;

    assign limit_lower_extended = {{38{limit_lower[31]}}, limit_lower};
    assign limit_upper_extended = {{38{limit_upper[31]}}, limit_upper};
    assign clamp_upper = scaled_result > limit_upper_extended;
    assign clamp_lower = scaled_result < limit_lower_extended;
    assign limited_sample = clamp_upper ? limit_upper :
                            (clamp_lower ? limit_lower : scaled_result[31:0]);

    always @(posedge clk) begin
        if (resetn == 1'b0) begin
            x1_reg <= 32'sd0;
            x2_reg <= 32'sd0;
            x3_reg <= 32'sd0;
            y1_reg <= 32'sd0;
            y2_reg <= 32'sd0;
            y3_reg <= 32'sd0;
            done <= 1'b0;
            saturated <= 1'b0;
            sample_out <= 32'sd0;
            sample_count <= 32'd0;
        end else if (clear_state == 1'b1) begin
            x1_reg <= 32'sd0;
            x2_reg <= 32'sd0;
            x3_reg <= 32'sd0;
            y1_reg <= 32'sd0;
            y2_reg <= 32'sd0;
            y3_reg <= 32'sd0;
            done <= 1'b0;
            saturated <= 1'b0;
            sample_out <= 32'sd0;
            sample_count <= 32'd0;
        end else begin
            done <= 1'b0;
            if (start == 1'b1) begin
                x3_reg <= x2_reg;
                x2_reg <= x1_reg;
                x1_reg <= sample_in;
                y3_reg <= y2_reg;
                y2_reg <= y1_reg;
                y1_reg <= limited_sample;
                sample_out <= limited_sample;
                sample_count <= sample_count + 32'd1;
                saturated <= clamp_upper || clamp_lower;
                done <= 1'b1;
            end
        end
    end

endmodule

`default_nettype wire
