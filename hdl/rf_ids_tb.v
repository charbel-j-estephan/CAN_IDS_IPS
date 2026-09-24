// Testbench for the generated rf_ids module.
// Streams every frame from generated/vectors.hex, one per clock, and checks
// the class the FPGA logic returns against the Python hard vote reference.
//
// Run from the repo root:
//   iverilog -g2005 -I hdl/generated -o build/rf_ids_sim hdl/rf_ids_tb.v hdl/generated/rf_ids.v
//   vvp build/rf_ids_sim            (add +vcd to write build/rf_ids.vcd for GTKWave)
`timescale 1ns/1ps
module rf_ids_tb;
    `include "rf_ids_params.vh"

    reg clk = 0;
    always #10 clk = ~clk;   // 50 MHz, like the DE10 Lite oscillator

    reg         rst = 1;
    reg         in_valid = 0;
    reg  [10:0] can_id = 0;
    reg  [3:0]  dlc = 0;
    reg  [63:0] data = 0;
    wire        out_valid;
    wire [CLS_BITS-1:0] class_out;

    rf_ids dut (
        .clk(clk), .rst(rst), .in_valid(in_valid),
        .can_id(can_id), .dlc(dlc), .data(data),
        .out_valid(out_valid), .class_out(class_out)
    );

    // can_id(12 bits in hex) dlc(4) data(64) expected(8) = 88 bits per line.
    reg [87:0] vec [0:N_VECTORS-1];
    integer sent = 0, checked = 0, errors = 0;
    integer first_out = -1, cycle = 0, first_in = -1;

    initial begin
        $readmemh("hdl/generated/vectors.hex", vec);
        if ($test$plusargs("vcd")) begin
            $dumpfile("build/rf_ids.vcd");
            $dumpvars(0, rf_ids_tb);
        end
        repeat (3) @(posedge clk);
        rst <= 0;
        while (sent < N_VECTORS) begin
            @(posedge clk);
            in_valid <= 1;
            {can_id, dlc, data} <= {vec[sent][86:76], vec[sent][75:72], vec[sent][71:8]};
            sent = sent + 1;
        end
        @(posedge clk);
        in_valid <= 0;
        repeat (LATENCY + 2) @(posedge clk);
        if (checked != N_VECTORS) begin
            $display("FAIL: only %0d of %0d results came out", checked, N_VECTORS);
            errors = errors + 1;
        end
        if (errors == 0)
            $display("PASS: %0d frames, all match Python. Latency %0d clocks.",
                     checked, first_out);
        else
            $display("FAIL: %0d mismatches out of %0d frames", errors, N_VECTORS);
        $finish;
    end

    // Count clocks from the first accepted frame to its result.
    always @(posedge clk) begin
        cycle <= cycle + 1;
        if (in_valid && first_in < 0) first_in = cycle;
        if (out_valid) begin
            if (checked == 0) first_out = cycle - first_in;
            if (class_out !== vec[checked][CLS_BITS-1:0]) begin
                if (errors < 10)
                    $display("mismatch frame %0d: got %0d expected %0d",
                             checked, class_out, vec[checked][CLS_BITS-1:0]);
                errors = errors + 1;
            end
            checked = checked + 1;
        end
    end
endmodule
