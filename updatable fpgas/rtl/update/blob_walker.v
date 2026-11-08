// Takes a model blob one byte at a time and writes it into the shadow bank.
// The same parser serves all three sources: SPI, flash and the boot ROM.
// While it goes it checks everything it can, so a bad blob can never give the
// engine a pointer outside its tables. Faults are sticky in `err` until start.
//
// err bits: 0 header  1 node counts  2 feature index  3 child pointer
//           4 leaf vote  5 length  6 bytes after the end
module blob_walker #(
    parameter N_TREES_MAX         = 8,
    parameter DEPTH_MAX           = 12,
    parameter N_FEAT_MAX          = 16,
    parameter N_CLASS_MAX         = 8,
    parameter NODES_PER_LEVEL_MAX = 128,
    parameter SLOT_BYTES          = 24576,
    // what each feature slot means on this front end, one byte per slot
    parameter [127:0] FE_FEATURE_MAP = 128'hFFFFFFFFFFFF09080706050403020100
) (
    clk, rst, start, s_valid, s_byte, s_ready,
    offset, done, err, crc_calc,
    h_total_len, h_model_id, h_version, h_device_id, h_protocol, h_n_trees, h_n_classes,
    h_feat_bytes, h_rule_en, h_rule_from, h_rule_to, h_n_selftest, h_flags,
    stored_crc, stored_mac,
    count_addr, count_q,
    wr_en, wr_tree, wr_level, wr_idx, wr_data,
    kt_wr_en, kt_wr_addr, kt_wr_data,
    meta_wr_en, meta_wr_addr, meta_wr_data
);
    localparam IDX_W     = $clog2(NODES_PER_LEVEL_MAX);
    localparam FEAT_W    = $clog2(N_FEAT_MAX);
    localparam NODE_W    = FEAT_W + 16 + 2 * (IDX_W + 1);
    localparam TREE_W    = $clog2(N_TREES_MAX);
    localparam LEVEL_W   = $clog2(DEPTH_MAX);
    localparam N_COUNTS  = N_TREES_MAX * DEPTH_MAX;

    localparam [15:0] OFF_MAP    = 64, OFF_COUNTS = 80, OFF_SELF = 176, OFF_KEYS = 448, OFF_NODES = 704;
    localparam [15:0] OVERHEAD   = 724;
    localparam [31:0] MAGIC      = 32'h4D464943;

    input  wire                 clk, rst, start;
    input  wire                 s_valid;
    input  wire [7:0]           s_byte;
    output wire                 s_ready;
    output reg  [15:0]          offset;
    output reg                  done;
    output reg  [6:0]           err;
    output wire [31:0]          crc_calc;
    output reg  [31:0]          h_total_len, h_model_id, h_version, h_device_id;
    output reg  [15:0]          h_protocol, h_flags;
    output reg  [7:0]           h_n_trees, h_n_classes, h_feat_bytes;
    output reg                  h_rule_en;
    output reg  [7:0]           h_rule_from, h_rule_to, h_n_selftest;
    output reg  [31:0]          stored_crc;
    output reg  [127:0]         stored_mac;
    input  wire [6:0]           count_addr;
    output wire [7:0]           count_q;
    output reg                  wr_en;
    output reg  [TREE_W-1:0]    wr_tree;
    output reg  [LEVEL_W-1:0]   wr_level;
    output reg  [IDX_W-1:0]     wr_idx;
    output reg  [NODE_W-1:0]    wr_data;
    output reg                  kt_wr_en;
    output reg  [10:0]          kt_wr_addr;
    output reg                  kt_wr_data;
    output reg                  meta_wr_en;
    output reg  [8:0]           meta_wr_addr;
    output reg  [7:0]           meta_wr_data;

    // Fields only the walker uses.
    reg [31:0] h_magic;
    reg [15:0] h_format;
    reg [7:0]  h_depth, h_n_features;

    reg [7:0] counts [0:N_COUNTS-1];
    assign count_q = counts[count_addr];

    localparam ST_ACCEPT = 3'd0, ST_CHK_HDR = 3'd1, ST_CHK_CNT = 3'd2, ST_KT = 3'd3,
               ST_FIND   = 3'd4, ST_ADV     = 3'd5;
    localparam SEC_META = 2'd0, SEC_KEYS = 2'd1, SEC_NODES = 2'd2, SEC_TRAILER = 2'd3;

    reg [2:0]  state;
    reg [1:0]  section;
    reg [31:0] crc;
    reg [15:0] splits;
    reg [7:0]  kt_byte;
    reg [3:0]  kt_bit;
    reg [10:0] kt_base;
    reg [2:0]  node_b;
    reg [31:0] node_sh;
    reg [3:0]  tree;          // 0..N_TREES_MAX
    reg [3:0]  level;
    reg [7:0]  idx;
    reg [4:0]  trailer_i;

    assign s_ready  = (state == ST_ACCEPT);
    assign crc_calc = ~crc;

    wire [31:0] crc_next;
    crc32_byte crc_unit (.crc(crc), .d(s_byte), .next(crc_next));

    wire        accept    = s_valid && s_ready;
    wire [7:0]  here      = counts[tree * DEPTH_MAX + level];
    wire [7:0]  below     = (level + 1 < DEPTH_MAX) ? counts[tree * DEPTH_MAX + level + 1] : 8'd0;

    // the node just completed (the 5th byte is s_byte)
    wire [3:0]  n_feat  = s_byte[FEAT_W-1:0];
    wire [7:0]  n_left  = node_sh[15:8];
    wire [7:0]  n_right = node_sh[7:0];
    wire        bad_feat  = (n_feat >= h_n_features);
    wire        bad_child = (!n_left[7]  && {1'b0, n_left[6:0]}  >= below)
                         || (!n_right[7] && {1'b0, n_right[6:0]} >= below);
    wire        bad_vote  = (n_left[7]  && n_left[6:0]  >= h_n_classes)
                         || (n_right[7] && n_right[6:0] >= h_n_classes);

    wire [15:0] nodes_end  = OFF_NODES + splits * 5;
    wire [7:0]  map_expect = FE_FEATURE_MAP[(offset - OFF_MAP) * 8 +: 8];

    always @(posedge clk) begin
        wr_en      <= 1'b0;
        kt_wr_en   <= 1'b0;
        meta_wr_en <= 1'b0;

        if (rst || start) begin
            state   <= ST_ACCEPT;
            section <= SEC_META;
            offset  <= 16'd0;
            done    <= 1'b0;
            err     <= 7'd0;
            crc     <= 32'hFFFFFFFF;
            splits  <= 16'd0;
            tree    <= 4'd0;
            level   <= 4'd0;
            idx     <= 8'd0;
            node_b  <= 3'd0;
            trailer_i <= 5'd0;
        end else case (state)

        ST_ACCEPT: if (accept) begin
            if (done) begin
                err[6] <= 1'b1;
            end else begin
                offset <= offset + 1'b1;
                if (section != SEC_TRAILER) crc <= crc_next;

                case (section)
                SEC_META: begin
                    meta_wr_en   <= 1'b1;
                    meta_wr_addr <= offset[8:0];
                    meta_wr_data <= s_byte;
                    case (offset)
                        16'd0:  h_magic[7:0]    <= s_byte;
                        16'd1:  h_magic[15:8]   <= s_byte;
                        16'd2:  h_magic[23:16]  <= s_byte;
                        16'd3:  h_magic[31:24]  <= s_byte;
                        16'd4:  h_format[7:0]   <= s_byte;
                        16'd5:  h_format[15:8]  <= s_byte;
                        16'd6:  h_flags[7:0]    <= s_byte;
                        16'd7:  h_flags[15:8]   <= s_byte;
                        16'd8:  h_total_len[7:0]   <= s_byte;
                        16'd9:  h_total_len[15:8]  <= s_byte;
                        16'd10: h_total_len[23:16] <= s_byte;
                        16'd11: h_total_len[31:24] <= s_byte;
                        16'd12: h_model_id[7:0]    <= s_byte;
                        16'd13: h_model_id[15:8]   <= s_byte;
                        16'd14: h_model_id[23:16]  <= s_byte;
                        16'd15: h_model_id[31:24]  <= s_byte;
                        16'd16: h_version[7:0]     <= s_byte;
                        16'd17: h_version[15:8]    <= s_byte;
                        16'd18: h_version[23:16]   <= s_byte;
                        16'd19: h_version[31:24]   <= s_byte;
                        16'd20: h_device_id[7:0]   <= s_byte;
                        16'd21: h_device_id[15:8]  <= s_byte;
                        16'd22: h_device_id[23:16] <= s_byte;
                        16'd23: h_device_id[31:24] <= s_byte;
                        16'd24: h_protocol[7:0]    <= s_byte;
                        16'd25: h_protocol[15:8]   <= s_byte;
                        16'd26: h_n_trees    <= s_byte;
                        16'd27: h_depth      <= s_byte;
                        16'd28: h_n_features <= s_byte;
                        16'd29: h_n_classes  <= s_byte;
                        16'd30: h_feat_bytes <= s_byte;
                        16'd31: h_rule_en    <= s_byte[0];
                        16'd32: h_rule_from  <= s_byte;
                        16'd33: h_rule_to    <= s_byte;
                        16'd34: h_n_selftest <= s_byte;
                        default: ;
                    endcase
                    if (offset >= OFF_MAP && offset < OFF_COUNTS
                        && s_byte != 8'hFF && s_byte != map_expect)
                        err[0] <= 1'b1;
                    if (offset >= OFF_COUNTS && offset < OFF_SELF) begin
                        counts[offset - OFF_COUNTS] <= s_byte;
                        splits <= splits + s_byte;
                    end
                    if (offset == OFF_MAP - 1)  state <= ST_CHK_HDR;
                    if (offset == OFF_SELF - 1) begin
                        state <= ST_CHK_CNT;
                        tree  <= 4'd0;
                        level <= 4'd0;
                    end
                    if (offset == OFF_KEYS - 1) section <= SEC_KEYS;
                end

                SEC_KEYS: begin
                    kt_byte <= s_byte;
                    kt_bit  <= 4'd0;
                    kt_base <= {offset[7:0], 3'b000};   // offset - 448 mod 256 = offset[7:0] - 192
                    state   <= ST_KT;
                end

                SEC_NODES: begin
                    node_sh <= {s_byte, node_sh[31:8]};
                    node_b  <= node_b + 1'b1;
                    if (node_b == 3'd4) begin
                        node_b <= 3'd0;
                        node_sh <= node_sh;
                        if (bad_feat)  err[2] <= 1'b1;
                        if (bad_child) err[3] <= 1'b1;
                        if (bad_vote)  err[4] <= 1'b1;
                        wr_en    <= 1'b1;
                        wr_tree  <= tree[TREE_W-1:0];
                        wr_level <= level[LEVEL_W-1:0];
                        wr_idx   <= idx[IDX_W-1:0];
                        wr_data  <= {s_byte[FEAT_W-1:0], node_sh};
                        state    <= ST_ADV;
                    end
                end

                SEC_TRAILER: begin
                    trailer_i <= trailer_i + 1'b1;
                    if (trailer_i < 4)
                        stored_crc[trailer_i * 8 +: 8] <= s_byte;
                    else
                        stored_mac[(trailer_i - 4) * 8 +: 8] <= s_byte;
                    if (trailer_i == 5'd19) begin
                        done <= 1'b1;
                        if ({16'd0, offset + 1'b1} != h_total_len) err[5] <= 1'b1;
                    end
                end
                endcase
            end
        end

        ST_CHK_HDR: begin
            if (h_magic != MAGIC || h_format != 16'd1 || h_flags != 16'd0
                || h_total_len < OVERHEAD || h_total_len > SLOT_BYTES
                || h_n_trees == 0 || h_n_trees > N_TREES_MAX
                || h_depth == 0 || h_depth > DEPTH_MAX
                || h_n_features == 0 || h_n_features > N_FEAT_MAX
                || h_n_classes == 0 || h_n_classes > N_CLASS_MAX
                || h_feat_bytes > 8
                || h_n_selftest == 0 || h_n_selftest > 8
                || h_rule_from >= h_n_classes || h_rule_to >= h_n_classes)
                err[0] <= 1'b1;
            state <= ST_ACCEPT;
        end

        // One count per clock: the shape of every tree must make sense.
        ST_CHK_CNT: begin
            if (tree < h_n_trees) begin
                if ((level == 0 && here != 8'd1)
                    || (level >= h_depth && here != 8'd0)
                    || here > NODES_PER_LEVEL_MAX)
                    err[1] <= 1'b1;
            end else if (here != 8'd0) begin
                err[1] <= 1'b1;
            end
            if (level == DEPTH_MAX - 1) begin
                level <= 4'd0;
                if (tree == N_TREES_MAX - 1) begin
                    tree  <= 4'd0;
                    state <= ST_ACCEPT;
                    if ({16'd0, splits} * 5 + OVERHEAD != h_total_len) err[5] <= 1'b1;
                end else begin
                    tree <= tree + 1'b1;
                end
            end else begin
                level <= level + 1'b1;
            end
        end

        // One key table bit per clock.
        ST_KT: begin
            kt_wr_en   <= 1'b1;
            kt_wr_addr <= {kt_base[10:3] - 8'd192, kt_bit[2:0]};
            kt_wr_data <= kt_byte[kt_bit[2:0]];
            kt_bit     <= kt_bit + 1'b1;
            if (kt_bit == 4'd7) begin
                if (offset == OFF_NODES) begin
                    section <= SEC_NODES;
                    tree    <= 4'd0;
                    level   <= 4'd0;
                    idx     <= 8'd0;
                    state   <= ST_FIND;
                end else begin
                    state <= ST_ACCEPT;
                end
            end
        end

        // Step to the next (tree, level) that has nodes, one per clock.
        ST_FIND: begin
            if (tree == N_TREES_MAX) begin
                section <= SEC_TRAILER;
                state   <= ST_ACCEPT;
            end else if (here != 8'd0 && level < DEPTH_MAX) begin
                state <= ST_ACCEPT;
            end else if (level == DEPTH_MAX - 1) begin
                level <= 4'd0;
                tree  <= tree + 1'b1;
            end else begin
                level <= level + 1'b1;
            end
        end

        ST_ADV: begin
            if (idx + 1'b1 < here) begin
                idx   <= idx + 1'b1;
                state <= ST_ACCEPT;
            end else begin
                idx <= 8'd0;
                if (level == DEPTH_MAX - 1) begin
                    level <= 4'd0;
                    tree  <= tree + 1'b1;
                end else begin
                    level <= level + 1'b1;
                end
                state <= ST_FIND;
            end
        end

        default: state <= ST_ACCEPT;
        endcase
    end
endmodule
