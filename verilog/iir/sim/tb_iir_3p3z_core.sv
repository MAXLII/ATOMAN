// SPDX-License-Identifier: MIT
/**
 * @file    tb_iir_3p3z_core.sv
 * @brief   Standalone numerical verification for the PL 3P3Z IIR core.
 * @details
 *          Compares every completed RTL sample against an independent
 *          fixed-point direct-form-I reference model. Tests cover signed
 *          multiplier quadrants, continuous pseudo-random input, stateful
 *          feedback, configurable output limiting, one-cycle completion,
 *          continuous one-sample-per-clock throughput, and sample counting
 *          without using AXI or PS software.
 *
 * @author  Max.Li
 * @date    2026-07-25
 * @version 2.0.0
 */

`timescale 1 ns / 1 ps

module tb_iir_3p3z_core;

    localparam integer COEFF_FRAC_BITS = 30;
    localparam integer EXPECTED_COMPLETION_CYCLES = 1;

    logic clk;
    logic resetn;
    logic start;
    logic clear_state;
    logic signed [31:0] sample_in;
    logic signed [31:0] coeff_b0;
    logic signed [31:0] coeff_b1;
    logic signed [31:0] coeff_b2;
    logic signed [31:0] coeff_b3;
    logic signed [31:0] coeff_a1;
    logic signed [31:0] coeff_a2;
    logic signed [31:0] coeff_a3;
    logic signed [31:0] limit_lower;
    logic signed [31:0] limit_upper;
    wire ready;
    wire busy;
    wire done;
    wire saturated;
    wire signed [31:0] sample_out;
    wire [31:0] sample_count;
    wire signed [31:0] history_x1;
    wire signed [31:0] history_x2;
    wire signed [31:0] history_x3;
    wire signed [31:0] history_y1;
    wire signed [31:0] history_y2;
    wire signed [31:0] history_y3;

    logic signed [31:0] model_x1;
    logic signed [31:0] model_x2;
    logic signed [31:0] model_x3;
    logic signed [31:0] model_y1;
    logic signed [31:0] model_y2;
    logic signed [31:0] model_y3;

    logic signed [31:0] sample_vectors [0:7];
    logic signed [31:0] coefficient_vectors [0:7];
    logic [31:0] lfsr;
    integer failure_count;
    integer vector_count;
    integer result_file;
    integer row_index;
    integer sample_index;
    integer coefficient_index;
    integer first_latency;

    iir_3p3z_core dut (
        .clk(clk),
        .resetn(resetn),
        .start(start),
        .clear_state(clear_state),
        .sample_in(sample_in),
        .coeff_b0(coeff_b0),
        .coeff_b1(coeff_b1),
        .coeff_b2(coeff_b2),
        .coeff_b3(coeff_b3),
        .coeff_a1(coeff_a1),
        .coeff_a2(coeff_a2),
        .coeff_a3(coeff_a3),
        .limit_lower(limit_lower),
        .limit_upper(limit_upper),
        .ready(ready),
        .busy(busy),
        .done(done),
        .saturated(saturated),
        .sample_out(sample_out),
        .sample_count(sample_count),
        .history_x1(history_x1),
        .history_x2(history_x2),
        .history_x3(history_x3),
        .history_y1(history_y1),
        .history_y2(history_y2),
        .history_y3(history_y3)
    );

    always #5 clk = ~clk;

    function automatic logic signed [63:0] multiply_32x32(
        input logic signed [31:0] operand_a,
        input logic signed [31:0] operand_b
    );
        begin
            multiply_32x32 = operand_a * operand_b;
        end
    endfunction

    task automatic record_failure(input string message);
        begin
            failure_count = failure_count + 1;
            $display("FAIL: %s", message);
        end
    endtask

    task automatic clear_model_and_core;
        begin
            model_x1 = 32'sd0;
            model_x2 = 32'sd0;
            model_x3 = 32'sd0;
            model_y1 = 32'sd0;
            model_y2 = 32'sd0;
            model_y3 = 32'sd0;
            @(negedge clk);
            clear_state = 1'b1;
            @(posedge clk);
            @(negedge clk);
            clear_state = 1'b0;
            if ((sample_count !== 32'd0) || (ready !== 1'b1)) begin
                record_failure("State reset did not clear count or restore ready");
            end
        end
    endtask

    task automatic reference_step(
        input logic signed [31:0] input_sample,
        output logic signed [31:0] expected_sample,
        output logic expected_saturation
    );
        logic signed [63:0] product_b0;
        logic signed [63:0] product_b1;
        logic signed [63:0] product_b2;
        logic signed [63:0] product_b3;
        logic signed [63:0] product_a1;
        logic signed [63:0] product_a2;
        logic signed [63:0] product_a3;
        logic signed [69:0] accumulator;
        logic signed [69:0] scaled;
        logic signed [69:0] lower_extended;
        logic signed [69:0] upper_extended;
        begin
            product_b0 = multiply_32x32(input_sample, coeff_b0);
            product_b1 = multiply_32x32(model_x1, coeff_b1);
            product_b2 = multiply_32x32(model_x2, coeff_b2);
            product_b3 = multiply_32x32(model_x3, coeff_b3);
            product_a1 = multiply_32x32(model_y1, coeff_a1);
            product_a2 = multiply_32x32(model_y2, coeff_a2);
            product_a3 = multiply_32x32(model_y3, coeff_a3);

            accumulator = {{6{product_b0[63]}}, product_b0} +
                          {{6{product_b1[63]}}, product_b1} +
                          {{6{product_b2[63]}}, product_b2} +
                          {{6{product_b3[63]}}, product_b3} -
                          {{6{product_a1[63]}}, product_a1} -
                          {{6{product_a2[63]}}, product_a2} -
                          {{6{product_a3[63]}}, product_a3};
            scaled = accumulator >>> COEFF_FRAC_BITS;
            lower_extended = {{38{limit_lower[31]}}, limit_lower};
            upper_extended = {{38{limit_upper[31]}}, limit_upper};

            if (scaled > upper_extended) begin
                expected_sample = limit_upper;
                expected_saturation = 1'b1;
            end else if (scaled < lower_extended) begin
                expected_sample = limit_lower;
                expected_saturation = 1'b1;
            end else begin
                expected_sample = scaled[31:0];
                expected_saturation = 1'b0;
            end

            model_x3 = model_x2;
            model_x2 = model_x1;
            model_x1 = input_sample;
            model_y3 = model_y2;
            model_y2 = model_y1;
            model_y1 = expected_sample;
        end
    endtask

    task automatic process_and_compare(
        input string test_name,
        input logic signed [31:0] input_sample
    );
        logic signed [31:0] expected_sample;
        logic expected_saturation;
        integer completion_cycles;
        begin
            reference_step(input_sample, expected_sample,
                           expected_saturation);
            while (ready !== 1'b1) begin
                @(posedge clk);
            end

            @(negedge clk);
            sample_in = input_sample;
            start = 1'b1;
            @(posedge clk);
            @(negedge clk);
            start = 1'b0;

            completion_cycles = 1;

            if (done !== 1'b1) begin
                record_failure("IIR core did not complete in one cycle");
            end else begin
                if (first_latency < 0) begin
                    first_latency = completion_cycles;
                end else if (completion_cycles != first_latency) begin
                    record_failure("IIR completion latency changed between samples");
                end
                if (completion_cycles != EXPECTED_COMPLETION_CYCLES) begin
                    record_failure("IIR completion latency was not one cycle");
                end
                if ((ready !== 1'b1) || (busy !== 1'b0)) begin
                    record_failure("Single-cycle core was not continuously ready");
                end
                if (sample_out !== expected_sample) begin
                    failure_count = failure_count + 1;
                    $display("FAIL: %s[%0d] input=%0d expected=%0d actual=%0d",
                             test_name, row_index, input_sample,
                             expected_sample, sample_out);
                end
                if (saturated !== expected_saturation) begin
                    record_failure("Saturation flag did not match reference");
                end
                if ((history_x1 !== model_x1) ||
                    (history_x2 !== model_x2) ||
                    (history_x3 !== model_x3) ||
                    (history_y1 !== model_y1) ||
                    (history_y2 !== model_y2) ||
                    (history_y3 !== model_y3)) begin
                    record_failure("RTL history registers did not match reference");
                end
            end

            $fdisplay(result_file, "%s,%0d,%0d,%0d,%0d,%0d,%0d",
                      test_name, row_index, input_sample, expected_sample,
                      sample_out, expected_saturation, completion_cycles);
            row_index = row_index + 1;
            vector_count = vector_count + 1;
            @(posedge clk);
        end
    endtask

    initial begin
        clk = 1'b0;
        resetn = 1'b0;
        start = 1'b0;
        clear_state = 1'b0;
        sample_in = 32'sd0;
        coeff_b0 = 32'sd0;
        coeff_b1 = 32'sd0;
        coeff_b2 = 32'sd0;
        coeff_b3 = 32'sd0;
        coeff_a1 = 32'sd0;
        coeff_a2 = 32'sd0;
        coeff_a3 = 32'sd0;
        limit_lower = 32'sh8000_0000;
        limit_upper = 32'sh7FFF_FFFF;
        model_x1 = 32'sd0;
        model_x2 = 32'sd0;
        model_x3 = 32'sd0;
        model_y1 = 32'sd0;
        model_y2 = 32'sd0;
        model_y3 = 32'sd0;
        failure_count = 0;
        vector_count = 0;
        row_index = 0;
        first_latency = -1;
        lfsr = 32'h1ACE_B00C;

        sample_vectors[0] = 32'sd0;
        sample_vectors[1] = 32'sd1;
        sample_vectors[2] = -32'sd1;
        sample_vectors[3] = 32'sh7FFF_FFFF;
        sample_vectors[4] = 32'sh8000_0000;
        sample_vectors[5] = 32'sh1234_5678;
        sample_vectors[6] = 32'shFEDC_BA98;
        sample_vectors[7] = 32'sh4000_0001;

        coefficient_vectors[0] = 32'sd0;
        coefficient_vectors[1] = 32'sd1;
        coefficient_vectors[2] = -32'sd1;
        coefficient_vectors[3] = 32'sh2000_0000;
        coefficient_vectors[4] = -32'sh2000_0000;
        coefficient_vectors[5] = 32'sh7FFF_FFFF;
        coefficient_vectors[6] = 32'sh8000_0000;
        coefficient_vectors[7] = 32'sh1234_5678;

        result_file = $fopen("iir_3p3z_core_numeric_results.csv", "w");
        if (result_file == 0) begin
            $fatal(1, "Unable to open core numerical result file");
        end
        $fdisplay(result_file,
                  "case,index,input,expected,actual,saturated,latency_cycles");

        repeat (8) @(posedge clk);
        @(negedge clk);
        resetn = 1'b1;
        repeat (3) @(posedge clk);

        /* Exercise all signed high/low multiplier quadrants independently. */
        coeff_b1 = 32'sd0;
        coeff_b2 = 32'sd0;
        coeff_b3 = 32'sd0;
        coeff_a1 = 32'sd0;
        coeff_a2 = 32'sd0;
        coeff_a3 = 32'sd0;
        for (coefficient_index = 0; coefficient_index < 8;
             coefficient_index = coefficient_index + 1) begin
            coeff_b0 = coefficient_vectors[coefficient_index];
            for (sample_index = 0; sample_index < 8;
                 sample_index = sample_index + 1) begin
                clear_model_and_core();
                process_and_compare("multiplier_corner",
                                    sample_vectors[sample_index]);
            end
        end

        /* Stateful 3P3Z sequence with mixed-sign coefficients. */
        coeff_b0 = 32'sh2000_0000;
        coeff_b1 = -32'sh1800_0000;
        coeff_b2 = 32'sh1000_0000;
        coeff_b3 = -32'sh0800_0000;
        coeff_a1 = 32'sh1000_0000;
        coeff_a2 = -32'sh0800_0000;
        coeff_a3 = 32'sh0400_0000;
        clear_model_and_core();
        for (sample_index = 0; sample_index < 256;
             sample_index = sample_index + 1) begin
            lfsr = {lfsr[30:0],
                    lfsr[31] ^ lfsr[21] ^ lfsr[1] ^ lfsr[0]};
            process_and_compare("continuous_prbs",
                                $signed({{11{lfsr[20]}}, lfsr[20:0]}));
        end

        /* Explicit positive and negative output saturation. */
        coeff_b0 = 32'sh7FFF_FFFF;
        coeff_b1 = 32'sd0;
        coeff_b2 = 32'sd0;
        coeff_b3 = 32'sd0;
        coeff_a1 = 32'sd0;
        coeff_a2 = 32'sd0;
        coeff_a3 = 32'sd0;
        clear_model_and_core();
        process_and_compare("positive_saturation", 32'sh7FFF_FFFF);
        clear_model_and_core();
        process_and_compare("negative_saturation", 32'sh8000_0000);

        /*
         * Verify programmable limiting and prove that the limited result,
         * rather than the raw accumulator result, becomes y[n-1].
         */
        coeff_b0 = 32'sh4000_0000;
        coeff_b1 = 32'sd0;
        coeff_b2 = 32'sd0;
        coeff_b3 = 32'sd0;
        coeff_a1 = -32'sh4000_0000;
        coeff_a2 = 32'sd0;
        coeff_a3 = 32'sd0;
        limit_lower = -32'sd100;
        limit_upper = 32'sd100;
        clear_model_and_core();
        process_and_compare("configured_upper_limit", 32'sd1000);
        if (history_y1 !== 32'sd100) begin
            record_failure("Upper-limited result was not stored in y[n-1]");
        end
        limit_lower = 32'sh8000_0000;
        limit_upper = 32'sh7FFF_FFFF;
        process_and_compare("limited_feedback_history", 32'sd0);
        if (sample_out !== 32'sd100) begin
            record_failure("Feedback did not use limited y[n-1]");
        end

        coeff_a1 = 32'sd0;
        limit_lower = -32'sd75;
        limit_upper = 32'sd125;
        clear_model_and_core();
        process_and_compare("configured_lower_limit", -32'sd1000);
        if (history_y1 !== -32'sd75) begin
            record_failure("Lower-limited result was not stored in y[n-1]");
        end

        /*
         * Hold start high and change the input once per cycle. With b0=1 and
         * all history coefficients zero, every rising edge must retire one
         * independent sample.
         */
        limit_lower = 32'sh8000_0000;
        limit_upper = 32'sh7FFF_FFFF;
        clear_model_and_core();
        @(negedge clk);
        start = 1'b1;
        for (sample_index = 1; sample_index <= 8;
             sample_index = sample_index + 1) begin
            sample_in = sample_index * 32'sd17;
            @(posedge clk);
            @(negedge clk);
            if ((done !== 1'b1) ||
                (sample_out !== (sample_index * 32'sd17))) begin
                record_failure("One-sample-per-clock throughput failed");
            end
        end
        start = 1'b0;
        if (sample_count !== 32'd8) begin
            record_failure("Continuous start did not retire every sample");
        end

        $fclose(result_file);
        if (failure_count == 0) begin
            $display("CORE_SIM_RESULT PASS vectors=%0d latency=%0d failures=0",
                     vector_count, first_latency);
        end else begin
            $display("CORE_SIM_RESULT FAIL vectors=%0d failures=%0d",
                     vector_count, failure_count);
            $fatal(1, "Standalone 3P3Z core simulation failed");
        end

        repeat (5) @(posedge clk);
        $finish;
    end

endmodule
