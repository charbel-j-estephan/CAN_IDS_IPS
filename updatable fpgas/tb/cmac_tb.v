// The seal checker (rtl/security/cmac_stream.v) against Python's `cryptography`
// AES-CMAC: RFC 4493 examples 2-4 (16, 40, 64 bytes: the K1 path for 16 and
// 64, the padded K2 path for 40), the three real blob lengths and random
// lengths (python/make_security_vectors.py). Bytes are fed with random gaps,
// honouring `ready`, then 16 extra bytes as the MAC field would be. Also: a
// restart in the middle of a message must not leak the old message's tag.
//
//   vvp -n cmac_sim +vec=sec_vectors/cmac.txt      (from the build folder)
`timescale 1ns/1ps
module cmac_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg          rst = 1, restart = 0, in_valid = 0;
    reg  [7:0]   in_byte = 0;
    reg  [127:0] key = 0;
    reg  [15:0]  msg_len = 0;
    wire         ready, tag_done, tag_strobe;
    wire [127:0] tag;

    cmac_stream dut (.clk(clk), .key(key), .rst(rst), .restart(restart), .msg_len(msg_len),
                     .len_valid(1'b1), .in_valid(in_valid), .in_byte(in_byte), .ready(ready),
                     .tag(tag), .tag_done(tag_done), .tag_strobe(tag_strobe));

    reg [7:0] msg [0:8191];
    integer f, r, n, i, c, hi, lo, cases = 0, bad = 0, seed = 7;
    reg [127:0] want;

    function integer hexval;
        input integer ch;
        hexval = (ch >= "a") ? ch - "a" + 10 : (ch >= "A") ? ch - "A" + 10 : ch - "0";
    endfunction

    task send;                       // one byte, waiting for ready, then a random gap
        input [7:0] b;
        begin
            @(negedge clk);
            while (!ready) @(negedge clk);
            in_byte = b; in_valid = 1;
            @(negedge clk);
            in_valid = 0;
            repeat ($unsigned($random(seed)) % 3) @(negedge clk);
        end
    endtask

    task run_msg;
        integer k;
        begin
            @(negedge clk); rst = 1; @(negedge clk); rst = 0;   // new key: new subkeys
            @(negedge clk); restart = 1; @(negedge clk); restart = 0;
            for (k = 0; k < n; k = k + 1) send(msg[k]);
            for (k = 0; k < 16; k = k + 1) send(8'hEE);        // the MAC field: ignored
            k = 0;
            while (!tag_done && k < 100) begin @(negedge clk); k = k + 1; end
        end
    endtask

    reg [8*256-1:0] name;
    initial begin
        if (!$value$plusargs("vec=%s", name)) name = "sec_vectors/cmac.txt";
        f = $fopen(name, "r");
        if (f == 0) begin $display("FAIL cannot open %0s", name); $finish; end
        while (!$feof(f)) begin
            r = $fscanf(f, "%h %d %h\n", key, n, want);
            if (r == 3) begin
                for (i = 0; i < n; i = i + 1) begin
                    hi = $fgetc(f); lo = $fgetc(f);
                    msg[i] = hexval(hi) * 16 + hexval(lo);
                end
                c = $fgetc(f);                                     // end of line
                while (c == 13) c = $fgetc(f);
                msg_len = n;
                run_msg;
                cases = cases + 1;
                if (!tag_done || tag !== want) begin
                    bad = bad + 1;
                    $display("FAIL length %0d: got %h (done %b) want %h", n, tag, tag_done, want);
                end else
                    $display("PASS length %0d", n);
            end
        end
        $fclose(f);

        // Restart in the middle: feed 30 bytes of a 40 byte message, restart,
        // then the RFC 40 byte message whole. Only its own tag may come out.
        key = 128'h2b7e151628aed2a6abf7158809cf4f3c;
        @(negedge clk); rst = 1; @(negedge clk); rst = 0;
        msg_len = 40;
        @(negedge clk); restart = 1; @(negedge clk); restart = 0;
        for (i = 0; i < 30; i = i + 1) send(8'h55);
        {msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], msg[6], msg[7],
         msg[8], msg[9], msg[10], msg[11], msg[12], msg[13], msg[14], msg[15]} = 128'h6bc1bee22e409f96e93d7e117393172a;
        {msg[16], msg[17], msg[18], msg[19], msg[20], msg[21], msg[22], msg[23],
         msg[24], msg[25], msg[26], msg[27], msg[28], msg[29], msg[30], msg[31]} = 128'hae2d8a571e03ac9c9eb76fac45af8e51;
        {msg[32], msg[33], msg[34], msg[35], msg[36], msg[37], msg[38], msg[39]} = 64'h30c81c46a35ce411;
        @(negedge clk); restart = 1; @(negedge clk); restart = 0;
        for (i = 0; i < 40; i = i + 1) send(msg[i]);
        i = 0;
        while (!tag_done && i < 100) begin @(negedge clk); i = i + 1; end
        cases = cases + 1;
        if (tag !== 128'hdfa66747de9ae63030ca32611497c827) begin
            bad = bad + 1; $display("FAIL restart mid message: got %h", tag);
        end else $display("PASS restart mid message");

        $display("CMAC: %0d cases, %0d wrong", cases, bad);
        if (bad == 0) $display("ALL PASS");
        $finish;
    end
    initial begin #2000000000; $display("FAIL testbench timeout"); $finish; end
endmodule
