// The whole updatable IDS/IPS without any board or vendor parts: CAN front end,
// generic classifier, update logic, SPI port. The flash is reached through the
// Avalon-MM ports of the On-Chip Flash IP (see rtl/nv).
//
//   GPIO RX -> can_rx -> can_features --[fv, valid]--> rf_classifier -> ips_flag -> TX
//                                                           ^
//   SPI -> spi_slave -> cmd_decoder -> update_core ---------+   (model banks)
module updatable_core #(
    parameter [31:0] DEVICE_ID     = 32'h0000_0001,
    parameter        READ_MODEL_EN = 1,
    parameter        ROM_INIT_HEX  = "default_model.hex",
    parameter [15:0] PROTOCOL_ID   = 16'd1,
    parameter [4:0]  NV_SETTLE     = 5'd31     // testbenches shorten this
) (
    input  wire        clk,
    input  wire        rst,
    input  wire        por,
    input  wire        rxd,
    output wire        tx_dominant,
    input  wire        ips_on,
    input  wire        maint,
    input  wire        mac_ok,
    // SPI
    input  wire        spi_sclk,
    input  wire        spi_cs_n,
    input  wire        spi_mosi,
    output wire        spi_miso,
    // On-Chip Flash IP
    output wire        csr_address,
    output wire        csr_read,
    output wire        csr_write,
    output wire [31:0] csr_writedata,
    input  wire [31:0] csr_readdata,
    output wire [13:0] data_address,
    output wire        data_read,
    output wire        data_write,
    output wire [31:0] data_writedata,
    input  wire [31:0] data_readdata,
    input  wire        data_waitrequest,
    input  wire        data_readdatavalid,
    output wire [1:0]  data_burstcount,
    // for LEDs, and for the testbenches
    output wire        frame_valid,
    output wire        feat_ready,
    output wire        verdict_valid,
    output wire [2:0]  verdict,
    output wire        verdict_bank,
    output wire        flag_start,
    output wire        crc_err,
    output wire        stuff_err,
    output wire        form_err,
    output wire [10:0] can_id,
    output wire [3:0]  dlc,
    output wire [63:0] data,
    output wire [31:0] status_word,
    output wire [31:0] active_model_id,
    output wire [31:0] active_version,
    output wire        model_valid,
    output wire        active_bank
);
    // ---- CAN front end -------------------------------------------------------
    wire        before_eof, bit_edge, in_frame, id_ready, dlc_ready;
    wire [3:0]  feat_bytes;
    wire [255:0] fe_fv;
    wire [10:0] fe_key;

    can_rx rx (
        .clk(clk), .rst(rst), .rxd(rxd), .feat_bytes(feat_bytes), .in_frame(in_frame),
        .frame_valid(frame_valid), .can_id(can_id), .dlc(dlc), .data(data),
        .crc_err(crc_err), .stuff_err(stuff_err), .form_err(form_err),
        .id_ready(id_ready), .dlc_ready(dlc_ready), .feat_ready(feat_ready),
        .before_eof(before_eof), .bit_edge(bit_edge)
    );
    can_features features (.can_id(can_id), .dlc(dlc), .data(data), .feat_bytes(feat_bytes),
                           .fv(fe_fv), .key(fe_key));

    // ---- classifier: live frames first, self test frames in the gaps ---------
    wire        st_valid, out_valid, out_bank, out_tag;
    wire [255:0] st_fv;
    wire [2:0]  out_forest, out_final;

    wire live     = feat_ready && model_valid;
    wire st_go    = st_valid && !live;
    wire in_valid = live || st_go;

    wire [7:0]  tree_en_b0, tree_en_b1;
    wire [1:0]  rule_en;
    wire [2:0]  rule_from_b0, rule_from_b1, rule_to_b0, rule_to_b1;
    wire        wr_en, wr_bank, rd_req, rd_bank, rd_ack, rd_valid;
    wire [2:0]  wr_tree, rd_tree;
    wire [3:0]  wr_level, rd_level;
    wire [6:0]  wr_idx, rd_idx;
    wire [35:0] wr_data, rd_data;
    wire        kt_wr_en, kt_wr_bank, kt_wr_data, kt_rd_req, kt_rd_bank, kt_rd_ack, kt_rd_valid, kt_rd_data;
    wire [10:0] kt_wr_addr, kt_rd_addr;

    rf_classifier classifier (
        .clk(clk), .rst(rst),
        .in_valid(in_valid), .in_bank(live ? active_bank : ~active_bank), .in_tag(st_go),
        .in_fv(live ? fe_fv : st_fv), .in_key(fe_key),
        .tree_en_b0(tree_en_b0), .tree_en_b1(tree_en_b1), .rule_en(rule_en),
        .rule_from_b0(rule_from_b0), .rule_from_b1(rule_from_b1),
        .rule_to_b0(rule_to_b0), .rule_to_b1(rule_to_b1),
        .wr_en(wr_en), .wr_tree(wr_tree), .wr_level(wr_level), .wr_bank(wr_bank),
        .wr_idx(wr_idx), .wr_data(wr_data),
        .rd_req(rd_req), .rd_tree(rd_tree), .rd_level(rd_level), .rd_bank(rd_bank),
        .rd_idx(rd_idx), .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .kt_wr_en(kt_wr_en), .kt_wr_bank(kt_wr_bank), .kt_wr_addr(kt_wr_addr), .kt_wr_data(kt_wr_data),
        .kt_rd_req(kt_rd_req), .kt_rd_bank(kt_rd_bank), .kt_rd_addr(kt_rd_addr),
        .kt_rd_ack(kt_rd_ack), .kt_rd_valid(kt_rd_valid), .kt_rd_data(kt_rd_data),
        .out_valid(out_valid), .out_bank(out_bank), .out_tag(out_tag),
        .out_forest(out_forest), .out_final(out_final)
    );

    assign verdict_valid = out_valid && !out_tag;
    assign verdict       = out_final;
    assign verdict_bank  = out_bank;

    // ---- IPS ------------------------------------------------------------------
    wire [31:0] n_rej, n_ilate, n_allow, n_abort;
    ips_flag ips (
        .clk(clk), .rst(rst), .enable(ips_on),
        .verdict_valid(verdict_valid), .verdict(verdict), .can_id(can_id), .data(data),
        .before_eof(before_eof), .bit_edge(bit_edge),
        .tx_dominant(tx_dominant), .flag_start(flag_start),
        .n_rej(n_rej), .n_ilate(n_ilate), .n_allow(n_allow), .n_abort(n_abort)
    );

    // ---- update port ------------------------------------------------------------
    wire        rx_valid, frame_start, frame_end;
    wire [7:0]  rx_byte, tx_byte;
    spi_slave spi (
        .clk(clk), .rst(rst), .sclk(spi_sclk), .cs_n(spi_cs_n), .mosi(spi_mosi), .miso(spi_miso),
        .rx_valid(rx_valid), .rx_byte(rx_byte), .tx_byte(tx_byte),
        .frame_start(frame_start), .frame_end(frame_end)
    );

    wire        cmd_start_upload, cmd_byte_valid, cmd_commit, cmd_reject, reg_req, reg_valid;
    wire [7:0]  cmd_byte, cmd_reject_code;
    wire [15:0] shadow_offset, reg_addr;
    wire        shadow_ready, locked, core_busy;
    wire [31:0] cmd_commit_crc, reg_data, version_info, n_seen, n_blocked, n_errors, n_sig;

    cmd_decoder decoder (
        .clk(clk), .rst(rst), .rx_valid(rx_valid), .rx_byte(rx_byte), .tx_byte(tx_byte),
        .frame_start(frame_start), .frame_end(frame_end),
        .maint(maint), .locked(locked), .core_busy(core_busy),
        .cmd_start_upload(cmd_start_upload), .cmd_byte_valid(cmd_byte_valid), .cmd_byte(cmd_byte),
        .shadow_offset(shadow_offset), .shadow_ready(shadow_ready),
        .cmd_commit(cmd_commit), .cmd_commit_crc(cmd_commit_crc),
        .cmd_reject(cmd_reject), .cmd_reject_code(cmd_reject_code),
        .reg_req(reg_req), .reg_addr(reg_addr), .reg_valid(reg_valid), .reg_data(reg_data),
        .status_word(status_word), .active_model_id(active_model_id), .active_version(active_version),
        .version_info(version_info), .n_seen(n_seen), .n_blocked(n_blocked),
        .n_errors(n_errors), .n_sig(n_sig)
    );

    wire [1:0]  nv_op;
    wire        nv_start, nv_done, nv_ok;
    wire [13:0] nv_addr;
    wire [31:0] nv_wdata, nv_rdata;

    update_core #(.DEVICE_ID(DEVICE_ID), .PROTOCOL_ID(PROTOCOL_ID), .READ_MODEL_EN(READ_MODEL_EN),
                  .ROM_INIT_HEX(ROM_INIT_HEX)) core (
        .clk(clk), .rst(rst), .por(por),
        .cmd_start_upload(cmd_start_upload), .cmd_byte_valid(cmd_byte_valid), .cmd_byte(cmd_byte),
        .shadow_offset(shadow_offset), .shadow_ready(shadow_ready),
        .cmd_commit(cmd_commit), .cmd_commit_crc(cmd_commit_crc),
        .cmd_reject(cmd_reject), .cmd_reject_code(cmd_reject_code),
        .reg_req(reg_req), .reg_addr(reg_addr), .reg_valid(reg_valid), .reg_data(reg_data),
        .status_word(status_word), .version_info(version_info),
        .active_model_id(active_model_id), .active_version(active_version),
        .n_seen_out(n_seen), .n_blocked_out(n_blocked), .n_errors_out(n_errors), .n_sig_out(n_sig),
        .locked(locked), .busy(core_busy),
        .maint(maint), .fe_idle(!in_frame), .fe_valid(feat_ready), .count_blocked(flag_start),
        .mac_ok(mac_ok), .model_valid(model_valid), .active(active_bank), .feat_bytes(feat_bytes),
        .tree_en_b0(tree_en_b0), .tree_en_b1(tree_en_b1), .rule_en(rule_en),
        .rule_from_b0(rule_from_b0), .rule_from_b1(rule_from_b1),
        .rule_to_b0(rule_to_b0), .rule_to_b1(rule_to_b1),
        .wr_en(wr_en), .wr_tree(wr_tree), .wr_level(wr_level), .wr_bank(wr_bank),
        .wr_idx(wr_idx), .wr_data(wr_data),
        .kt_wr_en(kt_wr_en), .kt_wr_bank(kt_wr_bank), .kt_wr_addr(kt_wr_addr), .kt_wr_data(kt_wr_data),
        .rd_req(rd_req), .rd_tree(rd_tree), .rd_level(rd_level), .rd_bank(rd_bank), .rd_idx(rd_idx),
        .rd_ack(rd_ack), .rd_valid(rd_valid), .rd_data(rd_data),
        .kt_rd_req(kt_rd_req), .kt_rd_bank(kt_rd_bank), .kt_rd_addr(kt_rd_addr),
        .kt_rd_ack(kt_rd_ack), .kt_rd_valid(kt_rd_valid), .kt_rd_data(kt_rd_data),
        .st_valid(st_valid), .st_fv(st_fv), .st_accept(st_go),
        .out_valid(out_valid), .out_tag(out_tag), .out_forest(out_forest),
        .nv_op(nv_op), .nv_start(nv_start), .nv_addr(nv_addr), .nv_wdata(nv_wdata),
        .nv_done(nv_done), .nv_ok(nv_ok), .nv_rdata(nv_rdata)
    );

    nv_ufm_avmm #(.SETTLE(NV_SETTLE)) nv (
        .clk(clk), .rst(rst), .op(nv_op), .start(nv_start), .addr(nv_addr), .wdata(nv_wdata),
        .done(nv_done), .ok(nv_ok), .rdata(nv_rdata),
        .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount)
    );
endmodule
