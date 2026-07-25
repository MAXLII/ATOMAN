`timescale 1 ns / 1 ps
`default_nettype none

module tb_uart_serial_core;

    reg clk = 1'b0;
    reg resetn = 1'b0;
    reg clear = 1'b0;
    reg enable = 1'b0;
    reg [31:0] baud_increment = 32'h8000_0000;
    reg [3:0] data_bits = 4'd8;
    reg [1:0] parity_mode = 2'd0;
    reg two_stop_bits = 1'b0;
    reg internal_loopback = 1'b1;
    reg uart_rxd = 1'b1;
    wire uart_txd;
    reg [7:0] tx_data = 8'd0;
    reg tx_valid = 1'b0;
    wire tx_ready;
    wire tx_busy;
    wire [7:0] rx_data;
    wire rx_valid;
    wire parity_error_pulse;
    wire frame_error_pulse;
    wire break_error_pulse;

    integer failures = 0;
    integer vectors = 0;
    integer bits;
    integer parity;
    integer stops;
    reg [7:0] expected_data;

    always #10 clk = ~clk;

    axi_unused_guard unused_guard();

    uart_serial_core dut (
        .clk(clk),
        .resetn(resetn),
        .clear(clear),
        .enable(enable),
        .baud_increment(baud_increment),
        .data_bits(data_bits),
        .parity_mode(parity_mode),
        .two_stop_bits(two_stop_bits),
        .internal_loopback(internal_loopback),
        .uart_rxd(uart_rxd),
        .uart_txd(uart_txd),
        .tx_data(tx_data),
        .tx_valid(tx_valid),
        .tx_ready(tx_ready),
        .tx_busy(tx_busy),
        .rx_data(rx_data),
        .rx_valid(rx_valid),
        .parity_error_pulse(parity_error_pulse),
        .frame_error_pulse(frame_error_pulse),
        .break_error_pulse(break_error_pulse)
    );

    task automatic pulse_clear;
        begin
            @(posedge clk);
            clear <= 1'b1;
            @(posedge clk);
            clear <= 1'b0;
        end
    endtask

    task automatic loopback_byte(input [7:0] value);
        reg [7:0] mask;
        begin
            while (!tx_ready) @(posedge clk);
            tx_data <= value;
            tx_valid <= 1'b1;
            @(posedge clk);
            tx_valid <= 1'b0;
            while (!rx_valid) @(posedge clk);
            mask = (data_bits == 4'd8) ? 8'hFF :
                   ((8'h01 << data_bits) - 8'h01);
            expected_data = value & mask;
            vectors = vectors + 1;
            if (rx_data !== expected_data) begin
                $display("CORE_MISMATCH bits=%0d parity=%0d stops=%0d expected=%02x actual=%02x",
                         data_bits, parity_mode, two_stop_bits + 1,
                         expected_data, rx_data);
                failures = failures + 1;
            end
            if (parity_error_pulse || frame_error_pulse || break_error_pulse) begin
                $display("CORE_UNEXPECTED_ERROR bits=%0d parity=%0d stops=%0d",
                         data_bits, parity_mode, two_stop_bits + 1);
                failures = failures + 1;
            end
        end
    endtask

    task automatic drive_bit(input bit value);
        begin
            uart_rxd <= value;
            repeat (32) @(posedge clk);
        end
    endtask

    reg parity_seen = 1'b0;
    reg frame_seen = 1'b0;
    reg break_seen = 1'b0;
    always @(posedge clk) begin
        if (parity_error_pulse) parity_seen <= 1'b1;
        if (frame_error_pulse) frame_seen <= 1'b1;
        if (break_error_pulse) break_seen <= 1'b1;
        if (clear) begin
            parity_seen <= 1'b0;
            frame_seen <= 1'b0;
            break_seen <= 1'b0;
        end
    end

    initial begin
        repeat (5) @(posedge clk);
        resetn <= 1'b1;
        enable <= 1'b1;

        for (bits = 5; bits <= 8; bits = bits + 1) begin
            for (parity = 0; parity <= 2; parity = parity + 1) begin
                for (stops = 0; stops <= 1; stops = stops + 1) begin
                    data_bits = bits[3:0];
                    parity_mode = parity[1:0];
                    two_stop_bits = stops[0];
                    pulse_clear();
                    loopback_byte(8'hA5 ^ bits ^ (parity << 4) ^ stops);
                end
            end
        end

        internal_loopback = 1'b0;
        data_bits = 4'd8;
        parity_mode = 2'd1;
        two_stop_bits = 1'b0;
        pulse_clear();
        repeat (4) @(posedge clk);
        drive_bit(1'b0);
        expected_data = 8'hA5;
        for (bits = 0; bits < 8; bits = bits + 1) drive_bit(expected_data[bits]);
        drive_bit(1'b1); // A5 has even XOR=0, so this parity bit is wrong.
        drive_bit(1'b0); // Invalid stop bit.
        drive_bit(1'b1);
        repeat (40) @(posedge clk);
        vectors = vectors + 1;
        if (!parity_seen || !frame_seen) begin
            $display("CORE_ERROR_INJECTION_FAIL parity=%0d frame=%0d",
                     parity_seen, frame_seen);
            failures = failures + 1;
        end

        if (failures == 0) begin
            $display("UART_CORE_SIM_RESULT PASS vectors=%0d failures=0", vectors);
            $finish;
        end
        $fatal(1, "UART_CORE_SIM_RESULT FAIL vectors=%0d failures=%0d",
               vectors, failures);
    end

endmodule

// Keeps Icarus from treating an otherwise empty compilation unit as a typo.
module axi_unused_guard;
endmodule

`default_nettype wire
