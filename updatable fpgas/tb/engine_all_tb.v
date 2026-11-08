// Phase A: the engine against Python on a file of frames. The model is written
// straight into bank 0 (backdoor); Phase B's tb_spi_all does the same through
// the SPI port. One frame per clock.
//
//   vvp -n engine_all_sim +blob=model.hex +in=frames.hex +out=classes.txt
//
// Input: one frame per line, 20 hex digits, ID (3) DLC (1) data (16), as
// python/fpga_accuracy.py writes them. Output, one line per frame, in order:
// two digits, forest class then final class (after the key table rule).
`timescale 1ns/1ps
module engine_all_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg         rst = 1;
    reg         in_valid = 0;
    reg  [10:0] can_id = 0;
    reg  [3:0]  dlc = 0;
    reg  [63:0] data = 0;

    reg  [3:0]  feat_bytes = 0;
    reg  [7:0]  tree_en = 0;
    reg         rule_en = 0;
    reg  [2:0]  rule_from = 0, rule_to = 0;

    reg         wr_en = 0;
    reg  [2:0]  wr_tree = 0;
    reg  [3:0]  wr_level = 0;
    reg  [6:0]  wr_idx = 0;
    reg  [35:0] wr_data = 0;
    reg         kt_wr_en = 0;
    reg  [10:0] kt_wr_addr = 0;
    reg         kt_wr_data = 0;

    wire [255:0] fv;
    wire [10:0]  key;
    can_features fe (.can_id(can_id), .dlc(dlc), .data(data), .feat_bytes(feat_bytes),
                     .fv(fv), .key(key));

    wire        out_valid, out_bank, rd_ack, rd_valid, kt_rd_ack, kt_rd_valid, kt_rd_data;
    wire        out_tag;
    wire [2:0]  out_forest, out_final;
    wire [35:0] rd_data;

    rf_classifier dut (
        .clk(clk), .rst(rst),
        .in_valid(in_valid), .in_bank(1'b0), .in_tag(1'b0), .in_fv(fv), .in_key(key),
        .tree_en_b0(tree_en), .tree_en_b1(8'h00),
        .rule_en({1'b0, rule_en}), .rule_from_b0(rule_from), .rule_from_b1(3'd0),
        .rule_to_b0(rule_to), .rule_to_b1(3'd0),
        .wr_en(wr_en), .wr_tree(wr_tree), .wr_level(wr_level), .wr_bank(1'b0),
        .wr_idx(wr_idx), .wr_data(wr_data),
        .rd_req(1'b0), .rd_tree(3'd0), .rd_level(4'd0), .rd_bank(1'b0), .rd_idx(7'd0),
        .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .kt_wr_en(kt_wr_en), .kt_wr_bank(1'b0), .kt_wr_addr(kt_wr_addr), .kt_wr_data(kt_wr_data),
        .kt_rd_req(1'b0), .kt_rd_bank(1'b0), .kt_rd_addr(11'd0),
        .kt_rd_ack(kt_rd_ack), .kt_rd_valid(kt_rd_valid), .kt_rd_data(kt_rd_data),
        .out_valid(out_valid), .out_bank(out_bank), .out_tag(out_tag),
        .out_forest(out_forest), .out_final(out_final)
    );

    reg [7:0] blob [0:65535];
    reg [8*256-1:0] blob_name, in_name, out_name;
    integer fin, fout, r, n_in = 0, n_out = 0;
    integer t, l, i, off, n_trees;
    reg [79:0] line;

    task load_blob;
        begin
            n_trees    = blob[26];
            feat_bytes = blob[30];
            rule_en    = blob[31][0];
            rule_from  = blob[32][2:0];
            rule_to    = blob[33][2:0];
            tree_en    = (8'd1 << n_trees) - 8'd1;
            // key table: 256 bytes, one bit per key
            for (i = 0; i < 2048; i = i + 1) begin
                @(posedge clk);
                kt_wr_en   <= 1'b1;
                kt_wr_addr <= i[10:0];
                kt_wr_data <= blob[448 + (i >> 3)][i & 7];
            end
            @(posedge clk);
            kt_wr_en <= 1'b0;
            // nodes: tree, level, index
            off = 704;
            for (t = 0; t < 8; t = t + 1)
                for (l = 0; l < 12; l = l + 1)
                    for (i = 0; i < blob[80 + t*12 + l]; i = i + 1) begin
                        @(posedge clk);
                        wr_en    <= 1'b1;
                        wr_tree  <= t[2:0];
                        wr_level <= l[3:0];
                        wr_idx   <= i[6:0];
                        wr_data  <= {blob[off+4][3:0], blob[off+3], blob[off+2], blob[off+1], blob[off]};
                        off = off + 5;
                    end
            @(posedge clk);
            wr_en <= 1'b0;
        end
    endtask

    initial begin
        if (!$value$plusargs("in=%s", in_name) || !$value$plusargs("out=%s", out_name)
            || !$value$plusargs("blob=%s", blob_name)) begin
            $display("usage: vvp engine_all_sim +blob=<model.hex> +in=<frames.hex> +out=<classes.txt>");
            $finish;
        end
        $readmemh(blob_name, blob);
        fin  = $fopen(in_name, "r");
        fout = $fopen(out_name, "w");
        if (fin == 0 || fout == 0) begin
            $display("cannot open %0s or %0s", in_name, out_name);
            $finish;
        end
        repeat (3) @(posedge clk);
        rst <= 0;
        load_blob;
        repeat (3) @(posedge clk);
        while (!$feof(fin)) begin
            r = $fscanf(fin, "%h\n", line);
            if (r == 1) begin
                n_in = n_in + 1;
                @(posedge clk);
                in_valid <= 1'b1;
                can_id   <= line[78:68];
                dlc      <= line[67:64];
                data     <= line[63:0];
            end
        end
        @(posedge clk);
        in_valid <= 1'b0;
        repeat (20) @(posedge clk);
        $fclose(fout);
        $display("DONE in=%0d out=%0d", n_in, n_out);
        $finish;
    end

    always @(posedge clk) if (!rst && out_valid) begin
        $fdisplay(fout, "%0d%0d", out_forest, out_final);
        n_out = n_out + 1;
    end
endmodule
