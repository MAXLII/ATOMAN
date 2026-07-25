// SPDX-License-Identifier: MIT
/**
 * @file    oled_frame_ram.v
 * @brief   One-frame OLED DMA snapshot memory.
 * @details
 *          The DMA side stores 256 32-bit words while the protocol side reads
 *          the same snapshot as 1024 sequential bytes.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module oled_frame_ram (
    input  wire        clk,
    input  wire        write_enable,
    input  wire [7:0]  write_address,
    input  wire [31:0] write_data,
    input  wire [9:0]  read_address,
    output wire [7:0]  read_data
);

  reg [31:0] memory [0:255];
  wire [31:0] read_word;

  always @(posedge clk) begin
    if (write_enable) begin
      memory[write_address] <= write_data;
    end
  end

  assign read_word = memory[read_address[9:2]];
  assign read_data =
      (read_address[1:0] == 2'd0) ? read_word[7:0] :
      (read_address[1:0] == 2'd1) ? read_word[15:8] :
      (read_address[1:0] == 2'd2) ? read_word[23:16] :
                                   read_word[31:24];

endmodule

`default_nettype wire
