// Reads the shadow bank back out as the same byte stream the blob_walker took
// in. Used to check the CRC of what is really in RAM and to save it to flash.
// Live frames always win the RAMs' read ports; this just waits for gaps.
module blob_reader #(
    parameter N_TREES_MAX         = 8,
    parameter DEPTH_MAX           = 12,
    parameter NODES_PER_LEVEL_MAX = 128
) (
    clk, rst, start, total_len, stored_crc, stored_mac,
    count_addr, count_q,
    out_valid, out_byte, out_ready, busy,
    meta_addr, meta_q,
    rd_req, rd_tree, rd_level, rd_idx, rd_ack, rd_valid, rd_data,
    kt_rd_req, kt_rd_addr, kt_rd_ack, kt_rd_valid, kt_rd_data
);
    localparam IDX_W   = $clog2(NODES_PER_LEVEL_MAX);
    localparam TREE_W  = $clog2(N_TREES_MAX);
    localparam LEVEL_W = $clog2(DEPTH_MAX);

    input  wire               clk, rst, start;
    input  wire [15:0]        total_len;
    input  wire [31:0]        stored_crc;
    input  wire [127:0]       stored_mac;
    output wire [6:0]         count_addr;
    input  wire [7:0]         count_q;
    output reg                out_valid;
    output reg  [7:0]         out_byte;
    input  wire               out_ready;
    output wire               busy;
    output reg  [8:0]         meta_addr;
    input  wire [7:0]         meta_q;
    output reg                rd_req;
    output wire [TREE_W-1:0]  rd_tree;
    output wire [LEVEL_W-1:0] rd_level;
    output wire [IDX_W-1:0]   rd_idx;
    input  wire               rd_ack, rd_valid;
    input  wire [35:0]        rd_data;
    output reg                kt_rd_req;
    output wire [10:0]        kt_rd_addr;
    input  wire               kt_rd_ack, kt_rd_valid, kt_rd_data;

    localparam OFF_KEYS = 448, OFF_NODES = 704;

    localparam S_IDLE = 4'd0, S_META_A = 4'd1, S_META_W = 4'd2, S_META_D = 4'd3,
               S_KT_REQ = 4'd4, S_KT_WAIT = 4'd5, S_FIND = 4'd6, S_NODE_REQ = 4'd7,
               S_NODE_WAIT = 4'd8, S_NODE_OUT = 4'd9, S_TRAILER = 4'd10, S_OUT = 4'd11;

    reg [3:0]  state;
    reg [15:0] offset;
    reg [3:0]  tree, level;
    reg [7:0]  idx;
    reg [3:0]  kt_bit;
    reg [7:0]  kt_acc;
    reg [39:0] node_sh;
    reg [2:0]  node_b;
    reg [4:0]  trailer_i;

    assign busy       = (state != S_IDLE);
    assign count_addr = tree * DEPTH_MAX + level;
    assign rd_tree    = tree[TREE_W-1:0];
    assign rd_level   = level[LEVEL_W-1:0];
    assign rd_idx     = idx[IDX_W-1:0];
    wire [7:0] key_byte = offset - OFF_KEYS;
    assign kt_rd_addr = {key_byte, kt_bit[2:0]};

    wire [15:0] body_end = total_len - 16'd20;

    // Where the byte at `offset` comes from.
    task next_byte;
        input [15:0] off;
        begin
            if (off >= total_len) state <= S_IDLE;
            else if (off < OFF_KEYS) begin meta_addr <= off[8:0]; state <= S_META_W; end
            else if (off < OFF_NODES) begin kt_bit <= 4'd0; kt_acc <= 8'd0; state <= S_KT_REQ; end
            else if (off < body_end) state <= S_NODE_OUT;   // only reached between the 5 bytes
            else begin trailer_i <= off - body_end; state <= S_TRAILER; end
        end
    endtask

    always @(posedge clk) begin
        if (rst || start) begin
            state     <= start ? S_META_A : S_IDLE;
            offset    <= 16'd0;
            out_valid <= 1'b0;
            rd_req    <= 1'b0;
            kt_rd_req <= 1'b0;
            tree      <= 4'd0;
            level     <= 4'd0;
            idx       <= 8'd0;
            node_b    <= 3'd0;
        end else case (state)

        S_IDLE: ;

        S_META_A: begin meta_addr <= offset[8:0]; state <= S_META_W; end
        S_META_W: state <= S_META_D;                  // the RAM's output is valid after this clock
        S_META_D: begin out_byte <= meta_q; out_valid <= 1'b1; state <= S_OUT; end

        S_KT_REQ: begin
            kt_rd_req <= 1'b1;
            if (kt_rd_req && kt_rd_ack) begin
                kt_rd_req <= 1'b0;
                state     <= S_KT_WAIT;
            end
        end
        S_KT_WAIT: if (kt_rd_valid) begin
            kt_acc[kt_bit[2:0]] <= kt_rd_data;
            if (kt_bit == 4'd7) begin
                out_byte  <= {kt_rd_data, kt_acc[6:0]};
                out_valid <= 1'b1;
                state     <= S_OUT;
            end else begin
                kt_bit <= kt_bit + 1'b1;
                state  <= S_KT_REQ;
            end
        end

        // Next (tree, level, idx) that exists, one step per clock.
        S_FIND: begin
            if (tree == N_TREES_MAX) begin
                trailer_i <= 5'd0;
                state     <= S_TRAILER;
            end else if (idx < count_q) begin
                state <= S_NODE_REQ;
            end else begin
                idx <= 8'd0;
                if (level == DEPTH_MAX - 1) begin
                    level <= 4'd0;
                    tree  <= tree + 1'b1;
                end else begin
                    level <= level + 1'b1;
                end
            end
        end

        S_NODE_REQ: begin
            rd_req <= 1'b1;
            if (rd_req && rd_ack) begin
                rd_req <= 1'b0;
                state  <= S_NODE_WAIT;
            end
        end
        S_NODE_WAIT: if (rd_valid) begin
            node_sh <= {4'd0, rd_data};
            node_b  <= 3'd0;
            state   <= S_NODE_OUT;
        end
        S_NODE_OUT: begin
            out_byte  <= node_sh[node_b * 8 +: 8];
            out_valid <= 1'b1;
            state     <= S_OUT;
        end

        S_TRAILER: begin
            if (offset >= total_len) begin
                state <= S_IDLE;
            end else begin
                out_byte  <= (trailer_i < 4) ? stored_crc[trailer_i * 8 +: 8]
                                             : stored_mac[(trailer_i - 4) * 8 +: 8];
                trailer_i <= trailer_i + 1'b1;
                out_valid <= 1'b1;
                state     <= S_OUT;
            end
        end

        // Hold the byte until it is taken, then work out the next one.
        S_OUT: if (out_ready) begin
            out_valid <= 1'b0;
            offset    <= offset + 1'b1;
            if (offset + 1'b1 >= total_len) begin
                state <= S_IDLE;
            end else if (offset + 1'b1 < OFF_KEYS) begin
                meta_addr <= offset[8:0] + 1'b1;
                state     <= S_META_W;
            end else if (offset + 1'b1 < OFF_NODES) begin
                kt_bit <= 4'd0;
                kt_acc <= 8'd0;
                state  <= S_KT_REQ;
            end else if (offset + 1'b1 == OFF_NODES) begin
                tree  <= 4'd0;
                level <= 4'd0;
                idx   <= 8'd0;
                state <= S_FIND;
            end else if (offset >= body_end) begin
                state <= S_TRAILER;             // inside the 20 trailer bytes
            end else if (node_b == 3'd4) begin
                node_b <= 3'd0;
                idx    <= idx + 1'b1;
                state  <= S_FIND;               // the next node, or the trailer
            end else begin
                node_b <= node_b + 1'b1;
                state  <= S_NODE_OUT;
            end
        end

        default: state <= S_IDLE;
        endcase
    end
endmodule
