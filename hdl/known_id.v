// The unknown ID rule's table: is this CAN ID one the car sent in the normal
// training traffic? The FPGA version of can_ids_id_known() in can_ids_model.h
// (model rf5-d9-p5-2ddda531, 38 IDs).
//
// Used after the forest, as classify.c does:
//   final = (forest == Normal && !known) ? Fuzzy : forest
// With 9 comparisons per tree the forest cannot pick out 38 IDs among 2048,
// so most fuzzing frames it misses carry an ID the car never uses.
//
// Combinational: a case statement on 11 bits becomes a small lookup table,
// the answer is there as soon as the ID is. 0x000 (the DoS ID) is not known;
// DoS keeps its class because the rule only changes Normal verdicts.
//
// If the model is retrained, the list changes: take it from the new
// can_ids_model.h (or bundle["known_ids"]) and rerun hdl/known_id_tb.v.
//
// Test: iverilog -g2005 -o build/known_id_sim hdl/known_id_tb.v hdl/known_id.v
module known_id (
    input  wire [10:0] can_id,
    output reg         known
);
    always @(*) begin
        case (can_id)
            11'h002, 11'h0A0, 11'h0A1, 11'h105, 11'h130, 11'h131, 11'h140, 11'h153,
            11'h18F, 11'h1F1, 11'h260, 11'h2A0, 11'h2B0, 11'h2C0, 11'h316, 11'h329,
            11'h350, 11'h370, 11'h430, 11'h43F, 11'h440, 11'h4B1, 11'h4F0, 11'h545,
            11'h5A0, 11'h5A2, 11'h5F0, 11'h608, 11'h690, 11'h6A1, 11'h6AA, 11'h71B,
            11'h7CF, 11'h7D9, 11'h7DA, 11'h7DE, 11'h7E8, 11'h7E9:
                known = 1'b1;
            default:
                known = 1'b0;
        endcase
    end
endmodule
