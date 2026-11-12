// The whole updatable design on a simulated CAN bus, the same test as
// hdl/frames_tb.v -DWITH_IPS for the hardcoded design: real dataset frames bit
// by bit, a wired-AND bus, the IPS on, a second receiver as the ESP32 logger.
// The model comes from the boot ROM. The log has the format python/compare_sim.py
// reads.
//
//   vvp -n core_frames_sim +log=got_ips.csv   (run in the folder with bus_bits.mem)
//
// Swap test (-DSWAP_TEST): the same bus with the IPS off, and in the middle of
// the traffic a new model goes in over the update port. Each verdict is logged
// with its bank, so python/check_swap.py can prove no frame was lost or mixed.
`timescale 1ns/1ps
`include "test_key.vh"
module core_frames_tb;
    `include "frames.vh"

    reg clk = 0;
    always #10 clk = ~clk;

    reg  rst = 1, por = 1, rxd = 1, sclk = 0, cs_n = 1, mosi = 0, maint = 0;
    wire miso, tx_dominant, frame_valid, feat_ready, verdict_valid, verdict_bank, flag_start;
    wire crc_err, stuff_err, form_err, model_valid, active_bank;
    wire [2:0]  verdict;
    wire [10:0] can_id;
    wire [3:0]  dlc;
    wire [63:0] data;
    wire [31:0] status_word, active_model_id, active_version;

    wire        csr_address, csr_read, csr_write, data_read, data_write, data_waitrequest, data_readdatavalid;
    wire [31:0] csr_writedata, csr_readdata, data_writedata, data_readdata;
    wire [13:0] data_address;
    wire [1:0]  data_burstcount;

`ifdef SWAP_TEST
    wire ips_on = 1'b0;
