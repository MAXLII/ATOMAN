// SPDX-License-Identifier: MIT
/**
 * @file    iir_3p3z_core.v
 * @brief   Sequential fixed-point 3P3Z IIR filter core.
 * @details
 *          The core evaluates one third-order direct-form-I sample with one
 *          pipelined signed multiplier. Coefficients use signed Q2.30 format;
 *          input and output samples are signed 32-bit values in a
 *          software-selected engineering scale.
 *
 *          Difference equation:
 *          y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3]
 *                 - a1*y[n-1] - a2*y[n-2] - a3*y[n-3]
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
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
    output wire                     ready,
    output reg                      busy,
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

    localparam [1:0] STATE_IDLE  = 2'd0;
    localparam [1:0] STATE_RUN   = 2'd1;
    localparam [1:0] STATE_FINAL = 2'd2;

    reg [1:0] state;

    reg signed [31:0] x0_work;
    reg signed [31:0] b0_work;
    reg signed [31:0] b1_work;
    reg signed [31:0] b2_work;
    reg signed [31:0] b3_work;
    reg signed [31:0] a1_work;
    reg signed [31:0] a2_work;
    reg signed [31:0] a3_work;

    reg signed [31:0] x1_reg;
    reg signed [31:0] x2_reg;
    reg signed [31:0] x3_reg;
    reg signed [31:0] y1_reg;
    reg signed [31:0] y2_reg;
    reg signed [31:0] y3_reg;

    reg [3:0] issue_count;
    reg [3:0] retire_count;
    reg signed [69:0] accumulator;

    reg signed [31:0] selected_sample;
    reg signed [31:0] selected_coefficient;
    reg               selected_subtract;
    wire              multiplier_input_valid;
    wire              multiplier_output_valid;
    wire signed [63:0] multiplier_product;
    wire              multiplier_subtract;

    wire signed [69:0] product_extended;
    wire signed [69:0] product_term;
    wire signed [69:0] scaled_accumulator;
    wire               scaled_value_fits;
    wire signed [31:0] limited_sample;

    assign ready = !busy;
    assign history_x1 = x1_reg;
    assign history_x2 = x2_reg;
    assign history_x3 = x3_reg;
    assign history_y1 = y1_reg;
    assign history_y2 = y2_reg;
    assign history_y3 = y3_reg;

    assign multiplier_input_valid = (state == STATE_RUN) &&
                                    (issue_count < 4'd7);
    assign product_extended = {{6{multiplier_product[63]}},
                               multiplier_product};
    assign product_term = multiplier_subtract ? -product_extended :
                                                product_extended;
    assign scaled_accumulator = accumulator >>> COEFF_FRAC_BITS;
    assign scaled_value_fits =
        (scaled_accumulator[69:31] == {39{scaled_accumulator[31]}});
    assign limited_sample = scaled_value_fits ? scaled_accumulator[31:0] :
                            (scaled_accumulator[69] ? 32'sh8000_0000 :
                                                     32'sh7FFF_FFFF);

    iir_signed_multiplier_32x32 multiplier (
        .clk(clk),
        .resetn(resetn),
        .clear(clear_state),
        .in_valid(multiplier_input_valid),
        .operand_a(selected_sample),
        .operand_b(selected_coefficient),
        .subtract_in(selected_subtract),
        .out_valid(multiplier_output_valid),
        .product_out(multiplier_product),
        .subtract_out(multiplier_subtract)
    );

    always @(*) begin
        selected_sample = 32'sd0;
        selected_coefficient = 32'sd0;
        selected_subtract = 1'b0;

        case (issue_count)
            4'd0: begin
                selected_sample = x0_work;
                selected_coefficient = b0_work;
            end
            4'd1: begin
                selected_sample = x1_reg;
                selected_coefficient = b1_work;
            end
            4'd2: begin
                selected_sample = x2_reg;
                selected_coefficient = b2_work;
            end
            4'd3: begin
                selected_sample = x3_reg;
                selected_coefficient = b3_work;
            end
            4'd4: begin
                selected_sample = y1_reg;
                selected_coefficient = a1_work;
                selected_subtract = 1'b1;
            end
            4'd5: begin
                selected_sample = y2_reg;
                selected_coefficient = a2_work;
                selected_subtract = 1'b1;
            end
            4'd6: begin
                selected_sample = y3_reg;
                selected_coefficient = a3_work;
                selected_subtract = 1'b1;
            end
            default: begin
            end
        endcase
    end

    always @(posedge clk) begin
        if (resetn == 1'b0) begin
            state <= STATE_IDLE;
            x0_work <= 32'sd0;
            b0_work <= 32'sd0;
            b1_work <= 32'sd0;
            b2_work <= 32'sd0;
            b3_work <= 32'sd0;
            a1_work <= 32'sd0;
            a2_work <= 32'sd0;
            a3_work <= 32'sd0;
            x1_reg <= 32'sd0;
            x2_reg <= 32'sd0;
            x3_reg <= 32'sd0;
            y1_reg <= 32'sd0;
            y2_reg <= 32'sd0;
            y3_reg <= 32'sd0;
            issue_count <= 4'd0;
            retire_count <= 4'd0;
            accumulator <= 70'sd0;
            busy <= 1'b0;
            done <= 1'b0;
            saturated <= 1'b0;
            sample_out <= 32'sd0;
            sample_count <= 32'd0;
        end else if (clear_state == 1'b1) begin
            state <= STATE_IDLE;
            x1_reg <= 32'sd0;
            x2_reg <= 32'sd0;
            x3_reg <= 32'sd0;
            y1_reg <= 32'sd0;
            y2_reg <= 32'sd0;
            y3_reg <= 32'sd0;
            issue_count <= 4'd0;
            retire_count <= 4'd0;
            accumulator <= 70'sd0;
            busy <= 1'b0;
            done <= 1'b0;
            saturated <= 1'b0;
            sample_out <= 32'sd0;
            sample_count <= 32'd0;
        end else begin
            done <= 1'b0;

            case (state)
                STATE_IDLE: begin
                    if (start == 1'b1) begin
                        x0_work <= sample_in;
                        b0_work <= coeff_b0;
                        b1_work <= coeff_b1;
                        b2_work <= coeff_b2;
                        b3_work <= coeff_b3;
                        a1_work <= coeff_a1;
                        a2_work <= coeff_a2;
                        a3_work <= coeff_a3;
                        issue_count <= 4'd0;
                        retire_count <= 4'd0;
                        accumulator <= 70'sd0;
                        busy <= 1'b1;
                        state <= STATE_RUN;
                    end
                end
                STATE_RUN: begin
                    if (issue_count < 4'd7) begin
                        issue_count <= issue_count + 4'd1;
                    end

                    if (multiplier_output_valid == 1'b1) begin
                        accumulator <= accumulator + product_term;
                        if (retire_count == 4'd6) begin
                            retire_count <= 4'd7;
                            state <= STATE_FINAL;
                        end else begin
                            retire_count <= retire_count + 4'd1;
                        end
                    end
                end
                STATE_FINAL: begin
                    x3_reg <= x2_reg;
                    x2_reg <= x1_reg;
                    x1_reg <= x0_work;
                    y3_reg <= y2_reg;
                    y2_reg <= y1_reg;
                    y1_reg <= limited_sample;
                    sample_out <= limited_sample;
                    sample_count <= sample_count + 32'd1;
                    saturated <= !scaled_value_fits;
                    busy <= 1'b0;
                    done <= 1'b1;
                    state <= STATE_IDLE;
                end
                default: begin
                    busy <= 1'b0;
                    state <= STATE_IDLE;
                end
            endcase
        end
    end

endmodule

/**
 * @brief Fully pipelined signed 32-by-32-bit multiplier.
 *
 * The operands are split into signed 17-bit quadrants. Each native-width
 * partial product has input, multiplier-output, and DSP-output registers so
 * Vivado can use the AREG/BREG, MREG, and PREG stages of DSP48E1 resources.
 */
