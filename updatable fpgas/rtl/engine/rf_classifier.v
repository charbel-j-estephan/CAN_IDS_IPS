// The forest engine plus the key table rule, still protocol free.
//
// The key table is a 2048 bit bitmap, one bit per key value, in updatable
// memory (both banks in one M9K). A front end gives each frame a key; for CAN
// the key is the 11 bit ID. The rule, set by the model's config:
//   final = (rule_en && forest == rule_from && !known) ? rule_to : forest
// With the CAN models: Normal (0) on an ID the car never sent becomes Fuzzy (2).
//
// LATENCY = DEPTH_MAX + 3 clocks, the same as the engine.
module rf_classifier #(
    parameter N_TREES_MAX         = 8,
    parameter DEPTH_MAX           = 12,
    parameter N_FEAT_MAX          = 16,
    parameter THRESH_W            = 16,
    parameter NODES_PER_LEVEL_MAX = 128,
    parameter N_CLASS_MAX         = 8,
    parameter TAG_W               = 1,
    parameter KEY_W               = 11
) (
    clk, rst,
    in_valid, in_bank, in_tag, in_fv, in_key,
    tree_en_b0, tree_en_b1, rule_en, rule_from_b0, rule_from_b1, rule_to_b0, rule_to_b1,
    wr_en, wr_tree, wr_level, wr_bank, wr_idx, wr_data,
    rd_req, rd_tree, rd_level, rd_bank, rd_idx, rd_ack, rd_valid, rd_data,
    kt_wr_en, kt_wr_bank, kt_wr_addr, kt_wr_data,
    kt_rd_req, kt_rd_bank, kt_rd_addr, kt_rd_ack, kt_rd_valid, kt_rd_data,
    out_valid, out_bank, out_tag, out_forest, out_final
);
    localparam IDX_W   = $clog2(NODES_PER_LEVEL_MAX);
    localparam FEAT_W  = $clog2(N_FEAT_MAX);
    localparam NODE_W  = FEAT_W + THRESH_W + 2 * (IDX_W + 1);
    localparam TREE_W  = $clog2(N_TREES_MAX);
    localparam LEVEL_W = $clog2(DEPTH_MAX);
    localparam CLS_W   = $clog2(N_CLASS_MAX);
    localparam FV_W    = N_FEAT_MAX * THRESH_W;
    localparam LATENCY = DEPTH_MAX + 3;

    input  wire                   clk, rst;
    input  wire                   in_valid, in_bank;
    input  wire [TAG_W-1:0]       in_tag;
    input  wire [FV_W-1:0]        in_fv;
    input  wire [KEY_W-1:0]       in_key;
    input  wire [N_TREES_MAX-1:0] tree_en_b0, tree_en_b1;
    input  wire [1:0]             rule_en;           // one bit per bank
    input  wire [CLS_W-1:0]       rule_from_b0, rule_from_b1, rule_to_b0, rule_to_b1;
    input  wire                   wr_en;
    input  wire [TREE_W-1:0]      wr_tree;
    input  wire [LEVEL_W-1:0]     wr_level;
    input  wire                   wr_bank;
    input  wire [IDX_W-1:0]       wr_idx;
    input  wire [NODE_W-1:0]      wr_data;
    input  wire                   rd_req;
    input  wire [TREE_W-1:0]      rd_tree;
    input  wire [LEVEL_W-1:0]     rd_level;
    input  wire                   rd_bank;
    input  wire [IDX_W-1:0]       rd_idx;
    output wire                   rd_ack, rd_valid;
    output wire [NODE_W-1:0]      rd_data;
    input  wire                   kt_wr_en, kt_wr_bank;
    input  wire [KEY_W-1:0]       kt_wr_addr;
    input  wire                   kt_wr_data;
    input  wire                   kt_rd_req, kt_rd_bank;
    input  wire [KEY_W-1:0]       kt_rd_addr;
    output wire                   kt_rd_ack;
    output reg                    kt_rd_valid;
    output wire                   kt_rd_data;
    output wire                   out_valid, out_bank;
    output wire [TAG_W-1:0]       out_tag;
    output wire [CLS_W-1:0]       out_forest;
    output wire [CLS_W-1:0]       out_final;

    rf_engine #(.N_TREES_MAX(N_TREES_MAX), .DEPTH_MAX(DEPTH_MAX), .N_FEAT_MAX(N_FEAT_MAX),
                .THRESH_W(THRESH_W), .NODES_PER_LEVEL_MAX(NODES_PER_LEVEL_MAX),
                .N_CLASS_MAX(N_CLASS_MAX), .TAG_W(TAG_W)) engine (
        .clk(clk), .rst(rst),
        .in_valid(in_valid), .in_bank(in_bank), .in_tag(in_tag), .in_fv(in_fv),
        .tree_en_b0(tree_en_b0), .tree_en_b1(tree_en_b1),
        .wr_en(wr_en), .wr_tree(wr_tree), .wr_level(wr_level), .wr_bank(wr_bank),
        .wr_idx(wr_idx), .wr_data(wr_data),
        .rd_req(rd_req), .rd_tree(rd_tree), .rd_level(rd_level), .rd_bank(rd_bank),
        .rd_idx(rd_idx), .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .out_valid(out_valid), .out_bank(out_bank), .out_tag(out_tag), .out_class(out_forest)
    );

    // Key table: a frame always gets the read port; a read request waits for a
    // clock without one.
    wire              known_q;
    assign kt_rd_ack  = kt_rd_req && !in_valid;
    wire [KEY_W:0]    kt_raddr = in_valid ? {in_bank, in_key} : {kt_rd_bank, kt_rd_addr};

    sdp_ram #(.DATA_W(1), .ADDR_W(KEY_W + 1)) key_table (
        .clk(clk), .we(kt_wr_en), .waddr({kt_wr_bank, kt_wr_addr}), .wdata(kt_wr_data),
        .raddr(kt_raddr), .rdata(known_q)
    );
    assign kt_rd_data = known_q;
    always @(posedge clk) kt_rd_valid <= rst ? 1'b0 : kt_rd_ack;

    // known_q belongs to the frame captured one clock ago: delay it to the
    // verdict (LATENCY - 1 more clocks).
    reg [LATENCY-2:0] known_pipe;
    always @(posedge clk) known_pipe <= {known_pipe[LATENCY-3:0], known_q};
    wire known_out = known_pipe[LATENCY-2];

    wire              en   = rule_en[out_bank];
    wire [CLS_W-1:0]  from = out_bank ? rule_from_b1 : rule_from_b0;
    wire [CLS_W-1:0]  to   = out_bank ? rule_to_b1   : rule_to_b0;
    assign out_final = (en && out_forest == from && !known_out) ? to : out_forest;
endmodule
