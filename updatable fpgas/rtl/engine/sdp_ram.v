// Simple dual port RAM: one write port, one read port, one clock. The read is
// synchronous (address captured on the clock edge, data valid after it).
//
// This is the only memory primitive the engine uses. On the MAX 10 it infers
// one M9K in 256 x 36 mode. On an ASIC, replace this one file with a wrapper
// around the foundry's SRAM macro; nothing above it changes.
module sdp_ram #(
    parameter DATA_W = 36,
    parameter ADDR_W = 8
) (
    input  wire              clk,
    input  wire              we,
    input  wire [ADDR_W-1:0] waddr,
    input  wire [DATA_W-1:0] wdata,
    input  wire [ADDR_W-1:0] raddr,
    output reg  [DATA_W-1:0] rdata
);
    (* ramstyle = "M9K, no_rw_check" *) reg [DATA_W-1:0] mem [0:(1 << ADDR_W) - 1];

    always @(posedge clk) begin
        if (we) mem[waddr] <= wdata;
        rdata <= mem[raddr];
    end
endmodule
