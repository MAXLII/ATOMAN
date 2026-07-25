`timescale 1 ns / 1 ps
`default_nettype none

module tb_axi_uart_dma;

    parameter integer TRANSFER_BYTES = 1048576;
    localparam [31:0] RX_BASE = 32'h0000_1000;
    localparam [31:0] TX_BASE = 32'h0000_2000;
    localparam integer RING_SIZE = 256;
    localparam integer CHUNK_SIZE = 128;

    reg clk = 1'b0;
    reg resetn = 1'b0;
    integer cycle_count = 0;
    integer failures = 0;
    integer sent_count = 0;
    integer received_count = 0;
    integer serial_tx_accept_count = 0;
    integer serial_rx_valid_count = 0;
    integer chunk;
    integer index;
    integer error_wait;

    reg [7:0] memory [0:12287];

    reg [7:0] s_awaddr = 0;
    reg [2:0] s_awprot = 0;
    reg s_awvalid = 0;
    wire s_awready;
    reg [31:0] s_wdata = 0;
    reg [3:0] s_wstrb = 0;
    reg s_wvalid = 0;
    wire s_wready;
    wire [1:0] s_bresp;
    wire s_bvalid;
    reg s_bready = 1;
    reg [7:0] s_araddr = 0;
    reg [2:0] s_arprot = 0;
    reg s_arvalid = 0;
    wire s_arready;
    wire [31:0] s_rdata;
    wire [1:0] s_rresp;
    wire s_rvalid;
    reg s_rready = 1;

    wire [31:0] m_awaddr;
    wire [7:0] m_awlen;
    wire [2:0] m_awsize;
    wire [1:0] m_awburst;
    wire m_awvalid;
    reg m_awready = 0;
    wire [31:0] m_wdata;
    wire [3:0] m_wstrb;
    wire m_wlast;
    wire m_wvalid;
    reg m_wready = 0;
    reg [1:0] m_bresp = 0;
    reg m_bvalid = 0;
    wire m_bready;
    wire [31:0] m_araddr;
    wire [7:0] m_arlen;
    wire [2:0] m_arsize;
    wire [1:0] m_arburst;
    wire m_arvalid;
    reg m_arready = 0;
    reg [31:0] m_rdata = 0;
    reg [1:0] m_rresp = 0;
    reg m_rlast = 1;
    reg m_rvalid = 0;
    wire m_rready;

    reg uart_rxd = 1;
    wire uart_txd;
    wire irq;

    reg aw_seen = 0;
    reg w_seen = 0;
    reg [31:0] captured_awaddr = 0;
    reg [31:0] captured_wdata = 0;
    reg [3:0] captured_wstrb = 0;
    reg force_read_error = 0;
    reg force_write_error = 0;
    reg [31:0] read_value;

    always #10 clk = ~clk;

    function automatic [7:0] pattern_byte(input integer position);
        pattern_byte = ((position * 37) + 8'h5A) & 8'hFF;
    endfunction

    task automatic axi_lite_write(
        input [7:0] address,
        input [31:0] value,
        input integer order_mode
    );
        reg aw_done;
        reg w_done;
        begin
            @(negedge clk);
            s_awaddr = address;
            s_wdata = value;
            s_wstrb = 4'hF;
            aw_done = 0;
            w_done = 0;
            if (order_mode == 1) begin
                s_awvalid = 1;
                do @(posedge clk); while (!s_awready);
                @(negedge clk);
                s_awvalid = 0;
                repeat (2) @(posedge clk);
                @(negedge clk);
                s_wvalid = 1;
                do @(posedge clk); while (!s_wready);
                @(negedge clk);
                s_wvalid = 0;
            end else if (order_mode == 2) begin
                s_wvalid = 1;
                do @(posedge clk); while (!s_wready);
                @(negedge clk);
                s_wvalid = 0;
                if (TRANSFER_BYTES <= 256) begin
                    $display("DMA_DEBUG w_first_data_accepted address=%02x", address);
                    $fflush();
                end
                repeat (2) @(posedge clk);
                @(negedge clk);
                s_awvalid = 1;
                do @(posedge clk); while (!s_awready);
                @(negedge clk);
                s_awvalid = 0;
                if (TRANSFER_BYTES <= 256) begin
                    $display("DMA_DEBUG w_first_address_accepted address=%02x", address);
                    $fflush();
                end
            end else begin
                s_awvalid = 1;
                s_wvalid = 1;
                while (!aw_done || !w_done) begin
                    @(posedge clk);
                    if (s_awvalid && s_awready) begin
                        aw_done = 1;
                    end
                    if (s_wvalid && s_wready) begin
                        w_done = 1;
                    end
                    @(negedge clk);
                    if (aw_done) s_awvalid = 0;
                    if (w_done) s_wvalid = 0;
                end
            end
            while (!s_bvalid) @(posedge clk);
            if ((order_mode == 2) && (TRANSFER_BYTES <= 256)) begin
                $display("DMA_DEBUG w_first_response address=%02x", address);
                $fflush();
            end
            @(posedge clk);
        end
    endtask

    task automatic axi_lite_read(input [7:0] address, output [31:0] value);
        begin
            @(negedge clk);
            s_araddr = address;
            s_arvalid = 1;
            do @(posedge clk); while (!s_arready);
            @(negedge clk);
            s_arvalid = 0;
            while (!s_rvalid) @(posedge clk);
            value = s_rdata;
            @(posedge clk);
        end
    endtask

    axi_uart_dma #(
        .UART_OVERSAMPLE(2)
    ) dut (
        .aclk(clk), .aresetn(resetn),
        .s_axi_awaddr(s_awaddr), .s_axi_awprot(s_awprot),
        .s_axi_awvalid(s_awvalid), .s_axi_awready(s_awready),
        .s_axi_wdata(s_wdata), .s_axi_wstrb(s_wstrb),
        .s_axi_wvalid(s_wvalid), .s_axi_wready(s_wready),
        .s_axi_bresp(s_bresp), .s_axi_bvalid(s_bvalid),
        .s_axi_bready(s_bready), .s_axi_araddr(s_araddr),
        .s_axi_arprot(s_arprot), .s_axi_arvalid(s_arvalid),
        .s_axi_arready(s_arready), .s_axi_rdata(s_rdata),
        .s_axi_rresp(s_rresp), .s_axi_rvalid(s_rvalid),
        .s_axi_rready(s_rready),
        .m_axi_awaddr(m_awaddr), .m_axi_awlen(m_awlen),
        .m_axi_awsize(m_awsize), .m_axi_awburst(m_awburst),
        .m_axi_awvalid(m_awvalid), .m_axi_awready(m_awready),
        .m_axi_wdata(m_wdata), .m_axi_wstrb(m_wstrb),
        .m_axi_wlast(m_wlast), .m_axi_wvalid(m_wvalid),
        .m_axi_wready(m_wready), .m_axi_bresp(m_bresp),
        .m_axi_bvalid(m_bvalid), .m_axi_bready(m_bready),
        .m_axi_araddr(m_araddr), .m_axi_arlen(m_arlen),
        .m_axi_arsize(m_arsize), .m_axi_arburst(m_arburst),
        .m_axi_arvalid(m_arvalid), .m_axi_arready(m_arready),
        .m_axi_rdata(m_rdata), .m_axi_rresp(m_rresp),
        .m_axi_rlast(m_rlast), .m_axi_rvalid(m_rvalid),
        .m_axi_rready(m_rready),
        .uart_rxd(uart_rxd), .uart_txd(uart_txd), .irq(irq)
    );

    always @(posedge clk) begin
        cycle_count <= cycle_count + 1;
        if (dut.serial_tx_ready && !dut.tx_fifo_empty)
            serial_tx_accept_count <= serial_tx_accept_count + 1;
        if (dut.serial_rx_valid)
            serial_rx_valid_count <= serial_rx_valid_count + 1;
        m_awready <= cycle_count[0];
        m_wready <= cycle_count[1];
        m_arready <= cycle_count[0] && !m_rvalid;

        if (m_awvalid && m_awready) begin
            captured_awaddr <= m_awaddr;
            aw_seen <= 1;
        end
        if (m_wvalid && m_wready) begin
            captured_wdata <= m_wdata;
            captured_wstrb <= m_wstrb;
            w_seen <= 1;
        end
        if (aw_seen && w_seen && !m_bvalid) begin
            for (index = 0; index < 4; index = index + 1) begin
                if (captured_wstrb[index])
                    memory[captured_awaddr + index] <=
                        captured_wdata[(index*8) +: 8];
            end
            m_bresp <= force_write_error ? 2'b10 : 2'b00;
            m_bvalid <= 1;
            aw_seen <= 0;
            w_seen <= 0;
            force_write_error <= 0;
        end
        if (m_bvalid && m_bready) m_bvalid <= 0;

        if (m_arvalid && m_arready) begin
            m_rdata <= {memory[m_araddr+3], memory[m_araddr+2],
                        memory[m_araddr+1], memory[m_araddr]};
            m_rresp <= force_read_error ? 2'b10 : 2'b00;
            m_rvalid <= 1;
            force_read_error <= 0;
        end
        if (m_rvalid && m_rready) m_rvalid <= 0;
    end

    initial begin
        for (index = 0; index < 12288; index = index + 1) memory[index] = 0;
        repeat (8) @(posedge clk);
        resetn <= 1;

        axi_lite_write(8'h0C, 32'hFFFF_FFFF, 0);
        axi_lite_write(8'h00, 32'h0000_0005, 1);

        while (sent_count < TRANSFER_BYTES) begin
            chunk = TRANSFER_BYTES - sent_count;
            if (chunk > CHUNK_SIZE) chunk = CHUNK_SIZE;
            for (index = 0; index < chunk; index = index + 1)
                memory[TX_BASE + ((sent_count + index) & (RING_SIZE-1))] =
                    pattern_byte(sent_count + index);
            sent_count = sent_count + chunk;
            axi_lite_write(8'h28, sent_count, 0);
            if (TRANSFER_BYTES <= 256)
                $display("DMA_DEBUG producer=%0d tx_consumed=%0d rx_produced=%0d",
                         sent_count, dut.tx_consumed, dut.rx_produced);
            if (TRANSFER_BYTES <= 256) $fflush();

            while ((dut.rx_produced - received_count) < chunk) @(posedge clk);
            if (TRANSFER_BYTES <= 256)
                $display("DMA_DEBUG received_target=%0d tx_consumed=%0d rx_produced=%0d",
                         received_count + chunk, dut.tx_consumed, dut.rx_produced);
            if (TRANSFER_BYTES <= 256) $fflush();
            for (index = 0; index < chunk; index = index + 1) begin
                if (memory[RX_BASE + ((received_count + index) & (RING_SIZE-1))] !==
                    pattern_byte(received_count + index)) begin
                    failures = failures + 1;
                    if (failures < 8)
                        $display("DMA_MISMATCH index=%0d expected=%02x actual=%02x",
                                 received_count + index,
                                 pattern_byte(received_count + index),
                                 memory[RX_BASE + ((received_count + index) &
                                                  (RING_SIZE-1))]);
                end
            end
            received_count = received_count + chunk;
            axi_lite_write(8'h1C, received_count, 0);
            if (TRANSFER_BYTES <= 256)
                $display("DMA_DEBUG consumed=%0d tx_consumed=%0d rx_produced=%0d",
                         received_count, dut.tx_consumed, dut.rx_produced);
            if (TRANSFER_BYTES <= 256) $fflush();
            if (irq) begin
                $display("DMA_UNEXPECTED_IRQ sent=%0d received=%0d status=%08x",
                         sent_count, received_count, dut.irq_status);
                failures = failures + 1;
            end
        end

        axi_lite_read(8'h18, read_value);
        if (read_value != TRANSFER_BYTES) failures = failures + 1;
        axi_lite_read(8'h2C, read_value);
        if (read_value != TRANSFER_BYTES) failures = failures + 1;

        axi_lite_write(8'h00, 32'h0000_0000, 0);
        axi_lite_write(8'h14, 32'd300, 0);
        axi_lite_write(8'h00, 32'h0000_0001, 0);
        repeat (10) @(posedge clk);
        if (!irq || !dut.irq_status[5]) begin
            $display("DMA_INVALID_CONFIG_IRQ_FAIL");
            failures = failures + 1;
        end
        axi_lite_write(8'h30, 32'hFFFF_FFFF, 0);

        axi_lite_write(8'h00, 32'h0000_0002, 0);
        repeat (4) @(posedge clk);
        axi_lite_write(8'h14, 32'd256, 2);
        memory[TX_BASE] = 8'hA6;
        axi_lite_write(8'h28, 32'd1, 0);
        force_read_error = 1;
        axi_lite_write(8'h00, 32'h0000_0005, 0);
        error_wait = 0;
        while (!dut.irq_status[3] && (error_wait < 1000)) begin
            @(posedge clk);
            error_wait = error_wait + 1;
        end
        if (!irq || !dut.irq_status[3] || !dut.tx_dma_halted) begin
            $display("DMA_AXI_READ_ERROR_IRQ_FAIL status=%08x", dut.irq_status);
            failures = failures + 1;
        end

        axi_lite_write(8'h00, 32'h0000_0002, 0);
        repeat (4) @(posedge clk);
        memory[TX_BASE] = 8'h3C;
        axi_lite_write(8'h28, 32'd1, 0);
        force_write_error = 1;
        axi_lite_write(8'h00, 32'h0000_0005, 0);
        error_wait = 0;
        while (!dut.irq_status[4] && (error_wait < 4000)) begin
            @(posedge clk);
            error_wait = error_wait + 1;
        end
        if (!irq || !dut.irq_status[4] || !dut.rx_dma_halted) begin
            $display("DMA_AXI_WRITE_ERROR_IRQ_FAIL status=%08x", dut.irq_status);
            failures = failures + 1;
        end

        if (failures == 0) begin
            $display("UART_DMA_AXI_SIM_RESULT PASS bytes=%0d wraps=%0d normal_irq=0 error_irq=PASS failures=0",
                     TRANSFER_BYTES, TRANSFER_BYTES / RING_SIZE);
            $finish;
        end
        $fatal(1, "UART_DMA_AXI_SIM_RESULT FAIL bytes=%0d failures=%0d",
               TRANSFER_BYTES, failures);
    end

    initial begin
        #6000000000;
        $fatal(1, "UART_DMA_AXI_SIM_RESULT TIMEOUT sent=%0d received=%0d txc=%0d rxp=%0d tx_accept=%0d rx_valid=%0d tx_fifo=%0d rx_fifo=%0d tx_state=%0d rx_state=%0d parity=%0d frame=%0d overflow=%0d",
               sent_count, received_count, dut.tx_consumed, dut.rx_produced,
               serial_tx_accept_count, serial_rx_valid_count,
               dut.tx_fifo_count, dut.rx_fifo_count,
               dut.serial_core.tx_state, dut.serial_core.rx_state,
               dut.parity_error_count, dut.frame_error_count,
               dut.overflow_error_count);
    end

endmodule

`default_nettype wire
