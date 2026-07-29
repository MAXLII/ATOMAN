// SPDX-License-Identifier: MIT
/**
 * @file    oled_serial_phy.v
 * @brief   Byte-oriented 4-wire OLED serial physical layer.
 * @details
 *          This module converts a decoupled byte stream into programmable
 *          clock, data, and data/command signals. It has no controller-command
 *          or framebuffer knowledge.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module oled_serial_phy (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [15:0] clock_divider,
    input  wire [7:0]  byte_data,
    input  wire        byte_is_data,
    input  wire        byte_valid,
    output wire        byte_ready,
    output reg         byte_done,
    output wire        busy,
    output reg         oled_clock,
    output reg         oled_data,
    output reg         oled_dc
);

  reg [7:0] shift_register;
  reg [2:0] bit_index;
  reg [15:0] divider_count;
  reg transfer_active;

  assign byte_ready = !transfer_active;
  assign busy = transfer_active;

  always @(posedge clk) begin
    if (!rst_n) begin
      shift_register <= 8'h00;
      bit_index <= 3'd0;
      divider_count <= 16'd0;
      transfer_active <= 1'b0;
      byte_done <= 1'b0;
      oled_clock <= 1'b0;
      oled_data <= 1'b0;
      oled_dc <= 1'b0;
    end else begin
      byte_done <= 1'b0;

      if (!transfer_active) begin
        oled_clock <= 1'b0;
        if (byte_valid) begin
          shift_register <= byte_data;
          bit_index <= 3'd7;
          divider_count <= clock_divider - 16'd1;
          transfer_active <= 1'b1;
          oled_data <= byte_data[7];
          oled_dc <= byte_is_data;
        end
      end else if (divider_count != 16'd0) begin
        divider_count <= divider_count - 16'd1;
      end else begin
        divider_count <= clock_divider - 16'd1;
        if (!oled_clock) begin
          oled_clock <= 1'b1;
        end else begin
          oled_clock <= 1'b0;
          if (bit_index == 3'd0) begin
            transfer_active <= 1'b0;
            byte_done <= 1'b1;
          end else begin
            bit_index <= bit_index - 3'd1;
            shift_register <= {shift_register[6:0], 1'b0};
            oled_data <= shift_register[6];
          end
        end
      end
    end
  end

endmodule

`default_nettype wire
