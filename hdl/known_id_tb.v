// Checks known_id.v on all 2048 IDs against the model's own list
// (build/sim/known_ids.mem, written by python/make_sim_frames.py from
// bundle["known_ids"]), so a typo in the case statement shows up here.
//
//   iverilog -g2005 -o build/known_id_sim hdl/known_id_tb.v hdl/known_id.v
//   vvp build/known_id_sim
`timescale 1ns/1ps
module known_id_tb;
    reg  [10:0] can_id = 0;
    wire        known;
    known_id dut (.can_id(can_id), .known(known));

    reg expect_mem [0:2047];
    integer i, errors = 0, n_known = 0;

    initial begin
        $readmemb("build/sim/known_ids.mem", expect_mem);
        for (i = 0; i < 2048; i = i + 1) begin
            can_id = i;
            #1;
            if (known) n_known = n_known + 1;
            if (known !== expect_mem[i]) begin
                errors = errors + 1;
                if (errors <= 10)
                    $display("ID %03h: known_id says %0d, the model says %0d", i[10:0], known, expect_mem[i]);
            end
        end
        if (errors == 0)
            $display("PASS: all 2048 IDs agree with the model (%0d known)", n_known);
        else
            $display("FAIL: %0d IDs disagree", errors);
        $finish;
    end
endmodule
