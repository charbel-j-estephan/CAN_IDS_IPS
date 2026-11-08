// Non volatile storage back end for the MAX 10: talks to Altera's On-Chip Flash
// IP over its Avalon-MM data and control ports, in the order Intel's own driver
// uses (HAL altera_onchip_flash.c): clear a sector's write protect, do the
// operation, poll the status register, set the protect again.
//
// The rest of the design only sees the small op interface, so an ASIC replaces
// this one file with a controller for an external SPI flash.
//
//   op 1 read   2 write   3 erase the page holding addr
//   start: 1 clock with op, addr, wdata. done: 1 clock later, with ok and rdata.
module nv_ufm_avmm #(
    parameter ADDR_W       = 14,        // 32 bit words
    parameter SECTOR2_BASE = 8192,
    parameter TIMEOUT      = 32'd40_000_000,  // clocks, longer than any flash operation
    parameter SETTLE       = 5'd31            // clocks to wait before polling status
) (
    input  wire              clk,
    input  wire              rst,
    input  wire [1:0]        op,
    input  wire              start,
    input  wire [ADDR_W-1:0] addr,
    input  wire [31:0]       wdata,
    output reg               done,
    output reg               ok,
    output reg  [31:0]       rdata,
    // to the On-Chip Flash IP
    output reg               csr_address,
    output reg               csr_read,
    output reg               csr_write,
    output reg  [31:0]       csr_writedata,
    input  wire [31:0]       csr_readdata,
    output reg  [ADDR_W-1:0] data_address,
    output reg               data_read,
    output reg               data_write,
    output reg  [31:0]       data_writedata,
    input  wire [31:0]       data_readdata,
    input  wire              data_waitrequest,
    input  wire              data_readdatavalid,
    output wire [1:0]        data_burstcount
);
    assign data_burstcount = 2'd1;

    localparam OP_READ = 2'd1, OP_WRITE = 2'd2, OP_ERASE = 2'd3;

    localparam S_IDLE     = 4'd0,  S_R_REQ   = 4'd1,  S_R_WAIT   = 4'd2,
               S_UNPROT   = 4'd3,  S_W_REQ   = 4'd4,  S_E_REQ    = 4'd5,
               S_SETTLE   = 4'd6,  S_POLL_R  = 4'd7,  S_POLL_D   = 4'd8,
               S_PROT     = 4'd9,  S_FINISH  = 4'd10;

    reg [3:0]  state;
    reg [1:0]  op_r;
    reg [ADDR_W-1:0] addr_r;
    reg [31:0] wdata_r;
    reg [31:0] timer;
    reg [4:0]  settle;
    reg        pass;

    // Control register fields (Intel's altera_onchip_flash_regs.h):
    //   [19:0] page erase address (words), [22:20] sector erase, 7 = none,
    //   [27:23] write protect per sector, 1 = protected.
    wire        in_sector2 = (addr_r >= SECTOR2_BASE);
    wire [4:0]  wp_open    = in_sector2 ? 5'b11101 : 5'b11110;
    wire [31:0] ctrl_open  = {4'hF, wp_open, 3'b111, 20'hFFFFF};
    wire [31:0] ctrl_erase = {4'hF, wp_open, 3'b111, {20-ADDR_W{1'b0}}, addr_r};
    wire [31:0] ctrl_lock  = {4'hF, 5'b11111, 3'b111, 20'hFFFFF};

    wire [1:0]  status_busy = csr_readdata[1:0];
    wire        status_wr_ok = csr_readdata[3];
    wire        status_er_ok = csr_readdata[4];

    always @(posedge clk) begin
        done <= 1'b0;
        if (rst) begin
            state      <= S_IDLE;
            csr_read   <= 1'b0;
            csr_write  <= 1'b0;
            data_read  <= 1'b0;
            data_write <= 1'b0;
        end else begin
            csr_write <= 1'b0;
            csr_read  <= 1'b0;
            if (state != S_IDLE) begin
                timer <= timer + 1'b1;
                if (timer == TIMEOUT) begin
                    ok         <= 1'b0;
                    done       <= 1'b1;
                    state      <= S_IDLE;
                    data_read  <= 1'b0;
                    data_write <= 1'b0;
                end
            end
            case (state)
            S_IDLE: if (start) begin
                op_r    <= op;
                addr_r  <= addr;
                wdata_r <= wdata;
                timer   <= 32'd0;
                state   <= (op == OP_READ) ? S_R_REQ : S_UNPROT;
            end

            S_R_REQ: begin
                data_address <= addr_r;
                data_read    <= 1'b1;
                if (data_read && !data_waitrequest) begin
                    data_read <= 1'b0;
                    state     <= S_R_WAIT;
                end
            end
            S_R_WAIT: if (data_readdatavalid) begin
                rdata <= data_readdata;
                ok    <= 1'b1;
                done  <= 1'b1;
                state <= S_IDLE;
            end

            S_UNPROT: begin
                csr_address   <= 1'b1;
                csr_write     <= 1'b1;
                csr_writedata <= ctrl_open;
                state         <= (op_r == OP_WRITE) ? S_W_REQ : S_E_REQ;
            end

            S_W_REQ: begin
                data_address   <= addr_r;
                data_writedata <= wdata_r;
                data_write     <= 1'b1;
                if (data_write && !data_waitrequest) begin
                    data_write <= 1'b0;
                    settle     <= 5'd0;
                    state      <= S_SETTLE;
                end
            end

            S_E_REQ: begin
                csr_address   <= 1'b1;
                csr_write     <= 1'b1;
                csr_writedata <= ctrl_erase;
                settle        <= 5'd0;
                state         <= S_SETTLE;
            end

            // Give the controller time to raise busy before polling it.
            S_SETTLE: begin
                settle <= settle + 1'b1;
                if (settle == SETTLE) state <= S_POLL_R;
            end

            S_POLL_R: begin
                csr_address <= 1'b0;
                csr_read    <= 1'b1;
                state       <= S_POLL_D;
                settle      <= 5'd0;
            end
            S_POLL_D: begin
                settle <= settle + 1'b1;
                if (settle == 5'd1) begin          // the status is on the bus by now
                    if (status_busy != 2'd0) begin
                        state <= S_POLL_R;
                    end else begin
                        pass  <= (op_r == OP_WRITE) ? status_wr_ok : status_er_ok;
                        state <= S_PROT;
                    end
                end
            end

            S_PROT: begin
                csr_address   <= 1'b1;
                csr_write     <= 1'b1;
                csr_writedata <= ctrl_lock;
                state         <= S_FINISH;
            end

            S_FINISH: begin
                ok    <= pass;
                done  <= 1'b1;
                state <= S_IDLE;
            end

            default: state <= S_IDLE;
            endcase
        end
    end
endmodule
