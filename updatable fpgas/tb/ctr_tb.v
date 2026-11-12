// The scrambling keystream (rtl/security/ctr_stream.v) against Python's
// `cryptography`: blocks 0, 1 and 2 for two keys and IVs, so the IV's byte
// order and the big endian block number are both checked here, not only far
// downstream as a failed self test. Also: a restart in the middle goes back to
// block 0.
//
//   vvp -n ctr_sim +vec=sec_vectors/ctr.txt      (from the build folder)
`timescale 1ns/1ps
module ctr_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg          rst = 1, restart = 0, take = 0;
    reg  [127:0] key = 0;
    reg  [95:0]  iv = 0;
    wire         ks_valid;
    wire [7:0]   ks_byte;

    ctr_stream dut (.clk(clk), .key(key), .rst(rst), .restart(restart), .iv(iv), .take(take),
                    .ks_valid(ks_valid), .ks_byte(ks_byte));

    reg  [127:0] want [0:5];
    reg  [127:0] k_of [0:5];
    reg  [95:0]  iv_of [0:5];
    integer f, r, blk, i, j, bad = 0, n = 0;
    reg [127:0] got;

    task next_block;              // take 16 bytes, gathering them into got
        begin
            for (j = 0; j < 16; j = j + 1) begin
                @(negedge clk);
                while (!ks_valid) @(negedge clk);
                got[127 - 8*j -: 8] = ks_byte;
                take = 1;
                @(negedge clk);
                take = 0;
            end
        end
    endtask

    reg [8*256-1:0] name;
    initial begin
        if (!$value$plusargs("vec=%s", name)) name = "sec_vectors/ctr.txt";
        f = $fopen(name, "r");
        if (f == 0) begin $display("FAIL cannot open %0s", name); $finish; end
        for (i = 0; i < 6; i = i + 1) r = $fscanf(f, "%h %h %d %h\n", k_of[i], iv_of[i], blk, want[i]);
        $fclose(f);
        repeat (3) @(negedge clk); rst = 0;

        for (i = 0; i < 6; i = i + 3) begin
            key = k_of[i]; iv = iv_of[i];
            @(negedge clk); restart = 1; @(negedge clk); restart = 0;
            for (blk = 0; blk < 3; blk = blk + 1) begin
                next_block;
                n = n + 1;
                if (got !== want[i + blk]) begin
                    bad = bad + 1;
                    $display("FAIL key %h iv %h block %0d: got %h want %h", key, iv, blk, got, want[i + blk]);
                end else $display("PASS block %0d", blk);
            end
        end

        // Restart after 5 bytes: the stream starts again at block 0.
        key = k_of[0]; iv = iv_of[0];
        @(negedge clk); restart = 1; @(negedge clk); restart = 0;
        for (j = 0; j < 5; j = j + 1) begin
            @(negedge clk); while (!ks_valid) @(negedge clk);
            take = 1; @(negedge clk); take = 0;
        end
        @(negedge clk); restart = 1; @(negedge clk); restart = 0;
        next_block;
        n = n + 1;
        if (got !== want[0]) begin bad = bad + 1; $display("FAIL restart: got %h", got); end
        else $display("PASS restart goes back to block 0");

        $display("CTR: %0d blocks, %0d wrong", n, bad);
        if (bad == 0) $display("ALL PASS");
        $finish;
    end
    initial begin #50000000; $display("FAIL testbench timeout"); $finish; end
endmodule
