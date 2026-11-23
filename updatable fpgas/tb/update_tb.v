// The update path end to end: SPI commands, double buffering, COMMIT, flash
// slots, boot, every rejection, power failures. Uses the behavioural flash
// (ufm_model.v). Run from the build folder, after make_test_blobs.py.
//
//   vvp -n update_sim
//
// Prints one PASS or FAIL line per check and a total. Exit message "ALL PASS".
`timescale 1ns/1ps
`include "test_key.vh"
module update_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg  rst = 1, por = 1, maint = 0, sclk = 0, cs_n = 1, mosi = 0, rxd = 1;
    reg  power_cut = 0;
    wire miso, tx_dominant;
    wire model_valid, active_bank, flag_start, verdict_valid, verdict_bank, frame_valid, feat_ready;
    wire crc_err, stuff_err, form_err;
    wire [2:0] verdict;
    wire [10:0] can_id;
    wire [3:0] dlc;
    wire [63:0] data;
    wire [31:0] status_word, active_model_id, active_version;

    wire        csr_address, csr_read, csr_write, data_read, data_write, data_waitrequest, data_readdatavalid;
    wire [31:0] csr_writedata, csr_readdata, data_writedata, data_readdata;
    wire [13:0] data_address;
    wire [1:0]  data_burstcount;

