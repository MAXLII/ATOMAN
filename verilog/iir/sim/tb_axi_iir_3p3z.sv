// SPDX-License-Identifier: MIT
/**
 * @file    tb_axi_iir_3p3z.sv
 * @brief   AXI4-Lite protocol and numerical testbench for the 3P3Z IIR.
 * @details
 *          Verifies independent AW/W ordering, byte strobes, register
 *          readback, impulse response, history, sample counting, and signed
 *          output saturation through the complete AXI peripheral wrapper.
 *
 * @author  Max.Li
 * @date    2026-07-17
 * @version 1.0.0
 */

`timescale 1 ns / 1 ps

module tb_axi_iir_3p3z;

    localparam logic [7:0] REG_CONTROL      = 8'h00;
    localparam logic [7:0] REG_STATUS       = 8'h04;
    localparam logic [7:0] REG_INPUT        = 8'h08;
    localparam logic [7:0] REG_OUTPUT       = 8'h0C;
    localparam logic [7:0] REG_B0           = 8'h10;
    localparam logic [7:0] REG_B1           = 8'h14;
    localparam logic [7:0] REG_B2           = 8'h18;
    localparam logic [7:0] REG_B3           = 8'h1C;
    localparam logic [7:0] REG_A1           = 8'h20;
    localparam logic [7:0] REG_A2           = 8'h24;
    localparam logic [7:0] REG_A3           = 8'h28;
    localparam logic [7:0] REG_SAMPLE_COUNT = 8'h2C;
    localparam logic [7:0] REG_VERSION      = 8'h30;
    localparam logic [7:0] REG_FORMAT       = 8'h34;
    localparam logic [7:0] REG_X1           = 8'h38;
    localparam logic [7:0] REG_Y1           = 8'h44;
    localparam logic [7:0] REG_Y2           = 8'h48;
    localparam logic [7:0] REG_Y3           = 8'h4C;

    localparam logic [31:0] CONTROL_START       = 32'h0000_0001;
    localparam logic [31:0] CONTROL_RESET_STATE = 32'h0000_0002;
    localparam logic [31:0] CONTROL_CLEAR_DONE  = 32'h0000_0004;

    logic clk;
    logic resetn;

    logic [7:0] awaddr;
    logic [2:0] awprot;
    logic awvalid;
    wire awready;
    logic [31:0] wdata;
    logic [3:0] wstrb;
    logic wvalid;
    wire wready;
    wire [1:0] bresp;
    wire bvalid;
    logic bready;
    logic [7:0] araddr;
    logic [2:0] arprot;
    logic arvalid;
    wire arready;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire rvalid;
    logic rready;

    integer failure_count;
    integer result_file;
    integer vector_index;
    integer signed_error;
    logic signed [31:0] impulse_input [0:7];
    logic signed [31:0] impulse_expected [0:7];
    logic [31:0] actual_value;
    logic [31:0] status_value;
    logic [31:0] read_value;

    axi_iir_3p3z dut (
        .s_axi_aclk(clk),
        .s_axi_aresetn(resetn),
        .s_axi_awaddr(awaddr),
        .s_axi_awprot(awprot),
        .s_axi_awvalid(awvalid),
        .s_axi_awready(awready),
        .s_axi_wdata(wdata),
        .s_axi_wstrb(wstrb),
        .s_axi_wvalid(wvalid),
        .s_axi_wready(wready),
        .s_axi_bresp(bresp),
        .s_axi_bvalid(bvalid),
        .s_axi_bready(bready),
        .s_axi_araddr(araddr),
        .s_axi_arprot(arprot),
        .s_axi_arvalid(arvalid),
        .s_axi_arready(arready),
        .s_axi_rdata(rdata),
        .s_axi_rresp(rresp),
        .s_axi_rvalid(rvalid),
        .s_axi_rready(rready)
    );

    always #5 clk = ~clk;

    task automatic record_failure(input string message);
        begin
            failure_count = failure_count + 1;
            $display("FAIL: %s", message);
        end
    endtask

    task automatic check_word(
        input string name,
        input logic [31:0] actual,
        input logic [31:0] expected
    );
        begin
            if (actual !== expected) begin
                failure_count = failure_count + 1;
                $display("FAIL: %s actual=0x%08h expected=0x%08h",
                         name, actual, expected);
            end
        end
    endtask

    task automatic finish_write_response;
        begin
            while (bvalid !== 1'b1) begin
                @(posedge clk);
            end
            if (bresp !== 2'b00) begin
                record_failure("AXI write response was not OKAY");
            end
            @(negedge clk);
            bready = 1'b1;
            @(posedge clk);
            @(negedge clk);
            bready = 1'b0;
        end
    endtask

    task automatic axi_write(
        input logic [7:0] address,
        input logic [31:0] value,
        input logic [3:0] byte_strobe
    );
        logic address_done;
        logic data_done;
        begin
            address_done = 1'b0;
            data_done = 1'b0;
            @(negedge clk);
            awaddr = address;
            awvalid = 1'b1;
            wdata = value;
            wstrb = byte_strobe;
            wvalid = 1'b1;

            while ((address_done == 1'b0) || (data_done == 1'b0)) begin
                @(posedge clk);
                if ((awvalid == 1'b1) && (awready == 1'b1)) begin
                    address_done = 1'b1;
                end
                if ((wvalid == 1'b1) && (wready == 1'b1)) begin
                    data_done = 1'b1;
                end
                @(negedge clk);
                if (address_done == 1'b1) begin
                    awvalid = 1'b0;
                end
                if (data_done == 1'b1) begin
                    wvalid = 1'b0;
                end
            end
            finish_write_response();
        end
    endtask

    task automatic axi_write_address_first(
        input logic [7:0] address,
        input logic [31:0] value,
        input logic [3:0] byte_strobe
    );
        begin
            @(negedge clk);
            awaddr = address;
            awvalid = 1'b1;
            while (awready !== 1'b1) begin
                @(posedge clk);
            end
            @(posedge clk);
            @(negedge clk);
            awvalid = 1'b0;
            repeat (2) @(posedge clk);

            @(negedge clk);
            wdata = value;
            wstrb = byte_strobe;
            wvalid = 1'b1;
            while (wready !== 1'b1) begin
                @(posedge clk);
            end
            @(posedge clk);
            @(negedge clk);
            wvalid = 1'b0;
            finish_write_response();
        end
    endtask

    task automatic axi_write_data_first(
        input logic [7:0] address,
        input logic [31:0] value,
        input logic [3:0] byte_strobe
    );
        begin
            @(negedge clk);
            wdata = value;
            wstrb = byte_strobe;
            wvalid = 1'b1;
            while (wready !== 1'b1) begin
                @(posedge clk);
            end
            @(posedge clk);
            @(negedge clk);
            wvalid = 1'b0;
            repeat (2) @(posedge clk);

            @(negedge clk);
            awaddr = address;
            awvalid = 1'b1;
            while (awready !== 1'b1) begin
                @(posedge clk);
            end
            @(posedge clk);
            @(negedge clk);
            awvalid = 1'b0;
            finish_write_response();
        end
    endtask

    task automatic axi_read(
        input logic [7:0] address,
        output logic [31:0] value
    );
        begin
            @(negedge clk);
            araddr = address;
            arvalid = 1'b1;
            while (arready !== 1'b1) begin
                @(posedge clk);
            end
            @(posedge clk);
            @(negedge clk);
            arvalid = 1'b0;

            while (rvalid !== 1'b1) begin
                @(posedge clk);
            end
            value = rdata;
            if (rresp !== 2'b00) begin
                record_failure("AXI read response was not OKAY");
            end
            @(negedge clk);
            rready = 1'b1;
            @(posedge clk);
            @(negedge clk);
            rready = 1'b0;
        end
    endtask

    task automatic process_sample(
        input logic signed [31:0] sample,
        output logic [31:0] output_sample,
        output logic [31:0] final_status
    );
        integer poll_count;
        begin
            axi_write(REG_INPUT, sample, 4'hF);
            axi_write(REG_CONTROL, CONTROL_START, 4'hF);
            poll_count = 0;
            final_status = 32'h0000_0000;
            while (((final_status & 32'h0000_0002) == 0) &&
                   (poll_count < 64)) begin
                axi_read(REG_STATUS, final_status);
                poll_count = poll_count + 1;
            end
            if ((final_status & 32'h0000_0002) == 0) begin
                record_failure("IIR completion timeout");
            end
            axi_read(REG_OUTPUT, output_sample);
            axi_write(REG_CONTROL, CONTROL_CLEAR_DONE, 4'hF);
        end
    endtask

    initial begin
        clk = 1'b0;
        resetn = 1'b0;
        awaddr = 8'h00;
        awprot = 3'b000;
        awvalid = 1'b0;
        wdata = 32'h0000_0000;
        wstrb = 4'h0;
        wvalid = 1'b0;
        bready = 1'b0;
        araddr = 8'h00;
        arprot = 3'b000;
        arvalid = 1'b0;
        rready = 1'b0;
        failure_count = 0;

        impulse_input[0] = 32'sd1048576;
        impulse_input[1] = 32'sd0;
        impulse_input[2] = 32'sd0;
        impulse_input[3] = 32'sd0;
        impulse_input[4] = 32'sd0;
        impulse_input[5] = 32'sd0;
        impulse_input[6] = 32'sd0;
        impulse_input[7] = 32'sd0;

        impulse_expected[0] = 32'sd524288;
        impulse_expected[1] = 32'sd131072;
        impulse_expected[2] = 32'sd32768;
        impulse_expected[3] = 32'sd8192;
        impulse_expected[4] = -32'sd14336;
        impulse_expected[5] = 32'sd512;
        impulse_expected[6] = 32'sd1152;
        impulse_expected[7] = 32'sd544;

        result_file = $fopen("iir_3p3z_numeric_results.csv", "w");
        if (result_file == 0) begin
            $fatal(1, "Unable to open numeric result file");
        end
        $fdisplay(result_file,
                  "case,index,input,expected,actual,error,status");

        repeat (8) @(posedge clk);
        @(negedge clk);
        resetn = 1'b1;
        repeat (3) @(posedge clk);

        axi_read(REG_VERSION, read_value);
        check_word("VERSION", read_value, 32'h0001_0000);
        axi_read(REG_FORMAT, read_value);
        check_word("FORMAT", read_value, 32'h0000_201E);
        axi_read(REG_B0, read_value);
        check_word("default B0", read_value, 32'h4000_0000);

        axi_write(REG_B0, 32'h1122_3344, 4'hF);
        axi_write_address_first(REG_B0, 32'h0000_AA00, 4'b0010);
        axi_read(REG_B0, read_value);
        check_word("WSTRB and AW-before-W", read_value, 32'h1122_AA44);
        axi_write_data_first(REG_B0, 32'h2000_0000, 4'hF);

        axi_write(REG_B1, 32'h1000_0000, 4'hF);
        axi_write(REG_B2, 32'h0800_0000, 4'hF);
        axi_write(REG_B3, 32'h0400_0000, 4'hF);
        axi_write(REG_A1, 32'h1000_0000, 4'hF);
        axi_write(REG_A2, 32'h0800_0000, 4'hF);
        axi_write(REG_A3, 32'h0400_0000, 4'hF);
        axi_write(REG_CONTROL, CONTROL_RESET_STATE, 4'hF);

        for (vector_index = 0; vector_index < 8; vector_index = vector_index + 1) begin
            process_sample(impulse_input[vector_index], actual_value, status_value);
            signed_error = $signed(actual_value) - impulse_expected[vector_index];
            $fdisplay(result_file, "impulse,%0d,%0d,%0d,%0d,%0d,0x%08h",
                      vector_index,
                      impulse_input[vector_index],
                      impulse_expected[vector_index],
                      $signed(actual_value),
                      signed_error,
                      status_value);
            check_word("impulse output", actual_value,
                       impulse_expected[vector_index]);
            if ((status_value & 32'h0000_0004) != 0) begin
                record_failure("Impulse response unexpectedly saturated");
            end
        end

        axi_read(REG_SAMPLE_COUNT, read_value);
        check_word("sample count", read_value, 32'd8);
        axi_read(REG_X1, read_value);
        check_word("x1 history", read_value, 32'd0);
        axi_read(REG_Y1, read_value);
        check_word("y1 history", read_value, 32'd544);
        axi_read(REG_Y2, read_value);
        check_word("y2 history", read_value, 32'd1152);
        axi_read(REG_Y3, read_value);
        check_word("y3 history", read_value, 32'd512);

        axi_write(REG_CONTROL, CONTROL_RESET_STATE, 4'hF);
        axi_write(REG_B0, 32'h7FFF_FFFF, 4'hF);
        axi_write(REG_B1, 32'h0000_0000, 4'hF);
        axi_write(REG_B2, 32'h0000_0000, 4'hF);
        axi_write(REG_B3, 32'h0000_0000, 4'hF);
        axi_write(REG_A1, 32'h0000_0000, 4'hF);
        axi_write(REG_A2, 32'h0000_0000, 4'hF);
        axi_write(REG_A3, 32'h0000_0000, 4'hF);
        process_sample(32'sh7FFF_FFFF, actual_value, status_value);
        $fdisplay(result_file,
                  "positive_saturation,0,2147483647,2147483647,%0d,%0d,0x%08h",
                  $signed(actual_value),
                  $signed(actual_value) - 32'sh7FFF_FFFF,
                  status_value);
        check_word("positive saturation", actual_value, 32'h7FFF_FFFF);
        if ((status_value & 32'h0000_0004) == 0) begin
            record_failure("Positive saturation flag was not set");
        end

        axi_write(REG_CONTROL, CONTROL_RESET_STATE, 4'hF);
        process_sample(32'sh8000_0000, actual_value, status_value);
        $fdisplay(result_file,
                  "negative_saturation,0,-2147483648,-2147483648,%0d,%0d,0x%08h",
                  $signed(actual_value),
                  $signed(actual_value) - (-32'sd2147483647 - 32'sd1),
                  status_value);
        check_word("negative saturation", actual_value, 32'h8000_0000);
        if ((status_value & 32'h0000_0004) == 0) begin
            record_failure("Negative saturation flag was not set");
        end

        $fclose(result_file);
        if (failure_count == 0) begin
            $display("SIM_RESULT PASS vectors=10 failures=0");
        end else begin
            $display("SIM_RESULT FAIL vectors=10 failures=%0d", failure_count);
            $fatal(1, "3P3Z AXI simulation failed");
        end

        repeat (5) @(posedge clk);
        $finish;
    end

endmodule
