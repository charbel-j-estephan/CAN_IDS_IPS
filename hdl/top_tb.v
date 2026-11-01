// The whole board design (quartus_board/can_ids_top.v) on a simulated CAN bus,
// driven only through its pins, as the real board will be:
//   GPIO[0] <- the bus (what the transceiver's RX shows)
//   GPIO[1] -> the bus (the transceiver's TX: 0 pulls it dominant)
//   KEY[0]  pressed for a moment (reset), SW[0] up (prevention on) or +ips_off
// The bus is wired-AND: bus = sender's bit AND GPIO[1]. A second can_rx on it
// plays the ESP32 receiver. The log has the same events as frames_tb.v, so
// python/compare_sim.py checks it frame by frame.
//
// From the repo root (frames for the 6-byte model first):
//   py -3.13 python/make_sim_frames.py --model models/rf5_d9_p6_time.joblib
//   iverilog -g2005 -I build/sim -o build/top_sim hdl/top_tb.v quartus_board/can_ids_top.v hdl/can_rx.v hdl/generated_p6/rf_ids.v hdl/known_id.v hdl/ips_flag.v
//   vvp build/top_sim
//   py -3.13 python/compare_sim.py --got got_top.csv --name compare_top
`timescale 1ns/1ps
module top_tb;
    `include "frames.vh"

    reg clk = 0;
    always #10 clk = ~clk;                 // MAX10_CLK1_50

    reg        sender = 1;                 // the sender's bit, recessive = idle
    reg  [1:0] key = 2'b10;                // KEY[0] pressed at the start (reset)
    reg  [9:0] sw  = 10'b1;                // SW[0] up: prevention on
    wire [9:0] ledr;
    wire [35:0] gpio;
    wire       bus = sender & gpio[1];     // wired-AND: the board's TX can pull it dominant
    assign gpio[0] = bus;

    can_ids_top dut (
        .ADC_CLK_10(1'b0), .MAX10_CLK1_50(clk), .MAX10_CLK2_50(clk),
        .KEY(key), .LEDR(ledr), .SW(sw), .GPIO(gpio)
    );

    // The ESP32 receiver: another node on the same bus.
    wire        r_valid, r_crc, r_stuff;
    wire [10:0] r_id;
    wire [3:0]  r_dlc;
    wire [63:0] r_data;
    can_rx receiver (
        .clk(clk), .rst(1'b0), .rxd(bus),
        .frame_valid(r_valid), .can_id(r_id), .dlc(r_dlc), .data(r_data),
        .crc_err(r_crc), .stuff_err(r_stuff), .id_ready(), .dlc_ready()
    );

    reg bits [0:N_BITS-1];
    integer i = 0, log, drift_ppm = 0;
    integer n_frame = 0, n_flag = 0, n_bus = 0, n_buserr = 0;
    real bit_ns;
    reg seen_led0 = 0, seen_led1 = 0, seen_led2 = 0;

    initial begin
        $readmemb("build/sim/bus_bits.mem", bits);
        log = $fopen("build/sim/got_top.csv", "w");
        $fdisplay(log, "event,bit,id,dlc,data,class");
        if ($test$plusargs("ips_off")) sw = 10'b0;
        if ($value$plusargs("drift_ppm=%d", drift_ppm))
            $display("Sender bit time off by %0d ppm", drift_ppm);
        bit_ns = 2000.0 * (1.0 + drift_ppm * 1.0e-6);
        repeat (20) @(posedge clk);
        key = 2'b11;                       // release KEY[0]
        repeat (20) @(posedge clk);
        #3.7;
        for (i = 0; i < N_BITS; i = i + 1) begin
            sender = bits[i];
            #(bit_ns);
        end
        sender = 1;
        #(bit_ns * 20);
        $fclose(log);
        $display("Board: %0d frames, %0d error flags (rej %0d, ilate %0d, abort %0d). Receiver: %0d survived, %0d thrown away",
                 n_frame, n_flag, dut.n_rej, dut.n_ilate, dut.n_abort, n_bus, n_buserr);
        $display("LEDs lit at some point: frame %0d, attack %0d, flag %0d; prevention LED %0d",
                 seen_led0, seen_led1, seen_led2, ledr[9]);
        $finish;
    end

    // Same events as frames_tb.v, read inside the board design.
    always @(posedge clk) begin
        if (ledr[0]) seen_led0 = 1;
        if (ledr[1]) seen_led1 = 1;
        if (ledr[2]) seen_led2 = 1;
        if (dut.feat_ready)
            $fdisplay(log, "FEAT,%0d,%03h,%0d,%016h,", i, dut.can_id, dut.dlc, dut.data);
        if (dut.rf_valid) begin
            $fdisplay(log, "CLASS,%0d,%03h,,,%0d", i, dut.can_id, dut.rf_class);
            $fdisplay(log, "FINAL,%0d,%03h,,,%0d", i, dut.can_id, dut.final_class);
        end
        if (dut.frame_valid) begin
            $fdisplay(log, "FRAME,%0d,%03h,%0d,%016h,", i, dut.can_id, dut.dlc, dut.data);
            n_frame = n_frame + 1;
        end
        if (dut.crc_err)   $fdisplay(log, "CRC_ERR,%0d,,,,", i);
        if (dut.stuff_err) $fdisplay(log, "STUFF_ERR,%0d,,,,", i);
        if (dut.flag_start) begin
            $fdisplay(log, "FLAG,%0d,%03h,,,", i, dut.can_id);
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
