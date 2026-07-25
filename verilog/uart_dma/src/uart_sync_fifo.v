// SPDX-License-Identifier: MIT
/**
 * @file    uart_sync_fifo.v
 * @brief   Small synchronous FIFO used by the PL UART DMA.
 * @details
 *          This file is part of the base project.
 *
 *          The FIFO decouples UART bit timing from AXI memory latency. It
 *          supports one push and one pop in the same clock cycle and uses no
 *          dynamic or vendor-specific storage primitive.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module uart_sync_fifo #(
    parameter integer DATA_WIDTH = 8,
    parameter integer ADDR_WIDTH = 4
)(
    input  wire                  clk,
    input  wire                  resetn,
    input  wire                  clear,
    input  wire                  push,
    input  wire [DATA_WIDTH-1:0] push_data,
    input  wire                  pop,
    output wire [DATA_WIDTH-1:0] pop_data,
    output wire                  empty,
    output wire                  full,
    output wire [ADDR_WIDTH:0]   count
);

    localparam integer DEPTH = (1 << ADDR_WIDTH);

    reg [DATA_WIDTH-1:0] memory [0:DEPTH-1];
    reg [ADDR_WIDTH-1:0] write_pointer;
    reg [ADDR_WIDTH-1:0] read_pointer;
    reg [ADDR_WIDTH:0] item_count;

    wire push_accept;
    wire pop_accept;

    assign empty = (item_count == 0);
    assign full = (item_count == DEPTH);
    assign count = item_count;
    assign pop_data = memory[read_pointer];
    assign pop_accept = pop && !empty;
    assign push_accept = push && (!full || pop_accept);

    always @(posedge clk) begin
        if (!resetn || clear) begin
            write_pointer <= {ADDR_WIDTH{1'b0}};
            read_pointer <= {ADDR_WIDTH{1'b0}};
            item_count <= {(ADDR_WIDTH+1){1'b0}};
        end else begin
            if (push_accept) begin
                memory[write_pointer] <= push_data;
                write_pointer <= write_pointer + {{(ADDR_WIDTH-1){1'b0}}, 1'b1};
            end

            if (pop_accept) begin
                read_pointer <= read_pointer + {{(ADDR_WIDTH-1){1'b0}}, 1'b1};
            end

            case ({push_accept, pop_accept})
                2'b10: item_count <= item_count + {{ADDR_WIDTH{1'b0}}, 1'b1};
                2'b01: item_count <= item_count - {{ADDR_WIDTH{1'b0}}, 1'b1};
                default: item_count <= item_count;
            endcase
        end
    end

endmodule

`default_nettype wire
