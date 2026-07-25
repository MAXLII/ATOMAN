`timescale 1 ns / 1 ps

module tb_axi_oled_dma;

  localparam logic [31:0] FRAME_BASE = 32'h1FF2_0000;

  logic clk = 1'b0;
  logic rst_n = 1'b0;

  logic [7:0] s_axi_awaddr = 8'd0;
  logic [2:0] s_axi_awprot = 3'd0;
  logic s_axi_awvalid = 1'b0;
  wire s_axi_awready;
  logic [31:0] s_axi_wdata = 32'd0;
  logic [3:0] s_axi_wstrb = 4'd0;
  logic s_axi_wvalid = 1'b0;
  wire s_axi_wready;
  wire [1:0] s_axi_bresp;
  wire s_axi_bvalid;
  logic s_axi_bready = 1'b1;
  logic [7:0] s_axi_araddr = 8'd0;
  logic [2:0] s_axi_arprot = 3'd0;
  logic s_axi_arvalid = 1'b0;
  wire s_axi_arready;
  wire [31:0] s_axi_rdata;
  wire [1:0] s_axi_rresp;
  wire s_axi_rvalid;
  logic s_axi_rready = 1'b1;

  wire [31:0] m_axi_awaddr;
  wire [7:0] m_axi_awlen;
  wire [2:0] m_axi_awsize;
  wire [1:0] m_axi_awburst;
  wire m_axi_awvalid;
  logic m_axi_awready = 1'b0;
  wire [31:0] m_axi_wdata;
  wire [3:0] m_axi_wstrb;
  wire m_axi_wlast;
  wire m_axi_wvalid;
  logic m_axi_wready = 1'b0;
  logic [1:0] m_axi_bresp = 2'b00;
  logic m_axi_bvalid = 1'b0;
  wire m_axi_bready;
  wire [31:0] m_axi_araddr;
  wire [7:0] m_axi_arlen;
  wire [2:0] m_axi_arsize;
  wire [1:0] m_axi_arburst;
  wire m_axi_arvalid;
  logic m_axi_arready = 1'b1;
  logic [31:0] m_axi_rdata = 32'd0;
  logic [1:0] m_axi_rresp = 2'b00;
  logic m_axi_rlast = 1'b0;
  logic m_axi_rvalid = 1'b0;
  wire m_axi_rready;

  wire oled_clock;
  wire oled_data;
  wire oled_dc;
  wire oled_reset_n;
  wire irq;

  logic [31:0] memory_words [0:255];
  logic [7:0] expected_bytes [0:1023];
  logic [7:0] command_bytes [0:255];
  logic [7:0] display_bytes [0:4095];
  integer command_count = 0;
  integer display_count = 0;
  logic [7:0] serial_shift = 8'd0;
  integer serial_bit_count = 0;

  logic read_active = 1'b0;
  logic [31:0] read_address = 32'd0;
  integer read_beat = 0;
  integer read_stall = 0;
  integer ar_count = 0;
  logic inject_axi_error = 1'b0;

  always #10 clk = ~clk;

  axi_oled_dma #(
      .RESET_LOW_CYCLES(4),
      .RESET_HIGH_CYCLES(4)
  ) dut (
      .aclk(clk),
      .aresetn(rst_n),
      .s_axi_awaddr(s_axi_awaddr),
      .s_axi_awprot(s_axi_awprot),
      .s_axi_awvalid(s_axi_awvalid),
      .s_axi_awready(s_axi_awready),
      .s_axi_wdata(s_axi_wdata),
      .s_axi_wstrb(s_axi_wstrb),
      .s_axi_wvalid(s_axi_wvalid),
      .s_axi_wready(s_axi_wready),
      .s_axi_bresp(s_axi_bresp),
      .s_axi_bvalid(s_axi_bvalid),
      .s_axi_bready(s_axi_bready),
      .s_axi_araddr(s_axi_araddr),
      .s_axi_arprot(s_axi_arprot),
      .s_axi_arvalid(s_axi_arvalid),
      .s_axi_arready(s_axi_arready),
      .s_axi_rdata(s_axi_rdata),
      .s_axi_rresp(s_axi_rresp),
      .s_axi_rvalid(s_axi_rvalid),
      .s_axi_rready(s_axi_rready),
      .m_axi_awaddr(m_axi_awaddr),
      .m_axi_awlen(m_axi_awlen),
      .m_axi_awsize(m_axi_awsize),
      .m_axi_awburst(m_axi_awburst),
      .m_axi_awvalid(m_axi_awvalid),
      .m_axi_awready(m_axi_awready),
      .m_axi_wdata(m_axi_wdata),
      .m_axi_wstrb(m_axi_wstrb),
      .m_axi_wlast(m_axi_wlast),
      .m_axi_wvalid(m_axi_wvalid),
      .m_axi_wready(m_axi_wready),
      .m_axi_bresp(m_axi_bresp),
      .m_axi_bvalid(m_axi_bvalid),
      .m_axi_bready(m_axi_bready),
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
      .m_axi_rready(m_axi_rready),
      .oled_clock(oled_clock),
      .oled_data(oled_data),
      .oled_dc(oled_dc),
      .oled_reset_n(oled_reset_n),
      .irq(irq)
  );

  always @(posedge clk) begin
    if (!rst_n) begin
      read_active <= 1'b0;
      m_axi_rvalid <= 1'b0;
      m_axi_rlast <= 1'b0;
      m_axi_rresp <= 2'b00;
      read_beat <= 0;
      read_stall <= 0;
      ar_count <= 0;
    end else begin
      if (m_axi_arvalid && m_axi_arready && !read_active) begin
        if ((m_axi_arlen !== 8'd15) ||
            (m_axi_arsize !== 3'd2) ||
            (m_axi_arburst !== 2'b01)) begin
          $fatal(1, "Invalid AXI burst attributes");
        end
        read_active <= 1'b1;
        read_address <= m_axi_araddr;
        read_beat <= 0;
        read_stall <= ar_count % 3;
        ar_count <= ar_count + 1;
      end

      if (m_axi_rvalid && m_axi_rready) begin
        m_axi_rvalid <= 1'b0;
        if (m_axi_rlast) begin
          read_active <= 1'b0;
          m_axi_rlast <= 1'b0;
        end else begin
          read_beat <= read_beat + 1;
          read_stall <= (read_beat + ar_count) % 3;
        end
      end else if (read_active && !m_axi_rvalid) begin
        if (read_stall != 0) begin
          read_stall <= read_stall - 1;
        end else begin
          m_axi_rdata <= memory_words[
              ((read_address - FRAME_BASE) >> 2) + read_beat];
          m_axi_rresp <=
              (inject_axi_error && (read_beat == 0)) ? 2'b10 : 2'b00;
          m_axi_rlast <= (read_beat == 15);
          m_axi_rvalid <= 1'b1;
          if (inject_axi_error && (read_beat == 0)) begin
            inject_axi_error <= 1'b0;
          end
        end
      end
    end
  end

  always @(posedge oled_clock) begin
    serial_shift = {serial_shift[6:0], oled_data};
    if (serial_bit_count == 7) begin
      if (oled_dc) begin
        display_bytes[display_count] = serial_shift;
        display_count = display_count + 1;
      end else begin
        command_bytes[command_count] = serial_shift;
        command_count = command_count + 1;
      end
      serial_bit_count = 0;
    end else begin
      serial_bit_count = serial_bit_count + 1;
    end
  end

  task automatic axi_write(
      input logic [7:0] address,
      input logic [31:0] data,
      input logic [3:0] strobe,
      input bit address_first
  );
    begin
      if (address_first) begin
        @(negedge clk);
        s_axi_awaddr = address;
        s_axi_awvalid = 1'b1;
        do @(posedge clk); while (!s_axi_awready);
        @(negedge clk);
        s_axi_awvalid = 1'b0;
        repeat (2) @(posedge clk);
        @(negedge clk);
        s_axi_wdata = data;
        s_axi_wstrb = strobe;
        s_axi_wvalid = 1'b1;
        do @(posedge clk); while (!s_axi_wready);
        @(negedge clk);
        s_axi_wvalid = 1'b0;
      end else begin
        @(negedge clk);
        s_axi_wdata = data;
        s_axi_wstrb = strobe;
        s_axi_wvalid = 1'b1;
        do @(posedge clk); while (!s_axi_wready);
        @(negedge clk);
        s_axi_wvalid = 1'b0;
        repeat (2) @(posedge clk);
        @(negedge clk);
        s_axi_awaddr = address;
        s_axi_awvalid = 1'b1;
        do @(posedge clk); while (!s_axi_awready);
        @(negedge clk);
        s_axi_awvalid = 1'b0;
      end
      do @(posedge clk); while (!s_axi_bvalid);
      if (s_axi_bresp !== 2'b00) begin
        $fatal(1, "AXI-Lite write response error");
      end
    end
  endtask

  task automatic axi_read(
      input logic [7:0] address,
      output logic [31:0] data
  );
    begin
      @(negedge clk);
      s_axi_araddr = address;
      s_axi_arvalid = 1'b1;
      do @(posedge clk); while (!s_axi_arready);
      @(negedge clk);
      s_axi_arvalid = 1'b0;
      do @(posedge clk); while (!s_axi_rvalid);
      data = s_axi_rdata;
      if (s_axi_rresp !== 2'b00) begin
        $fatal(1, "AXI-Lite read response error");
      end
    end
  endtask

  task automatic wait_register(
      input logic [7:0] address,
      input logic [31:0] mask,
      input logic [31:0] expected,
      input integer limit
  );
    logic [31:0] value;
    integer attempt;
    begin : wait_loop
      value = 32'd0;
      for (attempt = 0; attempt < limit; attempt = attempt + 1) begin
        axi_read(address, value);
        if ((value & mask) == expected) begin
          disable wait_loop;
        end
      end
      $fatal(1, "Register timeout address=%02x value=%08x", address, value);
    end
  endtask

  integer index;
  integer data_start;
  integer burst_start;
  logic [31:0] value;

  initial begin
    for (index = 0; index < 1024; index = index + 1) begin
      expected_bytes[index] = (index * 37 + 11) & 8'hFF;
    end
    for (index = 0; index < 256; index = index + 1) begin
      memory_words[index] = {
          expected_bytes[index*4 + 3],
          expected_bytes[index*4 + 2],
          expected_bytes[index*4 + 1],
          expected_bytes[index*4]
      };
    end

    repeat (5) @(posedge clk);
    rst_n = 1'b1;

    axi_read(8'h34, value);
    if (value !== 32'h0001_0000) $fatal(1, "Version mismatch");
    axi_read(8'h38, value);
    if (value !== 32'h0040_0080) $fatal(1, "Geometry mismatch");
    axi_read(8'h3C, value);
    if (value !== 32'd1024) $fatal(1, "Frame size mismatch");
    axi_write(8'h0C, 32'hFFFF_FF06, 4'h1, 1'b0);
    axi_read(8'h0C, value);
    if (value !== 32'd6) $fatal(1, "WSTRB register update mismatch");
    axi_write(8'h0C, 32'd5, 4'hF, 1'b1);

    axi_write(8'h00, 32'h0000_0021, 4'hF, 1'b1);
    wait_register(8'h04, 32'h0000_0002, 32'h0000_0002, 20000);
    if (command_count != 26) begin
      $fatal(1, "Initialization command count=%0d", command_count);
    end
    if ((command_bytes[0] !== 8'hAE) ||
        (command_bytes[25] !== 8'hAF)) begin
      $fatal(1, "Initialization command sequence mismatch first=%02x last=%02x",
             command_bytes[0], command_bytes[25]);
    end

    data_start = display_count;
    burst_start = ar_count;
    axi_write(8'h00, 32'h0000_0029, 4'hF, 1'b0);
    wait_register(8'h20, 32'hFFFF_FFFF, 32'd1, 200000);
    if ((ar_count - burst_start) != 16) begin
      $fatal(1, "Expected 16 DMA bursts, got %0d", ar_count - burst_start);
    end
    if ((display_count - data_start) != 1024) begin
      $fatal(1, "Expected 1024 display bytes, got %0d",
             display_count - data_start);
    end
    for (index = 0; index < 1024; index = index + 1) begin
      if (display_bytes[data_start + index] !== expected_bytes[index]) begin
        $fatal(1, "Framebuffer mismatch index=%0d actual=%02x expected=%02x",
               index, display_bytes[data_start + index], expected_bytes[index]);
      end
    end

    axi_read(8'h18, value);
    if ((value !== 32'd0) || irq) begin
      $fatal(1, "Normal path raised IRQ status=%08x irq=%0d", value, irq);
    end

    axi_write(8'h10, 32'd1000, 4'hF, 1'b0);
    axi_write(8'h00, 32'h0000_00A1, 4'hF, 1'b1);
    wait_register(8'h20, 32'hFFFF_FFFF, 32'd2, 200000);
    axi_write(8'h00, 32'h0000_0021, 4'hF, 1'b0);
    wait_register(8'h04, 32'h0000_001C, 32'd0, 200000);

    data_start = display_count;
    axi_write(8'h00, 32'h0000_0031, 4'hF, 1'b1);
    wait_register(8'h24, 32'hFFFF_FFFF, 32'd1, 200000);
    if ((display_count - data_start) != 1024) begin
      $fatal(1, "Clear frame byte count mismatch");
    end
    for (index = 0; index < 1024; index = index + 1) begin
      if (display_bytes[data_start + index] !== 8'h00) begin
        $fatal(1, "Clear data was not zero at index=%0d", index);
      end
    end

    axi_write(8'h08, 32'h1FF2_0004, 4'hF, 1'b0);
    wait_register(8'h18, 32'h0000_0001, 32'h0000_0001, 20);
    axi_write(8'h18, 32'h0000_0001, 4'hF, 1'b1);
    axi_write(8'h08, FRAME_BASE, 4'hF, 1'b0);

    inject_axi_error = 1'b1;
    axi_write(8'h00, 32'h0000_0029, 4'hF, 1'b1);
    wait_register(8'h18, 32'h0000_0002, 32'h0000_0002, 20000);
    axi_read(8'h28, value);
    if (value !== 32'd1) $fatal(1, "AXI error count mismatch");

    $display("TB_AXI_OLED_DMA result=PASS clear=1 serial_bytes=%0d",
             display_count);
    $finish;
  end

  initial begin
    #20000000;
    $fatal(1, "Simulation timeout");
  end

endmodule
