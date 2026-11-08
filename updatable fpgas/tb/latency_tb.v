// Measures the engine: latency in clocks, one frame per clock, and the read
// path giving way to frames. Run from the build folder; prints PASS or FAIL.
//
//   vvp -n latency_sim +blob=blobs/p5/model.hex
`timescale 1ns/1ps
module latency_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg         rst = 1, in_valid = 0, wr_en = 0, rd_req = 0;
    reg  [255:0] fv = 0;
    reg  [2:0]  wr_tree = 0, rd_tree = 0;
    reg  [3:0]  wr_level = 0, rd_level = 0;
    reg  [6:0]  wr_idx = 0, rd_idx = 0;
    reg  [35:0] wr_data = 0;
    reg  [7:0]  tree_en = 0;
    wire        out_valid, out_bank, out_tag, rd_ack, rd_valid, kt_a, kt_v, kt_d;
    wire [2:0]  out_forest, out_final;
    wire [35:0] rd_data;

    rf_classifier dut (
        .clk(clk), .rst(rst), .in_valid(in_valid), .in_bank(1'b0), .in_tag(1'b0), .in_fv(fv),
        .in_key(11'd0), .tree_en_b0(tree_en), .tree_en_b1(8'h00), .rule_en(2'b00),
        .rule_from_b0(3'd0), .rule_from_b1(3'd0), .rule_to_b0(3'd0), .rule_to_b1(3'd0),
        .wr_en(wr_en), .wr_tree(wr_tree), .wr_level(wr_level), .wr_bank(1'b0), .wr_idx(wr_idx),
        .wr_data(wr_data), .rd_req(rd_req), .rd_tree(rd_tree), .rd_level(rd_level),
        .rd_bank(1'b0), .rd_idx(rd_idx), .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .kt_wr_en(1'b0), .kt_wr_bank(1'b0), .kt_wr_addr(11'd0), .kt_wr_data(1'b0),
        .kt_rd_req(1'b0), .kt_rd_bank(1'b0), .kt_rd_addr(11'd0), .kt_rd_ack(kt_a),
        .kt_rd_valid(kt_v), .kt_rd_data(kt_d),
        .out_valid(out_valid), .out_bank(out_bank), .out_tag(out_tag),
        .out_forest(out_forest), .out_final(out_final)
    );

    reg [7:0] blob [0:65535];
    reg [8*256-1:0] blob_name;
    integer t, l, i, off, n, cycle = 0, sent_at = 0, got_at = -1, fails = 0, outs = 0;

    always @(negedge clk) cycle = cycle + 1;   // no race with the edge-sampled uses below
    always @(posedge clk) if (out_valid) begin
        outs = outs + 1;
        if (got_at < 0) got_at = cycle;
    end

    initial begin
        if (!$value$plusargs("blob=%s", blob_name)) $finish;
        $readmemh(blob_name, blob);
        tree_en = (8'd1 << blob[26]) - 8'd1;
        repeat (3) @(posedge clk);
        rst <= 0;
        off = 704;
        for (t = 0; t < 8; t = t + 1)
            for (l = 0; l < 12; l = l + 1) begin
                n = blob[80 + t*12 + l];
                for (i = 0; i < n; i = i + 1) begin
                    @(posedge clk);
                    wr_en <= 1; wr_tree <= t[2:0]; wr_level <= l[3:0]; wr_idx <= i[6:0];
                    wr_data <= {blob[off+4][3:0], blob[off+3], blob[off+2], blob[off+1], blob[off]};
                    off = off + 5;
                end
            end
        @(posedge clk); wr_en <= 0;
        repeat (3) @(posedge clk);

        // one frame: latency
        @(posedge clk); in_valid <= 1;
        @(posedge clk); in_valid <= 0; sent_at = cycle;
        repeat (30) @(posedge clk);
        $display("latency: %0d clocks (frame sampled on the edge at cycle %0d, verdict visible after cycle %0d)",
                 got_at - sent_at, sent_at, got_at);
        if (got_at - sent_at != 15) fails = fails + 1;

        // 100 frames back to back: one frame per clock
        outs = 0;
        @(posedge clk); in_valid <= 1;
        repeat (100) @(posedge clk);
        in_valid <= 0;
        repeat (30) @(posedge clk);
        $display("back to back: %0d frames in, %0d verdicts out", 100, outs);
        if (outs != 100) fails = fails + 1;

        // a read request must wait for a level with no frame and then return the node
        rd_tree <= 3'd0; rd_level <= 4'd0; rd_idx <= 7'd0;
        @(posedge clk); in_valid <= 1; rd_req <= 1;
        repeat (3) begin
            @(negedge clk);
            if (rd_ack) begin $display("FAIL: read taken while a frame needs the level"); fails = fails + 1; end
        end
        @(posedge clk); in_valid <= 0;
        @(posedge clk);
        wait (rd_ack); @(posedge clk); rd_req <= 0;
        wait (rd_valid);
        @(negedge clk);
        if (rd_data !== {blob[704+4][3:0], blob[704+3], blob[704+2], blob[704+1], blob[704]}) begin
            $display("FAIL: read back %h", rd_data); fails = fails + 1;
        end
        if (fails == 0) $display("PASS"); else $display("FAIL (%0d)", fails);
        $finish;
    end
endmodule
