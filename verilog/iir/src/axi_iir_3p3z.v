// SPDX-License-Identifier: MIT
/**
 * @file    axi_iir_3p3z.v
 * @brief   AXI4-Lite register peripheral for the fixed-point 3P3Z IIR core.
 * @details
 *          The PS configures signed Q2.30 coefficients, signed output limits,
 *          and exchanges signed 32-bit samples with PL logic through
 *          memory-mapped registers.
 *          AXI write address and write data channels are accepted
 *          independently, and byte write strobes are honored.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 2.0.0
 */

`timescale 1 ns / 1 ps
`default_nettype none

module axi_iir_3p3z #(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 8,
    parameter integer COEFF_FRAC_BITS = 30
)(
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aclk, ASSOCIATED_BUSIF S_AXI, ASSOCIATED_RESET s_axi_aresetn, FREQ_HZ 50000000" *)
    (* X_INTERFACE_INFO = "xilinx.com:signal:clock:1.0 s_axi_aclk CLK" *)
    input  wire                              s_axi_aclk,
    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME s_axi_aresetn, POLARITY ACTIVE_LOW" *)
    (* X_INTERFACE_INFO = "xilinx.com:signal:reset:1.0 s_axi_aresetn RST" *)
    input  wire                              s_axi_aresetn,

    (* X_INTERFACE_PARAMETER = "XIL_INTERFACENAME S_AXI, DATA_WIDTH 32, PROTOCOL AXI4LITE, ADDR_WIDTH 8, READ_WRITE_MODE READ_WRITE" *)
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI AWADDR" *)
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     s_axi_awaddr,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI AWPROT" *)
    input  wire [2:0]                        s_axi_awprot,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI AWVALID" *)
    input  wire                              s_axi_awvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI AWREADY" *)
    output wire                              s_axi_awready,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI WDATA" *)
    input  wire [C_S_AXI_DATA_WIDTH-1:0]     s_axi_wdata,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI WSTRB" *)
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0] s_axi_wstrb,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI WVALID" *)
    input  wire                              s_axi_wvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI WREADY" *)
    output wire                              s_axi_wready,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI BRESP" *)
    output wire [1:0]                        s_axi_bresp,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI BVALID" *)
    output wire                              s_axi_bvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI BREADY" *)
    input  wire                              s_axi_bready,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI ARADDR" *)
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]     s_axi_araddr,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI ARPROT" *)
    input  wire [2:0]                        s_axi_arprot,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI ARVALID" *)
    input  wire                              s_axi_arvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI ARREADY" *)
    output wire                              s_axi_arready,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI RDATA" *)
    output wire [C_S_AXI_DATA_WIDTH-1:0]     s_axi_rdata,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI RRESP" *)
    output wire [1:0]                        s_axi_rresp,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI RVALID" *)
    output wire                              s_axi_rvalid,
    (* X_INTERFACE_INFO = "xilinx.com:interface:aximm:1.0 S_AXI RREADY" *)
    input  wire                              s_axi_rready
);

    localparam [31:0] RTL_VERSION = 32'h0002_0000;
    localparam [31:0] FORMAT_VALUE = 32'h0000_201E;

    reg [31:0] input_reg;
    reg [31:0] coeff_b0_reg;
    reg [31:0] coeff_b1_reg;
    reg [31:0] coeff_b2_reg;
    reg [31:0] coeff_b3_reg;
    reg [31:0] coeff_a1_reg;
    reg [31:0] coeff_a2_reg;
    reg [31:0] coeff_a3_reg;
    reg [31:0] limit_lower_reg;
    reg [31:0] limit_upper_reg;

    reg start_pulse;
    reg clear_state_pulse;
    reg done_sticky;

    reg [C_S_AXI_ADDR_WIDTH-1:0] awaddr_hold;
    reg [C_S_AXI_DATA_WIDTH-1:0] wdata_hold;
    reg [(C_S_AXI_DATA_WIDTH/8)-1:0] wstrb_hold;
    reg aw_pending;
    reg w_pending;
    reg bvalid_reg;

    reg [C_S_AXI_DATA_WIDTH-1:0] rdata_reg;
    reg rvalid_reg;

    wire core_ready;
    wire core_busy;
    wire core_done;
    wire core_saturated;
    wire signed [31:0] core_output;
    wire [31:0] core_sample_count;
    wire signed [31:0] history_x1;
    wire signed [31:0] history_x2;
    wire signed [31:0] history_x3;
    wire signed [31:0] history_y1;
    wire signed [31:0] history_y2;
    wire signed [31:0] history_y3;

    wire write_commit;
    wire [5:0] write_index;
    wire [31:0] control_write_value;
    wire [31:0] status_value;

    function [31:0] apply_wstrb;
        input [31:0] current_value;
        input [31:0] write_value;
        input [3:0] byte_strobe;
        integer byte_index;
        begin
            apply_wstrb = current_value;
            for (byte_index = 0; byte_index < 4; byte_index = byte_index + 1) begin
                if (byte_strobe[byte_index] == 1'b1) begin
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
                6'h00: register_read = 32'h0000_0000;
                6'h01: register_read = status_value;
                6'h02: register_read = input_reg;
                6'h03: register_read = core_output;
                6'h04: register_read = coeff_b0_reg;
                6'h05: register_read = coeff_b1_reg;
                6'h06: register_read = coeff_b2_reg;
                6'h07: register_read = coeff_b3_reg;
                6'h08: register_read = coeff_a1_reg;
                6'h09: register_read = coeff_a2_reg;
                6'h0A: register_read = coeff_a3_reg;
                6'h0B: register_read = core_sample_count;
                6'h0C: register_read = RTL_VERSION;
                6'h0D: register_read = FORMAT_VALUE;
                6'h0E: register_read = history_x1;
                6'h0F: register_read = history_x2;
                6'h10: register_read = history_x3;
                6'h11: register_read = history_y1;
                6'h12: register_read = history_y2;
                6'h13: register_read = history_y3;
                6'h14: register_read = limit_lower_reg;
                6'h15: register_read = limit_upper_reg;
                default: register_read = 32'h0000_0000;
            endcase
        end
    endfunction

    assign write_commit = aw_pending && w_pending && !bvalid_reg;
    assign write_index = awaddr_hold[7:2];
    assign control_write_value = apply_wstrb(32'h0000_0000,
                                               wdata_hold,
                                               wstrb_hold);
    assign status_value = {28'h000_0000, core_ready, core_saturated,
                           done_sticky, core_busy};

    assign s_axi_awready = !aw_pending && !bvalid_reg;
    assign s_axi_wready = !w_pending && !bvalid_reg;
    assign s_axi_bresp = 2'b00;
    assign s_axi_bvalid = bvalid_reg;
    assign s_axi_arready = !rvalid_reg;
    assign s_axi_rdata = rdata_reg;
    assign s_axi_rresp = 2'b00;
    assign s_axi_rvalid = rvalid_reg;

    iir_3p3z_core #(
        .COEFF_FRAC_BITS(COEFF_FRAC_BITS)
    ) iir_core (
        .clk(s_axi_aclk),
        .resetn(s_axi_aresetn),
        .start(start_pulse),
        .clear_state(clear_state_pulse),
        .sample_in(input_reg),
        .coeff_b0(coeff_b0_reg),
        .coeff_b1(coeff_b1_reg),
        .coeff_b2(coeff_b2_reg),
        .coeff_b3(coeff_b3_reg),
        .coeff_a1(coeff_a1_reg),
        .coeff_a2(coeff_a2_reg),
        .coeff_a3(coeff_a3_reg),
        .limit_lower(limit_lower_reg),
        .limit_upper(limit_upper_reg),
        .ready(core_ready),
        .busy(core_busy),
        .done(core_done),
        .saturated(core_saturated),
        .sample_out(core_output),
        .sample_count(core_sample_count),
        .history_x1(history_x1),
        .history_x2(history_x2),
        .history_x3(history_x3),
        .history_y1(history_y1),
        .history_y2(history_y2),
        .history_y3(history_y3)
    );

    always @(posedge s_axi_aclk) begin
        if (s_axi_aresetn == 1'b0) begin
            awaddr_hold <= {C_S_AXI_ADDR_WIDTH{1'b0}};
            wdata_hold <= {C_S_AXI_DATA_WIDTH{1'b0}};
            wstrb_hold <= {(C_S_AXI_DATA_WIDTH/8){1'b0}};
            aw_pending <= 1'b0;
            w_pending <= 1'b0;
            bvalid_reg <= 1'b0;
            input_reg <= 32'h0000_0000;
            coeff_b0_reg <= 32'h4000_0000;
            coeff_b1_reg <= 32'h0000_0000;
            coeff_b2_reg <= 32'h0000_0000;
            coeff_b3_reg <= 32'h0000_0000;
            coeff_a1_reg <= 32'h0000_0000;
            coeff_a2_reg <= 32'h0000_0000;
            coeff_a3_reg <= 32'h0000_0000;
            limit_lower_reg <= 32'h8000_0000;
            limit_upper_reg <= 32'h7FFF_FFFF;
            start_pulse <= 1'b0;
            clear_state_pulse <= 1'b0;
            done_sticky <= 1'b0;
        end else begin
            start_pulse <= 1'b0;
            clear_state_pulse <= 1'b0;

            if (core_done == 1'b1) begin
                done_sticky <= 1'b1;
            end

            if (s_axi_awready && s_axi_awvalid) begin
                awaddr_hold <= s_axi_awaddr;
                aw_pending <= 1'b1;
            end

            if (s_axi_wready && s_axi_wvalid) begin
                wdata_hold <= s_axi_wdata;
                wstrb_hold <= s_axi_wstrb;
                w_pending <= 1'b1;
            end

            if (bvalid_reg && s_axi_bready) begin
                bvalid_reg <= 1'b0;
            end

            if (write_commit) begin
                case (write_index)
                    6'h00: begin
                        if (control_write_value[0] == 1'b1) begin
                            start_pulse <= 1'b1;
                            done_sticky <= 1'b0;
                        end
                        if (control_write_value[1] == 1'b1) begin
                            clear_state_pulse <= 1'b1;
                            done_sticky <= 1'b0;
                        end
                        if (control_write_value[2] == 1'b1) begin
                            done_sticky <= 1'b0;
                        end
                    end
                    6'h02: input_reg <= apply_wstrb(input_reg,
                                                     wdata_hold,
                                                     wstrb_hold);
                    6'h04: coeff_b0_reg <= apply_wstrb(coeff_b0_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h05: coeff_b1_reg <= apply_wstrb(coeff_b1_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h06: coeff_b2_reg <= apply_wstrb(coeff_b2_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h07: coeff_b3_reg <= apply_wstrb(coeff_b3_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h08: coeff_a1_reg <= apply_wstrb(coeff_a1_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h09: coeff_a2_reg <= apply_wstrb(coeff_a2_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h0A: coeff_a3_reg <= apply_wstrb(coeff_a3_reg,
                                                        wdata_hold,
                                                        wstrb_hold);
                    6'h14: limit_lower_reg <= apply_wstrb(limit_lower_reg,
                                                           wdata_hold,
                                                           wstrb_hold);
                    6'h15: limit_upper_reg <= apply_wstrb(limit_upper_reg,
                                                           wdata_hold,
                                                           wstrb_hold);
                    default: begin
                    end
                endcase
                aw_pending <= 1'b0;
                w_pending <= 1'b0;
                bvalid_reg <= 1'b1;
            end
        end
    end

    always @(posedge s_axi_aclk) begin
        if (s_axi_aresetn == 1'b0) begin
            rdata_reg <= {C_S_AXI_DATA_WIDTH{1'b0}};
            rvalid_reg <= 1'b0;
        end else begin
            if (rvalid_reg && s_axi_rready) begin
                rvalid_reg <= 1'b0;
            end

            if (s_axi_arready && s_axi_arvalid) begin
                rdata_reg <= register_read(s_axi_araddr[7:2]);
                rvalid_reg <= 1'b1;
            end
        end
    end

    wire unused_prot;
    assign unused_prot = ^{s_axi_awprot, s_axi_arprot};

endmodule

`default_nettype wire
