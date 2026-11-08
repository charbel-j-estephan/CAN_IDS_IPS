// Behavioural model of Altera's On-Chip Flash IP as this design uses it (same
// Avalon-MM ports as the generated nv_flash.v): 16384 words of 32 bits, erased
// = all ones, program can only clear bits, a word cannot be written while busy,
// write protect bits per sector, status register as the real one.
//
// Times are shorter than the real part (350 ms erase, 305 us write) so a
// simulation finishes; the order of events is what is being tested.
//
// power_cut: while high the flash does nothing. If it rises during a write or
// an erase, that word or page is left with garbage, as a real interrupted
// operation can leave it.
//
// It cannot prove how the real flash behaves. That needs the board.
`timescale 1ns/1ps
module ufm_model #(
    parameter WRITE_CLKS = 30,
    parameter ERASE_CLKS = 400,
    parameter READ_CLKS  = 2
) (
    input  wire        clk,
    input  wire        csr_address,
    input  wire        csr_read,
    input  wire        csr_write,
    input  wire [31:0] csr_writedata,
    output reg  [31:0] csr_readdata = 32'd0,
    input  wire [13:0] data_address,
    input  wire        data_read,
    input  wire        data_write,
    input  wire [31:0] data_writedata,
    output reg  [31:0] data_readdata = 32'd0,
    output reg         data_waitrequest = 1'b1,
    output reg         data_readdatavalid = 1'b0,
    input  wire [1:0]  data_burstcount,
    input  wire        power_cut
);
    reg [31:0] mem [0:16383];
    reg [4:0]  wp = 5'b11111;
    reg [1:0]  busy = 2'd0;       // 0 idle, 1 erase, 2 write, 3 read
    reg        r_pass = 0, w_pass = 0, e_pass = 0;
    integer    timer = 0;
    reg [13:0] op_addr;
    reg [31:0] op_data;
    reg [13:0] rd_addr;
    integer    rd_wait = -1;
    integer    i;

    // How many writes/erases to let through before the power goes (-1 = never).
    integer cut_after = -1;
    integer ops_done  = 0;
    reg     cut_now   = 1'b0;

    initial for (i = 0; i < 16384; i = i + 1) mem[i] = 32'hFFFFFFFF;

    task erase_page;
        input [13:0] a;
        reg [13:0] base;
        integer j;
        begin
            base = {a[13:11], 11'd0};
            for (j = 0; j < 2048; j = j + 1) mem[base + j] = 32'hFFFFFFFF;
        end
    endtask

    wire [31:0] status = {22'h3FFFFF, 5'b11100, e_pass, w_pass, r_pass, busy};

    always @(posedge clk) begin
        csr_readdata <= csr_address ? {2'b11, 2'b11, wp, 23'h7FFFFF} : status;
        data_readdatavalid <= 1'b0;

        if (power_cut) begin
            // An operation in progress is left half done.
            if (busy == 2'd2) mem[op_addr] = mem[op_addr] & (op_data | 32'hA5A5F00F);
            if (busy == 2'd1) begin
                for (i = 0; i < 2048; i = i + 1)
                    if (i % 3 != 0) mem[{op_addr[13:11], 11'd0} + i] = 32'hFFFF0000 ^ i;
            end
            busy             <= 2'd0;
            data_waitrequest <= 1'b1;
            rd_wait          <= -1;
        end else begin
            data_waitrequest <= 1'b1;

            if (busy != 2'd0) begin
                timer <= timer - 1;
                if (timer == 0) begin
                    if (busy == 2'd2) begin
                        mem[op_addr] = mem[op_addr] & op_data;
                        w_pass <= 1'b1;
                    end else if (busy == 2'd1) begin
                        erase_page(op_addr);
                        e_pass <= 1'b1;
                    end
                    busy <= 2'd0;
                end
            end else begin
                if (csr_write && csr_address) begin
                    wp <= csr_writedata[27:23];
                    if (csr_writedata[19:0] != 20'hFFFFF) begin
                        op_addr = csr_writedata[13:0];
                        if ((op_addr < 8192 && !csr_writedata[23]) ||
                            (op_addr >= 8192 && !csr_writedata[24])) begin
                            busy   <= 2'd1;
                            timer  <= ERASE_CLKS;
                            e_pass <= 1'b0;
                            ops_done = ops_done + 1;
                        end else begin
                            e_pass <= 1'b0;
                        end
                    end
                end
                if (data_write && data_waitrequest) begin
                    data_waitrequest <= 1'b0;
                    if ((data_address < 8192 && !wp[0]) || (data_address >= 8192 && !wp[1])) begin
                        op_addr = data_address;
                        op_data = data_writedata;
                        busy   <= 2'd2;
                        timer  <= WRITE_CLKS;
                        w_pass <= 1'b0;
                        ops_done = ops_done + 1;
                    end else begin
                        w_pass <= 1'b0;
                    end
                end
                if (data_read && data_waitrequest && rd_wait < 0) begin
                    data_waitrequest <= 1'b0;
                    rd_addr <= data_address;
                    rd_wait <= READ_CLKS;
                end
            end
            if (rd_wait == 0) begin
                data_readdata      <= mem[rd_addr];
                data_readdatavalid <= 1'b1;
                r_pass             <= 1'b1;
            end
            if (rd_wait >= 0) rd_wait <= rd_wait - 1;
        end
    end

    // A test can ask for the power to fail after the N-th write or erase starts.
    always @(posedge clk) begin
        cut_now <= (cut_after >= 0) && (ops_done > cut_after);
    end
endmodule
