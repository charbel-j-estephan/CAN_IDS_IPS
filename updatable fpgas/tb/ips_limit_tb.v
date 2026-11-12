// The IPS rate limit (rtl/frontend_can/ips_flag.v, LIMIT_FLAGS / LIMIT_WINDOW).
// Two copies side by side get the same attack verdicts: the default one (no
// limit) must flag every one, the limited one (2 per window) only the first
// two in each window, counting the rest in n_limit.
//
//   iverilog -g2005 -o build/ips_limit_sim tb/ips_limit_tb.v rtl/frontend_can/ips_flag.v
//   vvp -n build/ips_limit_sim
`timescale 1ns/1ps
module ips_limit_tb;
    localparam WINDOW = 20000;       // clocks: room for 5 verdicts of 3000 clocks each and more

    reg clk = 0;
    always #10 clk = ~clk;

    reg        rst = 1, verdict_valid = 0, before_eof = 0, bit_edge = 0;
    reg [2:0]  verdict = 0;
    wire       tx_free, tx_lim, fs_free, fs_lim;
    wire [31:0] rej_free, rej_lim, lim_free, lim_lim;

    ips_flag free (
        .clk(clk), .rst(rst), .enable(1'b1), .verdict_valid(verdict_valid), .verdict(verdict),
        .can_id(11'h123), .data(64'd0), .before_eof(before_eof), .bit_edge(bit_edge),
        .tx_dominant(tx_free), .flag_start(fs_free), .n_rej(rej_free), .n_ilate(), .n_allow(),
        .n_abort(), .n_limit(lim_free)
    );
    ips_flag #(.LIMIT_FLAGS(2), .LIMIT_WINDOW(WINDOW)) lim (
        .clk(clk), .rst(rst), .enable(1'b1), .verdict_valid(verdict_valid), .verdict(verdict),
        .can_id(11'h123), .data(64'd0), .before_eof(before_eof), .bit_edge(bit_edge),
        .tx_dominant(tx_lim), .flag_start(fs_lim), .n_rej(rej_lim), .n_ilate(), .n_allow(),
        .n_abort(), .n_limit(lim_lim)
    );

    // A bit edge every 100 clocks, as can_rx at 500 kbit/s.
    integer bc = 0;
    always @(posedge clk) begin
        bc <= (bc == 99) ? 0 : bc + 1;
        bit_edge <= (bc == 99);
    end

    integer checks = 0, fails = 0;
    task check;
        input [8*60:1] name;
        input          cond;
        begin
            checks = checks + 1;
            if (cond) $display("PASS %0s", name);
            else begin $display("FAIL %0s", name); fails = fails + 1; end
        end
    endtask

    // One attack verdict on a frame still on the bus, then 3000 clocks for the
    // 6 bit flag (600 clocks) to finish.
    task attack;
        begin
            @(negedge clk);
            before_eof = 1; verdict = 3'd1; verdict_valid = 1;
            @(negedge clk);
            verdict_valid = 0;
            repeat (3000) @(negedge clk);
            before_eof = 0;
        end
    endtask

    integer k;
    initial begin
        repeat (5) @(negedge clk);
        rst = 0;
        @(negedge clk);

        for (k = 0; k < 5; k = k + 1) attack;          // 15,000 clocks: one window
        check("no limit: 5 of 5 flagged", rej_free == 5 && lim_free == 0);
        check("limit 2: 2 of 5 flagged", rej_lim == 2);
        check("limit 2: 3 counted in n_limit", lim_lim == 3);

        while (lim.win_cnt != 0) @(negedge clk);       // the next window starts
        repeat (5) @(negedge clk);
        for (k = 0; k < 3; k = k + 1) attack;
        check("next window: 2 more flagged", rej_lim == 4);
        check("next window: 1 more limited", lim_lim == 4);
        check("no limit: all 8 flagged", rej_free == 8);
        check("TX released at the end", !tx_free && !tx_lim);

        $display("%0d checks, %0d failed", checks, fails);
        if (fails == 0) $display("ALL PASS");
        $finish;
    end
endmodule
