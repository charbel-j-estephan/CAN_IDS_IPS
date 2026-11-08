// SPI commands. One command per chip select. Multi byte fields are big endian.
//
//   WRITE_SHADOW  01  addr(2)  len(2)  len data bytes
//   READ          02  addr(2)  dummy   data(4)
//   COMMIT        03  crc(4)
//   STATUS        04  dummy    status(4)
//   GET_VERSION   05  dummy    model_id(4) version(4) info(4)
//   READ_COUNTERS 06  dummy    seen(4) blocked(4) errors(4) sig_fails(4)
//
// STATUS and GET_VERSION always work. Everything else needs the maintenance
// switch on. A locked out port also refuses WRITE_SHADOW and COMMIT; the read
// commands still work so the board can be examined. A refused command is
// counted and its remaining bytes are ignored. WRITE_SHADOW must be in order: address 0 starts
// a new upload, any other address must equal the bytes received so far.
module cmd_decoder (
    input  wire        clk,
    input  wire        rst,
    input  wire        rx_valid,
    input  wire [7:0]  rx_byte,
    output reg  [7:0]  tx_byte,
    input  wire        frame_start,
    input  wire        frame_end,
    input  wire        maint,
    input  wire        locked,
    input  wire        core_busy,
    // upload
    output reg         cmd_start_upload,
    output reg         cmd_byte_valid,
    output reg  [7:0]  cmd_byte,
    input  wire [15:0] shadow_offset,
    input  wire        shadow_ready,
    output reg         cmd_commit,
    output reg  [31:0] cmd_commit_crc,
    output reg         cmd_reject,
    output reg  [7:0]  cmd_reject_code,
    // registers
    output reg         reg_req,
    output reg  [15:0] reg_addr,
    input  wire        reg_valid,
    input  wire [31:0] reg_data,
    input  wire [31:0] status_word,
    input  wire [31:0] active_model_id,
    input  wire [31:0] active_version,
    input  wire [31:0] version_info,
    input  wire [31:0] n_seen,
    input  wire [31:0] n_blocked,
    input  wire [31:0] n_errors,
    input  wire [31:0] n_sig
);
    localparam [7:0] C_WRITE = 8'h01, C_READ = 8'h02, C_COMMIT = 8'h03,
                     C_STATUS = 8'h04, C_VERSION = 8'h05, C_COUNTERS = 8'h06;
    localparam [7:0] E_NOT_MAINT = 1, E_LOCKED = 2, E_BUSY = 16, E_SEQUENCE = 17;

    localparam P_CMD = 3'd0, P_ARGS = 3'd1, P_DATA = 3'd2, P_DUMMY = 3'd3, P_RESP = 3'd4, P_IGNORE = 3'd5;

    reg [2:0]   phase;
    reg [7:0]   cmd;
    reg [2:0]   arg_i;
    reg [31:0]  args;
    reg [15:0]  data_left;
    reg [127:0] resp;
    reg [4:0]   resp_left;

    wire [31:0] args_n = {args[23:0], rx_byte};

    always @(posedge clk) begin
        cmd_start_upload <= 1'b0;
        cmd_byte_valid   <= 1'b0;
        cmd_commit       <= 1'b0;
        cmd_reject       <= 1'b0;
        reg_req          <= 1'b0;

        if (rst || frame_end) begin
            phase   <= P_CMD;
            tx_byte <= 8'hFF;
        end else if (frame_start) begin
            phase   <= P_CMD;
            arg_i   <= 3'd0;
            tx_byte <= 8'hFF;
        end else if (rx_valid) begin
            case (phase)
            P_CMD: begin
                cmd   <= rx_byte;
                arg_i <= 3'd0;
                tx_byte <= 8'hFF;
                if (rx_byte == C_STATUS || rx_byte == C_VERSION) begin
                    phase <= P_DUMMY;
                end else if (!maint) begin
                    cmd_reject      <= 1'b1;
                    cmd_reject_code <= E_NOT_MAINT;
                    phase           <= P_IGNORE;
                end else if (rx_byte == C_COUNTERS) begin
                    phase <= P_DUMMY;
                end else if (rx_byte == C_READ) begin
                    phase <= P_ARGS;
                end else if (locked && (rx_byte == C_WRITE || rx_byte == C_COMMIT)) begin
                    cmd_reject      <= 1'b1;
                    cmd_reject_code <= E_LOCKED;
                    phase           <= P_IGNORE;
                end else if (rx_byte == C_WRITE || rx_byte == C_COMMIT) begin
                    phase <= P_ARGS;
                end else begin
                    phase <= P_IGNORE;
                end
            end

            P_ARGS: begin
                args  <= args_n;
                arg_i <= arg_i + 1'b1;
                if (cmd == C_READ && arg_i == 3'd1) begin
                    reg_addr <= args_n[15:0];
                    reg_req  <= 1'b1;
                    phase    <= P_DUMMY;
                end else if (cmd == C_COMMIT && arg_i == 3'd3) begin
                    if (core_busy) begin
                        cmd_reject      <= 1'b1;
                        cmd_reject_code <= E_BUSY;
                    end else begin
                        cmd_commit     <= 1'b1;
                        cmd_commit_crc <= args_n;
                    end
                    phase <= P_IGNORE;
                end else if (cmd == C_WRITE && arg_i == 3'd3) begin
                    data_left <= args_n[15:0];
                    if (core_busy) begin
                        cmd_reject      <= 1'b1;
                        cmd_reject_code <= E_BUSY;
                        phase           <= P_IGNORE;
                    end else if (args_n[31:16] == 16'd0) begin
                        cmd_start_upload <= 1'b1;
                        phase            <= P_DATA;
                    end else if (args_n[31:16] == shadow_offset) begin
                        phase <= P_DATA;
                    end else begin
                        cmd_reject      <= 1'b1;
                        cmd_reject_code <= E_SEQUENCE;
                        phase           <= P_IGNORE;
                    end
                end
            end

            P_DATA: begin
                if (data_left != 16'd0) begin
                    if (shadow_ready) begin
                        cmd_byte_valid <= 1'b1;
                        cmd_byte       <= rx_byte;
                    end else begin
                        cmd_reject      <= 1'b1;
                        cmd_reject_code <= E_SEQUENCE;
                    end
                    data_left <= data_left - 1'b1;
                end
            end

            // The dummy byte gives the register read time to finish.
            P_DUMMY: begin
                phase <= P_RESP;
                case (cmd)
                    C_STATUS:   begin resp <= {status_word, 96'd0};                        resp_left <= 5'd3;  tx_byte <= status_word[31:24]; end
                    C_VERSION:  begin resp <= {active_model_id, active_version, version_info, 32'd0}; resp_left <= 5'd11; tx_byte <= active_model_id[31:24]; end
                    C_COUNTERS: begin resp <= {n_seen, n_blocked, n_errors, n_sig};        resp_left <= 5'd15; tx_byte <= n_seen[31:24]; end
                    default:    begin resp <= {reg_data, 96'd0};                           resp_left <= 5'd3;  tx_byte <= reg_data[31:24]; end
                endcase
            end

            P_RESP: begin
                if (resp_left != 5'd0) begin
                    tx_byte   <= resp[119:112];
                    resp      <= {resp[119:0], 8'hFF};
                    resp_left <= resp_left - 1'b1;
                end else begin
                    tx_byte <= 8'hFF;
                end
            end

            default: tx_byte <= 8'hFF;
            endcase
        end
    end
endmodule
