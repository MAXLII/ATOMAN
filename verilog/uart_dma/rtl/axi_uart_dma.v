// SPDX-License-Identifier: MIT
/**
 * @file    axi_uart_dma.v
 * @brief   AXI controlled UART with autonomous DDR ring DMA.
 * @details
 *          This file is part of the base project.
 *
 *          The peripheral exposes configuration and monotonic ring counters
 *          through AXI4-Lite. Independent AXI read and write masters move UART
 *          bytes between FIFOs and DDR without normal-path interrupts. Only
 *          UART, ring overflow, invalid configuration, and AXI failures are
 *          interrupt sources.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module axi_uart_dma #(
    parameter integer C_S_AXI_ADDR_WIDTH = 8,
    parameter integer UART_OVERSAMPLE = 16
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

    input  wire                          uart_rxd,
    output wire                          uart_txd,
    output wire                          irq
);

    localparam [31:0] RTL_VERSION = 32'h0001_0000;
    localparam [31:0] DEFAULT_BAUD_INCREMENT = 32'd1266637395;
    localparam [31:0] DEFAULT_IRQ_ENABLE = 32'h0000_003F;

    localparam integer IRQ_PARITY  = 0;
    localparam integer IRQ_FRAME   = 1;
    localparam integer IRQ_OVERFLOW = 2;
    localparam integer IRQ_AXI_READ = 3;
    localparam integer IRQ_AXI_WRITE = 4;
    localparam integer IRQ_CONFIG = 5;

    reg [C_S_AXI_ADDR_WIDTH-1:0] lite_awaddr;
    reg [31:0] lite_wdata;
    reg [3:0] lite_wstrb;
    reg lite_aw_pending;
    reg lite_w_pending;
    reg lite_bvalid;
    reg [31:0] lite_rdata;
    reg lite_rvalid;

    reg uart_enable;
    reg internal_loopback;
    reg soft_reset_pulse;
    reg [31:0] uart_config;
    reg [31:0] baud_increment;
    reg [31:0] rx_ring_base;
    reg [31:0] rx_ring_size;
    reg [31:0] rx_produced;
    reg [31:0] rx_consumed;
    reg [31:0] tx_ring_base;
    reg [31:0] tx_ring_size;
    reg [31:0] tx_produced;
    reg [31:0] tx_consumed;
    reg [31:0] irq_status;
    reg [31:0] irq_enable;
    reg [15:0] parity_error_count;
    reg [15:0] frame_error_count;
    reg [15:0] overflow_error_count;
    reg [7:0] axi_read_error_count;
    reg [7:0] axi_write_error_count;
    reg [31:0] dma_stop_reason;
    reg rx_dma_halted;
    reg tx_dma_halted;

    wire [7:0] serial_rx_data;
    wire serial_rx_valid;
    wire serial_parity_error;
    wire serial_frame_error;
    wire serial_break_error;
    wire serial_tx_ready;
    wire serial_tx_busy;

    wire [7:0] rx_fifo_data;
    wire rx_fifo_empty;
    wire rx_fifo_full;
    wire [4:0] rx_fifo_count;
    reg rx_fifo_pop;
    reg rx_fifo_pop_delay;

    wire [7:0] tx_fifo_data;
    wire tx_fifo_empty;
    wire tx_fifo_full;
    wire [4:0] tx_fifo_count;
    reg tx_fifo_push;
    reg [7:0] tx_fifo_push_data;
    wire tx_fifo_pop;

    reg rx_write_active;
    reg rx_awvalid;
    reg rx_wvalid;
    reg rx_bready;
    reg [31:0] rx_write_address;
    reg [31:0] rx_write_data;
    reg [3:0] rx_write_strobe;

    reg tx_read_active;
    reg tx_arvalid;
    reg tx_rready;
    reg [31:0] tx_read_address;
    reg [1:0] tx_read_lane;

    wire lite_write_commit;
    wire [5:0] lite_write_index;
    wire [31:0] status_value;
    wire [31:0] uart_error_count_value;
    wire [31:0] dma_error_count_value;
    wire rx_ring_full;
    wire tx_ring_empty;
    wire ring_configuration_valid;
    wire uart_clear;

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

    function ring_size_valid;
        input [31:0] size_value;
        begin
            ring_size_valid =
                (size_value >= 32'd256) &&
                (size_value <= 32'd65536) &&
                ((size_value & (size_value - 32'd1)) == 32'd0);
        end
    endfunction

    function [31:0] register_read;
        input [5:0] register_index;
        begin
            case (register_index)
                6'h00: register_read = {29'd0, internal_loopback, 1'b0, uart_enable};
                6'h01: register_read = status_value;
                6'h02: register_read = uart_config;
                6'h03: register_read = baud_increment;
                6'h04: register_read = rx_ring_base;
                6'h05: register_read = rx_ring_size;
                6'h06: register_read = rx_produced;
                6'h07: register_read = rx_consumed;
                6'h08: register_read = tx_ring_base;
                6'h09: register_read = tx_ring_size;
                6'h0A: register_read = tx_produced;
                6'h0B: register_read = tx_consumed;
                6'h0C: register_read = irq_status;
                6'h0D: register_read = irq_enable;
                6'h0E: register_read = uart_error_count_value;
                6'h0F: register_read = dma_error_count_value;
                6'h10: register_read = dma_stop_reason;
                6'h11: register_read = RTL_VERSION;
                default: register_read = 32'h0000_0000;
            endcase
        end
    endfunction

    assign lite_write_commit = lite_aw_pending && lite_w_pending && !lite_bvalid;
    assign lite_write_index = lite_awaddr[7:2];
    assign s_axi_awready = !lite_aw_pending && !lite_bvalid;
    assign s_axi_wready = !lite_w_pending && !lite_bvalid;
    assign s_axi_bresp = 2'b00;
    assign s_axi_bvalid = lite_bvalid;
    assign s_axi_arready = !lite_rvalid;
    assign s_axi_rdata = lite_rdata;
    assign s_axi_rresp = 2'b00;
    assign s_axi_rvalid = lite_rvalid;

    assign rx_ring_full = ((rx_produced - rx_consumed) >= rx_ring_size);
    assign tx_ring_empty = (tx_produced == tx_consumed);
    assign ring_configuration_valid =
        ring_size_valid(rx_ring_size) &&
        ring_size_valid(tx_ring_size) &&
        (rx_ring_base[1:0] == 2'b00) &&
        (tx_ring_base[1:0] == 2'b00);
    assign status_value = {
        20'd0,
        tx_dma_halted,
        rx_dma_halted,
        |(irq_status[5:0]),
        tx_fifo_full,
        tx_fifo_empty,
        rx_fifo_full,
        rx_fifo_empty,
        tx_ring_empty,
        rx_ring_full,
        serial_tx_busy,
        uart_enable
    };
    assign uart_error_count_value = {frame_error_count, parity_error_count};
    assign dma_error_count_value =
        {axi_write_error_count, axi_read_error_count, overflow_error_count};
    assign irq = |(irq_status & irq_enable);
    assign uart_clear = soft_reset_pulse;

    assign tx_fifo_pop = serial_tx_ready && !tx_fifo_empty;

    assign m_axi_awaddr = rx_write_address;
    assign m_axi_awlen = 8'd0;
    assign m_axi_awsize = 3'b010;
    assign m_axi_awburst = 2'b01;
    assign m_axi_awvalid = rx_awvalid;
    assign m_axi_wdata = rx_write_data;
    assign m_axi_wstrb = rx_write_strobe;
    assign m_axi_wlast = 1'b1;
    assign m_axi_wvalid = rx_wvalid;
    assign m_axi_bready = rx_bready;

    assign m_axi_araddr = tx_read_address;
    assign m_axi_arlen = 8'd0;
    assign m_axi_arsize = 3'b010;
    assign m_axi_arburst = 2'b01;
    assign m_axi_arvalid = tx_arvalid;
    assign m_axi_rready = tx_rready;

    uart_sync_fifo rx_fifo (
        .clk(aclk),
        .resetn(aresetn),
        .clear(uart_clear),
        .push(serial_rx_valid),
        .push_data(serial_rx_data),
        .pop(rx_fifo_pop),
        .pop_data(rx_fifo_data),
        .empty(rx_fifo_empty),
        .full(rx_fifo_full),
        .count(rx_fifo_count)
    );

    uart_sync_fifo tx_fifo (
        .clk(aclk),
        .resetn(aresetn),
        .clear(uart_clear),
        .push(tx_fifo_push),
        .push_data(tx_fifo_push_data),
        .pop(tx_fifo_pop),
        .pop_data(tx_fifo_data),
        .empty(tx_fifo_empty),
        .full(tx_fifo_full),
        .count(tx_fifo_count)
    );

    uart_serial_core #(
        .OVERSAMPLE(UART_OVERSAMPLE)
    ) serial_core (
        .clk(aclk),
        .resetn(aresetn),
        .clear(uart_clear),
        .enable(uart_enable),
        .baud_increment(baud_increment),
        .data_bits(uart_config[3:0]),
        .parity_mode(uart_config[5:4]),
        .two_stop_bits(uart_config[6]),
        .internal_loopback(internal_loopback),
        .uart_rxd(uart_rxd),
        .uart_txd(uart_txd),
        .tx_data(tx_fifo_data),
        .tx_valid(!tx_fifo_empty),
        .tx_ready(serial_tx_ready),
        .tx_busy(serial_tx_busy),
        .rx_data(serial_rx_data),
        .rx_valid(serial_rx_valid),
        .parity_error_pulse(serial_parity_error),
        .frame_error_pulse(serial_frame_error),
        .break_error_pulse(serial_break_error)
    );

    always @(posedge aclk) begin
        if (!aresetn) begin
            lite_awaddr <= {C_S_AXI_ADDR_WIDTH{1'b0}};
            lite_wdata <= 32'h0000_0000;
            lite_wstrb <= 4'h0;
            lite_aw_pending <= 1'b0;
            lite_w_pending <= 1'b0;
            lite_bvalid <= 1'b0;
            uart_enable <= 1'b0;
            internal_loopback <= 1'b0;
            soft_reset_pulse <= 1'b0;
            uart_config <= 32'h0000_0008;
            baud_increment <= DEFAULT_BAUD_INCREMENT;
            rx_ring_base <= 32'h0000_1000;
            rx_ring_size <= 32'd256;
            rx_consumed <= 32'd0;
            tx_ring_base <= 32'h0000_2000;
            tx_ring_size <= 32'd256;
            tx_produced <= 32'd0;
            irq_enable <= DEFAULT_IRQ_ENABLE;
        end else begin
            soft_reset_pulse <= 1'b0;
            if (soft_reset_pulse) begin
                rx_consumed <= 32'd0;
                tx_produced <= 32'd0;
            end
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
                case (lite_write_index)
                    6'h00: begin
                        if (lite_wdata[1]) begin
                            soft_reset_pulse <= 1'b1;
                            uart_enable <= 1'b0;
                        end else begin
                            internal_loopback <= lite_wdata[2];
                            if (lite_wdata[0] && ring_configuration_valid &&
                                (uart_config[3:0] >= 4'd5) &&
                                (uart_config[3:0] <= 4'd8) &&
                                (uart_config[5:4] != 2'b11) &&
                                (baud_increment != 32'd0)) begin
                                uart_enable <= 1'b1;
                            end else if (!lite_wdata[0]) begin
                                uart_enable <= 1'b0;
                            end
                        end
                    end
                    6'h02: if (!uart_enable) uart_config <=
                        apply_wstrb(uart_config, lite_wdata, lite_wstrb);
                    6'h03: if (!uart_enable) baud_increment <=
                        apply_wstrb(baud_increment, lite_wdata, lite_wstrb);
                    6'h04: if (!uart_enable) rx_ring_base <=
                        apply_wstrb(rx_ring_base, lite_wdata, lite_wstrb);
                    6'h05: if (!uart_enable) rx_ring_size <=
                        apply_wstrb(rx_ring_size, lite_wdata, lite_wstrb);
                    6'h07: rx_consumed <=
                        apply_wstrb(rx_consumed, lite_wdata, lite_wstrb);
                    6'h08: if (!uart_enable) tx_ring_base <=
                        apply_wstrb(tx_ring_base, lite_wdata, lite_wstrb);
                    6'h09: if (!uart_enable) tx_ring_size <=
                        apply_wstrb(tx_ring_size, lite_wdata, lite_wstrb);
                    6'h0A: tx_produced <=
                        apply_wstrb(tx_produced, lite_wdata, lite_wstrb);
                    6'h0D: irq_enable <=
                        apply_wstrb(irq_enable, lite_wdata, lite_wstrb);
                    default: begin end
                endcase
                lite_aw_pending <= 1'b0;
                lite_w_pending <= 1'b0;
                lite_bvalid <= 1'b1;
            end
        end
    end

    always @(posedge aclk) begin
        if (!aresetn) begin
            lite_rdata <= 32'h0000_0000;
            lite_rvalid <= 1'b0;
        end else begin
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
        if (!aresetn || soft_reset_pulse) begin
            rx_produced <= 32'd0;
            tx_consumed <= 32'd0;
            irq_status <= 32'd0;
            parity_error_count <= 16'd0;
            frame_error_count <= 16'd0;
            overflow_error_count <= 16'd0;
            axi_read_error_count <= 8'd0;
            axi_write_error_count <= 8'd0;
            dma_stop_reason <= 32'd0;
            rx_dma_halted <= 1'b0;
            tx_dma_halted <= 1'b0;
            rx_fifo_pop <= 1'b0;
            rx_fifo_pop_delay <= 1'b0;
            tx_fifo_push <= 1'b0;
            tx_fifo_push_data <= 8'h00;
            rx_write_active <= 1'b0;
            rx_awvalid <= 1'b0;
            rx_wvalid <= 1'b0;
            rx_bready <= 1'b0;
            rx_write_address <= 32'd0;
            rx_write_data <= 32'd0;
            rx_write_strobe <= 4'd0;
            tx_read_active <= 1'b0;
            tx_arvalid <= 1'b0;
            tx_rready <= 1'b0;
            tx_read_address <= 32'd0;
            tx_read_lane <= 2'd0;
        end else begin
            rx_fifo_pop <= 1'b0;
            tx_fifo_push <= 1'b0;
            if (rx_fifo_pop_delay) begin
                rx_fifo_pop_delay <= 1'b0;
            end

            if (lite_write_commit && (lite_write_index == 6'h0C)) begin
                irq_status <= irq_status &
                    ~apply_wstrb(32'd0, lite_wdata, lite_wstrb);
            end

            if (serial_parity_error) begin
                irq_status[IRQ_PARITY] <= 1'b1;
                parity_error_count <= parity_error_count + 16'd1;
            end
            if (serial_frame_error || serial_break_error) begin
                irq_status[IRQ_FRAME] <= 1'b1;
                frame_error_count <= frame_error_count + 16'd1;
            end
            if (serial_rx_valid && rx_fifo_full) begin
                irq_status[IRQ_OVERFLOW] <= 1'b1;
                overflow_error_count <= overflow_error_count + 16'd1;
            end
            if (lite_write_commit && (lite_write_index == 6'h00) &&
                lite_wdata[0] && !ring_configuration_valid) begin
                irq_status[IRQ_CONFIG] <= 1'b1;
                dma_stop_reason[5] <= 1'b1;
            end

            if (uart_enable && !rx_dma_halted && !rx_write_active &&
                !rx_fifo_pop_delay &&
                !rx_fifo_empty) begin
                if (rx_ring_full) begin
                    rx_fifo_pop <= 1'b1;
                    rx_fifo_pop_delay <= 1'b1;
                    irq_status[IRQ_OVERFLOW] <= 1'b1;
                    overflow_error_count <= overflow_error_count + 16'd1;
                end else begin
                    rx_write_address <=
                        (rx_ring_base + (rx_produced & (rx_ring_size - 32'd1))) &
                        32'hFFFF_FFFC;
                    case ((rx_ring_base +
                           (rx_produced & (rx_ring_size - 32'd1))) & 32'd3)
                        32'd0: begin
                            rx_write_data <= {24'd0, rx_fifo_data};
                            rx_write_strobe <= 4'b0001;
                        end
                        32'd1: begin
                            rx_write_data <= {16'd0, rx_fifo_data, 8'd0};
                            rx_write_strobe <= 4'b0010;
                        end
                        32'd2: begin
                            rx_write_data <= {8'd0, rx_fifo_data, 16'd0};
                            rx_write_strobe <= 4'b0100;
                        end
                        default: begin
                            rx_write_data <= {rx_fifo_data, 24'd0};
                            rx_write_strobe <= 4'b1000;
                        end
                    endcase
                    rx_awvalid <= 1'b1;
                    rx_wvalid <= 1'b1;
                    rx_write_active <= 1'b1;
                end
            end

            if (rx_write_active) begin
                if (rx_awvalid && m_axi_awready) rx_awvalid <= 1'b0;
                if (rx_wvalid && m_axi_wready) rx_wvalid <= 1'b0;
                if ((!rx_awvalid || m_axi_awready) &&
                    (!rx_wvalid || m_axi_wready)) begin
                    rx_bready <= 1'b1;
                end
                if (rx_bready && m_axi_bvalid) begin
                    rx_bready <= 1'b0;
                    rx_write_active <= 1'b0;
                    rx_fifo_pop <= 1'b1;
                    rx_fifo_pop_delay <= 1'b1;
                    if (m_axi_bresp == 2'b00) begin
                        rx_produced <= rx_produced + 32'd1;
                    end else begin
                        irq_status[IRQ_AXI_WRITE] <= 1'b1;
                        axi_write_error_count <= axi_write_error_count + 8'd1;
                        dma_stop_reason[4] <= 1'b1;
                        rx_dma_halted <= 1'b1;
                    end
                end
            end

            if (uart_enable && !tx_dma_halted && !tx_read_active &&
                !tx_ring_empty && (tx_fifo_count < 5'd15)) begin
                tx_read_address <=
                    (tx_ring_base + (tx_consumed & (tx_ring_size - 32'd1))) &
                    32'hFFFF_FFFC;
                tx_read_lane <=
                    (tx_ring_base + (tx_consumed & (tx_ring_size - 32'd1))) & 2'd3;
                tx_arvalid <= 1'b1;
                tx_read_active <= 1'b1;
            end

            if (tx_read_active) begin
                if (tx_arvalid && m_axi_arready) begin
                    tx_arvalid <= 1'b0;
                    tx_rready <= 1'b1;
                end
                if (tx_rready && m_axi_rvalid) begin
                    tx_rready <= 1'b0;
                    tx_read_active <= 1'b0;
                    if ((m_axi_rresp == 2'b00) && m_axi_rlast) begin
                        case (tx_read_lane)
                            2'd0: tx_fifo_push_data <= m_axi_rdata[7:0];
                            2'd1: tx_fifo_push_data <= m_axi_rdata[15:8];
                            2'd2: tx_fifo_push_data <= m_axi_rdata[23:16];
                            default: tx_fifo_push_data <= m_axi_rdata[31:24];
                        endcase
                        tx_fifo_push <= 1'b1;
                        tx_consumed <= tx_consumed + 32'd1;
                    end else begin
                        irq_status[IRQ_AXI_READ] <= 1'b1;
                        axi_read_error_count <= axi_read_error_count + 8'd1;
                        dma_stop_reason[3] <= 1'b1;
                        tx_dma_halted <= 1'b1;
                    end
                end
            end
        end
    end

    wire unused_inputs;
    assign unused_inputs = ^{s_axi_awprot, s_axi_arprot, rx_fifo_count,
                             tx_fifo_count};

endmodule

`default_nettype wire
