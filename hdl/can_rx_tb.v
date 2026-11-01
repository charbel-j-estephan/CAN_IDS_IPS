// Testbench for the CAN receiver (can_rx), see docs/FPGA_INTERFACE.md.
//
// Plays hdl/vectors/bus_bits.mem into the rxd input at 2 us per bit
// (500 kbit/s) and checks every frame can_rx reports against
// hdl/vectors/expected.mem: ID, DLC, data, CRC errors and stuff errors.
//
// Run from the repo root:
//   python python/make_can_vectors.py              (only if the vectors are missing)
//   iverilog -g2005 -I hdl/vectors -o build/can_rx_sim hdl/can_rx_tb.v hdl/can_rx.v
//   vvp build/can_rx_sim
//   vvp build/can_rx_sim +drift_ppm=3000            sender clock 0.3 % slow
//   vvp build/can_rx_sim +drift_ppm=-3000           sender clock 0.3 % fast
//   vvp build/can_rx_sim +vcd                       waveform in build/can_rx.vcd
//
// The DUT must have exactly these ports:
//   module can_rx (input clk, input rst, input rxd,
//                  output frame_valid, output [10:0] can_id, output [3:0] dlc,
//                  output [63:0] data, output crc_err, output stuff_err,
//                  output id_ready, output dlc_ready);
`timescale 1ns/1ps
module can_rx_tb;
    `include "can_vectors.vh"

    reg clk = 0;
    always #10 clk = ~clk;                 // 50 MHz, the DE10 Lite oscillator

    reg rst = 1;
    reg rxd = 1;                           // recessive = 1, the idle bus
    wire        frame_valid, crc_err, stuff_err, id_ready, dlc_ready;
    wire [10:0] can_id;
    wire [3:0]  dlc;
    wire [63:0] data;

    can_rx dut (
        .clk(clk), .rst(rst), .rxd(rxd),
        .frame_valid(frame_valid), .can_id(can_id), .dlc(dlc), .data(data),
        .crc_err(crc_err), .stuff_err(stuff_err),
        .id_ready(id_ready), .dlc_ready(dlc_ready)
    );

    reg       bits [0:N_BITS-1];
    reg [87:0] exp_mem [0:N_FRAMES-1];     // flags(8) can_id(12) dlc(4) data(64)

    integer drift_ppm = 0;
    real    bit_ns;
    integer i;

    // ---- drive the bus ----
    initial begin
        $readmemb("hdl/vectors/bus_bits.mem", bits);
        $readmemh("hdl/vectors/expected.mem", exp_mem);
        if ($value$plusargs("drift_ppm=%d", drift_ppm))
            $display("Sender bit time off by %0d ppm", drift_ppm);
        if ($test$plusargs("vcd")) begin
            $dumpfile("build/can_rx.vcd");
            $dumpvars(0, can_rx_tb);
        end
        bit_ns = 2000.0 * (1.0 + drift_ppm * 1.0e-6);
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
        finish_check;
    end

    // ---- check what the receiver reports ----
    integer k = 0, errors = 0, n_valid = 0, n_crc = 0, n_stuff = 0;
    integer n_id = 0, n_dlc = 0;
    reg [7:0]  e_flag;
    reg [10:0] e_id;
    reg [3:0]  e_dlc;
    reg [63:0] e_data;

    task expect_next(input [7:0] got_flag);
        begin
            if (k >= N_FRAMES) begin
                errors = errors + 1;
                if (errors <= 10) $display("ERROR: extra event (flag %0d) after the last frame", got_flag);
            end else begin
                e_flag = exp_mem[k][87:80];
                e_id   = exp_mem[k][78:68];
                e_dlc  = exp_mem[k][67:64];
                e_data = exp_mem[k][63:0];
                if (got_flag != e_flag) begin
                    errors = errors + 1;
                    if (errors <= 10)
                        $display("ERROR frame %0d: expected %s, got %s", k, kind(e_flag), kind(got_flag));
                end else if (got_flag == 0 && (can_id != e_id || dlc != e_dlc || data != e_data)) begin
                    errors = errors + 1;
                    if (errors <= 10)
                        $display("ERROR frame %0d: got id %03h dlc %0d data %016h, expected id %03h dlc %0d data %016h",
                                 k, can_id, dlc, data, e_id, e_dlc, e_data);
                end
                k = k + 1;
            end
        end
    endtask

    function [127:0] kind(input [7:0] f);
        kind = (f == 0) ? "a good frame" : (f == 1) ? "a CRC error" : "a stuff error";
    endfunction

    always @(posedge clk) begin
        if (!rst) begin
            if (frame_valid) begin n_valid = n_valid + 1; expect_next(0); end
            if (crc_err)     begin n_crc   = n_crc + 1;   expect_next(1); end
            if (stuff_err)   begin n_stuff = n_stuff + 1; expect_next(2); end
            if (id_ready)    n_id  = n_id + 1;
            if (dlc_ready)   n_dlc = n_dlc + 1;
        end
    end

    task finish_check;
        begin
            if (k != N_FRAMES) begin
                errors = errors + 1;
                $display("ERROR: %0d of %0d frames were reported", k, N_FRAMES);
            end
            $display("frames: %0d good, %0d CRC errors, %0d stuff errors reported (expected %0d events)",
                     n_valid, n_crc, n_stuff, N_FRAMES);
            $display("early signals: id_ready %0d times, dlc_ready %0d times", n_id, n_dlc);
            if (errors == 0)
                $display("PASS: every frame decoded correctly");
            else
                $display("FAIL: %0d errors", errors);
            $finish;
        end
    endtask
endmodule
