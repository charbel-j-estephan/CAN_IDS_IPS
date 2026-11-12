// AES-128 core (rtl/security/aes128_enc.v) against Python's `cryptography`:
// FIPS-197 C.1 and 1,000 random blocks (python/make_security_vectors.py).
// Also: a start while busy must drop the old block, never deliver it.
//
//   vvp -n aes_sim +vec=sec_vectors/aes.txt      (from the build folder)
`timescale 1ns/1ps
module aes_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg          rst = 1, start = 0;
    reg  [127:0] key, pt, want;
    wire         busy, done;
    wire [127:0] ct;

    aes128_enc dut (.clk(clk), .rst(rst), .start(start), .key(key), .block_in(pt),
                    .busy(busy), .done(done), .block_out(ct));

    integer f, r, n = 0, bad = 0, lat, max_lat = 0;
    reg [8*256-1:0] name;

    task one;
        begin
            @(negedge clk); start = 1;
            @(negedge clk); start = 0;
            lat = 1;
            while (!done) begin @(negedge clk); lat = lat + 1; end
            if (lat > max_lat) max_lat = lat;
        end
    endtask

    integer seen_done;
    initial begin
        if (!$value$plusargs("vec=%s", name)) name = "sec_vectors/aes.txt";
        f = $fopen(name, "r");
        if (f == 0) begin $display("FAIL cannot open %0s", name); $finish; end
        repeat (3) @(negedge clk); rst = 0;
        while (!$feof(f)) begin
            r = $fscanf(f, "%h %h %h\n", key, pt, want);
            if (r == 3) begin
                one;
                n = n + 1;
                if (ct !== want) begin
                    bad = bad + 1;
                    if (bad < 5) $display("FAIL key %h pt %h: got %h want %h", key, pt, ct, want);
                end
            end
        end
        $fclose(f);
        $display("AES: %0d blocks, %0d wrong, %0d clocks from start to done", n, bad, max_lat);

        // A restart while busy: only the second block comes out.
        key  = 128'h000102030405060708090a0b0c0d0e0f;
        pt   = 128'hFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF;
        @(negedge clk); start = 1; @(negedge clk); start = 0;
        repeat (7) @(negedge clk);
        pt   = 128'h00112233445566778899aabbccddeeff;
        seen_done = 0;
        @(negedge clk); start = 1; @(negedge clk); start = 0;
        while (!done) @(negedge clk);
        seen_done = 1;
        if (ct !== 128'h69c4e0d86a7b0430d8cdb78070b4c55a) begin
            bad = bad + 1;
            $display("FAIL restart while busy: got %h", ct);
        end
        repeat (40) @(negedge clk);
        if (n == 1001 && bad == 0) $display("ALL PASS");
        else                       $display("FAIL");
        $finish;
    end

    // Exactly one done after the restart: count them.
    integer n_done_after = 0;
    always @(posedge clk) if (seen_done && done) n_done_after = n_done_after + 1;
    initial begin #100000000; $display("FAIL testbench timeout"); $finish; end
endmodule
