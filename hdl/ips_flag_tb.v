// Unit test for ips_flag.v: every rule on its own, with a bit boundary every
// 100 clocks (500 kbit/s at 50 MHz).
//
//   iverilog -g2005 -o build/ips_flag_sim hdl/ips_flag_tb.v hdl/ips_flag.v
//   vvp build/ips_flag_sim
`timescale 1ns/1ps
module ips_flag_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg         rst = 1, enable = 1, verdict_valid = 0, before_eof = 1;
    reg  [2:0]  verdict = 0;
    reg  [10:0] can_id = 11'h0A0;
    reg  [63:0] data = 0;

    // Bit boundaries: a 1-clock pulse every 100 clocks.
    integer phase = 0;
    reg     bit_edge = 0;
    always @(posedge clk) begin
        phase    <= (phase == 99) ? 0 : phase + 1;
        bit_edge <= (phase == 99);
    end

    wire tx, start;
    wire [31:0] n_rej, n_ilate, n_allow, n_abort;
    ips_flag dut (.clk(clk), .rst(rst), .enable(enable), .verdict_valid(verdict_valid),
                  .verdict(verdict), .can_id(can_id), .data(data), .before_eof(before_eof),
                  .bit_edge(bit_edge), .tx_dominant(tx), .flag_start(start),
                  .n_rej(n_rej), .n_ilate(n_ilate), .n_allow(n_allow), .n_abort(n_abort));

    // A copy whose flag is too long (10 bits): the safety cut must stop it at 8.
    wire tx_long;
    wire [31:0] long_rej, long_ilate, long_allow, long_abort;
    ips_flag #(.FLAG_BITS(10)) long_flag (.clk(clk), .rst(rst), .enable(enable),
                  .verdict_valid(verdict_valid), .verdict(verdict), .can_id(can_id), .data(data),
                  .before_eof(before_eof), .bit_edge(bit_edge), .tx_dominant(tx_long),
                  .flag_start(), .n_rej(long_rej), .n_ilate(long_ilate), .n_allow(long_allow),
                  .n_abort(long_abort));

    // Measure every flag: clocks from the bit boundary to TX dominant, and its length.
    integer cyc = 0, t_edge = 0, t_rise = 0, last_width = 0, last_delay = -1, long_width = 0, t_long = 0;
    reg tx_d = 0, long_d = 0;
    always @(posedge clk) begin
        cyc = cyc + 1;
        if (bit_edge) t_edge = cyc;
        if (tx && !tx_d)  begin t_rise = cyc; last_delay = cyc - t_edge; end
        if (!tx && tx_d)  last_width = cyc - t_rise;
        if (tx_long && !long_d) t_long = cyc;
        if (!tx_long && long_d && cyc - t_long > long_width) long_width = cyc - t_long;   // longest one
        tx_d   = tx;
        long_d = tx_long;
    end

    integer errors = 0;
    task check(input cond, input [8*64-1:0] what);
        begin
            if (cond) $display("  ok    %0s", what);
            else begin $display("  FAIL  %0s", what); errors = errors + 1; end
        end
    endtask

    // A verdict in the middle of a bit (phase 40).
    task give_verdict(input [2:0] cls);
        begin
            while (phase != 40) @(posedge clk);
            verdict <= cls; verdict_valid <= 1;
            @(posedge clk);
            verdict_valid <= 0;
        end
    endtask

    task wait_bits(input integer n); repeat (n * 100) @(posedge clk); endtask

    initial begin
        repeat (5) @(posedge clk);
        rst <= 0;
        wait_bits(2);

        $display("1. Normal frame (class 0): no flag");
        give_verdict(0); wait_bits(10);
        check(n_rej == 0 && !tx, "no flag, TX released");

        $display("2. Attack (DoS) in time: flag at the next bit boundary, 6 bits long");
        give_verdict(1); wait_bits(10);
        check(n_rej == 1, "one flag sent");
        check(last_delay >= 0 && last_delay <= 2, "starts within 2 clocks of the bit boundary");
        check(last_width == 600, "600 clocks = 6 bit times dominant");
        $display("        (started %0d clocks after the boundary, %0d clocks long)", last_delay, last_width);

        $display("3. Allow list: ID 7FF, data CANST...: no flag");
        can_id <= 11'h7FF; data <= 64'h43414E5354415254;   // "CANSTART"
        give_verdict(2); wait_bits(10);
        check(n_rej == 1 && n_allow == 1, "no flag, counted in n_allow");
        can_id <= 11'h0A0; data <= 0;

        $display("4. Prevention off: no flag");
        enable <= 0;
        give_verdict(1); wait_bits(10);
        check(n_rej == 1 && !tx, "no flag");
        enable <= 1;

        $display("5. Verdict after the end of frame began: too late");
        before_eof <= 0;
        give_verdict(4); wait_bits(10);
        check(n_rej == 1 && n_ilate == 1, "no flag, counted in n_ilate");
        before_eof <= 1;

        $display("6. In time at the verdict, too late at the bit boundary");
        give_verdict(3);
        before_eof <= 0;                 // before the next boundary (60 clocks away)
        wait_bits(10);
        check(n_rej == 1 && n_ilate == 2, "no flag, counted in n_ilate");
        before_eof <= 1;

        $display("7. Prevention switched off during a flag: TX released at once");
        give_verdict(1);
        wait_bits(3);
        check(tx, "flag on the bus");
        enable <= 0;
        @(posedge clk); @(posedge clk);
        check(!tx, "released within 2 clocks");
        enable <= 1;
        wait_bits(10);

        $display("8. Safety cut: a 10-bit flag is cut at 8 bits");
        check(long_abort >= 1, "the long flag was cut (n_abort)");
        check(long_width == 800, "dominant for 800 clocks = 8 bits, no more");
        $display("        (longest flag of the 10-bit copy: %0d clocks)", long_width);
        check(n_abort == 0, "the real 6-bit flag never needs the cut");

        if (errors == 0) $display("PASS: ips_flag follows every rule");
        else             $display("FAIL: %0d checks failed", errors);
        $finish;
    end
endmodule
