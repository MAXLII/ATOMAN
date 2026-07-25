`timescale 1 ns / 1 ps

module tb_ssd1306_protocol;

  logic clk = 1'b0;
  logic rst_n = 1'b0;
  logic [2:0] operation = 3'd0;
  logic [7:0] operation_value = 8'd0;
  logic operation_valid = 1'b0;
  wire operation_ready;
  wire operation_done;
  wire protocol_error;
  logic [7:0] frame_data = 8'd0;
  logic frame_data_valid = 1'b0;
  logic frame_data_last = 1'b0;
  wire frame_data_ready;
  wire [7:0] phy_byte_data;
  wire phy_byte_is_data;
  wire phy_byte_valid;
  logic phy_byte_ready = 1'b1;
  logic phy_busy = 1'b0;
  wire oled_reset_n;
  wire busy;

  integer frame_index = 0;
  integer frame_number = 0;
  integer total_frame_bytes = 0;
  integer command_count = 0;
  logic clear_mode = 1'b0;
  logic frame_mode = 1'b0;

  always #10 clk = ~clk;

  ssd1306_protocol #(
      .RESET_LOW_CYCLES(3),
      .RESET_HIGH_CYCLES(3)
  ) dut (
      .clk(clk),
      .rst_n(rst_n),
      .operation(operation),
      .operation_value(operation_value),
      .operation_valid(operation_valid),
      .operation_ready(operation_ready),
      .operation_done(operation_done),
      .protocol_error(protocol_error),
      .frame_data(frame_data),
      .frame_data_valid(frame_data_valid),
      .frame_data_last(frame_data_last),
      .frame_data_ready(frame_data_ready),
      .phy_byte_data(phy_byte_data),
      .phy_byte_is_data(phy_byte_is_data),
      .phy_byte_valid(phy_byte_valid),
      .phy_byte_ready(phy_byte_ready),
      .phy_busy(phy_busy),
      .oled_reset_n(oled_reset_n),
      .busy(busy)
  );

  always @* begin
    frame_data = (frame_number + frame_index * 37) & 8'hFF;
    frame_data_valid = frame_mode;
    frame_data_last = (frame_index == 1023);
  end

  always @(posedge clk) begin
    if (rst_n && phy_byte_valid && phy_byte_ready) begin
      if (phy_byte_is_data) begin
        if (clear_mode) begin
          if (phy_byte_data !== 8'h00) begin
            $fatal(1, "Clear protocol emitted nonzero data");
          end
        end else if (frame_mode) begin
          if (phy_byte_data !==
              ((frame_number + frame_index * 37) & 8'hFF)) begin
            $fatal(1, "Protocol frame mismatch frame=%0d index=%0d",
                   frame_number, frame_index);
          end
          total_frame_bytes <= total_frame_bytes + 1;
        end
      end else begin
        command_count <= command_count + 1;
      end

      if (frame_data_ready && frame_mode) begin
        if (frame_index == 1023) begin
          frame_index <= 0;
        end else begin
          frame_index <= frame_index + 1;
        end
      end
    end
  end

  task automatic execute_operation(
      input logic [2:0] requested_operation,
      input logic [7:0] requested_value
  );
    begin
      @(negedge clk);
      operation = requested_operation;
      operation_value = requested_value;
      operation_valid = 1'b1;
      do @(posedge clk); while (!operation_ready);
      @(negedge clk);
      operation_valid = 1'b0;
      do @(posedge clk); while (!operation_done);
      if (protocol_error) begin
        $fatal(1, "Unexpected protocol error operation=%0d",
               requested_operation);
      end
    end
  endtask

  integer test_frame;

  initial begin
    repeat (4) @(posedge clk);
    rst_n = 1'b1;

    execute_operation(3'd0, 8'hCF);
    if (command_count != 26) begin
      $fatal(1, "Initialization command count=%0d", command_count);
    end

    execute_operation(3'd3, 8'd0);
    execute_operation(3'd3, 8'd1);
    execute_operation(3'd4, 8'd1);
    execute_operation(3'd4, 8'd0);
    execute_operation(3'd5, 8'h7F);

    clear_mode = 1'b1;
    execute_operation(3'd2, 8'd0);
    clear_mode = 1'b0;

    frame_mode = 1'b1;
    for (test_frame = 0; test_frame < 1024; test_frame = test_frame + 1) begin
      frame_number = test_frame;
      frame_index = 0;
      execute_operation(3'd1, 8'd0);
    end
    frame_mode = 1'b0;

    if (total_frame_bytes != 1048576) begin
      $fatal(1, "Expected 1 MiB frame data, got %0d", total_frame_bytes);
    end

    $display("TB_SSD1306_PROTOCOL result=PASS frames=1024 bytes=%0d",
             total_frame_bytes);
    $finish;
  end

  initial begin
    #100000000;
    $fatal(1, "Protocol simulation timeout");
  end

endmodule
