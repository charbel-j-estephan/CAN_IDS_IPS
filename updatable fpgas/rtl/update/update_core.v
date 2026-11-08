// The update and boot logic: holds the two model banks' settings, runs COMMIT,
// saves models to flash, boots from flash or ROM, and answers register reads.
// Nothing in here knows what protocol the front end speaks.
//
// One state machine, used one job at a time:
//   boot:   scan the journal -> load newest slot -> check -> self test -> activate
//           (on failure: the other slot, then the boot ROM)
//   COMMIT: length, magic, protocol, device, version, ranges -> settings ->
//           CRC of what is in RAM -> self test -> MAC -> erase -> write ->
//           read back -> journal -> swap at a frame boundary
// The active bank keeps classifying the whole time. Any failure leaves it alone.
//
// Flash layout (words of 32 bits, 2048 words per 8 KB page):
//   slot A  0     .. 6143    journal A  6144  .. 8191    (UFM1)
//   slot B  8192  .. 14335   journal B  14336 .. 16383   (UFM0)
// A journal record is 4 words, appended last: marker, {sequence, slot}, version,
// check. A record cut short by a power failure fails its check and is ignored.
module update_core #(
    parameter [31:0] DEVICE_ID     = 32'h0000_0001,
    parameter [15:0] PROTOCOL_ID   = 16'd1,
    parameter        READ_MODEL_EN = 1,          // 0 in production: no READ of the model
    parameter        ROM_ADDR_W    = 13,
    parameter        ROM_INIT_HEX  = "default_model.hex"
) (
    input  wire         clk,
    input  wire         rst,                 // KEY reset: restarts boot
    input  wire         por,                 // power on only: clears the lockout
    // from the SPI command decoder
    input  wire         cmd_start_upload,
    input  wire         cmd_byte_valid,
    input  wire [7:0]   cmd_byte,
    output wire [15:0]  shadow_offset,
    output wire         shadow_ready,        // the walker can take a byte
    input  wire         cmd_commit,
    input  wire [31:0]  cmd_commit_crc,
    input  wire         cmd_reject,          // a command was refused
    input  wire [7:0]   cmd_reject_code,
    input  wire         reg_req,
    input  wire [15:0]  reg_addr,
    output reg          reg_valid,
    output reg  [31:0]  reg_data,
    output wire [31:0]  status_word,
    output wire [31:0]  version_info,
    output wire [31:0]  active_model_id,
    output wire [31:0]  active_version,
    output wire [31:0]  n_seen_out,
    output wire [31:0]  n_blocked_out,
    output wire [31:0]  n_errors_out,
    output wire [31:0]  n_sig_out,
    output wire         locked,
    output wire         busy,
    // front end and engine
    input  wire         maint,
    input  wire         fe_idle,             // no frame on the bus: a swap is safe
    input  wire         fe_valid,
    input  wire         count_blocked,
    input  wire         mac_ok,              // Phase 2 MAC check; tied to 1 in Phase 1
    output wire         model_valid,
    output reg          active,
    output wire [3:0]   feat_bytes,
    output wire [7:0]   tree_en_b0,
    output wire [7:0]   tree_en_b1,
    output wire [1:0]   rule_en,
    output wire [2:0]   rule_from_b0,
    output wire [2:0]   rule_from_b1,
    output wire [2:0]   rule_to_b0,
    output wire [2:0]   rule_to_b1,
    // loader to the node RAMs and key table
    output wire         wr_en,
    output wire [2:0]   wr_tree,
    output wire [3:0]   wr_level,
    output wire         wr_bank,
    output wire [6:0]   wr_idx,
    output wire [35:0]  wr_data,
    output wire         kt_wr_en,
    output wire         kt_wr_bank,
    output wire [10:0]  kt_wr_addr,
    output wire         kt_wr_data,
    // read path
    output wire         rd_req,
    output wire [2:0]   rd_tree,
    output wire [3:0]   rd_level,
    output wire         rd_bank,
    output wire [6:0]   rd_idx,
    input  wire         rd_ack,
    input  wire         rd_valid,
    input  wire [35:0]  rd_data,
    output wire         kt_rd_req,
    output wire         kt_rd_bank,
    output wire [10:0]  kt_rd_addr,
    input  wire         kt_rd_ack,
    input  wire         kt_rd_valid,
    input  wire         kt_rd_data,
    // self test frames
    output reg          st_valid,
    output wire [255:0] st_fv,
    input  wire         st_accept,           // the frame went into the engine on this clock
    input  wire         out_valid,
    input  wire         out_tag,
    input  wire [2:0]   out_forest,
    // flash
    output reg  [1:0]   nv_op,
    output reg          nv_start,
    output reg  [13:0]  nv_addr,
    output reg  [31:0]  nv_wdata,
    input  wire         nv_done,
    input  wire         nv_ok,
    input  wire [31:0]  nv_rdata
);
    localparam [13:0] SLOT_A = 14'd0, SLOT_B = 14'd8192, JOURNAL_A = 14'd6144, JOURNAL_B = 14'd14336;
    localparam [31:0] J_MARKER = 32'h4C4E524A;      // "JRNL"
    localparam [10:0] PAGE_WORDS = 11'd1024;        // doubled below: 2048 words per page
    localparam [15:0] SLOT_LIMIT = 16'd24576;

    localparam SRC_NONE = 2'd0, SRC_A = 2'd1, SRC_B = 2'd2, SRC_ROM = 2'd3;

    // Error codes (LAST_ERROR register, docs/REGISTER_MAP.md).
    localparam [7:0] E_OK = 0, E_NOT_MAINT = 1, E_LOCKED = 2, E_TRUNCATED = 3, E_HEADER = 4,
                     E_PROTOCOL = 5, E_DEVICE = 6, E_VERSION = 7, E_RANGE = 8, E_LENGTH = 9,
                     E_CRC = 10, E_SELFTEST = 11, E_MAC = 12, E_ERASE = 13, E_WRITE = 14,
                     E_VERIFY = 15, E_BUSY = 16, E_SEQUENCE = 17;

    localparam [5:0]
        S_SCAN_REQ  = 0,  S_SCAN_WAIT = 1,  S_CHOOSE    = 2,  S_LOAD_INIT = 3,
        S_LOAD      = 4,  S_ROM_A     = 5,  S_ROM_W     = 6,  S_ROM_D     = 7,
        S_LOAD_CHK  = 8,  S_LOAD_FAIL = 9,  S_CFG       = 10, S_ST_INIT   = 11,
        S_ST_ADDR   = 12, S_ST_WAITR  = 13, S_ST_DATA   = 14, S_ST_SEND   = 15,
        S_ST_RESULT = 16, S_ACTIVATE  = 17, S_IDLE      = 18, S_C_CHECK   = 19,
        S_C_RB_INIT = 20, S_C_RB      = 21, S_C_MAC     = 22, S_C_ERASE   = 23,
        S_C_ERASE_W = 24, S_C_WR_INIT = 25, S_C_WRITE   = 26, S_C_WRITE_W = 27,
        S_C_VF_INIT = 28, S_C_VERIFY  = 29, S_J_ROLL    = 30, S_J_ROLL_W  = 31,
        S_J_WRITE   = 32, S_J_WAIT    = 33, S_C_SWAP    = 34, S_HALT      = 35,
        S_LOAD_PRE  = 36;

    reg [5:0] state;
    assign busy = (state != S_IDLE);
    wire rdr_busy;
    reg  model_on;
    reg  [1:0] active_src;

    // ---- walker ----------------------------------------------------------
    reg         w_start;
    reg         w_valid;
    reg  [7:0]  w_byte;
    wire        w_ready, w_done;
    wire [6:0]  w_err;
    wire [15:0] w_offset;
    wire [31:0] w_crc;
    wire [31:0] h_total_len, h_model_id, h_version, h_device_id, stored_crc;
    wire [15:0] h_protocol, h_flags;
    wire [7:0]  h_n_trees, h_n_classes, h_feat_bytes, h_rule_from, h_rule_to, h_n_selftest;
    wire        h_rule_en;
    wire [127:0] stored_mac;
    wire [6:0]  count_addr;
    wire [7:0]  count_q;
    wire        meta_we;
    wire [8:0]  meta_wa;
    wire [7:0]  meta_wd;
    wire        w_wr_en;
    wire [2:0]  w_wr_tree;
    wire [3:0]  w_wr_level;
    wire [6:0]  w_wr_idx;
    wire [35:0] w_wr_data;

    wire        w_s_valid = (state == S_IDLE) ? cmd_byte_valid : w_valid;
    wire [7:0]  w_s_byte  = (state == S_IDLE) ? cmd_byte : w_byte;
    wire        w_s_start = w_start | ((state == S_IDLE) & cmd_start_upload);

    blob_walker walker (
        .clk(clk), .rst(rst), .start(w_s_start),
        .s_valid(w_s_valid), .s_byte(w_s_byte), .s_ready(w_ready),
        .offset(w_offset), .done(w_done), .err(w_err), .crc_calc(w_crc),
        .h_total_len(h_total_len), .h_model_id(h_model_id), .h_version(h_version),
        .h_device_id(h_device_id), .h_protocol(h_protocol), .h_n_trees(h_n_trees),
        .h_n_classes(h_n_classes), .h_feat_bytes(h_feat_bytes), .h_rule_en(h_rule_en),
        .h_rule_from(h_rule_from), .h_rule_to(h_rule_to), .h_n_selftest(h_n_selftest),
        .h_flags(h_flags), .stored_crc(stored_crc), .stored_mac(stored_mac),
        .count_addr(count_addr), .count_q(count_q),
        .wr_en(w_wr_en), .wr_tree(w_wr_tree), .wr_level(w_wr_level), .wr_idx(w_wr_idx),
        .wr_data(w_wr_data),
        .kt_wr_en(kt_wr_en), .kt_wr_addr(kt_wr_addr), .kt_wr_data(kt_wr_data),
        .meta_wr_en(meta_we), .meta_wr_addr(meta_wa), .meta_wr_data(meta_wd)
    );
    assign shadow_offset = w_offset;
    assign shadow_ready  = w_ready;

    assign wr_en      = w_wr_en;
    assign wr_tree    = w_wr_tree;
    assign wr_level   = w_wr_level;
    assign wr_idx     = w_wr_idx;
    assign wr_data    = w_wr_data;
    assign wr_bank    = ~active;
    assign kt_wr_bank = ~active;

    // ---- header and self test bytes (shadow only) --------------------------
    reg  [8:0]  st_addr;
    wire [8:0]  rdr_meta_addr;
    reg  [8:0]  spi_meta_addr;
    wire [7:0]  meta_q;
    wire        st_uses_meta = (state >= S_ST_INIT && state <= S_ST_RESULT);
    wire [8:0]  meta_ra = st_uses_meta ? st_addr : (rdr_busy ? rdr_meta_addr : spi_meta_addr);

    sdp_ram #(.DATA_W(8), .ADDR_W(9)) meta_ram (
        .clk(clk), .we(meta_we), .waddr(meta_wa), .wdata(meta_wd), .raddr(meta_ra), .rdata(meta_q)
    );

    // ---- reader ------------------------------------------------------------
    reg         rdr_start;
    wire        rdr_valid;
    wire [7:0]  rdr_byte;
    reg         rdr_ready;
    wire [2:0]  rdr_tree;
    wire [3:0]  rdr_level;
    wire [6:0]  rdr_idx;
    wire        rdr_rd_req, rdr_kt_req;
    wire [10:0] rdr_kt_addr;

    blob_reader reader (
        .clk(clk), .rst(rst), .start(rdr_start), .total_len(h_total_len[15:0]),
        .stored_crc(stored_crc), .stored_mac(stored_mac),
        .count_addr(count_addr), .count_q(count_q),
        .out_valid(rdr_valid), .out_byte(rdr_byte), .out_ready(rdr_ready), .busy(rdr_busy),
        .meta_addr(rdr_meta_addr), .meta_q(meta_q),
        .rd_req(rdr_rd_req), .rd_tree(rdr_tree), .rd_level(rdr_level), .rd_idx(rdr_idx),
        .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .kt_rd_req(rdr_kt_req), .kt_rd_addr(rdr_kt_addr), .kt_rd_ack(kt_rd_ack),
        .kt_rd_valid(kt_rd_valid), .kt_rd_data(kt_rd_data)
    );

    // Register read path into the model (development builds only).
    reg        spi_rd_req, spi_kt_req;
    reg [2:0]  spi_tree;
    reg [3:0]  spi_level;
    reg [6:0]  spi_idx;
    reg [10:0] spi_kt_addr;

    assign rd_req     = rdr_busy ? rdr_rd_req : spi_rd_req;
    assign rd_tree    = rdr_busy ? rdr_tree   : spi_tree;
    assign rd_level   = rdr_busy ? rdr_level  : spi_level;
    assign rd_idx     = rdr_busy ? rdr_idx    : spi_idx;
    assign rd_bank    = ~active;
    assign kt_rd_req  = rdr_busy ? rdr_kt_req : spi_kt_req;
    assign kt_rd_addr = rdr_busy ? rdr_kt_addr : spi_kt_addr;
    assign kt_rd_bank = ~active;

    // ---- boot ROM ----------------------------------------------------------
    reg  [ROM_ADDR_W-1:0] rom_addr;
    wire [7:0]            rom_q;
    boot_rom #(.ADDR_W(ROM_ADDR_W), .INIT_HEX(ROM_INIT_HEX)) rom (
        .clk(clk), .addr(rom_addr), .q(rom_q)
    );

    // ---- settings of the two banks -----------------------------------------
    reg        cfg_valid   [0:1];
    reg [31:0] cfg_model   [0:1];
    reg [31:0] cfg_version [0:1];
    reg [7:0]  cfg_tree_en [0:1];
    reg [3:0]  cfg_bytes   [0:1];
    reg        cfg_rule_en [0:1];
    reg [2:0]  cfg_from    [0:1];
    reg [2:0]  cfg_to      [0:1];

    assign tree_en_b0   = cfg_tree_en[0];
    assign tree_en_b1   = cfg_tree_en[1];
    assign rule_en      = {cfg_rule_en[1], cfg_rule_en[0]};
    assign rule_from_b0 = cfg_from[0];
    assign rule_from_b1 = cfg_from[1];
    assign rule_to_b0   = cfg_to[0];
    assign rule_to_b1   = cfg_to[1];
    assign feat_bytes   = model_on ? cfg_bytes[active] : 4'd8;
    assign active_model_id = model_on ? cfg_model[active]   : 32'd0;
    assign active_version  = model_on ? cfg_version[active] : 32'd0;

    assign model_valid = model_on;
    reg        swap_pending;
    reg [1:0]  load_src;
    reg [1:0]  shadow_src;

    // ---- counters and status -----------------------------------------------
    reg [31:0] n_seen, n_blocked, n_errors, n_sig;
    reg [7:0]  last_error;
    reg [1:0]  strikes = 2'd0;              // only power on clears it
    assign locked = (strikes == 2'd3);
    reg [31:0] floor;
    reg        busy_commit;

    assign n_seen_out    = n_seen;
    assign n_blocked_out = n_blocked;
    assign n_errors_out  = n_errors;
    assign n_sig_out     = n_sig;

    assign status_word = {8'hC1, 2'b00, state, swap_pending, w_done, active_src, active,
                          maint, locked, (state != S_IDLE), last_error};
    assign version_info = {12'd0, h_n_trees[3:0], 8'd0, 4'd0, active_src, 1'b0, active};

    // ---- journal scan results ----------------------------------------------
    reg [12:0] scan_i;                 // word number within the two pages (0..4095)
    reg [1:0]  scan_w;
    reg [31:0] scan_d0, scan_d1, scan_d2;
    reg        best_found;
    reg [23:0] best_seq;
    reg        best_slot;
    reg [9:0]  next_rec [0:1];         // first free record per journal page (0..512)
    reg        j_page;
    reg [9:0]  j_idx;
    reg [23:0] j_seq;
    reg        best_page;

    wire        j_page_now = scan_i[11];
    wire [13:0] scan_addr  = (j_page_now ? JOURNAL_B : JOURNAL_A) + {3'b000, scan_i[10:0]};
    wire [8:0]  rec_now    = scan_i[10:2];
    wire        rec_blank  = (scan_d0 == 32'hFFFFFFFF) && (scan_d1 == 32'hFFFFFFFF)
                          && (scan_d2 == 32'hFFFFFFFF) && (nv_rdata == 32'hFFFFFFFF);
    wire        rec_valid  = (scan_d0 == J_MARKER)
                          && (nv_rdata == ~(scan_d0 ^ scan_d1 ^ scan_d2));
    wire [23:0] rec_seq    = scan_d1[31:8];
    wire        rec_slot   = scan_d1[0];

    // ---- load attempts -----------------------------------------------------
    reg  [1:0]  attempt;               // 0 newest slot, 1 other slot, 2 ROM
    reg  [13:0] fs_addr;
    reg  [31:0] fs_word;
    reg  [2:0]  fs_left;
    reg         fs_pending;
    reg  [15:0] fs_count;
    reg         boot_mode;

    // ---- commit working values ---------------------------------------------
    reg  [31:0] expect_crc;
    reg  [31:0] run_crc;
    reg  [15:0] p_cnt;
    wire [31:0] run_next;
    reg  [7:0]  crc_in;
    crc32_byte crc_unit2 (.crc(run_crc), .d(crc_in), .next(run_next));

    reg         target_slot;           // 0 = A, 1 = B
    reg  [1:0]  c_page;
    reg  [31:0] fw_word;
    reg  [1:0]  fw_lane;
    reg  [13:0] fw_addr;
    reg  [2:0]  j_w;
    reg  [13:0] j_base;

    wire [15:0] body_len = h_total_len[15:0] - 16'd20;

    // ---- self test ----------------------------------------------------------
    reg  [3:0]   st_i;
    reg  [5:0]   st_j;
    reg  [7:0]   st_expect;
    reg  [255:0] st_vec;
    assign st_fv = st_vec;

    // Combinational part of the handshakes, so a byte is taken on exactly one clock.
    always @(*) begin
        rdr_ready = 1'b0;
        crc_in    = 8'd0;
        w_valid   = 1'b0;
        w_byte    = 8'd0;
        case (state)
        S_LOAD: begin
            w_valid = (fs_left != 3'd0) && !w_done && (w_err == 7'd0);
            w_byte  = fs_word[7:0];
        end
        S_ROM_D: begin
            w_valid = !w_done && (w_err == 7'd0);
            w_byte  = rom_q;
        end
        S_C_RB: begin
            rdr_ready = rdr_valid;
            crc_in    = rdr_byte;
        end
        S_C_WRITE: rdr_ready = rdr_valid;
        S_C_VERIFY: begin
            rdr_ready = rdr_valid && (fs_left != 3'd0);
            crc_in    = fs_word[7:0];
        end
        default: ;
        endcase
    end

    // Reading the model itself back. READ_MODEL_EN = 0 (production) answers
    // every one of these with DEAD0BAD: thresholds never leave the chip.
    // Windows: 0x1000+ header bytes, 0x2000+ key table bytes,
    // 0x4000 + {tree, level, idx} node bits 31:0, 0x8000 + same: node bits 35:32.
    localparam W_IDLE = 3'd0, W_META1 = 3'd1, W_META2 = 3'd2, W_KT_REQ = 3'd3, W_KT_WAIT = 3'd4,
               W_N_REQ = 3'd5, W_N_WAIT = 3'd6;
    reg [2:0] win = W_IDLE;
    reg       win_high;
    reg [3:0] win_bit;
    reg [7:0] win_acc;

    task win_start;
        begin
            if (!READ_MODEL_EN || state != S_IDLE || win != W_IDLE) begin
                reg_valid <= 1'b1;
                reg_data  <= 32'hDEAD0BAD;
            end else if (reg_addr < 16'h2000) begin
                spi_meta_addr <= reg_addr[8:0];
                win <= W_META1;
            end else if (reg_addr < 16'h4000) begin
                spi_kt_addr <= {reg_addr[7:0], 3'd0};
                spi_kt_req  <= 1'b1;
                win_bit     <= 4'd0;
                win <= W_KT_REQ;
            end else begin
                win_high    <= reg_addr[15];
                spi_tree    <= reg_addr[13:11];
                spi_level   <= reg_addr[10:7];
                spi_idx     <= reg_addr[6:0];
                spi_rd_req  <= 1'b1;
                win <= W_N_REQ;
            end
        end
    endtask

    task win_tick;
        begin
            case (win)
            W_META1: win <= W_META2;
            W_META2: begin reg_valid <= 1'b1; reg_data <= {24'd0, meta_q}; win <= W_IDLE; end
            W_KT_REQ: if (kt_rd_ack) begin spi_kt_req <= 1'b0; win <= W_KT_WAIT; end
            W_KT_WAIT: if (kt_rd_valid) begin
                win_acc[win_bit[2:0]] <= kt_rd_data;
                if (win_bit == 4'd7) begin
                    reg_valid <= 1'b1;
                    reg_data  <= {24'd0, kt_rd_data, win_acc[6:0]};
                    win <= W_IDLE;
                end else begin
                    win_bit     <= win_bit + 1'b1;
                    spi_kt_addr <= spi_kt_addr + 1'b1;
                    spi_kt_req  <= 1'b1;
                    win <= W_KT_REQ;
                end
            end
            W_N_REQ: if (rd_ack) begin spi_rd_req <= 1'b0; win <= W_N_WAIT; end
            W_N_WAIT: if (rd_valid) begin
                reg_valid <= 1'b1;
                reg_data  <= win_high ? {28'd0, rd_data[35:32]} : rd_data[31:0];
                win <= W_IDLE;
            end
            default: win <= W_IDLE;
            endcase
        end
    endtask

    task fail;
        input [7:0] code;
        begin
            last_error   <= code;
            n_errors     <= n_errors + 1'b1;
            busy_commit  <= 1'b0;
            state        <= S_IDLE;
        end
    endtask

    integer k;

    always @(posedge clk) begin
        nv_start  <= 1'b0;
        w_start   <= 1'b0;
        rdr_start <= 1'b0;
        reg_valid <= 1'b0;

        if (por) begin
            strikes <= 2'd0;
        end

        if (cmd_reject) begin
            last_error <= cmd_reject_code;
            n_errors   <= n_errors + 1'b1;
        end
        if (cmd_start_upload && state == S_IDLE) last_error <= E_OK;   // a new upload starts clean
        if (out_valid && !out_tag) n_seen <= n_seen + 1'b1;
        if (count_blocked)         n_blocked <= n_blocked + 1'b1;

        // The swap itself: only between frames, and the moment it is allowed.
        if (swap_pending && fe_idle) begin
            active       <= ~active;
            active_src   <= shadow_src;
            swap_pending <= 1'b0;
            model_on     <= 1'b1;
        end

        if (rst) begin
            state        <= S_SCAN_REQ;
            active       <= 1'b0;
            model_on     <= 1'b0;
            swap_pending <= 1'b0;
            active_src   <= SRC_NONE;
            shadow_src   <= SRC_NONE;
            last_error   <= E_OK;
            n_seen       <= 32'd0;
            n_blocked    <= 32'd0;
            n_errors     <= 32'd0;
            n_sig        <= 32'd0;
            floor        <= 32'd0;
            busy_commit  <= 1'b0;
            st_valid     <= 1'b0;
            scan_i       <= 13'd0;
            scan_w       <= 2'd0;
            best_found   <= 1'b0;
            best_seq     <= 24'd0;
            best_slot    <= 1'b0;
            best_page    <= 1'b0;
            next_rec[0]  <= 10'd0;
            next_rec[1]  <= 10'd0;
            attempt      <= 2'd0;
            boot_mode    <= 1'b1;
            spi_rd_req   <= 1'b0;
            spi_kt_req   <= 1'b0;
            for (k = 0; k < 2; k = k + 1) begin
                cfg_valid[k]   <= 1'b0;
                cfg_tree_en[k] <= 8'd0;
                cfg_rule_en[k] <= 1'b0;
                cfg_bytes[k]   <= 4'd8;
            end
        end else case (state)

        // ---------------- boot: read every journal record ----------------
        S_SCAN_REQ: begin
            nv_op    <= 2'd1;
            nv_addr  <= scan_addr;
            nv_start <= 1'b1;
            state    <= S_SCAN_WAIT;
        end

        S_SCAN_WAIT: if (nv_done) begin
            case (scan_w)
                2'd0: scan_d0 <= nv_rdata;
                2'd1: scan_d1 <= nv_rdata;
                2'd2: scan_d2 <= nv_rdata;
                2'd3: begin
                    if (!nv_ok || !rec_blank)
                        next_rec[j_page_now] <= {1'b0, rec_now} + 1'b1;
                    if (nv_ok && rec_valid) begin
                        if (!best_found || rec_seq > best_seq) begin
                            best_found <= 1'b1;
                            best_seq   <= rec_seq;
                            best_slot  <= rec_slot;
                            best_page  <= j_page_now;
                        end
                        if (scan_d2 > floor) floor <= scan_d2;
                    end
                end
            endcase
            scan_w <= scan_w + 1'b1;
            scan_i <= scan_i + 1'b1;
            if (scan_i == 13'd4095) state <= S_CHOOSE;
            else                    state <= S_SCAN_REQ;
        end

        S_CHOOSE: begin
            j_page <= best_found ? best_page : 1'b0;
            j_idx  <= best_found ? next_rec[best_page] : next_rec[0];
            j_seq  <= best_found ? best_seq + 1'b1 : 24'd1;
            attempt <= best_found ? 2'd0 : 2'd2;
            state   <= S_LOAD_INIT;
        end

        // ---------------- boot: load one candidate ----------------
        S_LOAD_INIT: begin
            w_start    <= 1'b1;
            fs_left    <= 3'd0;
            fs_pending <= 1'b0;
            fs_count   <= 16'd0;
            if (attempt == 2'd2) begin
                load_src <= SRC_ROM;
                rom_addr <= {ROM_ADDR_W{1'b0}};
                state    <= S_ROM_A;
            end else begin
                load_src  <= ((attempt == 2'd0) ? best_slot : ~best_slot) ? SRC_B : SRC_A;
                fs_addr   <= ((attempt == 2'd0) ? best_slot : ~best_slot) ? SLOT_B : SLOT_A;
                state     <= S_LOAD_PRE;
            end
        end

        // One clock for the walker to take its start, so its old flags are gone.
        S_LOAD_PRE: state <= S_LOAD;

        S_LOAD: begin
            if (w_done || w_err != 7'd0 || fs_count == SLOT_LIMIT) begin
                // Let a read already on its way finish, or its answer would be
                // taken for the first word of the next attempt.
                if (!fs_pending || nv_done) begin
                    fs_pending <= 1'b0;
                    state      <= S_LOAD_CHK;
                end
            end else begin
                if (fs_left == 3'd0) begin
                    if (!fs_pending) begin
                        nv_op      <= 2'd1;
                        nv_addr    <= fs_addr;
                        nv_start   <= 1'b1;
                        fs_pending <= 1'b1;
                    end else if (nv_done) begin
                        fs_pending <= 1'b0;
                        fs_addr    <= fs_addr + 1'b1;
                        if (nv_ok) begin
                            fs_word <= nv_rdata;
                            fs_left <= 3'd4;
                        end else begin
                            state <= S_LOAD_CHK;
                        end
                    end
                end else if (w_ready) begin
                    fs_word  <= {8'hFF, fs_word[31:8]};
                    fs_left  <= fs_left - 1'b1;
                    fs_count <= fs_count + 1'b1;
                end
            end
        end

        S_ROM_A: state <= S_ROM_W;              // rom_addr goes in on this clock
        S_ROM_W: state <= S_ROM_D;
        S_ROM_D: begin
            if (w_done || w_err != 7'd0) begin
                state <= S_LOAD_CHK;
            end else if (w_ready) begin
                rom_addr <= rom_addr + 1'b1;
                state    <= S_ROM_A;
            end
        end

        S_LOAD_CHK: begin
            if (w_done && w_err == 7'd0 && w_crc == stored_crc
                && h_protocol == PROTOCOL_ID
                && (h_device_id == DEVICE_ID
                    || (load_src == SRC_ROM && h_device_id == 32'hFFFFFFFF))
                && (load_src == SRC_ROM || mac_ok)) begin
                shadow_src <= load_src;
                state      <= S_CFG;
            end else begin
                state <= S_LOAD_FAIL;
            end
        end

        S_LOAD_FAIL: begin
            if (attempt == 2'd2) begin
                last_error <= E_HEADER;
                state      <= S_HALT;          // no model at all: stay out of the way
            end else begin
                attempt <= attempt + 1'b1;
                state   <= S_LOAD_INIT;
            end
        end

        // ---------------- settings, then self test (boot and COMMIT) ------
        S_CFG: begin
            cfg_valid[~active]   <= 1'b1;
            cfg_model[~active]   <= h_model_id;
            cfg_version[~active] <= h_version;
            cfg_tree_en[~active] <= (8'd1 << h_n_trees) - 8'd1;
            cfg_bytes[~active]   <= h_feat_bytes[3:0];
            cfg_rule_en[~active] <= h_rule_en;
            cfg_from[~active]    <= h_rule_from[2:0];
            cfg_to[~active]      <= h_rule_to[2:0];
            state <= boot_mode ? S_ST_INIT : S_C_RB_INIT;
        end

        S_ST_INIT: begin
            st_i    <= 4'd0;
            st_j    <= 6'd0;
            st_addr <= 9'd176;
            state   <= S_ST_ADDR;
        end
        S_ST_ADDR:  state <= S_ST_WAITR;       // st_addr goes in on this clock
        S_ST_WAITR: state <= S_ST_DATA;
        S_ST_DATA: begin
            if (st_j == 6'd32) begin
                st_expect <= meta_q;
                st_valid  <= 1'b1;
                state     <= S_ST_SEND;
            end else begin
                st_vec[st_j * 8 +: 8] <= meta_q;
                st_j    <= st_j + 1'b1;
                st_addr <= st_addr + 1'b1;
                state   <= S_ST_ADDR;
            end
        end
        S_ST_SEND: if (st_accept) begin
            st_valid <= 1'b0;
            state    <= S_ST_RESULT;
        end
        S_ST_RESULT: if (out_valid && out_tag) begin
            if ({5'd0, out_forest} != st_expect) begin
                if (boot_mode) state <= S_LOAD_FAIL;
                else           fail(E_SELFTEST);
            end else if (st_i + 1'b1 == h_n_selftest) begin
                state <= boot_mode ? S_ACTIVATE : S_C_MAC;
            end else begin
                st_i    <= st_i + 1'b1;
                st_j    <= 6'd0;
                st_addr <= 9'd176 + (st_i + 1'b1) * 34;
                state   <= S_ST_ADDR;
            end
        end

        S_ACTIVATE: begin
            swap_pending <= 1'b1;
            boot_mode    <= 1'b0;
            state        <= S_IDLE;
        end

        // ---------------- waiting for a command ------------------------------
        S_IDLE: begin
            if (cmd_commit && !swap_pending) begin
                expect_crc  <= cmd_commit_crc;
                busy_commit <= 1'b1;
                last_error  <= E_OK;
                state       <= S_C_CHECK;
            end else if (cmd_commit) begin
                last_error <= E_BUSY;
                n_errors   <= n_errors + 1'b1;
            end
        end

        S_HALT: ;                               // boot found no model: stay passive

        // ---------------- COMMIT ------------------------------------------------
        S_C_CHECK: begin
            if      (!w_done)                                 fail(E_TRUNCATED);
            else if (w_err[0])                                fail(E_HEADER);
            else if (h_protocol != PROTOCOL_ID)               fail(E_PROTOCOL);
            else if (h_device_id != DEVICE_ID)                fail(E_DEVICE);
            else if (h_version < floor)                       fail(E_VERSION);
            else if (w_err[4:1] != 4'd0)                      fail(E_RANGE);
            else if (w_err[6:5] != 2'd0)                      fail(E_LENGTH);
            else if (w_crc != expect_crc || stored_crc != expect_crc) fail(E_CRC);
            else begin
                state <= S_CFG;
            end
        end

        // CRC of what is actually in the shadow RAM.
        S_C_RB_INIT: begin
            rdr_start <= 1'b1;
            run_crc   <= 32'hFFFFFFFF;
            p_cnt     <= 16'd0;
            state     <= S_C_RB;
        end
        S_C_RB: begin
            if (rdr_valid && rdr_ready) begin
                p_cnt <= p_cnt + 1'b1;
                if (p_cnt < body_len) run_crc <= run_next;
            end else if (!rdr_busy && !rdr_valid && p_cnt != 16'd0) begin
                if (~run_crc != expect_crc) fail(E_CRC);
                else                         state <= S_ST_INIT;
            end
        end

        S_C_MAC: begin
            if (mac_ok) begin
                c_page      <= 2'd0;
                target_slot <= (active_src == SRC_A) ? 1'b1 : 1'b0;
                state       <= S_C_ERASE;
            end else begin
                n_sig <= n_sig + 1'b1;
                if (strikes != 2'd3) strikes <= strikes + 1'b1;
                fail(E_MAC);
            end
        end

        S_C_ERASE: begin
            nv_op    <= 2'd3;
            nv_addr  <= (target_slot ? SLOT_B : SLOT_A) + {1'b0, c_page, 11'd0};
            nv_start <= 1'b1;
            state    <= S_C_ERASE_W;
        end
        S_C_ERASE_W: if (nv_done) begin
            if (!nv_ok)              fail(E_ERASE);
            else if (c_page == 2'd2) state <= S_C_WR_INIT;
            else begin
                c_page <= c_page + 1'b1;
                state  <= S_C_ERASE;
            end
        end

        S_C_WR_INIT: begin
            rdr_start <= 1'b1;
            p_cnt     <= 16'd0;
            fw_word   <= 32'hFFFFFFFF;
            fw_lane   <= 2'd0;
            fw_addr   <= target_slot ? SLOT_B : SLOT_A;
            state     <= S_C_WRITE;
        end
        S_C_WRITE: if (rdr_valid && rdr_ready) begin
            p_cnt   <= p_cnt + 1'b1;
            fw_lane <= fw_lane + 1'b1;
            if (fw_lane == 2'd3 || p_cnt + 1'b1 == h_total_len[15:0]) begin
                nv_op    <= 2'd2;
                nv_addr  <= fw_addr;
                nv_wdata <= (fw_word & ~(32'hFF << (fw_lane * 8))) | ({24'd0, rdr_byte} << (fw_lane * 8));
                nv_start <= 1'b1;
                fw_word  <= 32'hFFFFFFFF;
                fw_addr  <= fw_addr + 1'b1;
                fw_lane  <= 2'd0;
                state    <= S_C_WRITE_W;
            end else begin
                fw_word[fw_lane * 8 +: 8] <= rdr_byte;
            end
        end
        S_C_WRITE_W: if (nv_done) begin
            if (!nv_ok)                                fail(E_WRITE);
            else if (p_cnt == h_total_len[15:0])       state <= S_C_VF_INIT;
            else                                       state <= S_C_WRITE;
        end

        // Read the slot back and compare it, byte by byte, with the shadow bank.
        S_C_VF_INIT: begin
            rdr_start  <= 1'b1;
            run_crc    <= 32'hFFFFFFFF;
            p_cnt      <= 16'd0;
            fs_left    <= 3'd0;
            fs_pending <= 1'b0;
            fs_addr    <= target_slot ? SLOT_B : SLOT_A;
            state      <= S_C_VERIFY;
        end
        S_C_VERIFY: begin
            if (p_cnt == h_total_len[15:0]) begin
                if (~run_crc != expect_crc) fail(E_VERIFY);
                else begin
                    state <= (j_idx == 10'd512) ? S_J_ROLL : S_J_WRITE;
                    j_w   <= 3'd0;
                end
            end else if (fs_left == 3'd0) begin
                if (!fs_pending) begin
                    nv_op      <= 2'd1;
                    nv_addr    <= fs_addr;
                    nv_start   <= 1'b1;
                    fs_pending <= 1'b1;
                end else if (nv_done) begin
                    fs_pending <= 1'b0;
                    fs_addr    <= fs_addr + 1'b1;
                    if (nv_ok) begin
                        fs_word <= nv_rdata;
                        fs_left <= 3'd4;
                    end else begin
                        fail(E_VERIFY);
                    end
                end
            end else if (rdr_valid && rdr_ready) begin
                if (rdr_byte != fs_word[7:0]) begin
                    fail(E_VERIFY);
                end else begin
                    fs_word <= {8'hFF, fs_word[31:8]};
                    fs_left <= fs_left - 1'b1;
                    p_cnt   <= p_cnt + 1'b1;
                    if (p_cnt < body_len) run_crc <= run_next;
                end
            end
        end

        // The journal page is full: start on the other one.
        S_J_ROLL: begin
            nv_op    <= 2'd3;
            nv_addr  <= j_page ? JOURNAL_A : JOURNAL_B;
            nv_start <= 1'b1;
            state    <= S_J_ROLL_W;
        end
        S_J_ROLL_W: if (nv_done) begin
            if (!nv_ok) fail(E_ERASE);
            else begin
                j_page <= ~j_page;
                j_idx  <= 10'd0;
                state  <= S_J_WRITE;
            end
        end

        S_J_WRITE: begin
            nv_op    <= 2'd2;
            nv_addr  <= (j_page ? JOURNAL_B : JOURNAL_A) + {2'b00, j_idx[8:0], j_w[1:0]};
            case (j_w)
                3'd0: nv_wdata <= J_MARKER;
                3'd1: nv_wdata <= {j_seq, 7'd0, target_slot};
                3'd2: nv_wdata <= h_version;
                default: nv_wdata <= ~(J_MARKER ^ {j_seq, 7'd0, target_slot} ^ h_version);
            endcase
            nv_start <= 1'b1;
            state    <= S_J_WAIT;
        end
        S_J_WAIT: if (nv_done) begin
            if (!nv_ok) begin
                fail(E_WRITE);
            end else if (j_w == 3'd3) begin
                j_idx      <= j_idx + 1'b1;
                j_seq      <= j_seq + 1'b1;
                floor      <= h_version;
                shadow_src <= target_slot ? SRC_B : SRC_A;
                swap_pending <= 1'b1;
                state      <= S_C_SWAP;
            end else begin
                j_w   <= j_w + 1'b1;
                state <= S_J_WRITE;
            end
        end

        S_C_SWAP: if (!swap_pending) begin
            busy_commit <= 1'b0;
            w_start     <= 1'b1;
            state       <= S_IDLE;
        end

        default: state <= S_HALT;
        endcase

        // ---- register reads --------------------------------------------------
        win_tick;
        if (reg_req && reg_addr >= 16'h1000) begin
            win_start;
        end else if (reg_req) begin
            reg_valid <= 1'b1;
            casez (reg_addr)
                16'h0000: reg_data <= status_word;
                16'h0004: reg_data <= active_model_id;
                16'h0008: reg_data <= active_version;
                16'h000C: reg_data <= {30'd0, active_src};
                16'h0010: reg_data <= n_seen;
                16'h0014: reg_data <= n_blocked;
                16'h0018: reg_data <= n_errors;
                16'h001C: reg_data <= n_sig;
                16'h0020: reg_data <= {24'd0, last_error};
                16'h0024: reg_data <= {16'd0, w_offset};
                16'h0028: reg_data <= floor;
                16'h002C: reg_data <= DEVICE_ID;
                16'h0030: reg_data <= {25'd0, w_err};
                default:  reg_data <= 32'hDEAD0BAD;
            endcase
        end
    end

endmodule
