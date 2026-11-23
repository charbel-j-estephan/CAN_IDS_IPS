// Phase B's version of engine_all_tb: the model goes in over the real SPI port
// and through COMMIT into flash, then every frame goes through the classifier,
// one per clock. Frames are forced straight into the classifier's input (the
// CAN decoder is checked separately, in core_frames_tb).
//
//   vvp -n spi_all_sim +blob=<test_blob.hex> +in=<frames.hex> +out=<classes.txt>
`timescale 1ns/1ps
`include "test_key.vh"
module spi_all_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg  rst = 1, por = 1, sclk = 0, cs_n = 1, mosi = 0, maint = 1;
    wire miso;

    wire        csr_address, csr_read, csr_write, data_read, data_write, data_waitrequest, data_readdatavalid;
    wire [31:0] csr_writedata, csr_readdata, data_writedata, data_readdata;
    wire [13:0] data_address;
    wire [1:0]  data_burstcount;
    wire [31:0] status_word, active_model_id, active_version;
    wire        model_valid, active_bank;

    updatable_core #(.DEVICE_ID(`TEST_DEVICE_ID), .MAC_KEY(`TEST_MAC_KEY), .ENC_KEY(`TEST_ENC_KEY), .NV_SETTLE(5'd6), .ROM_INIT_HEX("../../default_model.hex")) dut (
        .clk(clk), .rst(rst), .por(por), .rxd(1'b1), .tx_dominant(), .ips_on(1'b0), .strict(1'b0), .maint(maint),
        .spi_sclk(sclk), .spi_cs_n(cs_n), .spi_mosi(mosi), .spi_miso(miso),
        .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount),
        .frame_valid(), .feat_ready(), .verdict_valid(), .verdict(), .verdict_bank(), .flag_start(),
        .crc_err(), .stuff_err(), .form_err(), .can_id(), .dlc(), .data(),
        .status_word(status_word), .active_model_id(active_model_id), .active_version(active_version),
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

    reg [7:0] blob [0:32767];
    `include "spi_tasks.vh"

    reg [8*256-1:0] blob_name, in_name, out_name;
    integer fin, fout, r, n_in = 0, n_out = 0, blob_n;
    reg [31:0] blob_crc;
    reg [79:0] line;

    reg         f_valid = 0;
    reg [10:0]  f_id = 0;
    reg [3:0]   f_dlc = 0;
    reg [63:0]  f_data = 0;

    initial begin
        if (!$value$plusargs("in=%s", in_name) || !$value$plusargs("out=%s", out_name)
            || !$value$plusargs("blob=%s", blob_name)) begin
            $display("usage: vvp spi_all_sim +blob=<model.hex> +in=<frames.hex> +out=<classes.txt>");
            $finish;
        end
        $readmemh(blob_name, blob);
        blob_n   = {blob[11], blob[10], blob[9], blob[8]};
        blob_crc = {blob[blob_n-17], blob[blob_n-18], blob[blob_n-19], blob[blob_n-20]};
        fin  = $fopen(in_name, "r");
        fout = $fopen(out_name, "w");
        if (fin == 0 || fout == 0) begin
            $display("cannot open %0s or %0s", in_name, out_name);
            $finish;
        end
        repeat (5) @(posedge clk);
        por = 0;
        @(posedge clk);
        rst = 0;
        #1000;
        wait_boot;

        // Over the real SPI port, through COMMIT, into flash.
        spi_upload(blob_n);
        spi_commit(blob_crc);
        if (status_now[7:0] != 8'd0) begin
            $display("COMMIT failed: error %0d", status_now[7:0]);
            $finish;
        end
        spi_version;
        if (ver_model !== {blob[15], blob[14], blob[13], blob[12]}) begin
            $display("wrong model active: %h", ver_model);
            $finish;
        end

        // Frames, straight into the classifier's feature path.
        force dut.can_id     = f_id;
        force dut.dlc        = f_dlc;
        force dut.data       = f_data;
        force dut.feat_ready = f_valid;
        repeat (3) @(posedge clk);
        while (!$feof(fin)) begin
            r = $fscanf(fin, "%h\n", line);
            if (r == 1) begin
                n_in = n_in + 1;
                @(posedge clk);
                f_valid <= 1'b1;
                f_id    <= line[78:68];
                f_dlc   <= line[67:64];
                f_data  <= line[63:0];
            end
        end
        @(posedge clk);
        f_valid <= 1'b0;
        repeat (20) @(posedge clk);
        $fclose(fout);
        $display("DONE in=%0d out=%0d", n_in, n_out);
        $finish;
    end

    always @(posedge clk) if (!rst && dut.out_valid && !dut.out_tag) begin
        $fdisplay(fout, "%0d%0d", dut.out_forest, dut.out_final);
        n_out = n_out + 1;
    end
endmodule
