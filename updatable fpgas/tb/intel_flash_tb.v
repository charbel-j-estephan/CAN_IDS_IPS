// The flash controller (nv_ufm_avmm.v) against Intel's own simulation model of the
// On-Chip Flash, in Questa: erase, write, read, both sectors, and an erase leaving
// other pages alone. Step 8 of run_sim.bat runs it.
`timescale 1ns/1ps
module intel_flash_tb;
  reg clk=0; always #10 clk=~clk;
  reg rst=1;
  reg [1:0] op=0; reg start=0; reg [13:0] addr=0; reg [31:0] wdata=0;
  wire done, ok; wire [31:0] rdata;
  wire csr_address, csr_read, csr_write; wire [31:0] csr_writedata, csr_readdata;
  wire [13:0] data_address; wire data_read, data_write; wire [31:0] data_writedata, data_readdata; wire data_waitrequest, data_readdatavalid; wire [1:0] data_burstcount;
  nv_ufm_avmm nv (.clk(clk),.rst(rst),.op(op),.start(start),.addr(addr),.wdata(wdata),.done(done),.ok(ok),.rdata(rdata),
   .csr_address(csr_address),.csr_read(csr_read),.csr_write(csr_write),.csr_writedata(csr_writedata),.csr_readdata(csr_readdata),
   .data_address(data_address),.data_read(data_read),.data_write(data_write),.data_writedata(data_writedata),.data_readdata(data_readdata),.data_waitrequest(data_waitrequest),.data_readdatavalid(data_readdatavalid),.data_burstcount(data_burstcount));
  nv_flash dut (.clk_clk(clk),.nreset_reset_n(~rst),.csr_address(csr_address),.csr_read(csr_read),.csr_writedata(csr_writedata),.csr_write(csr_write),.csr_readdata(csr_readdata),
   .data_address(data_address),.data_read(data_read),.data_writedata(data_writedata),.data_write(data_write),.data_readdata(data_readdata),.data_waitrequest(data_waitrequest),.data_readdatavalid(data_readdatavalid),.data_burstcount(data_burstcount));
  integer fails=0;
  task doop(input [1:0] o, input [13:0] a, input [31:0] d); begin
    @(negedge clk); op=o; addr=a; wdata=d; start=1; @(negedge clk); start=0;
    while(!done) @(negedge clk);
    $display("t=%0t op %0d addr %0d ok=%b rdata=%h", $time, o, a, ok, rdata);
  end endtask
  task rd(input [13:0] a, input [31:0] want); begin doop(1,a,0); if(rdata!==want||ok!==1) begin $display("FAIL read %0d got %h want %h",a,rdata,want); fails=fails+1; end end endtask
  initial begin
    repeat(20) @(posedge clk); rst=0; repeat(200) @(posedge clk);
    doop(3, 14'd0, 0); if(ok!==1) begin $display("FAIL erase A"); fails=fails+1; end
    rd(0,32'hFFFFFFFF);
    doop(2, 14'd0, 32'h12345678); if(ok!==1) begin $display("FAIL write"); fails=fails+1; end
    doop(2, 14'd1, 32'hA5A55A5A); if(ok!==1) begin $display("FAIL write2"); fails=fails+1; end
    rd(0,32'h12345678); rd(1,32'hA5A55A5A);
    doop(3, 14'd8192, 0); if(ok!==1) begin $display("FAIL erase B"); fails=fails+1; end
    doop(2, 14'd8192, 32'hCAFEF00D); if(ok!==1) begin $display("FAIL write B"); fails=fails+1; end
    rd(8192,32'hCAFEF00D);
    rd(0,32'h12345678);
    doop(3, 14'd2048, 0);
    rd(0,32'h12345678);
    if(fails==0) $display("ALL PASS"); else $display("FAILS %0d",fails);
    $finish;
  end
  initial begin #2000000000; $display("TIMEOUT"); $finish; end
endmodule
