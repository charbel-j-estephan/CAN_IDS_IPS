// One level of one tree: its node RAM and its comparator.
//
// Both model banks live in this one RAM, the bank is the top address bit.
//
// A node is {feature, threshold, left, right}; left and right are each
// {is_leaf, index or class vote}. Rule, as sklearn: go left when
// feature <= threshold.
//
// Timing. On a clock edge the RAM captures `addr` and this level captures the
// feature vector and the state from the level above. During the following
// clock the node comes out of the RAM and the comparison runs, and the answer
// (done_o, val_o) goes straight to the next level's address input. That is one
// clock per level.
//
// A tree that already reached a leaf has done_i = 1: the RAM output is ignored
// and the vote passes down unchanged, so shallow branches work.
module tree_level #(
    parameter N_FEAT_MAX = 16,
    parameter THRESH_W   = 16,
    parameter IDX_W      = 7,            // NODES_PER_LEVEL_MAX = 2**IDX_W
    parameter FEAT_W     = 4,
    parameter NODE_W     = 36
) (
    input  wire                           clk,
    // loader write port (top address bit = bank)
    input  wire                           wr_en,
    input  wire [IDX_W:0]                 wr_addr,
    input  wire [NODE_W-1:0]              wr_data,
    // RAM read address, captured on this clock edge
    input  wire [IDX_W:0]                 addr,
    // state from the level above, captured on this clock edge
    input  wire                           done_i,
    input  wire [IDX_W-1:0]               val_i,
    // the feature vector, already captured by the engine for this level
    input  wire [N_FEAT_MAX*THRESH_W-1:0] fv_q,
    // the node read from the RAM (also used by the shadow bank read path)
    output wire [NODE_W-1:0]              node_q,
    // the answer for the next level
    output wire                           done_o,
    output wire [IDX_W-1:0]               val_o
);
    sdp_ram #(.DATA_W(NODE_W), .ADDR_W(IDX_W + 1)) ram (
        .clk(clk), .we(wr_en), .waddr(wr_addr), .wdata(wr_data),
        .raddr(addr), .rdata(node_q)
    );

    reg             done_q;
    reg [IDX_W-1:0] val_q;
    always @(posedge clk) begin
        done_q <= done_i;
        val_q  <= val_i;
    end

    wire [FEAT_W-1:0]   feat  = node_q[NODE_W-1 -: FEAT_W];
    wire [THRESH_W-1:0] thr   = node_q[2*(IDX_W+1) +: THRESH_W];
    wire [IDX_W:0]      left  = node_q[IDX_W+1 +: IDX_W+1];
    wire [IDX_W:0]      right = node_q[0 +: IDX_W+1];

    wire [THRESH_W-1:0] x       = fv_q[feat * THRESH_W +: THRESH_W];
    wire                go_left = (x <= thr);
    wire [IDX_W:0]      chosen  = go_left ? left : right;

    assign done_o = done_q ? 1'b1  : chosen[IDX_W];
    assign val_o  = done_q ? val_q : chosen[IDX_W-1:0];
endmodule
