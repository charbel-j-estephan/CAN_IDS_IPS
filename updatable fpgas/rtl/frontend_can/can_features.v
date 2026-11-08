// CAN feature extractor: turns a decoded frame into the engine's feature vector.
// This and the other files in rtl/frontend_can are the only CAN specific logic.
// A new protocol (LIN, Modbus, automotive Ethernet) replaces this folder and
// leaves rtl/engine alone.
//
// Slots of the feature vector, as python/dataset.py FEATURES:
//   0 CAN ID   1 DLC (at most 8)   2..9 data bytes 0..7   10..15 always 0
// Only the first feat_bytes data bytes are passed on; the rest read 0, the
// same as classify.c on the STM32 (they are not on the bus yet at feat_ready).
module can_features #(
    parameter N_FEAT_MAX = 16,
    parameter THRESH_W   = 16
) (
    input  wire [10:0]                    can_id,
    input  wire [3:0]                     dlc,
    input  wire [63:0]                    data,       // byte 0 in data[63:56]
    input  wire [3:0]                     feat_bytes, // 0..8, from the active model
    output wire [N_FEAT_MAX*THRESH_W-1:0] fv,
    output wire [10:0]                    key
);
    localparam PAD_ID  = THRESH_W - 11;
    localparam PAD_DLC = THRESH_W - 4;
    localparam PAD_B   = THRESH_W - 8;

    assign key = can_id;

    wire [3:0] dlc_cap = (dlc > 4'd8) ? 4'd8 : dlc;

    assign fv[0*THRESH_W +: THRESH_W] = {{PAD_ID{1'b0}},  can_id};
    assign fv[1*THRESH_W +: THRESH_W] = {{PAD_DLC{1'b0}}, dlc_cap};

    genvar i;
    generate
        for (i = 0; i < 8; i = i + 1) begin : bytes
            wire [7:0] b = data[63 - 8*i -: 8];
            assign fv[(2+i)*THRESH_W +: THRESH_W] =
                {{PAD_B{1'b0}}, (i < feat_bytes) ? b : 8'h00};
        end
        for (i = 10; i < N_FEAT_MAX; i = i + 1) begin : unused
            assign fv[i*THRESH_W +: THRESH_W] = {THRESH_W{1'b0}};
        end
    endgenerate
endmodule
