// SPDX-License-Identifier: MIT
/**
 * @file    uart_serial_core.v
 * @brief   Configurable oversampling UART transmitter and receiver.
 * @details
 *          This file is part of the base project.
 *
 *          The core implements 5 to 8 data bits, optional even or odd parity,
 *          one or two stop bits, and a 16x fractional baud tick. Receive input
 *          is synchronized before start, data, parity, and stop validation.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module uart_serial_core #(
    parameter integer OVERSAMPLE = 16
)(
    input  wire        clk,
    input  wire        resetn,
    input  wire        clear,
    input  wire        enable,
    input  wire [31:0] baud_increment,
    input  wire [3:0]  data_bits,
    input  wire [1:0]  parity_mode,
    input  wire        two_stop_bits,
    input  wire        internal_loopback,
    input  wire        uart_rxd,
    output wire        uart_txd,

    input  wire [7:0]  tx_data,
    input  wire        tx_valid,
    output wire        tx_ready,
    output wire        tx_busy,

    output reg  [7:0]  rx_data,
    output reg         rx_valid,
    output reg         parity_error_pulse,
    output reg         frame_error_pulse,
    output reg         break_error_pulse
);

    localparam [2:0] TX_IDLE   = 3'd0;
    localparam [2:0] TX_START  = 3'd1;
    localparam [2:0] TX_DATA   = 3'd2;
    localparam [2:0] TX_PARITY = 3'd3;
    localparam [2:0] TX_STOP1  = 3'd4;
    localparam [2:0] TX_STOP2  = 3'd5;

    localparam [2:0] RX_IDLE   = 3'd0;
    localparam [2:0] RX_START  = 3'd1;
    localparam [2:0] RX_DATA   = 3'd2;
    localparam [2:0] RX_PARITY = 3'd3;
    localparam [2:0] RX_STOP1  = 3'd4;
    localparam [2:0] RX_STOP2  = 3'd5;
    localparam integer SAMPLE_MID = (OVERSAMPLE / 2) - 1;
    localparam integer SAMPLE_LAST = OVERSAMPLE - 1;

    reg [31:0] baud_accumulator;
    reg [32:0] baud_sum;
    reg baud_tick;

    reg rxd_meta;
    reg rxd_sync;
    wire selected_rxd;

    reg [2:0] tx_state;
    reg [3:0] tx_tick_count;
    reg [2:0] tx_bit_index;
    reg [7:0] tx_shift;
    reg tx_parity;
    reg tx_output;

    reg [2:0] rx_state;
    reg [3:0] rx_tick_count;
    reg [2:0] rx_bit_index;
    reg [7:0] rx_shift;
    reg rx_parity;
    reg rx_all_low;

    wire parity_enabled;
    wire tx_expected_parity;
    wire rx_expected_parity;

    assign selected_rxd = internal_loopback ? tx_output : rxd_sync;
    assign uart_txd = tx_output;
    assign tx_ready = enable && (tx_state == TX_IDLE);
    assign tx_busy = (tx_state != TX_IDLE);
    assign parity_enabled = (parity_mode != 2'b00);
    assign tx_expected_parity =
        (parity_mode == 2'b10) ? ~tx_parity : tx_parity;
    assign rx_expected_parity =
        (parity_mode == 2'b10) ? ~rx_parity : rx_parity;

    always @(posedge clk) begin
        if (!resetn || clear) begin
            baud_accumulator <= 32'h0000_0000;
            baud_sum <= 33'h0_0000_0000;
            baud_tick <= 1'b0;
        end else begin
            baud_sum = {1'b0, baud_accumulator} + {1'b0, baud_increment};
            baud_accumulator <= baud_sum[31:0];
            baud_tick <= baud_sum[32] && enable;
        end
    end

    always @(posedge clk) begin
        if (!resetn || clear) begin
            rxd_meta <= 1'b1;
            rxd_sync <= 1'b1;
        end else begin
            rxd_meta <= uart_rxd;
            rxd_sync <= rxd_meta;
        end
    end

    always @(posedge clk) begin
        if (!resetn || clear || !enable) begin
            tx_state <= TX_IDLE;
            tx_tick_count <= 4'd0;
            tx_bit_index <= 3'd0;
            tx_shift <= 8'h00;
            tx_parity <= 1'b0;
            tx_output <= 1'b1;
        end else begin
            if ((tx_state == TX_IDLE) && tx_valid) begin
                tx_shift <= tx_data;
                tx_parity <= 1'b0;
                tx_bit_index <= 3'd0;
                tx_tick_count <= 4'd0;
                tx_output <= 1'b0;
                tx_state <= TX_START;
            end else if (baud_tick) begin
                if (tx_tick_count != SAMPLE_LAST) begin
                    tx_tick_count <= tx_tick_count + 4'd1;
                end else begin
                    tx_tick_count <= 4'd0;
                    case (tx_state)
                        TX_START: begin
                            tx_output <= tx_shift[0];
                            tx_parity <= tx_shift[0];
                            tx_state <= TX_DATA;
                        end
                        TX_DATA: begin
                            if ({1'b0, tx_bit_index} == (data_bits - 4'd1)) begin
                                if (parity_enabled) begin
                                    tx_output <= tx_expected_parity;
                                    tx_state <= TX_PARITY;
                                end else begin
                                    tx_output <= 1'b1;
                                    tx_state <= TX_STOP1;
                                end
                            end else begin
                                tx_bit_index <= tx_bit_index + 3'd1;
                                tx_shift <= {1'b0, tx_shift[7:1]};
                                tx_output <= tx_shift[1];
                                tx_parity <= tx_parity ^ tx_shift[1];
                            end
                        end
                        TX_PARITY: begin
                            tx_output <= 1'b1;
                            tx_state <= TX_STOP1;
                        end
                        TX_STOP1: begin
                            if (two_stop_bits) begin
                                tx_state <= TX_STOP2;
                            end else begin
                                tx_state <= TX_IDLE;
                            end
                        end
                        TX_STOP2: begin
                            tx_state <= TX_IDLE;
                        end
                        default: begin
                            tx_state <= TX_IDLE;
                            tx_output <= 1'b1;
                        end
                    endcase
                end
            end
        end
    end

    always @(posedge clk) begin
        if (!resetn || clear || !enable) begin
            rx_state <= RX_IDLE;
            rx_tick_count <= 4'd0;
            rx_bit_index <= 3'd0;
            rx_shift <= 8'h00;
            rx_parity <= 1'b0;
            rx_all_low <= 1'b1;
            rx_data <= 8'h00;
            rx_valid <= 1'b0;
            parity_error_pulse <= 1'b0;
            frame_error_pulse <= 1'b0;
            break_error_pulse <= 1'b0;
        end else begin
            rx_valid <= 1'b0;
            parity_error_pulse <= 1'b0;
            frame_error_pulse <= 1'b0;
            break_error_pulse <= 1'b0;

            if (rx_state == RX_IDLE) begin
                if (!selected_rxd) begin
                    rx_state <= RX_START;
                    rx_tick_count <= 4'd0;
                    rx_all_low <= 1'b1;
                end
            end else if (baud_tick) begin
                rx_all_low <= rx_all_low && !selected_rxd;
                case (rx_state)
                    RX_START: begin
                        if (rx_tick_count == SAMPLE_MID) begin
                            if (!selected_rxd) begin
                                rx_state <= RX_DATA;
                                rx_tick_count <= 4'd0;
                                rx_bit_index <= 3'd0;
                                rx_shift <= 8'h00;
                                rx_parity <= 1'b0;
                            end else begin
                                rx_state <= RX_IDLE;
                            end
                        end else begin
                            rx_tick_count <= rx_tick_count + 4'd1;
                        end
                    end
                    RX_DATA: begin
                        if (rx_tick_count == SAMPLE_LAST) begin
                            rx_tick_count <= 4'd0;
                            rx_shift[rx_bit_index] <= selected_rxd;
                            rx_parity <= rx_parity ^ selected_rxd;
                            if ({1'b0, rx_bit_index} == (data_bits - 4'd1)) begin
                                if (parity_enabled) begin
                                    rx_state <= RX_PARITY;
                                end else begin
                                    rx_state <= RX_STOP1;
                                end
                            end else begin
                                rx_bit_index <= rx_bit_index + 3'd1;
                            end
                        end else begin
                            rx_tick_count <= rx_tick_count + 4'd1;
                        end
                    end
                    RX_PARITY: begin
                        if (rx_tick_count == SAMPLE_LAST) begin
                            rx_tick_count <= 4'd0;
                            if (selected_rxd != rx_expected_parity) begin
                                parity_error_pulse <= 1'b1;
                            end
                            rx_state <= RX_STOP1;
                        end else begin
                            rx_tick_count <= rx_tick_count + 4'd1;
                        end
                    end
                    RX_STOP1: begin
                        if (rx_tick_count == SAMPLE_LAST) begin
                            rx_tick_count <= 4'd0;
                            if (!selected_rxd) begin
                                frame_error_pulse <= 1'b1;
                                if (rx_all_low) begin
                                    break_error_pulse <= 1'b1;
                                end
                            end
                            if (two_stop_bits) begin
                                rx_state <= RX_STOP2;
                            end else begin
                                rx_data <= rx_shift;
                                rx_valid <= 1'b1;
                                rx_state <= RX_IDLE;
                            end
                        end else begin
                            rx_tick_count <= rx_tick_count + 4'd1;
                        end
                    end
                    RX_STOP2: begin
                        if (rx_tick_count == SAMPLE_LAST) begin
                            if (!selected_rxd) begin
                                frame_error_pulse <= 1'b1;
                            end
                            rx_data <= rx_shift;
                            rx_valid <= 1'b1;
                            rx_state <= RX_IDLE;
                            rx_tick_count <= 4'd0;
                        end else begin
                            rx_tick_count <= rx_tick_count + 4'd1;
                        end
                    end
                    default: rx_state <= RX_IDLE;
                endcase
            end
        end
    end

endmodule

`default_nettype wire
