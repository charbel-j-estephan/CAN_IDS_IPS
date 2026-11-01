// Runs a file of frames through the FPGA's classifier: rf_ids.v (the forest)
// and known_id.v (the unknown ID rule), one frame per clock, and writes both
// answers per frame. Used by python/fpga_accuracy.py, which splits the whole
// dataset into chunks and runs one copy of this per CPU core.
//
//   iverilog -g2005 -o build/rf_all_sim hdl/rf_all_tb.v hdl/generated/rf_ids.v hdl/known_id.v
//   vvp -n build/rf_all_sim +in=frames.hex +out=classes.txt
//
// Input: one frame per line, 20 hex digits: ID (3) DLC (1) data bytes 0-7 (16),
// exactly what the forest sees at feat_ready (DLC at most 8, bytes past the
// DLC 0, and the bytes after the ones the model reads 0). Output: one line per frame, in the same order, two digits:
// forest class, final class (after the unknown ID rule). 0 Normal, 1 DoS,
// 2 Fuzzy, 3 Gear_spoof, 4 RPM_spoof.
`timescale 1ns/1ps
module rf_all_tb;
    reg clk = 0;
    always #10 clk = ~clk;

    reg         rst = 1;
    reg         in_valid = 0;
    reg  [10:0] can_id = 0;
    reg  [3:0]  dlc = 0;
    reg  [63:0] data = 0;
    wire        out_valid;
    wire [2:0]  class_out;

    rf_ids forest (
        .clk(clk), .rst(rst), .in_valid(in_valid),
        .can_id(can_id), .dlc(dlc), .data(data),
        .out_valid(out_valid), .class_out(class_out)
    );

    // Results come out in input order, so the ID of the frame now leaving the
    // forest is ids[n_out]. The rule as in classify.c: Normal + unknown -> Fuzzy.
    reg  [10:0] ids [0:(1 << 22) - 1];    // up to 4 M frames per chunk
    integer     n_in = 0, n_out = 0;
    wire [10:0] id_out = ids[n_out];
    wire        known;
    known_id table_lookup (.can_id(id_out), .known(known));
    wire [2:0]  final_class = (class_out == 3'd0 && !known) ? 3'd2 : class_out;

    reg [8*256-1:0] in_name, out_name;
    integer fin, fout, r;
    reg [79:0] line;

    initial begin
        if (!$value$plusargs("in=%s", in_name) || !$value$plusargs("out=%s", out_name)) begin
            $display("usage: vvp rf_all_sim +in=<frames.hex> +out=<classes.txt>");
            $finish;
        end
        fin  = $fopen(in_name, "r");
        fout = $fopen(out_name, "w");
        if (fin == 0 || fout == 0) begin
            $display("cannot open %0s or %0s", in_name, out_name);
            $finish;
        end
        repeat (3) @(posedge clk);
        rst <= 0;
        while (!$feof(fin)) begin
            r = $fscanf(fin, "%h\n", line);
            if (r == 1) begin
                ids[n_in] = line[78:68];
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
        repeat (10) @(posedge clk);
        $fclose(fout);
        $display("DONE in=%0d out=%0d", n_in, n_out);
        $finish;
    end

    always @(posedge clk) if (!rst && out_valid) begin
        $fdisplay(fout, "%0d%0d", class_out, final_class);
        n_out = n_out + 1;
    end
endmodule
