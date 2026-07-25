// SPDX-License-Identifier: MIT
/**
 * @file    ssd1306_protocol.v
 * @brief   SSD1306 command and framebuffer protocol engine.
 * @details
 *          This module translates generic display operations and frame bytes
 *          into command/data byte transactions. Electrical serial timing is
 *          delegated to the byte-oriented physical layer.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module ssd1306_protocol #(
    parameter integer RESET_LOW_CYCLES = 500000,
    parameter integer RESET_HIGH_CYCLES = 500000
)(
    input  wire       clk,
    input  wire       rst_n,
    input  wire [2:0] operation,
    input  wire [7:0] operation_value,
    input  wire       operation_valid,
    output wire       operation_ready,
    output reg        operation_done,
    output reg        protocol_error,
    input  wire [7:0] frame_data,
    input  wire       frame_data_valid,
    input  wire       frame_data_last,
    output wire       frame_data_ready,
    output wire [7:0] phy_byte_data,
    output wire       phy_byte_is_data,
    output wire       phy_byte_valid,
    input  wire       phy_byte_ready,
    input  wire       phy_busy,
    output reg        oled_reset_n,
    output wire       busy
);

  localparam [2:0] OP_INIT = 3'd0;
  localparam [2:0] OP_FRAME = 3'd1;
  localparam [2:0] OP_CLEAR = 3'd2;
  localparam [2:0] OP_DISPLAY = 3'd3;
  localparam [2:0] OP_INVERT = 3'd4;
  localparam [2:0] OP_CONTRAST = 3'd5;

  localparam [3:0] STATE_IDLE = 4'd0;
  localparam [3:0] STATE_RESET_LOW = 4'd1;
  localparam [3:0] STATE_RESET_HIGH = 4'd2;
  localparam [3:0] STATE_INIT = 4'd3;
  localparam [3:0] STATE_ADDRESS = 4'd4;
  localparam [3:0] STATE_FRAME = 4'd5;
  localparam [3:0] STATE_CLEAR = 4'd6;
  localparam [3:0] STATE_SIMPLE = 4'd7;
  localparam [3:0] STATE_WAIT_PHY = 4'd8;

  reg [3:0] state;
  reg [31:0] reset_count;
  reg [5:0] sequence_index;
  reg [9:0] frame_index;
  reg [7:0] saved_value;
  reg [2:0] saved_operation;
  reg [1:0] simple_length;

  function [7:0] initialization_byte;
    input [5:0] index;
    begin
      case (index)
        6'd0: initialization_byte = 8'hAE;
        6'd1: initialization_byte = 8'hD5;
        6'd2: initialization_byte = 8'h80;
        6'd3: initialization_byte = 8'hA8;
        6'd4: initialization_byte = 8'h3F;
        6'd5: initialization_byte = 8'hD3;
        6'd6: initialization_byte = 8'h00;
        6'd7: initialization_byte = 8'h40;
        6'd8: initialization_byte = 8'h8D;
        6'd9: initialization_byte = 8'h14;
        6'd10: initialization_byte = 8'h20;
        6'd11: initialization_byte = 8'h00;
        6'd12: initialization_byte = 8'hA1;
        6'd13: initialization_byte = 8'hC8;
        6'd14: initialization_byte = 8'hDA;
        6'd15: initialization_byte = 8'h12;
        6'd16: initialization_byte = 8'h81;
        6'd17: initialization_byte = 8'hCF;
        6'd18: initialization_byte = 8'hD9;
        6'd19: initialization_byte = 8'hF1;
        6'd20: initialization_byte = 8'hDB;
        6'd21: initialization_byte = 8'h40;
        6'd22: initialization_byte = 8'hA4;
        6'd23: initialization_byte = 8'hA6;
        6'd24: initialization_byte = 8'h2E;
        default: initialization_byte = 8'hAF;
      endcase
    end
  endfunction

  function [7:0] address_byte;
    input [2:0] index;
    begin
      case (index)
        3'd0: address_byte = 8'h21;
        3'd1: address_byte = 8'h00;
        3'd2: address_byte = 8'h7F;
        3'd3: address_byte = 8'h22;
        3'd4: address_byte = 8'h00;
        default: address_byte = 8'h07;
      endcase
    end
  endfunction

  wire simple_second_byte;
  wire [7:0] simple_byte;

  assign operation_ready = (state == STATE_IDLE) && !phy_busy;
  assign busy = (state != STATE_IDLE) || phy_busy;
  assign frame_data_ready =
      (state == STATE_FRAME) && phy_byte_ready;
  assign simple_second_byte = (sequence_index == 6'd1);
  assign simple_byte =
      (saved_operation == OP_DISPLAY) ?
          (saved_value[0] ? 8'hAF : 8'hAE) :
      (saved_operation == OP_INVERT) ?
          (saved_value[0] ? 8'hA7 : 8'hA6) :
      (simple_second_byte ? saved_value : 8'h81);
  assign phy_byte_data =
      (state == STATE_INIT) ? initialization_byte(sequence_index) :
      (state == STATE_ADDRESS) ? address_byte(sequence_index[2:0]) :
      (state == STATE_FRAME) ? frame_data :
      (state == STATE_CLEAR) ? 8'h00 :
      (state == STATE_SIMPLE) ? simple_byte :
                               8'h00;
  assign phy_byte_is_data =
      (state == STATE_FRAME) || (state == STATE_CLEAR);
  assign phy_byte_valid =
      (state == STATE_INIT) ||
      (state == STATE_ADDRESS) ||
      ((state == STATE_FRAME) && frame_data_valid) ||
      (state == STATE_CLEAR) ||
      (state == STATE_SIMPLE);

  always @(posedge clk) begin
    if (!rst_n) begin
      state <= STATE_IDLE;
      reset_count <= 32'd0;
      sequence_index <= 6'd0;
      frame_index <= 10'd0;
      saved_value <= 8'd0;
      saved_operation <= OP_INIT;
      simple_length <= 2'd0;
      operation_done <= 1'b0;
      protocol_error <= 1'b0;
      oled_reset_n <= 1'b1;
    end else begin
      operation_done <= 1'b0;
      protocol_error <= 1'b0;

      case (state)
        STATE_IDLE: begin
          oled_reset_n <= 1'b1;
          if (operation_valid && operation_ready) begin
            saved_operation <= operation;
            saved_value <= operation_value;
            sequence_index <= 6'd0;
            frame_index <= 10'd0;
            if (operation == OP_INIT) begin
              oled_reset_n <= 1'b0;
              reset_count <= 32'd0;
              state <= STATE_RESET_LOW;
            end else if ((operation == OP_FRAME) ||
                         (operation == OP_CLEAR)) begin
              state <= STATE_ADDRESS;
            end else if ((operation == OP_DISPLAY) ||
                         (operation == OP_INVERT)) begin
              simple_length <= 2'd1;
              state <= STATE_SIMPLE;
            end else if (operation == OP_CONTRAST) begin
              simple_length <= 2'd2;
              state <= STATE_SIMPLE;
            end else begin
              protocol_error <= 1'b1;
              operation_done <= 1'b1;
            end
          end
        end

        STATE_RESET_LOW: begin
          oled_reset_n <= 1'b0;
          if (reset_count >= RESET_LOW_CYCLES - 1) begin
            reset_count <= 32'd0;
            oled_reset_n <= 1'b1;
            state <= STATE_RESET_HIGH;
          end else begin
            reset_count <= reset_count + 32'd1;
          end
        end

        STATE_RESET_HIGH: begin
          if (reset_count >= RESET_HIGH_CYCLES - 1) begin
            sequence_index <= 6'd0;
            state <= STATE_INIT;
          end else begin
            reset_count <= reset_count + 32'd1;
          end
        end

        STATE_INIT: begin
          if (phy_byte_valid && phy_byte_ready) begin
            if (sequence_index == 6'd25) begin
              state <= STATE_WAIT_PHY;
            end else begin
              sequence_index <= sequence_index + 6'd1;
            end
          end
        end

        STATE_ADDRESS: begin
          if (phy_byte_valid && phy_byte_ready) begin
            if (sequence_index == 6'd5) begin
              frame_index <= 10'd0;
              if (saved_operation == OP_CLEAR) begin
                state <= STATE_CLEAR;
              end else begin
                state <= STATE_FRAME;
              end
            end else begin
              sequence_index <= sequence_index + 6'd1;
            end
          end
        end

        STATE_FRAME: begin
          if (phy_byte_valid && phy_byte_ready) begin
            if ((frame_index == 10'd1023) != frame_data_last) begin
              protocol_error <= 1'b1;
            end
            if (frame_index == 10'd1023) begin
              state <= STATE_WAIT_PHY;
            end else begin
              frame_index <= frame_index + 10'd1;
            end
          end
        end

        STATE_CLEAR: begin
          if (phy_byte_valid && phy_byte_ready) begin
            if (frame_index == 10'd1023) begin
              state <= STATE_WAIT_PHY;
            end else begin
              frame_index <= frame_index + 10'd1;
            end
          end
        end

        STATE_SIMPLE: begin
          if (phy_byte_valid && phy_byte_ready) begin
            if ((sequence_index + 6'd1) >= simple_length) begin
              state <= STATE_WAIT_PHY;
            end else begin
              sequence_index <= sequence_index + 6'd1;
            end
          end
        end

        STATE_WAIT_PHY: begin
          if (!phy_busy) begin
            operation_done <= 1'b1;
            state <= STATE_IDLE;
          end
        end

        default: begin
          protocol_error <= 1'b1;
          state <= STATE_IDLE;
          oled_reset_n <= 1'b1;
        end
      endcase
    end
  end

endmodule

`default_nettype wire