`else
    wire ips_on = 1'b1;
`endif
    wire bus = rxd & ~tx_dominant;

    updatable_core #(.DEVICE_ID(`TEST_DEVICE_ID), .MAC_KEY(`TEST_MAC_KEY), .ENC_KEY(`TEST_ENC_KEY), .NV_SETTLE(5'd6)) dut (
        .clk(clk), .rst(rst), .por(por), .rxd(bus), .tx_dominant(tx_dominant), .ips_on(ips_on),
        .maint(maint),
        .spi_sclk(sclk), .spi_cs_n(cs_n), .spi_mosi(mosi), .spi_miso(miso),
        .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount),
        .frame_valid(frame_valid), .feat_ready(feat_ready),
        .verdict_valid(verdict_valid), .verdict(verdict), .verdict_bank(verdict_bank),
        .flag_start(flag_start), .crc_err(crc_err), .stuff_err(stuff_err), .form_err(form_err),
        .can_id(can_id), .dlc(dlc), .data(data), .status_word(status_word),
        .active_model_id(active_model_id), .active_version(active_version),
        .model_valid(model_valid), .active_bank(active_bank)
    );

    ufm_model #(.WRITE_CLKS(6), .ERASE_CLKS(150)) ufm (
        .clk(clk), .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount), .power_cut(1'b0)
    );

    // The ESP32 receiver: another node on the same bus.
    wire        r_valid, r_crc, r_stuff, r_form;
    wire [10:0] r_id;
    wire [3:0]  r_dlc;
    wire [63:0] r_data;
    can_rx receiver (
        .clk(clk), .rst(rst), .rxd(bus), .feat_bytes(4'd8), .in_frame(),
        .frame_valid(r_valid), .can_id(r_id), .dlc(r_dlc), .data(r_data),
        .crc_err(r_crc), .stuff_err(r_stuff), .form_err(r_form),
        .id_ready(), .dlc_ready(), .feat_ready(), .before_eof(), .bit_edge()
    );

    reg bits [0:N_BITS-1];
    reg [7:0] blob [0:32767];
    `include "spi_tasks.vh"

    integer i = 0, log, n_frame = 0, n_feat = 0, n_class = 0, n_crc = 0, n_stuff = 0, n_bus = 0, n_buserr = 0, n_flag = 0;
    integer swap_at_frame = 25;
    reg start_swap = 1'b0;
    reg [8*256-1:0] log_name;

    integer blob_n;
    reg [31:0] blob_crc;
    // The new model goes in over the real SPI pins at 2.625 MHz, while the bus
    // traffic goes on (it used to be forced straight into the decoder).
    task swap_model;
        begin
            $display("Upload started at bit %0d", i);
            spi_upload(blob_n);
            maint = 1;
            cs_low;
            spi_byte(8'h03);
            spi_byte(blob_crc[31:24]); spi_byte(blob_crc[23:16]);
            spi_byte(blob_crc[15:8]);  spi_byte(blob_crc[7:0]);
            cs_high;
            $display("COMMIT sent at bit %0d", i);
        end
    endtask

    initial begin
        $readmemb("bus_bits.mem", bits);
        if (!$value$plusargs("log=%s", log_name)) log_name = "got.csv";
        log = $fopen(log_name, "w");
        $fdisplay(log, "event,bit,id,dlc,data,class");
        repeat (5) @(posedge clk);
        por = 0;
        @(posedge clk);
        rst = 0;
        wait (model_valid);
        #1;
        $display("booted: model %h version %0d", active_model_id, active_version);
`ifdef SWAP_TEST
        begin : load_blob
            integer m, r;
            reg [8*20:1] nm;
            reg [8*20:1] want;
            reg found;
            want = "v2_p8";
            found = 0;
            m = $fopen("test_blobs/manifest.txt", "r");
            while (!found && !$feof(m)) begin
                r = $fscanf(m, "%s %d %h\n", nm, blob_n, blob_crc);
                if (nm == want) found = 1;
            end
            $fclose(m);
            $readmemh("test_blobs/v2_p8.hex", blob, 0, blob_n - 1);
        end
        maint = 1;
`endif
        repeat (5) @(posedge clk);
        #3.7;
        for (i = 0; i < N_BITS; i = i + 1) begin
            rxd = bits[i];
            #2000.0;
`ifdef SWAP_TEST
            if (n_class == swap_at_frame && swap_at_frame >= 0) begin
                swap_at_frame = -1;
                start_swap = 1'b1;
            end
`endif
        end
        rxd = 1;
        #40000;
        $fclose(log);
        $display("Played %0d frames. FRAME %0d FEAT %0d VERDICT %0d CRC_ERR %0d STUFF_ERR %0d", N_FRAMES,
                 n_frame, n_feat, n_class, n_crc, n_stuff);
        $display("IPS: flags %0d. Receiver: %0d frames, %0d errors. Active model %h version %0d",
                 n_flag, n_bus, n_buserr, active_model_id, active_version);
        $finish;
    end

    always @(posedge start_swap) swap_model;

    always @(posedge clk) if (!rst) begin
        if (frame_valid) begin
            $fdisplay(log, "FRAME,%0d,%03h,%0d,%016h,", i, can_id, dlc, data);
            n_frame = n_frame + 1;
        end
        if (crc_err)   begin $fdisplay(log, "CRC_ERR,%0d,,,,", i);   n_crc = n_crc + 1; end
        if (stuff_err) begin $fdisplay(log, "STUFF_ERR,%0d,,,,", i); n_stuff = n_stuff + 1; end
        if (feat_ready) begin
            $fdisplay(log, "FEAT,%0d,%03h,%0d,%016h,", i, can_id, dlc, data);
            n_feat = n_feat + 1;
        end
        if (verdict_valid) begin
            $fdisplay(log, "FINAL,%0d,%03h,,,%0d", i, can_id, verdict);
            $fdisplay(log, "BANK,%0d,%03h,,,%0d", i, can_id, verdict_bank);
            n_class = n_class + 1;
        end
        if (flag_start) begin
            $fdisplay(log, "FLAG,%0d,%03h,,,", i, can_id);
            n_flag = n_flag + 1;
        end
        if (r_valid) begin
            $fdisplay(log, "BUSFRAME,%0d,%03h,%0d,%016h,", i, r_id, r_dlc, r_data);
            n_bus = n_bus + 1;
        end
        if (r_crc || r_stuff) begin
            $fdisplay(log, "BUSERR,%0d,,,,", i);
            n_buserr = n_buserr + 1;
        end
    end
endmodule
