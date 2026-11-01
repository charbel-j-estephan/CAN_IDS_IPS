// Plays real dataset frames into can_rx and logs everything it reports, so
// python/compare_sim.py can put what was sent next to what the FPGA read.
//
// Run from the repo root (py -3.13 python/make_sim_frames.py first):
//
//   decoder only (can_rx with the ports of can_rx_tb.v):
//     iverilog -g2005 -I build/sim -o build/frames_sim hdl/frames_tb.v hdl/can_rx.v
//
//   + the forest features (can_rx also has a feat_ready output):
//     iverilog -g2005 -DHAS_FEAT -I build/sim -o build/frames_sim hdl/frames_tb.v hdl/can_rx.v
//
//   + the forest (hdl/generated/rf_ids.v from python/export_verilog.py) and
//     the unknown ID rule (hdl/known_id.v):
//     iverilog -g2005 -DWITH_RF -I build/sim -o build/frames_sim hdl/frames_tb.v hdl/can_rx.v hdl/generated/rf_ids.v hdl/known_id.v
//
//   then:
//     vvp build/frames_sim                   writes build/sim/got.csv
//     vvp build/frames_sim +drift_ppm=3000   sender clock 0.3 % slow
//     vvp build/frames_sim +vcd              waveform in build/sim/frames.vcd
//     py -3.13 python/compare_sim.py
//
// got.csv has one line per event, tagged with the bus bit being played when it
// happened (the same bit numbers as start_bit in sent.csv):
//   FEAT   features ready: id, dlc, data as they are at that moment
//   CLASS  the forest's answer (with -DWITH_RF), 0 Normal .. 4 RPM_spoof
//   FINAL  the same after the unknown ID rule (what the IPS acts on)
//   FRAME  good frame complete: id, dlc, data
//   CRC_ERR, STUFF_ERR
//
// Bytes the model reads (default 5, the STM32's model): -DFEAT_BYTES=6 with a
// 6-byte rf_ids.v (python/export_verilog.py on a --payload-bytes 6 model).
`timescale 1ns/1ps
`ifdef WITH_RF
`define HAS_FEAT
`endif
`ifndef FEAT_BYTES
`define FEAT_BYTES 5
`endif

module frames_tb;
    `include "frames.vh"

    reg clk = 0;
    always #10 clk = ~clk;                 // 50 MHz, the DE10 Lite oscillator

    reg rst = 1;
    reg rxd = 1;                           // recessive = idle bus
    wire        frame_valid, crc_err, stuff_err, id_ready, dlc_ready;
    wire [10:0] can_id;
    wire [3:0]  dlc;
    wire [63:0] data;
`ifdef HAS_FEAT
    wire        feat_ready;
`endif

    can_rx #(.FEAT_BYTES(`FEAT_BYTES)) dut (
        .clk(clk), .rst(rst), .rxd(rxd),
        .frame_valid(frame_valid), .can_id(can_id), .dlc(dlc), .data(data),
        .crc_err(crc_err), .stuff_err(stuff_err),
        .id_ready(id_ready), .dlc_ready(dlc_ready)
`ifdef HAS_FEAT
        , .feat_ready(feat_ready)
`endif
    );

`ifdef WITH_RF
    // The forest gets the features the way classify.c builds them: DLC at
    // most 8, the bytes after the first FEAT_BYTES zero (they are not on the
    // bus yet at feat_ready).
    localparam [63:0] FEAT_MASK = ~(64'hFFFF_FFFF_FFFF_FFFF >> (8 * `FEAT_BYTES));
    wire       rf_valid;
    wire [2:0] rf_class;
    rf_ids forest (
        .clk(clk), .rst(rst), .in_valid(feat_ready),
        .can_id(can_id), .dlc(dlc > 4'd8 ? 4'd8 : dlc), .data(data & FEAT_MASK),
        .out_valid(rf_valid), .class_out(rf_class)
    );

    // The unknown ID rule, as classify.c: Normal on an ID the car never sent
    // becomes Fuzzy (2). can_id still holds this frame's ID at rf_valid.
    wire       id_known;
    known_id ids (.can_id(can_id), .known(id_known));
    wire [2:0] final_class = (rf_class == 3'd0 && !id_known) ? 3'd2 : rf_class;
`endif

    reg bits [0:N_BITS-1];
    integer i = 0;                         // the bus bit being played now
    integer drift_ppm = 0;
    real    bit_ns;
    integer log;
    integer n_feat = 0, n_frame = 0, n_crc = 0, n_stuff = 0, n_class = 0;

    initial begin
        $readmemb("build/sim/bus_bits.mem", bits);
        log = $fopen("build/sim/got.csv", "w");
        $fdisplay(log, "event,bit,id,dlc,data,class");
        if ($value$plusargs("drift_ppm=%d", drift_ppm))
            $display("Sender bit time off by %0d ppm", drift_ppm);
        if ($test$plusargs("vcd")) begin
            $dumpfile("build/sim/frames.vcd");
            $dumpvars(0, frames_tb);
        end
        bit_ns = 2000.0 * (1.0 + drift_ppm * 1.0e-6);   // 500 kbit/s
        repeat (5) @(posedge clk);
        rst = 0;
        repeat (5) @(posedge clk);
        #3.7;                              // bus edges are not aligned to the FPGA clock
        for (i = 0; i < N_BITS; i = i + 1) begin
            rxd = bits[i];
            #(bit_ns);
        end
        rxd = 1;
        #(bit_ns * 20);
        $fclose(log);
        $display("Played %0d frames (%0d bus bits). can_rx reported: %0d FRAME, %0d FEAT, %0d CLASS, %0d CRC_ERR, %0d STUFF_ERR",
                 N_FRAMES, N_BITS, n_frame, n_feat, n_class, n_crc, n_stuff);
        $display("Log: build/sim/got.csv. Next: py -3.13 python/compare_sim.py");
        $finish;
    end

    // One line per event. Data is printed as 16 hex digits, byte 0 first.
    always @(posedge clk) if (!rst) begin
        if (frame_valid) begin
            $fdisplay(log, "FRAME,%0d,%03h,%0d,%016h,", i, can_id, dlc, data);
            n_frame = n_frame + 1;
        end
        if (crc_err) begin
            $fdisplay(log, "CRC_ERR,%0d,,,,", i);
            n_crc = n_crc + 1;
        end
        if (stuff_err) begin
            $fdisplay(log, "STUFF_ERR,%0d,,,,", i);
            n_stuff = n_stuff + 1;
        end
`ifdef HAS_FEAT
        if (feat_ready) begin
            $fdisplay(log, "FEAT,%0d,%03h,%0d,%016h,", i, can_id, dlc, data);
            n_feat = n_feat + 1;
        end
`endif
`ifdef WITH_RF
        if (rf_valid) begin
            $fdisplay(log, "CLASS,%0d,%03h,,,%0d", i, can_id, rf_class);
            $fdisplay(log, "FINAL,%0d,%03h,,,%0d", i, can_id, final_class);
            n_class = n_class + 1;
        end
`endif
    end
endmodule