`ifndef READ_MODEL_EN
`define READ_MODEL_EN 1
`endif

    updatable_core #(.DEVICE_ID(`TEST_DEVICE_ID), .MAC_KEY(`TEST_MAC_KEY), .ENC_KEY(`TEST_ENC_KEY),
                     .READ_MODEL_EN(`READ_MODEL_EN), .NV_SETTLE(5'd6),
                     .NV_TIMEOUT(32'd20_000)) dut (
        .clk(clk), .rst(rst), .por(por), .rxd(rxd), .tx_dominant(tx_dominant), .ips_on(1'b1), .strict(1'b0),
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
        .data_burstcount(data_burstcount), .power_cut(power_cut)
    );

    reg [7:0] blob [0:32767];
    `include "spi_tasks.vh"

    integer fails = 0, checks = 0;

    localparam [31:0] ID_P5 = 32'h2DDDA531, ID_P6 = 32'hE7321E76, ID_P8 = 32'hC0F7E8B5;
    localparam SRC_A = 2'd1, SRC_B = 2'd2, SRC_ROM = 2'd3;

    localparam E_OK = 0, E_NOT_MAINT = 1, E_LOCKED = 2, E_TRUNCATED = 3, E_HEADER = 4, E_PROTOCOL = 5,
               E_DEVICE = 6, E_VERSION = 7, E_RANGE = 8, E_LENGTH = 9, E_CRC = 10, E_SELFTEST = 11,
               E_MAC = 12;

    // A test that hangs fails the run instead of running for ever. A full run
    // ends near 66 ms of simulated time.
    initial begin
        #150_000_000;
        $display("FAIL  timeout: no result after 150 ms (%0d checks so far)", checks);
        $display("%0d checks, %0d failed", checks + 1, fails + 1);
        $finish;
    end

    task check;
        input [8*40:1] name;
        input ok;
        begin
            checks = checks + 1;
            if (ok) $display("PASS  %0s", name);
            else begin $display("FAIL  %0s  (status %h)", name, status_now); fails = fails + 1; end
        end
    endtask

    // The blob file and its size and CRC from the manifest.
    integer blob_n;
    reg [31:0] blob_crc;
    reg [8*48:1] mname;
    task use_blob;
        input [8*20:1] name;
        integer m, r;
        reg [8*20:1] nm;
        reg found;
        reg [8*64:1] path;
        begin
            found = 0;
            m = $fopen("test_blobs/manifest.txt", "r");
            while (!found && !$feof(m)) begin
                r = $fscanf(m, "%s %d %h\n", nm, blob_n, blob_crc);
                if (nm == name) found = 1;
            end
            $fclose(m);
            if (!found) begin $display("no blob %0s", name); $finish; end
            $swrite(path, "test_blobs/%0s.hex", name);
            $readmemh(path, blob, 0, blob_n - 1);
        end
    endtask

    task power_on;
        begin
            rst = 1; por = 1;
            repeat (5) @(posedge clk);
            @(posedge clk); por = 0;
            @(posedge clk); rst = 0;
            #1000;
            wait_boot;
        end
    endtask

    task expect_model;
        input [8*40:1] name;
        input [31:0] id;
        input [31:0] ver;
        input [1:0]  src;
        begin
            spi_version;
            spi_status;
            check(name, ver_model == id && ver_version == ver && status_now[13:12] == src);
            if (!(ver_model == id && ver_version == ver && status_now[13:12] == src))
                $display("      got model %h version %0d source %0d", ver_model, ver_version, status_now[13:12]);
        end
    endtask

    task reject;
        input [8*20:1] name;
        input [7:0] code;
        begin
            use_blob(name);
            upload_blob;
            spi_commit(blob_crc);
            check(name, status_now[7:0] == code);
            if (status_now[7:0] != code) $display("      last_error %0d, wanted %0d", status_now[7:0], code);
        end
    endtask

    task good_upload;
        input [8*20:1] name;
        begin
            use_blob(name);
            upload_blob;
            spi_commit(blob_crc);
            check(name, status_now[7:0] == E_OK);
            if (status_now[7:0] != E_OK) $display("      last_error %0d", status_now[7:0]);
        end
    endtask

    // The power fails as soon as the flash has started enough operations.
    reg cut_done = 0;
    integer cut_delay = 0;
    always @(posedge ufm.cut_now) begin
        #(cut_delay);
        power_cut = 1;
        rst = 1;
        #2000;
        power_cut = 0;
        ufm.cut_after = -1;
        cut_done = 1;
    end

    task send_commit;
        input [31:0] crc;
        begin
            cs_low;
            spi_byte(8'h03);
            spi_byte(crc[31:24]); spi_byte(crc[23:16]);
            spi_byte(crc[15:8]);  spi_byte(crc[7:0]);
            cs_high;
        end
    endtask

    // A byte straight into the command decoder: one real SPI upload takes 600,000
    // clocks of simulation, so the other tests skip the wire. Status and the
    // first upload use the real SPI port.
    reg use_fast_upload = 0;
    task fast_byte;
        input [7:0] b;
        begin
            @(negedge clk);
            while (!dut.shadow_ready) @(negedge clk);
            force dut.rx_byte  = b;
            force dut.rx_valid = 1'b1;
            @(negedge clk);
            force dut.rx_valid = 1'b0;
            repeat (2) @(negedge clk);
        end
    endtask

    task fast_upload;
        input integer n;
        integer off, chunk, c;
        begin
            off = 0;
            while (off < n) begin
                chunk = (n - off > 256) ? 256 : (n - off);
                cs_low;
                fast_byte(8'h01);
                fast_byte(off[15:8]);   fast_byte(off[7:0]);
                fast_byte(chunk[15:8]); fast_byte(chunk[7:0]);
                for (c = 0; c < chunk; c = c + 1) fast_byte(blob[off + c]);
                cs_high;
                off = off + chunk;
            end
            release dut.rx_valid;
            release dut.rx_byte;
        end
    endtask

    task upload_blob;
        begin
            if (use_fast_upload) fast_upload(blob_n);
            else                 spi_upload(blob_n);
        end
    endtask

    integer i;
    initial begin
        // ---- empty flash: the boot ROM default ----------------------------
        power_on;
        expect_model("boot ROM when flash is empty", ID_P5, 32'd1, SRC_ROM);
        check("model is on", model_valid === 1'b1);

        // ---- outside maintenance mode: only STATUS and GET_VERSION work ----
        maint = 0;
        use_blob("v2_p8");
        spi_upload(32);
        spi_status;
        check("WRITE refused outside maintenance", status_now[7:0] == E_NOT_MAINT);
        spi_commit(blob_crc);
        check("COMMIT refused outside maintenance", status_now[7:0] == E_NOT_MAINT);
        expect_model("old model still active", ID_P5, 32'd1, SRC_ROM);

        // ---- a good update, over the real SPI port --------------------------
        maint = 1;
        use_fast_upload = 0;
        good_upload("v2_p8");
        use_fast_upload = 1;
        expect_model("v2 p8 active, in slot A", ID_P8, 32'd2, SRC_A);

        // ---- every rejection, old model untouched ------------------------------
        reject("bad_crc_body",   E_CRC);        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("truncated",      E_TRUNCATED);  expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("bad_feature",    E_RANGE);      expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("bad_child",      E_RANGE);      expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("lower_version",  E_VERSION);    expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("wrong_device",   E_DEVICE);     expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("wrong_protocol", E_PROTOCOL);   expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("bad_selftest",   E_SELFTEST);   expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("wildcard_device", E_DEVICE);    expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("few_bytes",      E_HEADER);     expect_model("  still v2", ID_P8, 32'd2, SRC_A);

        // ---- the stamp: each of these passes every check before it -----------
        // Every E_MAC is a strike, so a power cycle after each two keeps the
        // port open (and each power cycle boots v2 again, its stamp checked).
        reject("mac_missing",    E_MAC);        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("mac_bitflip",    E_MAC);        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        power_on;
        expect_model("stamped slot boots", ID_P8, 32'd2, SRC_A);
        reject("attacker_edit",  E_MAC);        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("other_key",      E_MAC);        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        power_on;

        // ---- no answers about the content without a stamp ------------------
        // A scrambled blob with one node byte made out of range. With this
        // board's stamp it is refused for the range (so the fault is real) ...
        reject("enc_bad_node",   E_RANGE);      expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        // ... without it, neither PARSE_ERRORS nor COMMIT says so: only E_MAC.
        use_blob("enc_bad_node_nokey");
        upload_blob;
        spi_read_reg(16'h0030);
        check("unstamped scrambled: 0030 hides node bits",
              reg_word[4:2] == 3'b000 && dut.core.w_err[2] == 1'b1);
        spi_commit(blob_crc);
        check("unstamped scrambled bad node: E_MAC", status_now[7:0] == E_MAC);
        expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        reject("bad_feature_nokey", E_MAC);     expect_model("  still v2", ID_P8, 32'd2, SRC_A);
        power_on;
        use_blob("enc_flag_cleared");
        upload_blob;
        spi_commit(blob_crc);
        check("scrambled, flag cleared: stamped, refused after it",
              status_now[7:0] == E_HEADER || status_now[7:0] == E_RANGE
              || status_now[7:0] == E_LENGTH || status_now[7:0] == E_SELFTEST);
        $display("      (refused with error %0d)", status_now[7:0]);
        expect_model("  still v2", ID_P8, 32'd2, SRC_A);

        // ---- fault injection: one zapped compare or lockout copy ------------
        // Force the first stamp compare to "yes": the second one still says no.
        force dut.core.mac_ok1 = 1'b1;
        reject("mac_bitflip",    E_MAC);
        release dut.core.mac_ok1;
        expect_model("  zapped compare: still v2", ID_P8, 32'd2, SRC_A);
        // Make the two lockout copies disagree: the port must lock.
        force dut.core.strikes_n = 2'b00;
        @(posedge clk); #1;
        release dut.core.strikes_n;
        spi_status;
        check("zapped lockout copy: port locked", status_now[9] == 1'b1);
        power_on;
        spi_status;
        check("  power cycle unlocks", status_now[9] == 1'b0);

        // A good blob with the wrong expected CRC.
        use_blob("v3_p5");
        upload_blob;
        spi_commit(blob_crc ^ 32'h1);
        check("wrong CRC in COMMIT", status_now[7:0] == E_CRC);
        expect_model("  still v2", ID_P8, 32'd2, SRC_A);

        // Out of order chunks.
        use_blob("v3_p5");
        cs_low; spi_byte(8'h01); spi_byte(8'h01); spi_byte(8'h00); spi_byte(8'h00); spi_byte(8'h04);
        for (i = 0; i < 4; i = i + 1) spi_byte(8'hAA);
        cs_high;
        spi_status;
        check("out of order WRITE refused", status_now[7:0] == 8'd17);

        // ---- second update: goes to the other slot --------------------------
        good_upload("v3_p5");
        expect_model("v3 p5 active, in slot B", ID_P5, 32'd3, SRC_B);

        // ---- power cycle: the new model is still there, no STM32 involved ----
        maint = 0;
        power_on;
        expect_model("boot from slot B", ID_P5, 32'd3, SRC_B);

        maint = 1;
        spi_read_reg(16'h0028);
        check("rollback floor kept in flash", reg_word == 32'd3);
        maint = 0;

        // ---- power cut while writing: v4 must not become active ---------------
        maint = 1;
        use_blob("v4_p6");
        upload_blob;
        ufm.ops_done = 0;
        cut_done = 0;
        cut_delay = 0;
        ufm.cut_after = 200;           // erases and writes let through
        send_commit(blob_crc);
        wait (cut_done);
        power_on;
        expect_model("power cut in write: last good model", ID_P5, 32'd3, SRC_B);

        // ---- power cut during the erase --------------------------------------
        use_blob("v4_p6");
        upload_blob;
        ufm.ops_done = 0;
        cut_done = 0;
        cut_delay = 1000;              // inside the erase
        ufm.cut_after = 0;
        send_commit(blob_crc);
        wait (cut_done);
        power_on;
        expect_model("power cut in erase: last good model", ID_P5, 32'd3, SRC_B);

        // ---- it still updates after all that -----------------------------------
        good_upload("v5_p8");
        expect_model("update works after power cuts", ID_P8, 32'd5, SRC_A);
        power_on;
        expect_model("boot after power cuts", ID_P8, 32'd5, SRC_A);

        // ---- corrupt the active slot: boot the other one --------------------
        for (i = 0; i < 100; i = i + 1) ufm.mem[0 + i] = 32'h12345678;    // slot A
        power_on;
        expect_model("active slot corrupt: other slot", ID_P5, 32'd3, SRC_B);

        // ---- corrupt both slots: boot the ROM -------------------------------
        for (i = 0; i < 100; i = i + 1) ufm.mem[8192 + i] = 32'h12345678; // slot B
        power_on;
        expect_model("both slots corrupt: boot ROM", ID_P5, 32'd1, SRC_ROM);

        // ---- three signature failures lock the port -----------------------
        // A good blob stamped with another master key: it passes every check
        // before the stamp (version 5, floor 5), so each try is a strike.
        maint = 1;
        use_blob("other_key_v5");
        upload_blob; spi_commit(blob_crc);
        check("signature failure 1", status_now[7:0] == E_MAC);
        upload_blob; spi_commit(blob_crc);
        check("signature failure 2", status_now[7:0] == E_MAC);
        upload_blob; spi_commit(blob_crc);
        check("signature failure 3", status_now[7:0] == E_MAC);
        check("port locked", status_now[9] == 1'b1);
        spi_upload(32);
        spi_status;
        check("locked: WRITE refused", status_now[7:0] == E_LOCKED);
        spi_commit(blob_crc);
        check("locked: COMMIT refused", status_now[7:0] == E_LOCKED);
        spi_counters;
        check("signature failures counted", cnt_sig == 32'd3);
        expect_model("locked: model unchanged", ID_P5, 32'd1, SRC_ROM);

        // KEY reset is not a power cycle: still locked.
        rst = 1; repeat (5) @(posedge clk); rst = 0;
        #1000; wait_boot;
        spi_status;
        check("reset does not clear the lockout", status_now[9] == 1'b1);

        // Power cycle clears it and the port works again.
        power_on;
        spi_status;
        check("power cycle clears the lockout", status_now[9] == 1'b0);
        good_upload("v5_p8");
        expect_model("updates again after lockout", ID_P8, 32'd5, SRC_A);

        // ---- the stamp is ready late: compare only once the tags are in ------
        // Hold the seal's result back past the end of the upload, then give it
        // on one clock, as a slow seal would. The old code compared on that
        // clock against an empty tag register and refused a good blob.
        use_blob("v5_p8");
        force dut.core.cmac_strobe = 1'b0;
        force dut.core.cmac_done   = 1'b0;
        upload_blob;
        send_commit(blob_crc);
        // COMMIT starts on its last byte, but send_commit returns only after
        // raising CS, about 30 clocks later: wait for "20 or more", not "20",
        // or that clock is gone and the wait never ends. The seal gives up at
        // 255, so this is still well inside the window.
        wait (dut.core.state == 6'd19 && dut.core.mac_wait >= 8'd20);
        @(negedge clk);
        force dut.core.cmac_strobe = 1'b1;
        force dut.core.cmac_done   = 1'b1;
        @(negedge clk);
        release dut.core.cmac_strobe;
        release dut.core.cmac_done;
        spi_status;
        while (status_now[8] || status_now[15]) begin #20000; spi_status; end
        check("stamp ready late: good blob accepted", status_now[7:0] == E_OK);
        expect_model("  v5 again, in slot B", ID_P8, 32'd5, SRC_B);

        // ---- a glitched swap request: no swap without the swap code ---------
        begin : glitch_swap
            reg bank_before;
            bank_before = active_bank;
            @(negedge clk);
            force dut.core.swap_pending = 1'b1;
            repeat (10) @(negedge clk);
            release dut.core.swap_pending;
            repeat (5) @(negedge clk);
            check("glitched swap request: bank unchanged, dropped",
                  active_bank == bank_before && dut.core.swap_pending == 1'b0);
        end
        expect_model("  still v5 in slot B", ID_P8, 32'd5, SRC_B);

        // ---- an upload while the boot swap waits for a gap: refused busy -----
        force dut.core.fe_idle = 1'b0;          // a frame on the bus, for good
        rst = 1; por = 1;
        repeat (5) @(posedge clk);
        @(posedge clk); por = 0;
        @(posedge clk); rst = 0;
        #1000;
        spi_status;
        while (status_now[8] || !status_now[15]) spi_status;
        spi_upload(32);
        spi_status;
        check("upload while the swap waits: E_BUSY", status_now[7:0] == 8'd16);
        release dut.core.fe_idle;
        wait_boot;
        expect_model("  then boots as normal", ID_P8, 32'd5, SRC_B);

        // ---- the flash stops answering: ROM at once, COMMIT refused ----------
        ufm.stuck_wait = 1;
        power_on;
        expect_model("dead flash: ROM boots at once", ID_P5, 32'd1, SRC_ROM);
        check("dead flash: error says flash", status_now[7:0] == 8'd18);
        use_blob("v5_p8");
        upload_blob;
        spi_commit(blob_crc);
        check("dead flash: COMMIT refused", status_now[7:0] == 8'd18);
        ufm.stuck_wait = 0;
        power_on;
        expect_model("flash back: boots v5 from slot B", ID_P8, 32'd5, SRC_B);

        // ---- the journal page fills up: the next record goes to the other page
        // Start from an empty flash, put one real update in slot A, then fill
        // journal page A to its last record (512) with valid records that point
        // at it. The next COMMIT must erase page B and write there.
        for (i = 0; i < 16384; i = i + 1) ufm.mem[i] = 32'hFFFFFFFF;
        power_on;
        expect_model("journal: empty flash, ROM", ID_P5, 32'd1, SRC_ROM);
        good_upload("v2_p8");
        expect_model("journal: v2 in slot A", ID_P8, 32'd2, SRC_A);
        for (i = 1; i < 512; i = i + 1) begin
            ufm.mem[6144 + 4*i]     = 32'h4C4E524A;
            ufm.mem[6144 + 4*i + 1] = {i[23:0] + 24'd1, 7'd0, 1'b0};
            ufm.mem[6144 + 4*i + 2] = 32'd2;
            ufm.mem[6144 + 4*i + 3] = ~(32'h4C4E524A ^ {i[23:0] + 24'd1, 7'd0, 1'b0} ^ 32'd2);
        end
        power_on;
        expect_model("journal: page A full, boots slot A", ID_P8, 32'd2, SRC_A);
        good_upload("v3_p5");
        expect_model("journal: rolled, v3 in slot B", ID_P5, 32'd3, SRC_B);
        check("journal: record 0 of page B written", ufm.mem[14336] == 32'h4C4E524A
              && ufm.mem[14337] == {24'd513, 7'd0, 1'b1});
        power_on;
        expect_model("journal: boots slot B from page B", ID_P5, 32'd3, SRC_B);
        good_upload("v5_p8");
        check("journal: next record on page B", ufm.mem[14340] == 32'h4C4E524A
              && ufm.mem[14341] == {24'd514, 7'd0, 1'b0});
        power_on;
        expect_model("journal: boots v5 after the roll", ID_P8, 32'd5, SRC_A);

        // ---- no usable model anywhere: the port must still take one -------
        // Empty flash and a broken ROM (its magic spoiled). The board boots with
        // no model, not busy, and a normal upload repairs it.
        for (i = 0; i < 16384; i = i + 1) ufm.mem[i] = 32'hFFFFFFFF;
        dut.core.rom.rom[0] = 8'h00;
        rst = 1; por = 1;
        repeat (5) @(posedge clk);
        @(posedge clk); por = 0;
        @(posedge clk); rst = 0;
        #1000;
        spi_status;
        while (status_now[8]) spi_status;
        check("no model: not busy, source none", status_now[8] == 1'b0 && status_now[13:12] == 2'd0);
        check("no model: classification off", model_valid === 1'b0);
        check("no model: error says header", status_now[7:0] == E_HEADER);
        good_upload("v5_p8");
        expect_model("no model: repaired over SPI", ID_P8, 32'd5, SRC_A);
        check("no model: classification on again", model_valid === 1'b1);
        dut.core.rom.rom[0] = 8'h43;          // 'C' of the magic: the ROM is whole again

        // ---- an unstamped rulebook in flash does not boot --------------------
        // As after the Stage 5 upgrade: a slot and journal record from before,
        // without a valid stamp. The board must fall back to the ROM model.
        for (i = 0; i < 16384; i = i + 1) ufm.mem[i] = 32'hFFFFFFFF;
        use_blob("unsigned_v5_p8");
        for (i = 0; i < blob_n; i = i + 4)
            ufm.mem[i / 4] = {blob[i+3], blob[i+2], blob[i+1], blob[i]};
        ufm.mem[6144] = 32'h4C4E524A;
        ufm.mem[6145] = {24'd1, 7'd0, 1'b0};
        ufm.mem[6146] = 32'd5;
        ufm.mem[6147] = ~(32'h4C4E524A ^ {24'd1, 7'd0, 1'b0} ^ 32'd5);
        power_on;
        expect_model("unstamped slot in flash: ROM boots", ID_P5, 32'd1, SRC_ROM);

        // ---- a scrambled rulebook --------------------------------------------
        good_upload("enc_v6_p6");
        expect_model("scrambled v6 active, in slot A", ID_P6, 32'd6, SRC_A);
        // Flash must hold the scrambled bytes, not the plain ones: compare the
        // first 1,000 node bytes in slot A with the plain blob of the same model.
        begin : flash_check
            integer same, scr_same, w;
            reg [7:0] plain [0:32767];
            reg [7:0] fb;
            $readmemh("test_blobs/plain_v6_p6.hex", plain, 0, blob_n - 1);
            same = 0; scr_same = 0;
            for (i = 704; i < 1704; i = i + 1) begin
                w = ufm.mem[i / 4];
                fb = w >> (8 * (i % 4));
                if (fb == plain[i]) same = same + 1;
                if (fb == blob[i])  scr_same = scr_same + 1;
            end
            $display("      of 1000 node bytes in flash: %0d equal the scrambled blob, %0d the plain one",
                     scr_same, same);
            check("flash holds the scrambled bytes", scr_same == 1000);
            check("flash does not hold the plain nodes", same < 20);
        end
        power_on;
        expect_model("scrambled slot boots after a power cycle", ID_P6, 32'd6, SRC_A);

        // ---- reading the model back ----------------------------------------
        spi_read_reg(16'h0004);
        check("READ of a status register", reg_word == ID_P6);
`ifdef EXPECT_READ_BLOCKED
        spi_read_reg(16'h4000);
        check("production: node read blocked", reg_word == 32'hDEAD0BAD);
        spi_read_reg(16'h1000);
        check("production: header read blocked", reg_word == 32'hDEAD0BAD);
`else
        // A development build can read the shadow bank.
        use_blob("v3_p5");
        upload_blob;
        spi_read_reg(16'h4000);        // tree 0, level 0, node 0, low 32 bits
        check("development: node readable",
              reg_word == {blob[704+3], blob[704+2], blob[704+1], blob[704]});
        spi_read_reg(16'h4000 | (16'd12 << 7));   // tree 0, level 12: does not exist
        check("development: level 12 refused", reg_word == 32'hDEAD0BAD);
        spi_read_reg(16'h4000);
        check("development: node readable after that",
              reg_word == {blob[704+3], blob[704+2], blob[704+1], blob[704]});
`endif

        $display("%0d checks, %0d failed", checks, fails);
        if (fails == 0) $display("ALL PASS");
        $finish;
    end
endmodule
