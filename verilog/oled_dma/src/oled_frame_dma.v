// SPDX-License-Identifier: MIT
/**
 * @file    oled_frame_dma.v
 * @brief   AXI4 read DMA for one 1024-byte OLED framebuffer.
 * @details
 *          Sixteen 16-beat bursts copy one aligned DDR framebuffer into the
 *          local snapshot RAM. AXI response and RLAST failures are latched.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module oled_frame_dma (
    input  wire        clk,
    input  wire        rst_n,
    input  wire        start,
    input  wire [31:0] framebuffer_base,
    output reg         busy,
    output reg         done,
    output reg         error,
    output reg [3:0]   error_reason,
    output reg         ram_write_enable,
    output reg [7:0]   ram_write_address,
    output reg [31:0]  ram_write_data,
    output wire [31:0] m_axi_araddr,
    output wire [7:0]  m_axi_arlen,
    output wire [2:0]  m_axi_arsize,
    output wire [1:0]  m_axi_arburst,
    output wire        m_axi_arvalid,
    input  wire        m_axi_arready,
    input  wire [31:0] m_axi_rdata,
    input  wire [1:0]  m_axi_rresp,
    input  wire        m_axi_rlast,
    input  wire        m_axi_rvalid,
    output wire        m_axi_rready
);

  localparam [1:0] DMA_IDLE = 2'd0;
  localparam [1:0] DMA_ADDRESS = 2'd1;
  localparam [1:0] DMA_DATA = 2'd2;

  reg [1:0] state;
  reg [3:0] burst_index;
  reg [4:0] beat_index;
  reg [7:0] word_index;
  reg response_failed;

  assign m_axi_araddr = framebuffer_base + {22'd0, burst_index, 6'd0};
  assign m_axi_arlen = 8'd15;
  assign m_axi_arsize = 3'd2;
  assign m_axi_arburst = 2'b01;
  assign m_axi_arvalid = (state == DMA_ADDRESS);
  assign m_axi_rready = (state == DMA_DATA);

  always @(posedge clk) begin
    if (!rst_n) begin
      state <= DMA_IDLE;
      burst_index <= 4'd0;
      beat_index <= 5'd0;
      word_index <= 8'd0;
      response_failed <= 1'b0;
      busy <= 1'b0;
      done <= 1'b0;
      error <= 1'b0;
      error_reason <= 4'd0;
      ram_write_enable <= 1'b0;
      ram_write_address <= 8'd0;
      ram_write_data <= 32'd0;
    end else begin
      done <= 1'b0;
      ram_write_enable <= 1'b0;

      case (state)
        DMA_IDLE: begin
          busy <= 1'b0;
          if (start) begin
            burst_index <= 4'd0;
            beat_index <= 5'd0;
            word_index <= 8'd0;
            response_failed <= 1'b0;
            error <= 1'b0;
            error_reason <= 4'd0;
            busy <= 1'b1;
            state <= DMA_ADDRESS;
          end
        end

        DMA_ADDRESS: begin
          if (m_axi_arready) begin
            beat_index <= 5'd0;
            state <= DMA_DATA;
          end
        end

        DMA_DATA: begin
          if (m_axi_rvalid) begin
            if (m_axi_rresp == 2'b00) begin
              ram_write_enable <= 1'b1;
              ram_write_address <= word_index;
              ram_write_data <= m_axi_rdata;
            end else begin
              response_failed <= 1'b1;
              error <= 1'b1;
              if (m_axi_rresp == 2'b10) begin
                error_reason[1] <= 1'b1;
              end else begin
                error_reason[2] <= 1'b1;
              end
            end

            word_index <= word_index + 8'd1;
            if (m_axi_rlast) begin
              if (beat_index != 5'd15) begin
                response_failed <= 1'b1;
                error <= 1'b1;
                error_reason[3] <= 1'b1;
              end

              if (response_failed ||
                  (m_axi_rresp != 2'b00) ||
                  (beat_index != 5'd15)) begin
                busy <= 1'b0;
                done <= 1'b1;
                state <= DMA_IDLE;
              end else if (burst_index == 4'd15) begin
                busy <= 1'b0;
                done <= 1'b1;
                state <= DMA_IDLE;
              end else begin
                burst_index <= burst_index + 4'd1;
                state <= DMA_ADDRESS;
              end
            end else if (beat_index == 5'd15) begin
              response_failed <= 1'b1;
              error <= 1'b1;
              error_reason[3] <= 1'b1;
              beat_index <= beat_index + 5'd1;
            end else begin
              beat_index <= beat_index + 5'd1;
            end
          end
        end

        default: begin
          state <= DMA_IDLE;
          busy <= 1'b0;
          error <= 1'b1;
          error_reason[0] <= 1'b1;
        end
      endcase
    end
  end

endmodule

`default_nettype wire
