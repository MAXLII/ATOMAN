// SPDX-License-Identifier: MIT
/**
 * @file    axi_oled_dma.v
 * @brief   AXI controlled OLED framebuffer DMA peripheral.
 * @details
 *          The top-level data-control plane owns registers, DDR DMA, snapshot
 *          storage, scheduling, and error reporting. Controller commands and
 *          electrical serial timing remain isolated in dedicated modules.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module axi_oled_dma #(
    parameter integer C_S_AXI_ADDR_WIDTH = 8,
    parameter integer RESET_LOW_CYCLES = 500000,
    parameter integer RESET_HIGH_CYCLES = 500000
)(
    input  wire                          aclk,
    input  wire                          aresetn,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] s_axi_awaddr,
    input  wire [2:0]                    s_axi_awprot,
    input  wire                          s_axi_awvalid,
    output wire                          s_axi_awready,
    input  wire [31:0]                   s_axi_wdata,
    input  wire [3:0]                    s_axi_wstrb,
    input  wire                          s_axi_wvalid,
    output wire                          s_axi_wready,
    output wire [1:0]                    s_axi_bresp,
    output wire                          s_axi_bvalid,
    input  wire                          s_axi_bready,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0] s_axi_araddr,
    input  wire [2:0]                    s_axi_arprot,
    input  wire                          s_axi_arvalid,
    output wire                          s_axi_arready,
    output wire [31:0]                   s_axi_rdata,
    output wire [1:0]                    s_axi_rresp,
    output wire                          s_axi_rvalid,
    input  wire                          s_axi_rready,
    output wire [31:0]                   m_axi_awaddr,
    output wire [7:0]                    m_axi_awlen,
    output wire [2:0]                    m_axi_awsize,
    output wire [1:0]                    m_axi_awburst,
    output wire                          m_axi_awvalid,
    input  wire                          m_axi_awready,
    output wire [31:0]                   m_axi_wdata,
    output wire [3:0]                    m_axi_wstrb,
    output wire                          m_axi_wlast,
    output wire                          m_axi_wvalid,
    input  wire                          m_axi_wready,
    input  wire [1:0]                    m_axi_bresp,
    input  wire                          m_axi_bvalid,
    output wire                          m_axi_bready,
    output wire [31:0]                   m_axi_araddr,
    output wire [7:0]                    m_axi_arlen,
    output wire [2:0]                    m_axi_arsize,
    output wire [1:0]                    m_axi_arburst,
    output wire                          m_axi_arvalid,
    input  wire                          m_axi_arready,
    input  wire [31:0]                   m_axi_rdata,
    input  wire [1:0]                    m_axi_rresp,
    input  wire                          m_axi_rlast,
    input  wire                          m_axi_rvalid,
    output wire                          m_axi_rready,
    output wire                          oled_clock,
    output wire                          oled_data,
    output wire                          oled_dc,
    output wire                          oled_reset_n,
    output wire                          irq
);

  localparam [31:0] RTL_VERSION = 32'h0001_0000;
  localparam [31:0] DEFAULT_FRAMEBUFFER_BASE = 32'h1FF2_0000;
  localparam [15:0] DEFAULT_SPI_DIVIDER = 16'd5;
  localparam [31:0] DEFAULT_REFRESH_PERIOD = 32'd50000000;

  localparam [2:0] OP_INIT = 3'd0;
  localparam [2:0] OP_FRAME = 3'd1;
  localparam [2:0] OP_CLEAR = 3'd2;
  localparam [2:0] OP_DISPLAY = 3'd3;
  localparam [2:0] OP_INVERT = 3'd4;
  localparam [2:0] OP_CONTRAST = 3'd5;

  localparam integer IRQ_CONFIG = 0;
  localparam integer IRQ_AXI = 1;
  localparam integer IRQ_COMMAND = 2;
  localparam integer IRQ_PROTOCOL = 3;

  reg [C_S_AXI_ADDR_WIDTH-1:0] lite_awaddr;
  reg [31:0] lite_wdata;
  reg [3:0] lite_wstrb;
  reg lite_aw_pending;
  reg lite_w_pending;
  reg lite_bvalid;
  reg [31:0] lite_rdata;
  reg lite_rvalid;

  reg peripheral_enable;
  reg display_on;
  reg inverted;
  reg auto_refresh_enable;
  reg soft_reset_pulse;
  reg [31:0] framebuffer_base;
  reg [15:0] spi_divider;
  reg [31:0] refresh_period;
  reg [7:0] contrast;

  reg [31:0] irq_status;
  reg [31:0] irq_enable;
  reg [31:0] frame_count;
  reg [31:0] clear_count;
  reg [31:0] axi_error_count;
  reg [31:0] command_error_count;
  reg [31:0] dma_stop_reason;
  reg initialized;
  reg dma_halted;

  reg init_request;
  reg present_request;
  reg clear_request;
  reg display_request;
  reg invert_request;
  reg contrast_request;
  reg auto_refresh_pending;
  reg [31:0] refresh_count;

  reg dma_start;
  wire dma_busy;
  wire dma_done;
  wire dma_error;
  wire [3:0] dma_error_reason;
  wire ram_write_enable;
  wire [7:0] ram_write_address;
  wire [31:0] ram_write_data;

  reg [2:0] protocol_operation;
  reg [7:0] protocol_value;
  reg protocol_operation_valid;
  wire protocol_operation_ready;
  wire protocol_operation_done;
  wire protocol_error;
  wire protocol_busy;
  reg [2:0] active_operation;

  reg [9:0] frame_read_address;
  wire [7:0] frame_read_data;
  wire frame_data_ready;
  wire frame_data_valid;
  wire frame_data_last;

  wire [7:0] phy_byte_data;
  wire phy_byte_is_data;
  wire phy_byte_valid;
  wire phy_byte_ready;
  wire phy_byte_done;
  wire phy_busy;
  wire core_rst_n;

  wire lite_write_commit;
  wire [5:0] lite_write_index;
  wire [31:0] control_value;
  wire [31:0] control_write_value;
  wire [31:0] framebuffer_write_value;
  wire [31:0] spi_divider_write_value;
  wire [31:0] refresh_period_write_value;
  wire [31:0] contrast_write_value;
  wire [31:0] status_value;
  wire controller_busy;
  wire framebuffer_valid;

  function [31:0] apply_wstrb;
    input [31:0] current_value;
    input [31:0] write_value;
    input [3:0] byte_strobe;
    integer byte_index;
    begin
      apply_wstrb = current_value;
      for (byte_index = 0; byte_index < 4; byte_index = byte_index + 1) begin
        if (byte_strobe[byte_index]) begin
          apply_wstrb[(byte_index*8) +: 8] =
              write_value[(byte_index*8) +: 8];
        end
      end
    end
  endfunction

  function [31:0] register_read;
    input [5:0] register_index;
    begin
      case (register_index)
        6'h00: register_read = control_value;
        6'h01: register_read = status_value;
        6'h02: register_read = framebuffer_base;
        6'h03: register_read = {16'd0, spi_divider};
        6'h04: register_read = refresh_period;
        6'h05: register_read = {24'd0, contrast};
        6'h06: register_read = irq_status;
        6'h07: register_read = irq_enable;
        6'h08: register_read = frame_count;
        6'h09: register_read = clear_count;
        6'h0A: register_read = axi_error_count;
        6'h0B: register_read = command_error_count;
        6'h0C: register_read = dma_stop_reason;
        6'h0D: register_read = RTL_VERSION;
        6'h0E: register_read = 32'h0040_0080;
        6'h0F: register_read = 32'd1024;
        default: register_read = 32'd0;
      endcase
    end
  endfunction

  assign core_rst_n = aresetn && !soft_reset_pulse;
  assign lite_write_commit =
      lite_aw_pending && lite_w_pending && !lite_bvalid;
  assign lite_write_index = lite_awaddr[7:2];
  assign control_value = {
      24'd0,
      auto_refresh_enable,
      inverted,
      display_on,
      4'd0,
      peripheral_enable
  };
  assign control_write_value =
      apply_wstrb(control_value, lite_wdata, lite_wstrb);
  assign framebuffer_write_value =
      apply_wstrb(framebuffer_base, lite_wdata, lite_wstrb);
  assign spi_divider_write_value =
      apply_wstrb({16'd0, spi_divider}, lite_wdata, lite_wstrb);
  assign refresh_period_write_value =
      apply_wstrb(refresh_period, lite_wdata, lite_wstrb);
  assign contrast_write_value =
      apply_wstrb({24'd0, contrast}, lite_wdata, lite_wstrb);
  assign framebuffer_valid = (framebuffer_base[9:0] == 10'd0);
  assign controller_busy =
      dma_busy || protocol_busy || protocol_operation_valid;
  assign status_value = {
      20'd0,
      (irq_status != 32'd0),
      !framebuffer_valid,
      dma_halted,
      auto_refresh_enable,
      inverted,
      display_on,
      protocol_operation_valid,
      protocol_busy,
      dma_busy,
      initialized,
      peripheral_enable
  };
  assign irq = |(irq_status & irq_enable);

  assign s_axi_awready = !lite_aw_pending && !lite_bvalid;
  assign s_axi_wready = !lite_w_pending && !lite_bvalid;
  assign s_axi_bresp = 2'b00;
  assign s_axi_bvalid = lite_bvalid;
  assign s_axi_arready = !lite_rvalid;
  assign s_axi_rdata = lite_rdata;
  assign s_axi_rresp = 2'b00;
  assign s_axi_rvalid = lite_rvalid;

  assign m_axi_awaddr = 32'd0;
  assign m_axi_awlen = 8'd0;
  assign m_axi_awsize = 3'd2;
  assign m_axi_awburst = 2'b01;
  assign m_axi_awvalid = 1'b0;
  assign m_axi_wdata = 32'd0;
  assign m_axi_wstrb = 4'd0;
  assign m_axi_wlast = 1'b1;
  assign m_axi_wvalid = 1'b0;
  assign m_axi_bready = 1'b0;

  assign frame_data_valid = (active_operation == OP_FRAME);
  assign frame_data_last = (frame_read_address == 10'd1023);

  oled_frame_dma frame_dma (
      .clk(aclk),
      .rst_n(core_rst_n),
      .start(dma_start),
      .framebuffer_base(framebuffer_base),
      .busy(dma_busy),
      .done(dma_done),
      .error(dma_error),
      .error_reason(dma_error_reason),
      .ram_write_enable(ram_write_enable),
      .ram_write_address(ram_write_address),
      .ram_write_data(ram_write_data),
      .m_axi_araddr(m_axi_araddr),
      .m_axi_arlen(m_axi_arlen),
      .m_axi_arsize(m_axi_arsize),
      .m_axi_arburst(m_axi_arburst),
      .m_axi_arvalid(m_axi_arvalid),
      .m_axi_arready(m_axi_arready),
      .m_axi_rdata(m_axi_rdata),
      .m_axi_rresp(m_axi_rresp),
      .m_axi_rlast(m_axi_rlast),
      .m_axi_rvalid(m_axi_rvalid),
      .m_axi_rready(m_axi_rready)
  );

  oled_frame_ram frame_ram (
      .clk(aclk),
      .write_enable(ram_write_enable),
      .write_address(ram_write_address),
      .write_data(ram_write_data),
      .read_address(frame_read_address),
      .read_data(frame_read_data)
  );

  ssd1306_protocol #(
      .RESET_LOW_CYCLES(RESET_LOW_CYCLES),
      .RESET_HIGH_CYCLES(RESET_HIGH_CYCLES)
  ) controller_protocol (
      .clk(aclk),
      .rst_n(core_rst_n),
      .operation(protocol_operation),
      .operation_value(protocol_value),
      .operation_valid(protocol_operation_valid),
      .operation_ready(protocol_operation_ready),
      .operation_done(protocol_operation_done),
      .protocol_error(protocol_error),
      .frame_data(frame_read_data),
      .frame_data_valid(frame_data_valid),
      .frame_data_last(frame_data_last),
      .frame_data_ready(frame_data_ready),
      .phy_byte_data(phy_byte_data),
      .phy_byte_is_data(phy_byte_is_data),
      .phy_byte_valid(phy_byte_valid),
      .phy_byte_ready(phy_byte_ready),
      .phy_busy(phy_busy),
      .oled_reset_n(oled_reset_n),
      .busy(protocol_busy)
  );

  oled_serial_phy serial_phy (
      .clk(aclk),
      .rst_n(core_rst_n),
      .clock_divider(spi_divider),
      .byte_data(phy_byte_data),
      .byte_is_data(phy_byte_is_data),
      .byte_valid(phy_byte_valid),
      .byte_ready(phy_byte_ready),
      .byte_done(phy_byte_done),
      .busy(phy_busy),
      .oled_clock(oled_clock),
      .oled_data(oled_data),
      .oled_dc(oled_dc)
  );

  always @(posedge aclk) begin
    if (!aresetn) begin
      lite_awaddr <= {C_S_AXI_ADDR_WIDTH{1'b0}};
      lite_wdata <= 32'd0;
      lite_wstrb <= 4'd0;
      lite_aw_pending <= 1'b0;
      lite_w_pending <= 1'b0;
      lite_bvalid <= 1'b0;
      lite_rdata <= 32'd0;
      lite_rvalid <= 1'b0;
    end else begin
      if (s_axi_awready && s_axi_awvalid) begin
        lite_awaddr <= s_axi_awaddr;
        lite_aw_pending <= 1'b1;
      end
      if (s_axi_wready && s_axi_wvalid) begin
        lite_wdata <= s_axi_wdata;
        lite_wstrb <= s_axi_wstrb;
        lite_w_pending <= 1'b1;
      end
      if (lite_bvalid && s_axi_bready) begin
        lite_bvalid <= 1'b0;
      end
      if (lite_write_commit) begin
        lite_aw_pending <= 1'b0;
        lite_w_pending <= 1'b0;
        lite_bvalid <= 1'b1;
      end

      if (lite_rvalid && s_axi_rready) begin
        lite_rvalid <= 1'b0;
      end
      if (s_axi_arready && s_axi_arvalid) begin
        lite_rdata <= register_read(s_axi_araddr[7:2]);
        lite_rvalid <= 1'b1;
      end
    end
  end

  always @(posedge aclk) begin
    if (!aresetn) begin
      peripheral_enable <= 1'b0;
      display_on <= 1'b1;
      inverted <= 1'b0;
      auto_refresh_enable <= 1'b0;
      soft_reset_pulse <= 1'b0;
      framebuffer_base <= DEFAULT_FRAMEBUFFER_BASE;
      spi_divider <= DEFAULT_SPI_DIVIDER;
      refresh_period <= DEFAULT_REFRESH_PERIOD;
      contrast <= 8'hCF;
      irq_status <= 32'd0;
      irq_enable <= 32'h0000_000F;
      frame_count <= 32'd0;
      clear_count <= 32'd0;
      axi_error_count <= 32'd0;
      command_error_count <= 32'd0;
      dma_stop_reason <= 32'd0;
      initialized <= 1'b0;
      dma_halted <= 1'b0;
      init_request <= 1'b0;
      present_request <= 1'b0;
      clear_request <= 1'b0;
      display_request <= 1'b0;
      invert_request <= 1'b0;
      contrast_request <= 1'b0;
      auto_refresh_pending <= 1'b0;
      refresh_count <= 32'd0;
      dma_start <= 1'b0;
      protocol_operation <= OP_INIT;
      protocol_value <= 8'd0;
      protocol_operation_valid <= 1'b0;
      active_operation <= OP_INIT;
      frame_read_address <= 10'd0;
    end else begin
      soft_reset_pulse <= 1'b0;
      dma_start <= 1'b0;

      if (protocol_operation_valid && protocol_operation_ready) begin
        protocol_operation_valid <= 1'b0;
        active_operation <= protocol_operation;
      end

      if ((active_operation == OP_FRAME) && frame_data_ready) begin
        frame_read_address <= frame_read_address + 10'd1;
      end

      if (protocol_operation_done) begin
        if (active_operation == OP_INIT) begin
          initialized <= 1'b1;
        end else if (active_operation == OP_FRAME) begin
          frame_count <= frame_count + 32'd1;
        end else if (active_operation == OP_CLEAR) begin
          clear_count <= clear_count + 32'd1;
        end
      end

      if (protocol_error) begin
        irq_status[IRQ_PROTOCOL] <= 1'b1;
        command_error_count <= command_error_count + 32'd1;
      end

      if (dma_done) begin
        if (dma_error) begin
          dma_halted <= 1'b1;
          dma_stop_reason <= {28'd0, dma_error_reason};
          axi_error_count <= axi_error_count + 32'd1;
          irq_status[IRQ_AXI] <= 1'b1;
        end else begin
          protocol_operation <= OP_FRAME;
          protocol_value <= 8'd0;
          protocol_operation_valid <= 1'b1;
          frame_read_address <= 10'd0;
        end
      end

      if (auto_refresh_enable && peripheral_enable) begin
        if (refresh_count >= (refresh_period - 32'd1)) begin
          refresh_count <= 32'd0;
          if (controller_busy || present_request || clear_request) begin
            auto_refresh_pending <= 1'b1;
          end else begin
            present_request <= 1'b1;
          end
        end else begin
          refresh_count <= refresh_count + 32'd1;
        end
      end else begin
        refresh_count <= 32'd0;
        auto_refresh_pending <= 1'b0;
      end

      if (!controller_busy && peripheral_enable) begin
        if (init_request) begin
          init_request <= 1'b0;
          initialized <= 1'b0;
          protocol_operation <= OP_INIT;
          protocol_value <= contrast;
          protocol_operation_valid <= 1'b1;
        end else if (clear_request) begin
          clear_request <= 1'b0;
          protocol_operation <= OP_CLEAR;
          protocol_value <= 8'd0;
          protocol_operation_valid <= 1'b1;
        end else if (display_request) begin
          display_request <= 1'b0;
          protocol_operation <= OP_DISPLAY;
          protocol_value <= {7'd0, display_on};
          protocol_operation_valid <= 1'b1;
        end else if (invert_request) begin
          invert_request <= 1'b0;
          protocol_operation <= OP_INVERT;
          protocol_value <= {7'd0, inverted};
          protocol_operation_valid <= 1'b1;
        end else if (contrast_request) begin
          contrast_request <= 1'b0;
          protocol_operation <= OP_CONTRAST;
          protocol_value <= contrast;
          protocol_operation_valid <= 1'b1;
        end else if (present_request || auto_refresh_pending) begin
          present_request <= 1'b0;
          auto_refresh_pending <= 1'b0;
          if (!framebuffer_valid || dma_halted) begin
            irq_status[IRQ_CONFIG] <= 1'b1;
            command_error_count <= command_error_count + 32'd1;
          end else begin
            dma_start <= 1'b1;
          end
        end
      end

      if (lite_write_commit) begin
        case (lite_write_index)
          6'h00: begin
            if (control_write_value[1]) begin
              soft_reset_pulse <= 1'b1;
              peripheral_enable <= 1'b0;
              display_on <= 1'b1;
              inverted <= 1'b0;
              auto_refresh_enable <= 1'b0;
              initialized <= 1'b0;
              dma_halted <= 1'b0;
              dma_stop_reason <= 32'd0;
              irq_status <= 32'd0;
              init_request <= 1'b0;
              present_request <= 1'b0;
              clear_request <= 1'b0;
              display_request <= 1'b0;
              invert_request <= 1'b0;
              contrast_request <= 1'b0;
              protocol_operation_valid <= 1'b0;
            end else begin
              if (!peripheral_enable && control_write_value[0]) begin
                init_request <= 1'b1;
              end
              if (display_on != control_write_value[5]) begin
                display_request <= 1'b1;
              end
              if (inverted != control_write_value[6]) begin
                invert_request <= 1'b1;
              end
              peripheral_enable <= control_write_value[0];
              display_on <= control_write_value[5];
              inverted <= control_write_value[6];
              auto_refresh_enable <= control_write_value[7];

              if (control_write_value[2]) begin
                if (controller_busy) begin
                  irq_status[IRQ_COMMAND] <= 1'b1;
                  command_error_count <= command_error_count + 32'd1;
                end else begin
                  init_request <= 1'b1;
                end
              end
              if (control_write_value[3]) begin
                if (controller_busy || present_request) begin
                  irq_status[IRQ_COMMAND] <= 1'b1;
                  command_error_count <= command_error_count + 32'd1;
                end else begin
                  present_request <= 1'b1;
                end
              end
              if (control_write_value[4]) begin
                if (controller_busy || clear_request) begin
                  irq_status[IRQ_COMMAND] <= 1'b1;
                  command_error_count <= command_error_count + 32'd1;
                end else begin
                  clear_request <= 1'b1;
                end
              end
            end
          end

          6'h02: begin
            framebuffer_base <= framebuffer_write_value;
            if (framebuffer_write_value[9:0] != 10'd0) begin
              irq_status[IRQ_CONFIG] <= 1'b1;
              command_error_count <= command_error_count + 32'd1;
            end
          end

          6'h03: begin
            if ((spi_divider_write_value[15:0] >= 16'd3) &&
                (spi_divider_write_value[15:0] <= 16'd25000)) begin
              spi_divider <= spi_divider_write_value[15:0];
            end else begin
              irq_status[IRQ_CONFIG] <= 1'b1;
              command_error_count <= command_error_count + 32'd1;
            end
          end

          6'h04: begin
            if (refresh_period_write_value != 32'd0) begin
              refresh_period <= refresh_period_write_value;
            end else begin
              irq_status[IRQ_CONFIG] <= 1'b1;
              command_error_count <= command_error_count + 32'd1;
            end
          end

          6'h05: begin
            contrast <= contrast_write_value[7:0];
            contrast_request <= 1'b1;
          end

          6'h06: irq_status <= irq_status & ~lite_wdata;
          6'h07: irq_enable <=
              apply_wstrb(irq_enable, lite_wdata, lite_wstrb);
          default: begin
          end
        endcase
      end
    end
  end

  wire unused_axi_write;
  wire unused_axi_protection;
  assign unused_axi_write =
      m_axi_awready || m_axi_wready || m_axi_bvalid ||
      (|m_axi_bresp);
  assign unused_axi_protection = (|s_axi_awprot) || (|s_axi_arprot);

endmodule

`default_nettype wire