module iir_signed_multiplier_32x32 (
    input  wire                     clk,
    input  wire                     resetn,
    input  wire                     clear,
    input  wire                     in_valid,
    input  wire signed [31:0]       operand_a,
    input  wire signed [31:0]       operand_b,
    input  wire                     subtract_in,
    output reg                      out_valid,
    output reg signed [63:0]        product_out,
    output reg                      subtract_out
);

    reg signed [16:0] a_high_stage0;
    reg signed [16:0] a_low_stage0;
    reg signed [16:0] b_high_stage0;
    reg signed [16:0] b_low_stage0;
    reg               subtract_stage0;
    reg               valid_stage0;

    (* use_dsp = "yes" *) reg signed [33:0] product_hh_stage1;
    (* use_dsp = "yes" *) reg signed [33:0] product_hl_stage1;
    (* use_dsp = "yes" *) reg signed [33:0] product_lh_stage1;
    (* use_dsp = "yes" *) reg signed [33:0] product_ll_stage1;
    reg               subtract_stage1;
    reg               valid_stage1;

    reg signed [33:0] product_hh_stage2;
    reg signed [33:0] product_hl_stage2;
    reg signed [33:0] product_lh_stage2;
    reg signed [33:0] product_ll_stage2;
    reg               subtract_stage2;
    reg               valid_stage2;

    wire signed [69:0] product_hh_extended;
    wire signed [69:0] product_hl_extended;
    wire signed [69:0] product_lh_extended;
    wire signed [69:0] product_ll_extended;
    wire signed [69:0] product_combined;

    assign product_hh_extended = {{36{product_hh_stage2[33]}},
                                  product_hh_stage2};
    assign product_hl_extended = {{36{product_hl_stage2[33]}},
                                  product_hl_stage2};
    assign product_lh_extended = {{36{product_lh_stage2[33]}},
                                  product_lh_stage2};
    assign product_ll_extended = {{36{product_ll_stage2[33]}},
                                  product_ll_stage2};
    assign product_combined = (product_hh_extended <<< 32) +
                              (product_hl_extended <<< 16) +
                              (product_lh_extended <<< 16) +
                              product_ll_extended;

    always @(posedge clk) begin
        if ((resetn == 1'b0) || (clear == 1'b1)) begin
            a_high_stage0 <= 17'sd0;
            a_low_stage0 <= 17'sd0;
            b_high_stage0 <= 17'sd0;
            b_low_stage0 <= 17'sd0;
            subtract_stage0 <= 1'b0;
            valid_stage0 <= 1'b0;
            product_hh_stage1 <= 34'sd0;
            product_hl_stage1 <= 34'sd0;
            product_lh_stage1 <= 34'sd0;
            product_ll_stage1 <= 34'sd0;
            subtract_stage1 <= 1'b0;
            valid_stage1 <= 1'b0;
            product_hh_stage2 <= 34'sd0;
            product_hl_stage2 <= 34'sd0;
            product_lh_stage2 <= 34'sd0;
            product_ll_stage2 <= 34'sd0;
            subtract_stage2 <= 1'b0;
            valid_stage2 <= 1'b0;
            out_valid <= 1'b0;
            product_out <= 64'sd0;
            subtract_out <= 1'b0;
        end else begin
            a_high_stage0 <= {operand_a[31], operand_a[31:16]};
            a_low_stage0 <= {1'b0, operand_a[15:0]};
            b_high_stage0 <= {operand_b[31], operand_b[31:16]};
            b_low_stage0 <= {1'b0, operand_b[15:0]};
            subtract_stage0 <= subtract_in;
            valid_stage0 <= in_valid;

            product_hh_stage1 <= a_high_stage0 * b_high_stage0;
            product_hl_stage1 <= a_high_stage0 * b_low_stage0;
            product_lh_stage1 <= a_low_stage0 * b_high_stage0;
            product_ll_stage1 <= a_low_stage0 * b_low_stage0;
            subtract_stage1 <= subtract_stage0;
            valid_stage1 <= valid_stage0;

            product_hh_stage2 <= product_hh_stage1;
            product_hl_stage2 <= product_hl_stage1;
            product_lh_stage2 <= product_lh_stage1;
            product_ll_stage2 <= product_ll_stage1;
            subtract_stage2 <= subtract_stage1;
            valid_stage2 <= valid_stage1;

            product_out <= product_combined[63:0];
            subtract_out <= subtract_stage2;
            out_valid <= valid_stage2;
        end
    end

endmodule

`default_nettype wire
