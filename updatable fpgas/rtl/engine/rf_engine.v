// Generic random forest engine. The model is data in RAM; this logic never
// changes. It knows nothing about CAN or any other protocol: its input is a
// feature vector plus a valid signal.
//
//   N_TREES_MAX trees in parallel, DEPTH_MAX levels each, one pipeline stage
//   per level, one frame per clock, a majority vote at the end.
//   LATENCY = DEPTH_MAX + 3 clocks (15 with the defaults).
//
// Every frame enters with a bank number that travels with it down the
// pipeline, so a bank swap can never give one frame a mix of two models.
//
// Read path. Each node RAM has one read port. A frame in the pipeline always
// has it; rd_req only gets through on a clock when no frame needs that level
// (rd_ack). Classification is never delayed by a read.
//
// Tag. `in_tag` comes out unchanged with the verdict, for the caller's use
// (the update logic marks its self test frames with it).
module rf_engine #(
    parameter N_TREES_MAX         = 8,
    parameter DEPTH_MAX           = 12,
    parameter N_FEAT_MAX          = 16,
    parameter THRESH_W            = 16,
    parameter NODES_PER_LEVEL_MAX = 128,
    parameter N_CLASS_MAX         = 8,
    parameter TAG_W               = 1
) (
    clk, rst,
    in_valid, in_bank, in_tag, in_fv,
    tree_en_b0, tree_en_b1,
    wr_en, wr_tree, wr_level, wr_bank, wr_idx, wr_data,
    rd_req, rd_tree, rd_level, rd_bank, rd_idx, rd_ack, rd_valid, rd_data,
    out_valid, out_bank, out_tag, out_class, out_unanimous
);
    localparam IDX_W    = $clog2(NODES_PER_LEVEL_MAX);
    localparam FEAT_W   = $clog2(N_FEAT_MAX);
    localparam NODE_W   = FEAT_W + THRESH_W + 2 * (IDX_W + 1);
    localparam TREE_W   = $clog2(N_TREES_MAX);
    localparam LEVEL_W  = $clog2(DEPTH_MAX);
    localparam CLS_W    = $clog2(N_CLASS_MAX);
    localparam FV_W     = N_FEAT_MAX * THRESH_W;

    input  wire                     clk;
    input  wire                     rst;
    // frames
    input  wire                     in_valid;
    input  wire                     in_bank;
    input  wire [TAG_W-1:0]         in_tag;
    input  wire [FV_W-1:0]          in_fv;
    // which tree slots vote, per bank (a slot the model does not use must not)
    input  wire [N_TREES_MAX-1:0]   tree_en_b0;
    input  wire [N_TREES_MAX-1:0]   tree_en_b1;
    // loader write port
    input  wire                     wr_en;
    input  wire [TREE_W-1:0]        wr_tree;
    input  wire [LEVEL_W-1:0]       wr_level;
    input  wire                     wr_bank;
    input  wire [IDX_W-1:0]         wr_idx;
    input  wire [NODE_W-1:0]        wr_data;
    // read path (shadow bank CRC, flash save, READ command)
    input  wire                     rd_req;
    input  wire [TREE_W-1:0]        rd_tree;
    input  wire [LEVEL_W-1:0]       rd_level;
    input  wire                     rd_bank;
    input  wire [IDX_W-1:0]         rd_idx;
    output wire                     rd_ack;     // this clock the request is taken
    output reg                      rd_valid;   // rd_data holds the node
    output reg  [NODE_W-1:0]        rd_data;
    // verdicts
    output reg                      out_valid;
    output reg                      out_bank;
    output reg  [TAG_W-1:0]         out_tag;
    output reg  [CLS_W-1:0]         out_class;
    output reg                      out_unanimous; // every enabled tree voted out_class

    // ---- pipeline control, one entry per level -----------------------------
    reg              v_q   [0:DEPTH_MAX-1];
    reg              bank_q[0:DEPTH_MAX-1];
    reg [TAG_W-1:0]  tag_q [0:DEPTH_MAX-1];
    reg [FV_W-1:0]   fv_q  [0:DEPTH_MAX-1];

    integer l;
    always @(posedge clk) begin
        v_q[0]    <= rst ? 1'b0 : in_valid;
        bank_q[0] <= in_bank;
        tag_q[0]  <= in_tag;
        fv_q[0]   <= in_fv;
        for (l = 1; l < DEPTH_MAX; l = l + 1) begin
            v_q[l]    <= rst ? 1'b0 : v_q[l-1];
            bank_q[l] <= bank_q[l-1];
            tag_q[l]  <= tag_q[l-1];
            fv_q[l]   <= fv_q[l-1];
        end
    end

    // A level's RAM is busy on this clock if a frame is arriving at it.
    wire [DEPTH_MAX-1:0] level_busy;
    assign level_busy[0] = in_valid;
    genvar g;
    generate
        for (g = 1; g < DEPTH_MAX; g = g + 1) begin : busy
            assign level_busy[g] = v_q[g-1];
        end
    endgenerate
    assign rd_ack = rd_req && !level_busy[rd_level];

    // ---- the trees ---------------------------------------------------------
    wire [N_TREES_MAX*DEPTH_MAX*NODE_W-1:0] all_nodes;
    wire [N_TREES_MAX*CLS_W-1:0]            leaf_votes;

    genvar t, k;
    generate
        for (t = 0; t < N_TREES_MAX; t = t + 1) begin : tree
            wire [DEPTH_MAX:0]         done_w;
            wire [(DEPTH_MAX+1)*IDX_W-1:0] val_w;
            assign done_w[0]         = 1'b0;     // a new frame starts at the root
            assign val_w[0 +: IDX_W] = {IDX_W{1'b0}};

            for (k = 0; k < DEPTH_MAX; k = k + 1) begin : level
                wire [IDX_W:0] frame_addr = (k == 0) ? {in_bank, {IDX_W{1'b0}}}
                                                     : {bank_q[k == 0 ? 0 : k-1], val_w[k*IDX_W +: IDX_W]};
                wire           this_rd    = rd_ack && (rd_tree == t) && (rd_level == k);
                wire [IDX_W:0] addr       = this_rd ? {rd_bank, rd_idx} : frame_addr;

                tree_level #(.N_FEAT_MAX(N_FEAT_MAX), .THRESH_W(THRESH_W), .IDX_W(IDX_W),
                             .FEAT_W(FEAT_W), .NODE_W(NODE_W)) stage (
                    .clk(clk),
                    .wr_en(wr_en && (wr_tree == t) && (wr_level == k)),
                    .wr_addr({wr_bank, wr_idx}),
                    .wr_data(wr_data),
                    .addr(addr),
                    .done_i(done_w[k]),
                    .val_i(val_w[k*IDX_W +: IDX_W]),
                    .fv_q(fv_q[k]),
                    .node_q(all_nodes[(t*DEPTH_MAX + k)*NODE_W +: NODE_W]),
                    .done_o(done_w[k+1]),
                    .val_o(val_w[(k+1)*IDX_W +: IDX_W])
                );
            end
            assign leaf_votes[t*CLS_W +: CLS_W] = val_w[DEPTH_MAX*IDX_W +: CLS_W];
        end
    endgenerate

    // ---- read path ---------------------------------------------------------
    reg                ack_d;
    reg [TREE_W-1:0]   tree_d;
    reg [LEVEL_W-1:0]  level_d;
    always @(posedge clk) begin
        ack_d    <= rst ? 1'b0 : rd_ack;
        tree_d   <= rd_tree;
        level_d  <= rd_level;
        rd_valid <= rst ? 1'b0 : ack_d;
        rd_data  <= all_nodes[(tree_d*DEPTH_MAX + level_d)*NODE_W +: NODE_W];
    end

    // ---- the vote: one tree one vote, ties go to the lowest class ----------
    localparam CNT_W = $clog2(N_TREES_MAX + 1);

    reg                    v_vote, bank_vote;
    reg [TAG_W-1:0]        tag_vote;
    reg [N_TREES_MAX*CLS_W-1:0] votes_r;
    always @(posedge clk) begin
        v_vote    <= rst ? 1'b0 : v_q[DEPTH_MAX-1];
        bank_vote <= bank_q[DEPTH_MAX-1];
        tag_vote  <= tag_q[DEPTH_MAX-1];
        votes_r   <= leaf_votes;
    end

    wire [N_TREES_MAX-1:0] tree_en = bank_vote ? tree_en_b1 : tree_en_b0;

    reg                v_cnt, bank_cnt;
    reg [TAG_W-1:0]    tag_cnt;
    reg [CNT_W-1:0]    n_en, n_en_n;
    reg [CNT_W-1:0]    cnt   [0:N_CLASS_MAX-1];
    reg [CNT_W-1:0]    cnt_n [0:N_CLASS_MAX-1];
    integer c, m;
    always @(*) begin
        n_en_n = {CNT_W{1'b0}};
        for (m = 0; m < N_TREES_MAX; m = m + 1)
            if (tree_en[m]) n_en_n = n_en_n + 1'b1;
        for (c = 0; c < N_CLASS_MAX; c = c + 1) begin
            cnt_n[c] = {CNT_W{1'b0}};
            for (m = 0; m < N_TREES_MAX; m = m + 1)
                if (tree_en[m] && (votes_r[m*CLS_W +: CLS_W] == c))
                    cnt_n[c] = cnt_n[c] + 1'b1;
        end
    end
    always @(posedge clk) begin
        v_cnt    <= rst ? 1'b0 : v_vote;
        bank_cnt <= bank_vote;
        tag_cnt  <= tag_vote;
        n_en     <= n_en_n;
        for (c = 0; c < N_CLASS_MAX; c = c + 1)
            cnt[c] <= cnt_n[c];
    end

    reg [CLS_W-1:0] best_c;
    reg [CNT_W-1:0] best_v;
    integer b;
    always @(*) begin
        best_c = {CLS_W{1'b0}};
        best_v = cnt[0];
        for (b = 1; b < N_CLASS_MAX; b = b + 1)
            if (cnt[b] > best_v) begin
                best_c = b[CLS_W-1:0];
                best_v = cnt[b];
            end
    end
    always @(posedge clk) begin
        out_valid <= rst ? 1'b0 : v_cnt;
        out_bank  <= bank_cnt;
        out_tag   <= tag_cnt;
        out_class <= best_c;
        out_unanimous <= (best_v == n_en) && (n_en != {CNT_W{1'b0}});
    end
endmodule
