// The flash controller (rtl/nv/nv_ufm_avmm.v) when the flash stops answering:
// an erase or write whose busy bit never clears, and a read or write that is
// never accepted (waitrequest stuck high). Each must end in exactly one done
// with ok = 0, the request lines down, the sector protected again, and the
// next operation must work once the flash answers again.
//
//   iverilog -g2005 -o build/nv_timeout_sim tb/nv_timeout_tb.v tb/ufm_model.v rtl/nv/nv_ufm_avmm.v
//   vvp -n build/nv_timeout_sim
`timescale 1ns/1ps
module nv_timeout_tb;
    localparam TIMEOUT = 2000;
    localparam [31:0] CTRL_LOCK = {4'hF, 5'b11111, 3'b111, 20'hFFFFF};

    reg clk = 0;
    always #10 clk = ~clk;

    reg         rst = 1, start = 0;
    reg  [1:0]  op = 0;
    reg  [13:0] addr = 0;
    reg  [31:0] wdata = 0;
    wire        done, ok;
    wire [31:0] rdata;

    wire        csr_address, csr_read, csr_write, data_read, data_write, data_waitrequest, data_readdatavalid;
    wire [31:0] csr_writedata, csr_readdata, data_writedata, data_readdata;
    wire [13:0] data_address;
    wire [1:0]  data_burstcount;

    nv_ufm_avmm #(.TIMEOUT(TIMEOUT), .SETTLE(5'd6)) dut (
        .clk(clk), .rst(rst), .op(op), .start(start), .addr(addr), .wdata(wdata),
        .done(done), .ok(ok), .rdata(rdata),
        .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount)
    );

    ufm_model #(.WRITE_CLKS(6), .ERASE_CLKS(150)) ufm (
        .clk(clk), .csr_address(csr_address), .csr_read(csr_read), .csr_write(csr_write),
        .csr_writedata(csr_writedata), .csr_readdata(csr_readdata),
        .data_address(data_address), .data_read(data_read), .data_write(data_write),
        .data_writedata(data_writedata), .data_readdata(data_readdata),
        .data_waitrequest(data_waitrequest), .data_readdatavalid(data_readdatavalid),
        .data_burstcount(data_burstcount), .power_cut(1'b0)
    );

    integer checks = 0, fails = 0;
    task check;
        input [8*60:1] name;
        input          cond;
        begin
            checks = checks + 1;
            if (cond) $display("PASS %0s", name);
            else begin $display("FAIL %0s", name); fails = fails + 1; end
        end
    endtask

    // Count every done pulse, so a second one would show up.
    integer n_done = 0;
    always @(negedge clk) if (done) n_done = n_done + 1;

    // One operation. Waits for done or gives up after `limit` clocks.
    reg got_done, got_ok;
    reg [31:0] got_rdata;
    task doop;
        input [1:0]  o;
        input [13:0] a;
        input [31:0] d;
        input integer limit;
        integer t;
        begin
            @(negedge clk);
            op = o; addr = a; wdata = d; start = 1;
            @(negedge clk);
            start = 0;
            got_done = 0;
            t = 0;
            while (!got_done && t < limit) begin
                @(posedge clk); #1;
                if (done) begin got_done = 1; got_ok = ok; got_rdata = rdata; end
                t = t + 1;
            end
            @(negedge clk);          // let the done counter see this pulse first
        end
    endtask

    integer before;
    initial begin
        repeat (5) @(posedge clk);
        rst = 0;
        repeat (5) @(posedge clk);

        // Sanity: the controller works when the flash answers.
        doop(3, 14'd0, 0, 10000);
        check("normal erase", got_done && got_ok);
        doop(2, 14'd5, 32'h12345678, 10000);
        check("normal write", got_done && got_ok);
        doop(1, 14'd5, 0, 10000);
        check("normal read", got_done && got_ok && got_rdata == 32'h12345678);

        // 1. An erase that never finishes (busy stuck).
        ufm.stuck_busy = 1;
        before = n_done;
        doop(3, 14'd2048, 0, TIMEOUT + 200);
        check("stuck erase: done comes", got_done);
        check("stuck erase: ok = 0", got_done && !got_ok);
        repeat (200) @(posedge clk); #1;
        check("stuck erase: exactly one done", n_done == before + 1);
        check("stuck erase: controller idle", dut.state == 0);
        check("stuck erase: lock written last", csr_writedata == CTRL_LOCK);
        check("stuck erase: request lines down", !data_read && !data_write);
        ufm.stuck_busy = 0;
        repeat (400) @(posedge clk);
        doop(1, 14'd5, 0, 10000);
        check("after stuck erase: next read works", got_done && got_ok && got_rdata == 32'h12345678);

        // 2. A write that is never accepted (waitrequest stuck).
        ufm.stuck_wait = 1;
        before = n_done;
        doop(2, 14'd6, 32'hCAFEF00D, TIMEOUT + 200);
        check("stuck write: done comes", got_done);
        check("stuck write: ok = 0", got_done && !got_ok);
        repeat (200) @(posedge clk); #1;
        check("stuck write: exactly one done", n_done == before + 1);
        check("stuck write: data_write down", !data_write);
        check("stuck write: controller idle", dut.state == 0);
        check("stuck write: lock written last", csr_writedata == CTRL_LOCK);

        // 3. A read that is never accepted.
        before = n_done;
        doop(1, 14'd5, 0, TIMEOUT + 200);
        check("stuck read: done comes", got_done);
        check("stuck read: ok = 0", got_done && !got_ok);
        repeat (200) @(posedge clk); #1;
        check("stuck read: exactly one done", n_done == before + 1);
        check("stuck read: data_read down", !data_read);
        check("stuck read: controller idle", dut.state == 0);
        ufm.stuck_wait = 0;
        repeat (20) @(posedge clk);

        // 4. Everything works again.
        doop(2, 14'd7, 32'h0BADF00D, 10000);
        check("after stuck read/write: write works", got_done && got_ok);
        doop(1, 14'd7, 0, 10000);
        check("after stuck read/write: read works", got_done && got_ok && got_rdata == 32'h0BADF00D);

        $display("%0d checks, %0d failed", checks, fails);
        if (fails == 0) $display("ALL PASS");
        $finish;
    end

    initial begin #20000000; $display("FAIL TIMEOUT of the testbench"); $finish; end
endmodule
